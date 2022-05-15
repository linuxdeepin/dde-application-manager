#ifndef A2862DC7_5DA3_4129_9796_671D88015BED
#define A2862DC7_5DA3_4129_9796_671D88015BED

#include "../../modules/startmanager/startmanager.h"

#include <QObject>
#include <QDBusObjectPath>
#include <QList>
#include <QMap>
#include <QDBusContext>

class Application;
class ApplicationInstance;
class ApplicationManagerPrivate;
class ApplicationManager : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_PROPERTY(QList<QDBusObjectPath> instances READ instances)
    Q_PROPERTY(QList<QDBusObjectPath> list READ list)
    QScopedPointer<ApplicationManagerPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), ApplicationManager)

    ApplicationManager(QObject *parent = nullptr);
    bool checkDMsgUid();

    StartManager *startManager;

public:
    ~ApplicationManager() override;
    static ApplicationManager* Instance() {
        static ApplicationManager manager;
        return &manager;
    }

    void addApplication(const QList<QSharedPointer<Application>> &list);

Q_SIGNALS:
    void AutostartChanged(QString status, QString filePath);

public: // PROPERTIES
    QList<QDBusObjectPath> instances() const;
    QList<QDBusObjectPath> list() const;

public Q_SLOTS: // METHODS
    QDBusObjectPath GetInformation(const QString &id);
    QList<QDBusObjectPath> GetInstances(const QString &id);
    QDBusObjectPath Run(const QString &id);

    // com.deepin.StartManager
    bool AddAutostart(QString fileName);
    bool RemoveAutostart(QString fileName);
    QStringList AutostartList();
    QString DumpMemRecord();
    //QString GetApps();
    bool IsAutostart(QString fileName);
    bool IsMemSufficient();
    //bool Launch(QString desktopFile); deprecated
    void LaunchApp(QString desktopFile, uint32_t timestamp, QStringList files);
    void LaunchAppAction(QString desktopFile, QString action, uint32_t timestamp);
    void LaunchAppWithOptions(QString desktopFile, uint32_t timestamp, QStringList files, QMap<QString, QString> options);
    //bool LaunchWithTimestamp(QString desktopFile, uint32_t timestamp); deprecated
    void RunCommand(QString exe, QStringList args);
    void RunCommandWithOptions(QString exe, QStringList args, QMap<QString, QString> options);
    void TryAgain(bool launch);
};

#endif /* A2862DC7_5DA3_4129_9796_671D88015BED */
