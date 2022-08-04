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

#ifndef ENTRY_H
#define ENTRY_H

#include "appinfo.h"
#include "appmenu.h"
#include "windowinfobase.h"
#include "windowinfomap.h"

#include <QMap>
#include <QVector>
#include <QObject>

#define ENTRY_NONE      0
#define ENTRY_NORMAL    1
#define ENTRY_RECENT    2

// 单个应用类
class Dock;
class Entry: public QObject
{
    Q_OBJECT
public:
    Entry(Dock *_dock, AppInfo *_app, QString _innerId, QObject *parent = nullptr);
    ~Entry();

    bool isValid();
    QString getId() const;
    QString path() const;
    QString getName();
    void updateName();
    QString getIcon();
    QString getInnerId();
    void setInnerId(QString _innerId);
    QString getFileName();
    AppInfo *getApp();
    void setApp(AppInfo *appinfo);
    bool getIsDocked() const;
    void setIsDocked(bool value);
    void startExport();
    void stopExport();
    void setMenu(AppMenu *_menu);
    void updateMenu();
    void updateIcon();
    void updateMode();
    void forceUpdateIcon();
    void updateIsActive();
    WindowInfoBase *getWindowInfoByPid(int pid);
    WindowInfoBase *getWindowInfoByWinId(XWindow windowId);
    void setPropIsDocked(bool docked);
    void setPropIcon(QString value);
    void setPropName(QString value);
    void setPropIsActive(bool active);
    void setPropCurrentWindow(XWindow value);
    void setCurrentWindowInfo(WindowInfoBase *windowInfo);
    WindowInfoBase *getCurrentWindowInfo();
    WindowInfoBase *findNextLeader();
    QString getExec(bool oneLine);
    bool hasWindow();
    void updateExportWindowInfos();
    bool detachWindow(WindowInfoBase *info);
    bool attachWindow(WindowInfoBase *info);
    void launchApp(uint32_t timestamp);
    bool containsWindow(XWindow xid);

    void handleMenuItem(uint32_t timestamp, QString itemId);
    void handleDragDrop(uint32_t timestamp, QStringList files);
    void requestDock(bool dockToEnd = false);
    void requestUndock(bool dockToEnd = false);
    void newInstance(uint32_t timestamp);
    void check();
    void forceQuit();
    void presentWindows();
    void active(uint32_t timestamp);
    int mode();

    XWindow getCurrentWindow();
    QString getDesktopFile();
    bool getIsActive();
    QString getMenu();
    QVector<XWindow> getAllowedClosedWindowIds();
    WindowInfoMap getExportWindowInfos();

public Q_SLOTS:
    QVector<WindowInfoBase *> getAllowedCloseWindows();

Q_SIGNALS:
    void isActiveChanged(bool value);
    void isDockedChanged(bool value);
    void menuChanged(QString value);
    void iconChanged(QString value);
    void nameChanged(QString value);
    void desktopFileChanged(QString value);
    void currentWindowChanged(uint32_t value);
    void windowInfosChanged(const WindowInfoMap &value);
    void modeChanged(int);

private:
    // 右键菜单项
    QVector<AppMenuItem> getMenuItemDesktopActions();
    AppMenuItem getMenuItemLaunch();
    AppMenuItem getMenuItemCloseAll();
    AppMenuItem getMenuItemForceQuit();
    AppMenuItem getMenuItemForceQuitAndroid();
    AppMenuItem getMenuItemDock();
    AppMenuItem getMenuItemUndock();
    AppMenuItem getMenuItemAllWindows();
    bool killProcess(int pid);
    bool setPropDesktopFile(QString value);
    bool isShowOnDock() const;
    int getCurrentMode();

private:
    Dock *dock;
    AppInfo *app;
    AppMenu *menu;

    bool isActive;
    bool isDocked;

    QString id;
    QString name;
    QString icon;
    QString innerId;
    QString desktopFile;

    // Dbus属性直接放到interface上
    QMap<XWindow, WindowInfoBase *> m_windowInfoMap; // 该应用所有窗口
    WindowInfoMap m_exportWindowInfos;    // 该应用导出的窗口属性
    WindowInfoBase *m_current; // 当前窗口
    XWindow m_currentWindow; //当前窗口Id
    bool m_winIconPreferred;
    int m_mode;
};

#endif // ENTRY_H
