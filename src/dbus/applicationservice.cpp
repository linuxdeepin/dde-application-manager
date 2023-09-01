// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationservice.h"
#include "APPobjectmanager1adaptor.h"
#include "applicationmanager1service.h"
#include "dbus/instanceadaptor.h"
#include "launchoptions.h"
#include <QUuid>
#include <QStringList>
#include <QList>
#include <QUrl>
#include <QRegularExpression>
#include <QProcess>
#include <QStandardPaths>
#include <algorithm>
#include <new>

ApplicationService::ApplicationService(DesktopFile source, ApplicationManager1Service *parent)
    : QObject(parent)
    , m_desktopSource(std::move(source))
{
}

ApplicationService::~ApplicationService()
{
    for (auto &instance : m_Instances.values()) {
        instance->m_Application = QDBusObjectPath{"/"};
        auto *ptr = instance.get();
        instance.reset(nullptr);
        ptr->setParent(qApp);  // detach all instances to qApp
    }
}

QSharedPointer<ApplicationService> ApplicationService::createApplicationService(DesktopFile source,
                                                                                ApplicationManager1Service *parent) noexcept
{
    QSharedPointer<ApplicationService> app{new (std::nothrow) ApplicationService{std::move(source), parent}};
    if (!app) {
        qCritical() << "new application service failed.";
        return nullptr;
    }

    QString objectPath;
    QTextStream sourceStream;

    auto appId = app->desktopFileSource().desktopId();

    if (!appId.isEmpty()) {
        objectPath = QString{DDEApplicationManager1ObjectPath} + "/" + escapeToObjectPath(appId);
    } else {
        objectPath = QString{DDEApplicationManager1ObjectPath} + "/" + QUuid::createUuid().toString(QUuid::Id128);
    }

    DesktopFileGuard guard{app->desktopFileSource()};

    if (!guard.try_open()) {
        return nullptr;
    }

    sourceStream.setDevice(app->desktopFileSource().sourceFile());
    std::unique_ptr<DesktopEntry> entry{std::make_unique<DesktopEntry>()};
    auto error = entry->parse(sourceStream);

    if (error != DesktopErrorCode::NoError) {
        qWarning() << "parse failed:" << error << app->desktopFileSource().sourcePath();
        return nullptr;
    }

    if (auto val = entry->value(DesktopFileEntryKey, "Hidden"); val.has_value()) {
        bool ok{false};
        if (auto hidden = val.value().toBoolean(ok); ok and hidden) {
            qWarning() << "invalid hidden value:" << *val.value().find(defaultKeyStr);
            return nullptr;
        }
    }

    app->m_entry.reset(entry.release());
    app->m_applicationPath = QDBusObjectPath{std::move(objectPath)};

    // TODO: icon lookup
    if (auto *ptr = new (std::nothrow) APPObjectManagerAdaptor{app.data()}; ptr == nullptr) {
        qCritical() << "new Object Manager of Application failed.";
        return nullptr;
    }

    return app;
}

