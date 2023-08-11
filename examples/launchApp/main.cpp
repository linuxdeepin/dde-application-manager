// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QVariantMap>
#include <QDebug>

class Demo : public QObject
{
    Q_OBJECT
public:
    Demo()
        : ApplicationManager(u8"org.deepin.dde.ApplicationManager1",
                             u8"/org/deepin/dde/ApplicationManager1",
                             u8"org.desktopspec.ApplicationManager1")
        , JobManager(u8"org.deepin.dde.ApplicationManager1",
                     u8"/org/deepin/dde/ApplicationManager1/JobManager1",
                     u8"org.desktopspec.JobManager1")
    {
        auto con = JobManager.connection();
        if (!con.connect(JobManager.service(),
                         JobManager.path(),
                         JobManager.interface(),
                         u8"JobNew",
                         this,
                         SLOT(onJobNew(QDBusObjectPath, QDBusObjectPath)))) {
            qFatal() << "connect JobNew failed.";
        }

        if (!con.connect(JobManager.service(),
                         JobManager.path(),
                         JobManager.interface(),
                         u8"JobRemoved",
                         this,
                         SLOT(onJobRemoved(QDBusObjectPath, QString, QVariantList)))) {
            qFatal() << "connect JobNew failed.";
        }
    }

    void launchApp(const QString &appId)
    {
        auto msg =
            ApplicationManager.callWithArgumentList(QDBus::Block, "Launch", {appId, QString{""}, QStringList{}, QVariantMap{}});
        qInfo() << "reply message:" << msg;
    }

public Q_SLOTS:
    void onJobNew(QDBusObjectPath job, QDBusObjectPath source)
    {
        qInfo() << "Job New ["
                << "Job Path:" << job.path() << source.path() << "add this job].";
    }

    void onJobRemoved(QDBusObjectPath job, QString status, QVariantList result)
    {
        qInfo() << "Job Removed ["
                << "Job Path:" << job.path() << "Job Status:" << status << "result:" << result;
    }

private:
    QDBusInterface ApplicationManager;
    QDBusInterface JobManager;
};

int main(int argc, char *argv[])
{
    QCoreApplication app{argc, argv};
    Demo demo;
    demo.launchApp("google-chrome");
    return QCoreApplication::exec();
}

#include "main.moc"
