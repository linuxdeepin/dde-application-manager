// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationservice.h"
#include "APPobjectmanager1adaptor.h"
#include "applicationchecker.h"
#include "applicationmanagerstorage.h"
#include "constant.h"
#include "global.h"
#include "iniParser.h"
#include "propertiesForwarder.h"
#include "dbus/instanceadaptor.h"
#include "launchoptions.h"
#include "desktopentry.h"
#include "desktopfileparser.h"
#include <QUuid>
#include <QStringList>
#include <QList>
#include <QUrl>
#include <QRegularExpression>
#include <QProcess>
#include <QStandardPaths>
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

double getScaleFactor() noexcept
{
    auto sessionBus = QDBusConnection::sessionBus();
    QDBusMessage reply1 = sessionBus.call(QDBusMessage::createMethodCall(
        "org.deepin.dde.XSettings1", "/org/deepin/dde/XSettings1", "org.deepin.dde.XSettings1", "GetScaleFactor"));

    if (reply1.type() != QDBusMessage::ReplyMessage) {
        qWarning() << "call GetScaleFactor Failed:" << reply1.errorMessage();
        return 1.0;
    }

    QDBusReply<double> ret1(reply1);
    double scale = ret1.isValid() ? ret1.value() : 1.0;
    scale = scale > 0 ? scale : 1;
    return scale;
}

void ApplicationService::appendExtraEnvironments(QVariantMap &runtimeOptions) const noexcept
{
    QString oldEnv;
    // scale factor
    // NOTE: Use QT_SCREEN_SCALE_FACTOR in multi-monitor situations, as this feature may be added in the future
    static QStringList scaleEnvs{"DEEPIN_WINE_SCALE=%1;", "QT_SCREEN_SCALE_FACTOR=%1;"};
    auto factor = scaleFactor();
    if (auto it = runtimeOptions.find("env"); it != runtimeOptions.cend()) {
        oldEnv = it->value<QString>();
    }

    for (const auto &env : scaleEnvs) {
        oldEnv.append(env.arg(factor));
    }

    // FIX #7528 GTK app only scale the UI, not scaling of text.
    // GDK_SCALE Must be set to an integer, see https://docs.gtk.org/gtk3/x11.html
    // 1.25 ==> 1 , 1.75 ==>2
    int scale = qRound(factor);
    oldEnv.append(QString("GDK_SCALE=%1;").arg(scale));
    if (scale > 0) { // avoid division by 0
        oldEnv.append(QString("GDK_DPI_SCALE=%1;").arg(qreal(1.0 / scale)));
    }

    // GIO
    oldEnv.append(QString{"GIO_LAUNCHED_DESKTOP_FILE=%1;"}.arg(m_desktopSource.sourcePath()));

    // set for dxcb platform plugin, which should use scale factor directly instead of processing double times.
    oldEnv.append(QString{"D_DXCB_FORCE_OVERRIDE_HIDPI=1"});

    runtimeOptions.insert("env", oldEnv);
}

ApplicationService::ApplicationService(DesktopFile source,
                                       ApplicationManager1Service *parent,
                                       std::weak_ptr<ApplicationManager1Storage> storage)
    : QObject(parent)
    , m_scaleFactor(getScaleFactor())
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

    value = storagePtr->readApplicationValue(appId, ApplicationPropertiesGroup, ScaleFactor);
    if (!value.isNull()) {
        bool ok{false};
        auto tmp = value.toDouble(&ok);
        if (ok) {
            m_scaleFactor = tmp;
            m_customScale = true;
        }
    }

    if (!QDBusConnection::sessionBus().connect("org.deepin.dde.XSettings1",
                                               "/org/deepin/dde/XSettings1",
                                               "org.deepin.dde.XSettings1",
                                               "SetScaleFactorDone",
                                               this,
                                               SLOT(onGlobalScaleFactorChanged()))) {
        qWarning() << "connect to org.deepin.dde.XSettings1 failed, scaleFactor is invalid.";
    }
}

