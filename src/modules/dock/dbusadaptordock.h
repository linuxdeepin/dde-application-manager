/*
 * Copyright (C) 2022 ~ 2023 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DBUSADAPTORDOCK_H
#define DBUSADAPTORDOCK_H

#include "dock.h"

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
#include <QRect>

/*
 * Adaptor class for interface org.deepin.dde.daemon.Dock1
 */
class DBusAdaptorDock: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.daemon.Dock1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.deepin.dde.daemon.Dock1\">\n"
"    <method name=\"CloseWindow\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"win\"/>\n"
"    </method>\n"
"    <method name=\"GetEntryIDs\">\n"
"      <arg direction=\"out\" type=\"as\" name=\"list\"/>\n"
"    </method>\n"
"    <method name=\"IsDocked\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"desktopFile\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"value\"/>\n"
"    </method>\n"
"    <method name=\"IsOnDock\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"desktopFile\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"value\"/>\n"
"    </method>\n"
"    <method name=\"MoveEntry\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"newIndex\"/>\n"
"    </method>\n"
"    <method name=\"QueryWindowIdentifyMethod\">\n"
"      <arg direction=\"in\" type=\"u\" name=\"win\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"identifyMethod\"/>\n"
"    </method>\n"
"    <method name=\"RequestDock\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"desktopFile\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"index\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
"    </method>\n"
"    <method name=\"RequestUndock\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"desktopFile\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
"    </method>\n"
"    <method name=\"SetFrontendWindowRect\">\n"
"      <arg direction=\"in\" type=\"i\" name=\"x\"/>\n"
"      <arg direction=\"in\" type=\"i\" name=\"y\"/>\n"
"      <arg direction=\"in\" type=\"u\" name=\"width\"/>\n"
"      <arg direction=\"in\" type=\"u\" name=\"height\"/>\n"
"    </method>\n"
"    <signal name=\"ServiceRestarted\"/>\n"
"    <signal name=\"EntryAdded\">\n"
"      <arg type=\"o\" name=\"path\"/>\n"
"      <arg type=\"i\" name=\"index\"/>\n"
"    </signal>\n"
"    <signal name=\"EntryRemoved\">\n"
"      <arg type=\"s\" name=\"entryId\"/>\n"
"    </signal>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"ShowTimeout\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"HideTimeout\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"WindowSizeEfficient\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"WindowSizeFashion\"/>\n"
"    <property access=\"read\" type=\"iiii\" name=\"FrontendWindowRect\"/>\n"
"    <property access=\"read\" type=\"d\" name=\"Opacity\"/>\n"
"    <property access=\"read\" type=\"ao\" name=\"Entries\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"HideMode\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"DisplayMode\"/>\n"
"    <property access=\"read\" type=\"i\" name=\"HideState\"/>\n"
"    <property access=\"readwrite\" type=\"i\" name=\"Position\"/>\n"
"    <property access=\"readwrite\" type=\"u\" name=\"IconSize\"/>\n"
"    <property access=\"read\" type=\"as\" name=\"DockedApps\"/>\n"
"  </interface>\n"
        "")
public:
    DBusAdaptorDock(QObject *parent);
    virtual ~DBusAdaptorDock();

public: // PROPERTIES
    Q_PROPERTY(int DisplayMode READ displayMode WRITE setDisplayMode NOTIFY DisplayModeChanged)
    int displayMode() const;
    void setDisplayMode(int value);

    Q_PROPERTY(QStringList DockedApps READ dockedApps NOTIFY DockedAppsChanged)
    QStringList dockedApps() const;

    Q_PROPERTY(QList<QDBusObjectPath> Entries READ entries NOTIFY EntriesChanged)
    QList<QDBusObjectPath> entries() const;

    Q_PROPERTY(int HideMode READ hideMode WRITE setHideMode NOTIFY HideModeChanged)
    int hideMode() const;
    void setHideMode(int value);

    Q_PROPERTY(int HideState READ hideState NOTIFY HideStateChanged)
    int hideState() const;

    Q_PROPERTY(uint HideTimeout READ hideTimeout WRITE setHideTimeout NOTIFY HideTimeoutChanged)
    uint hideTimeout() const;
    void setHideTimeout(uint value);

    Q_PROPERTY(uint WindowSizeEfficient READ windowSizeEfficient WRITE setWindowSizeEfficient NOTIFY WindowSizeEfficientChanged)
    uint windowSizeEfficient() const;
    void setWindowSizeEfficient(uint value);

    Q_PROPERTY(uint WindowSizeFashion READ windowSizeFashion WRITE setWindowSizeFashion NOTIFY WindowSizeFashionChanged)
    uint windowSizeFashion() const;
    void setWindowSizeFashion(uint value);

    Q_PROPERTY(QRect FrontendWindowRect READ frontendWindowRect NOTIFY FrontendWindowRectChanged)
    QRect frontendWindowRect() const;

    Q_PROPERTY(double Opacity READ opacity  NOTIFY OpacityChanged)
    double opacity() const;

    Q_PROPERTY(uint IconSize READ iconSize WRITE setIconSize NOTIFY IconSizeChanged)
    uint iconSize() const;
    void setIconSize(uint value);

    Q_PROPERTY(int Position READ position WRITE setPosition NOTIFY PositionChanged)
    int position() const;
    void setPosition(int value);

    Q_PROPERTY(uint ShowTimeout READ showTimeout WRITE setShowTimeout NOTIFY ShowTimeoutChanged)
    uint showTimeout() const;
    void setShowTimeout(uint value);

    Dock *parent() const;

public Q_SLOTS: // METHODS
    void CloseWindow(uint win);
    QStringList GetEntryIDs();
    bool IsDocked(const QString &desktopFile);
    bool IsOnDock(const QString &desktopFile);
    void MoveEntry(int index, int newIndex);
    QString QueryWindowIdentifyMethod(uint win);
    bool RequestDock(const QString &desktopFile, int index);
    bool RequestUndock(const QString &desktopFile);
    void SetFrontendWindowRect(int x, int y, uint width, uint height);

Q_SIGNALS: // SIGNALS
    void ServiceRestarted();
    void EntryAdded(const QDBusObjectPath &path, int index);
    void EntryRemoved(const QString &entryId);

    void DisplayModeChanged();
    void DockedAppsChanged();
    void OpacityChanged();
    void EntriesChanged();
    void HideModeChanged();
    void WindowSizeEfficientChanged();
    void WindowSizeFashionChanged();
    void HideStateChanged();
    void FrontendWindowRectChanged();
    void HideTimeoutChanged();
    void IconSizeChanged();
    void PositionChanged();
    void ShowTimeoutChanged();
};

#endif
