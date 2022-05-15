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

#include "startmanager.h"
#include "basedir.h"
#include "dfile.h"
#include "common.h"
#include "desktopinfo.h"
#include "startmanagersettings.h"
#include "startmanagerdbushandler.h"
#include "meminfo.h"

#include <QFileSystemWatcher>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QThread>

#define DESKTOPEXT ".desktop"
#define SETTING StartManagerSettings::instance()

StartManager::StartManager(QObject *parent)
    : QObject(parent)
    , minMemAvail(0)
    , maxSwapUsed(0)
    , dbusHandler(new StartManagerDBusHandler(this))
    , fileWatcher(new QFileSystemWatcher(this))
{
    // load sysMemLimitConfig
    loadSysMemLimitConfig();

    autostartFiles = getAutostartList();

    // listen autostart files
    listenAutostartFileEvents();

    // start autostart
    // TODO only running once when starting system
    //startAutostartProgram();
}

/**
 * @brief StartManager::addAutostart
 * @param fileName desktopFile
 * @return
 */
bool StartManager::addAutostart(QString fileName)
{
    return setAutostart(fileName, true);
}

/**
 * @brief StartManager::removeAutostart
 * @param fileName desktopFile
 * @return
 */
bool StartManager::removeAutostart(QString fileName)
{
    return setAutostart(fileName, false);
}

QStringList StartManager::autostartList()
{
    if (autostartFiles.size() == 0) {
        autostartFiles = getAutostartList();
    }

    return autostartFiles;
}

QString StartManager::dumpMemRecord()
{

}

QString StartManager::getApps()
{

}

/**
 * @brief StartManager::isAutostart
 * @param fileName  desktopFile
 * @return
 */
bool StartManager::isAutostart(QString fileName)
{
    if (!fileName.endsWith(DESKTOPEXT))
        return false;

    for (auto autostartDir : BaseDir::autoStartDirs()) {
        std::string filePath = autostartDir + fileName.toStdString();
        if (DFile::isExisted(filePath)) {
            DesktopInfo info(filePath);
            if (info.isValidDesktop() && !info.getIsHidden()) {
                return true;
            }
        }
    }

    return false;
}

bool StartManager::isMemSufficient()
{
    return SETTING->getMemCheckerEnabled() ? MemInfo::isSufficient(minMemAvail, maxSwapUsed) : true;
}

void StartManager::launchApp(QString desktopFile, uint32_t timestamp, QStringList files)
{
    doLaunchAppWithOptions(desktopFile, timestamp, files, QMap<QString, QString>());
}

void StartManager::launchAppAction(QString desktopFile, QString actionSection, uint32_t timestamp)
{
    DesktopInfo info(desktopFile.toStdString());
    if (!info.isValidDesktop())
        return;

    DesktopAction targetAction;
    for (auto action : info.getActions()) {
        if (!action.section.empty() && action.section.c_str() == actionSection) {
            targetAction = action;
            break;
        }
    }

    if (targetAction.section.empty()) {
        qWarning() << "launchAppAction: targetAction section is empty";
        return;
    }

    if (targetAction.exec.empty()) {
        qInfo() << "launchAppAction: targetAction exe is empty";
        return;
    }

    launch(&info, targetAction.exec.c_str(), timestamp, QStringList());

    // mark app launched
    dbusHandler->markLaunched(desktopFile);
}

void StartManager::launchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options)
{
    doLaunchAppWithOptions(desktopFile, timestamp, files, options);
}

void StartManager::runCommand(QString exe, QStringList args)
{
    doRunCommandWithOptions(exe, args, QMap<QString, QString>());
}

void StartManager::runCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options)
{
    doRunCommandWithOptions(exe, args, options);
}

void StartManager::tryAgain(bool launch)
{

}

void StartManager::onAutoStartupPathChange(const QString &dirPath)
{
    QStringList autostartFilesList = getAutostartList();
    QSet<QString> newAutostartFiles = QSet<QString>::fromList(autostartFilesList);
    QSet<QString> oldAutostartFiles = QSet<QString>::fromList(autostartFiles);

    // 添加
    QSet<QString> newFiles = newAutostartFiles - oldAutostartFiles;
    QStringList newFile = newFiles.toList();

    // 移除
    QSet<QString> deletedFiles = oldAutostartFiles - newAutostartFiles;
    QStringList deleteFile = deletedFiles.toList();

    // 更新autostartFiles记录
    autostartFiles = autostartFilesList;

    for (auto &file : newFile) {
        Q_EMIT autostartChanged(autostartAdded, file);
    }

    for (auto &file : deleteFile) {
        Q_EMIT autostartChanged(autostartDeleted, file);
    }
}

