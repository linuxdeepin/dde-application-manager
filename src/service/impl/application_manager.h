#ifndef A2862DC7_5DA3_4129_9796_671D88015BED
#define A2862DC7_5DA3_4129_9796_671D88015BED

#include <QObject>
#include <QDBusObjectPath>
#include <QList>

class Application;
class ApplicationInstance;
class ApplicationManagerPrivate;
class ApplicationManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QDBusObjectPath> instances READ instances)
    Q_PROPERTY(QList<QDBusObjectPath> list READ list)
    QScopedPointer<ApplicationManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), ApplicationManager)

    ApplicationManager(QObject *parent = nullptr);

public:
    ~ApplicationManager() override;
    static ApplicationManager* Instance() {
        static ApplicationManager manager;
        return &manager;
    }

    void addApplication(const QList<QSharedPointer<Application>> &list);

signals:
    void requestCreateInstance(const QSharedPointer<ApplicationInstance> instance);

public: // PROPERTIES
    QList<QDBusObjectPath> instances() const;
    QList<QDBusObjectPath> list() const;

public Q_SLOTS: // METHODS
    QDBusObjectPath GetId(int pid);
    QDBusObjectPath GetInformation(const QString &id);
    QList<QDBusObjectPath> GetInstances(const QString &id);
    QDBusObjectPath Run(const QString &id);
};

#endif /* A2862DC7_5DA3_4129_9796_671D88015BED */
