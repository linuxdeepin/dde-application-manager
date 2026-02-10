// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationservice.h"
#include "APPobjectmanager1adaptor.h"
#include "applicationchecker.h"
#include "applicationmanagerstorage.h"
#include "config.h"
#include "constant.h"
#include "dbus/instanceadaptor.h"
#include "desktopentry.h"
#include "desktopfileparser.h"
#include "global.h"
#include "iniParser.h"
#include "launchoptions.h"
#include "prelaunchsplashhelper.h"
#include "propertiesForwarder.h"
#include <DConfig>
#include <QList>
#include <QLoggingCategory>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QUrl>
#include <QUuid>
#include <algorithm>
#include <new>
#include <qcontainerfwd.h>
#include <qdbuserror.h>
#include <qfileinfo.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtmetamacros.h>
#include <utility>
#include <wordexp.h>

using namespace Qt::Literals::StringLiterals;

namespace {

void appendEnvs(const QVariant &var, QStringList &envs)
{
    if (var.canConvert<QStringList>()) {
        envs.append(var.value<QStringList>());
    } else if (var.canConvert<QString>()) {
        envs.append(var.value<QString>().split(";", Qt::SkipEmptyParts));
    }
}

void unescapeEnvs(QVariantMap &options) noexcept
{
    if (options.constFind("env") == options.cend()) {
        return;
    }
    QStringList result;
    const auto &envsVar = options["env"];
    auto envs = envsVar.toStringList();
    for (const auto &var : std::as_const(envs)) {
        if (var.startsWith(u"DSG_APP_ID="_s)) {
            result << var;
            continue;
        }
        wordexp_t p;
        if (wordexp(var.toStdString().c_str(), &p, 0) == 0) {
            for (size_t i = 0; i < p.we_wordc; i++) {
                result << QString::fromLocal8Bit(p.we_wordv[i]);
            }
            wordfree(&p);
        } else {
            return;
        }
    }

    options.insert("env", result);
}

} // namespace

void ApplicationService::appendExtraEnvironments(QVariantMap &runtimeOptions) const noexcept
{
    DCORE_USE_NAMESPACE
    QStringList envs, unsetEnvs;
    const QString &env = environ();
    if (!env.isEmpty())
        envs.append(env);

    if (auto it = runtimeOptions.find("env"); it != runtimeOptions.end()) {
        appendEnvs(*it, envs);
    }

    if (auto it = runtimeOptions.find("unsetEnv"); it != runtimeOptions.end()) {
        appendEnvs(*it, unsetEnvs);
    }

    std::unique_ptr<DConfig> config(DConfig::create(ApplicationServiceID,
                                                    ApplicationManagerConfig,
                                                    QString("/%1").arg((id()))));  // $appid as subpath
    if (config->isValid()) {
        const QStringList &extraEnvs = config->value(AppExtraEnvironments).toStringList();
        if (!extraEnvs.isEmpty())
            envs.append(extraEnvs);

        const QStringList &envsBlacklist = config->value(AppEnvironmentsBlacklist).toStringList();
        if (!envsBlacklist.isEmpty())
            unsetEnvs.append(envsBlacklist);
    }

    // it's useful for App to get itself AppId.
    envs.append(QString{"DSG_APP_ID=%1"}.arg(id()));

    runtimeOptions.insert("env", envs);
    runtimeOptions.insert("unsetEnv", unsetEnvs);
}

void ApplicationService::processCompatibility(const QString &action, QVariantMap &options, QString &execStr)
{
    auto compatibilityManager = parent()->getCompatibilityManager();

    auto getExec = [action, compatibilityManager](const QString& desktopID) -> QString {
        std::optional<QString> execValue;
        if (!action.isEmpty()) {
            const auto &actionHeader = QString{"%1%2"}.arg(DesktopFileActionKey, action);
            execValue = compatibilityManager->getExec(desktopID, actionHeader);
        } else {
            execValue = compatibilityManager->getExec(desktopID, DesktopFileEntryKey);
        }

        if (!execValue.has_value() || execValue->isEmpty()) {
            return {};
        }
        return execValue.value();
    };

    auto addEnv = [this, &options, compatibilityManager]() {
        auto envValue = compatibilityManager->getEnv(m_desktopSource.desktopId(), DesktopFileEntryKey);
        if (!envValue.isEmpty()) {
            if (auto it = options.find("env"); it != options.end()) {
                auto value = it->toStringList();
                value.append(envValue);
                options.insert("env", value);
            } else {
                options.insert("env", envValue);
            }
        }
    };

    auto exec = getExec(m_desktopSource.desktopId());
    if (!exec.isEmpty()) {
        execStr = exec;
        addEnv();
        qInfo() << "get compatibility : " << m_desktopSource.desktopId() << " Exec : " << execStr;
    }
}

