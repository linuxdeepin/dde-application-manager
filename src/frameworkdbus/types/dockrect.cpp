// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dockrect.h"
#include <QDebug>

DockRect::DockRect()
    : X(0)
    , Y(0)
    , Width(0)
    , Height(0)
{

}

QDebug operator<<(QDebug debug, const DockRect &rect)
{
    debug << QString("DockRect(%1, %2, %3, %4)").arg(rect.X)
                                                .arg(rect.Y)
                                                .arg(rect.Width)
                                                .arg(rect.Height);

    return debug;
}

DockRect::operator QRect() const
{
    return QRect(X, Y, Width, Height);
}

QDBusArgument &operator<<(QDBusArgument &arg, const DockRect &rect)
{
    arg.beginStructure();
    arg << rect.X << rect.Y << rect.Width << rect.Height;
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, DockRect &rect)
{
    arg.beginStructure();
    arg >> rect.X >> rect.Y >> rect.Width >> rect.Height;
    arg.endStructure();

    return arg;
}

void registerRectMetaType()
{
    qRegisterMetaType<DockRect>("DockRect");
    qDBusRegisterMetaType<DockRect>();
}
