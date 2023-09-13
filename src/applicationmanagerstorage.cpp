// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmanagerstorage.h"
#include "constant.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QDir>
#include <memory>

std::shared_ptr<ApplicationManager1Storage>
ApplicationManager1Storage::createApplicationManager1Storage(const QString &storageDir) noexcept
{
    QDir dir;
    auto dirPath = QDir::cleanPath(storageDir);
    if (!dir.mkpath(dirPath)) {
        qCritical() << "can't create directory";
        return nullptr;
    }

    dir.setPath(dirPath);
    auto storagePath = dir.filePath("storage.json");
    auto obj = std::shared_ptr<ApplicationManager1Storage>(new (std::nothrow) ApplicationManager1Storage{storagePath});

    if (!obj) {
        qCritical() << "new ApplicationManager1Storage failed.";
        return nullptr;
    }

    if (!obj->m_file->open(QFile::ReadWrite)) {
        qCritical() << "can't open file:" << storagePath;
        return nullptr;
    }

    auto content = obj->m_file->readAll();
    if (content.isEmpty()) {  // new file
        obj->setVersion(STORAGE_VERSION);
        return obj;
    }

    // TODO: support migrate from lower storage version.

    QJsonParseError err;
    auto json = QJsonDocument::fromJson(content, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "parse json failed:" << err.errorString() << "clear this file content.";
        obj->m_file->resize(0);
    } else {
        obj->m_data = json.object();
    }

    return obj;
}

ApplicationManager1Storage::ApplicationManager1Storage(const QString &storagePath)
    : m_file(std::make_unique<QFile>(storagePath))
{
}

void ApplicationManager1Storage::writeToFile() const noexcept
{
    if (!m_file) {
        qCritical() << "file is nullptr";
        return;
    }

    if (!m_file->resize(0)) {
        qCritical() << "failed to clear file:" << m_file->errorString();
        return;
    }

    auto content = QJsonDocument{m_data}.toJson(QJsonDocument::Compact);
    auto bytes = m_file->write(content, content.size());
    if (bytes != content.size()) {
        qWarning() << "Incomplete file writes:" << m_file->errorString();
    }

    if (!m_file->flush()) {
        qCritical() << "io error.";
    }
}

void ApplicationManager1Storage::setVersion(uint8_t version) noexcept
{
    m_data["version"] = version;
    writeToFile();
}

uint8_t ApplicationManager1Storage::version() const noexcept
{
    return m_data["version"].toInt(0);
}

void ApplicationManager1Storage::createApplicationValue(const QString &appId,
                                                        const QString &groupName,
                                                        const QString &valueKey,
                                                        const QVariant &value) noexcept
{
    if (appId.isEmpty() or groupName.isEmpty() or valueKey.isEmpty()) {
        return;
    }

    QJsonObject appObj;
    if (m_data.contains(appId)) {
        appObj = m_data[appId].toObject();
    }

    QJsonObject groupObj;
    if (appObj.contains(groupName)) {
        groupObj = appObj[groupName].toObject();
    }

    if (groupObj.contains(valueKey)) {
        return;
    }

    groupObj.insert(valueKey, value.toJsonValue());
    appObj.insert(groupName, groupObj);
    m_data.insert(appId, appObj);

    writeToFile();
}

void ApplicationManager1Storage::updateApplicationValue(const QString &appId,
                                                        const QString &groupName,
                                                        const QString &valueKey,
                                                        const QVariant &value) noexcept
{
    if (appId.isEmpty() or groupName.isEmpty() or valueKey.isEmpty()) {
        return;
    }

    if (!m_data.contains(appId)) {
        return;
    }
    auto appObj = m_data[appId].toObject();

    if (!appObj.contains(groupName)) {
        return;
    }
    auto groupObj = appObj[groupName].toObject();

    if (!groupObj.contains(valueKey)) {
        return;
    }

    groupObj.insert(valueKey, value.toJsonValue());
    appObj.insert(groupName, groupObj);
    m_data.insert(appId, appObj);

    writeToFile();
}

QVariant ApplicationManager1Storage::readApplicationValue(const QString &appId,
                                                          const QString &groupName,
                                                          const QString &valueKey) const noexcept
{
    return m_data[appId][groupName][valueKey].toVariant();
}

void ApplicationManager1Storage::deleteApplicationValue(const QString &appId,
                                                        const QString &groupName,
                                                        const QString &valueKey) noexcept
{
    if (appId.isEmpty()) {
        auto empty = QJsonObject{};
        m_data.swap(empty);
        return;
    }

    auto app = m_data.find(appId).value();
    if (app.isNull()) {
        return;
    }
    auto appObj = app.toObject();

    if (groupName.isEmpty()) {
        m_data.remove(appId);
        return;
    }

    auto group = appObj.find(groupName).value();
    if (group.isNull()) {
        return;
    }
    auto groupObj = group.toObject();

    if (valueKey.isEmpty()) {
        appObj.remove(groupName);
        m_data.insert(appId, appObj);
        return;
    }

    auto val = groupObj.find(valueKey).value();
    if (val.isNull()) {
        return;
    }

    groupObj.remove(valueKey);
    appObj.insert(groupName, groupObj);
    m_data.insert(appId, appObj);

    writeToFile();
}
