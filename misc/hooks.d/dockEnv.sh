#!/bin/sh

# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# This hook is required by dde-dock that detect identity of Application.
# May be remove on later.

export GIO_LAUNCHED_DESKTOP_FILE_PID=$$
exec "$@"
