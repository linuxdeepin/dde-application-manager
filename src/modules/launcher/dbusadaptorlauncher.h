// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSADAPTORLAUNCHER_H
#define DBUSADAPTORLAUNCHER_H

#include "launcher.h"
#include "launcheriteminfolist.h"

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

/*
 * Adaptor class for interface org.deepin.dde.daemon.Launcher1
 */
class DBusAdaptorLauncher: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.dde.daemon.Launcher1")
    Q_CLASSINFO("D-Bus Introspection", ""
                                       "  <interface name=\"org.deepin.dde.daemon.Launcher1\">\n"
                                       "    <method name=\"GetAllItemInfos\">\n"
                                       "      <arg direction=\"out\" type=\"a(ssssxxas)\" name=\"itemInfoList\"/>\n"
                                       "      <annotation value=\"LauncherItemInfoList\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"GetAllNewInstalledApps\">\n"
                                       "      <arg direction=\"out\" type=\"as\" name=\"apps\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"GetDisableScaling\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"b\" name=\"value\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"GetItemInfo\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"(ssssxxas)\" name=\"itemInfo\"/>\n"
                                       "      <annotation value=\"LauncherItemInfo\" name=\"org.qtproject.QtDBus.QtTypeName.Out0\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"GetUseProxy\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"b\" name=\"value\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"IsItemOnDesktop\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"b\" name=\"result\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"RequestRemoveFromDesktop\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"RequestSendToDesktop\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"out\" type=\"b\" name=\"ok\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"RequestUninstall\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"desktop\"/>\n"
                                       "      <arg direction=\"in\" type=\"b\" name=\"unused\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"SetDisableScaling\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"in\" type=\"b\" name=\"value\"/>\n"
                                       "    </method>\n"
                                       "    <method name=\"SetUseProxy\">\n"
                                       "      <arg direction=\"in\" type=\"s\" name=\"id\"/>\n"
                                       "      <arg direction=\"in\" type=\"b\" name=\"value\"/>\n"
                                       "    </method>\n"
                                       "    <signal name=\"ItemChanged\">\n"
                                       "      <arg type=\"s\" name=\"status\"/>\n"
                                       "      <arg type=\"(ssssxxas)\" name=\"itemInfo\"/>\n"
                                       "      <annotation value=\"LauncherItemInfo\" name=\"org.qtproject.QtDBus.QtTypeName.Out1\"/>\n"
                                       "      <arg type=\"x\" name=\"categoryID\"/>\n"
                                       "    </signal>\n"
                                       "    <signal name=\"NewAppLaunched\">\n"
                                       "      <arg type=\"s\" name=\"appID\"/>\n"
                                       "    </signal>\n"
                                       "    <signal name=\"UninstallSuccess\">\n"
                                       "      <arg type=\"s\" name=\"appID\"/>\n"
                                       "    </signal>\n"
                                       "    <signal name=\"UninstallFailed\">\n"
                                       "      <arg type=\"s\" name=\"appId\"/>\n"
                                       "      <arg type=\"s\" name=\"errMsg\"/>\n"
                                       "    </signal>\n"
                                       "    <property access=\"readwrite\" type=\"b\" name=\"Fullscreen\"/>\n"
                                       "    <property access=\"readwrite\" type=\"i\" name=\"DisplayMode\"/>\n"
                                       "  </interface>\n"
                                       "")
public:
    explicit DBusAdaptorLauncher(QObject *parent);
    virtual ~DBusAdaptorLauncher();

    Launcher *parent() const;

public: // PROPERTIES
    Q_PROPERTY(int DisplayMode READ displayMode WRITE setDisplayMode NOTIFY DisplayModeChanged)
    Q_PROPERTY(bool Fullscreen READ fullscreen WRITE setFullscreen NOTIFY FullscreenChanged)

    int displayMode() const;
    void setDisplayMode(int value);

    bool fullscreen() const;
    void setFullscreen(bool value);

public Q_SLOTS: // METHODS
    LauncherItemInfoList GetAllItemInfos();
    QStringList GetAllNewInstalledApps();
    bool GetDisableScaling(const QString &id);
    LauncherItemInfo GetItemInfo(const QString &id);
    bool GetUseProxy(const QString &id);
    bool IsItemOnDesktop(const QString &id);
    bool RequestRemoveFromDesktop(const QString &id);
    bool RequestSendToDesktop(const QString &id);
    void RequestUninstall(const QString &desktop, bool unused);
    void SetDisableScaling(const QString &id, bool value);
    void SetUseProxy(const QString &id, bool value);

Q_SIGNALS: // SIGNALS
    void ItemChanged(const QString &status, const LauncherItemInfo &itemInfo, qlonglong categoryID);
    void NewAppLaunched(const QString &appID);
    void UninstallFailed(const QString &appId, const QString &errMsg);
    void UninstallSuccess(const QString &appID);

    void DisplayModeChanged(int mode);
    // 该接口与1050 dbus服务接口签名保持一致
    void FullscreenChanged();
};

#endif
