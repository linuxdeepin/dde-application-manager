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

#ifndef WINDOWINFOBASE_H
#define WINDOWINFOBASE_H

#include "xcbutils.h"

#include <QString>
#include <QVector>

class Entry;
class AppInfo;
class ProcessInfo;
class WindowInfoBase
{
public:
    WindowInfoBase() : entry(nullptr), app(nullptr), processInfo(nullptr) {}
    virtual ~WindowInfoBase() {}


    virtual bool shouldSkip() = 0;
    virtual QString getIcon() = 0;
    virtual QString getTitle() = 0;
    virtual bool isDemandingAttention() = 0;
    virtual void close(uint32_t timestamp) = 0;
    virtual void activate() = 0;
    virtual void minimize() = 0;
    virtual bool isMinimized() = 0;
    virtual int64_t getCreatedTime() = 0;
    virtual QString getWindowType() = 0;
    virtual QString getDisplayName() = 0;
    virtual bool allowClose() = 0;
    virtual void update() = 0;
    virtual void killClient() = 0;

    XWindow getXid() {return xid;}
    void setEntry(Entry *value) {entry = value;}
    Entry *getEntry() {return entry;}
    QString getEntryInnerId() {return entryInnerId;}
    QString getInnerId() {return innerId;}
    void setEntryInnerId(QString value) {entryInnerId = value;}
    AppInfo *getAppInfo() {return app;}
    void setAppInfo(AppInfo *value) {app = value;}
    int getPid() {return pid;}
    ProcessInfo *getProcess() {return processInfo;}
    bool containAtom(QVector<XCBAtom> supports, XCBAtom ty) {return supports.indexOf(ty) != -1;}

protected:
    XWindow xid;            // 窗口id
    QString title;          // 窗口标题
    QString icon;           // 窗口图标
    int pid;                // 窗口所属应用进程
    QString entryInnerId;   // 窗口所属应用对应的innerId
    QString innerId;        // 窗口对应的innerId
    Entry *entry;           // 窗口所属应用
    AppInfo *app;           // 窗口所属应用对应的desktopFile信息
    ProcessInfo *processInfo;   // 窗口所属应用的进程信息
    int64_t createdTime;    // 创建时间
};

#endif // WINDOWINFOBASE_H
