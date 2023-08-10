// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include <QDBusConnection>
#include <QCoreApplication>
#include <QDir>
#include "dbus/applicationmanager1service.h"
#include "cgroupsidentifier.h"

static void registerComplexDbusType()
{
    qDBusRegisterMetaType<QMap<QString, QDBusUnixFileDescriptor>>();
    qDBusRegisterMetaType<QMap<uint, QMap<QString, QDBusUnixFileDescriptor>>>();
    qDBusRegisterMetaType<IconMap>();
}

int main(int argc, char *argv[])
{
    QCoreApplication app{argc, argv};

    auto &bus = ApplicationManager1DBus::instance();
    bus.initGlobalServerBus(DBusType::Session);
    bus.setDestBus("");
    auto &AMBus = bus.globalServerBus();

    registerComplexDbusType();
    ApplicationManager1Service AMService{std::make_unique<CGroupsIdentifier>(), AMBus};
    QList<DesktopFile> fileList{};
    QByteArray XDGDataDirs;
    XDGDataDirs = qgetenv("XDG_DATA_DIRS");
    if (XDGDataDirs.isEmpty()) {
        XDGDataDirs.append("/usr/local/share/:/usr/share/");
        qputenv("XDG_DATA_DIRS", XDGDataDirs);
    }
    auto desktopFileDirs = QString::fromLocal8Bit(XDGDataDirs).split(':', Qt::SkipEmptyParts);

    std::for_each(desktopFileDirs.begin(), desktopFileDirs.end(), [](QString &str) {
        str = QDir::cleanPath(str) + QDir::separator() + "applications";
    });

    applyIteratively(QList<QDir>(desktopFileDirs.begin(), desktopFileDirs.end()), [&AMService](const QFileInfo &info) -> bool {
        ParseError err{ParseError::NoError};
        auto ret = DesktopFile::searchDesktopFile(info.absoluteFilePath(), err);
        if (!ret.has_value()) {
            qWarning() << "failed to search File:" << err;
            return false;
        }
        AMService.addApplication(std::move(ret).value());
        return false;  // means to apply this function to the rest of the files
    });

    return app.exec();
}
