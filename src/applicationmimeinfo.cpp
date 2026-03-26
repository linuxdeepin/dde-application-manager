// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "applicationmimeinfo.h"
#include "global.h"
#include <QSaveFile>

Q_LOGGING_CATEGORY(DDEAMMime, "dde.am.mime")

using namespace Qt::StringLiterals;

namespace {

QString toString(const MimeContent &content) noexcept
{
    QString ret;

    for (const auto &[key, value] : content.asKeyValueRange()) {
        ret.append(u'[' % key % u"]\n"_sv);
        for (const auto &[iKey, iValue] : value.asKeyValueRange()) {
            ret.append(iKey % u'=');
            for (const auto &app : iValue) {
                ret.append(app % u';');
            }
            ret.append(u'\n');
        }
        ret.append(u'\n');
    }

    return ret;
}

void createUserConfig(const QString &filename) noexcept
{
    QSaveFile userFile{filename};
    if (!userFile.open(QFile::WriteOnly | QFile::Text)) {
        qCCritical(DDEAMMime) << "failed to create user file:" << filename << userFile.errorString();
        return;
    }

    constexpr auto initContent = "[Default Applications]\n";
    if (userFile.write(initContent) == -1) {
        qCWarning(DDEAMMime) << "failed to write content into" << filename << userFile.errorString();
        return;
    }

    if (userFile.commit()) {
        qCInfo(DDEAMMime) << "successfully created user mimeapps:" << filename;
    } else {
        qCWarning(DDEAMMime) << "failed to commit atomic write:" << filename << userFile.errorString();
    }
}

}  // namespace

std::optional<MimeFileBase> MimeFileBase::loadFromFile(const QFileInfo &fileInfo, bool desktopSpec)
{
    bool isWritable{false};
    auto filePath = fileInfo.absoluteFilePath();
    if (filePath.startsWith(getXDGConfigHome()) or filePath.startsWith(getXDGDataHome())) {
        isWritable = true;
    }

    QFile file{filePath};
    if (!file.open(QFile::ReadOnly | QFile::Text | QFile::ExistingOnly)) {
        qWarning() << "open" << filePath << "failed:" << file.errorString();
        return std::nullopt;
    }

    QTextStream stream{&file};
    stream.setEncoding(QStringConverter::Utf8);

    MimeContent content;
    MimeFileParser parser{stream, desktopSpec};
    if (auto err = parser.parse(content); err != ParserError::NoError) {
        qWarning() << "file:" << filePath << "parse failed:" << err;
        return std::nullopt;
    }

    return MimeFileBase{fileInfo, std::move(content), desktopSpec, isWritable};
}

MimeFileBase::MimeFileBase(const QFileInfo &info, MimeContent &&content, bool desktopSpec, bool writable)
    : m_desktopSpec(desktopSpec)
    , m_writable(writable)
    , m_info(info)
    , m_content(std::move(content))
{
}

void MimeFileBase::reload() noexcept
{
    auto newBase = MimeFileBase::loadFromFile(fileInfo(), isDesktopSpecific());
    if (!newBase) {
        qWarning() << "reload" << fileInfo().absoluteFilePath() << "failed, content wouldn't be changed.";
        return;
    }

    m_content = std::move(newBase->m_content);
}

MimeApps::MimeApps(MimeFileBase &&base)
    : MimeFileBase(std::move(base))
{
}

std::optional<MimeApps> MimeApps::createMimeApps(const QString &filePath, bool desktopSpec) noexcept
{
    auto baseOpt = MimeFileBase::loadFromFile(QFileInfo{filePath}, desktopSpec);

    if (!baseOpt) {
        return std::nullopt;
    }

    return MimeApps{std::move(baseOpt).value()};
}

void MimeApps::insertToSection(const QString &section, const QString &mimeType, const QString &appId) noexcept
{
    auto &map = content();

    auto targetSection = map.find(section);
    if (targetSection == map.end()) {
        targetSection = map.insert(section, {});
    }

    QStringList newApps{QString{appId}.append(desktopSuffix)};
    auto oldApps = targetSection->find(mimeType);
    if (oldApps != targetSection->end()) {
        newApps.append(*oldApps);
    }

    targetSection->insert(mimeType, newApps);
}

void MimeApps::addAssociation(const QString &mimeType, const QString &appId) noexcept
{
    insertToSection(addedAssociations, mimeType, appId);
}

void MimeApps::removeAssociation(const QString &mimeType, const QString &appId) noexcept
{
    insertToSection(removedAssociations, mimeType, appId);
}

void MimeApps::setDefaultApplication(const QString &mimeType, const QString &appId) noexcept
{
    auto &map = content();

    auto defaultSection = map.find(defaultApplications);
    if (defaultSection == map.end()) {
        defaultSection = map.insert(defaultApplications, {});
    }

    defaultSection->insert(mimeType, {QString{appId}.append(desktopSuffix)});
}

void MimeApps::unsetDefaultApplication(const QString &mimeType) noexcept
{
    auto &map = content();

    auto defaultSection = map.find(defaultApplications);
    if (defaultSection == map.end()) {
        return;
    }

    defaultSection->remove(mimeType);
}

