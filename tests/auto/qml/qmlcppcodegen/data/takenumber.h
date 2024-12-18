// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TAKENUMBER_H
#define TAKENUMBER_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class TakeNumber : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TakeNumber(QObject *parent = nullptr);

    Q_INVOKABLE void takeInt(int a);
    Q_INVOKABLE void takeNegativeInt(int a);
    Q_INVOKABLE void takeQSizeType(qsizetype a);
    Q_INVOKABLE void takeQLongLong(qlonglong a);

    int takenInt = 0;
    int takenNegativeInt = 0;
    qsizetype takenQSizeType = 0;
    qlonglong takenQLongLong = 0;
};

#endif // TAKENUMBER_H
