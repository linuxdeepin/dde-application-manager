// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
