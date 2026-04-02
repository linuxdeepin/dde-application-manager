// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MIMEAPPFILEPARSER_H
#define MIMEAPPFILEPARSER_H

#include "iniParser.h"
#include <QMimeDatabase>
#include <QMimeType>
#include <QLoggingCategory>

constexpr static auto &defaultApplications = u"Default Applications";
constexpr static auto &addedAssociations = u"Added Associations";
constexpr static auto &removedAssociations = u"Removed Associations";
constexpr static auto &mimeCache = u"MIME Cache";

Q_DECLARE_LOGGING_CATEGORY(DDEAMMimeParser)

class MimeFileParser final : public Parser<QStringList>
{
public:
    explicit MimeFileParser(QFile &file, bool isDesktopSpecific)
        : Parser(file)
        , m_desktopSpec(isDesktopSpecific)
    {
    }

    ParserError parse(Groups &ret) noexcept override;

protected:
    ParserError addGroup(Groups &ret, QString &groupName) noexcept override;
    ParserError addEntry(Groups::iterator group) noexcept override;

private:
    bool m_desktopSpec;
};

#endif
