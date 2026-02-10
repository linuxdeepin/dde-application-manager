// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopentry.h"
#include "dbus/applicationservice.h"
#include <gtest/gtest.h>
#include <QList>
#include <QString>

TEST(ApplicationServiceTest, UnescapeValue_Standard)
{
    struct TestCase
    {
        QString input;
        QString expected;
        QString reason;
    };

    const QList<TestCase> testCases = {{R"(Space\sTest)", "Space Test", "Support for ASCII space"},
                                       {R"(Line1\nLine2)", "Line1\nLine2", "Support for newline"},
                                       {R"(Tab\tTest)", "Tab\tTest", "Support for tab"},
                                       {R"(Return\rTest)", "Return\rTest", "Support for carriage return"},
                                       {R"(Backslash\\Test)", R"(Backslash\Test)", "Support for backslash"},
                                       {R"(Value1\;Value2)", "Value1;Value2", "Support for semicolons"},
                                       {R"(\\s)", R"(\s)", R"(Double backslash before 's' should result in literal '\s')"},
                                       {R"(\s\n\t\r\\)", " \n\t\r\\", "Multiple escapes in sequence"},
                                       {R"(\)", R"(\)", "Trailing backslash at the end of string should be preserved as literal"},
                                       {R"(\")", R"(\")", "Double quote should NOT be unescaped in first pass"},
                                       {R"(\$)", R"(\$)", "Dollar sign should NOT be unescaped in first pass"},
                                       {R"(\b)", R"(\b)", "Unknown escape sequences should preserve the backslash"},
                                       {"/path/to/bi_na=ry", "/path/to/bi_na=ry", "Escaped characters should be preserved"}};

    for (const auto &tc : testCases) {
        const auto result = unescapeValue(tc.input);
        EXPECT_EQ(result, tc.expected) << "Failed: " << tc.reason.toStdString() << "\nInput: " << tc.input.toStdString();
    }
}

TEST(ApplicationServiceTest, SplitExecArguments_PassPhase2_Spec)
{
    struct TestCase
    {
        QString input;  // after first pass
        std::optional<QStringList> expected;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {R"(myapp arg1 %f)", QStringList{"myapp", "arg1", "%f"}, "Simple split and placeholder preservation"},
        {R"(myapp "quoted arg" next)", QStringList{"myapp", "quoted arg", "next"}, "Basic double quotes handling"},
        {R"(myapp "with \"internal\" quotes")",
         QStringList{"myapp", R"(with "internal" quotes)"},
         "Escaped quotes inside a quoted string"},
        {R"(myapp /path/with\ space)", QStringList{"myapp", "/path/with space"}, "Unquoted: backslash escapes a space"},
        {R"(myapp path\\with\\backslash)",
         QStringList{"myapp", R"(path\with\backslash)"},
         "Unquoted: double backslash becomes single literal backslash"},
        {R"(myapp "cost \$100")", QStringList{"myapp", "cost $100"}, "Quoted: dollar sign is a special char, backslash removed"},
        {R"(myapp "a\b")", QStringList{"myapp", R"(a\b)"}, "Quoted: 'b' is not special, so backslash is PRESERVED"},
        {R"(myapp "path\\with\\backslash")",
         QStringList{"myapp", R"(path\with\backslash)"},
         R"(Quoted: Passphase 2 reduces \\ to \ )"},
        {R"(myapp --icon=%i --file %f)",
         QStringList{"myapp", "--icon=%i", "--file", "%f"},
         "Field codes can be part of an argument"},
        {R"(myapp "unclosed quote)", std::nullopt, "Unclosed quote should return nullopt"},
        {R"(myapp \)", std::nullopt, "Trailing backslash is illegal"}};

    for (const auto &tc : testCases) {
        auto result = ApplicationService::splitExecArguments(tc.input);
        EXPECT_EQ(result, tc.expected) << "Input: " << tc.input.toStdString()
                                       << "\nExpected: " << tc.expected.value_or(QStringList{}).join('|').toStdString()
                                       << "\nActual: " << result.value_or(QStringList{}).join("|").toStdString()
                                       << "\nReason: " << tc.reason.toStdString();
    }
}
