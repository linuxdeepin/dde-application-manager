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
