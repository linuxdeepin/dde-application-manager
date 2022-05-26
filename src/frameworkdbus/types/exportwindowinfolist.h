#ifndef EXPORTWINDOWINFOLIST_H
#define EXPORTWINDOWINFOLIST_H
#include "exportwindowinfo.h"

#include <QtCore/QList>
#include <QDBusMetaType>

typedef QList<ExportWindowInfo> ExportWindowInfoList;

bool compareWindowXid(ExportWindowInfo &info1, ExportWindowInfo &info2);

void sortExprotWindowInfoList(ExportWindowInfoList &list);

Q_DECLARE_METATYPE(ExportWindowInfoList)

void registerExportWindowInfoListMetaType();

#endif // EXPORTWINDOWINFOLIST_H
