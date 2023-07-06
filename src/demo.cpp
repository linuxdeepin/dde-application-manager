// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "demo.h"
#include <iostream>
#include <QString>

void greet()
{
    std::cout << QString{"Hello"}.toStdString() <<std::endl;
}

int main()
{
    greet();
    return 0;
}
