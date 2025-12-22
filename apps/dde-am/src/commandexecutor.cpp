// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "commandexecutor.h"
#include "global.h"

#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDBusMessage>

DCORE_USE_NAMESPACE

namespace {
void registerComplexDbusType()
{
    qRegisterMetaType<ObjectInterfaceMap>();
    qDBusRegisterMetaType<ObjectInterfaceMap>();
    qRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<ObjectMap>();
    qDBusRegisterMetaType<QStringMap>();
    qRegisterMetaType<QStringMap>();
    qRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<PropMap>();
    qDBusRegisterMetaType<QDBusObjectPath>();
}
}

void CommandExecutor::setProgram(const QString &program)
{
    m_program = program;
}

void CommandExecutor::setArguments(const QStringList &arguments)
{
    m_args = arguments;
}

void CommandExecutor::setType(const QString &type)
{
    m_type = type;
}

void CommandExecutor::setRunId(const QString &runId)
{
    m_runId = runId;
}

void CommandExecutor::setWorkDir(const QString &workdir)
{
    m_workdir = workdir;
}

void CommandExecutor::setEnvironmentVariables(const QStringList &envVars)
{
    m_environmentVariables = envVars;
}

DExpected<void> CommandExecutor::execute()
{
    registerComplexDbusType();
    auto con = QDBusConnection::sessionBus();
    auto msg = QDBusMessage::createMethodCall(
        DDEApplicationManager1ServiceName, DDEApplicationManager1ObjectPath, ApplicationManager1Interface, "executeCommand");
    
    QStringMap envMap;
    for (const auto &env : std::as_const(m_environmentVariables)) {
        int idx = env.indexOf('=');
        if (idx > 0) {
            envMap.insert(env.left(idx), env.mid(idx + 1));
        }
    }

    QList<QVariant> arguments;
    arguments << m_program << m_args << m_type << m_runId << QVariant::fromValue(envMap) << m_workdir;
    
    msg.setArguments(arguments);
    auto reply = con.call(msg);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return DUnexpected{emplace_tag::USE_EMPLACE,
                           static_cast<int>(reply.type()),
                           QString("Failed to execute command: %1, error: %2").arg(m_program, reply.errorMessage())};
    }
    return {};
}
