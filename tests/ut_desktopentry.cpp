// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "desktopentry.h"
#include "desktopfileparser.h"
#include <gtest/gtest.h>
#include <QTextStream>
#include <QSharedPointer>
#include <QLocale>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>

using namespace Qt::StringLiterals;

class TestDesktopEntry : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        env = qgetenv("XDG_DATA_DIRS");
        auto fakeXDG = QDir::current();
        ASSERT_TRUE(fakeXDG.cdUp());
        ASSERT_TRUE(fakeXDG.cdUp());
        ASSERT_TRUE(fakeXDG.cd("tests/data"));
        ASSERT_TRUE(qputenv("XDG_DATA_DIRS", fakeXDG.absolutePath().toLocal8Bit()));
        ParserError err;
        auto file = DesktopFile::searchDesktopFileById(u"deepin-editor"_s, err);
        if (!file.has_value()) {
            qWarning() << "search failed:" << err;
            return;
        }
        m_file.reset(new DesktopFile{std::move(file).value()});
    }

    static void TearDownTestCase() { qputenv("XDG_DATA_DIRS", env); }

    QSharedPointer<DesktopFile> file() { return m_file; }

private:
    static inline QSharedPointer<DesktopFile> m_file;
    static inline QByteArray env;
};

TEST_F(TestDesktopEntry, desktopFile)
{
    const auto &fileptr = file();
    ASSERT_FALSE(fileptr.isNull());
    const auto &exampleFile = file();
    auto desktopFileDir = QDir::current();
    ASSERT_TRUE(desktopFileDir.cdUp());
    ASSERT_TRUE(desktopFileDir.cdUp());
    ASSERT_TRUE(desktopFileDir.cd("tests/data/applications"));
    const auto &path = desktopFileDir.absoluteFilePath("deepin-editor.desktop");
    EXPECT_EQ(exampleFile->sourcePath(), path);
    EXPECT_EQ(exampleFile->desktopId().toStdString(), "deepin-editor");
}

TEST_F(TestDesktopEntry, prase)
{
    const auto &exampleFile = file();
    ASSERT_FALSE(exampleFile.isNull());
    DesktopEntry entry;
    QFile in{exampleFile->sourcePath()};
    ASSERT_TRUE(in.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text));
    auto err = entry.parse(in);
    ASSERT_EQ(err, ParserError::NoError);

    auto group = entry.group("Desktop Entry");
    ASSERT_TRUE(group);

    const auto &groupRef = group->get();
    auto name = groupRef.constFind("Name");
    ASSERT_NE(name, groupRef.cend());

    const auto &nameVal = *name;

    bool ok{true};
    toBoolean(nameVal, ok);
    EXPECT_FALSE(ok);

    toNumeric(nameVal, ok);
    EXPECT_FALSE(ok);

    EXPECT_TRUE(nameVal.canConvert<QStringMap>());
    auto defaultName = toString(nameVal);  // get default locale value.
    EXPECT_TRUE(defaultName == "Text Editor");

    auto localeString = toLocaleString(nameVal, QLocale{"zh_CN"});
    EXPECT_TRUE(localeString == "文本编辑器");
}

TEST(DesktopFileParser, desktopEntryMustBeFirstGroup)
{
    QTemporaryFile file;
    ASSERT_TRUE(file.open());
    QTextStream out(&file);
    out << "[Desktop Action one]\n"
           "Name=Action One\n\n"
           "[Desktop Entry]\n"
           "Type=Application\n"
           "Name=Example\n"
           "Exec=example\n";
    out.flush();
    file.seek(0);

    DesktopEntry entry;
    EXPECT_EQ(entry.parse(file), ParserError::InvalidFormat);
}

TEST(DesktopFileParser, serializeKeepsDesktopActionGroups)
{
    DesktopEntry entry;
    entry.insert(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryType), u"Application"_s);
    entry.insert(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryName), QVariant::fromValue(QStringMap{
                                                                                      {fromStaticRaw(DesktopFileDefaultKeyLocale), u"Example"_s}}));
    entry.insert(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryExec), u"example"_s);
    entry.insert(fromStaticRaw(DesktopFileEntryKey), fromStaticRaw(DesktopEntryActions), u"one;two;"_s);
    entry.insert(fromStaticRaw(DesktopFileActionKey) % u"one"_s, fromStaticRaw(DesktopEntryName), QVariant::fromValue(QStringMap{
                                                                                           {fromStaticRaw(DesktopFileDefaultKeyLocale), u"Action One"_s}}));
    entry.insert(fromStaticRaw(DesktopFileActionKey) % u"one"_s, fromStaticRaw(DesktopEntryExec), u"example --one"_s);
    entry.insert(fromStaticRaw(DesktopFileActionKey) % u"two"_s, fromStaticRaw(DesktopEntryName), QVariant::fromValue(QStringMap{
                                                                                           {fromStaticRaw(DesktopFileDefaultKeyLocale), u"Action Two"_s}}));
    entry.insert(fromStaticRaw(DesktopFileActionKey) % u"two"_s, fromStaticRaw(DesktopEntryExec), u"example --two"_s);

    const auto serialized = toString(entry.data());
    EXPECT_TRUE(serialized.contains(u"[Desktop Action one]\n"_s));
    EXPECT_TRUE(serialized.contains(u"[Desktop Action two]\n"_s));
    EXPECT_TRUE(serialized.contains(u"Exec=example --one\n"_s));
    EXPECT_TRUE(serialized.contains(u"Exec=example --two\n"_s));
}
