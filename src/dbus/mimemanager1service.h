// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef MIMEMANAGERSERVICE_H
#define MIMEMANAGERSERVICE_H

#include <QObject>
#include <QDBusContext>
#include <QDBusObjectPath>
#include <QFileSystemWatcher>
#include <QTimer>

#include "global.h"
#include "applicationmimeinfo.h"

class ApplicationManager1Service;

class MimeManager1Service : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit MimeManager1Service(ApplicationManager1Service *parent);
    ~MimeManager1Service() override;

    void appendMimeInfo(MimeInfo &&info);
    [[nodiscard]] const auto &infos() const noexcept { return m_infos; }
    [[nodiscard]] auto &infos() noexcept { return m_infos; }
    void reset() noexcept;
    void updateMimeCache(QString dir) noexcept;

public Q_SLOTS:
    [[nodiscard]] ObjectMap listApplications(const QString &mimeType) const noexcept;
    [[nodiscard]] QString queryDefaultApplication(const QString &content, QDBusObjectPath &application) const noexcept;
    void setDefaultApplication(const QStringMap &defaultApps) noexcept;
    void unsetDefaultApplication(const QStringList &mimeTypes) noexcept;

Q_SIGNALS:
    void MimeInfoReloaded();

private Q_SLOTS:
    void onMimeAppsFileChanged(const QString &path);
    void handleMimeAppsFileDebounced();

private:
    QMimeDatabase m_database;
    std::vector<MimeInfo> m_infos;
    QFileSystemWatcher m_mimeAppsWatcher;
    // 内部写入标志，用于避免触发外部修改的处理
    bool m_internalWriteInProgress{false};
    QTimer m_mimeAppsDebounceTimer;
};

#endif
