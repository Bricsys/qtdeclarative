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

#ifndef QSGVERTEXCOLORMATERIAL_H
#define QSGVERTEXCOLORMATERIAL_H

#include <QtQuick/qsgmaterial.h>

QT_BEGIN_NAMESPACE

class Q_QUICK_EXPORT QSGVertexColorMaterial : public QSGMaterial
{
public:
    QSGVertexColorMaterial();

    int compare(const QSGMaterial *other) const override;

protected:
    QSGMaterialType *type() const override;
    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode renderMode) const override;
};

QT_END_NAMESPACE

#endif // VERTEXCOLORMATERIAL_H
