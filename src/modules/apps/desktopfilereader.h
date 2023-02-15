//SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
//SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (C) 2022 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     donghualin <donghualin@uniontech.com>
 *
 * Maintainer: donghualin <donghualin@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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
