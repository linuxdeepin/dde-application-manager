#include "exportwindowinfo.h"

#include <QtDebug>

ExportWindowInfo::ExportWindowInfo()
 : m_xid(0)
 , m_flash(false)
{

}

ExportWindowInfo::ExportWindowInfo(uint32_t xid, const QString &title, bool flash)
 : m_xid(xid)
 , m_title(title)
 , m_flash(flash)
{

}

QDebug operator<<(QDebug debug, const ExportWindowInfo &info)
{
    debug << QString("ExportWindowInfo(%1, %2, %3)").arg(info.m_xid)
                                                .arg(info.m_title)
                                                .arg(info.m_flash);

    return debug;
}

QDBusArgument &operator<<(QDBusArgument &arg, const ExportWindowInfo &info)
{
    arg.beginStructure();
    arg << info.m_xid << info.m_title << info.m_flash;
    arg.endStructure();

    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, ExportWindowInfo &info)
{
    arg.beginStructure();
    arg >> info.m_xid >> info.m_title >> info.m_flash;
    arg.endStructure();

    return arg;
}

void registerExportWindowInfoMetaType()
{
    qRegisterMetaType<ExportWindowInfo>("ExportWindowInfo");
    qDBusRegisterMetaType<ExportWindowInfo>();
}
