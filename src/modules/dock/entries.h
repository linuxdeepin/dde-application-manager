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

#ifndef ENTRIES_H
#define ENTRIES_H

#include "entry.h"
#include <QVector>
#include <QWeakPointer>

class Dock;

// 所有应用管理类
class Entries
{
public:
    Entries(Dock *_dock);

    QVector<Entry *> filterDockedEntries();
    Entry *getByInnerId(QString innerId);
    void append(Entry *entry);
    void insert(Entry *entry, int index);
    void remove(Entry *entry);
    void move(int oldIndex, int newIndex);
    Entry *getByWindowPid(int pid);
    Entry *getByWindowId(XWindow windowId);
    Entry *getByDesktopFilePath(QString filePath);
    QStringList getEntryIDs();
    Entry *getDockedEntryByDesktopFile(const QString &desktopFile);
    QString queryWindowIdentifyMethod(XWindow windowId);
    void handleActiveWindowChanged(XWindow activeWindId);
    void deleteWindow(XWindow xid);
    void updateEntriesMenu();
    void insertCb(Entry *entry, int index);
    void removeCb(Entry *entry);

private:
    QList<Entry *> items;
    Dock *dock;
};

#endif // ENTRIES_H
