// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "prelaunchsplashhelper.h"

#include <QGuiApplication>
#include <QLoggingCategory>
#include <QPainter>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandshmbackingstore_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <algorithm>
#include <cmath>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(amPrelaunchSplash, "dde.am.prelaunch.splash")

struct PrelaunchSplashHelper::Impl {
    std::vector<std::unique_ptr<QtWaylandClient::QWaylandShmBuffer>> iconBuffers;
};

PrelaunchSplashHelper::PrelaunchSplashHelper()
    : QWaylandClientExtensionTemplate<PrelaunchSplashHelper>(1)
    , m_impl(std::make_unique<Impl>())
{
}

PrelaunchSplashHelper::~PrelaunchSplashHelper() = default;

namespace {

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
    m_impl->iconBuffers.emplace_back(std::move(buffer));
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

    m_impl->iconBuffers.clear();

    QIcon icon;
    if (!iconName.isEmpty()) {
        if (iconName.contains('/')) {
            icon = QIcon(iconName);
        } else {
            icon = QIcon::fromTheme(iconName);
        }
    }

    wl_buffer *buffer = buildIconBuffer(icon);
    create_splash(appId, QStringLiteral("dde-application-manager"), buffer);
    qCInfo(amPrelaunchSplash, "Sent create_splash for %s %s", qPrintable(appId), buffer ? "with icon buffer" : "without icon buffer");
}