bool StartManager::setAutostart(QString fileName, bool value)
{
    if (!fileName.endsWith(DESKTOPEXT))
        return false;

    if (isAutostart(fileName) == value)
        return true;

    QFileInfo info(fileName);
    QString appId = info.baseName();
    QString autostartDir(BaseDir::userAutoStartDir().c_str());
    QString autostartFileName = fileName;
    bool isUserAutostart = false;
    // if has no user autostart file, create it
    if (info.isAbsolute()) {
        if (info.exists() && info.baseName() == autostartDir) {
            isUserAutostart = true;
        }
    } else {
        autostartFileName = autostartDir + fileName;
        if (DFile::isExisted(autostartFileName.toStdString())) {
            isUserAutostart = true;
        }
    }

    if (!isUserAutostart && !DFile::isExisted(autostartFileName.toStdString())) {
        // get system autostart desktop file
        for (auto appDir : BaseDir::appDirs()) {
            QDir dir(appDir.c_str());
            dir.setFilter(QDir::Files);
            dir.setNameFilters({ "*.desktop" });
            bool hiddenStatusChanged = false;
            for (auto entry : dir.entryInfoList()) {
                QString desktopFile = entry.absoluteFilePath();
                if (!desktopFile.contains(fileName))
                    continue;

                if (!QFile::copy(desktopFile, autostartFileName))   // copy origin file
                    return false;

                hiddenStatusChanged = true;
                break;
            }

            if (hiddenStatusChanged)
                break;
        }
    }


    // change autostart hidden status in file
    KeyFile kf;
    kf.loadFile(autostartFileName.toStdString());
    kf.setKey(MainSection, KeyXDeepinCreatedBy.toStdString(), AMServiceName.toStdString());
    kf.setKey(MainSection, KeyXDeepinAppID.toStdString(), appId.toStdString());
    kf.setBool(MainSection, KeyHidden, !value ? "true" : "false");
    kf.saveToFile(autostartFileName.toStdString());

    if (value && autostartFiles.indexOf(fileName) != -1) {
        autostartFiles.push_back(fileName);
    } else if (!value) {
        autostartFiles.removeAll(fileName);
    }

    Q_EMIT autostartChanged(value ? autostartAdded : autostartDeleted, fileName);
    return true;
}

bool StartManager::doLaunchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options)
{
    // launchApp
    DesktopInfo info(desktopFile.toStdString());
    if (!info.isValidDesktop())
        return false;

    if (options.find("path") != options.end()) {
        info.getKeyFile()->setKey(MainSection, KeyPath, options["path"].toStdString());
    }

    if (options.find("desktop-override-exec") != options.end()) {
        info.setDesktopOverrideExec(options["desktop-override-exec"].toStdString());
    }

    if (info.getCommandLine().empty()) {
        qInfo() << "command line is empty";
        return false;
    }

    launch(&info, info.getCommandLine().c_str(), timestamp, files);

    // mark app launched
    dbusHandler->markLaunched(desktopFile);

    return true;
}

