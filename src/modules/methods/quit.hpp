// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QUIT_H_
#define QUIT_H_
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace Methods {
struct Quit {
    QString date;
    QString type{ "quit" };
    QString id;
    int     code;
};

inline void toJson(QByteArray &array, const Quit &quit) {
    QJsonObject obj {
        { "type", quit.type },
        { "date", quit.date },
        { "id", quit.id },
        { "code", quit.code }
    };

    array = QJsonDocument(obj).toJson();
}

inline void fromJson(const QByteArray &array, Quit &quit) {
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson quit failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("id") || !obj.contains("date") || !obj.contains("code")) {
        qWarning() << "id date code not exist in quit array";
        return;
    }

    quit.id = obj.value("id").toString();
    quit.date = obj.value("date").toString();
    quit.code = obj.value("code").toInt();
}

}  // namespace Methods

#endif  // QUIT_H_
