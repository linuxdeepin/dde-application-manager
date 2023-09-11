// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopentry.h"
#include "global.h"
#include <QFileInfo>
#include <QDir>
#include <algorithm>
#include <QRegularExpression>
#include <QDirIterator>
#include <QStringView>
#include <QVariant>
#include <iostream>
#include <chrono>
#include <cstdio>

namespace {
bool isInvalidLocaleString(const QString &str) noexcept
{
    constexpr auto Language = R"((?:[a-z]+))";        // language of locale postfix. eg.(en, zh)
    constexpr auto Country = R"((?:_[A-Z]+))";        // country of locale postfix. eg.(US, CN)
    constexpr auto Encoding = R"((?:\.[0-9A-Z-]+))";  // encoding of locale postfix. eg.(UFT-8)
    constexpr auto Modifier = R"((?:@[a-zA-Z=;]+))";  // modifier of locale postfix. eg.(euro;collation=traditional)
    const static auto validKey = QString(R"(^%1%2?%3?%4?$)").arg(Language, Country, Encoding, Modifier);
    // example: https://regex101.com/r/hylOay/2
    static const QRegularExpression _re = []() -> QRegularExpression {
        QRegularExpression tmp{validKey};
        tmp.optimize();
        return tmp;
    }();
    thread_local const auto re = _re;

    return re.match(str).hasMatch();
}

bool hasNonAsciiAndControlCharacters(const QString &str) noexcept
{
    static const QRegularExpression _matchControlChars = []() {
        QRegularExpression tmp{R"(\p{Cc})"};
        tmp.optimize();
        return tmp;
    }();
    thread_local const auto matchControlChars = _matchControlChars;
    static const QRegularExpression _matchNonAsciiChars = []() {
        QRegularExpression tmp{R"([^\x00-\x7f])"};
        tmp.optimize();
        return tmp;
    }();
    thread_local const auto matchNonAsciiChars = _matchNonAsciiChars;
    if (str.contains(matchControlChars) and str.contains(matchNonAsciiChars)) {
        return true;
    }

    return false;
}

struct Parser
{
    Parser(QTextStream &stream)
        : m_stream(stream){};
    QTextStream &m_stream;
    QString m_line;

    using Groups = QMap<QString, QMap<QString, DesktopEntry::Value>>;

