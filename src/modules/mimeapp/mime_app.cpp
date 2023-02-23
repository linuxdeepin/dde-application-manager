// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mime_app.h"
#include "utils.h"
#include "../methods/use_mime_app_info.h"
#include "../methods/use_default_app_info.h"
#include "appinfo.h"
#include "terminalinfo.h"
#include "appinfocommon.h"

#include <qmutex.h>
#include <QSettings>
#include <QVector>
#include <QFileInfo>
#include <fstream>
#include <iostream>
#include <QJsonParseError>
#include <QDebug>

class MimeAppPrivate : public QObject
{
    MimeApp *q_ptr = nullptr;
    Q_DECLARE_PUBLIC(MimeApp)

private:
    Methods::UserAppInfos userAppInfos;
    std::string filename;
    QMutex mutex;
public:
    MimeAppPrivate(MimeApp *parent) : QObject(parent), q_ptr(parent)
    {
        std::string homeDir = getUserHomeDir();
        if (!homeDir.empty()) {
            homeDir += "/.config/deepin/dde-daemon/user_mime.json";
            filename = homeDir;
        }
    }
    void Init()
    {
        QFile file(filename.c_str());
        if (!file.exists()) {
            return;
        }

        if (!file.open(QIODevice::ReadOnly)) {
            return;
        }

        QJsonParseError *error = new QJsonParseError;
        QJsonDocument jdc = QJsonDocument::fromJson(file.readAll(), error);

        Methods::fromJson(jdc.array(), userAppInfos);

        file.close();
    }

    void Write()
    {
        QFile file(filename.c_str());
        if (!file.exists()) {
            return;
        }

        if (!file.open(QIODevice::WriteOnly)) {
            return;
        }

        QJsonArray jsonArr;
        Methods::toJson(jsonArr, userAppInfos);

        QJsonDocument jdc;
        jdc.setArray(jsonArr);
        file.write(jdc.toJson(QJsonDocument::Compact)); //Indented:表示自动添加/n回车符
        file.close();
    }

    bool Add(const std::string &desktopId, QStringList mimeTypes)
    {
        bool bAdd = false;

        std::vector<Methods::UserAppInfo>::iterator iter = userAppInfos.appInfos.begin();
        while (iter != userAppInfos.appInfos.end()) {
            if (iter->deskopid == desktopId) {
                for (auto mimeType : mimeTypes) {
                    if (std::find(iter->supportedMime.begin(), iter->supportedMime.end(), mimeType) != iter->supportedMime.end()) {
                        bAdd = true;
                        iter->supportedMime.push_back(mimeType);
                    }
                }
                return bAdd;
            }
            iter++;
        }

        bAdd = true;
        Methods::UserAppInfo appInfo;
        for (auto mimeType : mimeTypes) {

            appInfo.supportedMime.push_back(mimeType);
        }

        userAppInfos.appInfos.push_back(appInfo);

        return bAdd;
    }

    bool Delete(const std::string &desktopId)
    {
        std::vector<Methods::UserAppInfo>::iterator iter = userAppInfos.appInfos.begin();

        while (iter != userAppInfos.appInfos.end()) {
            if (iter->deskopid == desktopId) {
                iter = userAppInfos.appInfos.erase(iter);
                return true;
            } else {
                iter++;
            }
        }

        return false;
    }

    bool DeleteMimeTypes(const std::string &desktopId, QStringList mimeTypes)
    {
        bool bDelete = false;
        std::vector<Methods::UserAppInfo>::iterator iter = userAppInfos.appInfos.begin();

        while (iter != userAppInfos.appInfos.end()) {
            if (iter->deskopid == desktopId) {
                for (auto mimeType : mimeTypes) {
                    if (std::find(iter->supportedMime.begin(), iter->supportedMime.end(), mimeType) == iter->supportedMime.end()) {
                        bDelete = true;
                        iter->supportedMime.push_back(mimeType);
                    }
                }
            }
            iter++;
        }

        return bDelete;
    }

    std::vector<Methods::UserAppInfo> GetUserAppInfosByMimeType(std::string mimeType)
    {
        std::vector<Methods::UserAppInfo> retVector;

        for (auto iter : userAppInfos.appInfos) {
            if (std::find(iter.supportedMime.begin(), iter.supportedMime.end(), mimeType.c_str()) != iter.supportedMime.end()) {
                retVector.push_back(iter);
            }
        }

        return retVector;
    }
    ~MimeAppPrivate() {}

};

