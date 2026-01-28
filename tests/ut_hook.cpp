// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationHooks.h"
#include <gtest/gtest.h>
#include <QDir>
#include <QStringList>

TEST(ApplicationHookTest, load)
{
    auto hooksDir = QDir::current();
    ASSERT_TRUE(hooksDir.cdUp());
    ASSERT_TRUE(hooksDir.cdUp());
    ASSERT_TRUE(hooksDir.cd("tests/data/hooks.d"));

    auto file = hooksDir.absoluteFilePath("1-test.json");
    auto hook = ApplicationHook::loadFromFile(file);
    EXPECT_TRUE(hook);
    EXPECT_EQ(hook->hookName(), QString{"1-test.json"});
    EXPECT_EQ(hook->execPath(), QString{"/usr/bin/stat"});

    QStringList tmp{"for test"};
    EXPECT_EQ(hook->args(), tmp);

    tmp.push_front("/usr/bin/stat");
    auto elem = generateHooks({std::move(hook).value()});
    EXPECT_EQ(elem, tmp);
}
