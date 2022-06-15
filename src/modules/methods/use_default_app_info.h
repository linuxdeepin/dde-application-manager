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
    j = QJsonObject{ { "appId", QJsonArray::fromVariantList(userAppInfo.appId) }, { "appType", userAppInfo.appType.c_str() }, { "supportedType", QJsonArray::fromVariantList(userAppInfo.supportedType) }};
}

inline void fromJson(const QJsonObject& j, DefaultUserAppInfo& userAppInfo)
{

    if (j.contains("appId")) {
        userAppInfo.appId = j.value("appId").toArray().toVariantList();
    }

    if (j.contains("appType")) {
        userAppInfo.appType = j.value("appType").toString().toStdString();
    }

    if (j.contains("supportedType")) {
        userAppInfo.supportedType = j.value("supportedType").toArray().toVariantList();
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
        {"appInfos", appInfoArray}
    };
}

inline void fromJson(const QJsonObject& j, DefaultUserAppInfos& userAppInfos)
{
    QJsonObject tmpObj = j;

    if (j.contains("appInfos")) {
        DefaultUserAppInfo userAppInfo;
        for (auto appInfo : tmpObj.take("appInfos").toArray()) {
            fromJson(appInfo.toObject(), userAppInfo);
            userAppInfos.appInfos.push_back(userAppInfo);
        }
    }
}
}  // namespace Methods

#endif // USE_DEFAULT_APP_INFO_H
