// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "launcher.h"
#include "lang.h"
#include "desktopinfo.h"
#include "settings.h"
#include "basedir.h"
#include "launchersettings.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QtDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QDateTime>
#include <QProcess>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QEventLoop>
#include <QFileInfo>

#include <DDesktopServices>

#include <regex>
#include <stdlib.h>
#include <thread>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

#define SETTING LauncherSettings::instance()

const QString LASTORE_SERVICE = "org.deepin.dde.Lastore1";
const QString LASTORE_PATH = "/org/deepin/dde/Lastore1";
const QString LASTORE_INTERFACE = "org.deepin.dde.Lastore1.Manager";

Launcher::Launcher(QObject *parent)
    : SynModule(parent)
    , m_appInfo(DesktopInfo(""))
{
    registeModule("launcher");
    appsHidden = SETTING->getHiddenApps();
    initSettings();

    for (auto dir : BaseDir::appDirs()) {
        appDirs.push_back(dir.c_str());
    }

    loadDesktopPkgMap();
    loadPkgCategoryMap();
    loadNameMap();
    initItems();

    initConnection();
}

Launcher::~Launcher()
{
    QDBusConnection::sessionBus().unregisterObject(dbusPath);
}

void Launcher::setSyncConfig(QByteArray ba)
{
    if (!SETTING)
        return;

    QJsonDocument doc = QJsonDocument::fromJson(ba);
    QJsonObject obj = doc.object();
    SETTING->setDisplayMode(obj["display_mode"].toInt() == 1 ? "category" : "free");
    SETTING->setFullscreenMode(obj["fullscreen"].toBool());
}

QByteArray Launcher::getSyncConfig()
{
    QJsonObject obj;
    obj["version"] = "1.0";
    obj["display_mode"] = SETTING->getDisplayMode();
    obj["fullscreen"] = SETTING->getFullscreenMode();
    QJsonDocument doc(obj);
    return doc.toJson();
}

const QMap<QString, Item> *Launcher::getItems()
{
    return &itemsMap;
}

int Launcher::getDisplayMode()
{
    return SETTING->getDisplayMode() == "category" ? 1 : 0;
}

bool Launcher::getFullScreen()
{
    return SETTING->getFullscreenMode();
}

void Launcher::setDisplayMode(int value)
{
    SETTING->setDisplayMode(value == 1 ? "category" : "free");
}

void Launcher::setFullscreen(bool isFull)
{
    SETTING->setFullscreenMode(isFull);
}

/**
 * @brief Launcher::getAllItemInfos 获取所有应用信息
 * @return
 */
LauncherItemInfoList Launcher::getAllItemInfos()
{
    LauncherItemInfoList allItemList;
    for (auto &item : m_desktopAndItemMap)
        allItemList.push_back(item.info);

    return allItemList;
}

/**
  * @brief Launcher::getAllNewInstalledApps 获取所有新安装且未打开的应用
  * @return
  */
QStringList Launcher::getAllNewInstalledApps()
{
    QStringList ret;
    QMap<QString, QStringList> newApps;
    QDBusInterface interface = QDBusInterface("org.deepin.dde.AlRecorder1", "/org/deepin/dde/AlRecorder1", "org.deepin.dde.AlRecorder1");
    QDBusReply<QMap<QString, QStringList>> reply = interface.call("GetNew");
    if (reply.isValid())
        newApps = reply;

    for (auto iter = newApps.begin(); iter != newApps.end(); iter++) {
        for (QString name : iter.value()) {
            QString path = iter.key() + name  + ".desktop";
            Item item = getItemByPath(path);
            if (item.isValid()) {
                ret << item.info.id;
            }
        }
    }
    return ret;
}

/**
 * @brief Launcher::getDisableScaling 获取应用是否禁用缩放
 * @param appId
 * @return
 */
bool Launcher::getDisableScaling(QString appId)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return false;

    for (const auto &app : SETTING->getDisableScalingApps()) {
        if (app == appId)
            return true;
    }
    return false;
}

/**
 * @brief Launcher::getItemInfo 获取应用信息
 * @param appId
 * @return
 */
LauncherItemInfo Launcher::getItemInfo(QString appId)
{
    LauncherItemInfo info;
    if (itemsMap.find(appId) == itemsMap.end())
        return info;

    info = itemsMap[appId].info;
    return info;
}

/**
 * @brief Launcher::getUseProxy 获取应用是否代理
 * @param appId
 * @return
 */
bool Launcher::getUseProxy(QString appId)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return false;

    for (const auto &app : SETTING->getUseProxyApps()) {
        if (app == appId)
            return true;
    }
    return false;
}

