/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
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

#include "process.h"
#include "macro.h"
#include "dstring.h"
#include "dfile.h"

#include <algorithm>
#include <dirent.h>
#include <unistd.h>

#define FILECONTENLEN 2048

Process::Process()
 : pid(0)
 , ppid(0)
{

}

Process::Process(int _pid)
 : pid(_pid)
 , ppid(0)
{

}

bool Process::isExist()
{
    std::string procDir = "/proc/" + std::to_string(pid);
    return DFile::isExisted(procDir);
}

std::vector<std::string> Process::getCmdLine()
{
    if (cmdLine.size() == 0) {
        std::string cmdlineFile = getFile("cmdline");
        cmdLine = readFile(cmdlineFile);
    }

    return cmdLine;
}

std::string Process::getCwd()
{
    if (cwd.empty()) {
        std::string cwdFile = getFile("cwd");
        char path[MAX_FILEPATH_LEN] = {};
        ssize_t len = readlink(cwdFile.c_str(), path, MAX_FILEPATH_LEN);
        if (len > 0 && len < MAX_FILEPATH_LEN) {
            cwd = std::string(path) + "/";
        }
    }

    return cwd;
}

std::string Process::getExe()
{
    if (exe.empty()) {
        std::string cmdLineFile = getFile("exe");
        char path[MAX_FILEPATH_LEN] = {};
        ssize_t len = readlink(cmdLineFile.c_str(), path, MAX_FILEPATH_LEN);
        if (len > 0 && len < MAX_FILEPATH_LEN) {
            exe = std::string(path);
        }
    }

    return exe;
}

std::vector<std::string> Process::getEnviron()
{
    if (environ.size() == 0) {
        std::string envFile = getFile("environ");
        environ = readFile(envFile);
    }

    return environ;
}

std::string Process::getEnv(const std::string &key)
{
    if (environ.size() == 0)
        environ = getEnviron();

    std::string keyPrefix = key + "=";
    for (auto & env : environ) {
        if (DString::startWith(env, keyPrefix)) {
            ulong len = keyPrefix.size();
            return env.substr(len, env.size() - len);
        }
     }

    return "";
}

Status Process::getStatus()
{
    if (status.size() == 0) {
        std::string statusFile = getFile("status");
        FILE *fp = fopen(statusFile.c_str(), "r");
        if (!fp)
            return status;

        char line[MAX_LINE_LEN] = {0};
        while (fgets(line, MAX_LINE_LEN, fp)) {
            std::string info(line);
            std::vector<std::string> parts = DString::splitStr(info, ':');
            if (parts.size() == 2)
            status[parts[0]] = parts[1];
        }
        fclose(fp);
    }

    return status;
}

std::vector<int> Process::getUids()
{
    if (uids.size() == 0) {
        if (status.find("Uid") != status.end()) {
            std::string uidGroup = status["Uid"];
            std::vector<std::string> parts = DString::splitStr(uidGroup, '\t');
            uids.reserve(parts.size());
            std::transform(parts.begin(), parts.end(), uids.begin(),
                           [](std::string idStr) -> int {return std::stoi(idStr);});
        }
    }

    return uids;
}

int Process::getPid()
{
    return pid;
}

int Process::getPpid()
{
    if (ppid == 0) {
        if (status.find("PPid") != status.end()) {
            ppid = std::stoi(status["PPid"]);
        }
    }

    return ppid;
}

std::string Process::getFile(const std::string &name)
{
    return "/proc/" + std::to_string(pid) + "/" + name;
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
