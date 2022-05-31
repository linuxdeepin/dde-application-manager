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

#include "entries.h"
#include "dock.h"

Entries::Entries(Dock *_dock)
 : dock(_dock)
{

}

QVector<Entry *> Entries::filterDockedEntries()
{
    QVector<Entry *> ret;
    for (auto &entry : items) {
        if (entry->isValid() && entry->getIsDocked())
            ret.push_back(entry);
    }

    return ret;
}

Entry *Entries::getByInnerId(QString innerId)
{
    Entry *ret = nullptr;
    for (auto &entry : items) {
        if (entry->getInnerId() == innerId)
            ret = entry;
    }

    return ret;
}

void Entries::append(Entry *entry)
{
    insert(entry, -1);
}

void Entries::insert(Entry *entry, int index)
{
    if (index < 0 || index >= items.size()) {
        // append
        index = items.size();
        items.push_back(entry);
    } else {
        // insert
        items.insert(index, entry);
    }

    insertCb(entry, index);
}

void Entries::remove(Entry *entry)
{
    for (auto iter = items.begin(); iter != items.end();) {
        if ((*iter)->getId() == entry->getId()) {
            iter = items.erase(iter);
            removeCb(entry);
            delete entry;
        } else {
            iter++;
        }
    }
}

void Entries::move(int oldIndex, int newIndex)
{
    if (oldIndex == newIndex || oldIndex < 0 || newIndex < 0 || oldIndex >= items.size() || newIndex >= items.size())
        return;

    items.swap(oldIndex, newIndex);
}

Entry *Entries::getByWindowPid(int pid)
{
    Entry *ret = nullptr;
    for (auto &entry : items) {
        if (entry->getWindowInfoByPid(pid)) {
            ret = entry;
            break;
         }
    }

    return ret;
}

Entry *Entries::getByWindowId(XWindow windowId)
{
    Entry *ret = nullptr;
    for (auto &entry : items) {
        if (entry->getWindowInfoByWinId(windowId)) {
            ret = entry;
            break;
         }
    }

    return ret;
}

Entry *Entries::getByDesktopFilePath(QString filePath)
{
    Entry *ret = nullptr;
    for (auto &entry : items) {
        if (entry->getFileName() == filePath) {
            ret = entry;
            break;
         }
    }

    return ret;
}

QStringList Entries::getEntryIDs()
{
    QStringList list;
    for (auto item : items)
        list.push_back(item->getId());

    return list;
}

Entry *Entries::getDockedEntryByDesktopFile(const QString &desktopFile)
{
    Entry *ret = nullptr;
    for (auto entry : filterDockedEntries()) {
        if (!entry->isValid())
            continue;

        if (desktopFile == entry->getFileName()) {
            ret = entry;
            break;
        }
    }

    return ret;
}

QString Entries::queryWindowIdentifyMethod(XWindow windowId)
{
    QString ret;
    for (auto entry : items) {
        auto window = entry->getWindowInfoByWinId(windowId);
        if (window) {
            auto app = window->getAppInfo();
            if (app) {
                ret = app->getIdentifyMethod();
            } else {
                    ret = "Failed";
            }
            break;
        }
    }

    return ret;
}

void Entries::handleActiveWindowChanged(XWindow activeWindId)
{
    for (auto entry : items) {
        auto windowInfo = entry->getWindowInfoByWinId(activeWindId);
        if (windowInfo) {
            entry->setPropIsActive(true);
            entry->setCurrentWindowInfo(windowInfo);
            entry->updateName();
            entry->updateIcon();
        } else {
            entry->setPropIsActive(false);
        }
    }
}

void Entries::deleteWindow(XWindow xid)
{
    for (auto entry : items) {
        if (entry->containsWindow(xid)) {
            entry->deleteWindow(xid);
            break;
        }
    }
}

void Entries::updateEntriesMenu()
{
    for (auto entry : items) {
        entry->updateMenu();
    }
}

void Entries::insertCb(Entry *entry, int index)
{
    QString objPath = entryDBusObjPathPrefix + entry->getId();
    Q_EMIT dock->entryAdded(QDBusObjectPath(objPath), index);
}

void Entries::removeCb(Entry *entry)
{
    Q_EMIT dock->entryRemoved(entry->getId());
    entry->stopExport();
}

