#include "dbusalrecorderadaptor.h"

DBusAdaptorRecorder::DBusAdaptorRecorder(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
    if (QMetaType::type("UnlaunchedAppMap") == QMetaType::UnknownType)
        registerUnLaunchedAppMapMetaType();

    AlRecorder *recorder = static_cast<AlRecorder *>(QObject::parent());
    if (recorder) {
        connect(recorder, &AlRecorder::launched, this, &DBusAdaptorRecorder::Launched);
        connect(recorder, &AlRecorder::serviceRestarted, this, &DBusAdaptorRecorder::ServiceRestarted);
        connect(recorder, &AlRecorder::statusSaved, this, &DBusAdaptorRecorder::StatusSaved);
    }
}

DBusAdaptorRecorder::~DBusAdaptorRecorder()
{
}

AlRecorder *DBusAdaptorRecorder::parent() const
{
    return static_cast<AlRecorder *>(QObject::parent());
}

UnLaunchedAppMap DBusAdaptorRecorder::GetNew()
{
    return parent()->getNew();
}

void DBusAdaptorRecorder::MarkLaunched(const QString &desktopFile)
{
    parent()->markLaunched(desktopFile);
}

void DBusAdaptorRecorder::UninstallHints(const QStringList &desktopFiles)
{
    parent()->uninstallHints(desktopFiles);
}

void DBusAdaptorRecorder::WatchDirs(const QStringList &dirs)
{
    parent()->watchDirs(dirs);
}

