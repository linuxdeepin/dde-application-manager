/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp ./dde-application-manager/api/dbus/org.desktopspec.ApplicationManager1.xml -a ./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.ApplicationManager1Adaptor -i ./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.ApplicationManager1.h
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef ORG_DESKTOPSPEC_APPLICATIONMANAGER1ADAPTOR_H
#define ORG_DESKTOPSPEC_APPLICATIONMANAGER1ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.ApplicationManager1.h"
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface org.desktopspec.ApplicationManager1
 */
class ApplicationManager1Adaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.desktopspec.ApplicationManager1")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.desktopspec.ApplicationManager1\">\n"
"    <property access=\"read\" type=\"ao\" name=\"List\"/>\n"
"    <method name=\"ReloadApplications\">\n"
"      <annotation value=\"This method is used to update the desktop file cache when needed.                        You can pass a list of absolute paths to files or directories that you want to update.                        Note: Application Manager only supports directory paths in $XDG_DATA_DIRS.\" name=\"org.freedesktop.DBus.Description\"/>\n"
"    </method>\n"
"    <method name=\"Identify\">\n"
"      <arg direction=\"in\" type=\"h\" name=\"pidfd\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"id\"/>\n"
"      <arg direction=\"out\" type=\"o\" name=\"instance\"/>\n"
"      <arg direction=\"out\" type=\"a{sa{sv}}\" name=\"application_instance_info\"/>\n"
"      <annotation value=\"ObjectInterfaceMap\" name=\"org.qtproject.QtDBus.QtTypeName.Out2\"/>\n"
"      <annotation value=\"Given a pidfd,                        this method return a destkop file id,                        an application instance object path,                        as well as an application object path.                         NOTE:                        1. You should use pidfd_open(2) to get a pidfd.\" name=\"org.freedesktop.DBus.Description\"/>\n"
"    </method>\n"
"    <method name=\"addUserApplication\">\n"
"      <arg direction=\"in\" type=\"a{sv}\" name=\"desktop_file\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\"/>\n"
"      <arg direction=\"out\" type=\"s\" name=\"app_id\"/>\n"
"      <annotation value=\"QVariantMap\" name=\"org.qtproject.QtDBus.QtTypeName.In0\"/>\n"
"      <annotation value=\"Desktop-entry-spec: https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html,                        type of `v` is depends on the property which you want to set.                        examples:                        {'Name':{'en_US':'example','default':'测试'},{'custom':10}} : a{sa{sv}} // Name=测试 Name[en_US]=example custom=10                        {'custom':20,'Name':'example'} : a{sv} // custom=20 Name=example                       \" name=\"org.freedesktop.DBus.Description\"/>\n"
"    </method>\n"
"    <method name=\"deleteUserApplication\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"app_id\"/>\n"
"      <annotation value=\"Delete a user application by app_id.\" name=\"org.freedesktop.DBus.Description\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    ApplicationManager1Adaptor(QObject *parent);
    virtual ~ApplicationManager1Adaptor();

public: // PROPERTIES
    Q_PROPERTY(QList<QDBusObjectPath> List READ list)
    QList<QDBusObjectPath> list() const;

public Q_SLOTS: // METHODS
    QString Identify(const QDBusUnixFileDescriptor &pidfd, QDBusObjectPath &instance, ObjectInterfaceMap &application_instance_info);
    void ReloadApplications();
    QString addUserApplication(const QVariantMap &desktop_file, const QString &name);
    void deleteUserApplication(const QString &app_id);
Q_SIGNALS: // SIGNALS
};

#endif
