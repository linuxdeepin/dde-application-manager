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
    explicit ProcessInfo(int pid);
    explicit ProcessInfo(std::vector<std::string> &cmd);
    virtual ~ProcessInfo();

    std::string getEnv(std::string key);
    std::vector<std::string> getCmdLine();
    std::vector<std::string> getArgs();
    int getPid();
    int getPpid();
    bool initWithPid();
    bool isValid();
    std::string getExe();
    std::string getOneCommandLine();
    std::string getShellScriptLines();

private:
    std::string getJoinedExeArgs();

    std::vector<std::string> m_cmdLine;
    std::vector<std::string> m_args;
    std::string m_exe;
    std::string m_cwd;

    bool m_hasPid;
    bool m_isValid;
    Process m_process;
};

#endif // PROCESSINFO_H
