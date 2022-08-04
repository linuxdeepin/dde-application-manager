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

#include "dbusadaptorentry.h"

DBusAdaptorEntry::DBusAdaptorEntry(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{

    // constructor
    setAutoRelaySignals(true);

    if (QMetaType::type("WindowInfo") == QMetaType::UnknownType)
        registerWindowInfoMetaType();

    if (QMetaType::type("WindowInfoMap") == QMetaType::UnknownType)
        registerWindowInfoMapMetaType();

    Entry *entry = static_cast<Entry *>(QObject::parent());
    if (entry) {
        connect(entry, &Entry::isActiveChanged, this, &DBusAdaptorEntry::IsActiveChanged);
        connect(entry, &Entry::isDockedChanged, this, &DBusAdaptorEntry::IsDockedChanged);
        connect(entry, &Entry::menuChanged, this, &DBusAdaptorEntry::MenuChanged);
        connect(entry, &Entry::iconChanged, this, &DBusAdaptorEntry::IconChanged);
        connect(entry, &Entry::nameChanged, this, &DBusAdaptorEntry::NameChanged);
        connect(entry, &Entry::desktopFileChanged, this, &DBusAdaptorEntry::DesktopFileChanged);
        connect(entry, &Entry::currentWindowChanged, this, &DBusAdaptorEntry::CurrentWindowChanged);
        connect(entry, &Entry::windowInfosChanged, this, &DBusAdaptorEntry::WindowInfosChanged);
        connect(entry, &Entry::modeChanged, this, &DBusAdaptorEntry::ModeChanged);
    }
}

DBusAdaptorEntry::~DBusAdaptorEntry()
{
}

uint DBusAdaptorEntry::currentWindow() const
{
    return parent()->getCurrentWindow();
}

QString DBusAdaptorEntry::desktopFile() const
{
    return parent()->getDesktopFile();
}

QString DBusAdaptorEntry::icon() const
{
    return parent()->getIcon();
}

QString DBusAdaptorEntry::id() const
{
   return parent()->getId();
}

bool DBusAdaptorEntry::isActive() const
{
    return parent()->getIsActive();
}

bool DBusAdaptorEntry::isDocked() const
{
    return parent()->getIsDocked();
}

int DBusAdaptorEntry::mode() const
{
    return parent()->mode();
}

QString DBusAdaptorEntry::menu() const
{
    return parent()->getMenu();
}

QString DBusAdaptorEntry::name() const
{
    return parent()->getName();
}

WindowInfoMap DBusAdaptorEntry::windowInfos()
{
    return parent()->getExportWindowInfos();
}

Entry *DBusAdaptorEntry::parent() const
{
    return static_cast<Entry *>(QObject::parent());
}

void DBusAdaptorEntry::Activate(uint timestamp)
{
    parent()->active(timestamp);
}

void DBusAdaptorEntry::Check()
{
    parent()->check();
}

void DBusAdaptorEntry::ForceQuit()
{
    parent()->forceQuit();
}

QList<QVariant> DBusAdaptorEntry::GetAllowedCloseWindows()
{
    auto ids = parent()->getAllowedClosedWindowIds();
    QList<QVariant> ret;
    for (auto id : ids)
        ret.push_back(id);

    return ret;
}

void DBusAdaptorEntry::HandleDragDrop(uint timestamp, const QStringList &files)
{
    parent()->handleDragDrop(timestamp, files);
}

void DBusAdaptorEntry::HandleMenuItem(uint timestamp, const QString &id)
{
    parent()->handleMenuItem(timestamp, id);
}

void DBusAdaptorEntry::NewInstance(uint timestamp)
{
    parent()->newInstance(timestamp);
}

void DBusAdaptorEntry::PresentWindows()
{
    parent()->presentWindows();
}

void DBusAdaptorEntry::RequestDock()
{
    parent()->requestDock();
}

void DBusAdaptorEntry::RequestUndock()
{
    parent()->requestUndock();
}

