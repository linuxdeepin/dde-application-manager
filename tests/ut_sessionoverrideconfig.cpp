// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationchecker.h"
#include "constant.h"
#include "desktopentry.h"
#include "global.h"
#include "sessionoverrideconfig.h"
#include "sessiontype.h"
#include <gtest/gtest.h>
#include <QTextStream>
#include <QTemporaryFile>

using namespace Qt::StringLiterals;

namespace {

void fillConfig(SessionOverrideConfig &config)
{
    config.m_overrides[u"org.example.app"_s] = {
        {u"Exec"_s, u"!AM_FULL! --ozone-platform=wayland"_s},
        {u"TryExec"_s, u"/usr/bin/env"_s},
        {u"Icon"_s, u"my-override-icon"_s}
    };
    config.m_overrides[u"org.example.hidden"_s] = {
        {u"TryExec"_s, u"/nonexistent/binary"_s}
    };
    config.m_overrides[u"org.example.forced"_s] = {
        {u"TryExec"_s, u""_s}
    };
    config.m_loaded = {
        u"org.example.app"_s,
        u"org.example.hidden"_s,
        u"org.example.forced"_s,
        u"org.example.missing"_s,
        u"org.example.other"_s
    };
}

bool parseDesktopEntry(DesktopEntry &entry, const QString &content)
{
    QTemporaryFile file;
    if (!file.open()) {
        return false;
    }
    QTextStream out(&file);
    out << content;
    out.flush();
    file.seek(0);
    return entry.parse(file) == ParserError::NoError;
}

}  // namespace

TEST(SessionOverrideConfigTest, GetValueReturnsOverrideForDesktopEntryGroup)
{
    SessionOverrideConfig config;
    fillConfig(config);

    const auto exec = config.getValue(u"org.example.app"_s, fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryExec));
    ASSERT_TRUE(exec.has_value());
    EXPECT_EQ(*exec, u"!AM_FULL! --ozone-platform=wayland"_s);

    const auto tryExec = config.getValue(u"org.example.app"_s, fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryTryExec));
    ASSERT_TRUE(tryExec.has_value());
    EXPECT_EQ(*tryExec, u"/usr/bin/env"_s);

    const auto icon = config.getValue(u"org.example.app"_s, fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryIcon));
    ASSERT_TRUE(icon.has_value());
    EXPECT_EQ(*icon, u"my-override-icon"_s);
}

TEST(SessionOverrideConfigTest, GetValueReturnsNulloptForNonDesktopEntryGroup)
{
    SessionOverrideConfig config;
    fillConfig(config);

    const auto actionGroup = QString(fromStaticRaw(DesktopFileActionKey)).append(u"new-window"_s);
    EXPECT_FALSE(config.getValue(u"org.example.app"_s, actionGroup, fromStaticRaw(DesktopEntryExec)).has_value());
    EXPECT_FALSE(config.getValue(u"org.example.app"_s, u"Arbitrary Group"_s, u"Exec"_s).has_value());
}

TEST(SessionOverrideConfigTest, GetValueReturnsNulloptForMissingFieldOrApp)
{
    SessionOverrideConfig config;
    fillConfig(config);

    EXPECT_FALSE(config.getValue(u"org.example.missing"_s, fromStaticRaw(DesktopFileEntryKey), u"Exec"_s).has_value());
    EXPECT_FALSE(config.getValue(u"org.example.app"_s, fromStaticRaw(DesktopFileEntryKey), u"NotExist"_s).has_value());
    EXPECT_FALSE(config.getValue(u"org.example.missing"_s, fromStaticRaw(DesktopFileEntryKey), u"TryExec"_s).has_value());
}

TEST(SessionOverrideConfigTest, HasOverrideDetectsPresence)
{
    SessionOverrideConfig config;
    fillConfig(config);

    EXPECT_TRUE(config.hasOverride(u"org.example.app"_s));
    EXPECT_TRUE(config.hasOverride(u"org.example.hidden"_s));
    EXPECT_TRUE(config.hasOverride(u"org.example.forced"_s));
    EXPECT_FALSE(config.hasOverride(u"org.example.missing"_s));
}

TEST(SessionOverrideConfigTest, ResolveExecValueSubstitutesFullPlaceholder)
{
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(u"!AM_FULL! --flag"_s, u"/usr/bin/prog %U"_s),
              u"/usr/bin/prog %U --flag"_s);
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(u"!AM_FULL!"_s, u"/usr/bin/prog"_s), u"/usr/bin/prog"_s);
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(u"/bin/env !AM_FULL!"_s, u"prog arg"_s), u"/bin/env prog arg"_s);
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(u"a !AM_FULL! b !AM_FULL!"_s, u"x"_s), u"a x b x"_s);
}

TEST(SessionOverrideConfigTest, ResolveExecValueKeepsValueWithoutPlaceholder)
{
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(u"/usr/bin/example --x11"_s, u"orig"_s), u"/usr/bin/example --x11"_s);
    EXPECT_EQ(SessionOverrideConfig::resolveExecValue(QString{}, u"orig"_s), QString{});
}

TEST(SessionOverrideConfigTest, TryExecCheckAppliesOverride)
{
    DesktopEntry entry;
    ASSERT_TRUE(parseDesktopEntry(entry,
                                  uR"([Desktop Entry]
Type=Application
Name=Example
Exec=/usr/bin/example
)"_s));

    SessionOverrideConfig config;
    fillConfig(config);

    // override points to an existing executable -> app should be shown
    EXPECT_FALSE(ApplicationFilter::tryExecCheck(entry, u"org.example.app"_s, &config));

    // override points to a missing binary -> app should be hidden
    EXPECT_TRUE(ApplicationFilter::tryExecCheck(entry, u"org.example.hidden"_s, &config));

    // empty override is treated as force-show
    EXPECT_FALSE(ApplicationFilter::tryExecCheck(entry, u"org.example.forced"_s, &config));
}

TEST(SessionOverrideConfigTest, TryExecCheckWithoutOverrideUsesDesktopEntry)
{
    DesktopEntry entry;
    ASSERT_TRUE(parseDesktopEntry(entry,
                                  uR"([Desktop Entry]
Type=Application
Name=Example
TryExec=/usr/bin/env
Exec=/usr/bin/example
)"_s));

    SessionOverrideConfig config;
    fillConfig(config);

    // no override for this app, fall back to desktop entry TryExec (/usr/bin/env exists) -> shown
    EXPECT_FALSE(ApplicationFilter::tryExecCheck(entry, u"org.example.other"_s, &config));
}

TEST(SessionTypeTest, DetectReturnsKnownValue)
{
    const auto type = currentSessionType();
    EXPECT_TRUE(type == SessionType::X11 || type == SessionType::Wayland || type == SessionType::Unknown);
}