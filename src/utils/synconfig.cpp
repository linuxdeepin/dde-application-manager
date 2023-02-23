// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "synconfig.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QtDebug>
#include <QDBusInterface>
#include <QDBusConnectionInterface>

const QString syncDBusService = "org.deepin.dde.Sync1.Config";
const QString syncDBusObject = "/org/deepin/dde/Sync1/Config";
const QString syncDBusDaemonService = "com.deepin.sync.Daemon";
const QString syncDBusDaemonObject = "/com/deepin/sync/Daemon";
const QString syncDBusDaemonInterface = syncDBusDaemonService;

QMap<QString, SynModuleBase*> SynConfig::synModulesMap;

SynConfig *SynConfig::instance(QObject *parent)
{
    static SynConfig synConfig(parent);

    return &synConfig;
}

bool SynConfig::registe(QString moduleName, SynModuleBase *module)
{
    bool registed = false;
    if (moduleName.size() > 0 && module) {
        synModulesMap[moduleName] = module;
        registed = true;
    }

    return registed;
}

QByteArray SynConfig::GetSyncConfig(QString moduleName)
{
    if (synModulesMap.find(moduleName) == synModulesMap.end())
        return {};

    SynModuleBase *module = synModulesMap[moduleName];
    if (module)
        return module->getSyncConfig();
    else
        return {};
}

void SynConfig::SetSyncConfig(QString moduleName, QByteArray ba)
{
    if (synModulesMap.find(moduleName) == synModulesMap.end())
        return;

    SynModuleBase *module = synModulesMap[moduleName];
    if (module) {
        module->setSyncConfig(ba);
    }
}

SynConfig::SynConfig(QObject *parent)
 : QObject(parent)
{
    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(syncDBusService))
    {
        qCritical() << "register service org.deepin.dde.Sync1.Config error:" << con.lastError().message();
        return;
    }

    if (!con.registerObject(syncDBusObject, this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals))
    {
        qCritical() << "register object /org/deepin/dde/Sync1/Config error:" << con.lastError().message();
        return;
    }

    // 只注册一次
    registerOnSyncDaemon();
    // 关联com.deepin.sync.Daemon所有者变化
    QDBusConnectionInterface *ifc = QDBusConnection::sessionBus().interface();
    connect(ifc, &QDBusConnectionInterface::serviceOwnerChanged, this, [ & ](const QString & name, const QString & oldOwner, const QString & newOwner) {
        Q_UNUSED(oldOwner)
        if (name == syncDBusDaemonService && !newOwner.isEmpty()) {
            this->registerOnSyncDaemon();
        }
    });
}

SynConfig::~SynConfig()
{

}

void SynConfig::registerOnSyncDaemon()
{
    QDBusInterface interface = QDBusInterface(syncDBusDaemonService, syncDBusDaemonObject, syncDBusDaemonInterface);
    // 修改V20注册方式，需同步到deepin-deepinid-daemon
    interface.call("Register", syncDBusService, syncDBusObject);
}
