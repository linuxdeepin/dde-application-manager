// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>

#include "impl/application_manager.h"
#include "impl/application.h"
#include "manageradaptor.h"
#include "application1adaptor.h"
#include "applicationhelper.h"
#include "mime1adaptor.h"
#include "settings.h"
#include "dsysinfo.h"
#include "../modules/apps/appmanager.h"
#include "../modules/launcher/launchermanager.h"
#include "../modules/dock/dockmanager.h"
#include "../modules/startmanager/startmanager.h"
#include "../modules/mimeapp/mime_app.h"

#include <QDir>
#include <DLog>
#include <pwd.h>

DCORE_USE_NAMESPACE

#define ApplicationManagerServiceName "org.deepin.dde.Application1.Manager"
#define ApplicationManagerServicePath "/org/deepin/dde/Application1/Manager"
#define ApplicationManagerInterface   "org.deepin.dde.Application1.Manager"

QFileInfoList scan(const QString &path)
{
    QDir dir(path);
    dir.setFilter(QDir::Files);
    dir.setNameFilters({ "*.desktop" });
    return dir.entryInfoList();
}

// 扫描系统目录
// 扫描用户目录
QList<QSharedPointer<Application>> scanFiles()
{
    QList<QSharedPointer<Application>> applications;
    auto apps = scan("/usr/share/applications/");
    for (const QFileInfo &info : apps) {
        applications << QSharedPointer<Application>(new Application(
            "freedesktop",
            Application::Type::System,
            QSharedPointer<modules::ApplicationHelper::Helper>(new modules::ApplicationHelper::Helper(info.filePath()))
        ));
    }

    struct passwd *user = getpwent();
    while (user) {
        auto userApps = scan(QString("%1/.local/share/applications/").arg(user->pw_dir));
        for (const QFileInfo &info : userApps) {
            applications << QSharedPointer<Application>(new Application(
                "freedesktop",
                Application::Type::System,
                QSharedPointer<modules::ApplicationHelper::Helper>(new modules::ApplicationHelper::Helper(info.filePath()))
            ));
        }
        user = getpwent();
    }
    endpwent();
    auto linglong = scan("/persistent/linglong/entries/share/applications/");
    for (const QFileInfo &info : linglong) {
        applications << QSharedPointer<Application>(new Application(
            "linglong",
            Application::Type::System,
            QSharedPointer<modules::ApplicationHelper::Helper>(new modules::ApplicationHelper::Helper(info.filePath()))
        ));
    }

    return applications;
}

void init()
{
    // 从DConfig中读取当前的显示模式，如果为空，则认为是第一次进入（新安装的系统），否则，就认为系统之前已经进入设置过，直接返回即可
    QSharedPointer<DConfig> config(Settings::ConfigPtr("com.deepin.dde.dock"));
    if (config.isNull() || !config->value("Display_Mode").toString().isEmpty())
        return;

    // 然后判断当前系统是否为社区版，社区版默认任务栏模式为时尚模式，其他版本默认为高效模式
    QString displayMode = DSysInfo::isCommunityEdition() ? QString("fashion") : QString("efficient");
    config->setValue("Display_Mode", displayMode);
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-application-manager");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    QTranslator *translator = new QTranslator();
    translator->load(QString("/usr/share/dde-application-manager/translations/dde-application-manager_%1.qm").arg(QLocale::system().name()));
    QCoreApplication::installTranslator(translator);

    // 初始化
    init();

    new AppManager(ApplicationManager::instance());
    new LauncherManager(ApplicationManager::instance());
    new DockManager(ApplicationManager::instance());
    new ManagerAdaptor(ApplicationManager::instance());

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService("org.deepin.dde.Application1")) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    if (!connection.registerService(ApplicationManagerServiceName)) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    if (!connection.registerObject(ApplicationManagerServicePath, ApplicationManagerInterface, ApplicationManager::instance())) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    QList<QSharedPointer<Application>> apps{ scanFiles() };
    QList<QSharedPointer<Application1Adaptor>> appAdapters;
    for (const QSharedPointer<Application> app : apps) {
        QSharedPointer<Application1Adaptor> adapter = QSharedPointer<Application1Adaptor>(new Application1Adaptor(app.get()));
        appAdapters << adapter;
        if (!connection.registerObject(app->path().path(), "org.deepin.dde.Application1", app.get())) {
            qWarning() << "error: " << connection.lastError().message();
            continue;
        }
    }

    ApplicationManager::instance()->addApplication(apps);

    ApplicationManager::instance()->launchAutostartApps();

    MimeApp* mimeApp = new MimeApp;

    new Mime1Adaptor(mimeApp);
    if (!connection.registerService("org.deepin.dde.Mime1")) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    if (!connection.registerObject("/org/deepin/dde/Mime1", "org.deepin.dde.Mime1", mimeApp)) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    return app.exec();
}
