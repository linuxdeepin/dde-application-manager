// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationservice.h"
#include <gtest/gtest.h>
#include <QString>
#include <QTextStream>

QT_BEGIN_NAMESPACE
void PrintTo(const QStringList& list, ::std::ostream* os) {
    *os << "QStringList(";
    bool first = true;
    for (const QString& s : list) {
        if (!first) {
            *os << ", ";
        }
        *os << "\"" << qPrintable(s) << "\""; // Enclose each QString in quotes
        first = false;
    }
    *os << ")";
}
QT_END_NAMESPACE

TEST(UnescapeExec, blankSpace)
{
    QList<std::pair<QString, QList<QString>>> testCases{
        {
            R"(/usr/bin/hello\sworld --arg1=val1 -h --str="rrr ggg bbb")",
            {
                R"(/usr/bin/hello\sworld)",
                "--arg1=val1",
                "-h",
                "--str=rrr ggg bbb",
            },
        },
        {
            R"("/usr/bin/hello world" a b -- "c d")",
            {
                "/usr/bin/hello world",
                "a",
                "b",
                "--",
                "c d",
            },
        },
        {
            R"("/usr/bin/hello\t\nworld" a b -- c d)",
            {
                "/usr/bin/hello\t\nworld",
                "a",
                "b",
                "--",
                "c",
                "d",
            },
        },
    };

    for (auto &testCase : testCases) {
        EXPECT_EQ(ApplicationService::unescapeExecArgs(testCase.first), testCase.second);
    }
}

TEST(UnescapeExec, dollarSignEscape)
{
    QList<std::pair<QString, QList<QString>>> testCases{
        {
            R"(/path/to/file$name.txt)",  // Input: 1 $
            {
                "/path/to/file$$name.txt",  // Output: 2 $
            },
        },
        {
            R"(/path/to/file$$name.txt)",  // Input: 2 $
            {
                "/path/to/file$$$$name.txt",  // Output: 4 $
            },
        },
        {
            R"(/home/user/document$$2023.pdf)",  // Input: 2 $
            {
                "/home/user/document$$$$2023.pdf",  // Output: 4 $
            },
        },
        {
            R"(/usr/bin/app $$double-dollar-test)",  // Input: 2 $
            {
                "/usr/bin/app",
                "$$$$double-dollar-test",  // Output: 4 $
            },
        },
        {
            R"(/mixed/path$$with/normal/parts.txt)",  // Input: 2 $
            {
                "/mixed/path$$$$with/normal/parts.txt",  // Output: 4 $
            },
        },
        {
            R"(/usr/bin/app --file=/path/with$$dollars.txt --other=normal)",  // Input: 2 $
            {
                "/usr/bin/app",
                "--file=/path/with$$$$dollars.txt",  // Output: 4 $
                "--other=normal",
            },
        },
    };

    for (auto &testCase : testCases) {
        auto result = ApplicationService::unescapeExecArgs(testCase.first);
        EXPECT_EQ(result, testCase.second) << "Failed for input: " << testCase.first.toStdString();
    }
}
