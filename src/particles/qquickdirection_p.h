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

#ifndef VARYINGVECTOR_H
#define VARYINGVECTOR_H

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

#include <QObject>
#include <QPointF>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickDirection : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NullVector)
    QML_ADDED_IN_VERSION(2, 0)
    QML_UNCREATABLE("Abstract type. Use one of the inheriting types instead.")

public:
    explicit QQuickDirection(QObject *parent = 0);

    virtual QPointF sample(const QPointF &from);
Q_SIGNALS:

public Q_SLOTS:

protected:
};

QT_END_NAMESPACE
#endif // VARYINGVECTOR_H
