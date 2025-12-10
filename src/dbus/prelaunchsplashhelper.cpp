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

namespace {

static const struct wl_buffer_listener kBufferListener = {
    PrelaunchSplashHelper::bufferRelease,
};

QSize pickBestSize(const QList<QSize> &sizes)
{
    if (sizes.isEmpty()) {
        return QSize{64, 64};
    }

    auto it = std::min_element(sizes.cbegin(), sizes.cend(), [](const QSize &a, const QSize &b) {
        constexpr int target = 64;
        const int da = std::abs(a.width() - target);
        const int db = std::abs(b.width() - target);
        return da < db;
    });
    return *it;
}

} // namespace

wl_buffer *PrelaunchSplashHelper::createBufferFromPixmap(const QPixmap &pixmap)
{
    auto *waylandIntegration = integration();
    auto *waylandDisplay = waylandIntegration ? waylandIntegration->display() : nullptr;
    if (!waylandDisplay) {
        qCWarning(amPrelaunchSplash, "%s", "Skip splash icon: missing Wayland display");
        return nullptr;
    }

    const QSize squareSize = pixmap.size().expandedTo(pixmap.size().transposed());
    auto buffer = std::make_unique<QtWaylandClient::QWaylandShmBuffer>(
        waylandDisplay, squareSize, QImage::Format_ARGB32_Premultiplied, pixmap.devicePixelRatio());
    if (!buffer || !buffer->image()) {
        qCWarning(amPrelaunchSplash, "%s", "Failed to allocate shm buffer for splash icon");
        return nullptr;
    }

    QRect targetRect = pixmap.rect();
    targetRect.moveCenter(buffer->image()->rect().center());
    QPainter painter(buffer->image());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(buffer->image()->rect(), Qt::transparent);
    painter.drawPixmap(targetRect, pixmap, pixmap.rect());

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
    if (sizes.isEmpty()) {
        sizes.append(QSize{64, 64});
    }

    // Pick the size closest to 64x64
    const QSize chosen = pickBestSize(sizes);
    const QPixmap pixmap = icon.pixmap(chosen);
    if (pixmap.isNull()) {
        return nullptr;
    }

    return createBufferFromPixmap(pixmap);
}

void PrelaunchSplashHelper::show(const QString &appId, const QString &iconName)
{
    if (!isActive()) {
        qCWarning(amPrelaunchSplash, "Skip prelaunch splash (extension inactive): %s", qPrintable(appId));
        return;
    }

    QIcon icon;
    if (!iconName.isEmpty()) {
        if (iconName.contains('/')) {
            icon = QIcon(iconName);
        } else {
            icon = QIcon::fromTheme(iconName);
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
        return holder && holder->buffer() == buffer;
    });

    if (it != m_iconBuffers.end()) {
        m_iconBuffers.erase(it);
    }
}

