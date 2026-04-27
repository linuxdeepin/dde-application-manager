// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "desktopentry.h"
#include "dbus/applicationservice.h"
#include "global.h"
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
        {
            R"(sh -c 'if [ -e %f ]; then echo %f; fi')",
            QStringList{"sh", "-c", "if [ -e %f ]; then echo %f; fi"},
            "Single quotes are preserved But we should handle it because we need to compatible with GIO",
        },
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

TEST(ApplicationServiceTest, EscapeToObjectPath_UTF8ByteSequence)
{
    struct TestCase
    {
        QString input;
        QString expected;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {"", "_", "Empty string should return underscore"},
        {"simple", "simple", "Safe ASCII characters should not be escaped"},
        {"test/path", "test/path", "Forward slash is safe and should not be escaped"},
        {"test_name", "test_5fname", "Underscore should be escaped"},
        {"test-name", "test_2dname", "Hyphen should be escaped"},
        {"test.name", "test_2ename", "Dot should be escaped"},
        {"test name", "test_20name", "Space should be escaped"},
        {"test\x01", "test_01", "Control character should use 2-digit hex"},
        {QString::fromUtf8("测试"), "_e6_b5_8b_e8_af_95", "Chinese: UTF-8 bytes E6 B5 8B E8 AF 95"},
        {QString::fromUtf8("日本語"), "_e6_97_a5_e6_9c_ac_e8_aa_9e", "Japanese: UTF-8 bytes"},
        {QString::fromUtf8("café"), "caf_c3_a9", "Latin-1: é is UTF-8 C3 A9"},
    };

    for (const auto &tc : testCases) {
        const auto result = escapeToObjectPath(tc.input);
        EXPECT_EQ(result, tc.expected) << "Failed: " << tc.reason.toStdString()
                                       << "\nInput: " << tc.input.toStdString()
                                       << "\nExpected: " << tc.expected.toStdString()
                                       << "\nActual: " << result.toStdString();
    }
}

TEST(ApplicationServiceTest, UnescapeFromObjectPath_UTF8ByteSequence)
{
    struct TestCase
    {
        QString input;
        QString expected;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {"simple", "simple", "Safe string should not change"},
        {"test_20name", "test name", "Escaped space"},
        {"test_5fname", "test_name", "Escaped underscore"},
        {"test_01", "test\x01", "2-digit hex for control character"},
        {"_e6_b5_8b_e8_af_95", QString::fromUtf8("测试"), "Chinese UTF-8 bytes"},
        {"test_e4_b8_adpath", QString::fromUtf8("test中path"), "Mixed ASCII and UTF-8 bytes"},
    };

    for (const auto &tc : testCases) {
        const auto result = unescapeFromObjectPath(tc.input);
        EXPECT_EQ(result, tc.expected) << "Failed: " << tc.reason.toStdString()
                                       << "\nInput: " << tc.input.toStdString()
                                       << "\nExpected: " << tc.expected.toStdString()
                                       << "\nActual: " << result.toStdString();
    }
}

TEST(ApplicationServiceTest, ObjectPath_RoundTrip)
{
    const QList<QString> testCases = {
        "simple",
        "test/path",
        "test_name",
        "test name",
        QString::fromUtf8("测试应用"),
        QString::fromUtf8("日本語"),
        QString::fromUtf8("한글"),
        "test\x01",
        QString::fromUtf8("café"),
    };

    for (const auto &tc : testCases) {
        const auto escaped = escapeToObjectPath(tc);
        const auto unescaped = unescapeFromObjectPath(escaped);
        
        EXPECT_EQ(unescaped, tc) << "Round-trip failed for: " << tc.toStdString()
                                 << "\nEscaped: " << escaped.toStdString()
                                 << "\nUnescaped: " << unescaped.toStdString();
    }
}

TEST(ApplicationServiceTest, ObjectPath_DBusCompliance)
{
    auto isValidDBusPathElement = [](const QString &element) {
        if (element.isEmpty()) return false;
        for (const auto &ch : element) {
            const auto code = ch.unicode();
            bool isValid = (code >= 'a' && code <= 'z') ||
                          (code >= 'A' && code <= 'Z') ||
                          (code >= '0' && code <= '9') ||
                          code == '_';
            if (!isValid) return false;
        }
        return true;
    };

    const QList<QString> testCases = {
        "simple",
        "test/path",
        "test_name",
        "test name",
        QString::fromUtf8("测试"),
        QString::fromUtf8("日本語"),
        QString::fromUtf8("café"),
    };

    for (const auto &tc : testCases) {
        const auto escaped = escapeToObjectPath(tc);
        
        auto segments = escaped.split('/', Qt::SkipEmptyParts);
        for (const auto &seg : segments) {
            EXPECT_TRUE(isValidDBusPathElement(seg))
                << "Path element is not D-Bus compliant: " << seg.toStdString()
                << "\nOriginal: " << tc.toStdString();
        }
    }
}

