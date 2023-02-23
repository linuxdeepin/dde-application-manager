// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MACRO_H
#define MACRO_H

#define _likely_(x) (__builtin_expect(!!(x), 1))
#define _unlikely_(x) (__builtin_expect(!!(x), 0))

#define MAX(x, y) (x) > (y) ? (x) : (y)
#define MIN(x, y) (x) < (y) ? (x) : (y)

#define MAX_FILEPATH_LEN 256
#define MAX_LINE_LEN 256


#endif // MACRO_H
