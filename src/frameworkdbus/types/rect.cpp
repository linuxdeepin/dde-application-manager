#include "rect.h"
#include <QDebug>

Rect::Rect()
    : X(0)
    , Y(0)
    , Width(0)
    , Height(0)
{

}

QDebug operator<<(QDebug debug, const Rect &rect)
{
    debug << QString("Rect(%1, %2, %3, %4)").arg(rect.X)
                                                .arg(rect.Y)
                                                .arg(rect.Width)
                                                .arg(rect.Height);

    return debug;
}

Rect::operator QRect() const
{
    return QRect(X, Y, Width, Height);
}

QDBusArgument &operator<<(QDBusArgument &arg, const Rect &rect)
{
    arg.beginStructure();
    arg << rect.X << rect.Y << rect.Width << rect.Height;
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Rect &rect)
{
    arg.beginStructure();
    arg >> rect.X >> rect.Y >> rect.Width >> rect.Height;
    arg.endStructure();

    return arg;
}

void registerRectMetaType()
{
    qRegisterMetaType<Rect>("Rect");
    qDBusRegisterMetaType<Rect>();
}
