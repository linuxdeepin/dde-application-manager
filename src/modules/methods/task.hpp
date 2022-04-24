#ifndef B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C
#define B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C


#include <utility>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

namespace Methods
{
    struct Task
    {
        QString id;
        QString runId;
        QString type{"task"};
        QString date;
        QList<QString> arguments;
        QMap<QString, QString> environments;
    };

    inline void toJson(QByteArray &array, const Task &task) {
        QJsonArray argArray;
        for (auto arg : task.arguments) {
            argArray.append(arg);
        }

        QVariantMap envsMap;
        for (auto it = task.environments.constBegin(); it != task.environments.constEnd(); ++it) {
            envsMap.insert(it.key(), it.value());
        }

        QJsonObject obj = {
            {"type", task.type},
            {"id", task.id},
            {"run_id", task.runId},
            {"date", task.date},
            {"arguments",  argArray},
            {"environments", QJsonObject::fromVariantMap(envsMap)}
        };

        array = QJsonDocument(obj).toJson();
    }

    inline void fromJson(const QByteArray &array, Task &task) {
        QJsonDocument doc = QJsonDocument::fromJson(array);
		if (!doc.isObject()) {
			qWarning() << "fromJson task failed";
			return;
		}

        QJsonObject obj = doc.object();
        if (!obj.contains("id") || !obj.contains("runId") || !obj.contains("date") \
            || !obj.contains("arguments") || !obj.contains("environments")) {
            qWarning() << "id runId date arguments environments not exist in task array";
            return;
        }

        task.id = obj.value("id").toString();
        task.runId = obj.value("runId").toString();
        task.date = obj.value("date").toString();
        for (auto arg : obj.value("arguments").toArray()) {
            task.arguments.append(arg.toString());
        }
        
        QVariantMap envsMap = obj.value("environments").toObject().toVariantMap();
        for (auto it = envsMap.constBegin(); it != envsMap.constEnd(); ++it) {
            task.environments.insert(it.key(), it.value().toString());
        }
    }
} // namespace Methods

#endif /* B0B88BD6_CF1E_4E87_926A_E6DBE6B9B19C */