AppList MimeApps::queryTypes(QString appId) const noexcept
{
    AppList ret;
    appId.append(desktopSuffix);
    const auto &lists = content();

    if (const auto &adds = lists.constFind(addedAssociations); adds != lists.cend()) {
        std::for_each(adds->constKeyValueBegin(), adds->constKeyValueEnd(), [&ret, &appId](const auto &it) {
            if (it.second.contains(appId)) {
                ret.added.append(it.first);
            }
        });
    }

    if (const auto &removes = lists.constFind(removedAssociations); removes != lists.cend()) {
        std::for_each(removes->constKeyValueBegin(), removes->constKeyValueEnd(), [&ret, &appId](const auto &it) {
            if (it.second.contains(appId)) {
                ret.removed.removeOne(it.first);
            }
        });
    }

    return ret;
}

bool MimeApps::writeToFile() const noexcept
{
    const auto filePath = fileInfo().absoluteFilePath();

    if (!isWritable()) {
        qCWarning(DDEAMMime) << "file is not writable:" << filePath;
        return false;
    }

    QSaveFile file(filePath);

    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        qCWarning(DDEAMMime) << "failed to open save file:" << filePath << file.errorString();
        return false;
    }

    const auto newContent = toString(content()).toUtf8();

    if (file.write(newContent) != newContent.size()) {
        qCWarning(DDEAMMime) << "incomplete write for:" << filePath << "error:" << file.errorString();
        return false;
    }

    if (!file.commit()) {
        qCWarning(DDEAMMime) << "failed to atomic commit:" << filePath << file.errorString();
        return false;
    }

    qCInfo(DDEAMMime) << "successfully updated mimeapps:" << filePath;
    return true;
}

QString MimeApps::queryDefaultApp(const QString &type) const noexcept
{
    const auto &map = content();
    auto defaultSection = map.constFind(defaultApplications);
    if (defaultSection == map.cend()) {
        return {};
    }

    auto defaultApp = defaultSection->constFind(type);
    if (defaultApp == defaultSection->end()) {
        return {};
    }

    const auto &app = defaultApp.value();
    if (app.isEmpty()) {
        return {};
    }

    const QStringView appView{app.first()};
    return appView.chopped(desktopSuffix.size()).toString();
}

QStringList MimeCache::queryTypes(QString appId) const noexcept
{
    QStringList ret;
    appId.append(desktopSuffix);
    const auto &cache = content()[mimeCache];
    for (auto it = cache.constKeyValueBegin(); it != cache.constKeyValueEnd(); ++it) {
        if (it->second.contains(appId)) {
            ret.append(it->first);
        }
    }
    return ret;
}

std::optional<MimeCache> MimeCache::createMimeCache(const QString &filePath) noexcept
{
    auto baseOpt = MimeFileBase::loadFromFile(QFileInfo{filePath}, false);

    if (!baseOpt) {
        return std::nullopt;
    }

    return MimeCache{std::move(baseOpt).value()};
}

MimeCache::MimeCache(MimeFileBase &&base)
    : MimeFileBase(std::move(base))
{
}

QStringList MimeCache::queryApps(const QString &type) const noexcept
{
    const auto &content = this->content();
    auto it = content.constFind(mimeCache);
    if (it == content.constEnd()) {
        qDebug() << "this cache is broken, please reload.";
        return {};
    }

    QStringList ret;
    if (auto kv = it->constFind(type); kv != it->constEnd()) {
        const auto &apps = kv.value();
        for (const auto &e : apps) {
            if (!e.endsWith(desktopSuffix)) {
                continue;
            }
            ret.append(e.chopped(desktopSuffix.size()));
        }
    }
    return ret;
}

std::optional<MimeInfo> MimeInfo::createMimeInfo(const QString &directory) noexcept
{
    MimeInfo ret;
    QDir dir;

    if (!QFileInfo::exists(directory)) {
        qCritical() << "directory " << directory << "doesn't exists.";
        return std::nullopt;
    }
    dir.setPath(directory);
    ret.m_directory = directory;

    const QFileInfo cacheFile{dir.filePath(u"mimeinfo.cache"_s)};
    if (cacheFile.exists() && cacheFile.isFile()) {
        ret.m_cache = MimeCache::createMimeCache(cacheFile.absoluteFilePath());
    }

    const auto &desktops = getCurrentDesktops();
    for (const auto &desktop : desktops) {
        const QFileInfo desktopAppList{dir.filePath(desktop % u"-mimeapps.list"_s)};
        if (desktopAppList.exists() && desktopAppList.isFile()) {
            auto desktopApps = MimeApps::createMimeApps(desktopAppList.absoluteFilePath(), true);
            if (desktopApps) {
                ret.m_appsList.emplace_back(std::move(desktopApps).value());
            }
        }
    }

    QFileInfo appList{dir.filePath(u"mimeapps.list"_s)};
    appList.setCaching(false);

    auto userMimeApps = appList.absoluteFilePath();
    if (userMimeApps.startsWith(getXDGConfigHome()) && (!appList.exists() || !appList.isFile())) {
        createUserConfig(userMimeApps);
    }

    if (appList.exists() && appList.isFile()) {
        auto apps = MimeApps::createMimeApps(appList.absoluteFilePath(), false);
        if (apps) {
            ret.m_appsList.emplace_back(std::move(apps).value());
        }
    }

    return ret;
}

void MimeInfo::reload() noexcept
{
    for (auto &app : m_appsList) {
        app.reload();
    }

    if (m_cache) {
        m_cache->reload();
    }
}