QDBusObjectPath ApplicationService::Launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    QString execStr;
    bool ok;
    const auto &supportedActions = actions();

    while (!action.isEmpty() and !supportedActions.isEmpty()) {  // break trick
        if (auto index = supportedActions.indexOf(action); index == -1) {
            qWarning() << "can't find " << action << " in supported actions List. application will use default action to launch.";
            break;
        }

        const auto &actionHeader = QString{"%1%2"}.arg(DesktopFileActionKey, action);
        const auto &actionExec = m_entry->value(actionHeader, "Exec");
        if (!actionExec) {
            break;
        }
        execStr = actionExec->toString(ok);
        if (!ok) {
            qWarning() << "exec value to string failed, try default action.";
            break;
        }
        break;
    }

    if (execStr.isEmpty()) {
        auto Actions = m_entry->value(DesktopFileEntryKey, "Exec");
        if (!Actions) {
            qWarning() << "application has isn't executable.";
            return {};
        }
        execStr = Actions->toString(ok);
        if (!ok) {
            qWarning() << "default action value to string failed. abort...";
            return {};
        }
    }

    auto cmds = generateCommand(options);

    auto [bin, execCmds, res] = unescapeExec(execStr, fields);

    cmds.append(std::move(execCmds));

    qDebug() << "run application with:" << cmds;

    auto &jobManager = static_cast<ApplicationManager1Service *>(parent())->jobManager();

    return jobManager.addJob(
        m_applicationPath.path(),
        [this, binary = std::move(bin), commands = std::move(cmds)](QVariant variantValue) mutable -> QVariant {
            auto resourceFile = variantValue.toString();
            auto instanceRandomUUID = QUuid::createUuid().toString(QUuid::Id128);
            auto objectPath = m_applicationPath.path() + "/" + instanceRandomUUID;
            commands.push_front(QString{"--SourcePath=%1"}.arg(m_desktopSource.sourcePath()));

            if (resourceFile.isEmpty()) {
                commands.push_front(QString{R"(--unitName=app-DDE-%1@%2.service)"}.arg(
                    escapeApplicationId(this->id()), instanceRandomUUID));  // launcher should use this instanceId
                QProcess process;
                process.start(m_launcher, commands);
                process.waitForFinished();
                if (auto code = process.exitCode(); code != 0) {
                    qWarning() << "Launch Application Failed. exitCode:" << code;
                    return QString{""};
                }
                return objectPath;
            }

            qsizetype location{0};
            location = commands.indexOf(R"(%f)");
            if (location != -1) {  // due to std::move, there only remove once
                commands.remove(location);
            }

            auto url = QUrl::fromUserInput(resourceFile);
            if (!url.isValid()) {  // if url is invalid, passing to launcher directly
                auto scheme = url.scheme();
                if (!scheme.isEmpty()) {
                    // TODO: resourceFile = processRemoteFile(resourceFile);
                }
            }

            // NOTE: resourceFile must be available in the following contexts
            commands.insert(location, resourceFile);
            commands.push_front(QString{R"(--unitName=DDE-%1@%2.service)"}.arg(this->id(), instanceRandomUUID));
            QProcess process;
            process.start(getApplicationLauncherBinary(), commands);
            process.waitForFinished();
            auto exitCode = process.exitCode();
            if (exitCode != 0) {
                qWarning() << "Launch Application Failed:" << binary << commands;
                return QString{""};
            }
            return objectPath;
        },
        std::move(res));
}

bool ApplicationService::SendToDesktop() const noexcept
{
    if (isOnDesktop()) {
        return true;
    }

    auto dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty()) {
        qDebug() << "no desktop directory found.";
        return false;
    }

    auto desktopFile = QDir{dir}.filePath(m_desktopSource.desktopId() + ".desktop");
    auto success = m_desktopSource.sourceFileRef().link(desktopFile);
    if (!success) {
        qDebug() << "create link failed:" << m_desktopSource.sourceFileRef().errorString() << "path:" << desktopFile;
    }

    return success;
}

bool ApplicationService::RemoveFromDesktop() const noexcept
{
    if (!isOnDesktop()) {
        return true;
    }

    auto dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty()) {
        qDebug() << "no desktop directory found.";
        return false;
    }

    QFile desktopFile{QDir{dir}.filePath(m_desktopSource.desktopId() + ".desktop")};
    auto success = desktopFile.remove();

    if (!success) {
        qDebug() << "remove desktop file failed:" << desktopFile.errorString();
    }

    return success;
}

bool ApplicationService::isOnDesktop() const noexcept
{
    auto dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

    if (dir.isEmpty()) {
        qDebug() << "no desktop directory found.";
        return false;
    }

    QFileInfo info{QDir{dir}.filePath(m_desktopSource.desktopId() + ".desktop")};

    if (!info.exists()) {
        return false;
    }

    if (!info.isSymbolicLink()) {
        return false;
    }

    return info.symLinkTarget() == m_desktopSource.sourcePath();
}

QStringList ApplicationService::actions() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "Actions", EntryValueType::String);

    if (val.isNull()) {
        return {};
    }

    auto actionList = val.toString().split(";", Qt::SkipEmptyParts);
    return actionList;
}

QStringList ApplicationService::categories() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "Categories", EntryValueType::String);

    if (val.isNull()) {
        return {};
    }

    return val.toString().split(';', Qt::SkipEmptyParts);
}

