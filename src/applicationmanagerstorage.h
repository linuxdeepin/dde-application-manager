// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef APPLICATIONMANAGERSTORAGE_H
#define APPLICATIONMANAGERSTORAGE_H

#include <QString>
#include <QJsonObject>
#include <QFile>

enum class ModifyMode { Create, Update };

class ApplicationManager1Storage
{
public:
    ApplicationManager1Storage(const ApplicationManager1Storage &) = delete;
    ApplicationManager1Storage(ApplicationManager1Storage &&) = default;
    ApplicationManager1Storage &operator=(const ApplicationManager1Storage &) = delete;
    ApplicationManager1Storage &operator=(ApplicationManager1Storage &&) = default;
    ~ApplicationManager1Storage() = default;

    bool createApplicationValue(const QString &appId,
                                const QString &groupName,
                                const QString &valueKey,
                                const QVariant &value) noexcept;
    bool updateApplicationValue(const QString &appId,
                                const QString &groupName,
                                const QString &valueKey,
                                const QVariant &value) noexcept;
    [[nodiscard]] QVariant
    readApplicationValue(const QString &appId, const QString &groupName, const QString &valueKey) const noexcept;
    bool deleteApplicationValue(const QString &appId, const QString &groupName, const QString &valueKey) noexcept;
    bool clearData() noexcept;
    bool deleteApplication(const QString &appId) noexcept;
    bool deleteGroup(const QString &appId, const QString &groupName) noexcept;

    bool setVersion(uint8_t version) noexcept;
    [[nodiscard]] uint8_t version() const noexcept;

    static std::shared_ptr<ApplicationManager1Storage> createApplicationManager1Storage(const QString &storageDir) noexcept;

private:
    [[nodiscard]] bool writeToFile() const noexcept;
    explicit ApplicationManager1Storage(const QString &storagePath);
    std::unique_ptr<QFile> m_file;
    QJsonObject m_data;
};

#endif
