// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopentry.h"
#include <gtest/gtest.h>
#include <QTextStream>
#include <QSharedPointer>
#include <QLocale>
#include <QDir>
#include <QFile>

class TestDesktopEntry : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        auto curDir = QDir::current();
        QString path{curDir.absolutePath() + "/data/desktopExample.desktop"};
        ParseError err;
        auto file = DesktopFile::searchDesktopFile(path,err);
        if (!file.has_value()) {
            qWarning() << "search " << path << "failed:" << err;
            return;
        }
        m_file.reset(new DesktopFile{std::move(file).value()});
    }
    void SetUp() override
    {
        
    }
    void TearDown() override {}
    QSharedPointer<DesktopFile> file() { return m_file; }
private:
    static inline QSharedPointer<DesktopFile> m_file;
};

TEST_F(TestDesktopEntry, desktopFile)
{
    const auto& fileptr = file();
    ASSERT_FALSE(fileptr.isNull());
    const auto &exampleFile = file();
    auto curDir = QDir::current();
    QString path{curDir.absolutePath() + "/data/desktopExample.desktop"};
    EXPECT_EQ(exampleFile->filePath().toStdString(), path.toStdString());
    EXPECT_EQ(exampleFile->desktopId().toStdString(), "");
}

TEST_F(TestDesktopEntry, prase)
{
    const auto &exampleFile = file();
    ASSERT_FALSE(exampleFile.isNull());;
    DesktopEntry entry;
    QFile in{exampleFile->filePath()};
    ASSERT_TRUE(in.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text));
    QTextStream fs{&in};
    auto err = entry.parse(fs);
    ASSERT_EQ(err, ParseError::NoError);

    auto group = entry.group("Desktop Entry");
    ASSERT_FALSE(group.isEmpty());

    auto name = group.constFind("Name");
    ASSERT_NE(name, group.cend());

    bool ok;
    name->toBoolean(ok);
    EXPECT_FALSE(ok);

    name->toNumeric(ok);
    EXPECT_FALSE(ok);

    auto defaultName = name->toString(ok);
    ASSERT_TRUE(ok);
    EXPECT_TRUE(defaultName == "Text Editor");

    auto localeString = name->toLocaleString(QLocale{"zh_CN"}, ok);
    ASSERT_TRUE(ok);

    EXPECT_TRUE(localeString == "文本编辑器");
}