ApplicationService::ApplicationService(DesktopFile source,
                                       ApplicationManager1Service *parent,
                                       std::weak_ptr<ApplicationManager1Storage> storage)
    : QObject(parent)
    , m_storage(std::move(storage))
    , m_desktopSource(std::move(source))
{
    auto storagePtr = m_storage.lock();
    if (!storagePtr) {
        m_lastLaunch = -1;
        return;
    }

    auto appId = id();
    auto value = storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, InstalledTime);

    if (storagePtr->firstLaunch()) {
        auto ret = value.isNull() ? storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, InstalledTime, 0) :
                                    storagePtr->updateApplicationValue(appId, ApplicationPropertiesGroup, InstalledTime, 0);
        if (!ret) {
            m_installedTime = -1;
            qWarning() << "failed to set InstalledTime for" << appId << "at first launch";
        }
    } else {
        if (value.isNull()) {
            auto newInstalledTime = QDateTime::currentMSecsSinceEpoch();
            if (!storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, InstalledTime, newInstalledTime)) {
                m_installedTime = -1;
                qWarning() << "failed to set InstalledTime for new apps:" << appId;
            } else {
                m_installedTime = newInstalledTime;
            }
        } else {
            m_installedTime = value.toLongLong();
        }
    }

    value = storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, LastLaunchedTime);
    if (value.isNull()) {
        if (!storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, LastLaunchedTime, 0)) {
            qWarning() << "failed to set LastLaunchedTime for" << appId;
            m_lastLaunch = -1;
        }
    } else {
        m_lastLaunch = value.toLongLong();
    }

    value = storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, ::LaunchedTimes);
    if (value.isNull()) {
        if (!storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, ::LaunchedTimes, 0)) {
            qWarning() << "failed to set LaunchedTimes for" << appId;
            m_launchedTimes = -1;
        }
    } else {
        m_launchedTimes = value.toLongLong();
    }

    value = storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, ::Environ);
    if (!value.isNull()) {
        m_environ = value.toString();
    }
}

