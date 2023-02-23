// SPDX-FileCopyrightText: 2015 Jolla Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef QT_DBUS_EXTENDED_H
#define QT_DBUS_EXTENDED_H

#if defined(QT_DBUS_EXTENDED_LIBRARY)
#  define QT_DBUS_EXTENDED_EXPORT Q_DECL_EXPORT
#else
#  define QT_DBUS_EXTENDED_EXPORT Q_DECL_IMPORT
#endif

#endif /* QT_DBUS_EXTENDED_H */
