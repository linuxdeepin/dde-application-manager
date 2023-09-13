// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationservice.h"
#include "APPobjectmanager1adaptor.h"
#include "applicationchecker.h"
#include "applicationmanager1service.h"
#include "applicationmanagerstorage.h"
#include "propertiesForwarder.h"
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
#include <utility>
#include <wordexp.h>

ApplicationService::ApplicationService(DesktopFile source,
                                       ApplicationManager1Service *parent,
                                       std::weak_ptr<ApplicationManager1Storage> storage)
    : QObject(parent)
    , m_storage(std::move(storage))
    , m_desktopSource(std::move(source))
{
}

ApplicationService::~ApplicationService()
{
    for (auto &instance : m_Instances.values()) {
        orphanedInstances->append(instance);
        instance->m_orphaned = true;
    }
}

QSharedPointer<ApplicationService> ApplicationService::createApplicationService(
    DesktopFile source, ApplicationManager1Service *parent, std::weak_ptr<ApplicationManager1Storage> storage) noexcept
{
    QSharedPointer<ApplicationService> app{new (std::nothrow) ApplicationService{std::move(source), parent, std::move(storage)}};
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

    if (!shouldBeShown(entry)) {
        qDebug() << "application shouldn't be shown:" << app->desktopFileSource().sourcePath();
        return nullptr;
    }

    app->m_entry.reset(entry.release());
    app->m_applicationPath = QDBusObjectPath{std::move(objectPath)};

    // TODO: icon lookup
    if (auto *ptr = new (std::nothrow) APPObjectManagerAdaptor{app.data()}; ptr == nullptr) {
        qCritical() << "new Object Manager of Application failed.";
        return nullptr;
    }

    if (auto *ptr = new (std::nothrow) PropertiesForwarder{app->m_applicationPath.path(), app.data()}; ptr == nullptr) {
        qCritical() << "new PropertiesForwarder of Application failed.";
        return nullptr;
    }

    auto ptr = app->m_storage.lock();
    if (!ptr) {
        qWarning() << "runtime storage doesn't exists.";
        return app;
    }

    return app;
}

bool ApplicationService::shouldBeShown(const std::unique_ptr<DesktopEntry> &entry) noexcept
{
    if (ApplicationFilter::hiddenCheck(entry)) {
        return false;
    }

    if (ApplicationFilter::tryExecCheck(entry)) {
        return false;
    }

    if (ApplicationFilter::showInCheck(entry)) {
        return false;
    }

    return true;
}

