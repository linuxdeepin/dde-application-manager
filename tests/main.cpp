// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <gtest/gtest.h>
#include <QTimer>
#include <sanitizer/asan_interface.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    int ret{0};
    QTimer::singleShot(0, &app, [&ret] {
        ret = RUN_ALL_TESTS();
        QCoreApplication::quit();
    });
    __sanitizer_set_report_path("asan_am.log");
    QCoreApplication::exec();
    return ret;
}
