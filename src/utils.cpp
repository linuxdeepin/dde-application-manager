#include "global.h"

bool registerObjectToDbus(QObject *o, const QString &path, const QString &interface)
{
    auto &con = ApplicationManager1DBus::instance().CustomBus();
    if (!con.registerObject(path, interface, o)) {
        qCritical() << "register object failed:" << path << interface << con.lastError();
        return false;
    }
    return true;
}