QDBusObjectPath ApplicationService::Launch(const QString &action, const QStringList &fields, const QVariantMap &options)
{
    QString execStr;
    bool ok;
    const auto &supportedActions = actions();
    auto optionsMap = options;
    QString oldEnv;

    auto factor = getDeepinWineScaleFactor(m_desktopSource.desktopId()).toDouble();
    if (factor != 1.0) {
        if (auto it = optionsMap.find("env"); it != optionsMap.cend()) {
            oldEnv = it->value<QString>();
        }
        oldEnv.append(QString{"DEEPIN_WINE_SCALE=%1;"}.arg(factor));
        optionsMap.insert("env", oldEnv);
    }

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
            QString msg{"application can't be executed."};
            qWarning() << msg;
            sendErrorReply(QDBusError::Failed, msg);
            return {};
        }
        execStr = Actions->toString(ok);
        if (!ok) {
            QString msg{"maybe entry actions's format is invalid, abort launch."};
            qWarning() << msg;
            sendErrorReply(QDBusError::Failed, msg);
            return {};
        }
    }

    auto cmds = generateCommand(optionsMap);

    auto [bin, execCmds, res] = unescapeExec(execStr, fields);
    if (bin.isEmpty()) {
        qCritical() << "error command is detected, abort.";
        sendErrorReply(QDBusError::Failed);
        return {};
    }

    cmds.append(std::move(execCmds));
    auto &jobManager = static_cast<ApplicationManager1Service *>(parent())->jobManager();
    return jobManager.addJob(
        m_applicationPath.path(),
        [this, binary = std::move(bin), commands = std::move(cmds)](const QVariant &variantValue) mutable -> QVariant {
            auto resourceFile = variantValue.toString();
            auto instanceRandomUUID = QUuid::createUuid().toString(QUuid::Id128);
            auto objectPath = m_applicationPath.path() + "/" + instanceRandomUUID;
            commands.push_front(QString{"--SourcePath=%1"}.arg(m_desktopSource.sourcePath()));

            auto location = commands.indexOf(R"(%f)");
            if (location != -1) {  // due to std::move, there only remove once
                commands.remove(location);
            }

            if (resourceFile.isEmpty()) {
                commands.push_front(QString{R"(--unitName=app-DDE-%1@%2.service)"}.arg(
                    escapeApplicationId(this->id()), instanceRandomUUID));  // launcher should use this instanceId
                QProcess process;
                qDebug() << "run with commands:" << commands;
                process.start(m_launcher, commands);
                process.waitForFinished();
                if (auto code = process.exitCode(); code != 0) {
                    qWarning() << "Launch Application Failed";
                    return QDBusError::Failed;
                }
                return objectPath;
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
            qDebug() << "run with commands:" << commands;
            process.start(getApplicationLauncherBinary(), commands);
            process.waitForFinished();
            auto exitCode = process.exitCode();
            if (exitCode != 0) {
                qWarning() << "Launch Application Failed";
                return QDBusError::Failed;
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
        sendErrorReply(QDBusError::ErrorType::Failed, m_desktopSource.sourceFileRef().errorString());
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
        sendErrorReply(QDBusError::ErrorType::Failed, desktopFile.errorString());
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

bool ApplicationService::noDisplay() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "NoDisplay", EntryValueType::Boolean);

    if (val.isNull()) {
        return false;
    }

    return val.toBool();
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
    return !val.isNull();
}

bool ApplicationService::x_linglong() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "X-linglong", EntryValueType::String);
    return !val.isNull();
}

qulonglong ApplicationService::installedTime() const noexcept
{
    return m_desktopSource.createTime();
}

qulonglong ApplicationService::lastLaunchedTime() const noexcept
{
    return m_lastLaunch;
}

bool ApplicationService::autostartCheck(const QString &linkPath) noexcept
{
    QFileInfo info{linkPath};

    if (info.exists()) {
        if (info.isSymbolicLink()) {
            return true;
        }
        qWarning() << "same name desktop file exists:" << linkPath << "but this may not created by AM.";
    }

    return false;
}

bool ApplicationService::isAutoStart() const noexcept
{
    auto path = getAutoStartDirs().first();
    auto linkName = QDir{path}.filePath(m_desktopSource.desktopId() + ".desktop");
    return autostartCheck(linkName);
}

void ApplicationService::setAutoStart(bool autostart) noexcept
{
    auto path = getAutoStartDirs().first();
    auto linkName = QDir{path}.filePath(m_desktopSource.desktopId() + ".desktop");
    auto &file = m_desktopSource.sourceFileRef();

    if (autostart) {
        if (!autostartCheck(linkName) and !file.link(linkName)) {
            qWarning() << "link to autostart failed:" << file.errorString();
            return;
        }
    } else {
        if (autostartCheck(linkName)) {
            QFile linkFile{linkName};
            if (!linkFile.remove()) {
                qWarning() << "remove link failed:" << linkFile.errorString();
                return;
            }
        }
    }

    emit autostartChanged();
}

QList<QDBusObjectPath> ApplicationService::instances() const noexcept
{
    return m_Instances.keys();
}

