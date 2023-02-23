// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSADAPTORENTRY_H
#define DBUSADAPTORENTRY_H

#include "entry.h"
#include "windowinfomap.h"

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

#include <DBusExtendedAbstractInterface>

/*
 * Adaptor class for interface org.deepin.dde.daemon.Dock1.Entry
 */
class DBusAdaptorEntry: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.daemon.Dock1.Entry")
    Q_CLASSINFO("D-Bus Introspection", ""
                                       "  <interface name=\"org.deepin.dde.daemon.Dock1.Entry\">\n"
                                       "    <method name=\"Activate\">\n"
                                       "      <arg direction=\"in\" type=\"u\" name=\"timestamp\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"Check\"/>\n"
                                       "    <method name=\"ForceQuit\"/>\n"
                                       "    <method name=\"ActiveWindow\">\n"
                                       "      <arg direction=\"in\" type=\"u\" name=\"winId\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"GetAllowedCloseWindows\">\n"
                                       "      <arg direction=\"out\" type=\"au\" name=\"windows\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"HandleDragDrop\">\n"
                                       "      <arg direction=\"in\" type=\"u\" name=\"timestamp\"/>\n"
                                       "      <arg direction=\"in\" type=\"as\" name=\"files\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"HandleMenuItem\">\n"
                                       "      <arg direction=\"in\" type=\"u\" name=\"timestamp\"/>\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"NewInstance\">\n"
                                       "      <arg direction=\"in\" type=\"u\" name=\"timestamp\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"PresentWindows\"/>\n"
                                       "    <method name=\"RequestDock\"/>\n"
                                       "    <method name=\"RequestUndock\"/>\n"
                                       "    <property access=\"read\" type=\"s\" name=\"Name\"/>\n"
                                       "    <property access=\"read\" type=\"s\" name=\"Icon\"/>\n"
                                       "    <property access=\"read\" type=\"s\" name=\"Id\"/>\n"
                                       "    <property access=\"read\" type=\"b\" name=\"IsActive\"/>\n"
                                       "    <property access=\"read\" type=\"u\" name=\"CurrentWindow\"/>\n"
                                       "    <property access=\"read\" type=\"b\" name=\"IsDocked\"/>\n"
                                       "    <property access=\"read\" type=\"s\" name=\"Menu\"/>\n"
                                       "    <property access=\"read\" type=\"s\" name=\"DesktopFile\"/>\n"
                                       "    <property access=\"read\" type=\"a{u(sb)}\" name=\"WindowInfos\"/>\n"
                                       "    <property access=\"read\" type=\"i\" name=\"Mode\"/>\n"
                                       "    <annotation value=\"WindowInfoMap\" name=\"org.qtproject.QtDBus.QtTypeName\"/>\n"
                                       "  </interface>\n"
                                       "")

public:
    explicit DBusAdaptorEntry(QObject *parent);
    virtual ~DBusAdaptorEntry();

public: // PROPERTIES
    Q_PROPERTY(uint CurrentWindow READ currentWindow NOTIFY CurrentWindowChanged)
    uint currentWindow() const;

    Q_PROPERTY(QString DesktopFile READ desktopFile NOTIFY DesktopFileChanged)
    QString desktopFile() const;

    Q_PROPERTY(QString Icon READ icon NOTIFY IconChanged)
    QString icon() const;

    Q_PROPERTY(QString Id READ id)
    QString id() const;

    Q_PROPERTY(bool IsActive READ isActive NOTIFY IsActiveChanged)
    bool isActive() const;

    Q_PROPERTY(bool IsDocked READ isDocked NOTIFY IsDockedChanged)
    bool isDocked() const;

    Q_PROPERTY(QString Menu READ menu NOTIFY MenuChanged)
    QString menu() const;

    Q_PROPERTY(QString Name READ name NOTIFY NameChanged)
    QString name() const;

    Q_PROPERTY(WindowInfoMap WindowInfos READ windowInfos NOTIFY WindowInfosChanged)
    WindowInfoMap windowInfos();

    Q_PROPERTY(int Mode READ mode NOTIFY ModeChanged)
    int mode() const;

    Entry *parent() const;

public Q_SLOTS: // METHODS
    void Activate(uint timestamp);
    void Check();
    void ForceQuit();
    void ActiveWindow(quint32 winId);
    QList<QVariant> GetAllowedCloseWindows();
    void HandleDragDrop(uint timestamp, const QStringList &files);
    void HandleMenuItem(uint timestamp, const QString &id);
    void NewInstance(uint timestamp);
    void PresentWindows();
    void RequestDock();
    void RequestUndock();

Q_SIGNALS: // SIGNALS
    void IsActiveChanged(bool value) const;
    void IsDockedChanged(bool value) const;
    void MenuChanged(const QString &value) const;
    void IconChanged(const QString &value) const;
    void NameChanged(const QString &value) const;
    void DesktopFileChanged(const QString &value) const;
    void CurrentWindowChanged(uint32_t value) const;
    void WindowInfosChanged(WindowInfoMap value) const;
    void ModeChanged(int value) const;
};

#endif
