// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <pwd.h>
#include <QDir>
#include <vector>

std::string getUserHomeDir();

std::string getUserDataDir();

std::string getUserConfigDir();

std::string getUserDir(const char* envName);

std::vector<std::string> getSystemDataDirs();

std::vector<std::string> getSystemConfigDirs();

std::vector<std::string> getSystemDirs(const char* envName);

std::string lookPath(std::string file);

void walk(std::string root, std::vector<std::string>& skipdir, std::map<std::string, int>& retMap);

void walk(std::string root, std::string name, std::vector<std::string>& skipdir, std::map<std::string, int>& retMap);

bool hasEnding(std::string const& fullString, std::string const& ending);

bool hasBeginWith(std::string const& fullString, std::string const& ending);
#endif // UTILS_H

