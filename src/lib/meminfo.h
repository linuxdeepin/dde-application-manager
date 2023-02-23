// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MEMINFO_H
#define MEMINFO_H

#include <stdint.h>

// MemoryInfo show the current memory stat, sum by kb
struct MemoryInfo
{
    uint64_t memTotal;
    uint64_t memFree;
    uint64_t memAvailable;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swapTotal;
    uint64_t swapFree;
    uint64_t swapCached;
};

class MemInfo
{
public:
    MemInfo();

    static MemoryInfo getMemoryInfo();

    static bool isSufficient(uint64_t minMemAvail, uint64_t maxSwapUsed);
};

#endif // MEMINFO_H
