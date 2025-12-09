// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef PRELAUNCHSPLASHHELPER_H
#define PRELAUNCHSPLASHHELPER_H

#include <QObject>
#include <QtWaylandClient/QWaylandClientExtension>
#include <QIcon>
#include <QLoggingCategory>
#include <QPixmap>
#include <memory>

#include "qwayland-treeland-prelaunch-splash-v1.h"
// Wayland C types
struct wl_buffer;
namespace QtWaylandClient {
class QWaylandShmBuffer;
}

Q_DECLARE_LOGGING_CATEGORY(amPrelaunchSplash)
class PrelaunchSplashHelper
    : public QWaylandClientExtensionTemplate<PrelaunchSplashHelper>
    , public QtWayland::treeland_prelaunch_splash_manager_v1
{
    Q_OBJECT
public:
    explicit PrelaunchSplashHelper();
    ~PrelaunchSplashHelper();
    void show(const QString &appId, const QString &iconName);

private:
    wl_buffer *buildIconBuffer(const QIcon &icon);
    wl_buffer *createBufferFromPixmap(const QPixmap &pixmap);
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // PRELAUNCHSPLASHHELPER_H
