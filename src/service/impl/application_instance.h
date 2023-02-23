// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef D6D05668_8A58_43AA_91C5_C6278643A1AF
#define D6D05668_8A58_43AA_91C5_C6278643A1AF

#include <QDBusObjectPath>
#include <QObject>

#include "../../modules/methods/task.hpp"

namespace modules {
namespace ApplicationHelper {
class Helper;
}
}  // namespace modules

class Application;
class ApplicationInstancePrivate;
class ApplicationInstance : public QObject {
    Q_OBJECT
    QScopedPointer<ApplicationInstancePrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), ApplicationInstance)

    QStringList m_files; // 实例打开的文件
public:
    ApplicationInstance(Application* parent, QSharedPointer<modules::ApplicationHelper::Helper> helper, QStringList files);
    ~ApplicationInstance() override;

public:  // PROPERTIES
    Q_PROPERTY(QDBusObjectPath id READ id)
    QDBusObjectPath id() const;

    Q_PROPERTY(quint64 startuptime READ startuptime)
    quint64 startuptime() const;

    QDBusObjectPath path() const;
    QString         hash() const;
    Methods::Task   taskInfo() const;

Q_SIGNALS:
    void taskFinished(int exitCode) const;

public Q_SLOTS:  // METHODS
    void Exit();
    void Kill();
    void Success(const QString& data);
    uint32_t getPid();
};

#endif /* D6D05668_8A58_43AA_91C5_C6278643A1AF */
