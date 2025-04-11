// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKLAYERITEM_P_H
#define QQUICKLAYERITEM_P_H

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

#include <QtGui/qmatrix4x4.h>
#include <QtQuick/qquickitem.h>
#include <QtQuickVectorImageHelpers/qtquickvectorimagehelpersexports.h>

QT_BEGIN_NAMESPACE

class Q_QUICKVECTORIMAGEHELPERS_EXPORT QQuickLayerItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QMatrix4x4 transformMatrix READ transformMatrix NOTIFY transformMatrixChanged)
    QML_NAMED_ELEMENT(LayerItem)
    Q_DECLARE_PRIVATE(QQuickItem)

public:
    QQuickLayerItem(QQuickItem *parent = nullptr);
    QMatrix4x4 transformMatrix();

protected:
    void itemChange(ItemChange change, const ItemChangeData &) override;

private:
    bool m_transformDirty = true;
    QMatrix4x4 m_transform;

signals:
    void transformMatrixChanged();
};

QT_END_NAMESPACE

#endif // QQUICKLAYERITEM_P_H

