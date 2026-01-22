// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
#include <QProcess>

#include "dbus/mimemanager1adaptor.h"
#include "applicationmanager1service.h"
#include "applicationservice.h"  // IWYU pragma: keep
#include "constant.h"

MimeManager1Service::MimeManager1Service(ApplicationManager1Service *parent)
    : QObject(parent)
{
    auto *adaptor = new (std::nothrow) MimeManager1Adaptor{this};
    if (adaptor == nullptr or !registerObjectToDBus(this, DDEApplicationManager1MimeManager1ObjectPath, MimeManager1Interface)) {
        std::terminate();
    }

    // 监控用户配置目录下的 mimeapps.list 文件
    connect(&m_mimeAppsWatcher, &QFileSystemWatcher::fileChanged, this, &MimeManager1Service::onMimeAppsFileChanged);

    // 添加用户配置目录下的 mimeapps.list 文件到监控
    QString userMimeAppsFile = getXDGConfigHome() + "/mimeapps.list";
    if (QFileInfo::exists(userMimeAppsFile)) {
        m_mimeAppsWatcher.addPath(userMimeAppsFile);
    } else {
        qWarning() << "User mimeapps.list file does not exist:" << userMimeAppsFile;
    }

    m_mimeAppsDebounceTimer.setSingleShot(true);
    m_mimeAppsDebounceTimer.setInterval(50);
    connect(&m_mimeAppsDebounceTimer, &QTimer::timeout, this, &MimeManager1Service::handleMimeAppsFileDebounced);
}

MimeManager1Service::~MimeManager1Service() = default;

ObjectMap MimeManager1Service::listApplications(const QString &mimeType) const noexcept
{
    auto type = m_database.mimeTypeForName(mimeType).name();
    if (type.isEmpty()) {
        qInfo() << "try to query raw type:" << mimeType;
        type = mimeType;
    }

    QStringList appIds;

    for (auto it = m_infos.rbegin(); it != m_infos.rend(); ++it) {
        const auto &info = it->cacheInfo();
        if (!info) {
            continue;
        }
        auto apps = info->queryApps(type);
        appIds.append(std::move(apps));
    }
    appIds.removeDuplicates();
    qInfo() << "query" << mimeType << "find:" << appIds;
    const auto &apps = dynamic_cast<ApplicationManager1Service *>(parent())->findApplicationsByIds(appIds);
    return dumpDBusObject(apps);
}

QString MimeManager1Service::queryDefaultApplication(const QString &content, QDBusObjectPath &application) const noexcept
{
    QMimeType mime;
    QFileInfo info{content};
    application = QDBusObjectPath{"/"};

    if (info.isAbsolute() and info.exists()) {
        mime = m_database.mimeTypeForFile(content);
    } else {
        mime = m_database.mimeTypeForName(content);
    }

    auto type = mime.name();

    if (type.isEmpty()) {
        qInfo() << "try to query raw content:" << content;
        type = content;
    }

    QString defaultAppId;
    for (auto it1 = m_infos.rbegin(); it1 != m_infos.rend(); ++it1) {
        const auto &list = it1->appsList();
        for (auto it2 = list.rbegin(); it2 != list.rend(); ++it2) {
            if (auto app = it2->queryDefaultApp(type); !app.isEmpty()) {
                defaultAppId = app;
            }
        }
    }

    if (defaultAppId.isEmpty()) {
        qInfo() << "file's mimeType:" << mime.name() << "but can't find a default application for this type.";
        return type;
    }

    const auto &apps = dynamic_cast<ApplicationManager1Service *>(parent())->findApplicationsByIds({defaultAppId});
    if (apps.isEmpty()) {
        qWarning() << "default application has been found:" << defaultAppId
                   << " but we can't find corresponding application in ApplicationManagerService.";
    } else {
        application = apps.keys().first();
    }

    return type;
}

void MimeManager1Service::setDefaultApplication(const QStringMap &defaultApps) noexcept
{
    auto &app = m_infos.front().appsList();

    if (app.empty()) {
        safe_sendErrorReply(QDBusError::InternalError);
        return;
    }

    auto userConfig = std::find_if(
        app.begin(), app.end(), [](const MimeApps &config) { return !config.isDesktopSpecific(); });  // always find this

    if (userConfig == app.end()) {
        qWarning() << "couldn't find user mimeApps";
        safe_sendErrorReply(QDBusError::InternalError);
        return;
    }

    m_internalWriteInProgress = true;
    for (auto it = defaultApps.constKeyValueBegin(); it != defaultApps.constKeyValueEnd(); ++it) {
        userConfig->setDefaultApplication(it->first, it->second);
    }

    if (!userConfig->writeToFile()) {
        safe_sendErrorReply(QDBusError::Failed, "set default app failed, these config will be reset after re-login.");
        m_internalWriteInProgress = false;
        return;
    }
}

void MimeManager1Service::unsetDefaultApplication(const QStringList &mimeTypes) noexcept
{
    auto &app = m_infos.front().appsList();
    auto userConfig = std::find_if(app.begin(), app.end(), [](const MimeApps &config) { return !config.isDesktopSpecific(); });

    if (userConfig == app.end()) {
        qWarning() << "couldn't find user mimeApps";
        safe_sendErrorReply(QDBusError::InternalError);
        return;
    }

    m_internalWriteInProgress = true;
    for (const auto &mime : mimeTypes) {
        userConfig->unsetDefaultApplication(mime);
    }

    if (!userConfig->writeToFile()) {
        safe_sendErrorReply(QDBusError::Failed, "unset default app failed, these config will be reset after re-login.");
        m_internalWriteInProgress = false;
        return;
    }
}

void MimeManager1Service::appendMimeInfo(MimeInfo &&info)
{
    m_infos.emplace_back(std::move(info));
}

void MimeManager1Service::reset() noexcept
{
    m_infos.clear();
}

void MimeManager1Service::updateMimeCache(QString dir) noexcept
{
    QProcess process;
    process.start("update-desktop-database", {dir});
    process.waitForFinished();
    auto exitCode = process.exitCode();
    if (exitCode != 0) {
        qWarning() << "Launch Application Failed";
    }

}

void MimeManager1Service::onMimeAppsFileChanged(const QString &path)
{
    if (!m_mimeAppsWatcher.files().contains(path) && QFileInfo::exists(path)) {
        m_mimeAppsWatcher.addPath(path);
    }

    // 如果是内部写入导致的文件变化，忽略
    if (m_internalWriteInProgress) {
        m_internalWriteInProgress = false;
        return;
    }

    m_mimeAppsDebounceTimer.start();
}

void MimeManager1Service::handleMimeAppsFileDebounced()
{
    auto *parentService = qobject_cast<ApplicationManager1Service *>(parent());
    if (!parentService) {
        return;
    }

    qInfo() << "Reloading MIME info due to external configuration change.";
    parentService->reloadMimeInfos();
}