/**
 * @brief Launcher::isItemOnDesktop 桌面是否存在应用desktop文件
 * @param appId
 * @return
 */
bool Launcher::isItemOnDesktop(QString appId)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return  false;

    QString filePath(QDir::homePath() + "/Desktop/" + appId + ".desktop");
    QFileInfo info(filePath);
    return info.exists();
}

/**
 * @brief Launcher::requestRemoveFromDesktop 移除桌面快捷方式
 * @param appId
 * @return
 */
bool Launcher::requestRemoveFromDesktop(QString appId)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return false;

    QString filePath(QDir::homePath() + "/Desktop/" + appId + ".desktop");
    QFileInfo info(filePath);
    if (!info.exists())
        return true;

    QFile file(filePath);
    return file.remove();
}

/**
 * @brief Launcher::requestSendToDesktop 发送应用到桌面
 * @param appId
 * @return
 */
bool Launcher::requestSendToDesktop(QString appId)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return false;

    QString filePath(QDir::homePath() + "/Desktop/" + appId + ".desktop");
    QFileInfo info(filePath);
    if (info.exists()) // 已经存在
        return false;

    // 创建桌面快捷方式文件
    DesktopInfo dinfo(itemsMap[appId].info.path.toStdString());
    dinfo.getKeyFile()->setKey(MainSection, dbusService.toStdString(), "X-Deepin-CreatedBy");
    dinfo.getKeyFile()->setKey(MainSection, appId.toStdString(), "X-Deepin-AppID");
    if (!dinfo.getKeyFile()->saveToFile(filePath.toStdString()))
        return false;

    // 播放音频
    DDesktopServices::playSystemSoundEffect(DDesktopServices::SSE_SendFileComplete);

    return true;
}

/**
 * @brief Launcher::requestUninstall 卸载应用
 * @param appId
 */
void Launcher::requestUninstall(const QString &desktop)
{
    QString appDesktopPath = desktop;

    bool exist = false;
    for (const Item &item : m_desktopAndItemMap.values()) {
        if (item.info.path == appDesktopPath) {
            exist = true;
            break;
        }
    }

    if (!exist) {
        qWarning() << QString(" %1 uninstall faill ...").arg(appDesktopPath);
        return;
    }

    // 限制调用方
    QString servicePid = QString::number(QDBusConnection::sessionBus().interface()->servicePid(message().service()));
    QString cmd = QString("cat /proc/%1/cmdline").arg(servicePid);
    QProcess process;
    QStringList args {"-c", cmd};
    process.start("sh", args);
    process.waitForReadyRead();
    QString result = QString::fromUtf8(process.readAllStandardOutput());
    process.close();
    if (result != launcherExe) {
        qWarning() << result << " has no right to uninstall " << appDesktopPath;
        return;
    }

    if (!m_desktopAndItemMap.keys().contains(appDesktopPath)) {
        QFileInfo fileInfo(appDesktopPath);
        if (!fileInfo.isSymLink()) {
            qWarning() << QString("can't find desktopPath: %1").arg(appDesktopPath);
            return;
        }

        appDesktopPath = fileInfo.symLinkTarget();
    }

    const Item &item = m_desktopAndItemMap[appDesktopPath];
    DesktopInfo info(item.info.path.toStdString());
    if (!info.isValidDesktop()) {
        qWarning() << QString("%1 desktop file is invalid...").arg(item.info.name);
        return;
    }

    m_appInfo = info;
    doUninstall(info, item);
}

/**
 * @brief Launcher::setDisableScaling 设置应用是否禁用缩放
 * @param appId
 * @param value
 */
void Launcher::setDisableScaling(QString appId, bool value)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return;

    QVector<QString> apps = SETTING->getDisableScalingApps();
    if (value) {
        for (const auto &app : apps) {
            if (app == appId) {
                return;
            }
        }
        apps.append(appId);
    } else {
        bool found = false;
        for (auto iter = apps.begin(); iter != apps.end(); iter++) {
            if (*iter == appId) {
                found = true;
                apps.erase(iter);
                break;
            }
        }
        if (!found)
            return;
    }

    SETTING->setDisableScalingApps(apps);
}

/**
 * @brief Launcher::setUseProxy 设置用户代理
 * @param appId
 * @param value
 */
