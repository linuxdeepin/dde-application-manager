// SPDX-FileCopyrightText: 2022 ~ 2022 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DESKTOPFILEREADER_H
#define DESKTOPFILEREADER_H

#include <QObject>
#include <QMap>

struct BamfData {
    QString directory;
    QString instanceName;
    QString lineData;
};

class DesktopFileReader
{
public:
    static DesktopFileReader *instance();
    QString fileName(const QString &instanceName) const;

protected:
    DesktopFileReader();
    ~DesktopFileReader();

private:
    QStringList applicationDirs() const;
    void loadDesktopFiles();

private:
    QList<BamfData> m_bamfLineData;
};

#endif // DESKTOPFILEREADER_H
