// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef DESKTOPFILEPARSER_H
#define DESKTOPFILEPARSER_H

#include "iniParser.h"
#include "desktopentry.h"

class DesktopFileParser final : public Parser<DesktopEntry::Value>
{
public:
    using Parser<DesktopEntry::Value>::Parser;
    ParserError parse(Groups &ret) noexcept override;

protected:
    ParserError addGroup(Groups &groups, QString &groupName) noexcept override;
    ParserError addEntry(Groups::iterator groups) noexcept override;
};

QString toString(const DesktopFileParser::Groups &groups);

#endif