void ApplicationService::onGlobalScaleFactorChanged() noexcept
{
    if (!m_customScale) {
        m_scaleFactor = getScaleFactor();
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

    if (!realExec.isNull()) {  // we want to replace exec of this applications.
        if (realExec.isEmpty()) {
            qWarning() << "try to replace exec but it's empty.";
            return {};
        }

        execStr = realExec;
    }

    while (execStr.isEmpty() and !action.isEmpty() and !supportedActions.isEmpty()) {  // break trick
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
            break;
        }
        break;
    }

    if (execStr.isEmpty()) {
        auto Actions = m_entry->value(DesktopFileEntryKey, "Exec");
        if (!Actions) {
            QString msg{"application can't be executed."};
            qWarning() << msg;
            if (calledFromDBus()) {
                sendErrorReply(QDBusError::Failed, msg);
            }
            return {};
        }

        execStr = Actions.value().toString();
        if (execStr.isEmpty()) {
            QString msg{"maybe entry actions's format is invalid, abort launch."};
            qWarning() << msg;
            if (calledFromDBus()) {
                sendErrorReply(QDBusError::Failed, msg);
            }
            return {};
        }
    }

    optionsMap.remove("_hooks");  // this is internal property, user shouldn't pass it to Application Manager
    if (const auto &hooks = parent()->applicationHooks(); !hooks.isEmpty()) {
        optionsMap.insert("_hooks", hooks);
    }
    optionsMap.insert("_builtIn_searchExec", parent()->systemdPathEnv());

    auto cmds = generateCommand(optionsMap);
    auto task = unescapeExec(execStr, fields);
    if (!task) {
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::InternalError, "Invalid Command.");
        }
        return {};
    }

    auto [bin, execCmds, res] = std::move(task);
    if (bin.isEmpty()) {
        qCritical() << "error command is detected, abort.";
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed);
        }
        return {};
    }

    if (terminal()) {
        // don't change this sequence
        execCmds.push_front("-C");           // means run a shellscript
        execCmds.push_front("--keep-open");  // keep terminal open, prevent exit immediately
        execCmds.push_front("deepin-terminal");
    }
    cmds.append(std::move(execCmds));

    auto &jobManager = parent()->jobManager();
    return jobManager.addJob(
        m_applicationPath.path(),
        [this, binary = std::move(bin), commands = std::move(cmds)](const QVariant &variantValue) -> QVariant {
            auto resourceFile = variantValue.toString();
            auto instanceRandomUUID = QUuid::createUuid().toString(QUuid::Id128);
            auto objectPath = m_applicationPath.path() + "/" + instanceRandomUUID;
            auto newCommands = commands;

            newCommands.push_front(QString{"--SourcePath=%1"}.arg(m_desktopSource.sourcePath()));
            auto location = newCommands.indexOf(R"(%f)");
            if (location != -1) {  // due to std::move, there only remove once
                newCommands.remove(location);
            }

            if (resourceFile.isEmpty()) {
                newCommands.push_front(QString{R"(--unitName=app-DDE-%1@%2.service)"}.arg(
                    escapeApplicationId(this->id()), instanceRandomUUID));  // launcher should use this instanceId
                QProcess process;
                qDebug() << "run with commands:" << newCommands;
                process.start(m_launcher, newCommands);
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
            newCommands.insert(location, resourceFile);

            newCommands.push_front(QString{R"(--unitName=DDE-%1@%2.service)"}.arg(this->id(), instanceRandomUUID));
            QProcess process;
            qDebug() << "run with commands:" << newCommands;
            process.start(getApplicationLauncherBinary(), newCommands);
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
    const auto &actionList = actions();

    for (const auto &action : actionList) {
        auto rawActionKey = DesktopFileActionKey % action;
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
    auto actionList = actions();
    for (const auto &action : actionList) {
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

bool ApplicationService::terminal() const noexcept
{
    auto val = findEntryValue(DesktopFileEntryKey, "Terminal", EntryValueType::String);
    if (!val.isNull()) {
        return val.toBool();
    }
    return false;
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

double ApplicationService::scaleFactor() const noexcept
{
    return m_scaleFactor;
}

void ApplicationService::setScaleFactor(double value) noexcept
{
    auto storagePtr = m_storage.lock();
    if (!storagePtr) {
        qCritical() << "broken storage.";
        sendErrorReply(QDBusError::InternalError);
        return;
    }

    auto appId = id();
    if (value == 0) {
        m_customScale = false;
        m_scaleFactor = getScaleFactor();
        if (!storagePtr->deleteApplicationValue(appId, ApplicationPropertiesGroup, ScaleFactor)) {
            qCritical() << "failed to delete app's property:" << appId;
        }
        return;
    }

    if (m_customScale) {
        if (!storagePtr->updateApplicationValue(appId, ApplicationPropertiesGroup, ScaleFactor, value)) {
            sendErrorReply(QDBusError::Failed, "update scaleFactor failed.");
            return;
        }
    } else {
        if (!storagePtr->createApplicationValue(appId, ApplicationPropertiesGroup, ScaleFactor, value)) {
            sendErrorReply(QDBusError::Failed, "set scaleFactor failed.");
        }
    }

    m_scaleFactor = value;
    emit scaleFactorChanged();
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

    // file has been removed
    if (destDesktopFile != m_autostartSource.m_filePath) {
        return false;
    }

    return autostartCheck(destDesktopFile);
}

void ApplicationService::setAutoStart(bool autostart) noexcept
{
    if (isAutoStart() == autostart) {
        return;
    }

    auto fileName = QDir{getAutoStartDirs().first()}.filePath(m_desktopSource.desktopId() + ".desktop");
    QFile autostartFile{fileName};
    if (!autostartFile.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
        qWarning() << "open file" << fileName << "failed:" << autostartFile.error();
        sendErrorReply(QDBusError::Failed);
        return;
    }

    DesktopEntry newEntry;
    if (!m_autostartSource.m_entry.data().isEmpty()) {
        newEntry = m_autostartSource.m_entry;
    } else {
        newEntry = *m_entry;
    }

    newEntry.insert(DesktopFileEntryKey, X_Deepin_GenerateSource, m_autostartSource.m_filePath);
    newEntry.insert(DesktopFileEntryKey, DesktopEntryHidden, !autostart);

    setAutostartSource({fileName, newEntry});

    auto hideAutostart = toString(newEntry.data()).toLocal8Bit();
    auto writeBytes = autostartFile.write(hideAutostart);

    if (writeBytes != hideAutostart.size() or !autostartFile.flush()) {
        qWarning() << "incomplete write:" << autostartFile.error();
        sendErrorReply(QDBusError::Failed, "set failed: filesystem error.");
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
    for (const auto &it : tmp.removed) {
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
        sendErrorReply(QDBusError::Failed, "user-specific config file doesn't exists.");
        return;
    }

    const auto &list = userInfo->appsList().rbegin();
    const auto &appId = id();
    for (const auto &add : newAdds) {
        list->addAssociation(add, appId);
    }
    for (const auto &remove : newRemoved) {
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

void ApplicationService::detachAllInstance() noexcept
{
    for (auto &instance : m_Instances.values()) {
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
    emit scaleFactorChanged();
    emit launchedTimesChanged();
}

std::optional<QStringList> ApplicationService::unescapeExecArgs(const QString &str) noexcept
{
    auto unescapedStr = unescape(str, true);
    if (unescapedStr.isEmpty()) {
        qWarning() << "unescape Exec failed.";
        return std::nullopt;
    }

    auto deleter = [](wordexp_t *word) {
        wordfree(word);
        delete word;
    };

    std::unique_ptr<wordexp_t, decltype(deleter)> words{new (std::nothrow) wordexp_t{0, nullptr, 0}, deleter};
    if (words == nullptr) {
        qCritical() << "couldn't new wordexp_t";
        return std::nullopt;
    }

    if (auto ret = wordexp(unescapedStr.toLocal8Bit(), words.get(), WRDE_SHOWERR); ret != 0) {
        if (ret != 0) {
            QString errMessage;
            switch (ret) {
            case WRDE_BADCHAR:
                errMessage = "BADCHAR";
                break;
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
            return std::nullopt;
        }
    }

    QStringList execList;
    for (std::size_t i = 0; i < words->we_wordc; ++i) {
        execList.emplace_back(words->we_wordv[i]);
    }

    return execList;
}

LaunchTask ApplicationService::unescapeExec(const QString &str, const QStringList &fields) noexcept
{
    LaunchTask task;
    auto opt = unescapeExecArgs(str);

    if (!opt.has_value()) {
        qWarning() << "unescapeExecArgs failed.";
        return {};
    }

    auto execList = std::move(opt).value();
    if (execList.isEmpty()) {
        qWarning() << "exec format is invalid.";
        return {};
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
    if (list.count() != 1) {
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
    case 'F': {
        execList.remove(location);
        auto it = execList.begin() + location;
        for (const auto &field : fields) {
            auto tmp = QUrl{field};
            if (auto scheme = tmp.scheme(); scheme.startsWith("file") or scheme.isEmpty()) {
                it = execList.insert(it, tmp.toLocalFile());
            } else {
                qWarning() << "shouldn't replace %F with an URL:" << field;
                it = execList.insert(it, field);
            }
            ++it;
        }
        task.command.append(std::move(execList));
    } break;
    case 'U': {
        execList.removeAt(location);
        auto it = execList.begin() + location;
        for (const auto &field : fields) {
            it = execList.insert(it, field);
            ++it;
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

        auto iconStr = toIconString(val.value());
        if (iconStr.isEmpty()) {
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

        const auto &rawValue = val.value();
        if (!rawValue.canConvert<QStringMap>()) {
            qDebug() << "Name's underlying type mismatch:"
                     << "QStringMap" << rawValue.metaType().name();
            task.command.append(std::move(execList));
            return task;
        }

        auto NameStr = toLocaleString(rawValue.value<QStringMap>(), getUserLocale());
        if (NameStr.isEmpty()) {
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
}
