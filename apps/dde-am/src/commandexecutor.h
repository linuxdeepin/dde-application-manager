// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <DExpected>

class CommandExecutor
{
public:
    void setProgram(const QString &program);
    void setArguments(const QStringList &arguments);
    void setType(const QString &type);
    void setRunId(const QString &runId);
    void setWorkDir(const QString &workdir);
    void setEnvironmentVariables(const QStringList &envVars);

    Dtk::Core::DExpected<void> execute();

private:
    QString m_program;
    QStringList m_args;
    QString m_type;
    QString m_runId;
    QString m_workdir;
    QStringList m_environmentVariables;
};