ApplicationService::~ApplicationService()
{
    detachAllInstance();
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

    objectPath = getObjectPathFromAppId(app->desktopFileSource().desktopId());
    DesktopFileGuard guard{app->desktopFileSource()};

    if (!guard.try_open()) {
        qDebug() << "open source desktop failed.";
        return nullptr;
    }

    sourceStream.setDevice(app->desktopFileSource().sourceFile());
    std::unique_ptr<DesktopEntry> entry{std::make_unique<DesktopEntry>()};
    auto error = entry->parse(sourceStream);

    if (error != ParserError::NoError) {
        qDebug() << "parse failed:" << error << app->desktopFileSource().sourcePath();
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

    return app;
}

bool ApplicationService::shouldBeShown(const std::unique_ptr<DesktopEntry> &entry) noexcept
{
    if (ApplicationFilter::hiddenCheck(entry)) {
        qDebug() << "hidden check failed.";
        return false;
    }

    if (ApplicationFilter::tryExecCheck(entry)) {
        qDebug() << "tryExec check failed";
        return false;
    }

    if (ApplicationFilter::showInCheck(entry)) {
        qDebug() << "showIn check failed.";
        return false;
    }

    return true;
}

QDBusObjectPath
ApplicationService::Launch(const QString &action, const QStringList &fields, const QVariantMap &options, const QString &realExec)
{
    QString execStr{};
    const auto &supportedActions = actions();
    auto optionsMap = options;
    appendExtraEnvironments(optionsMap);

    if (realExec.isEmpty()) {  // we want to replace exec of this applications.
        execStr = realExec;
    }

    auto workDir = m_entry->value(DesktopFileEntryKey, "Path");
    if (workDir && !optionsMap.contains(setWorkingPathLaunchOption::key())) {
        optionsMap.insert(setWorkingPathLaunchOption::key(), workDir->toString());
    }

    while (execStr.isEmpty() && !action.isEmpty() && !supportedActions.isEmpty()) {  // break trick
        if (auto index = supportedActions.indexOf(action); index == -1) {
            qWarning() << "can't find " << action << " in supported actions List. application will use default action to launch.";
            break;
        }

        const auto &actionHeader = QString{"%1%2"}.arg(DesktopFileActionKey, action);
        const auto &actionExec = m_entry->value(actionHeader, "Exec");
        if (!actionExec) {
            break;
        }

        execStr = toString(actionExec.value());
        if (execStr.isEmpty()) {
            qWarning() << "exec value to string failed, try default action.";  // we need this log.
        }

        break;
    }

    if (execStr.isEmpty()) {
        auto Actions = m_entry->value(DesktopFileEntryKey, "Exec");
        if (!Actions) {
            const QString msg{"application can't be executed."};
            qWarning() << msg;
            safe_sendErrorReply(QDBusError::Failed, msg);
            return {};
        }

        execStr = Actions.value().toString();
        if (execStr.isEmpty()) {
            const QString msg{"maybe entry actions's format is invalid, abort launch."};
            qWarning() << msg;
            safe_sendErrorReply(QDBusError::Failed, msg);
            return {};
        }
    }

    // Notify the compositor to show a splash screen if in a Wayland session.
    // Suppress splash for system autostart launches or singleton apps with existing instances.
    const bool isAutostartLaunch = optionsMap.contains("_autostart") && optionsMap.value("_autostart").toBool();
    const bool isSingleton = findEntryValue(DesktopFileEntryKey, "X-Deepin-Singleton", EntryValueType::Boolean).toBool();
    const bool singletonWithInstance = isSingleton && !m_Instances.isEmpty();
    if (isAutostartLaunch) {
        qCInfo(amPrelaunchSplash) << "Skip prelaunch splash (autostart)" << id();
    } else if (singletonWithInstance) {
        qCInfo(amPrelaunchSplash) << "Skip prelaunch splash (singleton with existing instance)" << id();
    } else if (auto *am = parent()) {
        if (auto *helper = am->splashHelper()) {
            const auto iconVar = findEntryValue(DesktopFileEntryKey, "Icon", EntryValueType::IconString);
            const QString iconName = iconVar.isNull() ? QString{} : iconVar.toString();
            qCInfo(amPrelaunchSplash) << "Show prelaunch splash request" << id() << "icon" << iconName;
            helper->show(id(), iconName);
        } else {
            qCInfo(amPrelaunchSplash) << "Skip prelaunch splash (no helper instance)" << id();
        }
    } else {
        qCWarning(amPrelaunchSplash) << "Skip prelaunch splash (no parent ApplicationManager1Service)" << id();
    }

    // Those are internal properties, user shouldn't pass them to Application Manager
    optionsMap.remove("_autostart");
    optionsMap.remove("_hooks");
    optionsMap.remove("_builtIn_searchExec");
    if (const auto &hooks = parent()->applicationHooks(); !hooks.isEmpty()) {
        optionsMap.insert("_hooks", hooks);
    }
    optionsMap.insert("_builtIn_searchExec", parent()->systemdPathEnv());

    processCompatibility(action, optionsMap, execStr);
    unescapeEnvs(optionsMap);

    auto workingDir = optionsMap.value("path").toString();
    if (!workingDir.isEmpty() && QDir::isRelativePath(workingDir)) {
        qWarning() << "relative working dir is not supported: " << workingDir;
        workingDir.clear();
    }

    auto cmds = generateCommand(optionsMap);
    auto task = processExec(execStr, fields, workingDir);
    if (!task) {
        safe_sendErrorReply(QDBusError::InternalError, "Invalid Command.");
        return {};
    }

    if (task.LaunchBin.isEmpty()) {
        qCritical() << "error command is detected, abort.";
        safe_sendErrorReply(QDBusError::Failed);
        return {};
    }

    if (terminal()) {
        // don't change this sequence
        cmds.push_back("deepin-terminal");
        cmds.push_back("--keep-open");
        cmds.push_back("-e");           // run all original execution commands in deepin-terminal
    }

    auto &jobManager = parent()->jobManager();
    return jobManager.addJob(
        m_applicationPath.path(),
        [this, task = task, cmds = std::move(cmds)](const QVariant &value) mutable -> QVariant {
            const auto instanceRandomUUID = QUuid::createUuid().toString(QUuid::Id128);
            const auto objectPath = m_applicationPath.path() + "/" + instanceRandomUUID;

            QStringList newCommands;
            const int estimatedSize = 5 + cmds.size() + task.command.size() + (value.isValid() ? 1 : 0);
            newCommands.reserve(estimatedSize);
            newCommands
                << QStringLiteral("--unitName=app-DDE-%1@%2.service").arg(escapeApplicationId(this->id()), instanceRandomUUID);
            newCommands << QStringLiteral("--SourcePath=%1").arg(m_desktopSource.sourcePath());
            newCommands << std::move(cmds);

            QStringList formattedRes;
            if (!value.isNull()) {
                QList<QUrl> urls;
                if (value.canConvert<QList<QUrl>>()) {
                    urls = std::move(value).value<QList<QUrl>>();
                } else {
                    urls.append(std::move(value).value<QUrl>());
                }

                formattedRes.reserve(urls.size());
                for (const auto &url : std::as_const(urls)) {
                    if (!task.local) {
                        formattedRes << url.toString();
                    } else if (url.isLocalFile()) {
                        formattedRes << url.toLocalFile();
                    } else {
                        // TODO: Remote file handling logic
                        qWarning() << "Remote file not supported yet, skipping:" << url;
                    }
                }
            }

            for (int i = 0; i < task.command.size(); ++i) {
                if (i != task.argNum) {
                    newCommands << std::move(task.command[i]);
                    continue;
                }

                auto targetArg = std::move(task.command[i]);
                if (task.fieldLocation != -1) {
                    if (formattedRes.size() > 1) {
                        qWarning() << "multiple resources are found, only the first one will be used.";
                    }

                    targetArg.replace(task.fieldLocation, 2, formattedRes.isEmpty() ? QString{} : formattedRes.takeFirst());
                    newCommands << std::move(targetArg);

                    if (!formattedRes.isEmpty()) {
                        newCommands << std::move(formattedRes);
                    }
                } else {
                    newCommands << std::move(formattedRes);
                }
            }

            QProcess process;
            const auto &bin = getApplicationLauncherBinary();
            qDebug().noquote() << "Launcher path:" << bin << ", Run with commands:" << newCommands;

            process.start(bin, newCommands);
            process.waitForFinished();
            auto exitCode = process.exitCode();
            if (exitCode != 0) {
                qWarning() << "Launch Application Failed";
                return QDBusError::Failed;
            }

            return objectPath;
        },
        std::move(task.Resources));
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
        safe_sendErrorReply(QDBusError::ErrorType::Failed, m_desktopSource.sourceFileRef().errorString());
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
        safe_sendErrorReply(QDBusError::ErrorType::Failed, desktopFile.errorString());
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
    const auto &actionList = actions();

    for (const auto &action : actionList) {
        const QString rawActionKey = DesktopFileActionKey % action;
        auto value = m_entry->value(rawActionKey, "Name");
        if (!value.has_value()) {
            continue;
        }
        ret.insert(action, std::move(value).value().value<QStringMap>());
    }

    return ret;
}

QStringMap ApplicationService::name() const noexcept
{
    auto value = m_entry->value(DesktopFileEntryKey, "Name");
    if (!value) {
        return {};
    }

    if (!value->canConvert<QStringMap>()) {
        return {};
    }

    return value->value<QStringMap>();
}

QStringMap ApplicationService::genericName() const noexcept
{
    auto value = m_entry->value(DesktopFileEntryKey, "GenericName");
    if (!value) {
        return {};
    }

    if (!value->canConvert<QStringMap>()) {
        return {};
    }

    return value->value<QStringMap>();
}

QStringMap ApplicationService::icons() const noexcept
{
    QStringMap ret;
    const auto &actionList = actions();
    for (const auto &action : std::as_const(actionList)) {
        auto actionKey = QString{action}.prepend(DesktopFileActionKey);
        auto value = m_entry->value(actionKey, "Icon");
        if (!value.has_value()) {
            continue;
        }
        ret.insert(actionKey, value->value<QString>());
    }

    auto mainIcon = m_entry->value(DesktopFileEntryKey, "Icon");
    if (mainIcon.has_value()) {
        ret.insert(DesktopFileEntryKey, mainIcon->value<QString>());
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

QString ApplicationService::X_Deepin_Vendor() const noexcept
{
    return findEntryValue(DesktopFileEntryKey, "X-Deepin-Vendor", EntryValueType::String).toString();
}

QString ApplicationService::X_Deepin_CreateBy() const noexcept
{
    return findEntryValue(DesktopFileEntryKey, "X-Deepin-CreateBy", EntryValueType::String).toString();
}

QStringMap ApplicationService::execs() const noexcept
{
    QStringMap ret;

    auto mainExec = m_entry->value(DesktopFileEntryKey, "Exec");
    if (mainExec.has_value()) {
        ret.insert(DesktopFileEntryKey, mainExec->value<QString>());
    }

    const auto &actionList = actions();
    for (const auto &action : std::as_const(actionList)) {
        auto actionKey = QString{action}.prepend(DesktopFileActionKey);
        auto value = m_entry->value(actionKey, "Exec");
        if (!value.has_value()) {
            continue;
        }
        ret.insert(actionKey, value->value<QString>());
    }

    return ret;
}

QString ApplicationService::X_CreatedBy() const noexcept
{
    return findEntryValue(DesktopFileEntryKey, "X-Created-By", EntryValueType::String).toString();
}

bool ApplicationService::terminal() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "Terminal", EntryValueType::String);
    if (!val.isNull()) {
        return val.toBool();
    }
    return false;
}

QString ApplicationService::startupWMClass() const noexcept
{
    auto value = findEntryValue(DesktopFileEntryKey, "StartupWMClass", EntryValueType::String);
    return value.isNull() ? QString{} : value.toString();
}

qint64 ApplicationService::installedTime() const noexcept
{
    return m_installedTime;
}

qint64 ApplicationService::lastLaunchedTime() const noexcept
{
    return m_lastLaunch;
}

qint64 ApplicationService::launchedTimes() const noexcept
{
    return m_launchedTimes;
}

QString ApplicationService::environ() const noexcept
{
    return m_environ;
}

void ApplicationService::setEnviron(const QString &value) noexcept
{
    if (environ().trimmed() == value.trimmed()) {
        return;
    }

    auto storagePtr = m_storage.lock();
    if (!storagePtr) {
        qCritical() << "broken storage.";
        safe_sendErrorReply(QDBusError::InternalError);
        return;
    }

    auto appId = id();
    if (!storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, Environ).isNull()) {
        if (!storagePtr->updateApplicationValue(appId, ApplicationPropertiesGroup, Environ, value)) {
            safe_sendErrorReply(QDBusError::Failed, "update environ failed.");
            return;
        }
    } else {
        if (!storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, Environ, value)) {
            safe_sendErrorReply(QDBusError::Failed, "set environ failed.");
        }
    }

    m_environ = value;
    emit environChanged();
}

