#include <QCoreApplication>

#include "impl/application_manager.h"
#include "impl/application.h"
#include "applicationmanageradaptor.h"
#include "applicationadaptor.h"
#include "applicationhelper.h"
#include "mimeadaptor.h"
#include "../modules/apps/appmanager.h"
#include "../modules/launcher/launchermanager.h"
#include "../modules/dock/dockmanager.h"
#include "../modules/startmanager/startmanager.h"
#include "../modules/mimeapp/mime_app.h"

#include <QDir>
#include <DLog>
#include <pwd.h>

DCORE_USE_NAMESPACE

#define ApplicationManagerServiceName "org.desktopspec.ApplicationManager"
#define ApplicationManagerServicePath "/org/desktopspec/ApplicationManager"
#define ApplicationManagerInterface   "org.desktopspec.ApplicationManager"

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

    new AppManager(ApplicationManager::instance());
    new LauncherManager(ApplicationManager::instance());
    new DockManager(ApplicationManager::instance());
    new ApplicationManagerAdaptor(ApplicationManager::instance());

    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService("org.desktopspec.Application")) {
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
    QList<QSharedPointer<ApplicationAdaptor>> appAdapters;
    for (const QSharedPointer<Application> app : apps) {
        QSharedPointer<ApplicationAdaptor> adapter = QSharedPointer<ApplicationAdaptor>(new ApplicationAdaptor(app.get()));
        appAdapters << adapter;
        if (!connection.registerObject(app->path().path(), "org.desktopspec.Application", app.get())) {
            qWarning() << "error: " << connection.lastError().message();
            continue;
        }
    }

    ApplicationManager::instance()->addApplication(apps);

    ApplicationManager::instance()->launchAutostartApps();

    MimeApp* mimeApp = new MimeApp;

    new MimeAdaptor(mimeApp);
    if (!connection.registerService("org.deepin.daemon.Mime1")) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    if (!connection.registerObject("/org/deepin/daemon/Mime1", "org.deepin.daemon.Mime1", mimeApp)) {
        qWarning() << "error: " << connection.lastError().message();
        return -1;
    }

    return app.exec();
}
