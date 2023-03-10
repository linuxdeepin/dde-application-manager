/*
 * This file was generated by qdbusxml2cpp-fix version 0.8
 * Command line was: qdbusxml2cpp-fix -c Launcher -p generated/org_deepin_dde_launcher1 ../xml/org.deepin.dde.Launcher1.xml
 *
 * qdbusxml2cpp-fix is Copyright (C) 2016 Deepin Technology Co., Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef ORG_DEEPIN_DDE_LAUNCHER1_H
#define ORG_DEEPIN_DDE_LAUNCHER1_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <DBusExtendedAbstractInterface>
#include <QtDBus/QtDBus>


/*
 * Proxy class for interface org.deepin.dde.Launcher1
 */
class __LauncherPrivate;
class LauncherFront : public DBusExtendedAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return "org.deepin.dde.Launcher1"; }

public:
    explicit LauncherFront(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = 0);

    ~LauncherFront();

    Q_PROPERTY(bool Visible READ visible NOTIFY VisibleChanged)
    bool visible();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<> Exit()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Exit"), argumentList);
    }

    inline void ExitQueued()
    {
        QList<QVariant> argumentList;

        CallQueued(QStringLiteral("Exit"), argumentList);
    }


    inline QDBusPendingReply<> Hide()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Hide"), argumentList);
    }

    inline void HideQueued()
    {
        QList<QVariant> argumentList;

        CallQueued(QStringLiteral("Hide"), argumentList);
    }


    inline QDBusPendingReply<bool> IsVisible()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("IsVisible"), argumentList);
    }



    inline QDBusPendingReply<> Show()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Show"), argumentList);
    }

    inline void ShowQueued()
    {
        QList<QVariant> argumentList;

        CallQueued(QStringLiteral("Show"), argumentList);
    }


    inline QDBusPendingReply<> ShowByMode(qlonglong in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);
        return asyncCallWithArgumentList(QStringLiteral("ShowByMode"), argumentList);
    }

    inline void ShowByModeQueued(qlonglong in0)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(in0);

        CallQueued(QStringLiteral("ShowByMode"), argumentList);
    }


    inline QDBusPendingReply<> Toggle()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("Toggle"), argumentList);
    }

    inline void ToggleQueued()
    {
        QList<QVariant> argumentList;

        CallQueued(QStringLiteral("Toggle"), argumentList);
    }


    inline QDBusPendingReply<> UninstallApp(const QString &appKey)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(appKey);
        return asyncCallWithArgumentList(QStringLiteral("UninstallApp"), argumentList);
    }

    inline void UninstallAppQueued(const QString &appKey)
    {
        QList<QVariant> argumentList;
        argumentList << QVariant::fromValue(appKey);

        CallQueued(QStringLiteral("UninstallApp"), argumentList);
    }



Q_SIGNALS: // SIGNALS
    void Closed();
    void Shown();
    void VisibleChanged(bool visible);
    // begin property changed signals
    //void VisibleChanged(bool  value) const;

public Q_SLOTS:
    void CallQueued(const QString &callName, const QList<QVariant> &args);

private Q_SLOTS:
    void onPendingCallFinished(QDBusPendingCallWatcher *w);
    void onPropertyChanged(const QString &propName, const QVariant &value);

private:
    __LauncherPrivate *d_ptr;
};

namespace org {
  namespace deepin {
    namespace dde {
      typedef ::LauncherFront LauncherFront;
    }
  }
}
#endif
