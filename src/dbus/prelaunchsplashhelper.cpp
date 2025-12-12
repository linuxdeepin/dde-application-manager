// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "prelaunchsplashhelper.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMessageLogger>
#include <QPainter>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>
#include <wayland-client-core.h>
#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(amPrelaunchSplash, "dde.am.prelaunch.splash")

PrelaunchSplashHelper::PrelaunchSplashHelper()
    : QWaylandClientExtensionTemplate<PrelaunchSplashHelper>(1)
{
}

PrelaunchSplashHelper::~PrelaunchSplashHelper() = default;

static const struct wl_buffer_listener kBufferListener = {
    PrelaunchSplashHelper::bufferRelease,
};

wl_buffer *PrelaunchSplashHelper::createBufferWithPainter(int iconSize, qreal devicePixelRatio, const QIcon &icon)
{
    auto *waylandIntegration = integration();
    auto *waylandDisplay = waylandIntegration ? waylandIntegration->display() : nullptr;
    if (!waylandDisplay) {
        qCWarning(amPrelaunchSplash, "%s", "Skip splash icon: missing Wayland display");
        return nullptr;
    }

    const int pixelSize = std::lround(iconSize * devicePixelRatio);
    const QSize squareSize{pixelSize, pixelSize};
    auto buffer = std::make_unique<QtWaylandClient::QWaylandShmBuffer>(
        waylandDisplay, squareSize, QImage::Format_ARGB32_Premultiplied, devicePixelRatio);
    if (!buffer->image()) {
        qCWarning(amPrelaunchSplash, "%s", "Failed to allocate shm buffer for splash icon");
        return nullptr;
    }

    const QSize logicalImageSize = buffer->image()->size() / buffer->image()->devicePixelRatio();
    QRect targetRect(QPoint(0, 0), QSize{iconSize, iconSize});
    targetRect.moveCenter(QRect(QPoint(0, 0), logicalImageSize).center());
    QPainter painter(buffer->image());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(buffer->image()->rect(), Qt::transparent);
    icon.paint(&painter, targetRect);

    auto *wlBuf = buffer->buffer();
    wl_buffer_add_listener(wlBuf, &kBufferListener, this);

    m_iconBuffers.emplace_back(std::move(buffer));
    return wlBuf;
}

wl_buffer *PrelaunchSplashHelper::buildIconBuffer(const QIcon &icon)
{
    if (icon.isNull()) {
        return nullptr;
    }

    QList<QSize> sizes = icon.availableSizes();

    QSize chosen(0,0);
    // Pick the size closest to 128x128, prefer larger sizes
    for (const QSize &size : sizes) {
        if (size.width() == 128) {
            chosen = size;
            break;
        }
        if (size.width() > 128) {
            chosen = size.width() < chosen.width() || chosen.width() == 0 ? size : chosen;
            continue;
        }
        chosen = size.width() > chosen.width() ? size : chosen;
    };

    if (chosen.isEmpty()) {
        chosen = QSize(128, 128);
    }

    const int iconSize = chosen.width();
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;

    return createBufferWithPainter(iconSize, dpr, icon);
}

void PrelaunchSplashHelper::show(const QString &appId, const QString &iconName)
{
    if (!isActive()) {
        qCWarning(amPrelaunchSplash, "Skip prelaunch splash (extension inactive): %s", qPrintable(appId));
        return;
    }

    QIcon icon = QIcon::fromTheme(iconName);

    // Keep previously sent buffers alive; compositor releases them asynchronously.
    wl_buffer *buffer = buildIconBuffer(icon);
    create_splash(appId, QStringLiteral("dde-application-manager"), buffer);
    qCInfo(amPrelaunchSplash, "Sent create_splash for %s %s", qPrintable(appId), buffer ? "with icon buffer" : "without icon buffer");
}

/*static*/ void PrelaunchSplashHelper::bufferRelease(void *data, wl_buffer *buffer)
{
    if (auto *helper = static_cast<PrelaunchSplashHelper *>(data)) {
        helper->handleBufferRelease(buffer);
    }
}

void PrelaunchSplashHelper::handleBufferRelease(wl_buffer *buffer)
{
    auto it = std::find_if(m_iconBuffers.begin(), m_iconBuffers.end(), [buffer](const auto &holder) {
        return holder && holder->buffer() == buffer;
    });

    if (it != m_iconBuffers.end()) {
        m_iconBuffers.erase(it);
    }
}

