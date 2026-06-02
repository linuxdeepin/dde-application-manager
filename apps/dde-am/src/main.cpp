// SPDX-FileCopyrightText: 2024 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFileInfo>
#include <QUrl>

#include <DUtil>

#include "launcher.h"
#include "commandexecutor.h"
#include "global.h"

DCORE_USE_NAMESPACE
namespace {

QString getAppIdFromInput(const QString &input)
{
    // Use QUrl::fromUserInput to handle both URIs and file paths
    // 优先直接用 DUtil::getAppIdFromAbsolutePath 判断
    QString appId = DUtil::getAppIdFromAbsolutePath(input);
    if (!appId.isEmpty()) {
        return appId;
    }

    // 如果失败，再尝试解析为本地文件路径
    const QUrl url = QUrl::fromUserInput(input);
    if (!url.isLocalFile()) {
        return {};
    }

    const QString path = url.toLocalFile();
    // fallback: 仅当文件存在且为.desktop时才用basename
    if (path.endsWith(desktopSuffix) && QFileInfo::exists(path)) {
        const QFileInfo fileInfo(path);
        appId = fileInfo.completeBaseName();
        return appId;
    }
    return {};
}

int handleList()
{
    const auto apps = Launcher::appIds();
    if (!apps) {
        qWarning() << apps.error();
        return static_cast<int>(apps.error().getErrorCode());
    }

    for (const auto &item : apps.value()) {
        qDebug() << qPrintable(item);
    }

    return 0;
}

int handleExecuteCommand(const QCommandLineParser &parser,
                         const QCommandLineOption &executeOption,
                         const QCommandLineOption &typeOption,
                         const QCommandLineOption &runIdOption,
                         const QCommandLineOption &workdirOption,
                         const QCommandLineOption &envOption)
{
    CommandExecutor executor;
    if (!executor.setProgram(parser.value(executeOption))) {
        return -1;
    }

    if (parser.isSet(typeOption)) {
        executor.setType(parser.value(typeOption));
    } else {
        executor.setType("portablebinary");
    }

    if (parser.isSet(runIdOption)) {
        executor.setRunId(parser.value(runIdOption));
    }

    if (parser.isSet(workdirOption)) {
        executor.setWorkDir(parser.value(workdirOption));
    } else {
        executor.setWorkDir(QDir::currentPath());
    }

    if (parser.isSet(envOption)) {
        executor.setEnvironmentVariables(parser.values(envOption));
    }

    auto args = parser.positionalArguments();
    executor.setArguments(args);

    auto ret = executor.execute();
    if (!ret) {
        qWarning() << ret.error();
        return static_cast<int>(ret.error().getErrorCode());
    }

    return 0;
}

int handleLaunchApp(const QCommandLineParser &parser,
                    const QCommandLineOption &envOption,
                    const QCommandLineOption &actionOption,
                    const QCommandLineOption &launchedByUserOption,
                    const QCommandLineOption &autostartOption,
                    const QCommandLineOption &launchTypeOption,
                    const QStringList &extraArgs)
{
    QString appId;
    QString action;
    QStringList envVars;

    // Handle environment variables - prioritize -e/--env option
    if (parser.isSet(envOption)) {
        envVars.append(parser.values(envOption));
    }

    auto arguments = parser.positionalArguments();
    // Remove extra args from the end of positional arguments.
    // After '--', QCommandLineParser treats all remaining args as positional,
    // but we already extracted them separately from argv.
    if (!extraArgs.isEmpty()) {
        for (int i = 0; i < extraArgs.size() && !arguments.isEmpty(); ++i) {
            arguments.removeLast();
        }
    }
    QString inputArg;
    if (!arguments.isEmpty()) {
        inputArg = arguments.takeFirst();
        appId = getAppIdFromInput(inputArg);
        // 支持 action 作为第二个位置参数
        if (!arguments.isEmpty()) {
            action = arguments.takeFirst();
        }
    }

    // Handle action - prioritize --action option
    if (parser.isSet(actionOption)) {
        action = parser.value(actionOption);
    }

    Launcher launcher;
    if (parser.isSet(launchedByUserOption)) {
        launcher.setLaunchedType(Launcher::Type::ByUser);
    }

    if (parser.isSet(autostartOption)) {
        launcher.setAutostart(true);
    }

    if (parser.isSet(launchTypeOption)) {
        launcher.setLaunchType(parser.value(launchTypeOption));
    }

    QString appPath;
    if (!appId.isEmpty()) {
        // 解析出 appId，说明输入是 appId 或 desktop 文件
        appPath = fromStaticRaw(DDEApplicationManager1ObjectPath) % u'/' % DUtil::escapeToObjectPath(appId);
    } else if (!inputArg.isEmpty() && !inputArg.startsWith(u'/')) {
        // 既不是绝对路径，也不是 desktop 文件等，视为 appId
        appPath = fromStaticRaw(DDEApplicationManager1ObjectPath) % u'/' % DUtil::escapeToObjectPath(inputArg);
    } else {
        // 绝对路径或特殊用法
        qWarning() << "unknown input: " << inputArg << ", just use it as path.";
        appPath = inputArg;
    }
    launcher.setPath(appPath);

    if (!action.isEmpty()) {
        launcher.setAction(action);
    }

    if (!envVars.isEmpty()) {
        launcher.setEnvironmentVariables(envVars);
    }

    if (!extraArgs.isEmpty()) {
        launcher.setExtraArgs(extraArgs);
    }

    auto ret = launcher.run();
    if (!ret) {
        qWarning() << ret.error();
        return static_cast<int>(ret.error().getErrorCode());
    }

    return 0;
}

}  // namespace

