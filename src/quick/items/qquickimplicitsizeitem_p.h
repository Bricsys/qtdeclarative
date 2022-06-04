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

#ifndef QQUICKIMPLICITSIZEITEM_H
#define QQUICKIMPLICITSIZEITEM_H

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

#include "qquickpainteditem.h"
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickImplicitSizeItemPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickImplicitSizeItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal implicitWidth READ implicitWidth NOTIFY implicitWidthChanged)
    Q_PROPERTY(qreal implicitHeight READ implicitHeight NOTIFY implicitHeightChanged)
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(6, 2)

protected:
    QQuickImplicitSizeItem(QQuickImplicitSizeItemPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickImplicitSizeItem)
    Q_DECLARE_PRIVATE(QQuickImplicitSizeItem)
};

QT_END_NAMESPACE

#endif // QQUICKIMPLICITSIZEITEM_H