    DesktopErrorCode parse(Groups &groups) noexcept;

private:
    void skip() noexcept;
    DesktopErrorCode addGroup(Groups &groups) noexcept;
    DesktopErrorCode addEntry(Groups::iterator &group) noexcept;
};

void Parser::skip() noexcept
{
    while (!m_stream.atEnd() and (m_line.startsWith('#') or m_line.isEmpty())) {
        m_line = m_stream.readLine().trimmed();
    }
}

DesktopErrorCode Parser::parse(Groups &ret) noexcept
{
    std::remove_reference_t<decltype(ret)> groups;
    while (!m_stream.atEnd()) {
        auto err = addGroup(groups);
        if (err != DesktopErrorCode::NoError) {
            return err;
        }

        if (groups.size() != 1) {
            continue;
        }

        if (groups.keys().first() != DesktopFileEntryKey) {
            qWarning() << "There should be nothing preceding "
                          "'Desktop Entry' group in the desktop entry file "
                          "but possibly one or more comments.";
            return DesktopErrorCode::InvalidFormat;
        }
    }

    if (!m_line.isEmpty()) {
        qCritical() << "Something is wrong in Desktop file parser, check logic.";
        return DesktopErrorCode::InternalError;
    }

    ret = std::move(groups);
    return DesktopErrorCode::NoError;
}

DesktopErrorCode Parser::addGroup(Groups &ret) noexcept
{
    skip();
    if (!m_line.startsWith('[')) {
        qWarning() << "Invalid desktop file format: unexpected line:" << m_line;
        return DesktopErrorCode::InvalidFormat;
    }

    // Parsing group header.
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header

    auto groupHeader = m_line.sliced(1, m_line.size() - 2).trimmed();

    if (groupHeader.contains('[') || groupHeader.contains(']') || hasNonAsciiAndControlCharacters(groupHeader)) {
        qWarning() << "group header invalid:" << m_line;
        return DesktopErrorCode::InvalidFormat;
    }

    if (ret.find(groupHeader) != ret.end()) {
        qWarning() << "duplicated group header detected:" << groupHeader;
        return DesktopErrorCode::InvalidFormat;
    }

    auto group = ret.insert(groupHeader, {});

    m_line.clear();
    while (!m_stream.atEnd() && !m_line.startsWith('[')) {
        skip();
        if (m_line.startsWith('[')) {
            break;
        }
        auto err = addEntry(group);
        if (err != DesktopErrorCode::NoError) {
            return err;
        }
    }
    return DesktopErrorCode::NoError;
}

DesktopErrorCode Parser::addEntry(Groups::iterator &group) noexcept
{
    auto line = m_line;
    m_line.clear();
    auto splitCharIndex = line.indexOf('=');
    if (splitCharIndex == -1) {
        qWarning() << "invalid line in desktop file, skip it:" << line;
        return DesktopErrorCode::NoError;
    }
    auto keyStr = line.first(splitCharIndex).trimmed();
    auto valueStr = line.sliced(splitCharIndex + 1).trimmed();

    QString key{""};
    QString localeStr{defaultKeyStr};
    // NOTE:
    // We are process "localized keys" here, for usage check:
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#localized-keys
    qsizetype localeBegin = keyStr.indexOf('[');
    qsizetype localeEnd = keyStr.lastIndexOf(']');
    if ((localeBegin == -1 && localeEnd != -1) || (localeBegin != -1 && localeEnd == -1)) {
        qWarning() << "unmatched [] detected in desktop file, skip this line: " << line;
        return DesktopErrorCode::NoError;
    }

    if (localeBegin == -1 && localeEnd == -1) {
        key = keyStr;
    } else {
        key = keyStr.sliced(0, localeBegin);
        localeStr = keyStr.sliced(localeBegin + 1, localeEnd - localeBegin - 1);  // strip '[' and ']'
    }

    static const QRegularExpression _re = []() {
        QRegularExpression tmp{"R([^A-Za-z0-9-])"};
        tmp.optimize();
        return tmp;
    }();
    // NOTE: https://stackoverflow.com/a/25583104
    thread_local const QRegularExpression re = _re;
    if (re.match(key).hasMatch()) {
        qWarning() << "invalid key name, skip this line:" << line;
        return DesktopErrorCode::NoError;
    }

    if (localeStr != defaultKeyStr && !isInvalidLocaleString(localeStr)) {
        qWarning().noquote() << QString("invalid LOCALE (%2) for key \"%1\"").arg(key, localeStr);
    }

    auto keyIt = group->find(key);
    if (keyIt != group->end() && keyIt->find(localeStr) != keyIt->end()) {
        qWarning() << "duplicated localestring, skip this line:" << line;
        return DesktopErrorCode::NoError;
    }

    if (keyIt == group->end()) {
        group->insert(key, {{localeStr, valueStr}});
        return DesktopErrorCode::NoError;
    }

    keyIt->insert(localeStr, valueStr);
    return DesktopErrorCode::NoError;
}

}  // namespace

QString DesktopFile::sourcePath() const noexcept
{
    if (!m_fileSource) {
        return "";
    }

    QFileInfo info(*m_fileSource);
    return info.absoluteFilePath();
}

bool DesktopEntry::checkMainEntryValidation() const noexcept
{
    auto it = m_entryMap.find(DesktopFileEntryKey);
    if (it == m_entryMap.end()) {
        return false;
    }

    if (auto name = it->find("Name"); name == it->end()) {
        qWarning() << "No Name.";
        return false;
    }

    auto type = it->find("Type");
    if (type == it->end()) {
        qWarning() << "No Type.";
        for (auto tmp = it->constKeyValueBegin(); tmp != it->constKeyValueEnd(); ++tmp) {
            const auto &[k, v] = *tmp;
            qInfo() << "keyName:" << k;
            for (auto valIt = v.constKeyValueBegin(); valIt != v.constKeyValueEnd(); ++valIt) {
                const auto &[key, value] = *valIt;
                qInfo() << key << value;
            }
        }
        return false;
    }

    const auto &typeStr = *type->find(defaultKeyStr);

    if (typeStr == "Link") {
        if (it->find("URL") == it->end()) {
            return false;
        }
    }

    return true;
}

std::optional<DesktopFile> DesktopFile::createTemporaryDesktopFile(std::unique_ptr<QFile> temporaryFile) noexcept
{
    auto [ctime, mtime, _] = getFileTimeInfo(QFileInfo{*temporaryFile});
    if (mtime == 0) {
        qWarning() << "create temporary file failed.";
        return std::nullopt;
    }
    return DesktopFile{std::move(temporaryFile), "", mtime, ctime};
}

