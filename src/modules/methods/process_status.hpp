// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef PROCESS_STATUS_H_
#define PROCESS_STATUS_H_

#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>

namespace Methods {
struct ProcessStatus {
    QString data;
    QString type;
    QString id;
    int     code;
};

inline void toJson(QByteArray& array, const ProcessStatus& processStatus)
{
    QJsonObject obj {
        { "type", processStatus.type },
        { "data", processStatus.data },
        { "id",   processStatus.id },
        { "code", processStatus.code }
    };

    array = QJsonDocument(obj).toJson();
}
inline void fromJson(const QByteArray& array, ProcessStatus& quit)
{
    QJsonDocument doc = QJsonDocument::fromJson(array);
    if (!doc.isObject()) {
        qWarning() << "fromJson quit failed";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("id") || !obj.contains("data") || !obj.contains("code")) {
        qWarning() << "id data code not exist in quit array";
        return;
    }

    quit.id = obj.value("id").toString();
    quit.data = obj.value("data").toString();
    quit.code = obj.value("code").toInt();
    quit.type = obj.value("type").toString();
}
}  // namespace Methods

#endif  // PROCESS_STATUS_H_
