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

auto DesktopEntry::parserGroupHeader(const QString &str) noexcept
{
    auto groupHeader = str.sliced(1, str.size() - 2);
    auto it = m_entryMap.find(groupHeader);
    if (it == m_entryMap.cend()) {
        return m_entryMap.insert(groupHeader, {});
    }
    return it;
}

ParseError DesktopEntry::parseEntry(const QString &str, decltype(m_entryMap)::iterator &currentGroup) noexcept
{
    if (str.startsWith("#")) {
        return ParseError::NoError;
    }

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
    QString key;
    QString valueKey{defaultKeyStr};

    constexpr auto MainKey = R"re((?<MainKey>[0-9a-zA-Z-]+))re";  // main key. eg.(Name, X-CUSTOM-KEY).
    constexpr auto Language = R"re((?:[a-z]+))re";                // language of locale postfix. eg.(en, zh)
    constexpr auto Country = R"re((?:_[A-Z]+))re";                // country of locale postfix. eg.(US, CN)
    constexpr auto Encoding = R"re((?:\.[0-9A-Z-]+))re";          // encoding of locale postfix. eg.(UFT-8)
    constexpr auto Modifier = R"re((?:@[a-z=;]+))re";             // modifier of locale postfix. eg(euro;collation=traditional)
    const static auto validKey =
        QString("^%1(?:\\[(?<LOCALE>%2%3?%4?%5?)\\])?$").arg(MainKey).arg(Language).arg(Country).arg(Encoding).arg(Modifier);
    // example: https://regex101.com/r/hylOay/1
    QRegularExpression re{validKey};
    re.optimize();
    auto matcher = re.match(keyStr);
    if (!matcher.hasMatch()) {
        qWarning() << "invalid key: " << keyStr;
        return ParseError::EntryKeyInvalid;
    }

    key = matcher.captured("MainKey");

    if (auto locale = matcher.captured("LOCALE"); !locale.isEmpty()) {
        valueKey = locale;
    }

    auto cur = currentGroup->find(key);
    if (cur == currentGroup->end()) {
        currentGroup->insert(keyStr, {{valueKey, valueStr}});
        return ParseError::NoError;
    }

    auto value = cur->find(valueKey);
    if (value == cur->end()) {
        cur->insert(valueKey, valueStr);
        return ParseError::NoError;
    }

    qWarning() << "duplicated postfix and this line will be aborted, maybe format is invalid.\n"
               << "exist: " << value.key() << "[" << value.value() << "]"
               << "current: " << str;

    return ParseError::NoError;
}

std::optional<DesktopFile> DesktopFile::searchDesktopFile(const QString &desktopFile, ParseError &err) noexcept
{
    if (auto tmp = desktopFile.split("."); tmp.last() != "desktop") {
        qWarning() << "file isn't a desktop file";
        err = ParseError::MismatchedFile;
        return std::nullopt;
    }

    QString path;
    QString id;

    QFileInfo Fileinfo{desktopFile};
    if (Fileinfo.isAbsolute() and Fileinfo.exists()) {
        path = desktopFile;
    } else {
        auto XDGDataDirs = QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS")).split(':', Qt::SkipEmptyParts);
        std::for_each(XDGDataDirs.begin(), XDGDataDirs.end(), [](QString &str) {
            str = QDir::cleanPath(str) + QDir::separator() + "applications";
        });
        auto fileName = Fileinfo.fileName();

        applyIteratively(QList<QDir>{XDGDataDirs.begin(), XDGDataDirs.end()}, [&fileName, &path](const QFileInfo &file) -> bool {
            if (file.fileName() == fileName) {
                path = file.absoluteFilePath();
                return true;
            }
            return false;
        });
    }

    if (path.isEmpty()) {
        qWarning() << "desktop file not found.";
        err = ParseError::NotFound;
        return std::nullopt;
    }
    auto tmp = path.chopped(8);  // remove ".desktop"
    auto components = tmp.split(QDir::separator()).toList();
    auto it = std::find(components.cbegin(), components.cend(), "applications");
    if (it == components.cend()) {
        qWarning() << "custom location detected, Id wouldn't be generated.";
    } else {
        QString FileId;
        ++it;
        while (it != components.cend())
            FileId += (*(it++) + "-");
        id = FileId.chopped(1);  // remove extra "-""
    }

    struct stat buf;
    if (auto ret = stat(path.toLatin1().data(), &buf); ret == -1) {
        err = ParseError::OpenFailed;
        qWarning() << "get file" << path << "state failed:" << std::strerror(errno);
        return std::nullopt;
    }

    err = ParseError::NoError;
    constexpr std::size_t nanoToSec = 1e9;

    return DesktopFile{std::move(path), std::move(id), buf.st_mtim.tv_sec * nanoToSec + buf.st_mtim.tv_nsec};
}

bool DesktopFile::modified(std::size_t time) const noexcept
{
    return time != m_mtime;
}

ParseError DesktopEntry::parse(const DesktopFile &desktopFile) noexcept
{
    auto file = QFile(desktopFile.filePath());
    if (!file.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
        qWarning() << desktopFile.filePath() << "can't open.";
        return ParseError::OpenFailed;
    }

    QTextStream in{&file};
    return parse(in);
}

ParseError DesktopEntry::parse(QTextStream &stream) noexcept
{
    if (stream.atEnd()) {
        return ParseError::OpenFailed;
    }

    stream.setEncoding(QStringConverter::Utf8);
    decltype(m_entryMap)::iterator currentGroup;

    ParseError err{ParseError::NoError};
    while (!stream.atEnd()) {
        auto line = stream.readLine().trimmed();

        if (line.isEmpty()) {
            continue;
        }

        if (line.startsWith("[")) {
            if (!line.endsWith("]")) {
                return ParseError::GroupHeaderInvalid;
            }
            currentGroup = parserGroupHeader(line);
            continue;
        }

        if (auto error = parseEntry(line, currentGroup); error != ParseError::NoError) {
            err = error;
            qWarning() << "an error occurred,this line will be skipped:" << line;
        }
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
        qWarning() << "group " << groupKey << " can't be found.";
        return std::nullopt;
    }

    auto it = destGroup->find(valueKey);
    if (it == destGroup->cend()) {
        qWarning() << "value " << valueKey << " can't be found.";
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

QDebug operator<<(QDebug debug, const ParseError &v)
{
    QDebugStateSaver saver{debug};
    QString errMsg;
    switch (v) {
        case ParseError::NoError: {
            errMsg = "no error.";
            break;
        }
        case ParseError::NotFound: {
            errMsg = "file not found.";
            break;
        }
        case ParseError::MismatchedFile: {
            errMsg = "file type is mismatched.";
            break;
        }
        case ParseError::InvalidLocation: {
            errMsg = "file location is invalid, please check $XDG_DATA_DIRS.";
            break;
        }
        case ParseError::OpenFailed: {
            errMsg = "couldn't open the file.";
            break;
        }
        case ParseError::GroupHeaderInvalid: {
            errMsg = "groupHead syntax is invalid.";
            break;
        }
        case ParseError::EntryKeyInvalid: {
            errMsg = "key syntax is invalid.";
            break;
        }
    }
    debug << errMsg;
    return debug;
}
