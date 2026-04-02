// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "propertiesForwarder.h"
#include "global.h"
#include <QDBusAbstractAdaptor>
#include <QDBusMessage>
#include <QHash>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QMutex>
#include <QMutexLocker>

namespace {
struct PropertyCacheEntry
{
    QMetaMethod signal;
    QByteArray propertyName;
};

const QHash<int, PropertyCacheEntry> &propertyCacheFor(const QMetaObject *metaObject)
{
    static QMutex mutex;
    static QHash<const QMetaObject *, QHash<int, PropertyCacheEntry>> cache;

    {
        QMutexLocker locker(&mutex);
        auto it = cache.constFind(metaObject);
        if (it != cache.cend()) {
            return *it;
        }
    }

    QHash<int, PropertyCacheEntry> propertyCache;
    propertyCache.reserve(metaObject->propertyCount() - metaObject->propertyOffset());
    for (auto i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        const auto prop = metaObject->property(i);
        if (!prop.hasNotifySignal()) {
            continue;
        }

        const auto signal = prop.notifySignal();
        propertyCache.insert(signal.methodIndex(), PropertyCacheEntry{signal, prop.name()});
    }

    QMutexLocker locker(&mutex);
    return cache.insert(metaObject, std::move(propertyCache)).value();
}
}  // namespace

PropertiesForwarder::PropertiesForwarder(QString path, QString interfaceName, QObject *parent)
    : QObject(parent)
    , m_path(std::move(path))
    , m_interfaceName(std::move(interfaceName))
{
    const auto *mo = parent->metaObject();

    if (mo == nullptr) {
        qCritical() << "relay propertiesChanged failed.";
        return;
    }

    const auto slot = metaObject()->method(metaObject()->indexOfSlot("PropertyChanged()"));
    const auto &propertyCache = propertyCacheFor(mo);
    for (auto it = propertyCache.cbegin(); it != propertyCache.cend(); ++it) {
        QObject::connect(parent, it->signal, this, slot);
    }
}

void PropertiesForwarder::PropertyChanged()
{
    auto *sender = QObject::sender();
    auto sigIndex = QObject::senderSignalIndex();

    const auto *mo = sender->metaObject();
    if (mo == nullptr) {
        qCritical() << "PropertiesForwarder::PropertyChanged [relay propertiesChanged failed.]";
        return;
    }

    const auto &propertyCache = propertyCacheFor(mo);
    const auto propIt = propertyCache.constFind(sigIndex);
    if (propIt == propertyCache.cend()) {
        qDebug() << "can't find corresponding property for signal index:" << sigIndex;
        return;
    }

    const auto &propName = propIt->propertyName;
    const auto propIndex = mo->indexOfProperty(propName.constData());
    auto prop = mo->property(propIndex);
    auto value = prop.read(sender);

    auto msg = QDBusMessage::createSignal(m_path, fromStaticRaw(SystemdPropInterfaceName), "PropertiesChanged");

    msg << m_interfaceName;
    msg << QVariantMap{{QString{propName}, value}};
    msg << QStringList{};

    ApplicationManager1DBus::instance().globalServerBus().send(msg);
}