int main(int argc, char *argv[])
{
    const QCoreApplication app{argc, argv};

    // Extract extra arguments after '--' for launch mode.
    // In launch mode, '--' separates dde-am options from extra arguments
    // passed to the application's Exec command.
    // In command mode ('-c'), '--' is handled by QCommandLineParser normally.
    QStringList extraArgs;
    {
        bool foundDashDash = false;
        for (int i = 1; i < argc; ++i) {
            if (qstrcmp(argv[i], "--") == 0) {
                foundDashDash = true;
                continue;
            }
            if (foundDashDash) {
                extraArgs.append(QString::fromLocal8Bit(argv[i]));
            }
        }
    }

    QCommandLineParser parser;

    parser.setApplicationDescription("Deepin Application Manager Client\n\n"
                                     "Usage:\n"
                                     "  dde-am [options] <appId> [-- extra-args]  Launch application\n"
                                     "  dde-am -c <program> [args...]             Execute a command\n"
                                     "  dde-am --list                             List all applications\n\n"
                                     "Examples:\n"
                                     "  dde-am org.deepin.calculator              # Launch by ID\n"
                                     "  dde-am org.deepin.dde.control-center -- --show -p \"wallpaper\"\n"
                                     "  dde-am -c /usr/bin/ls -- -l               # Execute command with args");
    parser.addHelpOption();
    parser.addVersionOption();

    // Application Launching Options
    const QCommandLineOption listOption("list", "List all installed application IDs.");
    parser.addOption(listOption);
    const QCommandLineOption launchedByUserOption("by-user", "Mark launch as initiated by user (useful for usage statistics).");
    parser.addOption(launchedByUserOption);

    // autostart is an undocumented option, this option is only used by dde-autostart, DO NOT SHOW IT TO USERS.
    QCommandLineOption autostartOption("autostart", "Launched by system autostart. Suppress prelaunch splash.");
    autostartOption.setFlags(QCommandLineOption::Flag::HiddenFromHelp);
    parser.addOption(autostartOption);

    const QCommandLineOption launchTypeOption("launch-type", "Launch source name for event reporting (e.g. dde-launchpad, dde-shell).", "type");
    parser.addOption(launchTypeOption);

    const QCommandLineOption actionOption({"a", "action"}, "Specify action identifier to trigger for the application.", "action");
    parser.addOption(actionOption);

    // Common Options
    const QCommandLineOption envOption(
        {"e", "env"}, "Set environment variable, format: NAME=VALUE (can be used multiple times).", "env");
    parser.addOption(envOption);

    // Command Execution Options
    const QCommandLineOption executeOption({"c", "command"}, "Execute a command/program directly.", "program");
    parser.addOption(executeOption);

    const QCommandLineOption typeOption("type",
                                        "Execution type (shortcut, script, portablebinary). Default: portablebinary.\n"
                                        "Only valid with --command.",
                                        "type");
    parser.addOption(typeOption);

    const QCommandLineOption runIdOption("run-id",
                                         "Custom Run ID for the execution.\n"
                                         "Only valid with --command.",
                                         "runId");
    parser.addOption(runIdOption);

    const QCommandLineOption workdirOption("workdir",
                                           "Working directory for the execution.\n"
                                           "Only valid with --command.",
                                           "dir");
    parser.addOption(workdirOption);

    parser.addPositionalArgument("appId",
                                 "Application ID, .desktop file path, or URI to launch.\n"
                                 "If --command is specified, this acts as arguments for the command "
                                 "(use '--' before arguments starting with '-').");

    parser.process(app);

    // Validate mode and arguments
    const bool hasCommand = parser.isSet(executeOption);
    const QStringList posArgs = parser.positionalArguments();

    if (parser.isSet(listOption)) {
        return handleList();
    }

    if (hasCommand) {
        // Command mode: positional args are command arguments
        return handleExecuteCommand(parser, executeOption, typeOption, runIdOption, workdirOption, envOption);
    }

    // Launch mode: positional args must contain appId
    if (posArgs.isEmpty()) {
        qCritical() << "Error: Missing appId. Use --help for usage information.";
        return 1;
    }

    return handleLaunchApp(parser, envOption, actionOption, launchedByUserOption, autostartOption, launchTypeOption, extraArgs);
}
