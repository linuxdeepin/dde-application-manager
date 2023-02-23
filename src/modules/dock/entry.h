// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
class DBusAdaptorEntry;

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
    void activeWindow(quint32 winId);
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
    Dock *m_dock;
    AppInfo *m_app;
    AppMenu *m_menu;

    bool m_isActive;
    bool m_isDocked;

    QString m_id;
    QString m_name;
    QString m_icon;
    QString m_innerId;
    QString m_desktopFile;

    DBusAdaptorEntry *m_adapterEntry;

    // Dbus属性直接放到interface上
    QMap<XWindow, WindowInfoBase *> m_windowInfoMap; // 该应用所有窗口
    WindowInfoMap m_exportWindowInfos;    // 该应用导出的窗口属性
    WindowInfoBase *m_current; // 当前窗口
    XWindow m_currentWindow; //当前窗口Id
    bool m_winIconPreferred;
    int m_mode;
};

#endif // ENTRY_H
