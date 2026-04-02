// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "global.h"
#include "applicationchecker.h"
#include "desktopentry.h"

#include <QCoreApplication>
#include <QDir>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <QSet>

using namespace Qt::StringLiterals;

namespace {

// Use dde-am to launch the application.
// dde-am will handle the DBus communication with dde-application-manager,
// ensuring proper environment setup and systemd scope wrapping.
constexpr auto program = QStringView{DDEApplicationBin};

void launchApplication(const DesktopFile &file)
{
    qInfo() << "launch autostart application" << file.desktopId() << "by dde-am";

    QStringList args;
    args.reserve(2);
    // Mark this launch as autostart to suppress splash in AM
    args << u"--autostart"_s << file.sourcePath();

    QProcess launcher;
    launcher.setProgram(program.toString());
    launcher.setArguments(args);

    if (!launcher.startDetached()) {
        qWarning() << "failed to start" << program << "with args" << args << ":" << launcher.errorString();
    }
}

void scanAndLaunch()
{
    QSet<QString> seenFileNames;
    for (const auto &dirPath : getAutoStartDirs()) {
        const QDir dir{dirPath};
        if (!dir.exists()) {
            continue;
        }

        const auto infoList = dir.entryInfoList({u"*.desktop"_s}, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable, QDir::Name);
        for (const auto &info : infoList) {
            if (seenFileNames.contains(info.fileName())) {
                continue;
            }

            ParserError searchErr{ParserError::InternalError};
            auto desktopSource = DesktopFile::searchDesktopFileByPath(info.absoluteFilePath(), searchErr);
            if (searchErr != ParserError::NoError) {
                qWarning() << "skip" << info.absoluteFilePath();
                continue;
            }

            auto desktopFile = std::move(desktopSource).value();
            seenFileNames.insert(info.fileName());
            const auto &id = desktopFile.desktopId();
            DesktopFileGuard guard{desktopFile};
            if (!guard.try_open()) {
                continue;
            }

            auto &file = desktopFile.sourceFileRef();
            DesktopEntry tmp;
            auto parseErr = tmp.parse(file);
            if (parseErr != ParserError::NoError) {
                qWarning() << "parse autostart file" << desktopFile.sourcePath() << " error:" << parseErr;
                continue;
            }

            if (ApplicationFilter::tryExecCheck(tmp) || ApplicationFilter::showInCheck(tmp)
                || ApplicationFilter::hiddenCheck(tmp)) {
                qInfo() << "autostart application " << id << " couldn't pass check:" << desktopFile.sourcePath();
                continue;
            }

            launchApplication(desktopFile);
        }
    }
}
}  // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(u"dde-autostart"_s);
    QCoreApplication::setOrganizationName(u"deepin"_s);

    auto value = qgetenv("XDG_SESSION_TYPE");
    if (!value.isEmpty() && value == "wayland") {
        scanAndLaunch();
        return 0;
    }

    const auto XSettings = u"org.deepin.dde.XSettings1"_s;
    auto *watcher =
        new QDBusServiceWatcher(XSettings, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration, &app);

    auto launchSlot = [] {
        qDebug() << "XSettings is registered, launching autostart apps.";
        scanAndLaunch();
        QCoreApplication::quit();
    };

    QObject::connect(watcher, &QDBusServiceWatcher::serviceRegistered, &app, launchSlot);

    auto msg =
        QDBusMessage::createMethodCall("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus", "NameHasOwner");
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
