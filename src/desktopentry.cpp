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

auto DesktopEntry::parseGroupHeader(const QString &str) noexcept
{
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header

    auto groupHeader = str.sliced(1, str.size() - 2).trimmed();
    decltype(m_entryMap)::iterator it{m_entryMap.end()};

    // NOTE:
    // This regex match '[', ']', control characters
    // and all non-ascii characters.
    // They are invalid in group header.
    // https://regex101.com/r/bZhHZo/1
    QRegularExpression re{R"([^\x20-\x5a\x5e-\x7e\x5c])"};
    auto matcher = re.match(groupHeader);
    if (matcher.hasMatch()) {
        qWarning() << "group header invalid:" << str;
        return it;
    }

    auto tmp = m_entryMap.find(groupHeader);
    if (tmp == m_entryMap.end()) {
        it = m_entryMap.insert(groupHeader, {});
    }

    qWarning() << "group header already exists:" << str;
    return it;
}

QString DesktopFile::sourcePath() const noexcept
{
    if (!m_fileSource) {
        return "";
    }

    QFileInfo info(*m_fileSource);
    return info.absoluteFilePath();
}

bool DesktopEntry::isInvalidLocaleString(const QString &str) noexcept
{
    constexpr auto Language = R"((?:[a-z]+))";        // language of locale postfix. eg.(en, zh)
    constexpr auto Country = R"((?:_[A-Z]+))";        // country of locale postfix. eg.(US, CN)
    constexpr auto Encoding = R"((?:\.[0-9A-Z-]+))";  // encoding of locale postfix. eg.(UFT-8)
    constexpr auto Modifier = R"((?:@[a-z=;]+))";     // modifier of locale postfix. eg.(euro;collation=traditional)
    const static auto validKey = QString(R"(^%1%2?%3?%4?$)").arg(Language, Country, Encoding, Modifier);
    // example: https://regex101.com/r/hylOay/2
    static QRegularExpression re = []() -> QRegularExpression {
        QRegularExpression tmp{validKey};
        tmp.optimize();
        return tmp;
    }();

    return re.match(str).hasMatch();
}

QPair<QString, QString> DesktopEntry::processEntry(const QString &str) noexcept
{
    auto splitCharIndex = str.indexOf(']');
    if (splitCharIndex != -1) {
        for (; splitCharIndex < str.size(); ++splitCharIndex) {
            if (str.at(splitCharIndex) == '=') {
                break;
            }
        }
    } else {
        splitCharIndex = str.indexOf('=');
    }
    auto keyStr = str.first(splitCharIndex).trimmed();
    auto valueStr = str.sliced(splitCharIndex + 1).trimmed();
    return qMakePair(std::move(keyStr), std::move(valueStr));
}

std::optional<QPair<QString, QString>> DesktopEntry::processEntryKey(const QString &keyStr) noexcept
{
    QString key;
    QString localeStr;
    // NOTE:
    // We are process "localized keys" here, for usage check:
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#localized-keys
    if (auto index = keyStr.indexOf('['); index != -1) {
        key = keyStr.sliced(0, index);
        localeStr = keyStr.sliced(index + 1, keyStr.length() - 1 - index - 1);  // strip '[' and ']'
        if (!isInvalidLocaleString(localeStr)) {
            qWarning().noquote() << QString("invalid LOCALE (%2) for key \"%1\"").arg(key, localeStr);
        }
    } else {
        key = keyStr;
    }

    QRegularExpression re{"R([^A-Za-z0-9-])"};
    if (re.match(key).hasMatch()) {
        qWarning() << "keyName's format is invalid.";
        return std::nullopt;
    }

    return qMakePair(std::move(key), std::move(localeStr));
}

