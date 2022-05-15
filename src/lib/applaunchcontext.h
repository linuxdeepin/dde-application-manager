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