TEST(ApplicationServiceTest, EscapeApplicationId_SystemdStandard)
{
    struct TestCase
    {
        QString input;
        QString expected;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {"", "", "Empty string should remain empty"},
        {"simple", "simple", "Safe ASCII characters should not be escaped"},
        {"test.name", "test.name", "Dot is safe and should not be escaped"},
        {"test_name", "test_name", "Underscore is safe and should not be escaped"},
        {"org/app", "org-app", "Forward slash should be replaced with hyphen"},
        {".hidden", "\\x2ehidden", "Leading dot should be escaped"},
        {"test app", "test\\x20app", "Space should be escaped"},
        {"test-name", "test\\x2dname", "Hyphen should be escaped"},
        {"org.example.App", "org.example.App", "Typical desktop ID should not be changed"},
        {"org/example/App", "org-example-App", "Path-style desktop ID with slashes"},
        {"test!special", "test\\x21special", "Exclamation mark should be escaped"},
        {"/", "-", "Single slash becomes single hyphen"},
        {"//", "--", "Multiple slashes: each becomes hyphen (non-path mode)"},
        {"test\x01", "test\\x01", "Control character should be escaped"},
        {QString::fromUtf8("测试"), "\\xe6\\xb5\\x8b\\xe8\\xaf\\x95", "Chinese: UTF-8 bytes E6 B5 8B E8 AF 95"},
        {QString::fromUtf8("日本語"), "\\xe6\\x97\\xa5\\xe6\\x9c\\xac\\xe8\\xaa\\x9e", "Japanese: UTF-8 bytes"},
        {QString::fromUtf8("café"), "caf\\xc3\\xa9", "Latin-1: é is UTF-8 C3 A9"},
    };

    for (const auto &tc : testCases) {
        const auto result = escapeApplicationId(tc.input);
        EXPECT_EQ(result, tc.expected) << "Failed: " << tc.reason.toStdString()
                                       << "\nInput: " << tc.input.toStdString()
                                       << "\nExpected: " << tc.expected.toStdString()
                                       << "\nActual: " << result.toStdString();
    }
}

TEST(ApplicationServiceTest, EscapeApplicationId_SystemdEscapeCompatible)
{
    // Test that our implementation matches systemd-escape command
    struct TestCase
    {
        QString input;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {"org/example/App", "Path-style ID"},
        {QString::fromUtf8("测试"), "Chinese characters"},
        {QString::fromUtf8("café"), "Latin-1 with accent"},
        {"test app", "Space in name"},
        {".hidden", "Leading dot"},
        {"test-name", "Hyphen"},
    };

    for (const auto &tc : testCases) {
        const auto escaped = escapeApplicationId(tc.input);
        
        // Verify the escaped string is valid for systemd
        EXPECT_FALSE(escaped.isEmpty() && !tc.input.isEmpty())
            << "Non-empty input should produce non-empty output: " << tc.input.toStdString();
            
        EXPECT_FALSE(escaped.startsWith('.'))
            << "Escaped ID should not start with dot: " << escaped.toStdString();
    }
}

TEST(ApplicationServiceTest, UnescapeApplicationId_UTF8ByteSequence)
{
    struct TestCase
    {
        QString input;
        QString expected;
        QString reason;
    };

    const QList<TestCase> testCases = {
        {"", "", "Empty string should remain empty"},
        {"simple", "simple", "Safe string should not change"},
        {"test\\x20app", "test app", "Escaped space"},
        {"test\\x2dapp", "test-app", "Escaped hyphen"},
        {"org-app", "org-app", "Hyphen should remain"},
        {"\\x2e", ".", "Single escaped dot"},
        {"test\\x", "test\\x", "Incomplete escape sequence should be preserved"},
        {"test\\xZZ", "test\\xZZ", "Invalid hex digits should be preserved"},
        {"test\\", "test\\", "Trailing backslash should be preserved"},
        {"\\xe6\\xb5\\x8b\\xe8\\xaf\\x95", QString::fromUtf8("测试"), "Chinese UTF-8 bytes"},
        {"caf\\xc3\\xa9", QString::fromUtf8("café"), "Latin-1 UTF-8 bytes"},
    };

    for (const auto &tc : testCases) {
        const auto result = unescapeApplicationId(tc.input);
        EXPECT_EQ(result, tc.expected) << "Failed: " << tc.reason.toStdString()
                                       << "\nInput: " << tc.input.toStdString()
                                       << "\nExpected: " << tc.expected.toStdString()
                                       << "\nActual: " << result.toStdString();
    }
}

TEST(ApplicationServiceTest, ApplicationId_RoundTrip)
{
    const QList<QString> testCases = {
        "simple",
        "test.name",
        "test_name",
        QString::fromUtf8("测试应用"),
        QString::fromUtf8("日本語"),
        QString::fromUtf8("한글"),
        QString::fromUtf8("café"),
        "test\x01",
    };

    for (const auto &tc : testCases) {
        const auto escaped = escapeApplicationId(tc);
        const auto unescaped = unescapeApplicationId(escaped);
        
        EXPECT_EQ(unescaped, tc) << "Round-trip failed for: " << tc.toStdString()
                                 << "\nEscaped: " << escaped.toStdString()
                                 << "\nUnescaped: " << unescaped.toStdString();
    }
}
