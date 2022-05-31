/*
 * Copyright (C) 2021 ~ 2022 Deepin Technology Co., Ltd.
 *
 * Author:     weizhixiang <weizhixiang@uniontech.com>
 *
 * Maintainer: weizhixiang <weizhixiang@uniontech.com>
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

#ifndef GSETTING_H
#define GSETTING_H

#include <string>
#include <gio/gio.h>

class GSetting
{
public:
    explicit GSetting(std::string settings);
    ~GSetting();

    std::string getString(std::string key);
    void setString(std::string key, std::string value);

    int getInt(std::string key);
    void setInt(std::string key, int value);

    double getDouble(std::string key);
    void setDouble(std::string key, double value);

    bool getBool(std::string key);
    void setBool(std::string key, bool value);

private:
    GSettings *m_gs;
    std::string m_settings;
};

#endif // GSETTING_H
