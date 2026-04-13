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
    skip();

    while (!atEnd()) {
        if (m_line.isEmpty()) {
            break;
        }

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
    if (m_line.isEmpty() || m_line.front() != '[' || m_line.back() != ']') {
        qCDebug(DDEAMMimeParser) << "Invalid mime file format: unexpected line:" << toUtf8String(m_line);
        return ParserError::InvalidFormat;
    }

    // Parsing group header, this format is same as desktop file's group

    const auto headerBytes = m_line.sliced(1, m_line.size() - 2).trimmed();
    if (std::any_of(headerBytes.cbegin(), headerBytes.cend(), [](auto ch) { return ch == '[' || ch == ']'; })) {
        qCWarning(DDEAMMimeParser) << "group header invalid:" << toUtf8String(m_line);
        return ParserError::InvalidFormat;
    }

    groupName = toUtf8String(headerBytes);
    const auto headerView = QStringView{groupName};
    if (hasNonAsciiAndControlCharacters(headerView)) {
        qCWarning(DDEAMMimeParser) << "group header invalid:" << headerView;
        return ParserError::InvalidFormat;
    }

    if (m_desktopSpec && (headerView == addedAssociations || headerView == removedAssociations)) {
        qCWarning(DDEAMMimeParser)
            << "desktop-specific mimeapp.list is not possible to add or remove associations from these files, skip this group.";
        while (!atEnd()) {
            skip();

            if (m_line.isEmpty()) {
                break;
            }

            if (m_line.startsWith('[')) {
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

        if (m_line.startsWith('[')) {
            // End of this group and start of another group, just break
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
    const auto splitCharIndex = m_line.indexOf('=');
    if (splitCharIndex == -1) {
        qWarning() << "invalid line in desktop file, skip it:" << toUtf8String(m_line);
        clearLine();
        return ParserError::NoError;
    }

    const auto keyView = m_line.first(splitCharIndex).trimmed();
    const auto valueView = m_line.sliced(splitCharIndex + 1).trimmed();

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
