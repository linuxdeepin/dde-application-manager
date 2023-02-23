// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "terminalinfo.h"
#include "utils.h"
#include "appinfocommon.h"

#include <QVariant>

TerminalInfo& TerminalInfo::getInstanceTerminal()
{
    static TerminalInfo terminal;

    return terminal;
}

TerminalInfo::TerminalInfo()
    : gsSchemaDefaultTerminal("com.deepin.desktop.default-applications.terminal")
    , gsKeyExec("exec")
    , gsKeyExecArg("exec-arg")
    , gsKeyAppId("app-id")
    , categoryTerminalEmulator("TerminalEmulator")
    , execXTerminalEmulator("x-terminal-emulator")
    , defaultTerminal(new QGSettings(gsSchemaDefaultTerminal.c_str()))
{
    init();
}

void TerminalInfo::resetTerminal()
{
    defaultTerminal->reset(gsKeyExec.c_str());
    defaultTerminal->reset(gsKeyExecArg.c_str());
    defaultTerminal->reset(gsKeyExecArg.c_str());
}

std::string TerminalInfo::getPresetTerminalPath()
{
    std::string path;

    for (auto term : terms) {
        path = lookPath(term);
        if (!path.empty()) {
            return path;
        }
    }

    return "";
}

void TerminalInfo::init()
{
    termBlackList = {"guake",
                     "tilda",
                     "org.kde.yakuake",
                     "qterminal_drop",
                     "Terminal"
                    };

    execArgMap = {{"gnome-terminal", "-x"},
        {"mate-terminal", "-x"},
        {"terminator",    "-x"},
        {"xfce4-terminal", "-x"}
    };

    terms = {"deepin-terminal",
             "gnome-terminal",
             "terminator",
             "xfce4-terminal",
             "rxvt",
             "xterm"
            };
}
std::shared_ptr<AppInfoManger> TerminalInfo::getDefaultTerminal()
{

    std::string appId = defaultTerminal->get(gsKeyAppId.c_str()).toString().toStdString();

    if (!hasEnding(appId, AppinfoCommon::DesktopExt)) {
        appId += AppinfoCommon::DesktopExt;
    }

    std::vector<std::shared_ptr<AppInfoManger>> appInfos = getTerminalInfos();

    for (auto iter : appInfos) {
        if (iter->getDesktopId() == appId) {
            return iter;
        }
    }

    for (auto appInfo : appInfos) {
        for (auto term : terms) {
            if (appInfo->getCmdline() == term) {
                return appInfo;
            }
        }
    }

    return nullptr;
}

bool TerminalInfo::setDefaultTerminal(std::string id)
{
    std::vector<std::shared_ptr<AppInfoManger>> appInfos = getTerminalInfos();

    for (auto iter : appInfos) {
        if (iter->getDesktopId() == id) {
            std::string cmdline = iter->getCmdline();
            std::string exec = cmdline.substr(0, cmdline.find(" "));
            defaultTerminal->set(gsKeyExec.c_str(), exec.c_str());

            std::string arg = "-e";
            if (execArgMap.count(exec) != 0) {
                arg = execArgMap[exec];
            }
            defaultTerminal->set(gsKeyExecArg.c_str(), arg.c_str());

            if (hasEnding(id, AppinfoCommon::DesktopExt)) {
                id = id.substr(0, id.find(AppinfoCommon::DesktopExt));
            }
            defaultTerminal->set(gsKeyAppId.c_str(), id.c_str());
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<AppInfoManger>> TerminalInfo::getTerminalInfos()
{
    std::map<std::string, std::vector<std::string>> skipDirs;
    std::vector<std::shared_ptr<AppInfoManger>> appInfos = AppInfoManger::getAll(skipDirs);

    std::vector<std::shared_ptr<AppInfoManger>>::iterator iter = appInfos.begin();

    while (iter != appInfos.end()) {
        if (isTerminalApp(*iter)) {
            (*iter)->setDesktopId((*iter)->getDesktopId() + AppinfoCommon::DesktopExt);
            iter++;
        } else {
            iter = appInfos.erase(iter);
        }
    }

    return appInfos;
}

bool TerminalInfo::isTerminalApp(std::shared_ptr<AppInfoManger> appInfo)
{
    if (std::find(termBlackList.begin(), termBlackList.end(), appInfo->getDesktopId()) != termBlackList.end()) {
        return false;
    }

    std::vector<std::string> categories = appInfo->getCategories();
    if (std::find(categories.begin(), categories.end(), categoryTerminalEmulator) == categories.end()) {
        return false;
    }

    std::string cmdline = appInfo->getCmdline();
    if (cmdline.find(execXTerminalEmulator) != std::string::npos) {
        return false;
    }

    return true;
}
