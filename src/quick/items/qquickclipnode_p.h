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

#ifndef QQUICKCLIPNODE_P_H
#define QQUICKCLIPNODE_P_H

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

#include <private/qtquickglobal_p.h>
#include <QtQuick/qsgnode.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_PRIVATE_EXPORT QQuickDefaultClipNode : public QSGClipNode
{
public:
    QQuickDefaultClipNode(const QRectF &);

    void setRect(const QRectF &);
    QRectF rect() const { return m_rect; }

    void setRadius(qreal radius);
    qreal radius() const { return m_radius; }

    virtual void update();

private:
    void updateGeometry();
    QRectF m_rect;
    qreal m_radius;

    uint m_dirty_geometry : 1;
    uint m_reserved : 31;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

#endif // QQUICKCLIPNODE_P_H
