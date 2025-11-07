// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef UTILS_H
#define UTILS_H

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class Utils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableStyles READ availableStyles CONSTANT FINAL)
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit Utils(QObject *parent = nullptr);

    QStringList availableStyles() const;
};

#endif // UTILS_H
