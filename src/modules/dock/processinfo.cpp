/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "processinfo.h"
#include "dstring.h"

#include <QFileInfo>
#include <QDir>

ProcessInfo::ProcessInfo(int pid)
 : hasPid(true)
 , process(Process(pid))
{
    if (pid == 0)
        return;

    // exe
    exe = process.getExe();
    // cwd
    cwd = process.getCwd();
    // cmdline
    cmdLine = process.getCmdLine();
    // ppid
    Status pstatus = process.getStatus();
    if (pstatus.size() > 0) {
        int ppid = process.getPpid();
    }
    // args
    auto verifyExe =  [](std::string exe, std::string cwd, std::string firstArg){
        if (firstArg.size() == 0)
            return false;

        QFileInfo info(firstArg.c_str());
        if (info.baseName() == firstArg.c_str())
            return true;

        if (!QDir::isAbsolutePath(firstArg.c_str()))
            firstArg = cwd + firstArg;

        return exe == firstArg;
    };
    if (!verifyExe(exe, cwd, cmdLine[0])) {
        auto parts = DString::splitStr(cmdLine[0], ' ');
        // try again
        if (verifyExe(exe, cwd, parts[0])) {
            for (int j = 1; j < parts.size(); j++) {
                args.push_back(parts[j]);
            }
            for (int i = 1; i < cmdLine.size(); i++) {
                args.push_back(cmdLine[i]);
            }
        }
    } else {
        for (int i = 1; i < cmdLine.size(); i++) {
            args.push_back(cmdLine[i]);
        }
    }
}

ProcessInfo::ProcessInfo(std::vector<std::string> &cmd)
 : hasPid(false)
 , process(Process())
{
    if (cmd.size() == 0)
        return;

    cmdLine = cmd;
    exe = cmd[0];
    for (ulong i=0; i < cmd.size(); i++) {
        if (i > 0) {
            args.push_back(cmd[i]);
        }
    }
}

std::string ProcessInfo::getEnv(std::string key)
{
    return process.getEnv(key);
}

std::vector<std::string> ProcessInfo::getCmdLine()
{
    return cmdLine;
}

std::vector<std::string> ProcessInfo::getArgs()
{
    return args;
}

int ProcessInfo::getPid()
{
    return process.getPid();
}

int ProcessInfo::getPpid()
{
    return process.getPpid();
}

bool ProcessInfo::initWithPid()
{
    return hasPid;
}

std::string ProcessInfo::getExe()
{
    return exe;
}

std::string ProcessInfo::getOneCommandLine()
{
    std::string cmdline = getJoinedExeArgs();
    return "sh -c 'cd " + cwd + "; exec " + cmdline + ";'";
}

std::string ProcessInfo::getShellScriptLines()
{
    std::string cmdline = getJoinedExeArgs();
    return "#!/bin/sh\n cd " + cwd + "\n exec " + cmdline + "\n";
}

std::string ProcessInfo::getJoinedExeArgs()
{
    std::string ret = "\"" + exe + "\"";
    for (auto arg : args) {
        ret += " \"" + arg + "\"";
    }

    ret += " $@";
    return ret;
}


