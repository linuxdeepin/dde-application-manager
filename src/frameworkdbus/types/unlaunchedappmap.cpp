#include "unlaunchedappmap.h"

void registerUnLaunchedAppMapMetaType()
{
    qRegisterMetaType<UnLaunchedAppMap>("UnLaunchedAppMap");
    qDBusRegisterMetaType<UnLaunchedAppMap>();
}
