// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "propertiesForwarder.h"
#include "global.h"
#include <QMetaProperty>
#include <QDBusMessage>
#include <QDBusAbstractAdaptor>

PropertiesForwarder::PropertiesForwarder(QString path, QObject *parent)
    : QObject(parent)
    , m_path(std::move(path))
{
    const auto *mo = parent->metaObject();

    if (mo == nullptr) {
        qCritical() << "relay propertiesChanged failed.";
        return;
    }

    for (auto i = mo->propertyOffset(); i < mo->propertyCount(); ++i) {
        auto prop = mo->property(i);
        if (!prop.hasNotifySignal()) {
            continue;
        }

        auto signal = prop.notifySignal();
        auto slot = metaObject()->method(metaObject()->indexOfSlot("PropertyChanged()"));
        QObject::connect(parent, signal, this, slot);
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

    auto sig = mo->property(sigIndex);
    const auto *propName = sig.name();
    auto value = sig.read(sender);

    auto childs = sender->children();
    QString interface;
    for (const auto &adaptor : childs) {
        if (adaptor->inherits("QDBusAbstractAdaptor")) {
            const auto *adaptorMo = adaptor->metaObject();
            if (adaptorMo->indexOfProperty(propName) != -1) {
                interface = getDBusInterface(adaptor->metaObject()->metaType());
                break;
            }
        }
    };

    if (interface.isEmpty()) {
        return;
    }

    auto msg = QDBusMessage::createSignal(m_path, interface, "PropertiesChanged");
    msg << QString{ApplicationInterface};
    msg << QVariantMap{{QString{propName}, value}};
    msg << QStringList{};

    ApplicationManager1DBus::instance().globalServerBus().send(msg);
}
