// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mimefileparser.h"

Q_LOGGING_CATEGORY(DDEAMMimeParser, "dde.am.mime.parser")

namespace {
QString toUtf8String(QByteArrayView view) noexcept
{
    return QString::fromUtf8(view);
}
}  // namespace

ParserError MimeFileParser::parse(Groups &ret) noexcept
{
    while (!atEnd()) {
        QString groupName;
        auto err = addGroup(ret, groupName);
        if (err != ParserError::NoError) {
            return err;
        }
    }

    if (!m_line.isEmpty()) {
        qCCritical(DDEAMMimeParser) << "Something is wrong in mimeapp.list parser, check logic.";
        return ParserError::InternalError;
    }

    return ParserError::NoError;
}

ParserError MimeFileParser::addGroup(Groups &ret, QString &groupName) noexcept
{
    if (!m_line.startsWith('[')) {
        skip();
    }

    auto lineView = m_line;
    if (!lineView.startsWith('[') || !lineView.endsWith(']')) {
        qWarning() << "Invalid mimeapp.list format: unexpected line:" << toUtf8String(lineView);
        return ParserError::InvalidFormat;
    }

    // Parsing group header, this format is same as desktop file's group

    const auto headerBytes = lineView.sliced(1, lineView.size() - 2).trimmed();
    if (std::any_of(headerBytes.cbegin(), headerBytes.cend(), [](auto ch) { return ch == '[' || ch == ']'; })) {
        qWarning() << "group header invalid:" << toUtf8String(lineView);
        return ParserError::InvalidFormat;
    }

    const auto header = toUtf8String(headerBytes);
    const auto headerView = QStringView{header};
    if (hasNonAsciiAndControlCharacters(headerView)) {
        qWarning() << "group header invalid:" << header;
        return ParserError::InvalidFormat;
    }

    groupName = header;
    if (m_desktopSpec && (headerView == addedAssociations || headerView == removedAssociations)) {
        qWarning()
            << "desktop-specific mimeapp.list is not possible to add or remove associations from these files, skip this group.";
        while (!atEnd()) {
            skip();

            lineView = m_line;
            if (lineView.startsWith('[')) {
                break;
            }
        }

        return ParserError::NoError;
    }

    auto groupIt = ret.find(groupName);
    if (groupIt == ret.end()) {
        groupIt = ret.insert(groupName, {});
    }

    clearLine();
    while (!atEnd()) {
        skip();

        lineView = m_line;
        if (lineView.startsWith('[')) {
            break;
        }

        auto err = addEntry(groupIt);
        if (err != ParserError::NoError) {
            return err;
        }
    }

    return ParserError::NoError;
}

ParserError MimeFileParser::addEntry(Groups::iterator group) noexcept
{
    if (m_line.isEmpty()) {
        return ParserError::NoError;
    }

    const auto lineView = m_line;

    const auto splitCharIndex = lineView.indexOf('=');
    if (splitCharIndex == -1) {
        qWarning() << "invalid line in desktop file, skip it:" << toUtf8String(lineView);
        clearLine();
        return ParserError::NoError;
    }

    const auto keyView = lineView.first(splitCharIndex).trimmed();
    const auto valueView = lineView.sliced(splitCharIndex + 1).trimmed();

    if (valueView.isEmpty()) {
        clearLine();
        return ParserError::InvalidFormat;
    }

    auto &list = (*group)[toUtf8String(keyView)];  // NOLINT
    if (!list.isEmpty()) {
        qWarning() << "duplicated entry detected:" << toUtf8String(keyView);
        clearLine();
        return ParserError::InvalidFormat;
    }

    const auto values = QByteArray{valueView}.split(';');
    for (const auto &subValue : values) {
        auto trimmedSubView = QByteArrayView{subValue}.trimmed();
        if (!trimmedSubView.isEmpty()) {
            list.append(toUtf8String(trimmedSubView));
        }
    }

    clearLine();
    return ParserError::NoError;
}
