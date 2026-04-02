// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmanagerstorage.h"
#include "constant.h"
#include <QFileInfo>
#include <QJsonDocument>
#include <QDir>
#include <memory>
#include <QSaveFile>

constexpr QStringView firstLaunchKey{u"firstLaunch"};
constexpr QStringView versionKey{u"version"};

std::shared_ptr<ApplicationManager1Storage>
ApplicationManager1Storage::createApplicationManager1Storage(const QString &storageDir) noexcept
{
    using namespace Qt::StringLiterals;

    const QDir dir{QDir::cleanPath(storageDir)};
    if (!dir.mkpath(u"."_s)) {
        qCritical() << "can't create directory";
        return nullptr;
    }

    auto storagePath = dir.filePath(u"storage.json"_s);
    auto obj = std::shared_ptr<ApplicationManager1Storage>(new (std::nothrow) ApplicationManager1Storage{storagePath});

    if (!obj) {
        qCritical() << "new ApplicationManager1Storage failed.";
        return nullptr;
    }

    QFile file{storagePath};
    if (!file.open(QFile::ReadWrite | QFile::Text)) {
        qCritical() << "can't open file:" << storagePath;
        return nullptr;
    }

    auto content = file.readAll();
    if (file.error() != QFile::NoError) {
        qCritical() << "read file:" << storagePath << "failed:" << file.errorString();
        return nullptr;
    }

    if (content.isEmpty()) {  // new file
        obj->m_data.insert(versionKey, STORAGE_VERSION);
        obj->m_data.insert(firstLaunchKey, true);
        return obj;
    }

    // TODO: support migrate from lower storage version.

    QJsonParseError err;
    auto json = QJsonDocument::fromJson(content, &err);
    if (err.error != QJsonParseError::NoError) {
        qDebug() << "parse json failed:" << err.errorString() << "clear this file content.";
        file.resize(0);
        obj->m_data.insert(versionKey, STORAGE_VERSION);
        obj->m_data.insert(firstLaunchKey, true);
    } else {
        obj->m_data = json.object();
        obj->m_data.insert(firstLaunchKey, false);
    }

    return obj;
}

ApplicationManager1Storage::ApplicationManager1Storage(QString storagePath)
    : m_storagePath(std::move(storagePath))
{
}

bool ApplicationManager1Storage::writeToFile() noexcept
{
    if (m_batchUpdate) {
        m_pendingWrite = true;
        return true;
    }

    QSaveFile file{m_storagePath};
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "open file:" << m_storagePath << "failed:" << file.errorString();
        return false;
    }

    auto content = QJsonDocument{m_data}.toJson(QJsonDocument::Compact);
    auto bytes = file.write(content);
    if (bytes != content.size()) {
        qWarning() << "Incomplete file writes:" << file.errorString();
    }

    if (!file.commit()) {
        qCritical() << "commit new content failed:" << file.errorString();
        return false;
    }

    m_pendingWrite = false;
    return true;
}

bool ApplicationManager1Storage::setVersion(uint8_t version) noexcept
{
    m_data.insert(versionKey, version);
    return writeToFile();
}

uint8_t ApplicationManager1Storage::version() const noexcept
{
    return m_data.value(versionKey).toInt(-1);
}

bool ApplicationManager1Storage::setFirstLaunch(bool first) noexcept
{
    m_data.insert(firstLaunchKey, first);
    return writeToFile();
}

[[nodiscard]] bool ApplicationManager1Storage::firstLaunch() const noexcept
{
    return m_data.value(firstLaunchKey).toBool(true);
}

void ApplicationManager1Storage::beginBatchUpdate() noexcept
{
    m_batchUpdate = true;
}

bool ApplicationManager1Storage::endBatchUpdate() noexcept
{
    m_batchUpdate = false;
    if (!m_pendingWrite) {
        return true;
    }

    return writeToFile();
}

