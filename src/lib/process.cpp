// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"
#include "macro.h"
#include "dstring.h"
#include "dfile.h"

#include <algorithm>
#include <fstream>
#include <dirent.h>
#include <unistd.h>

#define FILECONTENLEN 2048

Process::Process()
 : m_pid(0)
 , m_ppid(0)
{

}

Process::Process(int _pid)
 : m_pid(_pid)
 , m_ppid(0)
{

}

bool Process::isExist()
{
    std::string procDir = "/proc/" + std::to_string(m_pid);
    return DFile::isExisted(procDir);
}

std::vector<std::string> Process::getCmdLine()
{
    if (m_cmdLine.size() == 0) {
        std::string cmdlineFile = getFile("cmdline");
        m_cmdLine = readFile(cmdlineFile);
    }

    return m_cmdLine;
}

std::string Process::getCwd()
{
    if (m_cwd.empty()) {
        std::string cwdFile = getFile("cwd");
        char path[MAX_FILEPATH_LEN] = {};
        ssize_t len = readlink(cwdFile.c_str(), path, MAX_FILEPATH_LEN);
        if (len > 0 && len < MAX_FILEPATH_LEN) {
            m_cwd = std::string(path) + "/";
        }
    }

    return m_cwd;
}

std::string Process::getExe()
{
    if (m_exe.empty()) {
        std::string cmdLineFile = getFile("exe");
        char path[MAX_FILEPATH_LEN] = {};
        ssize_t len = readlink(cmdLineFile.c_str(), path, MAX_FILEPATH_LEN);
        if (len > 0 && len < MAX_FILEPATH_LEN) {
            m_exe = std::string(path);
        }
    }

    return m_exe;
}

std::vector<std::string> Process::getEnviron()
{
    if (m_environ.size() == 0) {
        std::string envFile = getFile("environ");
        m_environ = readFile(envFile);
    }

    return m_environ;
}

std::string Process::getEnv(const std::string &key)
{
    if (m_environ.size() == 0)
        m_environ = getEnviron();

    std::string keyPrefix = key + "=";
    for (auto & env : m_environ) {
        if (DString::startWith(env, keyPrefix)) {
            ulong len = keyPrefix.size();
            return env.substr(len, env.size() - len);
        }
     }

    return "";
}

Status Process::getStatus()
{
    if (!m_status.empty()){
        return m_status;
    }

    std::string statusFile = getFile("status");

    std::ifstream fs(statusFile);
    if (!fs.is_open()) {
        return m_status;
    }

    std::string tmp = "";
    while (std::getline(fs, tmp)) {
        auto pos = tmp.find_first_of(':');
        if (pos == std::string::npos) {
            continue;
        }

        std::string value;
        if (pos + 1 < tmp.length()) {
            value = tmp.substr(pos + 1);
        }

        m_status[tmp.substr(0, pos)] = value;
    }

    return m_status;
}

std::vector<int> Process::getUids()
{
    if (m_uids.size() == 0) {
        if (m_status.find("Uid") != m_status.end()) {
            std::string uidGroup = m_status["Uid"];
            std::vector<std::string> parts = DString::splitStr(uidGroup, '\t');
            m_uids.reserve(parts.size());
            std::transform(parts.begin(), parts.end(), m_uids.begin(),
                           [](std::string idStr) -> int {return std::stoi(idStr);});
        }
    }

    return m_uids;
}

int Process::getPid()
{
    return m_pid;
}

int Process::getPpid()
{
    if (m_ppid == 0) {
        if (m_status.find("PPid") != m_status.end()) {
            m_ppid = std::stoi(m_status["PPid"]);
        }
    }

    return m_ppid;
}

std::string Process::getFile(const std::string &name)
{
    return "/proc/" + std::to_string(m_pid) + "/" + name;
}

// This funciton only can used to read `environ` and `cmdline`
std::vector<std::string> Process::readFile(std::string fileName)
{
    std::vector<std::string> ret;
    std::ifstream fs(fileName);
    if (!fs.is_open()) {
            return ret;
    }

    std::string tmp;
    while (std::getline(fs, tmp, '\0')) {
        ret.push_back(tmp);
    }

    return ret;
}