MimeApp::MimeApp(QObject *parent) : QObject(parent), dd_ptr(new MimeAppPrivate(this))
{
    Q_D(MimeApp);
    d->Init();
    initConfigData();
}

MimeApp::~MimeApp()
{

}

void MimeApp::deleteMimeAssociation(std::string mimeType, std::string desktopId)
{
    Q_D(MimeApp);

    KeyFile keyFile;
    keyFile.loadFile(d->filename);

    std::vector<std::string> sessions{AppinfoCommon::SectionDefaultApps, AppinfoCommon::SectionAddedAssociations};

    for (auto iter : sessions) {
        std::vector<std::string> apps = keyFile.getStrList(iter, mimeType);
        std::vector<std::string>::iterator app = apps.begin();
        while (app != apps.end()) {
            if ((*app) == desktopId) {
                app = apps.erase(app);
            } else {
                app++;
            }
        }
        if (apps.empty()) {
            //keyFile.DeleteKey(section, mimeType);
        } else {
            //keyFile.SetStringList(iter,mimeType,apps);
        }
    }

    keyFile.saveToFile(d->filename);
}

void MimeApp::initConfigData()
{
    // TODO 这个配置文件当前仍然是在dde-daemon中，但mime的服务已经迁移到此项目，后续应该把这个配置文件拿过来
    std::string filename = findFilePath("/dde-daemon/mime/data.json");

    QFile file(filename.c_str());
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QJsonParseError *error = new QJsonParseError;
    QJsonDocument jdc = QJsonDocument::fromJson(file.readAll(), error);

    Methods::DefaultUserAppInfos defaultAppInfos;
    Methods::fromJson(jdc.object(), defaultAppInfos);

    file.close();

    for (auto defaultApp : defaultAppInfos.appInfos) {
        std::string validAppId;

        for (auto type : defaultApp.supportedType) {
            // 如果之前用户有修改默认程序，在每次初始化时不应该再使用配置文件里面的默认程序
            std::string appId = AppInfoManger::getDefaultApp(type.toString().toStdString(), false);
            if (!appId.empty()) {
                continue;
            }

            if (!validAppId.empty()) {
                if (setDefaultApp(type.toString().toStdString(), validAppId)) {
                    continue;
                }
            }

            for (auto appId : defaultApp.appId) {
                if (setDefaultApp(type.toString().toStdString(), appId.toString().toStdString())) {
                    validAppId = appId.toString().toStdString();
                    break;
                } else {
                    continue;
                }
            }
        }
    }
}

std::string MimeApp::findFilePath(std::string fileName)
{
    std::string homeDir = getUserHomeDir();
    std::string path = homeDir + "/.local/share" + fileName;

    QFileInfo file(path.c_str());
    if (file.isFile()) {
        return path;
    }

    path = "/usr/local/share" + fileName;

    file.setFile(path.c_str());
    if (file.isFile()) {
        return path;
    }

    return "/usr/share" + fileName;
}

void MimeApp::AddUserApp(QStringList mimeTypes, const QString &desktopId)
{
    qInfo() << "AddUserApp mimeTypes: " << mimeTypes << ", desktopId: " << desktopId;
    Q_D(MimeApp);

    std::shared_ptr<AppInfoManger> appInfo = AppInfoManger::loadByDesktopId(desktopId.toStdString());
    if (!appInfo) {
        return;
    }

    bool bAdd = d->Add(desktopId.toStdString(), mimeTypes);

    if (bAdd) {
        d->Write();
    }

    return;
}

bool MimeApp::setDefaultApp(const std::string &mimeType, const  std::string &desktopId)
{
    qInfo() << "setDefaultApp mimeType: " << mimeType.c_str() << ", desktopId: " << desktopId.c_str();
    Q_D(MimeApp);

    KeyFile keyFile;
    keyFile.loadFile(d->filename);

    std::string curDeskTopId = keyFile.getStr(AppinfoCommon::SectionDefaultApps, mimeType);
    if (curDeskTopId == desktopId) {
        return true;
    }

    return  AppInfoManger::setDefaultApp(mimeType, desktopId);
}

