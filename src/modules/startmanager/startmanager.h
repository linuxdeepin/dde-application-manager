/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef STARTMANAGER_H
#define STARTMANAGER_H

#include <QObject>
#include <QMap>

class AppLaunchContext;
class StartManagerDBusHandler;
class DesktopInfo;
class QProcess;
class QFileSystemWatcher;
class ApplicationManager;

class StartManager : public QObject
{
    Q_OBJECT
public:
    explicit StartManager(QObject *parent = nullptr);

    bool addAutostart(QString fileName);
    bool removeAutostart(QString fileName);
    QStringList autostartList();
    QString dumpMemRecord();
    QString getApps();
    bool isAutostart(QString fileName);
    bool isMemSufficient();
    void launchApp(QString desktopFile, uint32_t timestamp, QStringList files);
    void launchAppAction(QString desktopFile, QString actionSection, uint32_t timestamp);
    void launchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options);
    void runCommand(QString exe, QStringList args);
    void runCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options);
    void tryAgain(bool launch);

Q_SIGNALS:
    void autostartChanged(QString status, QString fileName);

public Q_SLOTS:
    void onAutoStartupPathChange(const QString &dirPath);

private:
    bool setAutostart(QString fileName, bool value);
    bool doLaunchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options);
    bool launch(DesktopInfo *info, QString cmdLine, uint32_t timestamp, QStringList files);
    bool doRunCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options);
    void waitCmd(DesktopInfo *info, QProcess *process, QString cmdName);
    bool shouldUseProxy(QString appId);
    bool shouldDisableScaling(QString appId);
    void loadSysMemLimitConfig();
    QStringList getDefaultTerminal();
    void listenAutostartFileEvents();
    void startAutostartProgram();
    QStringList getAutostartList();

    uint64_t minMemAvail;
    uint64_t maxSwapUsed;
    StartManagerDBusHandler *dbusHandler;
    QStringList autostartFiles;
    QFileSystemWatcher *fileWatcher;
    ApplicationManager *am;
};

#endif // STARTMANAGER_H
