// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROCESS_H
#define PROCESS_H

#include <map>
#include <vector>
#include <string>

typedef std::map<std::string, std::string> Status;

class Process
{
public:
    explicit Process();
    explicit Process(int _pid);

    bool isExist();
    std::vector<std::string> getCmdLine();
    std::string getCwd();
    std::string getExe();
    std::vector<std::string> getEnviron();
    std::string getEnv(const std::string &key);
    Status getStatus();
    std::vector<int> getUids();
    int getPid();
    int getPpid();

private:
    std::string getFile(const std::string &name);
    std::vector<std::string> readFile(std::string fileName);

    int m_pid;
    std::vector<std::string> m_cmdLine;
    std::string m_cwd;
    std::string m_exe;
    std::vector<std::string> m_environ;
    Status m_status;
    std::vector<int> m_uids;
    int m_ppid;
};

#endif // PROCESS_H