PropMap ApplicationService::actionName() const noexcept
{
    PropMap ret;
    auto actionList = actions();

    for (auto &action : actionList) {
        action.prepend(DesktopFileActionKey);
        auto value = m_entry->value(action, "Name");
        if (!value.has_value()) {
            continue;
        }
        ret.insert(action, {std::move(value).value()});
    }

    return ret;
}

PropMap ApplicationService::displayName() const noexcept
{
    PropMap ret;
    auto value = std::move(m_entry->value(DesktopFileEntryKey, "Name")).value();
    ret.insert(QString{"Name"}, {std::move(value)});
    return ret;
}

PropMap ApplicationService::icons() const noexcept
{
    PropMap ret;
    auto actionList = actions();
    for (const auto &action : actionList) {
        auto value = m_entry->value(QString{action}.prepend(DesktopFileActionKey), "Icon");
        if (!value.has_value()) {
            continue;
        }
        ret.insert(action, {std::move(value).value()});
    }

    auto mainIcon = m_entry->value(DesktopFileEntryKey, "Icon");
    if (mainIcon.has_value()) {
        ret.insert(defaultKeyStr, {std::move(mainIcon).value()});
    }

    return ret;
}

ObjectMap ApplicationService::GetManagedObjects() const
{
    return dumpDBusObject(m_Instances);
}

QString ApplicationService::id() const noexcept
{
    return m_desktopSource.desktopId();
}

bool ApplicationService::x_Flatpak() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "X-flatpak", EntryValueType::String);
    return val.isNull();
}

bool ApplicationService::x_linglong() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "X-linglong", EntryValueType::String);
    return val.isNull();
}

qulonglong ApplicationService::installedTime() const noexcept
{
    return m_desktopSource.createTime();
}

qulonglong ApplicationService::lastLaunchedTime() const noexcept
{
    return m_lastLaunch;
}

bool ApplicationService::isAutoStart() const noexcept
{
    return m_AutoStart;
}

void ApplicationService::setAutoStart(bool autostart) noexcept
{
    m_AutoStart = autostart;
}

QList<QDBusObjectPath> ApplicationService::instances() const noexcept
{
    return m_Instances.keys();
}

bool ApplicationService::addOneInstance(const QString &instanceId, const QString &application, const QString &systemdUnitPath)
{
    auto *service = new InstanceService{instanceId, application, systemdUnitPath};
    auto *adaptor = new InstanceAdaptor(service);
    QString objectPath{m_applicationPath.path() + "/" + instanceId};

    if (registerObjectToDBus(service, objectPath, InstanceInterface)) {
        m_Instances.insert(QDBusObjectPath{objectPath}, QSharedPointer<InstanceService>{service});
        service->moveToThread(this->thread());
        adaptor->moveToThread(this->thread());
        emit InterfacesAdded(QDBusObjectPath{objectPath}, getChildInterfacesAndPropertiesFromObject(service));
        return true;
    }

    adaptor->deleteLater();
    service->deleteLater();
    return false;
}

void ApplicationService::removeOneInstance(const QDBusObjectPath &instance) noexcept
{
    if (auto it = m_Instances.find(instance); it != m_Instances.cend()) {
        emit InterfacesRemoved(instance, getChildInterfacesFromObject(it->data()));
        unregisterObjectFromDBus(instance.path());
        m_Instances.remove(instance);
    }
}

void ApplicationService::removeAllInstance() noexcept
{
    for (const auto &instance : m_Instances.keys()) {
        removeOneInstance(instance);
    }
}

QDBusObjectPath ApplicationService::findInstance(const QString &instanceId) const
{
    for (const auto &[path, ptr] : m_Instances.asKeyValueRange()) {
        if (ptr->instanceId() == instanceId) {
            return path;
        }
    }
    return {};
}

void ApplicationService::resetEntry(DesktopEntry *newEntry) noexcept
{
    m_entry.reset(newEntry);
}

