// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef USE_MIME_APP_INFO_H
#define USE_MIME_APP_INFO_H

#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

namespace Methods {
struct UserAppInfo {
    std::string                         deskopid;
    QVariantList                        supportedMime;
};

struct UserAppInfos {
    std::vector<UserAppInfo>  appInfos;
};

inline void toJson(QJsonObject& j, const UserAppInfo& userAppInfo)
{
    j = QJsonObject{ { "DesktopId", userAppInfo.deskopid.c_str() }, { "SupportedMime", QJsonArray::fromVariantList(userAppInfo.supportedMime) } };
}

inline void fromJson(const QJsonObject& j, UserAppInfo& userAppInfo)
{
    if (j.contains("DesktopId")) {
        userAppInfo.deskopid = j.value("DesktopId").toString().toStdString();
    }

    if (j.contains("SupportedMime")) {
        userAppInfo.supportedMime = j.value("SupportedMime").toArray().toVariantList();
    }
}
inline void toJson(QJsonArray& j, const UserAppInfos& userAppInfos)
{
    QJsonObject tmpObj;
    QJsonArray appInfoArray;

    for (auto appInfo : userAppInfos.appInfos) {
        toJson(tmpObj, appInfo);
        appInfoArray.append(tmpObj);
    }

    j = appInfoArray;
}

inline void fromJson(const QJsonArray& j, UserAppInfos& userAppInfos)
{
    QJsonArray tmpObj = j;

    for (auto iter : tmpObj) {
        UserAppInfo userAppInfo;
        fromJson(iter.toObject(), userAppInfo);
        userAppInfos.appInfos.push_back(userAppInfo);
    }
}
}  // namespace Methods

#endif // USE_MIME_APP_INFO_H