bool StartManager::launch(DesktopInfo *info, QString cmdLine, uint32_t timestamp, QStringList files)
{
    QProcess process;
    QStringList cmdPrefixesEnvs;
    QStringList envs;
    QString desktopFile(info->getFileName().c_str());
    QFileInfo fileInfo(desktopFile);
    QString appId = fileInfo.baseName();

    bool useProxy = shouldUseProxy(appId);
    for (QString var : QProcess::systemEnvironment()) {
        if (useProxy && (var.startsWith("auto_proxy")
                         || var.startsWith("http_proxy")
                         || var.startsWith("https_proxy")
                         || var.startsWith("ftp_proxy")
                         || var.startsWith("all_proxy")
                         || var.startsWith("SOCKS_SERVER")
                         || var.startsWith("no_proxy"))) {
            continue;
        }

        envs << var;
    }

    if (!appId.isEmpty() && shouldDisableScaling(appId)) {
        double scale = SETTING->getScaleFactor();
        scale = scale > 0 ? 1 / scale : 1;
        QString qtEnv = "QT_SCALE_FACTOR=" + QString::number(scale, 'f', -1);
        cmdPrefixesEnvs << "/usr/bin/env" << "GDK_DPI_SCALE=1" << "GDK_SCALE=1" << qtEnv;
    }

    envs << cmdPrefixesEnvs;

    QStringList exeArgs;
    exeArgs << cmdLine.split(" ") << files;

    if (info->getTerminal()) {
        exeArgs.insert(0, SETTING->getDefaultTerminalExecArg());
        exeArgs.insert(0, SETTING->getDefaultTerminalExec());
    }

    std::string workingDir = info->getKeyFile()->getStr(MainSection, KeyPath);
    if (workingDir.empty()) {
        workingDir = BaseDir::homeDir();
    }

    QString exec = exeArgs[0];
    exeArgs.removeAt(0);

    qDebug() << "launchApp: " << desktopFile << " exec:  " << exec << " args:   " << exeArgs;
    process.setWorkingDirectory(workingDir.c_str());
    process.setEnvironment(envs);
    return process.startDetached(exec, exeArgs);
}

bool StartManager::doRunCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options)
{
    QProcess process;
    if (options.find("dir") != options.end()) {
        process.setWorkingDirectory(options["dir"]);
    }

    return process.startDetached(exe, args);
}

void StartManager::waitCmd(DesktopInfo *info, QProcess *process, QString cmdName)
{

}

bool StartManager::shouldUseProxy(QString appId)
{
    auto useProxyApps = SETTING->getUseProxyApps();
    if (!useProxyApps.contains(appId))
        return false;

    if (dbusHandler->getProxyMsg().isEmpty())
        return false;

    return true;
}

bool StartManager::shouldDisableScaling(QString appId)
{
    auto disableScalingApps = SETTING->getDisableScalingApps();
    return disableScalingApps.contains(appId);
}

void StartManager::loadSysMemLimitConfig()
{
    std::string configPath = BaseDir::userConfigDir() + "deepin/startdde/memchecker.json";
    QFile file(configPath.c_str());
    if (!file.exists())
        file.setFileName(sysMemLimitConfig);

    do {
        if (!file.exists())
            break;

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            break;

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (!doc.isObject())
            break;

        minMemAvail = uint64_t(doc.object()["min-mem-available"].toInt());
        maxSwapUsed = uint64_t(doc.object()["max-swap-used"].toInt());
        return;
    } while (0);

    minMemAvail = defaultMinMemAvail;
    maxSwapUsed = defaultMaxSwapUsed;
}

void StartManager::listenAutostartFileEvents()
{
    for (auto autostartDir : BaseDir::autoStartDirs()) {
        fileWatcher->addPath(autostartDir.c_str());
    }
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &StartManager::onAutoStartupPathChange, Qt::QueuedConnection);
}

void StartManager::startAutostartProgram()
{
    auto func = [&] (QString file, uint64_t delayTime) {
        QThread::sleep(uint64_t(delayTime));
        this->launchApp(file, 0, QStringList());
    };

    for (QString desktopFile : autostartList()) {
        DesktopInfo info(desktopFile.toStdString());
        if (!info.isValidDesktop())
            continue;

        int delayTime = info.getKeyFile()->getInt(MainSection, KeyXGnomeAutostartDelay.toStdString());
        QTimer::singleShot(0, this, [&, desktopFile, delayTime] {
            QThread::sleep(uint64_t(delayTime));
            this->launchApp(desktopFile, 0, QStringList());
        });
    }
}

QStringList StartManager::getAutostartList()
{
    QStringList ret;
    for (auto autostartDir : BaseDir::autoStartDirs()) {
        if (!DFile::isExisted(autostartDir))
            continue;

        QDir dir(autostartDir.c_str());
        dir.setFilter(QDir::Files);
        dir.setNameFilters({ "*.desktop" });
        for (auto entry : dir.entryInfoList()) {
            if (ret.contains(entry.absoluteFilePath()))
                continue;

            ret.push_back(entry.absoluteFilePath());
        }
    }

    return ret;
}
