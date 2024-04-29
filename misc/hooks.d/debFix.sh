#!/bin/bash

# SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-or-later

# This hook script wraps the startup of all applications within this script, 
# in order to fix the issue of some applications not being able to start without shebang in their startup scripts

firstArg=$1

if [[ -f ${firstArg} ]]; then
    shebangFound=false
    while IFS= read -r line; do
        if [[ "$line" == "#!"* ]]; then
            shebangFound=true
            break
        fi
    done < "$firstArg"

    if [ "$shebangFound" = false ] && file -b "$firstArg" | grep -q "Python script"; then
        pythonInterpreter=$(command -v python3 || command -v python)
        if [ -z "${pythonInterpreter}" ]; then
            echo "Python interpreter not found."
            exit 1
        fi
        exec ${pythonInterpreter} "$@"
    fi
fi

exec "$@"