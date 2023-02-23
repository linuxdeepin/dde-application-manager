// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"
#include "macro.h"
#include "dstring.h"
#include "dfile.h"

#include <algorithm>
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
    if (m_status.size() == 0) {
        std::string statusFile = getFile("status");
        FILE *fp = fopen(statusFile.c_str(), "r");
        if (!fp)
            return m_status;

        char line[MAX_LINE_LEN] = {0};
        while (fgets(line, MAX_LINE_LEN, fp)) {
            std::string info(line);
            std::vector<std::string> parts = DString::splitStr(info, ':');
            if (parts.size() == 2)
            m_status[parts[0]] = parts[1];
        }
        fclose(fp);
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

// /proc is not real file system
std::vector<std::string> Process::readFile(std::string fileName)
{
    std::vector<std::string> ret;
    std::FILE *fp = std::fopen(fileName.c_str(), "r");
    if (!fp)
        return ret;

    std::vector<char> content(FILECONTENLEN);
    std::size_t len = std::fread(&content[0], 1, FILECONTENLEN, fp);
    std::fclose(fp);

    ret = DString::splitVectorChars(content, len, '\0');
    return ret;
}
