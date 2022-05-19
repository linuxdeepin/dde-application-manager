#ifndef TYPES_H
#define TYPES_H

#include <QMap>
#include <QStringList>
#include <QDBusMetaType>

typedef QMap<QString, QStringList> UnLaunchedAppMap;

Q_DECLARE_METATYPE(UnLaunchedAppMap)

void registerUnLaunchedAppMapMetaType();

#endif // TYPES_H
