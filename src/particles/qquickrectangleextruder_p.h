/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef RECTANGLEEXTRUDER_H
#define RECTANGLEEXTRUDER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qquickparticleextruder_p.h"

QT_BEGIN_NAMESPACE

class QQuickRectangleExtruder : public QQuickParticleExtruder
{
    Q_OBJECT
    Q_PROPERTY(bool fill READ fill WRITE setFill NOTIFY fillChanged)
    QML_NAMED_ELEMENT(RectangleShape)
    QML_ADDED_IN_VERSION(2, 0)

public:
    explicit QQuickRectangleExtruder(QObject *parent = 0);
    QPointF extrude(const QRectF &) override;
    bool contains(const QRectF &bounds, const QPointF &point) override;
    bool fill() const
    {
        return m_fill;
    }

Q_SIGNALS:

    void fillChanged(bool arg);

public Q_SLOTS:

    void setFill(bool arg)
    {
        if (m_fill != arg) {
            m_fill = arg;
            Q_EMIT fillChanged(arg);
        }
    }
protected:
    bool m_fill;
};

QT_END_NAMESPACE

#endif // RectangleEXTRUDER_H
