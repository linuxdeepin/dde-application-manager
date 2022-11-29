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
#include "../../service/impl/application_manager.h"

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
    , m_autostartFileWatcher(new QFileSystemWatcher(this))
    , m_autostartFiles(getAutostartList())
    , m_isDBusCalled(false)
{
    loadSysMemLimitConfig();
    getDesktopToAutostartMap();
    listenAutostartFileEvents();
    startAutostartProgram();
}

bool StartManager::addAutostart(const QString &desktop)
{
    setIsDBusCalled(true);
    return setAutostart(desktop, true);
}

bool StartManager::removeAutostart(const QString &desktop)
{
    setIsDBusCalled(true);
    return setAutostart(desktop, false);
}

QStringList StartManager::autostartList()
{
    if (m_autostartFiles.isEmpty()) {
        m_autostartFiles = getAutostartList();
    }

    return m_autostartFiles;
}

/**desktop为全路径或者相对路径都应该返回true
 * 其他情况返回false
 * @brief StartManager::isAutostart
 * @param desktop
 * @return
 */
bool StartManager::isAutostart(const QString &desktop)
{
    if (!desktop.endsWith(DESKTOPEXT))
        return false;

    QFileInfo file(desktop);
    for (auto autostartDir : BaseDir::autoStartDirs()) {
        std::string filePath = autostartDir + file.baseName().toStdString();
        QDir dir(autostartDir.c_str());
        if (dir.exists(file.fileName())) {
            DesktopInfo info(desktop.toStdString());
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

void StartManager::launchApp(const QString &desktopFile)
{
    doLaunchAppWithOptions(desktopFile);
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

void StartManager::onAutoStartupPathChange(const QString &path)
{
    const QStringList &autostartFilesList = getAutostartList();
    const QSet<QString> newAutostartFiles = QSet<QString>(autostartFilesList.begin(), autostartFilesList.end());
    const QSet<QString> oldAutostartFiles = QSet<QString>(m_autostartFiles.begin(), m_autostartFiles.end());

    const QSet<QString> newFiles = newAutostartFiles - oldAutostartFiles;
    const QSet<QString> deletedFiles = oldAutostartFiles - newAutostartFiles;

    QString desktopFullPath;
    QDir autostartDir(BaseDir::userAutoStartDir().c_str());
    if (deletedFiles.size() && !isDBusCalled()) {
        for (const QString &path : deletedFiles) {
            QFileInfo info(path);
            const QString &autostartDesktopPath = autostartDir.path() + QString("/") + info.fileName();

            for (const std::string &appDir : BaseDir::appDirs()) {
                QDir dir(appDir.c_str());
                dir.setFilter(QDir::Files);
                dir.setNameFilters({ "*.desktop" });
                for (const auto &entry : dir.entryInfoList()) {
                    const QString &desktopPath = entry.absoluteFilePath();
                    if (desktopPath.contains(info.baseName())) {
                        desktopFullPath = desktopPath;
                        break;
                    }
                }

                if (!desktopFullPath.isEmpty())
                    break;
            }

            m_autostartFiles.removeAll(autostartDesktopPath);
            autostartDir.remove(info.fileName());

            if (m_desktopDirToAutostartDirMap.keys().contains(desktopFullPath)) {
                m_desktopDirToAutostartDirMap.remove(desktopFullPath);
                Q_EMIT autostartChanged(autostartDeleted, desktopFullPath);
            }
        }
    } else if (newFiles.size() && !isDBusCalled()) {
        for (const QString &path : newFiles) {
            QFileInfo info(path);
            const QString &autostartDesktopPath = autostartDir.path() + QString("/") + info.fileName();
            m_autostartFiles.push_back(autostartDesktopPath);
            const bool ret = QFile::copy(info.filePath(), autostartDesktopPath);
            if (!ret)
                qWarning() << "add to autostart list failed...";

            /* 设置为自启动时，手动将Hidden字段写入到自启动目录的desktop文件中，并设置为false，只有这样，
             * 安全中心才不会弹出自启动确认窗口, 这种操作是沿用V20阶段的约定规范，这块已经与安全中心研发对接过 */
            KeyFile kf;
            kf.loadFile(autostartDesktopPath.toStdString());
            kf.setKey(MainSection, KeyXDeepinCreatedBy.toStdString(), AMServiceName.toStdString());
            kf.setKey(MainSection, KeyXDeepinAppID.toStdString(), info.baseName().toStdString());
            kf.setBool(MainSection, KeyHidden, "false");
            kf.saveToFile(autostartDesktopPath.toStdString());

            for (const std::string &appDir : BaseDir::appDirs()) {
                QDir dir(appDir.c_str());
                dir.setFilter(QDir::Files);
                dir.setNameFilters({ "*.desktop" });
                for (const auto &entry : dir.entryInfoList()) {
                    const QString &desktopPath = entry.absoluteFilePath();
                    if (desktopPath.contains(info.baseName())) {
                        desktopFullPath = desktopPath;
                        break;
                    }
                }

                if (!desktopFullPath.isEmpty())
                    break;
            }

            if (!m_desktopDirToAutostartDirMap.keys().contains(desktopFullPath)) {
                m_desktopDirToAutostartDirMap[desktopFullPath] = autostartDesktopPath;
                Q_EMIT autostartChanged(autostartAdded, desktopFullPath);
            }
        }
    }

    // 如果是用户通过启动器或者使用dbus接口调用方式添加或者删除自启动，则文件监控的不发送信号
    // 如果是用户直接删除自启动目录下的文件就发送信号
    m_autostartFiles = autostartFilesList;
}

bool StartManager::setAutostart(const QString &desktop, const bool value)
{
    if (!desktop.endsWith(".desktop")) {
        qWarning() << "invalid desktop item...";
        return false;
    }

    QString desktopFullPath = desktop;
    QFileInfo info(desktopFullPath);
    if (!info.isAbsolute()) {
        for (const std::string &appDir : BaseDir::appDirs()) {
            QDir dir(appDir.c_str());
            dir.setFilter(QDir::Files);
            dir.setNameFilters({ "*.desktop" });
            for (const auto &entry : dir.entryInfoList()) {
                const QString &desktopPath = entry.absoluteFilePath();
                if (!desktopPath.contains(desktop))
                    continue;

                desktopFullPath = desktopPath;
                info.setFile(desktopFullPath);
                break;
            }
        }
    }

    // 本地没有找到该应用就直接返回
    if (!info.isAbsolute()) {
        qWarning() << "invalid item...";
        return false;
    }

    QDir autostartDir(BaseDir::userAutoStartDir().c_str());
    const QString &appId = info.baseName();

   if (value && isAutostart(desktopFullPath)) {
       qWarning() << "invalid desktop or item is already in the autostart list.";
       return false;
   }

   if (!value && !isAutostart(desktopFullPath)) {
       qWarning() << "can't find autostart item";
       return false;
   }

   const QString &autostartDesktopPath = autostartDir.path() + QString("/") + info.fileName();
   if (value && !m_autostartFiles.contains(autostartDesktopPath)) {
       m_autostartFiles.push_back(autostartDesktopPath);

       // 建立映射关系
       if (!m_desktopDirToAutostartDirMap.keys().contains(desktopFullPath))
           m_desktopDirToAutostartDirMap[desktopFullPath] = autostartDesktopPath;

       const bool ret = QFile::copy(info.filePath(), autostartDesktopPath);
       if (!ret)
           qWarning() << "add to autostart list failed...";

       /* 设置为自启动时，手动将Hidden字段写入到自启动目录的desktop文件中，并设置为false，只有这样，
        * 安全中心才不会弹出自启动确认窗口, 这种操作是沿用V20阶段的约定规范，这块已经与安全中心研发对接过 */
       KeyFile kf;
       kf.loadFile(autostartDesktopPath.toStdString());
       kf.setKey(MainSection, KeyXDeepinCreatedBy.toStdString(), AMServiceName.toStdString());
       kf.setKey(MainSection, KeyXDeepinAppID.toStdString(), appId.toStdString());
       kf.setBool(MainSection, KeyHidden, "false");
       kf.saveToFile(autostartDesktopPath.toStdString());
   } else if (!value && m_autostartFiles.contains(autostartDesktopPath)) {
       // 删除映射关系
       if (m_desktopDirToAutostartDirMap.keys().contains(desktopFullPath))
           m_desktopDirToAutostartDirMap.remove(desktopFullPath);

       m_autostartFiles.removeAll(autostartDesktopPath);
       autostartDir.remove(info.fileName());
   } else {
       qWarning() << "error happen...";
       return false;
   }

   Q_EMIT autostartChanged(value ? autostartAdded : autostartDeleted, desktopFullPath);
   setIsDBusCalled(false);
   return true;
}

bool StartManager::doLaunchAppWithOptions(const QString &desktopFile)
{
    DesktopInfo info(desktopFile.toStdString());
    if (!info.isValidDesktop())
        return false;

    launch(&info, info.getCommandLine().c_str(), 0, QStringList());

    dbusHandler->markLaunched(desktopFile);

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

#ifdef QT_DEBUG
    qDebug() << "launchApp: " << desktopFile << " exec:  " << exec << " args:   " << exeArgs;
#endif

    process.setWorkingDirectory(workingDir.c_str());
    process.setEnvironment(envs);

    if (desktopFile.contains("/persistent/linglong")) {
        exeArgs.clear();

#ifdef QT_DEBUG
        qDebug() << "exeArgs:"  << cmdLine.section(" ", 1, 2);
#endif

        exeArgs.append(cmdLine.section(" ", 1, 2).split(" "));
    }

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
    m_autostartFileWatcher->addPath(BaseDir::userAutoStartDir().c_str());
    connect(m_autostartFileWatcher, &QFileSystemWatcher::directoryChanged, this, &StartManager::onAutoStartupPathChange, Qt::QueuedConnection);
}

void StartManager::startAutostartProgram()
{
    for (const QString &desktopFile : autostartList()) {
        DesktopInfo info(desktopFile.toStdString());
        if (!info.isValidDesktop())
            continue;

        launchApp(desktopFile);
    }
}

QStringList StartManager::getAutostartList()
{
    QStringList autostartList;
    for (const std::string &autostartDir : BaseDir::autoStartDirs()) {
        QDir dir(autostartDir.c_str());
        if (!dir.exists())
            continue;

        dir.setFilter(QDir::Files);
        dir.setNameFilters({ "*.desktop" });
        for (const auto &entry : dir.entryInfoList()) {
            if (autostartList.contains(entry.absoluteFilePath()))
                continue;

            autostartList.push_back(entry.absoluteFilePath());
        }
    }

    return autostartList;
}

QMap<QString, QString> StartManager::getDesktopToAutostartMap()
{
    // 获取已加入到自启动列表应用的desktop全路径
    QDir autostartDir(BaseDir::userAutoStartDir().c_str());
    autostartDir.setFilter(QDir::Files);
    autostartDir.setNameFilters({ "*.desktop" });
    for (const auto &entry : autostartDir.entryInfoList()) {
        const QFileInfo &fileInfo(entry.absoluteFilePath());
        for (const std::string &appDir : BaseDir::appDirs()) {
            QDir dir(appDir.c_str());
            dir.setFilter(QDir::Files);
            dir.setNameFilters({ "*.desktop" });
            for (const auto &entry : dir.entryInfoList()) {
                const QString &desktopPath = entry.absoluteFilePath();
                if (desktopPath.contains(fileInfo.baseName()) &&
                        m_desktopDirToAutostartDirMap.find(desktopPath) == m_desktopDirToAutostartDirMap.end()) {
                    m_desktopDirToAutostartDirMap.insert(desktopPath, entry.absoluteFilePath());
                }
            }
        }
    }

    return m_desktopDirToAutostartDirMap;
}

void StartManager::setIsDBusCalled(const bool state)
{
    m_isDBusCalled = state;
}

bool StartManager::isDBusCalled() const
{
    return m_isDBusCalled;
}
