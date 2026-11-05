// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "activationtokenhelper.h"

#include <QLoggingCategory>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>
#include <QtWaylandClient/private/qwaylandintegration_p.h>
#include <wayland-client-core.h>

Q_LOGGING_CATEGORY(amActivationToken, "dde.am.activation.token")

ActivationTokenHelper::ActivationTokenHelper()
    : QWaylandClientExtensionTemplate<ActivationTokenHelper>(1)
{
}

ActivationTokenHelper::~ActivationTokenHelper() = default;

namespace {

class TokenObject : public QtWayland::treeland_activation_token_v1
{
public:
    QString token;
    bool received = false;

protected:
    void treeland_activation_token_v1_token(const QString &t) override
    {
        token = t;
        received = true;
    }
};

} // namespace

QString ActivationTokenHelper::requestToken()
{
    if (!isActive()) {
        qCWarning(amActivationToken, "%s", "Skip activation token request (extension inactive)");
        return {};
    }

    TokenObject obj;

    // request_token() returns a newly minted proxy; guard against a null
    // result (e.g. OOM or protocol error) before attaching the listener.
    if (auto *raw = request_token()) {
        obj.init(raw);
    } else {
        qCWarning(amActivationToken, "%s", "Failed to create token object");
        return {};
    }

    auto *waylandIntegration = integration();
    auto *waylandDisplay = waylandIntegration ? waylandIntegration->display() : nullptr;
    if (!waylandDisplay) {
        qCWarning(amActivationToken, "%s", "Missing Wayland display, cannot roundtrip");
        obj.destroy();
        return {};
    }

    if (wl_display_roundtrip(waylandDisplay->wl_display()) < 0) {
        qCWarning(amActivationToken, "%s", "Wayland roundtrip failed (connection error)");
        obj.destroy();
        return {};
    }

    obj.destroy();

    if (!obj.received) {
        qCWarning(amActivationToken, "%s", "Activation token not received from compositor");
        return {};
    }

    return obj.token;
}