std::optional<DesktopFile> DesktopFile::createTemporaryDesktopFile(const QString &temporaryFile) noexcept
{
    const static QString userTmp = QString{"/run/user/%1/"}.arg(getCurrentUID());
    auto tempFile = std::make_unique<QFile>(QString{userTmp + QUuid::createUuid().toString(QUuid::Id128) + ".desktop"});

    if (!tempFile->open(QFile::NewOnly | QFile::WriteOnly | QFile::Text)) {
        qWarning() << "failed to create temporary desktop file:" << QFileInfo{*tempFile}.absoluteFilePath()
                   << tempFile->errorString();
        return std::nullopt;
    }

    auto content = temporaryFile.toLocal8Bit();
    auto writeByte = tempFile->write(content);

    if (writeByte == -1 || writeByte != content.length()) {
        qWarning() << "write to temporary file failed:" << tempFile->errorString();
        return std::nullopt;
    }

    tempFile->close();

    return createTemporaryDesktopFile(std::move(tempFile));
}

std::optional<DesktopFile> DesktopFile::searchDesktopFileByPath(const QString &desktopFile, DesktopErrorCode &err) noexcept
{
    decltype(auto) desktopSuffix = ".desktop";

    if (!desktopFile.endsWith(desktopSuffix)) {
        qWarning() << "file isn't a desktop file:" << desktopFile;
        err = DesktopErrorCode::MismatchedFile;
        return std::nullopt;
    }

    QFileInfo fileinfo{desktopFile};
    if (!fileinfo.isAbsolute() or !fileinfo.exists()) {
        qWarning() << "desktop file not found.";
        err = DesktopErrorCode::NotFound;
        return std::nullopt;
    }

    QString path{desktopFile};
    QString id;

    const auto &XDGDataDirs = getDesktopFileDirs();
    auto idGen = std::any_of(XDGDataDirs.cbegin(), XDGDataDirs.cend(), [&desktopFile](const QString &suffixPath) {
        return desktopFile.startsWith(suffixPath);
    });

    if (idGen) {
        auto tmp = path.chopped(sizeof(desktopSuffix) - 1);
        auto components = tmp.split(QDir::separator()).toList();
        auto it = std::find(components.cbegin(), components.cend(), "applications");
        QString FileId;
        ++it;
        while (it != components.cend()) {
            FileId += (*(it++) + "-");
        }
        id = FileId.chopped(1);  // remove extra "-""
    }

    auto filePtr = std::make_unique<QFile>(std::move(path));

    auto [ctime, mtime, _] = getFileTimeInfo(QFileInfo{*filePtr});

    err = DesktopErrorCode::NoError;
    return DesktopFile{std::move(filePtr), std::move(id), mtime, ctime};
}

std::optional<DesktopFile> DesktopFile::searchDesktopFileById(const QString &appId, DesktopErrorCode &err) noexcept
{
    auto XDGDataDirs = getDesktopFileDirs();
    constexpr auto desktopSuffix = u8".desktop";

    for (const auto &dir : XDGDataDirs) {
        auto app = QFileInfo{dir + QDir::separator() + appId + desktopSuffix};
        while (!app.exists()) {
            auto filePath = app.absoluteFilePath();
            auto hyphenIndex = filePath.indexOf('-');
            if (hyphenIndex == -1) {
                break;
            }
            filePath.replace(hyphenIndex, 1, QDir::separator());
            app.setFile(filePath);
        }

        if (app.exists()) {
            return searchDesktopFileByPath(app.absoluteFilePath(), err);
        }
    }

    err = DesktopErrorCode::NotFound;
    return std::nullopt;
}

bool DesktopFile::modified(qint64 time) const noexcept
{
    return time != m_mtime;
}

DesktopErrorCode DesktopEntry::parse(DesktopFile &file) noexcept
{
    DesktopFileGuard guard{file};

    if (!guard.try_open()) {
        qWarning() << file.sourcePath() << "can't open.";
        return DesktopErrorCode::OpenFailed;
    }

    QTextStream stream;
    stream.setDevice(file.sourceFile());
    return parse(stream);
}

DesktopErrorCode DesktopEntry::parse(QTextStream &stream) noexcept
{
    if (m_parsed == true) {
        return DesktopErrorCode::Parsed;
    }

    if (stream.atEnd()) {
        return DesktopErrorCode::OpenFailed;
    }

    stream.setEncoding(QStringConverter::Utf8);

    DesktopErrorCode err{DesktopErrorCode::NoError};
    Parser p(stream);
    err = p.parse(m_entryMap);
    m_parsed = true;
    if (err != DesktopErrorCode::NoError) {
        return err;
    }

    if (!checkMainEntryValidation()) {
        qWarning() << "invalid MainEntry, abort.";
        err = DesktopErrorCode::MissingInfo;
    }

    return err;
}

