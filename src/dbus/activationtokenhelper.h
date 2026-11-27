// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef ACTIVATIONTOKENHELPER_H
#define ACTIVATIONTOKENHELPER_H

#include <QLoggingCategory>
#include <QObject>
#include <QString>
#include <QtWaylandClient/QWaylandClientExtension>

#include "qwayland-treeland-activation-token-unstable-v1.h"

Q_DECLARE_LOGGING_CATEGORY(amActivationToken)

/**
 * @brief Helper for requesting xdg-activation tokens via Wayland.
 *
 * Uses the treeland_activation_token_manager_v1 protocol to obtain
 * one-shot activation tokens for applications about to be launched.
 * Each requestToken() call mints a fresh token; tokens are not cached.
 *
 * Not thread-safe; use from the Qt GUI thread.
 */
class ActivationTokenHelper
    : public QWaylandClientExtensionTemplate<ActivationTokenHelper>
    , public QtWayland::treeland_activation_token_manager_v1
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ActivationTokenHelper)
public:
    explicit ActivationTokenHelper();
    ~ActivationTokenHelper();

    /**
     * Synchronously request an activation token from the compositor.
     * Returns an empty string if the extension is inactive or the
     * compositor does not deliver a token.
     */
    QString requestToken();
};

#endif // ACTIVATIONTOKENHELPER_H
