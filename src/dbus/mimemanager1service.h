// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MIMEMANAGERSERVICE_H
#define MIMEMANAGERSERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QDBusObjectPath>
#include "global.h"
#include "applicationmimeinfo.h"

class ApplicationManager1Service;

class MimeManager1Service : public QObject, public QDBusContext
{
    Q_OBJECT
public:
    explicit MimeManager1Service(ApplicationManager1Service *parent);
    ~MimeManager1Service() override;

    void appendMimeInfo(MimeInfo &&info);
    [[nodiscard]] const auto &infos() const noexcept { return m_infos; }
    [[nodiscard]] auto &infos() noexcept { return m_infos; }

public Q_SLOTS:
    [[nodiscard]] ObjectMap listApplications(const QString &mimeType) const noexcept;
    [[nodiscard]] QString queryFileTypeAndDefaultApplication(const QString &filePath,
                                                             QDBusObjectPath &application) const noexcept;
    void setDefaultApplication(const KVPairs &defaultApps) noexcept;
    void unsetDefaultApplication(const QStringList &mimeTypes) noexcept;

private:
    QMimeDatabase m_database;
    std::vector<MimeInfo> m_infos;
};

#endif
