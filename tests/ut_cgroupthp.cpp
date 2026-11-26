// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "cgroupthp.h"

#include <gtest/gtest.h>

using namespace Qt::StringLiterals;

TEST(CgroupThpTest, AcceptsModesSupportedByKernel)
{
    EXPECT_TRUE(CgroupThp::isValidMode(u"inherit"_s));
    EXPECT_TRUE(CgroupThp::isValidMode(u"default"_s));
    EXPECT_TRUE(CgroupThp::isValidMode(u"disable"_s));
    EXPECT_TRUE(CgroupThp::isValidMode(u"force_enable"_s));
}

TEST(CgroupThpTest, RejectsUnknownModes)
{
    EXPECT_FALSE(CgroupThp::isValidMode(u""_s));
    EXPECT_FALSE(CgroupThp::isValidMode(u"enable"_s));
    EXPECT_FALSE(CgroupThp::isValidMode(u"force-enable"_s));
    EXPECT_FALSE(CgroupThp::isValidMode(u"FORCE_ENABLE"_s));
    EXPECT_FALSE(CgroupThp::isValidMode(u" never "_s));
}
