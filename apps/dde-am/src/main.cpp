// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QUrl>

#include "launcher.h"
#include "global.h"
#include <DUtil>

DCORE_USE_NAMESPACE

QString getAppIdFromInput(const QString &input)
{
    // Use QUrl::fromUserInput to handle both URIs and file paths
    // 优先直接用 DUtil::getAppIdFromAbsolutePath 判断
    QString appId = DUtil::getAppIdFromAbsolutePath(input);
    if (!appId.isEmpty()) {
        return appId;
    }
    // 如果失败，再尝试解析为本地文件路径
    QUrl url = QUrl::fromUserInput(input);
    if (!url.isLocalFile()) {
        return {};
    }
    QString path = url.toLocalFile();
    // fallback: 仅当文件存在且为.desktop时才用basename
    if (path.endsWith(".desktop") && QFileInfo::exists(path)) {
        QFileInfo fileInfo(path);
        appId = fileInfo.completeBaseName();
        return appId;
    }
    return {};
}

int main(int argc, char *argv[])
{
    QCoreApplication app{argc, argv};

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption listOption("list", "List all appId.");
    parser.addOption(listOption);
    QCommandLineOption launchedByUserOption("by-user",
                                            "Launched by user, it's useful for counting launched times.");
    parser.addOption(launchedByUserOption);
    QCommandLineOption actionOption(QStringList() << "a" << "action",
                                   "Specify action identifier for the application", "action");
    parser.addOption(actionOption);
    QCommandLineOption envOption(QStringList() << "e" << "env",
                                "Set environment variable, format: NAME=VALUE (can be used multiple times)", "env");
    parser.addOption(envOption);
    
    parser.addPositionalArgument("appId", "Application's ID, .desktop file path, or URI (e.g.: file:///path/to/app.desktop).");;

    parser.process(app);
    if (parser.isSet(listOption)) {
        const auto apps = Launcher::appIds();
        if (!apps) {
            qWarning() << apps.error();
            return apps.error().getErrorCode();
        }
        for (const auto &item :apps.value()) {
            qDebug() << qPrintable(item);
        }
        return 0;
    }

    QString appId;
    QString action;
    QStringList envVars;
    
    // Handle environment variables - prioritize -e/--env option
    if (parser.isSet(envOption)) {
        envVars.append(parser.values(envOption));
    }
    
    auto arguments = parser.positionalArguments();
    QString inputArg;
    if (!arguments.isEmpty()) {
        inputArg = arguments.takeFirst();
        appId = getAppIdFromInput(inputArg);
        // 支持 action 作为第二个位置参数
        if (!arguments.isEmpty()) {
            action = arguments.takeFirst();
        }
    } else {
        parser.showHelp();
    }

    // Handle action - prioritize --action option 
    if (parser.isSet(actionOption)) {
        action = parser.value(actionOption);
    }

    Launcher launcher;
    if (parser.isSet(launchedByUserOption))
        launcher.setLaunchedType(Launcher::ByUser);

    QString appPath;
    if (!appId.isEmpty()) {
        // 解析出 appId，说明输入是 appId 或 desktop 文件
        appPath = QString("%1/%2").arg(DDEApplicationManager1ObjectPath, escapeToObjectPath(appId));
    } else if (!inputArg.isEmpty() && !inputArg.startsWith("/")) {
        // 既不是绝对路径，也不是 desktop 文件等，视为 appId
        appPath = QString("%1/%2").arg(DDEApplicationManager1ObjectPath, escapeToObjectPath(inputArg));
    } else {
        // 绝对路径或特殊用法
        appPath = inputArg;
    }
    launcher.setPath(appPath);
    
    if (!action.isEmpty()) {
        launcher.setAction(action);
    }
    
    if (!envVars.isEmpty()) {
        launcher.setEnvironmentVariables(envVars);
    }

    auto ret = launcher.run();
    if (!ret) {
        qWarning() << ret.error();
        return ret.error().getErrorCode();
    }
    return 0;
}
