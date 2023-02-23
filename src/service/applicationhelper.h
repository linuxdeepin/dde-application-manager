// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "../modules/tools/desktop_deconstruction.hpp"

#include <QString>

namespace modules {
namespace ApplicationHelper {
class Helper {
    QString m_file;

public:
    Helper(const QString &desktop)
        : m_file(desktop)
    {

    }

    inline QString desktop() const { return m_file; }

    template <typename T>
    T value(const QString &key) const
    {
        QSettings settings = DesktopDeconstruction(m_file);
        settings.beginGroup("Desktop Entry");
        return settings.value(key).value<T>();
    }

    QStringList categories() const
    {
        QStringList result;
        QStringList tmp{ value<QString>("Categories").split(";") };
        for (auto t : tmp) {
            if (t.isEmpty()) {
                continue;
            }
            result << t;
        }
        return result;
    }

    QString icon() const
    {
        return value<QString>("Icon");
    }

    // appId
    QString id() const
    {
        return m_file.split("/").last().split(".").first();
    }

    QStringList mimetypes() const
    {
        QStringList result;
        QStringList tmp{ value<QString>("MimeType").split(";") };
        for (auto t : tmp) {
            if (t.isEmpty()) {
                continue;
            }
            result << t;
        }
        return result;
    }

    QString comment(const QString &locale) const
    {
        return value<QString>(QString("Comment[%1]").arg(locale));
    }

    QString name(const QString &name) const
    {
        return value<QString>(QString("Name[%1]").arg(name));
    }
};
}  // namespace ApplicationHelper
}  // namespace modules