bool ApplicationService::addOneInstance(const QString &instanceId,
                                        const QString &application,
                                        const QString &systemdUnitPath,
                                        const QString &launcher)
{
    auto *service = new InstanceService{instanceId, application, systemdUnitPath, launcher};
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
    for (auto it = m_Instances.constKeyValueBegin(); it != m_Instances.constKeyValueEnd(); ++it) {
        const auto &[path, ptr] = *it;
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
    auto deleter = [](wordexp_t *word) { wordfree(word); };
    std::unique_ptr<wordexp_t, decltype(deleter)> words{new (std::nothrow) wordexp_t{0, nullptr, 0}, deleter};

    if (auto ret = wordexp(str.toLocal8Bit().constData(), words.get(), WRDE_SHOWERR); ret != 0) {
        if (ret != 0) {
            QString errMessage;
            switch (ret) {
            case WRDE_BADCHAR:
                errMessage = "BADCHAR";
                qWarning() << "wordexp error: " << errMessage;
                return {};
            case WRDE_BADVAL:
                errMessage = "BADVAL";
                break;
            case WRDE_CMDSUB:
                errMessage = "CMDSUB";
                break;
            case WRDE_NOSPACE:
                errMessage = "NOSPACE";
                break;
            case WRDE_SYNTAX:
                errMessage = "SYNTAX";
                break;
            default:
                errMessage = "unknown";
            }
            qWarning() << "wordexp error: " << errMessage;
            return {};
        }
    }

    QStringList execList;
    for (int i = 0; i < words->we_wordc; ++i) {
        execList.emplace_back(words->we_wordv[i]);
    }
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
        for (const auto &field : fields) {
            task.Resources.emplace_back(field);
        }
    } break;
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
    } break;
    case 'F':
        [[fallthrough]];
    case 'U': {
        execList.removeAt(location);
        auto it = execList.begin() + location;
        for (const auto &field : fields) {
            it = execList.insert(it, field);
        }
        task.command.append(std::move(execList));
    } break;
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
    } break;
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
    } break;
    case 'k': {  // ignore all desktop file location for now.
        execList.removeAt(location);
        task.command.append(std::move(execList));
    } break;
    case 'd':
    case 'D':
    case 'n':
    case 'N':
    case 'v':
        [[fallthrough]];  // Deprecated field codes should be removed from the command line and ignored.
    case 'm': {
        execList.removeAt(location);
        task.command.append(std::move(execList));
    } break;
    default: {
        qDebug() << "unrecognized file code.";
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

QString getDeepinWineScaleFactor(const QString &appId) noexcept
{
    qCritical() << "Don't using env to control the window scale factor,  this function"
                   "should via using graphisc server(Wayland Compositor/Xorg Xft) in deepin wine.";

    QString factor{"1.0"};
    auto objectPath = QString{"/dde_launcher/org_deepin_dde_launcher/%1"}.arg(getCurrentUID());
    auto systemBus = QDBusConnection::systemBus();

    auto msg = QDBusMessage::createMethodCall(
        "org.desktopspec.ConfigManager", objectPath, "org.desktopspec.ConfigManager.Manager", "value");
    msg.setArguments({QString{"Apps_Disable_Scaling"}});
    auto reply = systemBus.call(msg);

    if (reply.type() != QDBusMessage::ReplyMessage) {
        qWarning() << "get Apps_Disable_Scaling failed:" << reply.errorMessage();
        return factor;
    }

    QDBusReply<QDBusVariant> ret{reply};
    if (!ret.isValid()) {
        qWarning() << "invalid reply:" << ret.error();
        return factor;
    }

    QVariantList appList;
    ret.value().variant().value<QDBusArgument>() >> appList;

    for (const auto &val : appList) {
        if (val.value<QString>() == appId) {
            return factor;
        }
    }

    auto sessionBus = QDBusConnection::sessionBus();
    QDBusMessage reply1 = sessionBus.call(QDBusMessage::createMethodCall(
        "org.deepin.dde.XSettings1", "/org/deepin/dde/XSettings1", "org.deepin.dde.XSettings1", "GetScaleFactor"));

    if (reply1.type() != QDBusMessage::ReplyMessage) {
        qWarning() << "call GetScaleFactor Failed:" << reply1.errorMessage();
        return factor;
    }

    QDBusReply<double> ret1(reply1);
    double scale = ret1.isValid() ? ret1.value() : 1.0;
    scale = scale > 0 ? scale : 1;
    factor = QString::number(scale, 'f', -1);

    return factor;
}
