// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QRegularExpression>
#include "desktopfileparser.h"
#include "constant.h"

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

}  // namespace

ParserError DesktopFileParser::parse(Groups &ret) noexcept
{
    std::remove_reference_t<decltype(ret)> groups;
    while (!m_stream.atEnd()) {
        auto err = addGroup(groups);
        if (err != ParserError::NoError) {
            return err;
        }

        if (groups.size() != 1) {
            continue;
        }

        if (groups.keys().first() != DesktopFileEntryKey) {
            qWarning() << "There should be nothing preceding "
                          "'Desktop Entry' group in the desktop entry file "
                          "but possibly one or more comments.";
            return ParserError::InvalidFormat;
        }
    }

    if (!m_line.isEmpty()) {
        qCritical() << "Something is wrong in Desktop file parser, check logic.";
        return ParserError::InternalError;
    }

    ret = std::move(groups);
    return ParserError::NoError;
}

ParserError DesktopFileParser::addGroup(Groups &ret) noexcept
{
    skip();
    if (!m_line.startsWith('[')) {
        qWarning() << "Invalid desktop file format: unexpected line:" << m_line;
        return ParserError::InvalidFormat;
    }

    // Parsing group header.
    // https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header

    auto groupHeader = m_line.sliced(1, m_line.size() - 2).trimmed();

    if (groupHeader.contains('[') || groupHeader.contains(']') || hasNonAsciiAndControlCharacters(groupHeader)) {
        qWarning() << "group header invalid:" << m_line;
        return ParserError::InvalidFormat;
    }

    if (ret.find(groupHeader) != ret.end()) {
        qWarning() << "duplicated group header detected:" << groupHeader;
        return ParserError::InvalidFormat;
    }

    auto group = ret.insert(groupHeader, {});

    m_line.clear();
    while (!m_stream.atEnd() && !m_line.startsWith('[')) {
        skip();
        if (m_line.startsWith('[')) {
            break;
        }
        auto err = addEntry(group);
        if (err != ParserError::NoError) {
            return err;
        }
    }
    return ParserError::NoError;
}

ParserError DesktopFileParser::addEntry(typename Groups::iterator &group) noexcept
{
    auto line = m_line;
    m_line.clear();
    auto splitCharIndex = line.indexOf('=');
    if (splitCharIndex == -1) {
        qWarning() << "invalid line in desktop file, skip it:" << line;
        return ParserError::NoError;
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
        return ParserError::NoError;
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
        return ParserError::NoError;
    }

    if (localeStr != defaultKeyStr && !isInvalidLocaleString(localeStr)) {
        qWarning().noquote() << QString("invalid LOCALE (%2) for key \"%1\"").arg(key, localeStr);
    }

    auto keyIt = group->find(key);
    if (keyIt != group->end() && keyIt->find(localeStr) != keyIt->end()) {
        qWarning() << "duplicated localestring, skip this line:" << line;
        return ParserError::NoError;
    }

    if (keyIt == group->end()) {
        group->insert(key, {{localeStr, valueStr}});
        return ParserError::NoError;
    }

    keyIt->insert(localeStr, valueStr);
    return ParserError::NoError;
}
