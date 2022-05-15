/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "common.h"
#include "synmodule.h"
#include "category.h"

#include <QObject>
#include <QMap>
#include <QVector>
#include <QDBusMessage>
#include <QDBusContext>

// 同步数据
struct SyncData {
    QString version;
    QString display_mode;
    bool fullscreen;
};

// 应用类型
enum class AppType {
    Flatpak,
    ChromeShortcut,
    CrossOver,
    WineApp,
    Default
};

// 应用基本信息
struct ItemInfo
{
    QString path;
    QString name;
    QString enName;
    QString id;
    QString icon;
    Categorytype categoryId;
    int64_t timeInstalled;
};

// 应用信息类
struct Item {
   inline bool isValid() {return !info.path.isEmpty();}

   ItemInfo info;
   QStringList keywords;
   QStringList categories;
   QString xDeepinCategory;
   QString exec;
   QString genericName;
   QString comment;
   std::map<QString, int> searchTargets;
};


class DesktopInfo;
class Launcher : public SynModule, public QDBusContext
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent);
    ~Launcher();

    // 设置配置
    void setSynConfig(QByteArray ba);
    QByteArray getSyncConfig();
    const QMap<QString, Item> *getItems();

    int getDisplayMode();
    bool getFullScreen();
    void setDisplayMode(int value);
    void setFullScreen(bool isFull);

    QVector<ItemInfo> getAllItemInfos();
    QStringList getAllNewInstalledApps();
    bool getDisableScaling(QString appId);
    ItemInfo getItemInfo(QString appId);
    bool getUseProxy(QString appId);
    bool isItemOnDesktop(QString appId);
    bool requestRemoveFromDesktop(QString appId);
    bool requestSendToDesktop(QString appId);
    void requestUninstall(QString appId);
    void setDisableScaling(QString appId, bool value);
    void setUseProxy(QString appId, bool value);

Q_SIGNALS:
    void ItemChanged(QString status, ItemInfo itemInfo, Categorytype ty);
    void NewAppLaunched(QString appId);
    void UninstallSuccess(QString appId);
    void UninstallFailed(QString appId, QString errMsg);

    void displayModeChanged(int mode);
    void fullScreenChanged(bool isFull);

private Q_SLOTS:
    void handleFSWatcherEvents(QDBusMessage msg);
    void handleLRecoderRestart(QDBusMessage msg);

private:
    void initSettings();
    void loadDesktopPkgMap();
    void loadPkgCategoryMap();
    void checkDesktopFile(QString filePath);
    void handleAppHiddenChanged();
    void loadNameMap();
    void initItems();
    QString getAppIdByFilePath(QString filePath, QStringList dirs);
    bool isDeepinCustomDesktopFile(QString fileName);
    Item NewItemWithDesktopInfo(DesktopInfo &info);
    void addItem(Item item);
    Categorytype queryCategoryId(const Item *item);
    Categorytype getXCategory(const Item *item);
    QString queryPkgName(const QString &itemID, const QString &itemPath);
    QString queryPkgNameWithDpkg(const QString &itemPath);
    Item getItemByPath(QString itemPath);
    void watchDataDirs();
    void emitItemChanged(const Item *item, QString status);
    AppType getAppType(DesktopInfo &info, const Item &item);
    bool doUninstall(DesktopInfo &info, const Item &item);
    bool uninstallFlatpak(DesktopInfo &info, const Item &item);
    bool uninstallWineApp(const Item &item);
    bool uninstallSysApp(const QString &name, const QString &pkg);
    bool removeDesktop(const Item &item);
    void notifyUninstallDone(const Item &item, bool result);


    QMap<QString, Item> itemsMap;
    QMap<QString, QString> desktopPkgMap;
    QMap<QString, Categorytype> pkgCategoryMap;
    QMap<QString, QString> nameMap;
    QMap<QString, int> noPkgItemIds;
    QVector<QString> appsHidden;
    QStringList appDirs;
};

#endif // LAUNCHER_H
