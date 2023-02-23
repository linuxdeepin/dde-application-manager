// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef REGISTER_H_
#define REGISTER_H_
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

namespace Methods {
struct Registe {
    QString date;
    QString id;
    QString type{ "registe" };
    QString hash;
    bool        state;
};

inline void toJson(QByteArray &array, const Registe &registe) {
    QJsonObject obj = {
        { "type", registe.type },
        { "id", registe.id },
        { "hash", registe.hash },
        { "state", registe.state },
        { "date", registe.date }
    };

    array = QJsonDocument(obj).toJson();
}

inline void fromJson(const QByteArray &array, Registe &registe) {
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson registe failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("id") || !obj.contains("date") || !obj.contains("hash")\
        || !obj.contains("state")) {
        qWarning() << "id date code state not exist in registe array";
        return;
    }
    
    registe.id = obj.value("id").toString();
    registe.date = obj.value("date").toString();
    registe.hash = obj.value("hash").toString();
    registe.state = obj.value("state").toBool();
}

}  // namespace Methods

#endif  // REGISTER_H_
