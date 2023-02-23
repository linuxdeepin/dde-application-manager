// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef USE_DEFAULT_APP_INFO_H
#define USE_DEFAULT_APP_INFO_H

#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>

namespace Methods {
struct DefaultUserAppInfo {
    QVariantList             appId;
    std::string              appType;
    QVariantList             supportedType;
};

struct DefaultUserAppInfos {
    std::vector<DefaultUserAppInfo>  appInfos;
};

inline void toJson(QJsonObject& j, const DefaultUserAppInfo& userAppInfo)
{
    j = QJsonObject{ { "AppId", QJsonArray::fromVariantList(userAppInfo.appId) }, { "AppType", userAppInfo.appType.c_str() }, { "SupportedType", QJsonArray::fromVariantList(userAppInfo.supportedType) }};
}

inline void fromJson(const QJsonObject& j, DefaultUserAppInfo& userAppInfo)
{

    if (j.contains("AppId")) {
        userAppInfo.appId = j.value("AppId").toArray().toVariantList();
    }

    if (j.contains("AppType")) {
        userAppInfo.appType = j.value("AppType").toString().toStdString();
    }

    if (j.contains("SupportedType")) {
        userAppInfo.supportedType = j.value("SupportedType").toArray().toVariantList();
    }
}

inline void toJson(QJsonObject& j, const DefaultUserAppInfos& userAppInfos)
{
    QJsonObject tmpObj;
    QJsonArray appInfoArray;

    for (auto appInfo : userAppInfos.appInfos) {
        toJson(tmpObj, appInfo);
        appInfoArray.append(tmpObj);
    }

    j = QJsonObject {
        {"DefaultApps", appInfoArray}
    };
}

inline void fromJson(const QJsonObject& j, DefaultUserAppInfos& userAppInfos)
{
    QJsonObject tmpObj = j;

    if (j.contains("DefaultApps")) {
        DefaultUserAppInfo userAppInfo;
        for (auto appInfo : tmpObj.take("DefaultApps").toArray()) {
            fromJson(appInfo.toObject(), userAppInfo);
            userAppInfos.appInfos.push_back(userAppInfo);
        }
    }
}
}  // namespace Methods

#endif // USE_DEFAULT_APP_INFO_H
