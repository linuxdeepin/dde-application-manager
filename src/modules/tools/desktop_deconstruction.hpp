// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BB9B5BB3_BEAF_4D25_B4F6_55273B263973
#define BB9B5BB3_BEAF_4D25_B4F6_55273B263973

#ifdef USE_QT

#include <QSettings>
#include <QTextStream>

static QSettings DesktopDeconstruction(const QString &desktop)
{
    auto IniReadFunc = [](QIODevice &device, QSettings::SettingsMap &settingsMap) -> bool {
        QTextStream stream(&device);
        QString     group;
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (group.isEmpty()) {
                if (line.front() == '[' && line.back() == ']') {
                    group = line.mid(1, line.size() - 2);
                }
            }
            else {
                if (line.isEmpty()) {
                    group.clear();
                }
                else {
                    int index = line.indexOf("=");
                    if (index != -1) {
                        QString name = group + "/" + line.mid(0, index);
                        QVariant value = QVariant(line.mid(index + 1));
                        settingsMap.insert(name, value);
                    }
                }
            }
        }
        return true;
    };

    return QSettings(desktop, QSettings::registerFormat("ini", IniReadFunc, nullptr));
}

#else

#include <algorithm>
#include <any>
#include <fstream>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <variant>
#include <vector>

class DesktopDeconstruction
{
    std::string m_path;
    std::string m_group;

    struct Entry {
        typedef std::string Key;
        typedef std::any Value;
        std::string name;
        std::vector<std::pair<Key, Value>> pairs;
    };

    std::vector<std::shared_ptr<Entry>> _parse()
    {
        std::ifstream file(m_path);
        if (!file) {
            return {};
        }

        std::string line;
        std::regex group_regex("\\[(.*?)\\]");
        std::vector<std::shared_ptr<struct Entry>> entrys;
        std::shared_ptr<Entry> currentEntry;
        while (std::getline(file, line)) {
            // 匹配 group，如果是，就进入判断
            if (std::regex_match(line, group_regex)) {
                std::smatch result;
                if (std::regex_search(line, result, group_regex)) {
                    currentEntry = std::make_shared<Entry>();
                    currentEntry.get()->name = result.str();
                    entrys.push_back(currentEntry);
                }
                continue;
            }
            if (!currentEntry) {
                continue;
            }

            // 跳过 # 开头的注释
            if (!line.empty() && line[0] == '#') {
                continue;
            }

            const size_t index = line.find('=');
            const std::string key = line.substr(0, index);
            const std::any value = line.substr(index + 1, line.size());
            currentEntry.get()->pairs.push_back({key, value});
        }
        file.close();

        return entrys;
    }

public:
    DesktopDeconstruction(const std::string &path)
        : m_path(path)
    {
    }
    ~DesktopDeconstruction()
    {
    }

    void beginGroup(const std::string group)
    {
        m_group = group;
    }

    void endGroup()
    {
        m_group.clear();
    }

    std::vector<std::string> listKeys()
    {
        std::vector<std::shared_ptr<Entry>> entrys{ _parse() };
        std::vector<std::string> result;
        for (const std::shared_ptr<Entry> entry : entrys) {
            for (const auto pair : entry.get()->pairs) {
                result.push_back(pair.first);
            }
        }
        return result;
    }

    template <typename T>
    T value(const std::string key)
    {
        std::vector<std::shared_ptr<Entry>> entrys{ _parse() };
        for (const std::shared_ptr<Entry> entry : entrys) {
            if (entry.get()->name == "[" + m_group + "]") {
                for (const auto pair : entry.get()->pairs) {
                    if (pair.first == key) {
                        return std::any_cast<T>(pair.second);
                    }
                }
            }
        }
        return {};
    }
};

#endif

#endif /* BB9B5BB3_BEAF_4D25_B4F6_55273B263973 */
