// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef APPLAUNCHCONTEXT_H
#define APPLAUNCHCONTEXT_H

#include <string>
#include <vector>

class DesktopInfo;
class AppLaunchContext
{
public:
    AppLaunchContext();

    void setEnv(const std::vector<std::string> &value) {m_env = value;}
    std::vector<std::string> getEnv() {return m_env;}

    void setTimestamp(uint32_t value) {m_timestamp = value;}
    uint32_t getTimestamp() {return m_timestamp;}

    void setCmdPrefixes(const std::vector<std::string> &value) {m_cmdPrefixes = value;}
    std::vector<std::string> getCmdPrefixes() {return m_cmdPrefixes;}

    void setCmdSuffixes(const std::vector<std::string> &value) {m_cmdSuffixes = value;}
    std::vector<std::string> getCmdSuffixes() {return m_cmdSuffixes;}

    std::string getStartupNotifyId(const DesktopInfo *info, std::vector<std::string> files);

    void launchFailed(std::string startupNotifyId);

private:
    uint m_count;
    uint32_t m_timestamp;
    std::vector<std::string> m_cmdPrefixes;
    std::vector<std::string> m_cmdSuffixes;
    std::vector<std::string> m_env;
};

#endif // APPLAUNCHCONTEXT_H
