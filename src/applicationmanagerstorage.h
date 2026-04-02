// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGERSTORAGE_H
#define APPLICATIONMANAGERSTORAGE_H

#include <QString>
#include <QJsonObject>
#include <QFile>

enum class ModifyMode : uint8_t { Create, Update };

class ApplicationManager1Storage
{
public:
    ApplicationManager1Storage(const ApplicationManager1Storage &) = delete;
    ApplicationManager1Storage(ApplicationManager1Storage &&) = default;
    ApplicationManager1Storage &operator=(const ApplicationManager1Storage &) = delete;
    ApplicationManager1Storage &operator=(ApplicationManager1Storage &&) = default;
    ~ApplicationManager1Storage() = default;

    [[nodiscard]] bool createApplicationValue(
        QStringView appId, QStringView groupName, QStringView valueKey, const QVariant &value, bool deferCommit = false) noexcept;
    [[nodiscard]] bool updateApplicationValue(
        QStringView appId, QStringView groupName, QStringView valueKey, const QVariant &value, bool deferCommit = false) noexcept;
    [[nodiscard]] QVariant readApplicationValue(QStringView appId, QStringView groupName, QStringView valueKey) const noexcept;
    [[nodiscard]] bool
    deleteApplicationValue(QStringView appId, QStringView groupName, QStringView valueKey, bool deferCommit = false) noexcept;
    [[nodiscard]] bool clearData() noexcept;
    [[nodiscard]] bool deleteApplication(QStringView appId, bool deferCommit = false) noexcept;
    [[nodiscard]] bool deleteGroup(QStringView appId, QStringView groupName, bool deferCommit = false) noexcept;

    bool setVersion(uint8_t version) noexcept;
    [[nodiscard]] uint8_t version() const noexcept;

    bool setFirstLaunch(bool first) noexcept;
    [[nodiscard]] bool firstLaunch() const noexcept;
    void beginBatchUpdate() noexcept;
    [[nodiscard]] bool endBatchUpdate() noexcept;

    [[nodiscard]] static std::shared_ptr<ApplicationManager1Storage>
    createApplicationManager1Storage(const QString &storageDir) noexcept;

private:
    [[nodiscard]] bool writeToFile() noexcept;
    explicit ApplicationManager1Storage(QString storagePath);
    QString m_storagePath;
    QJsonObject m_data;
    bool m_batchUpdate{false};
    bool m_pendingWrite{false};
};

#endif
