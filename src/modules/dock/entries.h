// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ENTRIES_H
#define ENTRIES_H

#include "entry.h"
#include "docksettings.h"

#include <QVector>
#include <QWeakPointer>

#define MAX_UNOPEN_RECENT_COUNT 3

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
    void updateEntriesMenu();
    const QList<Entry *> unDockedEntries() const;
    void moveEntryToLast(Entry *entry);
    bool shouldInRecent();
    void removeLastRecent();
    void setDisplayMode(DisplayMode displayMode);
    void updateShowRecent();

private:
    void insertCb(Entry *entry, int index);
    void removeCb(Entry *entry);

private:
    QList<Entry *> m_items;
    Dock *m_dock;
};

#endif // ENTRIES_H
