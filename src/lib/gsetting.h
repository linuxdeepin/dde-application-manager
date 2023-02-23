// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