bool ApplicationService::autostartCheck(const QString &filePath) const noexcept
{
    qDebug() << "current check autostart file:" << filePath;

    DesktopEntry s;
    if (!m_autostartSource.m_entry.data().isEmpty()) {
        s = m_autostartSource.m_entry;
    } else {
        QFile file{filePath};
        if (!file.open(QFile::ExistingOnly | QFile::ReadOnly | QFile::Text)) {
            qWarning() << "open" << filePath << "failed:" << file.errorString();
            return false;
        }

        QTextStream stream{&file};
        if (auto err = s.parse(stream); err != ParserError::NoError) {
            qWarning() << "parse" << filePath << "failed:" << err;
            return false;
        }
    }

    QString source = s.value(DesktopFileEntryKey, X_Deepin_GenerateSource).value_or(DesktopEntry::Value{}).toString();
    // file has been removed
    if (source != m_autostartSource.m_filePath && filePath != m_autostartSource.m_filePath) {
        return false;
    }

    auto hiddenVal = s.value(DesktopFileEntryKey, DesktopEntryHidden);
    if (!hiddenVal) {
        qDebug() << "no hidden in autostart desktop";
        return true;
    }

    auto hidden = hiddenVal.value().toString();
    return hidden.compare("false", Qt::CaseInsensitive) == 0;
}

