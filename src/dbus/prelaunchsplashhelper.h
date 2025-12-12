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
#include <vector>

#include "qwayland-treeland-prelaunch-splash-v1.h"
// Wayland C types
struct wl_buffer;
namespace QtWaylandClient {
class QWaylandShmBuffer;
}

Q_DECLARE_LOGGING_CATEGORY(amPrelaunchSplash)

/**
 * @brief Helper for sending prelaunch splash requests over Wayland.
 *
 * Creates wl_buffers from application icons using shared memory buffers and
 * sends them to the compositor via treeland_prelaunch_splash_manager_v1.
 * Not thread-safe; use from the Qt GUI thread.
 */
class PrelaunchSplashHelper
    : public QWaylandClientExtensionTemplate<PrelaunchSplashHelper>
    , public QtWayland::treeland_prelaunch_splash_manager_v1
{
    Q_OBJECT
public:
    explicit PrelaunchSplashHelper();
    ~PrelaunchSplashHelper();

    /**
     * Show a prelaunch splash for the given app id and icon name.
     * iconName may be a theme name or absolute path; empty means no icon.
     */
    void show(const QString &appId, const QString &iconName);
    static void bufferRelease(void *data, wl_buffer *buffer);

private:
    wl_buffer *buildIconBuffer(const QIcon &icon);
    wl_buffer *createBufferWithPainter(int iconSize, qreal devicePixelRatio, const QIcon &icon);
    void handleBufferRelease(wl_buffer *buffer);

    std::vector<std::unique_ptr<QtWaylandClient::QWaylandShmBuffer>> m_iconBuffers;  // keep alive until compositor releases
};

#endif // PRELAUNCHSPLASHHELPER_H
