// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopfileparser.h"
#include "global.h"
#include <gtest/gtest.h>
#include <QVariant>

using Groups = DesktopFileParser::Groups;

TEST(DesktopFileParserToString, emptyGroups)
{
    Groups groups;
    EXPECT_TRUE(toString(groups).isEmpty());
}

TEST(DesktopFileParserToString, missingMainEntry)
{
    Groups groups;
    groups.insert("Some Other Group", {{"Key", "value"}});
    EXPECT_TRUE(toString(groups).isEmpty());
}

TEST(DesktopFileParserToString, basicStringValues)
{
    Groups groups;
    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Name", "TestApp");
    entry.insert("Exec", "/usr/bin/test");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Entry]\n"));
    EXPECT_TRUE(result.contains("Type=Application\n"));
    EXPECT_TRUE(result.contains("Name=TestApp\n"));
    EXPECT_TRUE(result.contains("Exec=/usr/bin/test\n"));
    EXPECT_TRUE(result.endsWith("\n\n"));
}

TEST(DesktopFileParserToString, stringListValues)
{
    Groups groups;
    QMap<QString, QVariant> entry;
    entry.insert("MimeType", QStringList{"text/html", "text/xml", "image/png"});
    entry.insert("Type", "Application");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("MimeType=text/html;text/xml;image/png\n"));
}

TEST(DesktopFileParserToString, stringMapDefaultLocaleOnly)
{
    Groups groups;
    QMap<QString, QVariant> entry;
    QStringMap nameMap{{"default", "MyApp"}};
    entry.insert("Name", QVariant::fromValue(nameMap));
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("Name=MyApp\n"));
    EXPECT_FALSE(result.contains("Name[default]"));
}

TEST(DesktopFileParserToString, stringMapMultipleLocales)
{
    Groups groups;
    QMap<QString, QVariant> entry;
    QStringMap nameMap{{"default", "MyApp"}, {"zh_CN", QString::fromUtf8("我的应用")}, {"en_US", "My Application"}};
    entry.insert("Name", QVariant::fromValue(nameMap));
    entry.insert("Type", "Application");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("Name=MyApp\n"));
    EXPECT_TRUE(result.contains(QString::fromUtf8("Name[zh_CN]=我的应用\n")));
    EXPECT_TRUE(result.contains("Name[en_US]=My Application\n"));
}

TEST(DesktopFileParserToString, actionsBasic)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Name", "TestApp");
    entry.insert("Actions", "new;");
    entry.insert("Exec", "/usr/bin/test");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> actionEntry;
    actionEntry.insert("Name", "New Window");
    actionEntry.insert("Exec", "/usr/bin/test --new");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "new", actionEntry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Entry]\n"));
    EXPECT_TRUE(result.contains("[Desktop Action new]\n"));
    EXPECT_TRUE(result.contains("Name=New Window\n"));
    EXPECT_TRUE(result.contains("Exec=/usr/bin/test --new\n"));
}

TEST(DesktopFileParserToString, actionsWithMultipleLocales)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", "open;");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> actionEntry;
    QStringMap actionNameMap{{"default", "Open"}, {"zh_CN", QString::fromUtf8("打开")}};
    actionEntry.insert("Name", QVariant::fromValue(actionNameMap));
    actionEntry.insert("Exec", "/usr/bin/test --open");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "open", actionEntry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Action open]\n"));
    EXPECT_TRUE(result.contains("Name=Open\n"));
    EXPECT_TRUE(result.contains(QString::fromUtf8("Name[zh_CN]=打开\n")));
}

TEST(DesktopFileParserToString, actionsEmptyActionSkipped)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", ";;valid;;");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> actionEntry;
    actionEntry.insert("Name", "Valid");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "valid", actionEntry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Action valid]\n"));
}

TEST(DesktopFileParserToString, actionsNoActionGroupFound)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", "missing;");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Entry]\n"));
    EXPECT_TRUE(result.contains("Type=Application\n"));
}

TEST(DesktopFileParserToString, otherGroupsSerialized)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Name", "Test");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> extraEntry;
    extraEntry.insert("SomeKey", "SomeValue");
    groups.insert("X-Custom Group", extraEntry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Entry]\n"));
    EXPECT_TRUE(result.contains("[X-Custom Group]\n"));
    EXPECT_TRUE(result.contains("SomeKey=SomeValue\n"));
}

TEST(DesktopFileParserToString, actionGroupsNotDuplicated)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", "act1;");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> actionEntry;
    actionEntry.insert("Name", "Action1");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "act1", actionEntry);

    auto result = toString(groups);

    int count = 0;
    int idx = 0;
    while ((idx = result.indexOf("[Desktop Action act1]", idx)) != -1) {
        ++count;
        idx += 1;
    }
    EXPECT_EQ(count, 1);
}

TEST(DesktopFileParserToString, noActionsKeyInMainEntry)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Name", "NoActions");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Entry]\n"));
    EXPECT_TRUE(result.contains("Name=NoActions\n"));
    EXPECT_FALSE(result.contains("Desktop Action"));
}

TEST(DesktopFileParserToString, actionsEntryNotQStringWarning)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", QStringList{"act1"});
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> actionEntry;
    actionEntry.insert("Name", "Action1");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "act1", actionEntry);

    auto result = toString(groups);
    EXPECT_FALSE(result.isEmpty());
}

TEST(DesktopFileParserToString, mixedValueTypes)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    QStringMap commentMap{{"default", "A comment"}, {"fr", "Un commentaire"}};
    entry.insert("Comment", QVariant::fromValue(commentMap));
    entry.insert("Categories", QStringList{"Utility", "FileManager"});
    entry.insert("Version", "1.0");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("Comment=A comment\n"));
    EXPECT_TRUE(result.contains("Comment[fr]=Un commentaire\n"));
    EXPECT_TRUE(result.contains("Categories=Utility;FileManager\n"));
    EXPECT_TRUE(result.contains("Version=1.0\n"));
}

TEST(DesktopFileParserToString, multipleActionGroups)
{
    Groups groups;

    QMap<QString, QVariant> entry;
    entry.insert("Type", "Application");
    entry.insert("Actions", "new;open;save;");
    groups.insert(fromStaticRaw(DesktopFileEntryKey), entry);

    QMap<QString, QVariant> newAction;
    newAction.insert("Name", "New");
    newAction.insert("Exec", "/usr/bin/app --new");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "new", newAction);

    QMap<QString, QVariant> openAction;
    openAction.insert("Name", "Open");
    openAction.insert("Exec", "/usr/bin/app --open");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "open", openAction);

    QMap<QString, QVariant> saveAction;
    QStringMap saveNameMap{{"default", "Save"}, {"zh_CN", QString::fromUtf8("保存")}};
    saveAction.insert("Name", QVariant::fromValue(saveNameMap));
    saveAction.insert("Exec", "/usr/bin/app --save");
    groups.insert(fromStaticRaw(DesktopFileActionKey) % "save", saveAction);

    auto result = toString(groups);
    EXPECT_TRUE(result.contains("[Desktop Action new]\n"));
    EXPECT_TRUE(result.contains("[Desktop Action open]\n"));
    EXPECT_TRUE(result.contains("[Desktop Action save]\n"));
    EXPECT_TRUE(result.contains("Name=New\n"));
    EXPECT_TRUE(result.contains("Name=Open\n"));
    EXPECT_TRUE(result.contains("Name=Save\n"));
    EXPECT_TRUE(result.contains(QString::fromUtf8("Name[zh_CN]=保存\n")));
}