std::optional<QMap<QString, DesktopEntry::Value>> DesktopEntry::group(const QString &key) const noexcept
{
    if (auto group = m_entryMap.find(key); group != m_entryMap.cend()) {
        return *group;
    }
    return std::nullopt;
}

std::optional<DesktopEntry::Value> DesktopEntry::value(const QString &groupKey, const QString &valueKey) const noexcept
{
    const auto &destGroup = group(groupKey);
    if (!destGroup) {
        qDebug() << "group " << groupKey << " can't be found.";
        return std::nullopt;
    }

    auto it = destGroup->find(valueKey);
    if (it == destGroup->cend()) {
        qDebug() << "value " << valueKey << " can't be found.";
        return std::nullopt;
    }
    return *it;
}

QString DesktopEntry::Value::unescape(const QString &str) noexcept
{
    QString unescapedStr;
    for (qsizetype i = 0; i < str.size(); ++i) {
        auto c = str.at(i);
        if (c != '\\') {
            unescapedStr.append(c);
            continue;
        }

        switch (str.at(i + 1).toLatin1()) {
        default:
            unescapedStr.append(c);
            break;
        case 'n':
            unescapedStr.append('\n');
            ++i;
            break;
        case 't':
            unescapedStr.append('\t');
            ++i;
            break;
        case 'r':
            unescapedStr.append('\r');
            ++i;
            break;
        case '\\':
            unescapedStr.append('\\');
            ++i;
            break;
        case ';':
            unescapedStr.append(';');
            ++i;
            break;
        case 's':
            unescapedStr.append(' ');
            ++i;
            break;
        }
    }

    return unescapedStr;
}

QString DesktopEntry::Value::toString(bool &ok) const noexcept
{
    ok = false;
    auto str = this->find(defaultKeyStr);

    if (str == this->end()) {
        qWarning() << "value not found.";
        return {};
    }

    auto unescapedStr = unescape(*str);
    if (hasNonAsciiAndControlCharacters(unescapedStr)) {
        return {};
    }

    ok = true;
    return unescapedStr;
}

QString DesktopEntry::Value::toLocaleString(const QLocale &locale, bool &ok) const noexcept
{
    ok = false;
    for (auto it = this->constKeyValueBegin(); it != this->constKeyValueEnd(); ++it) {
        auto [a, b] = *it;
        if (QLocale{a}.name() == locale.name()) {
            ok = true;
            return unescape(b);
        }
    }
    return toString(ok);
}

QString DesktopEntry::Value::toIconString(bool &ok) const noexcept
{
    return toString(ok);
}

bool DesktopEntry::Value::toBoolean(bool &ok) const noexcept
{
    ok = false;
    const auto &str = (*this)[defaultKeyStr];
    if (str == "true") {
        ok = true;
        return true;
    }
    if (str == "false") {
        ok = true;
        return false;
    }
    return false;
}

float DesktopEntry::Value::toNumeric(bool &ok) const noexcept
{
    const auto &str = (*this)[defaultKeyStr];
    QVariant v{str};
    return v.toFloat(&ok);
}

QDebug operator<<(QDebug debug, const DesktopEntry::Value &v)
{
    QDebugStateSaver saver{debug};
    debug << static_cast<const QMap<QString, QString> &>(v);
    return debug;
}

QDebug operator<<(QDebug debug, const DesktopErrorCode &v)
{
    QDebugStateSaver saver{debug};
    QString errMsg;
    switch (v) {
    case DesktopErrorCode::NoError: {
        errMsg = "no error.";
    } break;
    case DesktopErrorCode::NotFound: {
        errMsg = "file not found.";
    } break;
    case DesktopErrorCode::MismatchedFile: {
        errMsg = "file type is mismatched.";
    } break;
    case DesktopErrorCode::InvalidLocation: {
        errMsg = "file location is invalid, please check $XDG_DATA_DIRS.";
    } break;
    case DesktopErrorCode::OpenFailed: {
        errMsg = "couldn't open the file.";
    } break;
    case DesktopErrorCode::InvalidFormat: {
        errMsg = "the format of desktopEntry file is invalid.";
    } break;
    case DesktopErrorCode::MissingInfo: {
        errMsg = "missing required infomation.";
    } break;
    case DesktopErrorCode::Parsed: {
        errMsg = "this desktop entry is parsed.";
    } break;
    case DesktopErrorCode::InternalError: {
        errMsg = "internal error of parser.";
    } break;
    }
    debug << errMsg;
    return debug;
}
