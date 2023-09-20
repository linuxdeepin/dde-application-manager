// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/mimemanager1adaptor.h"
#include "applicationmanager1service.h"
#include "applicationservice.h"
#include "constant.h"

MimeManager1Service::MimeManager1Service(ApplicationManager1Service *parent)
    : QObject(parent)
{
    new MimeManager1Adaptor{this};
    if (!registerObjectToDBus(this, DDEApplicationManager1MimeManager1ObjectPath, MimeManager1Interface)) {
        std::terminate();
    }
}

MimeManager1Service::~MimeManager1Service() = default;

ObjectMap MimeManager1Service::listApplications(const QString &mimeType) const noexcept
{
    auto mime = m_database.mimeTypeForName(mimeType);
    if (!mime.isValid()) {
        qWarning() << "can't find" << mimeType;
        return {};
    }

    QStringList appIds;

    for (auto it = m_infos.rbegin(); it != m_infos.rend(); ++it) {
        const auto &info = it->cacheInfo();
        if (!info) {
            continue;
        }
        auto apps = info->queryApps(mime);
        appIds.append(std::move(apps));
    }
    appIds.removeDuplicates();
    qInfo() << "query" << mimeType << "find:" << appIds;
    const auto &apps = static_cast<ApplicationManager1Service *>(parent())->findApplicationsByIds(appIds);
    return dumpDBusObject(apps);
}

QString MimeManager1Service::queryFileTypeAndDefaultApplication(const QString &filePath,
                                                                QDBusObjectPath &application) const noexcept
{
    QString mimeType;
    application = QDBusObjectPath{"/"};

    auto mime = m_database.mimeTypeForFile(filePath);
    if (mime.isValid()) {
        mimeType = mime.name();
    }

    QString defaultAppId;
    for (auto it1 = m_infos.rbegin(); it1 != m_infos.rend(); ++it1) {
        const auto &list = it1->appsList();
        for (auto it2 = list.rbegin(); it2 != list.rend(); ++it2) {
            if (auto app = it2->queryDefaultApp(mime); !app.isEmpty()) {
                defaultAppId = app;
            }
        }
    }

    if (defaultAppId.isEmpty()) {
        qInfo() << "file's mimeType:" << mime.name() << "but can't find a default application for this type.";
        return mimeType;
    }

    const auto &apps = static_cast<ApplicationManager1Service *>(parent())->findApplicationsByIds({defaultAppId});
    if (apps.isEmpty()) {
        qWarning() << "default application has been found:" << defaultAppId
                   << " but we can't find corresponding application in ApplicationManagerService.";
    } else {
        application = apps.firstKey();
    }

    return mimeType;
}

void MimeManager1Service::setDefaultApplication(const KVPairs &defaultApps) noexcept
{
    auto &app = m_infos.front().appsList();
    auto userConfig = std::find_if(
        app.begin(), app.end(), [](const MimeApps &config) { return !config.isDesktopSpecific(); });  // always find this

    for (auto it = defaultApps.constKeyValueBegin(); it != defaultApps.constKeyValueEnd(); ++it) {
        userConfig->setDefaultApplication(it->first, it->second);
    }

    if (!userConfig->writeToFile()) {
        sendErrorReply(QDBusError::Failed, "set default app failed, these config will be reset after re-login.");
    }
}

void MimeManager1Service::unsetDefaultApplication(const QStringList &mimeTypes) noexcept
{
    auto &app = m_infos.front().appsList();
    auto userConfig = std::find_if(app.begin(), app.end(), [](const MimeApps &config) { return !config.isDesktopSpecific(); });

    for (const auto &mime : mimeTypes) {
        userConfig->unsetDefaultApplication(mime);
    }

    if (!userConfig->writeToFile()) {
        sendErrorReply(QDBusError::Failed, "unset default app failed, these config will be reset after re-login.");
    }
}

void MimeManager1Service::appendMimeInfo(MimeInfo &&info)
{
    m_infos.emplace_back(std::move(info));
}