bool ApplicationService::isAutoStart() const noexcept
{
    if (m_autostartSource.m_filePath.isEmpty()) {
        return false;
    }

    auto appId = id();
    auto dirs = getAutoStartDirs();
    QString destDesktopFile;

    applyIteratively(
        QList<QDir>(dirs.crbegin(), dirs.crend()),
        [&appId, &destDesktopFile](const QFileInfo &file) {
            auto filePath = file.absoluteFilePath();
            if (appId == getAutostartAppIdFromAbsolutePath(filePath)) {
                destDesktopFile = filePath;
            }
            return false;
        },
        QDir::Readable | QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    return autostartCheck(destDesktopFile);
}

void ApplicationService::setAutoStart(bool autostart) noexcept
{
    if (isAutoStart() == autostart) {
        return;
    }

    const QDir startDir(getAutoStartDirs().constFirst());
    if (!startDir.exists() && !startDir.mkpath(startDir.path())) {
        qWarning() << "mkpath " << startDir.path() << "failed";
        safe_sendErrorReply(QDBusError::InternalError);
        return;
    }

    auto fileName = startDir.filePath(m_desktopSource.desktopId() + ".desktop");
    QFile autostartFile{fileName};
    if (!autostartFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << "open file" << fileName << "failed:" << autostartFile.error();
        safe_sendErrorReply(QDBusError::Failed);
        return;
    }

    DesktopEntry newEntry;
    QString originalSource;
    if (!m_autostartSource.m_entry.data().isEmpty()) {
        newEntry = m_autostartSource.m_entry;
        originalSource = newEntry.value(DesktopFileEntryKey, X_Deepin_GenerateSource)
                             .value_or(DesktopEntry::Value{m_autostartSource.m_filePath})
                             .toString();
    } else {
        newEntry = *m_entry;
        originalSource = m_desktopSource.sourcePath();
    }

    newEntry.insert(DesktopFileEntryKey, X_Deepin_GenerateSource, originalSource);
    newEntry.insert(DesktopFileEntryKey, DesktopEntryHidden, !autostart);

    setAutostartSource({fileName, newEntry});

    auto hideAutostart = toString(newEntry.data()).toLocal8Bit();
    auto writeBytes = autostartFile.write(hideAutostart);

    if (writeBytes != hideAutostart.size() or !autostartFile.flush()) {
        qWarning() << "incomplete write:" << autostartFile.error();
        safe_sendErrorReply(QDBusError::Failed, "set failed: filesystem error.");
        return;
    }

    emit autostartChanged();
}

QStringList ApplicationService::mimeTypes() const noexcept
{
    QStringList ret;
    const auto &desktopFilePath = m_desktopSource.sourcePath();
    const auto &cacheList = parent()->mimeManager().infos();
    auto cache = std::find_if(cacheList.cbegin(), cacheList.cend(), [&desktopFilePath](const MimeInfo &info) {
        return desktopFilePath.startsWith(info.directory());
    });

    if (cache == cacheList.cend()) {
        qWarning() << "error occurred when get mimeTypes for" << desktopFilePath;
        return ret;
    }

    const auto &info = cache->cacheInfo();
    if (info) {
        ret.append(info->queryTypes(id()));
    }

    AppList tmp;

    for (auto it = cacheList.crbegin(); it != cacheList.crend(); ++it) {
        const auto &list = it->appsList();
        std::for_each(list.crbegin(), list.crend(), [&tmp, this](const MimeApps &app) {
            auto [added, removed] = app.queryTypes(id());
            tmp.added.append(std::move(added));
            tmp.removed.append(std::move(removed));
        });
    };

    tmp.added.removeDuplicates();
    tmp.removed.removeDuplicates();
    for (const auto &it : std::as_const(tmp.removed)) {
        tmp.added.removeOne(it);
    }

    ret.append(std::move(tmp.added));
    ret.removeDuplicates();
    return ret;
}

void ApplicationService::setMimeTypes(const QStringList &value) noexcept
{
    auto oldMimes = mimeTypes();
    auto newMimes = value;
    std::sort(oldMimes.begin(), oldMimes.end());
    std::sort(newMimes.begin(), newMimes.end());

    QStringList newAdds;
    QStringList newRemoved;

    std::set_difference(oldMimes.begin(), oldMimes.end(), newMimes.begin(), newMimes.end(), std::back_inserter(newRemoved));
    std::set_difference(newMimes.begin(), newMimes.end(), oldMimes.begin(), oldMimes.end(), std::back_inserter(newAdds));

    static QString userDir = getXDGConfigHome();
    auto &infos = parent()->mimeManager().infos();
    auto userInfo = std::find_if(infos.begin(), infos.end(), [](const MimeInfo &info) { return info.directory() == userDir; });
    if (userInfo == infos.cend()) {
        safe_sendErrorReply(QDBusError::Failed, "user-specific config file doesn't exists.");
        return;
    }

    const auto &list = userInfo->appsList().rbegin();
    const auto &appId = id();
    for (const auto &add : std::as_const(newAdds)) {
        list->addAssociation(add, appId);
    }
    for (const auto &remove :std::as_const(newRemoved)) {
        list->removeAssociation(remove, appId);
    }

    if (!list->writeToFile()) {
        qWarning() << "error occurred when write mime association to file";
    }

    emit MimeTypesChanged();
}

QList<QDBusObjectPath> ApplicationService::instances() const noexcept
{
    return m_Instances.keys();
}

bool ApplicationService::addOneInstance(const QString &instanceId,
                                        const QString &application,
                                        const QString &systemdUnitPath,
                                        const QString &launcher) noexcept
{
    auto *service = new (std::nothrow) InstanceService{instanceId, application, systemdUnitPath, launcher};
    if (service == nullptr) {
        qCritical() << "couldn't new InstanceService.";
        return false;
    }

    auto *adaptor = new (std::nothrow) InstanceAdaptor{service};
    QString objectPath{m_applicationPath.path() + "/" + instanceId};

    if (adaptor == nullptr or !registerObjectToDBus(service, objectPath, InstanceInterface)) {
        adaptor->deleteLater();
        service->deleteLater();
        return false;
    }

    m_Instances.insert(QDBusObjectPath{objectPath}, QSharedPointer<InstanceService>{service});
    service->moveToThread(this->thread());
    adaptor->moveToThread(this->thread());
    emit InterfacesAdded(QDBusObjectPath{objectPath}, getChildInterfacesAndPropertiesFromObject(service));

    return true;
}

void ApplicationService::removeOneInstance(const QDBusObjectPath &instance) noexcept
{
    if (auto it = m_Instances.constFind(instance); it != m_Instances.cend()) {
        emit InterfacesRemoved(instance, getChildInterfacesFromObject(it->data()));
        unregisterObjectFromDBus(instance.path());
        m_Instances.remove(instance);
    }
}

void ApplicationService::removeAllInstance() noexcept
{
    for (auto it = m_Instances.constBegin(); it != m_Instances.constEnd(); ++it) {
        removeOneInstance(it.key());
    }
}

void ApplicationService::detachAllInstance() noexcept
{
    for (auto it = m_Instances.constBegin(); it != m_Instances.constEnd(); ++it) {
        const auto& instance = it.value();
        orphanedInstances.append(instance);
        instance->setProperty("Orphaned", true);
    }

    m_Instances.clear();
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
    emit autostartChanged();
    emit noDisplayChanged();
    emit isOnDesktopChanged();
    emit installedTimeChanged();
    emit x_FlatpakChanged();
    emit x_linglongChanged();
    emit instanceChanged();
    emit lastLaunchedTimeChanged();
    emit iconsChanged();
    emit nameChanged();
    emit genericNameChanged();
    emit actionNameChanged();
    emit actionsChanged();
    emit categoriesChanged();
    emit MimeTypesChanged();
    emit terminalChanged();
    emit environChanged();
    emit launchedTimesChanged();
    emit startupWMClassChanged();
    emit xDeepinCreatedByChanged();
    emit execsChanged();
    emit xCreatedByChanged();
}

std::optional<QStringList> ApplicationService::splitExecArguments(QStringView str) noexcept
{
    if (str.isEmpty()) {
        return std::nullopt;
    }

    QStringList args;
    QString currentToken;
    currentToken.reserve(str.size());

    bool inQuotes{false};
    bool hasToken{false};

    for (const auto *it = str.begin(); it != str.end(); ++it) {
        const auto c = *it;

        if (c == u'\\') {
            if ((it + 1) == str.end()) {
                qWarning() << "Exec parsing error: Backslash at end of line";
                return std::nullopt;
            }

            const auto next = *(++it);

            if (inQuotes) {
                if (next == u'"' || next == u'\\' || next == u'$' || next == u'`') {
                    currentToken.append(next);
                } else {
                    currentToken.append(u'\\');
                    currentToken.append(next);
                }
            } else {
                currentToken.append(next);
            }
            hasToken = true;
            continue;
        }

        if (c == u'"') {
            inQuotes = !inQuotes;
            hasToken = true;
            continue;
        }

        if (c.isSpace() && !inQuotes) {
            if (hasToken) {
                args.append(currentToken);
                currentToken.clear();
                hasToken = false;
            }
        } else {
            currentToken.append(c);
            hasToken = true;
        }
    }

    if (inQuotes) {
        qWarning() << "Exec parsing error: Unterminated double quote";
        return std::nullopt;
    }

    if (hasToken) {
        args.append(currentToken);
    }

    return args;
}

LaunchTask ApplicationService::processExec(const QString &str, const QStringList& fields, const QString& dir) noexcept
{
    auto args = splitExecArguments(str);
    if (!args) {
        qWarning() << "splitExecArguments failed.";
        return {};
    }

    if (args->isEmpty()) {
        qWarning() << "exec format is invalid.";
        return {};
    }

    qDebug() << "splitExecArguments:" << *args;

    LaunchTask task;
    task.LaunchBin = args->first();
    task.command.reserve(args->size() + 2); // 2 for icon

    const QChar percentage{u'%'};
    bool exclusiveField{false};

    for (auto &rawArg : *args) {
        if (!rawArg.contains(percentage)) {
            task.command.append(std::move(rawArg));
            continue;
        }

        QString processedArg;
        processedArg.reserve(rawArg.size());
        bool dynamicField{false};

        for (qsizetype j = 0; j < rawArg.size(); ++j) {
            const auto ch = rawArg[j];
            if (ch != percentage || j + 1 >= rawArg.size()) {
                processedArg.append(ch);
                continue;
            }

            const auto code = rawArg[++j];
            switch (code.unicode()) {
            case u'f':
            case u'F':
            case u'u':
                [[fallthrough]];
            case u'U': {
                if (exclusiveField) {
                    qDebug() << QString{"exclusive field is detected again, %%1 will be ignored."}.arg(code);
                    break;
                }
                exclusiveField = true;

                if (fields.empty()) {
                    qDebug() << QString{"fields is empty, %%1 will be ignored."}.arg(code);
                    break;
                }

                dynamicField = true;
                task.argNum = task.command.size();
                task.fieldLocation = processedArg.size();
                task.local = (code.toLower() == u'f');

                // respect the original url, replace it to exec directly
                QList<QUrl> resources;
                resources.reserve(fields.size());
                for (const auto& field : fields) {
                    resources.emplace_back(QUrl::fromUserInput(field, dir, QUrl::AssumeLocalFile));
                }

                if (code.isUpper()) {
                    task.Resources.emplace_back(std::in_place_type<QList<QUrl>>, std::move(resources));
                } else {
                    task.Resources.reserve(fields.size());
                    for (auto &&resource : resources) {
                        task.Resources.emplace_back(std::in_place_type<QUrl>, std::move(resource));
                    }
                }

                processedArg.append(percentage).append(code);
            } break;
            case u'i': {
                auto val = m_entry->value(DesktopFileEntryKey, "Icon");
                if (!val) {
                    qDebug() << R"(Application Icons can't be found. %i will be ignored.)";
                    break;
                }

                auto iconStr = toIconString(val.value());
                if (iconStr.isEmpty()) {
                    qDebug() << R"(Icons Convert to string failed. %i will be ignored.)";
                    break;
                }

                if (!processedArg.isEmpty()) {
                    task.command.append(std::move(processedArg));
                    processedArg.clear();
                }

                // spec says:
                // The Icon key of the desktop entry expanded as two arguments, first --icon and then the value of the Icon key.
                task.command << QStringLiteral("--icon") << std::move(iconStr);
            } break;
            case u'c': {
                auto val = m_entry->value(DesktopFileEntryKey, "Name");
                if (!val) {
                    qDebug() << R"(Application Name can't be found. %c will be ignored.)";
                    break;
                }

                const auto &rawValue = val.value();
                if (!rawValue.canConvert<QStringMap>()) {
                    qDebug() << "Name's underlying type mismatch:" << "QStringMap" << rawValue.metaType().name();
                    break;
                }

                auto NameStr = toLocaleString(rawValue.value<QStringMap>(), getUserLocale());
                if (NameStr.isEmpty()) {
                    qDebug() << R"(Name Convert to locale string failed. %c will be ignored.)";
                    break;
                }

                processedArg.append(NameStr);
            } break;
            case u'k': {
                processedArg.append(m_desktopSource.sourcePath());
            } break;
            case u'%': {
                processedArg.append(percentage);
            } break;
            case u'd':
            case u'D':
            case u'n':
            case u'N':
            case u'v':
                [[fallthrough]];  // Deprecated field codes should be removed from the command line and ignored.
            case u'm': {
                qWarning() << "field code" << code << "has been deprecated.";
            } break;
            default: {
                // spec says:
                // Command lines that contain a field code that is not listed in this specification are invalid and MUST NOT be processed
                qCritical() << "unknown field code:" << code << ", Invalid.";
                return {};
            }
            }
        }

        if (dynamicField || !processedArg.isEmpty()) {
            task.command.append(std::move(processedArg));
        }
    }

    if (task.Resources.isEmpty()) {
        task.Resources.emplace_back(QVariant{});  // mapReduce should run once at least
    }

    qDebug() << "Parsed Exec:" << task.LaunchBin << "Cmds:" << task.command;
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
    case EntryValueType::Raw: {
        auto valStr = val.toString();
        if (!valStr.isEmpty()) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    case EntryValueType::String: {
        auto valStr = toString(val);
        if (!valStr.isEmpty()) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    case EntryValueType::LocaleString: {
        if (!val.canConvert<QStringMap>()) {
            return ret;
        }
        auto valStr = toLocaleString(val.value<QStringMap>(), locale);
        if (!valStr.isEmpty()) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    case EntryValueType::Boolean: {
        auto valBool = toBoolean(val, ok);
        if (ok) {
            ret = QVariant::fromValue(valBool);
        }
    } break;
    case EntryValueType::IconString: {
        auto valStr = toIconString(val);
        if (!valStr.isEmpty()) {
            ret = QVariant::fromValue(valStr);
        }
    } break;
    }

    return ret;
}

void ApplicationService::updateAfterLaunch(bool isLaunch) noexcept
{
    if (!isLaunch) {
        return;
    }

    auto timestamp = QDateTime::currentMSecsSinceEpoch();
    if (auto ptr = m_storage.lock(); ptr) {
        if (!ptr->updateApplicationValue(m_desktopSource.desktopId(),
                                         ApplicationPropertiesGroup,
                                         ::LastLaunchedTime,
                                         QVariant::fromValue(timestamp),
                                         true)) {
            qWarning() << "failed to update LastLaunchedTime:" << id();
            return;
        }

        if (!ptr->updateApplicationValue(m_desktopSource.desktopId(),
                                         ApplicationPropertiesGroup,
                                         ::LaunchedTimes,
                                         QVariant::fromValue(m_launchedTimes + 1))) {
            qWarning() << "failed to update LaunchedTimes:" << id();
            return;
        }

        m_lastLaunch = timestamp;
        m_launchedTimes += 1;
        emit lastLaunchedTimeChanged();
        emit launchedTimesChanged();
    }
}

void ApplicationService::setAutostartSource(AutostartSource &&source) noexcept
{
    m_autostartSource = std::move(source);
    emit autostartChanged();
}

