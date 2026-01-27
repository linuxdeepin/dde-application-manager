// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "desktopentry.h"
#include "applicationchecker.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QDBusMessage>
#include <QDBusReply>
#include <QTimer>

void launchApplication(const DesktopFile &file, const DesktopEntry &entry)
{
    Q_UNUSED(entry);
    qInfo() << "launch autostart application" << file.desktopId() << "by dde-am";

    // Use dde-am to launch the application.
    // dde-am will handle the DBus communication with dde-application-manager,
    // ensuring proper environment setup and systemd scope wrapping.
    QString program = "/usr/bin/dde-am";
    QStringList args;
    // Mark this launch as autostart to suppress splash in AM
    args << "--autostart" << file.sourcePath();

    if (!QProcess::startDetached(program, args)) {
        qWarning() << "Failed to launch" << file.desktopId() << "by" << program << "args:" << args;
    }
}

void scanAndLaunch()
{
    auto autostartDirs = getAutoStartDirs();
    std::map<QString, DesktopFile> autostartItems;

    applyIteratively(
        QList<QDir>{autostartDirs.crbegin(), autostartDirs.crend()},
        [&autostartItems](const QFileInfo &info) {
            ParserError err{ParserError::InternalError};
            auto desktopSource = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), err);
            if (err != ParserError::NoError) {
                qWarning() << "skip" << info.absoluteFilePath();
                return false;
            }
            auto file = std::move(desktopSource).value();
            autostartItems.insert_or_assign(file.desktopId(), std::move(file));
            return false;
        },
        QDir::Readable | QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
        {"*.desktop"},
        QDir::Name | QDir::DirsLast);

    for (auto &it : autostartItems) {
        auto desktopFile = std::move(it.second);
        DesktopFileGuard guard{desktopFile};
        if (!guard.try_open()) {
            continue;
        }

        auto &file = desktopFile.sourceFileRef();
        QTextStream stream{&file};
        DesktopEntry tmp;
        auto err = tmp.parse(stream);
        if (err != ParserError::NoError) {
            qWarning() << "parse autostart file" << desktopFile.sourcePath() << " error:" << err;
            continue;
        }

        if (ApplicationFilter::tryExecCheck(tmp) || ApplicationFilter::showInCheck(tmp) || ApplicationFilter::hiddenCheck(tmp)) {
            qInfo() << "autostart application couldn't pass check:" << desktopFile.sourcePath();
            continue;
        }

        launchApplication(desktopFile, tmp);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("dde-autostart");
    app.setOrganizationName("deepin");

    auto value = QString::fromLocal8Bit(qgetenv("XDG_SESSION_TYPE"));
    if (!value.isEmpty() && value == QString::fromLocal8Bit("wayland")) {
        scanAndLaunch();
        return 0;
    } else {
        constexpr auto XSettings = "org.deepin.dde.XSettings1";

        auto *watcher =
            new QDBusServiceWatcher(XSettings, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration, &app);

        auto launchSlot = []() {
            qDebug() << "XSettings is registered, launching autostart apps.";
            scanAndLaunch();
            QCoreApplication::quit();
        };

        QObject::connect(watcher, &QDBusServiceWatcher::serviceRegistered, &app, launchSlot);

        auto msg = QDBusMessage::createMethodCall(
            "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner");
        msg << XSettings;

        auto reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ReplyMessage) {
            if (reply.arguments().constFirst().toBool()) {
                launchSlot();
                return 0;
            }
        } else {
            qWarning() << "call org.freedesktop.DBus::NameHasOwner failed:" << reply.errorMessage();
        }

        // Wait for signal
        return QCoreApplication::exec();
    }
}
