// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "common.h"
#include "synmodule.h"
#include "category.h"
#include "launcheriteminfolist.h"
#include "desktopinfo.h"

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
    QString id;
    QString icon;
    Categorytype categoryId;
    int64_t timeInstalled;
    //QStringList keywords; // 存放前端搜索关键字 name enName
};

// 应用信息类
struct Item {
   inline bool isValid() {return !info.path.isEmpty();}

   LauncherItemInfo info;
   QStringList desktopKeywords; //desktop file keywords
   QStringList categories;
   QString xDeepinCategory;
   QString exec;
   QString genericName;
   QString comment;
   std::map<QString, int> searchTargets;
};

class Launcher : public SynModule, public QDBusContext
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent);
    ~Launcher();

    // 设置配置
    void setSyncConfig(QByteArray ba);
    QByteArray getSyncConfig();

    void initItems();
    const QMap<QString, Item> *getItems();

    int getDisplayMode();
    bool getFullScreen();
    void setDisplayMode(int value);
    void setFullscreen(bool isFull);

    LauncherItemInfoList getAllItemInfos();
    QStringList getAllNewInstalledApps();
    bool getDisableScaling(QString appId);
    LauncherItemInfo getItemInfo(QString appId);
    bool getUseProxy(QString appId);
    bool isItemOnDesktop(QString appId);
    bool requestRemoveFromDesktop(QString appId);
    bool requestSendToDesktop(QString appId);
    void requestUninstall(const QString &desktop);
    void setDisableScaling(QString appId, bool value);
    void setUseProxy(QString appId, bool value);

Q_SIGNALS:
    void itemChanged(QString status, LauncherItemInfo itemInfo, qint64 ty);
    void newAppLaunched(QString appId);
    void uninstallSuccess(const QString &desktop);
    void uninstallFailed(const QString &desktop, QString errMsg);

    void displayModeChanged(int mode);
    void fullScreenChanged(bool isFull);

    void uninstallStatusChanged(const bool status);

private Q_SLOTS:
    void handleFSWatcherEvents(QDBusMessage msg);
    void onCheckDesktopFile(const QString &filePath, int type = 0);
    void onNewAppLaunched(const QString &filePath);
    void onHandleUninstall(const QDBusMessage &message);

private:
    void initConnection();
    void initSettings();
    void loadDesktopPkgMap();
    void loadPkgCategoryMap();
    void handleAppHiddenChanged();
    void loadNameMap();
    QString getAppIdByFilePath(QString filePath, QStringList dirs);
    bool isDeepinCustomDesktopFile(QString fileName);
    Item NewItemWithDesktopInfo(DesktopInfo &info);
    void addItem(Item &item);
    Categorytype queryCategoryId(const Item *item);
    Categorytype getXCategory(const Item *item);
    QString queryPkgName(const QString &itemID, const QString &itemPath);
    QString queryPkgNameWithDpkg(const QString &itemPath);
    Item getItemByPath(QString itemPath);
    void emitItemChanged(const Item *item, QString status);
    AppType getAppType(DesktopInfo &info, const Item &item);
    bool isLingLongApp(const QString &filePath);
    void doUninstall(DesktopInfo &info, const Item &item);
    void uninstallFlatpak(DesktopInfo &info, const Item &item);
    bool uninstallWineApp(const Item &item);
    void uninstallApp(const QString &name, const QString &pkg);
    void removeDesktop(const QString &desktop);
    void notifyUninstallDone(const Item &item, bool result);
    void removeAutoStart(const QString &desktop);

private:
    QMap<QString, Item> itemsMap;                                   // appId, Item
    QMap<QString, QString> desktopPkgMap;
    QMap<QString, Categorytype> pkgCategoryMap;
    QMap<QString, QString> nameMap;                                 // appId, Name
    QMap<QString, int> noPkgItemIds;
    QVector<QString> appsHidden;

    QStringList appDirs;

    QMap<QString, Item> m_desktopAndItemMap;                        // desktoppath,Item
    DesktopInfo m_appInfo;                                          // 卸载应用
};

#endif // LAUNCHER_H
