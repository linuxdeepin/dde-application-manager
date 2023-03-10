// SPDX-FileCopyrightText: 2015 Jolla Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DBUSEXTENDEDABSTRACTINTERFACE_H
#define DBUSEXTENDEDABSTRACTINTERFACE_H

#include <DBusExtended>

#include <QDBusAbstractInterface>
#include <QDBusError>

class QDBusPendingCallWatcher;
class DBusExtendedPendingCallWatcher;

class QT_DBUS_EXTENDED_EXPORT DBusExtendedAbstractInterface: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    virtual ~DBusExtendedAbstractInterface();

    Q_PROPERTY(bool sync READ sync WRITE setSync)
    inline bool sync() const { return m_sync; }
    void setSync(bool sync);
    void setSync(bool sync, bool autoStart);

    Q_PROPERTY(bool useCache READ useCache WRITE setUseCache)
    inline bool useCache() const { return m_useCache; }
    inline void setUseCache(bool useCache) { m_useCache = useCache; }

    void getAllProperties();
    inline QDBusError lastExtendedError() const { return m_lastExtendedError; }

public Q_SLOTS:
    void startServiceProcess();

protected:
    DBusExtendedAbstractInterface(const QString &service,
                                  const QString &path,
                                  const char *interface,
                                  const QDBusConnection &connection,
                                  QObject *parent);

    void connectNotify(const QMetaMethod &signal);
    void disconnectNotify(const QMetaMethod &signal);
    QVariant internalPropGet(const char *propname, void *propertyPtr);
    void internalPropSet(const char *propname, const QVariant &value, void *propertyPtr);

Q_SIGNALS:
    void serviceValidChanged(const bool valid) const;
    void serviceStartFinished(const quint32 ret) const;
    void propertyChanged(const QString &propertyName, const QVariant &value);
    void propertyInvalidated(const QString &propertyName);
    void asyncPropertyFinished(const QString &propertyName);
    void asyncSetPropertyFinished(const QString &propertyName);
    void asyncGetAllPropertiesFinished();

private Q_SLOTS:
    void onPropertiesChanged(const QString& interfaceName,
                             const QVariantMap& changedProperties,
                             const QStringList& invalidatedProperties);
    void onDBusNameOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void onAsyncPropertyFinished(QDBusPendingCallWatcher *w);
    void onAsyncSetPropertyFinished(QDBusPendingCallWatcher *w);
    void onAsyncGetAllPropertiesFinished(QDBusPendingCallWatcher *watcher);
    void onStartServiceProcessFinished(QDBusPendingCallWatcher *w);

private:
    QVariant asyncProperty(const QString &propertyName);
    void asyncSetProperty(const QString &propertyName, const QVariant &value);
    static QVariant demarshall(const QString &interface, const QMetaProperty &metaProperty, const QVariant &value, QDBusError *error);

    bool m_sync;
    bool m_useCache;
    QDBusPendingCallWatcher *m_getAllPendingCallWatcher;
    QDBusError m_lastExtendedError;
    QString m_dbusOwner;
    bool m_propertiesChangedConnected;
};

#endif /* DBUSEXTENDEDABSTRACTINTERFACE_H */
