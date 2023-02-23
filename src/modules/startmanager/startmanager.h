// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    bool addAutostart(const QString &desktop);
    bool removeAutostart(const QString &desktop);
    QStringList autostartList();
    bool isAutostart(const QString &desktop);
    bool isMemSufficient();
    bool launchApp(const QString &desktopFile);
    bool launchApp(QString desktopFile, uint32_t timestamp, QStringList files);
    bool launchAppAction(QString desktopFile, QString actionSection, uint32_t timestamp);
    bool launchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QVariantMap options);
    bool runCommand(QString exe, QStringList args);
    bool runCommandWithOptions(QString exe, QStringList args, QVariantMap options);

Q_SIGNALS:
    void autostartChanged(const QString &status, const QString &fileName);

public Q_SLOTS:
    void onAutoStartupPathChange(const QString &dirPath);

private:
    bool setAutostart(const QString &fileName, const bool value);
    bool doLaunchAppWithOptions(const QString &desktopFile);
    bool doLaunchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QVariantMap options);
    bool launch(DesktopInfo *info, QString cmdLine, uint32_t timestamp, QStringList files);
    bool doRunCommandWithOptions(QString exe, QStringList args, QVariantMap options);
    void waitCmd(DesktopInfo *info, QProcess *process, QString cmdName);
    bool shouldUseProxy(QString appId);
    bool shouldDisableScaling(QString appId);
    void loadSysMemLimitConfig();
    QStringList getDefaultTerminal();
    void listenAutostartFileEvents();
    void startAutostartProgram();
    QStringList getAutostartList();
    QMap<QString, QString> getDesktopToAutostartMap();
    void setIsDBusCalled(const bool state);
    bool isDBusCalled() const;
    void handleRecognizeArgs(QStringList &exeArgs, QStringList files);

    uint64_t minMemAvail;
    uint64_t maxSwapUsed;
    StartManagerDBusHandler *dbusHandler;
    QStringList m_autostartFiles;
    QMap<QString, QString> m_desktopDirToAutostartDirMap;   // Desktop全路径和自启动目录
    QFileSystemWatcher *m_autostartFileWatcher;
    bool m_isDBusCalled;
};

#endif // STARTMANAGER_H
