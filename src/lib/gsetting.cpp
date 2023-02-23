// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
