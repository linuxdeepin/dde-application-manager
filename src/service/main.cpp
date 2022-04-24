#include <QCoreApplication>

#include "impl/application_manager.h"
#include "impl/application.h"
#include "applicationmanageradaptor.h"
#include "applicationadaptor.h"
#include "../modules/applicationhelper/helper.h"
#include "../modules/apps/appmanager.h"
#include "../modules/launcher/launchermanager.h"
#include "../modules/dock/dockmanager.h"

#include <QDir>
#include <DLog>
#include <pwd.h>

DCORE_USE_NAMESPACE

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
    auto linglong = scan("/deepin/linglong/entries/share/applications/");
    for (const QFileInfo &info : linglong) {
        applications << QSharedPointer<Application>(new Application(
            "linglong",
            Application::Type::System,
            QSharedPointer<modules::ApplicationHelper::Helper>(new modules::ApplicationHelper::Helper(info.filePath()))
        ));
    }

    return applications;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("deepin");
    app.setApplicationName("dde-application-manager");

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();

    new AppManager(ApplicationManager::Instance());
    new LauncherManager(ApplicationManager::Instance());
    new DockManager(ApplicationManager::Instance());
    new ApplicationManagerAdaptor(ApplicationManager::Instance());

    QDBusConnection::sessionBus().registerService("org.desktopspec.Application");
    QDBusConnection::sessionBus().registerService("org.desktopspec.ApplicationManager");
    QDBusConnection::sessionBus().registerObject("/org/desktopspec/ApplicationManager", "org.desktopspec.ApplicationManager", ApplicationManager::Instance());

    QList<QSharedPointer<Application>> apps{ scanFiles() };
    QList<QSharedPointer<ApplicationAdaptor>> appAdapters;
    for (const QSharedPointer<Application> app : apps) {
        QSharedPointer<ApplicationAdaptor> adapter = QSharedPointer<ApplicationAdaptor>(new ApplicationAdaptor(app.get()));
        appAdapters << adapter;
        QDBusConnection::sessionBus().registerObject(app->path().path(), "org.desktopspec.Application", app.get());
    }

    ApplicationManager::Instance()->addApplication(apps);

    return app.exec();
}
