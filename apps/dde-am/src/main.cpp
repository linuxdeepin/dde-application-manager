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
    QCommandLineOption fieldsOption(QStringList() << "f" << "fields",
                                   "Set launch fields, can be used multiple times", "field");
    parser.addOption(fieldsOption);
    
    parser.addPositionalArgument("appId", "Application's ID, .desktop file path, or URI (e.g.: file:///path/to/app.desktop).");;

    // 优雅地处理 -- 分隔符
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    auto argsSplittorPos = args.indexOf(QStringLiteral("--"));
    QStringList arguments;
    QStringList fieldsAfterDoubleDash;

    if (argsSplittorPos != -1) {
        while (argsSplittorPos > 0) {
            arguments << args.takeFirst();
            argsSplittorPos--;
        }
        args.removeFirst(); // 移除 "--"
        fieldsAfterDoubleDash = args; // 剩余的就是 fields
    } else {
        arguments = args;
    }

    // 使用处理过的参数解析
    if (!parser.parse(arguments)) {
        qDebug() << "Parse error:" << parser.errorText();
        parser.showHelp();
        return 1;
    }
    
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
    QStringList fields;
    
    // 处理 -- 之后的参数作为 fields
    if (!fieldsAfterDoubleDash.isEmpty()) {
        fields.append(fieldsAfterDoubleDash);
    }
    
    // Handle environment variables - prioritize -e/--env option
    if (parser.isSet(envOption)) {
        envVars.append(parser.values(envOption));
    }
    // Handle fields - prioritize -f/--fields option
    if (parser.isSet(fieldsOption)) {
        fields.append(parser.values(fieldsOption));
    }
    
    // 从处理前的参数中获取第一个位置参数（appId）
    QString inputArg;
    if (!arguments.isEmpty()) {
        // 移除所有选项参数，找到第一个位置参数
        QStringList tempArgs = arguments;
        
        // 移除已知的选项及其值
        if (parser.isSet(actionOption)) {
            tempArgs.removeAll("-a");
            tempArgs.removeAll("--action");
            tempArgs.removeAll(parser.value(actionOption));
        }
        if (parser.isSet(envOption)) {
            tempArgs.removeAll("-e");
            tempArgs.removeAll("--env");
            for (const auto &env : parser.values(envOption)) {
                tempArgs.removeAll(env);
            }
        }
        if (parser.isSet(fieldsOption)) {
            tempArgs.removeAll("-f");
            tempArgs.removeAll("--fields");
            for (const auto &field : parser.values(fieldsOption)) {
                tempArgs.removeAll(field);
            }
        }
        tempArgs.removeAll("--by-user");
        tempArgs.removeAll("--list");
        
        if (!tempArgs.isEmpty()) {
            inputArg = tempArgs.first();
            appId = getAppIdFromInput(inputArg);
            // 如果有第二个参数，作为 action
            if (tempArgs.size() > 1) {
                action = tempArgs.at(1);
            }
        }
    }
    
    if (inputArg.isEmpty()) {
        parser.showHelp();
        return 1;
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
    if (!fields.isEmpty()) {
        launcher.setFields(fields);
    }

    auto ret = launcher.run();
    if (!ret) {
        qWarning() << ret.error();
        return ret.error().getErrorCode();
    }
    return 0;
}
