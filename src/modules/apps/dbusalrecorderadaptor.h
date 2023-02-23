// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSALRECORDERADAPTOR_H
#define DBUSALRECORDERADAPTOR_H

#include "alrecorder.h"
#include "unlaunchedappmap.h"

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

class DBusAdaptorRecorder: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.AlRecorder1")
    Q_CLASSINFO("D-Bus Introspection", ""
                                       "  <interface name=\"org.deepin.dde.AlRecorder1\">\n"
                                       "    <method name=\"GetNew\">\n"
                                       "      <arg direction=\"out\" type=\"a{sas}\" name=\"newApps\"/>\n"
                                       "      <annotation value=\"UnLaunchedAppMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"MarkLaunched\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"desktopFile\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"UninstallHints\">\n"
                                       "      <arg direction=\"in\" type=\"as\" name=\"desktopFiles\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"WatchDirs\">\n"
                                       "      <arg direction=\"in\" type=\"as\" name=\"dirs\"/>\n"
                                       "    </method>\n"
                                       "    <signal name=\"Launched\">\n"
                                       "      <arg type=\"s\" name=\"file\"/>\n"
                                       "    </signal>\n"
                                       "    <signal name=\"StatusSaved\">\n"
                                       "      <arg type=\"s\" name=\"root\"/>\n"
                                       "      <arg type=\"s\" name=\"file\"/>\n"
                                       "      <arg type=\"b\" name=\"ok\"/>\n"
                                       "    </signal>\n"
                                       "    <signal name=\"ServiceRestarted\"/>\n"
                                       "  </interface>\n"
                                       "")

    AlRecorder *parent() const;

public:
    explicit DBusAdaptorRecorder(QObject *parent);
    virtual ~DBusAdaptorRecorder();

public: // PROPERTIES
public Q_SLOTS: // METHODS
    UnLaunchedAppMap GetNew();
    void MarkLaunched(const QString &desktopFile);
    void UninstallHints(const QStringList &desktopFiles);
    void WatchDirs(const QStringList &dirs);
Q_SIGNALS: // SIGNALS
    void Launched(const QString &file);
    void ServiceRestarted();
    void StatusSaved(const QString &root, const QString &file, bool ok);
};

#endif
