#include "exportwindowinfolist.h"

void sortExprotWindowInfoList(ExportWindowInfoList &list)
{
    qSort(list.begin(), list.end(), compareWindowXid);
}

void registerExportWindowInfoListMetaType()
{
    qRegisterMetaType<ExportWindowInfoList>("ExportWindowInfoList");
    qDBusRegisterMetaType<ExportWindowInfoList>();
}

// 按xid进行排序
bool compareWindowXid(ExportWindowInfo &info1, ExportWindowInfo &info2)
{
    return info1.getXid() < info2.getXid();
}
