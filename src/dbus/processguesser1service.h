// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PROCESSGUESSER1SERVICE_H
#define PROCESSGUESSER1SERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QDBusUnixFileDescriptor>
#include "applicationmanager1service.h"

class ProcessGuesser1Service : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    ProcessGuesser1Service(QDBusConnection &connection, ApplicationManager1Service *parent) noexcept;
    ~ProcessGuesser1Service() override = default;
    ProcessGuesser1Service(const ProcessGuesser1Service &) = delete;
    ProcessGuesser1Service(ProcessGuesser1Service &&) = delete;
    ProcessGuesser1Service &operator=(const ProcessGuesser1Service &) = delete;
    ProcessGuesser1Service &operator=(ProcessGuesser1Service &&) = delete;
    ApplicationManager1Service *myParent() { return dynamic_cast<ApplicationManager1Service *>(QObject::parent()); }
    [[nodiscard]] const ApplicationManager1Service *myParent() const
    {
        return dynamic_cast<ApplicationManager1Service *>(QObject::parent());
    }

public Q_SLOTS:
    QString GuessApplicationId(const QDBusUnixFileDescriptor &pidfd) noexcept;

private:
    [[nodiscard]] static bool checkTryExec(QString tryExec) noexcept;
};

#endif
