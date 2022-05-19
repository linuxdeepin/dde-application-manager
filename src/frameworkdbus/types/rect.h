#ifndef DOCKRECT_H
#define DOCKRECT_H

#include <QRect>
#include <QDBusMetaType>

struct Rect
{
public:
    Rect();
    operator QRect() const;

    friend QDebug operator<<(QDebug debug, const Rect &rect);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, Rect &rect);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const Rect &rect);

    qint32 X, Y, Width, Height;
};

Q_DECLARE_METATYPE(Rect)

void registerRectMetaType();

#endif // DOCKRECT_H
