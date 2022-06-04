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

#ifndef QQuickCUMULATIVEDIRECTION_P_H
#define QQuickCUMULATIVEDIRECTION_P_H

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
#include "qquickdirection_p.h"
#include <QQmlListProperty>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickCumulativeDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QQuickDirection> directions READ directions)
    Q_CLASSINFO("DefaultProperty", "directions")
    QML_NAMED_ELEMENT(CumulativeDirection)
    QML_ADDED_IN_VERSION(2, 0)
public:
    explicit QQuickCumulativeDirection(QObject *parent = 0);
    QQmlListProperty<QQuickDirection> directions();
    QPointF sample(const QPointF &from) override;
private:
    QList<QQuickDirection*> m_directions;
};

QT_END_NAMESPACE

#endif // QQuickCUMULATIVEDIRECTION_P_H
