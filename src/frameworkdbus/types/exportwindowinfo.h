#ifndef EXPORTWINDOWINFO_H
#define EXPORTWINDOWINFO_H

#include <QRect>
#include <QDBusMetaType>

struct ExportWindowInfo
{
public:
    ExportWindowInfo();
    ExportWindowInfo(uint32_t xid, const QString &title, bool flash);

    friend QDebug operator<<(QDebug debug, const ExportWindowInfo &rect);
    friend const QDBusArgument &operator>>(const QDBusArgument &arg, ExportWindowInfo &rect);
    friend QDBusArgument &operator<<(QDBusArgument &arg, const ExportWindowInfo &rect);

    uint32_t getXid() {return m_xid;}
    QString getTitle() {return m_title;}
    bool getFlash() {return m_flash;}

private:
    uint32_t m_xid;
    QString m_title;
    bool m_flash;
};

Q_DECLARE_METATYPE(ExportWindowInfo)

void registerExportWindowInfoMetaType();

#endif // EXPORTWINDOWINFO_H
