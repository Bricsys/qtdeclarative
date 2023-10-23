// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qquickdirection_p.h"

QT_BEGIN_NAMESPACE
/*!
    \qmltype Direction
    \instantiates QQuickDirection
    \inqmlmodule QtQuick.Particles
    \brief For specifying a vector space.
    \ingroup qtquick-particles


*/


QQuickDirection::QQuickDirection(QObject *parent) :
    QObject(parent)
{
}

QPointF QQuickDirection::sample(const QPointF &from)
{
    Q_UNUSED(from);
    return QPointF();
}

QT_END_NAMESPACE

#include "moc_qquickdirection_p.cpp"