LaunchTask ApplicationService::unescapeExec(const QString &str, const QStringList &fields)
{
    LaunchTask task;
    QStringList execList = str.split(' ');
    task.LaunchBin = execList.first();

    QRegularExpression re{"%[fFuUickdDnNvm]"};
    auto matcher = re.match(str);
    if (!matcher.hasMatch()) {
        task.command.append(std::move(execList));
        task.Resources.emplace_back(QString{""});  // mapReduce should run once at least
        return task;
    }

    auto list = matcher.capturedTexts();
    if (list.count() > 1) {
        qWarning() << "invalid exec format, all filed code will be ignored.";
        for (const auto &code : list) {
            execList.removeOne(code);
        }
        task.command.append(std::move(execList));
        return task;
    }

    auto filesCode = list.first().back().toLatin1();
    auto codeStr = QString(R"(%%1)").arg(filesCode);
    auto location = execList.indexOf(codeStr);

    switch (filesCode) {
    case 'f': {  // Defer to async job
        task.command.append(std::move(execList));
        for (auto &field : fields) {
            task.Resources.emplace_back(std::move(field));
        }
        break;
    }
    case 'u': {
        execList.removeAt(location);
        if (fields.empty()) {
            task.command.append(execList);
            break;
        }
        if (fields.count() > 1) {
            qDebug() << R"(fields count is greater than one, %u will only take first element.)";
        }
        execList.insert(location, fields.first());
        task.command.append(execList);
        break;
    }
    case 'F':
        [[fallthrough]];
    case 'U': {
        execList.removeAt(location);
        auto it = execList.begin() + location;
        for (const auto &field : fields) {
            it = execList.insert(it, field);
        }
        task.command.append(std::move(execList));
        break;
    }
    case 'i': {
        execList.removeAt(location);
        auto val = m_entry->value(DesktopFileEntryKey, "Icon");
        if (!val) {
            qDebug() << R"(Application Icons can't be found. %i will be ignored.)";
            task.command.append(std::move(execList));
            return task;
        }
        bool ok;
        auto iconStr = val->toIconString(ok);
        if (!ok) {
            qDebug() << R"(Icons Convert to string failed. %i will be ignored.)";
            task.command.append(std::move(execList));
            return task;
        }
        auto it = execList.insert(location, iconStr);
        execList.insert(it, "--icon");
        task.command.append(std::move(execList));
        break;
    }
    case 'c': {
        execList.removeAt(location);
        auto val = m_entry->value(DesktopFileEntryKey, "Name");
        if (!val) {
            qDebug() << R"(Application Name can't be found. %c will be ignored.)";
            task.command.append(std::move(execList));
            return task;
        }
        bool ok;
        auto NameStr = val->toLocaleString(getUserLocale(), ok);
        if (!ok) {
            qDebug() << R"(Name Convert to locale string failed. %c will be ignored.)";
            task.command.append(std::move(execList));
            return task;
        }
        execList.insert(location, NameStr);
        task.command.append(std::move(execList));
        break;
    }
    case 'k': {  // ignore all desktop file location for now.
        execList.removeAt(location);
        task.command.append(std::move(execList));
        break;
    }
    case 'd':
    case 'D':
    case 'n':
    case 'N':
    case 'v':
        [[fallthrough]];  // Deprecated field codes should be removed from the command line and ignored.
    case 'm': {
        execList.removeAt(location);
        task.command.append(std::move(execList));
        break;
    }
    }

    if (task.Resources.isEmpty()) {
        task.Resources.emplace_back(QString{""});  // mapReduce should run once at least
    }

    return task;
}

QVariant ApplicationService::findEntryValue(const QString &group,
                                            const QString &valueKey,
                                            EntryValueType type,
                                            const QLocale &locale) const noexcept
{
    QVariant ret;
    auto tmp = m_entry->value(group, valueKey);
    if (!tmp.has_value()) {
        return ret;
    }

    auto val = std::move(tmp).value();
    bool ok{false};

    switch (type) {
    case EntryValueType::String: {
        auto valStr = val.toString(ok);
        if (ok) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    case EntryValueType::LocaleString: {
        auto valStr = val.toLocaleString(locale, ok);
        if (ok) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    case EntryValueType::Boolean: {
        auto valBool = val.toBoolean(ok);
        if (ok) {
            ret = QVariant::fromValue(valBool);
        }
    } break;
    case EntryValueType::IconString: {
        auto valStr = val.toIconString(ok);
        if (ok) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    }

    return ret;
}
