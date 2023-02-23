// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef C664E26D_6517_412B_950F_07E20963349E
#define C664E26D_6517_412B_950F_07E20963349E

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace Methods {
struct Instance {
    QString hash;
    QString type{ "instance" };
};

inline void toJson(QByteArray &array, const Instance &instance) {
    QJsonObject obj {
        { "type", instance.type },
        { "hash", instance.hash }
    };

    array = QJsonDocument(obj).toJson();
}

inline void fromJson(const QByteArray &array, Instance &instance) {
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson instance failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("hash")) {
        qWarning() << "hash not exist in instance array";
        return;
    }

    instance.hash = obj.value("hash").toString();
}

}  // namespace Methods

#endif /* C664E26D_6517_412B_950F_07E20963349E */
