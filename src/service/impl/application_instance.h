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
public:
    ApplicationInstance(Application* parent, QSharedPointer<modules::ApplicationHelper::Helper> helper);
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
};

#endif /* D6D05668_8A58_43AA_91C5_C6278643A1AF */