DesktopErrorCode DesktopEntry::parseEntry(const QString &str, decltype(m_entryMap)::iterator &currentGroup) noexcept
{
    auto [key, value] = processEntry(str);
    auto keyPair = processEntryKey(key);

    if (!keyPair.has_value()) {
        return DesktopErrorCode::InvalidFormat;
    }

    auto [keyName, localeStr] = std::move(keyPair).value();
    if (localeStr.isEmpty()) {
        localeStr = defaultKeyStr;
    }

    auto valueIt = currentGroup->find(keyName);
    if (valueIt == currentGroup->end()) {
        currentGroup->insert(keyName, {{localeStr, value}});
        return DesktopErrorCode::NoError;
    }

    auto innerIt = valueIt->find(localeStr);
    if (innerIt == valueIt->end()) {
        valueIt->insert(localeStr, value);
        return DesktopErrorCode::NoError;
    }

    qWarning() << "duplicated postfix and this line will be aborted, maybe format is invalid.\n"
               << "exist: " << innerIt.key() << "[" << innerIt.value() << "]"
               << "current: " << keyName << "[" << localeStr << "]";

    return DesktopErrorCode::NoError;
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
        for (const auto &[k, v] : it->asKeyValueRange()) {
            qInfo() << "keyName:" << k;
            for (const auto &[key, value] : v.asKeyValueRange()) {
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
    auto mtime = getFileModifiedTime(*temporaryFile);
    if (mtime == 0) {
        qWarning() << "create temporary file failed.";
        return std::nullopt;
    }
    return DesktopFile{std::move(temporaryFile), "", mtime};
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

    auto mtime = getFileModifiedTime(*filePtr);

    if (mtime == 0) {
        err = DesktopErrorCode::OpenFailed;
        return std::nullopt;
    }

    err = DesktopErrorCode::NoError;
    return DesktopFile{std::move(filePtr), std::move(id), mtime};
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

bool DesktopFile::modified(std::size_t time) const noexcept
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

bool DesktopEntry::skipCheck(const QString &line) noexcept
{
    return line.startsWith('#') or line.isEmpty();
}

DesktopErrorCode DesktopEntry::parse(QTextStream &stream) noexcept
{
    if (stream.atEnd()) {
        if (m_context == EntryContext::Done) {
            return DesktopErrorCode::NoError;
        }
        return DesktopErrorCode::OpenFailed;
    }

    stream.setEncoding(QStringConverter::Utf8);
    decltype(m_entryMap)::iterator currentGroup;

    DesktopErrorCode err{DesktopErrorCode::NoError};
    bool mainEntryParsed{false};
    QString line;

    while (!stream.atEnd()) {
        switch (m_context) {
            case EntryContext::Unknown: {
                qWarning() << "entry context is unknown,abort.";
                err = DesktopErrorCode::InvalidFormat;
                return err;
            } break;
            case EntryContext::EntryOuter: {
                if (skipCheck(line)) {
                    line = stream.readLine().trimmed();
                    continue;
                }

                if (line.startsWith('[')) {
                    auto group = parseGroupHeader(line);

                    if (group == m_entryMap.end()) {
                        return DesktopErrorCode::InvalidFormat;
                    }
                    currentGroup = group;
                    bool isMainEntry = (currentGroup.key() == DesktopFileEntryKey);

                    if ((!mainEntryParsed and isMainEntry) or (mainEntryParsed and !isMainEntry)) {
                        m_context = EntryContext::Entry;
                        continue;
                    }
                }
                qWarning() << "groupName format error:" << line;
                err = DesktopErrorCode::InvalidFormat;
                return err;
            } break;
            case EntryContext::Entry: {
                line = stream.readLine().trimmed();

                if (skipCheck(line)) {
                    continue;
                }

                if (line.startsWith('[')) {
                    m_context = EntryContext::EntryOuter;

                    if (currentGroup.key() == DesktopFileEntryKey) {
                        mainEntryParsed = true;
                    }
                    continue;
                }

                err = parseEntry(line, currentGroup);
                if (err != DesktopErrorCode::NoError) {
                    qWarning() << "Entry format error:" << line;
                    return err;
                }
            } break;
            case EntryContext::Done:
                break;
        }
    }

    m_context = EntryContext::Done;
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
#ifdef DEBUG_MODE
        qWarning() << "group " << groupKey << " can't be found.";
#endif
        return std::nullopt;
    }

    auto it = destGroup->find(valueKey);
    if (it == destGroup->cend()) {
#ifdef DEBUG_MODE
        qWarning() << "value " << valueKey << " can't be found.";
#endif
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
        return {};
    }
    auto unescapedStr = unescape(*str);
    constexpr auto controlChars = "\\p{Cc}";
    constexpr auto asciiChars = "[^\x00-\x7f]";
    if (unescapedStr.contains(QRegularExpression{controlChars}) and unescapedStr.contains(QRegularExpression{asciiChars})) {
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
            break;
        }
        case DesktopErrorCode::NotFound: {
            errMsg = "file not found.";
            break;
        }
        case DesktopErrorCode::MismatchedFile: {
            errMsg = "file type is mismatched.";
            break;
        }
        case DesktopErrorCode::InvalidLocation: {
            errMsg = "file location is invalid, please check $XDG_DATA_DIRS.";
            break;
        }
        case DesktopErrorCode::OpenFailed: {
            errMsg = "couldn't open the file.";
            break;
        }
        case DesktopErrorCode::InvalidFormat: {
            errMsg = "the format of desktopEntry file is invalid.";
            break;
        }
        case DesktopErrorCode::MissingInfo: {
            errMsg = "missing required infomation.";
            break;
        }
    }
    debug << errMsg;
    return debug;
}
