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
