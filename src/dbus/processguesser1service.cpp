// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "processguesser1adaptor.h"
#include "applicationservice.h"
#include "global.h"

ProcessGuesser1Service::ProcessGuesser1Service(QDBusConnection &connection, ApplicationManager1Service *parent) noexcept
    : QObject(parent)
{
    if (!connection.registerService("org.desktopspec.ProcessGuesser1")) {
        qFatal("%s", connection.lastError().message().toLocal8Bit().data());
    }

    if (auto *tmp = new (std::nothrow) ProcessGuesser1Adaptor{this}; tmp == nullptr) {
        qFatal("new Process Guesser Adaptor failed.");
    }

    if (!registerObjectToDBus(this, "/org/desktopspec/ProcessGuesser1", "org.desktopspec.ProcessGuesser1")) {
        std::terminate();
    }
}

bool ProcessGuesser1Service::checkTryExec(QString tryExec) noexcept
{
    if (!tryExec.isEmpty()) {
        return false;
    }

    QFileInfo exeBin{tryExec};
    if (!exeBin.isAbsolute()) {
        tryExec = QStandardPaths::findExecutable(tryExec);
    }

    if (!tryExec.isEmpty()) {
        exeBin.setFile(tryExec);
        if (!exeBin.exists() or !exeBin.isFile() or !exeBin.isExecutable()) {
            return false;
        }
    }

    return true;
}

QString ProcessGuesser1Service::GuessApplicationId(const QDBusUnixFileDescriptor &pidfd) noexcept
{
    if (!pidfd.isValid()) {
        sendErrorReply(QDBusError::InvalidArgs);
        return {};
    }

    auto pid = getPidFromPidFd(pidfd);
    if (pid == 0) {
        sendErrorReply(QDBusError::Failed, "Pid is invalid");
        return {};
    }

    QString exePath = QString{"/proc/%1/exe"}.arg(pid);
    QFileInfo info{exePath};

    if (!info.exists()) {
        sendErrorReply(QDBusError::Failed, "Pid is invalid.");
        return {};
    }
    const auto &binary = info.symLinkTarget();

    const auto &applications = myParent()->Applications();
    QString appId;

    for (const QSharedPointer<ApplicationService> &app : applications) {
        auto exec = app->findEntryValue(DesktopFileEntryKey, "Exec", EntryValueType::Raw).toString();
        if (exec.isEmpty()) {  // NOTE: Exec is not required in desktop file.
            continue;
        }

        auto opt = ApplicationService::unescapeExecArgs(exec);
        if (!opt) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        auto execList = std::move(opt).value();
        if (execList.isEmpty()) {
            sendErrorReply(QDBusError::InternalError);
            return {};
        }

        auto execBin = execList.first();
        QFileInfo execInfo{execBin};
        if (!execInfo.isAbsolute()) {
            execBin = QStandardPaths::findExecutable(execBin);
        }

        if (!execBin.isEmpty() and execBin == binary) {
            if (!appId.isEmpty()) {
                sendErrorReply(QDBusError::Failed, "Multiple binary have been detected.");
                return {};
            }
            appId = app->id();
            continue;
        }

        auto tryExec = app->findEntryValue(DesktopFileEntryKey, "TryExec", EntryValueType::String).toString();
        if (!checkTryExec(tryExec)) {
            sendErrorReply(QDBusError::Failed, "Couldn't find the binary which corresponding with process.");
            return {};
        }
    }

    if (appId.isEmpty()) {
        sendErrorReply(QDBusError::Failed, "Couldn't found application.");
        return {};
    }

    return appId;
}
