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

#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include "process.h"

#include <string>
#include <vector>

// 进程信息
class ProcessInfo
{
public:
    ProcessInfo(int pid);
    ProcessInfo(std::vector<std::string> &cmd);

    std::string getEnv(std::string key);
    std::vector<std::string> getCmdLine();
    std::vector<std::string> getArgs();
    int getPid();
    int getPpid();
    std::string getExe();
    std::string getOneCommandLine();
    std::string getShellScriptLines();

private:
    std::string getJoinedExeArgs();

    std::vector<std::string> cmdLine;
    std::vector<std::string> args;
    std::string exe;
    std::string cwd;

    bool hasPid;
    Process process;
};

#endif // PROCESSINFO_H