void MimeApp::DeleteApp(QStringList mimeTypes, const QString &desktopId)
{
    qInfo() << "DeleteApp mimeTypes: " << mimeTypes << ", desktopId: " << desktopId;
    Q_D(MimeApp);

    if (d->DeleteMimeTypes(desktopId.toStdString(), mimeTypes)) {
        d->Write();
        return;
    }

    std::shared_ptr<AppInfoManger> appInfo = AppInfoManger::loadByDesktopId(desktopId.toStdString());
    if (!appInfo) {
        return;
    }

    std::vector<std::string>  originMimeTypes = appInfo->getStringList(AppinfoCommon::MainSection, AppinfoCommon::KeyMimeType);

    for (auto iter : mimeTypes) {
        if (std::find(originMimeTypes.begin(), originMimeTypes.end(), iter.toStdString()) == originMimeTypes.end()) {
            deleteMimeAssociation(iter.toStdString(), desktopId.toStdString());
        }
    }
}

void MimeApp::DeleteUserApp(const QString &desktopId)
{
    qInfo() << "DeleteUserApp desktopId: " << desktopId;
    Q_D(MimeApp);

    bool bDelete = d->Delete(desktopId.toStdString());

    if (bDelete) {
        d->Write();
    }
}

QString MimeApp::GetDefaultApp(const QString &mimeType)
{
    qInfo() << "GetDefaultApp mimeType: " << mimeType;
    std::shared_ptr<AppInfoManger> appInfo;

    if (mimeType.toStdString().compare(AppinfoCommon::AppMimeTerminal) == 0) {
        appInfo = TerminalInfo::getInstanceTerminal().getDefaultTerminal();
    } else {
        std::string appId = AppInfoManger::getDefaultApp(mimeType.toStdString(), false);
        appInfo = AppInfoManger::loadByDesktopId(appId);
    }

    if (!appInfo) {
        return "";
    }

    return appInfo->toJson().c_str();
}

QString MimeApp::ListApps(const QString &mimeType)
{
    qInfo() << "ListApps mimeType: " << mimeType;
    std::vector<std::shared_ptr<AppInfoManger>> appInfos;

    if (mimeType.toStdString().compare(AppinfoCommon::AppMimeTerminal) == 0) {
        appInfos = TerminalInfo::getInstanceTerminal().getTerminalInfos();
    } else {
        std::vector<std::string> appList = AppInfoManger::getAppList(mimeType.toStdString());

        for (auto iter : appList) {
            std::shared_ptr<AppInfoManger> appInfo = AppInfoManger::loadByDesktopId(iter);
            if (appInfo) {
                appInfos.push_back(appInfo);
            }
        }
    }


    std::string userAppDir = getUserDataDir() + "/applications";
    std::vector<std::shared_ptr<AppInfoManger>>::iterator iter = appInfos.begin();

    while (iter != appInfos.end()) {
        QDir dir((*iter)->getFileName().c_str());

        if (dir.path().toStdString() == userAppDir && hasBeginWith(dir.path().toStdString(), "deepin-custom-")) {
            iter = appInfos.erase(iter);
        } else {
            iter++;
        }
    }

    return AppInfoManger::toJson(appInfos).c_str();
}

QString MimeApp::ListUserApps(const QString &mimeType)
{
    qInfo() << "ListUserApps mimeType: " << mimeType;
    Q_D(MimeApp);

    std::vector<std::shared_ptr<AppInfoManger>> retAppInfos;

    std::vector<Methods::UserAppInfo> userAppInfos = d->GetUserAppInfosByMimeType(mimeType.toStdString());
    std::vector<Methods::UserAppInfo>::iterator iter = userAppInfos.begin();

    while (iter != userAppInfos.end()) {
        std::shared_ptr<AppInfoManger> appInfo = AppInfoManger::loadByDesktopId(iter->deskopid);
        if (appInfo) {
            appInfo->setCanDelete(true);
            retAppInfos.push_back(appInfo);
        }
        iter++;
    }

    return AppInfoManger::toJson(retAppInfos).c_str();
}

void MimeApp::SetDefaultApp(const QStringList &mimeTypes, const QString &desktopId)
{
    qInfo() << "SetDefaultApp mimeTypes: " << mimeTypes << ", desktopId: " << desktopId;

    bool bSuccess = false;
    for (auto mime : mimeTypes) {
        if (mime.toStdString().compare(AppinfoCommon::AppMimeTerminal) == 0) {
            bSuccess = TerminalInfo::getInstanceTerminal().setDefaultTerminal(desktopId.toStdString());
        } else {
            bSuccess = AppInfoManger::setDefaultApp(mime.toStdString(), desktopId.toStdString());
        }

        if (!bSuccess) {
            qWarning() << "SetDefaultApp faile";
            break;
        }
    }
}