bool ApplicationManager1Storage::createApplicationValue(
    QStringView appId, QStringView groupName, QStringView valueKey, const QVariant &value, bool deferCommit) noexcept
{
    if (appId.isEmpty() || groupName.isEmpty() || valueKey.isEmpty()) {
        qWarning() << "unexpected empty string";
        return false;
    }

    auto app = m_data.find(appId);
    auto appObj = app == m_data.end() ? QJsonObject{} : app->toObject();

    auto group = appObj.find(groupName);
    auto groupObj = group == appObj.end() ? QJsonObject{} : group->toObject();

    if (groupObj.contains(valueKey)) {
        qInfo() << "value" << valueKey << value << "is already exists.";
        return true;
    }

    groupObj.insert(valueKey, QJsonValue::fromVariant(value));
    appObj.insert(groupName, std::move(groupObj));

    if (app != m_data.end()) {
        *app = std::move(appObj);
    } else {
        m_data.insert(appId, std::move(appObj));
    }

    return deferCommit ? true : writeToFile();
}

bool ApplicationManager1Storage::updateApplicationValue(
    QStringView appId, QStringView groupName, QStringView valueKey, const QVariant &value, bool deferCommit) noexcept
{
    if (appId.isEmpty() || groupName.isEmpty() || valueKey.isEmpty()) {
        qWarning() << "unexpected empty string";
        return false;
    }

    auto app = m_data.find(appId);
    if (app == m_data.constEnd()) {
        qInfo() << "app" << appId << "doesn't exists.";
        return false;
    }
    auto appObj = app->toObject();

    auto group = appObj.find(groupName);
    if (group == appObj.constEnd()) {
        qInfo() << "group" << groupName << "doesn't exists.";
        return false;
    }
    auto groupObj = group->toObject();

    auto val = groupObj.find(valueKey);
    if (val == groupObj.end()) {
        qInfo() << "value" << valueKey << "doesn't exists.";
        return false;
    }

    *val = QJsonValue::fromVariant(value);
    appObj.insert(groupName, std::move(groupObj));
    *app = std::move(appObj);

    return deferCommit ? true : writeToFile();
}

QVariant
ApplicationManager1Storage::readApplicationValue(QStringView appId, QStringView groupName, QStringView valueKey) const noexcept
{
    if (appId.isEmpty() || groupName.isEmpty() || valueKey.isEmpty()) {
        qWarning() << "unexpected empty string";
        return {};
    }

    auto app = m_data.constFind(appId);
    if (app == m_data.constEnd()) {
        return {};
    }
    auto appObj = app->toObject();

    auto group = appObj.constFind(groupName);
    if (group == appObj.constEnd()) {
        return {};
    }
    auto groupObj = group->toObject();

    auto value = groupObj.constFind(valueKey);
    if (value == groupObj.constEnd()) {
        return {};
    }

    return value->toVariant();
}

bool ApplicationManager1Storage::deleteApplicationValue(QStringView appId,
                                                        QStringView groupName,
                                                        QStringView valueKey,
                                                        bool deferCommit) noexcept
{
    if (appId.isEmpty() || groupName.isEmpty() || valueKey.isEmpty()) {
        qWarning() << "unexpected empty string";
        return false;
    }

    auto app = m_data.find(appId);
    if (app == m_data.end()) {
        return true;
    }
    auto appObj = app->toObject();

    auto group = appObj.find(groupName);
    if (group == appObj.end()) {
        return true;
    }
    auto groupObj = group->toObject();

    auto val = groupObj.find(valueKey);
    if (val == groupObj.end()) {
        return true;
    }

    groupObj.erase(val);
    appObj.insert(groupName, std::move(groupObj));
    m_data.insert(appId, std::move(appObj));

    return deferCommit ? true : writeToFile();
}

bool ApplicationManager1Storage::clearData() noexcept
{
    QJsonObject obj;
    m_data.swap(obj);
    return setVersion(STORAGE_VERSION);
}

bool ApplicationManager1Storage::deleteApplication(QStringView appId, bool deferCommit) noexcept
{
    if (appId.isEmpty()) {
        qWarning() << "unexpected empty string.";
        return false;
    }

    m_data.remove(appId);
    return deferCommit ? true : writeToFile();
}

bool ApplicationManager1Storage::deleteGroup(QStringView appId, QStringView groupName, bool deferCommit) noexcept
{
    if (appId.isEmpty() || groupName.isEmpty()) {
        qWarning() << "unexpected empty string.";
        return false;
    }

    auto app = m_data.find(appId);
    if (app == m_data.end()) {
        return true;
    }
    auto appObj = app->toObject();

    auto group = appObj.find(groupName);
    if (group == appObj.end()) {
        return true;
    }

    appObj.erase(group);
    m_data.insert(appId, std::move(appObj));
    return deferCommit ? true : writeToFile();
}