void Launcher::setUseProxy(QString appId, bool value)
{
    if (itemsMap.find(appId) == itemsMap.end())
        return;


    QVector<QString> apps = SETTING->getUseProxyApps();
    if (value) {
        for (const auto &app : apps) {
            if (app == appId) {
                return;
            }
        }
        apps.append(appId);
    } else {
        bool found = false;
        for (auto iter = apps.begin(); iter != apps.end(); iter++) {
            if (*iter == appId) {
                found = true;
                apps.erase(iter);
                break;
            }
        }
        if (!found)
            return;
    }
    SETTING->setUseProxy(apps);
}

/**
 * @brief Launcher::handleFSWatcherEvents 处理文件改变事件
 * @param msg
 */
void Launcher::handleFSWatcherEvents(QDBusMessage msg)
{
    QList<QVariant> ret = msg.arguments();
    if (ret.size() != 2)
        return;

    QString filePath = ret[0].toString();
    int op = ret[0].toInt();

    // desktop包文件变化
    if (filePath == desktopPkgMapFile) {
        loadDesktopPkgMap();

        // retry queryPkgName for m.noPkgItemIDs
        for (auto iter = noPkgItemIds.begin(); iter != noPkgItemIds.end(); iter++) {
            QString id = iter.key();
            QString pkg = queryPkgName(id, "");
            if (pkg.isEmpty())
                continue;

            Item &item = itemsMap[id];
            Categorytype ty = queryCategoryId(&item);
            if (qint64(ty) != item.info.categoryId) {
                item.info.categoryId = qint64(ty);
                emitItemChanged(&item, appStatusModified);
            }
            noPkgItemIds.remove(id);
        }
    } else if (filePath == applicationsFile) {  // 应用信息文件变化
        loadPkgCategoryMap();
    } else if (filePath.endsWith(".desktop")){  // desktop文件变化
        onCheckDesktopFile(filePath);
    }
}

void Launcher::onHandleUninstall(const QDBusMessage &message)
{
    QList<QVariant> arguments = message.arguments();

    if (3 != arguments.count())
        return;

    QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
    const QStringList keys = changedProps.keys();

    QString status;
    if (keys.contains("Status")) {
        status = changedProps["Status"].toString();
    }

#ifdef QT_DEBUG
        qInfo() <<  "changedProps: " << changedProps << ", status: " << status;
#endif
    const QString &desktop = QString::fromStdString(m_appInfo.getFileName());
    if (status == "succeed" || status == "end") {
        removeDesktop(desktop);
        removeAutoStart(desktop);
        Q_EMIT uninstallSuccess(desktop);
    } else if (status == "failed") {
        Q_EMIT uninstallFailed(desktop, QString());
    }
}

/**
 * @brief Launcher::initSettings 初始化启动器配置
 */
void Launcher::initSettings()
{
    connect(SETTING, &LauncherSettings::displayModeChanged, this, [&](QString mode) {
        Q_EMIT displayModeChanged(mode == "category" ? 1 : 0);
    });
    connect(SETTING, &LauncherSettings::fullscreenChanged, this, &Launcher::fullScreenChanged);
    connect(SETTING, &LauncherSettings::hiddenAppsChanged, this, &Launcher::handleAppHiddenChanged);
}

/**
 * @brief Launcher::loadDesktopPkgMap 加载应用包信息
 */
