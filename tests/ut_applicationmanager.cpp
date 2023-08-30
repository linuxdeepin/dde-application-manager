// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dbus/applicationmanager1service.h"
#include "dbus/applicationservice.h"
#include "cgroupsidentifier.h"
#include "constant.h"
#include <gtest/gtest.h>
#include <QDBusConnection>
#include <QSharedPointer>
#include <sys/syscall.h>
#include <QProcess>
#include <thread>
#include <chrono>
#include <QDBusUnixFileDescriptor>

namespace {
int pidfd_open(pid_t pid, uint flags)
{
    return syscall(SYS_pidfd_open, pid, flags);
}
}  // namespace

class TestApplicationManager : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        if (!QDBusConnection::sessionBus().isConnected()) {
            GTEST_SKIP() << "skip for now.";
        }
        auto &bus = ApplicationManager1DBus::instance();
        bus.initGlobalServerBus(DBusType::Session);
        bus.setDestBus();

        m_am = new ApplicationManager1Service{std::make_unique<CGroupsIdentifier>(), bus.globalServerBus()};
        auto ptr = std::make_unique<QFile>(QString{"/usr/share/applications/test-Application.desktop"});
        DesktopFile file{std::move(ptr), "test-Application", 0};
        QSharedPointer<ApplicationService> app = QSharedPointer<ApplicationService>::create(std::move(file), nullptr);
        QSharedPointer<InstanceService> instance =
            QSharedPointer<InstanceService>::create(InstancePath.path().split('/').last(), ApplicationPath.path(), QString{"/"});
        app->m_Instances.insert(InstancePath, instance);
        m_am->m_applicationList.insert(ApplicationPath, app);
    }

    static void TearDownTestCase() { m_am->deleteLater(); }

    static inline ApplicationManager1Service *m_am{nullptr};
    const static inline QDBusObjectPath ApplicationPath{QString{DDEApplicationManager1ObjectPath} + "/" +
                                                        QUuid::createUuid().toString(QUuid::Id128)};
    const static inline QDBusObjectPath InstancePath{ApplicationPath.path() + "/" + QUuid::createUuid().toString(QUuid::Id128)};
};

TEST_F(TestApplicationManager, identifyService)
{
    if (m_am == nullptr) {
        GTEST_SKIP() << "skip for now...";
    }
    using namespace std::chrono_literals;
    // for service unit
    auto workingDir = QDir::cleanPath(QDir::current().absolutePath() + QDir::separator() + ".." + QDir::separator() + "tools");
    QFile pidFile{workingDir + QDir::separator() + "pid.txt"};
    if (pidFile.exists()) {
        ASSERT_TRUE(pidFile.remove());
    }

    QProcess fakeServiceProc;
    fakeServiceProc.setWorkingDirectory(workingDir);
    auto InstanceId = InstancePath.path().split('/').last();
    fakeServiceProc.start("/usr/bin/systemd-run",
                          {{QString{R"(--unit=app-DDE-test\x2dApplication@%1.service)"}.arg(InstanceId)},
                           {"--user"},
                           {QString{"--working-directory=%1"}.arg(workingDir)},
                           {"--slice=app.slice"},
                           {"./fake-process.sh"}});
    fakeServiceProc.waitForFinished();
    if (fakeServiceProc.exitStatus() != QProcess::ExitStatus::NormalExit) {
        GTEST_SKIP() << "invoke systemd-run failed.";
    }

    std::this_thread::sleep_for(500ms);

    auto success = pidFile.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly);
    EXPECT_TRUE(success);

    if (!success) {
        qWarning() << pidFile.errorString();
        fakeServiceProc.terminate();
        ASSERT_TRUE(false);
    }

    bool ok{false};
    auto fakePid = pidFile.readLine().toInt(&ok);
    ASSERT_TRUE(ok);

    auto pidfd = pidfd_open(fakePid, 0);
    ASSERT_TRUE(pidfd > 0) << std::strerror(errno);
    QDBusObjectPath application;
    QDBusObjectPath application_instance;
    auto appId = m_am->Identify(QDBusUnixFileDescriptor{pidfd}, application, application_instance);
    EXPECT_EQ(appId.toStdString(), QString{"test-Application"}.toStdString());
    EXPECT_EQ(application.path().toStdString(), ApplicationPath.path().toStdString());
    EXPECT_EQ(application_instance.path().toStdString(), InstancePath.path().toStdString());
    close(pidfd);

    if (pidFile.exists()) {
        pidFile.remove();
    }

    // for scope unit
    ASSERT_TRUE(QProcess::startDetached("/usr/bin/systemd-run",
                                        {{"--scope"},
                                         {QString{R"(--unit=app-DDE-test\x2dApplication-%1.scope)"}.arg(InstanceId)},
                                         {"--user"},
                                         {QString{"--working-directory=%1"}.arg(workingDir)},
                                         {"--slice=app.slice"},
                                         {"./fake-process.sh"},
                                         {"Scope"}},
                                        workingDir));

    std::this_thread::sleep_for(500ms);

    success = pidFile.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly);
    EXPECT_TRUE(success);

    if (!success) {
        qWarning() << pidFile.errorString();
        ASSERT_TRUE(false);
    }

    ok = false;
    fakePid = pidFile.readLine().toInt(&ok);
    ASSERT_TRUE(ok);

    pidfd = pidfd_open(fakePid, 0);
    ASSERT_TRUE(pidfd > 0) << std::strerror(errno);

    appId = m_am->Identify(QDBusUnixFileDescriptor{pidfd}, application, application_instance);
    EXPECT_EQ(appId.toStdString(), QString{"test-Application"}.toStdString());
    EXPECT_EQ(application.path().toStdString(), ApplicationPath.path().toStdString());
    EXPECT_EQ(application_instance.path().toStdString(), InstancePath.path().toStdString());
    close(pidfd);

    if (pidFile.exists()) {
        pidFile.remove();
    }
}
