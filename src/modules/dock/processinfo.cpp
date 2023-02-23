// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "processinfo.h"
#include "dstring.h"

#include <QFileInfo>
#include <QDir>
#include <QDebug>

ProcessInfo::ProcessInfo(int pid)
    : m_hasPid(true)
    , m_process(Process(pid))
    , m_isValid(true)
{
    if (pid == 0)
        return;

    // exe
    m_exe = m_process.getExe();
    // cwd
    m_cwd = m_process.getCwd();
    // cmdline
    m_cmdLine = m_process.getCmdLine();
    // 部分root进程在/proc文件系统查找不到exe、cwd、cmdline信息
    if (m_exe.empty() || m_cwd.empty() || m_cmdLine.size() == 0) {
        m_isValid = false;
        return;
    }

    // ppid
    Status pstatus = m_process.getStatus();
    if (pstatus.size() > 0) {
        int ppid = m_process.getPpid();
    }

    // args
    qInfo() << "ProcessInfo: exe=" << m_exe.c_str() << " cwd=" << m_cwd.c_str() << " cmdLine=" << (m_cmdLine[0].empty() ? " " : m_cmdLine[0].c_str());
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

    if (!m_cmdLine[0].empty()) {
        if (!verifyExe(m_exe, m_cwd, m_cmdLine[0])) {
            auto parts = DString::splitStr(m_cmdLine[0], ' ');
            // try again
            if (verifyExe(m_exe, m_cwd, parts[0])) {
                for (int j = 1; j < parts.size(); j++) {
                    m_args.push_back(parts[j]);
                }
                for (int i = 1; i < m_cmdLine.size(); i++) {
                    m_args.push_back(m_cmdLine[i]);
                }
            }
        } else {
            for (int i = 1; i < m_cmdLine.size(); i++) {
                m_args.push_back(m_cmdLine[i]);
            }
        }
    }
}

ProcessInfo::ProcessInfo(std::vector<std::string> &cmd)
    : m_hasPid(false)
    , m_isValid(true)
    , m_process(Process())
{
    if (cmd.size() == 0) {
        m_isValid = false;
        return;
    }

    m_cmdLine = cmd;
    m_exe = cmd[0];
    for (ulong i=0; i < cmd.size(); i++) {
        if (i > 0) {
            m_args.push_back(cmd[i]);
        }
    }
}

ProcessInfo::~ProcessInfo()
{
}

std::string ProcessInfo::getEnv(std::string key)
{
    return m_process.getEnv(key);
}

std::vector<std::string> ProcessInfo::getCmdLine()
{
    return m_cmdLine;
}

std::vector<std::string> ProcessInfo::getArgs()
{
    return m_args;
}

int ProcessInfo::getPid()
{
    return m_process.getPid();
}

int ProcessInfo::getPpid()
{
    return m_process.getPpid();
}

bool ProcessInfo::initWithPid()
{
    return m_hasPid;
}

bool ProcessInfo::isValid()
{
    return m_isValid;
}

std::string ProcessInfo::getExe()
{
    return m_exe;
}

std::string ProcessInfo::getOneCommandLine()
{
    std::string cmdline = getJoinedExeArgs();
    return "sh -c 'cd " + m_cwd + "; exec " + cmdline + ";'";
}

std::string ProcessInfo::getShellScriptLines()
{
    std::string cmdline = getJoinedExeArgs();
    return "#!/bin/sh\n cd " + m_cwd + "\n exec " + cmdline + "\n";
}

std::string ProcessInfo::getJoinedExeArgs()
{
    std::string ret = "\"" + m_exe + "\"";
    for (auto arg : m_args) {
        ret += " \"" + arg + "\"";
    }

    ret += " $@";
    return ret;
}