void Launcher::loadDesktopPkgMap()
{
    QFile file(desktopPkgMapFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QVariantMap varMap = obj.toVariantMap();
    desktopPkgMap.clear();
    for (auto iter = varMap.begin(); iter != varMap.end(); iter++) {
        if (!QDir::isAbsolutePath(iter.key()))
            continue;

        QString appId = getAppIdByFilePath(iter.key(), appDirs);
        if (appId == "")
            continue;

        desktopPkgMap[appId] = iter.value().toString();
    }
}

/**
 * @brief Launcher::loadPkgCategoryMap 加载应用类型信息
 */
void Launcher::loadPkgCategoryMap()
{
    QFile file(applicationsFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QVariantMap varMap = obj.toVariantMap();
    pkgCategoryMap.clear();
    for (auto iter = varMap.begin(); iter != varMap.end(); iter++) {
        if (!iter.value().toJsonValue().isObject())
            continue;

        QJsonObject infoObj = iter.value().toJsonObject();
        QVariantMap infoMap = infoObj.toVariantMap();
        QString category = infoMap["category"].toString();
        pkgCategoryMap[iter.key()] = Category::parseCategoryString(category);
    }
}

void Launcher::onCheckDesktopFile(const QString &filePath, int type)
{
    Q_UNUSED(type);

    if (filePath.isEmpty()) {
        qWarning() << "Desktop path is empty. ";
        return;
    }

    DesktopInfo info(filePath.toStdString());
    if (info.isValidDesktop()) {
        Item newItem = NewItemWithDesktopInfo(info);
        bool shouldShow = info.shouldShow() &&
                !isDeepinCustomDesktopFile(newItem.info.path) &&
                !appsHidden.contains(newItem.info.id);

        if (m_desktopAndItemMap.find(filePath) != m_desktopAndItemMap.end()) {
            if (shouldShow) {
                // update item
                addItem(newItem);
                emitItemChanged(&newItem, appStatusModified);
            } else {
                // hide item
                emitItemChanged(&newItem, appStatusDeleted);
            }
        } else if (shouldShow) {
            if (info.isExecutableOk()) {
                // add item
                addItem(newItem);
                emitItemChanged(&newItem, appStatusCreated);
            }
        }
    } else {
        if (m_desktopAndItemMap.find(filePath) != m_desktopAndItemMap.end()) {
            // remove item
            const Item &item = itemsMap[filePath];
            removeDesktop(filePath);
            emitItemChanged(&item, appStatusDeleted);
        }
    }
}

void Launcher::onNewAppLaunched(const QString &filePath)
{
    Item item = getItemByPath(filePath);

    if (item.isValid())
        Q_EMIT newAppLaunched(item.info.id);
}

void Launcher::initConnection()
{
    QDBusConnection::sessionBus().connect("org.deepin.dde.DFWatcher1",
                                          "/org/deepin/dde/DFWatcher1",
                                          "org.deepin.dde.DFWatcher1",
                                          "Event",
                                          this,
                                          SLOT(onCheckDesktopFile(const QString &, int)));

    QDBusConnection::sessionBus().connect("org.deepin.dde.AlRecorder1",
                                          "/org/deepin/dde/AlRecorder1",
                                          "org.deepin.dde.AlRecorder1",
                                          "Launched",
                                          this,
                                          SLOT(onNewAppLaunched(const QString &)));
}

/**
 * @brief Launcher::handleAppHiddenChanged 响应DConfig中隐藏应用配置变化
 */
void Launcher::handleAppHiddenChanged()
{
    auto hiddenApps = SETTING->getHiddenApps();
    QSet<QString> newSet, oldSet;
    for (const auto &app : hiddenApps)
        newSet.insert(app);

    for (const auto &app : appsHidden)
        oldSet.insert(app);

    // 处理新增隐藏应用
    for (const auto &app : newSet - oldSet) {
        if (itemsMap.find(app) == itemsMap.end())
            continue;

        emitItemChanged(&itemsMap[app], appStatusDeleted);
        itemsMap.remove(app);
    }

    // 处理显示应用
    for (const auto &appId : oldSet - newSet) {
        if (itemsMap.find(appId) == itemsMap.end())
            continue;

        DesktopInfo info = DesktopInfo(itemsMap[appId].info.path.toStdString());
        if (!info.isValidDesktop()) {
            qWarning() << "invalid Desktop Path";
            continue;
        }

        Item item = NewItemWithDesktopInfo(info);

        if (!(info.shouldShow() && !isDeepinCustomDesktopFile(info.getFileName().c_str())))
            continue;

        addItem(item);
        emitItemChanged(&item, appStatusCreated);
    }

    appsHidden.clear();
    for (const auto &appId : hiddenApps) {
        appsHidden.push_back(appId);
    }
}

/**
 * @brief Launcher::loadNameMap 加载翻译应用信息
 */
void Launcher::loadNameMap()
{
    QFile file(appNameTranslationsFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    QJsonObject obj = doc.object();
    QVariantMap varMap = obj.toVariantMap();
    nameMap.clear();

    QString lang(queryLangs()[0].data());
    for (auto iter = varMap.begin(); iter != varMap.end(); iter++) {
        // 过滤非Object
        if (!iter.value().toJsonValue().isObject())
            continue;

        // 过滤非本地语言
        if (iter.key() != lang)
            continue;

        QJsonObject infoObj = iter.value().toJsonObject();
        QVariantMap infoMap = infoObj.toVariantMap();
        for (auto infoIter = infoMap.begin(); infoIter != infoMap.end(); infoIter++) {
            // 过滤Object
            if (infoIter.value().toJsonValue().isObject())
                continue;

            nameMap[infoIter.key()] = infoIter.value().toString();
        }
    }
}

/**
 * @brief Launcher::initItems 初始化应用信息
 */
void Launcher::initItems()
{
    itemsMap.clear();
    m_desktopAndItemMap.clear();
    std::vector<DesktopInfo> infos = AppsDir::getAllDesktopInfos();
    for (auto &app : infos) {
        if (!app.isExecutableOk()
                || app.getId().empty()
                || isDeepinCustomDesktopFile(app.getFileName().c_str()))
        {
            continue;
        }

        Item item = NewItemWithDesktopInfo(app);
        if (appsHidden.contains(item.info.id))
            continue;

        addItem(item);
    }
}

void Launcher::addItem(Item &item)
{
    if (!item.isValid()) {
        qDebug() << "item is invalid, item info:" << item.info.path;
        return;
    }

    if (nameMap.size() > 0 && nameMap.find(item.info.id) != nameMap.end()) {
        QString name = nameMap[item.info.id];
        if (!name.isEmpty())
            item.info.name = name;
    }

    item.info.categoryId = qint64(queryCategoryId(&item));
    itemsMap[item.info.id] = item;

    QFileInfo desktopInfo(item.info.path);
    if (desktopInfo.isSymLink()) {
        m_desktopAndItemMap[desktopInfo.symLinkTarget()] = item;
    } else {
        m_desktopAndItemMap[item.info.path] = item;
    }
}

Categorytype Launcher::queryCategoryId(const Item *item)
{
    QString pkg = queryPkgName(item->info.id, item->info.path);
    if (pkg.isEmpty()) {
        noPkgItemIds[item->info.id] = 1;

        if (pkgCategoryMap.find(pkg) != pkgCategoryMap.end())
            return pkgCategoryMap[pkg];
    }

    Categorytype category = Category::parseCategoryString(item->xDeepinCategory);
    if (category != Categorytype::CategoryErr)
        return category;

    return getXCategory(item);
}

/**
 * @brief Launcher::getXCategory 获取应用类型
 * @param item
 * @return
 */
Categorytype Launcher::getXCategory(const Item *item)
{
    // 统计应用类型
    QMap<Categorytype, int> categoriesMap;
    for (const auto &category : item->categories) {
        QString lCategory = category.toLower();
        QList<Categorytype> tys;
        Categorytype ty = Category::parseCategoryString(lCategory);
        if (ty != Categorytype::CategoryErr)
            tys.push_back(ty);
        else if (Category::parseXCategoryString(lCategory).size() > 0) {
            tys.append(Category::parseXCategoryString(lCategory));
        }

        for (const auto & ty : tys)
            categoriesMap[ty]++;
    }

    categoriesMap.remove(Categorytype::CategoryOthers);
    if (categoriesMap.size() == 0)
        return Categorytype::CategoryOthers;

    // 计算最多的类型
    int max = 0;
    Categorytype ty {Categorytype::CategoryOthers};
    for (auto iter = categoriesMap.begin(); iter != categoriesMap.end(); iter++) {
        if (iter.value() > max) {
            max = iter.value();
            ty = iter.key();
        }
    }

    QList<Categorytype> maxCatogories {ty};
    for (auto iter = categoriesMap.begin(); iter != categoriesMap.end(); iter++) {
        if (iter.key() != ty && iter.value() == max) {
            maxCatogories.push_back(iter.key());
        }
    }

    if (maxCatogories.size() == 1)
        return maxCatogories[0];

    std::sort(maxCatogories.begin(), maxCatogories.end());

    // 检查是否同时存在音乐和视频播放器
    QPair<bool, bool> found;
    for (auto iter = maxCatogories.begin(); iter != maxCatogories.end(); iter++) {
        if (*iter == Categorytype::CategoryMusic)
            found.first = true;
        else if (*iter == Categorytype::CategoryVideo)
            found.second = true;
    }

    if (found.first && found.second)
        return Categorytype::CategoryVideo;

    return maxCatogories[0];
}

/**
 * @brief Launcher::queryPkgNameWithDpkg 使用dpkg -S，通过文件路径查包
 * @param itemPath
 * @return
 */
QString Launcher::queryPkgNameWithDpkg(const QString &itemPath)
{
    QProcess process;
    process.start("dpkg -S " + itemPath);
    if (!process.waitForFinished())
        return QString();

    QByteArray output = process.readAllStandardOutput();
    if (output.size() == 0)
        return QString();

    std::vector<std::string> splits = DString::splitChars(output.data(), '\n');
    if (splits.size() > 0) {
        std::vector<std::string> parts = DString::splitStr(splits[0], ':');
        if (parts.size() == 2)
            return parts[0].c_str();
    }
    return QString();
}

/**
 * @brief Launcher::queryPkgName 通过id、path查询包名
 * @param itemID
 * @param itemPath
 * @return
 */
QString Launcher::queryPkgName(const QString &itemID, const QString &itemPath)
{
    QFileInfo itemInfo(itemPath);
    if (itemPath.isEmpty() || !itemInfo.isFile())
        return QString();

    // 处理desktop文件是软连接的情况
    if (itemInfo.isSymLink()) {
        std::string path = itemInfo.symLinkTarget().toStdString();
        std::smatch result;
        const std::regex e("^/opt/apps/([^/]+)/entries/applications/.*");
        if (std::regex_match(path, result, e) && result.size() == 2) {
            // dpkg命令检查通过路径匹配的包是否存在
            QString pkgName(result[1].str().c_str());
            QProcess process;
            process.start("dpkg -s " + pkgName);
            if (process.waitForFinished())
                return pkgName;

            // 当包不存在则使用dpkg -S来查找包
            process.start("dpkg -S" + pkgName);
            if (!process.waitForFinished())
                return QString();

            QByteArray output = process.readAllStandardOutput();
            if (output.size() == 0)
                return QString();

            std::vector<std::string> splits = DString::splitChars(output.data(), ':');
            if (splits.size() < 2)
                return QString();

            return splits[0].c_str();
        }
    }

    if (DString::startWith(itemID.toStdString(), "org.deepin.flatdeb."))
        return QString("deepin-fpapp-") + itemID;

    if (desktopPkgMap.find(itemID) != desktopPkgMap.end())
        return desktopPkgMap[itemID];

    return QString();
}

/**
 * @brief Launcher::getItemByPath 根据desktop路径获取Item信息
 * @param itemPath
 * @return
 */
Item Launcher::getItemByPath(QString itemPath)
{
    QString appId = getAppIdByFilePath(itemPath, appDirs);
    if (itemsMap.find(appId) == itemsMap.end())
        return Item();

    if (itemsMap[appId].info.path == itemPath)
        return itemsMap[appId];

    return Item();
}

void Launcher::emitItemChanged(const Item *item, QString status)
{
    LauncherItemInfo info(item->info);
    Q_EMIT itemChanged(status, info, info.categoryId);
}

AppType Launcher::getAppType(DesktopInfo &info, const Item &item)
{
    AppType ty = AppType::Default;
    QFileInfo  fileInfo;
    // 判断是否为flatpak应用
    do {
        if (!info.getKeyFile()->containKey(MainSection, "X-Flatpak"))
            break;

        std::vector<std::string> parts = DString::splitStr(info.getCommandLine(), ' ');
        if (parts.size() == 0)
            break;

        fileInfo.setFile(parts[0].c_str());
        if (flatpakBin != fileInfo.baseName())
            break;

        ty = AppType::Flatpak;
        goto end;
    } while (0);

    // 判断是否为ChromeShortcut
    do {
        if (!DString::startWith(item.info.id.toStdString(), "chrome-"))
            break;

        if (!std::regex_match(item.exec.toStdString(), std::regex("google-chrome.*--app-id=")))
            break;

        ty = AppType::ChromeShortcut;
        goto end;
    } while (0);

    // 判断是否为CrossOver
    do {
        fileInfo.setFile(info.getExecutable().c_str());
        QString execBase(fileInfo.baseName());
        if (execBase.contains("CrossOver") && (execBase.contains("crossover") || execBase.contains("cxuninstall"))) {
            ty = AppType::CrossOver;
            goto end;
        }
    } while (0);

    // 判断是否为wineApp
    do {
        std::string createdBy = info.getKeyFile()->getStr(MainSection, "X-Created-By");
        if (DString::startWith(createdBy, "cxoffice-") || strstr(info.getCommandLine().c_str(), "env WINEPREFIX=")) {
            ty = AppType::WineApp;
            goto end;
        }
    } while (0);


end:
    return ty;
}

bool Launcher::isLingLongApp(const QString &filePath)
{
    return filePath.contains("/persistent/linglong");
}

/**
 * @brief Launcher::doUninstall 执行卸载操作
 * @param info
 * @param item
 * @return
 */
void Launcher::doUninstall(DesktopInfo &info, const Item &item)
{
    if (isLingLongApp(QString::fromStdString(info.getFileName()))) {
        QProcess process;
        // 获取 appId
        QStringList args;
        QString appId = item.exec.section(' ', 2, 2);
        args.append("uninstall");
        args.append(appId);

        int retCode = QProcess::execute("ll-cli", args);
        if (retCode != 0) {
            Q_EMIT uninstallFailed(item.info.path, QString());
            notifyUninstallDone(item, false);
            qWarning() << QString("uninstall %1 failed...").arg(info.getFileName().c_str());
            return;
        }

        Q_EMIT uninstallSuccess(item.info.path);

        // 浮窗通知
        notifyUninstallDone(item, true);
    } else {
        // 查询包名
        QString pkg = queryPkgName(item.info.id, item.info.path);
        if (pkg.isEmpty())
            pkg = queryPkgNameWithDpkg(item.info.path);

        if (pkg.isEmpty()) {
            qWarning() << "uninstall failed, becase package name is Empty";
            return;
        }

        // 检测包是否安装
        QDBusInterface lastoreDbus = QDBusInterface(LASTORE_SERVICE, LASTORE_PATH, LASTORE_INTERFACE, QDBusConnection::systemBus());
        QDBusReply<bool> reply = lastoreDbus.call("PackageExists", pkg);

        // 包未安装时
        if (!(reply.isValid() && reply.value()))
            return;

        // 卸载系统应用
        uninstallApp(item.info.name, pkg);
    }
}

/**
 * @brief Launcher::uninstallFlatpak 卸载flatpak应用
 * @param info
 * @param item
 */
void Launcher::uninstallFlatpak(DesktopInfo &info, const Item &item)
{
    struct FlatpakApp {
        std::string name;
        std::string arch;
        std::string branch;
    } flat;
    std::vector<std::string> parts = DString::splitStr(info.getCommandLine(), ' ');
    for (unsigned long idx = 0; idx < parts.size(); idx++) {
        if (flat.branch.empty() && DString::startWith(parts[idx], "--branch="))
            flat.branch.assign(parts[idx].c_str() + strlen("--branch="));

        if (flat.arch.empty() && DString::startWith(parts[idx], "--arch="))
            flat.arch.assign(parts[idx].c_str() + strlen("--arch="));

        if (flat.name.empty() && idx != 0 && !DString::startWith(parts[idx], "--") && strstr(parts[idx].c_str(), "."))
            flat.name.assign(parts[idx]);

        if (!flat.branch.empty() && !flat.arch.empty() && !flat.name.empty())
            break;
    }

    if (flat.branch.empty() || flat.arch.empty() || flat.name.empty()) {
        qInfo() << "uninstall Flatpak failed";
        return;
    }

    bool userApp = item.info.path.startsWith(QDir::homePath());
    QString sysOrUser = "--user";
    if (!userApp) {
        sysOrUser = "--system";
        QString pkgFile = QString("/usr/share/deepin-flatpak/app/%1/%2/%3/pkg").arg(flat.name.c_str()).arg(flat.arch.c_str()).arg(flat.branch.c_str());
        QFile file(pkgFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return;

        QString content(file.readAll());
        QString pkgName = content.trimmed();
        uninstallApp(item.info.name, pkgName);
    } else {
        QString ref = QString("app/%1/%2/%3").arg(flat.name.c_str()).arg(flat.arch.c_str()).arg(flat.branch.c_str());
        qInfo() << "uninstall flatpak ref= " << ref;
        QProcess process;
        process.start("flatpak " + sysOrUser + " uninstall " + ref);
        bool res = process.waitForFinished();
        std::thread thread([&] {
            notifyUninstallDone(item, res);
        });
        thread.detach();
    }
}

/**
 * @brief Launcher::uninstallWineApp 卸载wine应用
 * @param item
 * @return
 */
bool Launcher::uninstallWineApp(const Item &item)
{
    QProcess process;
    process.start("/opt/deepinwine/tools/uninstall.sh" + item.info.path);
    bool res = process.waitForFinished();
    std::thread thread([&] {
        notifyUninstallDone(item, res);
    });
    return res;
}

void Launcher::uninstallApp(const QString &name, const QString &pkg)
{
    QDBusInterface lastoreDbus = QDBusInterface(LASTORE_SERVICE, LASTORE_PATH, LASTORE_INTERFACE, QDBusConnection::systemBus());
    QDBusReply<QDBusObjectPath> reply = lastoreDbus.call(QDBus::Block, "RemovePackage", name, pkg);

    if (!reply.isValid() || reply.value().path().isEmpty()) {
        qWarning() << "RemovePackage failed: " << reply.error();
        return;
    }

    QString servicePath = reply.value().path();

    QDBusConnection::systemBus().disconnect(LASTORE_SERVICE, servicePath, "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged","sa{sv}as", this, SLOT(onHandleUninstall(const QDBusMessage &)));
    QDBusConnection::systemBus().connect(LASTORE_SERVICE, servicePath, "org.freedesktop.DBus.Properties",
                                         "PropertiesChanged","sa{sv}as", this, SLOT(onHandleUninstall(const QDBusMessage &)));
}

/** 移除desktop文件
 * @brief Launcher::removeDesktop
 * @param desktop
 */
void Launcher::removeDesktop(const QString &desktop)
{
    QFileInfo fileInfo(desktop);
    const QString &curDesktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString &appDesktopPath = curDesktop + "/" + fileInfo.fileName();

    QFile file(appDesktopPath);
    if (!file.exists()) {
        qDebug() << "file not exist...item info: " << desktop;
        return;
    }

    // 删除应用列表中的数据
    m_desktopAndItemMap.remove(desktop);

    // 删除发送到桌面的应用
    file.remove();
}

/**
 * @brief Launcher::notifyUninstallDone 发送卸载结果
 * @param item
 * @param result
 */
void Launcher::notifyUninstallDone(const Item &item, bool result)
{
    QString msg;
    if (result)
        msg = QString(tr("Removed successfully"));
    else
        msg = QString(tr("Failed to remove the app"));

    QList<QVariant> argList;
    argList << QString("deepin-app-store")
            << quint32(0)
            << QString("deepin-app-store")
            << msg
            << QString("")
            << QStringList("")
            << QVariantMap()
            << qint32(-1);

    QDBusInterface interface = QDBusInterface("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications");
    interface.callWithArgumentList(QDBus::Block, "Notify", argList);
}

void Launcher::removeAutoStart(const QString &desktop)
{
   QFileInfo fileInfo(desktop);
   const QString &autostartPath = BaseDir::userAutoStartDir().c_str() + fileInfo.fileName();

   QFile file(autostartPath);
   if (!file.exists()) {
       qDebug() << QString("desktop file  %1 doesn't exist...").arg(autostartPath);
       return;
   }

   file.remove();
}

/**
 * @brief Launcher::isDeepinCustomDesktopFile 检测是否为Deepin定制应用
 * @param fileName
 * @return
 */
bool Launcher::isDeepinCustomDesktopFile(QString fileName)
{
    QFileInfo fileInfo(fileName);
    return fileInfo.dir() == QDir::homePath() + ".local/share/applications"
            && fileInfo.baseName().startsWith("deepin-custom-");
}

Item Launcher::NewItemWithDesktopInfo(DesktopInfo &info)
{
    QString enName(info.getKeyFile()->getStr(MainSection, KeyName).c_str());
    QString enComment(info.getKeyFile()->getStr(MainSection, KeyComment).c_str());
    QString xDeepinCategory(info.getKeyFile()->getStr(MainSection, "X-Deepin-Category").c_str());
    QString xDeepinVendor(info.getKeyFile()->getStr(MainSection, "X-Deepin-Vendor").c_str());

    QString appName;
    if (xDeepinVendor == "deepin")
        appName = info.getGenericName().c_str();

    if (appName.isEmpty())
        appName = info.getName().c_str();

    if (appName.isEmpty())
        appName = info.getId().c_str();

    QString appFileName(info.getFileName().c_str());
    QFileInfo fileInfo(appFileName);
    int64_t ctime = fileInfo.birthTime().toSecsSinceEpoch();

    Item item;
    item.info.path = appFileName;
    item.info.name = appName;
    item.info.keywords << enName << appName;
    item.info.id = getAppIdByFilePath(item.info.path, appDirs);
    item.info.timeInstalled = ctime;
    item.exec = info.getCommandLine().c_str();
    item.genericName = info.getGenericName().c_str();
    item.comment = enComment;
    if (!info.getIcon().empty()) {
        item.info.icon = info.getIcon().c_str();
    }

    xDeepinCategory = xDeepinCategory.toLower();

    for (auto &keyWord : info.getKeywords()) {
        item.desktopKeywords.push_back(QString(keyWord.c_str()).toLower());
    }

    for (auto &category : info.getCategories()) {
        item.categories.push_back(QString(category.c_str()).toLower());
    }
    return item;
}

QString Launcher::getAppIdByFilePath(QString filePath, QStringList dirs)
{
    QString path = QDir::cleanPath(filePath);
    QString appId;
    for (auto dir : dirs) {
        if (path.startsWith(dir)) {
            appId = QDir(dir).relativeFilePath(path);
            break;
        }
    }

    if (appId.isEmpty())
        return QString();

    return appId.remove(".desktop");
}
