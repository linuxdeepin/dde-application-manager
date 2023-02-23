// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "meminfo.h"
#include "macro.h"
#include "dstring.h"

#include <stdio.h>
#include <string>
#include <vector>

MemInfo::MemInfo()
{

}

MemoryInfo MemInfo::getMemoryInfo()
{
    MemoryInfo ret;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp)
        return ret;

    char line[MAX_LINE_LEN] = {0};
    while (fgets(line, MAX_LINE_LEN, fp)) {
        std::string info(line);
        std::vector<std::string> parts = DString::splitStr(info, ':');
        if (parts.size() != 2)
            continue;

        uint64_t num = std::stoll(parts[1]);
        if (parts[0] == "MemTotal") {
            ret.memTotal = num;
        }  else if (parts[0] == "MemFree") {
            ret.memFree = num;
        } else if (parts[0] == "MemAvailable") {
            ret.memAvailable = num;
        } else if (parts[0] == "Buffers") {
            ret.buffers = num;
        } else if (parts[0] == "Cached") {
            ret.cached = num;
        } else if (parts[0] == "SwapTotal") {
            ret.swapTotal = num;
        } else if (parts[0] == "SwapFree") {
            ret.swapFree = num;
        } else if (parts[0] == "SwapCached") {
            ret.swapCached = num;
        }
    }
    fclose(fp);

    return ret;
}

// IsSufficient check the memory whether reaches the qualified value
bool MemInfo::isSufficient(uint64_t minMemAvail, uint64_t maxSwapUsed)
{
    if (minMemAvail == 0)
        return true;

    MemoryInfo info = getMemoryInfo();
    uint64_t used = info.swapTotal - info.swapFree - info.swapCached;

    if (info.memAvailable < minMemAvail)
        return false;

    if (maxSwapUsed == 0 || info.memAvailable > used)
        return true;

    return used < maxSwapUsed;
}
