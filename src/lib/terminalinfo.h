// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TERMINALINFO_H
#define TERMINALINFO_H

#include "appinfo.h"

#include <memory>
#include <vector>
#include <map>
#include <QGSettings>

class TerminalInfo
{
public:
    static TerminalInfo& getInstanceTerminal();
    void resetTerminal();
    std::string getPresetTerminalPath();
    bool setDefaultTerminal(std::string id);
    std::shared_ptr<AppInfoManger> getDefaultTerminal();
    std::vector<std::shared_ptr<AppInfoManger>> getTerminalInfos();
    TerminalInfo(const TerminalInfo& term) = delete;
    TerminalInfo& operator=(const TerminalInfo& term) = delete;

private:
    TerminalInfo();
    void init();

    bool isTerminalApp(std::shared_ptr<AppInfoManger> appInfo);

private:

    std::vector<std::string>            termBlackList;
    std::map<std::string, std::string>  execArgMap;
    std::vector<std::string>            terms;
    const std::string                   gsSchemaDefaultTerminal;
    const std::string                   gsKeyExec;
    const std::string                   gsKeyExecArg;
    const std::string                   gsKeyAppId;
    const std::string                   categoryTerminalEmulator;
    const std::string                   execXTerminalEmulator;
    std::shared_ptr<QGSettings>         defaultTerminal;
};

#endif // TERMINALINFO_H
