// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "utils.h"

#include <QtQuickControls2/private/qquickstyle_p.h>

Utils::Utils(QObject *parent)
    : QObject(parent)
{
}

QStringList Utils::availableStyles() const
{
    return QQuickStylePrivate::builtInStyles();
}
