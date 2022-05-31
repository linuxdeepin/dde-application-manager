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

#include "gsetting.h"

#include <iostream>

GSetting::GSetting(std::string settings)
 : m_gs(nullptr)
 , m_settings(settings)
{
    m_gs = g_settings_new(m_settings.c_str());
    if (!m_gs) {
        std::cout << "GSetting: g_settings_new error" << std::endl;
    }
}

GSetting::~GSetting()
{
    if (m_gs) {
        g_object_unref(m_gs);
        m_gs = nullptr;
    }
}

std::string GSetting::getString(std::string key)
{
    std::string ret;
    if (m_gs) {
        ret = g_settings_get_string(m_gs, key.c_str());
    }
    return ret;
}

void GSetting::setString(std::string key, std::string value)
{
    if (m_gs) {
        g_settings_set_string(m_gs, key.c_str(), value.c_str());
    }
}

int GSetting::getInt(std::string key)
{
    int ret = 0;
    if (m_gs) {
        ret = g_settings_get_int(m_gs, key.c_str());
    }
    return ret;
}

void GSetting::setInt(std::string key, int value)
{
    if (m_gs) {
        g_settings_set_int(m_gs, key.c_str(), value);
    }
}

double GSetting::getDouble(std::string key)
{
    double ret = 0;
    if (m_gs) {
        ret = g_settings_get_double(m_gs, key.c_str());
    }
    return ret;
}

void GSetting::setDouble(std::string key, double value)
{
    if (m_gs) {
        g_settings_set_double(m_gs, key.c_str(), value);
    }
}

bool GSetting::getBool(std::string key)
{
    bool ret = false;
    if (m_gs) {
        ret = g_settings_get_boolean(m_gs, key.c_str());
    }
    return ret;
}

void GSetting::setBool(std::string key, bool value)
{
    if (m_gs) {
        g_settings_set_double(m_gs, key.c_str(), value);
    }
}
