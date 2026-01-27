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

wl_buffer *PrelaunchSplashHelper::createBufferWithPainter(const QSize &iconSize, qreal devicePixelRatio, const QIcon &icon)
{
    auto *waylandIntegration = integration();
    auto *waylandDisplay = waylandIntegration ? waylandIntegration->display() : nullptr;
    if (!waylandDisplay) {
        qCWarning(amPrelaunchSplash, "%s", "Skip splash icon: missing Wayland display");
        return nullptr;
    }

    const int pixelWidth = std::lround(iconSize.width() * devicePixelRatio);
    const int pixelHeight = std::lround(iconSize.height() * devicePixelRatio);
    const QSize bufferSize{pixelWidth, pixelHeight};
    auto buffer = std::make_unique<QtWaylandClient::QWaylandShmBuffer>(
        waylandDisplay, bufferSize, QImage::Format_ARGB32_Premultiplied, devicePixelRatio);
    if (!buffer->image()) {
        qCWarning(amPrelaunchSplash, "%s", "Failed to allocate shm buffer for splash icon");
        return nullptr;
    }

    const QSize logicalImageSize = buffer->image()->size() / buffer->image()->devicePixelRatio();
    QRect targetRect(QPoint(0, 0), iconSize);
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

    QSize iconSize(0, 0);
    // Pick the size closest to 128x128, prefer larger sizes
    for (const QSize &size : std::as_const(sizes)) {
        if (size.width() == 128) {
            iconSize = size;
            break;
        }
        if (size.width() > 128) {
            iconSize = (iconSize.width() == 0 || size.width() < iconSize.width()) ? size : iconSize;
            continue;
        }
        iconSize = size.width() > iconSize.width() ? size : iconSize;
    }

    if (iconSize.isEmpty()) {
        iconSize = QSize(128, 128);
    }

    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;

    return createBufferWithPainter(iconSize, dpr, icon);
}

void PrelaunchSplashHelper::show(const QString &appId, const QString &iconName)
{
    if (!isActive()) {
        qCWarning(amPrelaunchSplash, "Skip prelaunch splash (extension inactive): %s", qPrintable(appId));
        return;
    }

    QIcon icon;
    if (iconName.isEmpty()) {
        qCWarning(amPrelaunchSplash, "%s", "Icon name empty; splash will be sent without icon buffer");
    } else {
        icon = QIcon::fromTheme(iconName);
        if (icon.isNull()) {
            qCWarning(amPrelaunchSplash, "Icon not found in theme: %s", qPrintable(iconName));
        }
    }

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
        return holder->buffer() == buffer;
    });

    if (it != m_iconBuffers.end()) {
        m_iconBuffers.erase(it);
    }
}

