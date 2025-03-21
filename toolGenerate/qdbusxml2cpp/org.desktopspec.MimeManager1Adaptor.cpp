/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp ./dde-application-manager/api/dbus/org.desktopspec.MimeManager1.xml -a ./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.MimeManager1Adaptor -i ./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.MimeManager1.h
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#include "./dde-application-manager/toolGenerate/qdbusxml2cpp/org.desktopspec.MimeManager1Adaptor.h"
#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

/*
 * Implementation of adaptor class MimeManager1Adaptor
 */

MimeManager1Adaptor::MimeManager1Adaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    // constructor
    setAutoRelaySignals(true);
}

MimeManager1Adaptor::~MimeManager1Adaptor()
{
    // destructor
}

ObjectMap MimeManager1Adaptor::listApplications(const QString &mimeType)
{
    // handle method call org.desktopspec.MimeManager1.listApplications
    ObjectMap applications_and_properties;
    QMetaObject::invokeMethod(parent(), "listApplications", Q_RETURN_ARG(ObjectMap, applications_and_properties), Q_ARG(QString, mimeType));
    return applications_and_properties;
}

QString MimeManager1Adaptor::queryDefaultApplication(const QString &content, QDBusObjectPath &application)
{
    // handle method call org.desktopspec.MimeManager1.queryDefaultApplication
    //return static_cast<YourObjectType *>(parent())->queryDefaultApplication(content, application);
}

void MimeManager1Adaptor::setDefaultApplication(const QStringMap &defaultApps)
{
    // handle method call org.desktopspec.MimeManager1.setDefaultApplication
    QMetaObject::invokeMethod(parent(), "setDefaultApplication", Q_ARG(QStringMap, defaultApps));
}

void MimeManager1Adaptor::unsetDefaultApplication(const QStringList &mimeTypes)
{
    // handle method call org.desktopspec.MimeManager1.unsetDefaultApplication
    QMetaObject::invokeMethod(parent(), "unsetDefaultApplication", Q_ARG(QStringList, mimeTypes));
}

