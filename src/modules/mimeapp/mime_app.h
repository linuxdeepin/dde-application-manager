// SPDX-FileCopyrightText: 2018 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MIME_APP_H
#define MIME_APP_H

#include "../../lib/dfile.h"

#include <QObject>


class MimeAppPrivate;
class MimeApp : public QObject
{
    Q_OBJECT
    QScopedPointer<MimeAppPrivate> dd_ptr;
    Q_DECLARE_PRIVATE_D(qGetPtrHelper(dd_ptr), MimeApp)
public:
    explicit MimeApp(QObject *parent = nullptr);
    void deleteMimeAssociation(std::string mimeType, std::string desktopId);
    void initConfigData();
    bool setDefaultApp(const std::string &mimeType, const  std::string &desktopId);
    std::string findFilePath(std::string fileName);
    ~MimeApp() override;

Q_SIGNALS:
    // Standard Notifications dbus implementation
    void Change();
public: // PROPERTIES

public Q_SLOTS: // METHODS
    void AddUserApp(QStringList mimeTypes, const QString &desktopId);
    void DeleteApp(QStringList mimeTypes, const QString &desktopId);
    void DeleteUserApp(const QString &desktopId);
    QString GetDefaultApp(const QString &mimeType);
    QString ListApps(const QString &mimeType);
    QString ListUserApps(const QString &mimeType);
    void SetDefaultApp(const QStringList &mimeTypes, const QString &desktopId);
private:

};

#endif // MIME_APP_H
