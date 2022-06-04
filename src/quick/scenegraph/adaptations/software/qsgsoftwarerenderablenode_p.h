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

#ifndef QSGSOFTWARERENDERABLENODE_H
#define QSGSOFTWARERENDERABLENODE_H

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

#include <QtQuick/private/qtquickglobal_p.h>

#include <QtGui/QRegion>
#include <QtCore/QRect>
#include <QtGui/QTransform>
#include <QtQuick/qsgrectanglenode.h>
#include <QtQuick/qsgimagenode.h>
#include <QtQuick/qsgninepatchnode.h>

QT_BEGIN_NAMESPACE

class QSGSimpleRectNode;
class QSGSimpleTextureNode;
class QSGSoftwareInternalImageNode;
class QSGSoftwarePainterNode;
class QSGSoftwareInternalRectangleNode;
class QSGSoftwareGlyphNode;
class QSGSoftwareNinePatchNode;
class QSGSoftwareSpriteNode;
class QSGRenderNode;

class Q_QUICK_PRIVATE_EXPORT QSGSoftwareRenderableNode
{
public:
    enum NodeType {
        Invalid = -1,
        SimpleRect,
        SimpleTexture,
        Image,
        Painter,
        Rectangle,
        Glyph,
        NinePatch,
        SimpleRectangle,
        SimpleImage,
#if QT_CONFIG(quick_sprite)
        SpriteNode,
#endif
        RenderNode
    };

    QSGSoftwareRenderableNode(NodeType type, QSGNode *node);
    ~QSGSoftwareRenderableNode();

    void update();

    QRegion renderNode(QPainter *painter, bool forceOpaquePainting = false);
    QRect boundingRectMin() const { return m_boundingRectMin; }
    QRect boundingRectMax() const { return m_boundingRectMax; }
    NodeType type() const { return m_nodeType; }
    bool isOpaque() const { return m_isOpaque; }
    bool isDirty() const { return m_isDirty; }
    bool isDirtyRegionEmpty() const;
    QSGNode *handle() const { return m_handle.node; }

    void setTransform(const QTransform &transform);
    void setClipRegion(const QRegion &clipRegion, bool hasClipRegion = true);
    void setOpacity(float opacity);
    QTransform transform() const { return m_transform; }
    QRegion clipRegion() const { return m_clipRegion; }
    float opacity() const { return m_opacity; }

    void markGeometryDirty();
    void markMaterialDirty();

    void addDirtyRegion(const QRegion &dirtyRegion, bool forceDirty = true);
    void subtractDirtyRegion(const QRegion &dirtyRegion);

    QRegion previousDirtyRegion(bool wasRemoved = false) const;
    QRegion dirtyRegion() const;

private:
    union RenderableNodeHandle {
        QSGNode *node;
        QSGSimpleRectNode *simpleRectNode;
        QSGSimpleTextureNode *simpleTextureNode;
        QSGSoftwareInternalImageNode *imageNode;
        QSGSoftwarePainterNode *painterNode;
        QSGSoftwareInternalRectangleNode *rectangleNode;
        QSGSoftwareGlyphNode *glpyhNode;
        QSGSoftwareNinePatchNode *ninePatchNode;
        QSGRectangleNode *simpleRectangleNode;
        QSGImageNode *simpleImageNode;
        QSGSoftwareSpriteNode *spriteNode;
        QSGRenderNode *renderNode;
    };

    const NodeType m_nodeType;
    RenderableNodeHandle m_handle;

    bool m_isOpaque;

    bool m_isDirty;
    QRegion m_dirtyRegion;
    QRegion m_previousDirtyRegion;

    QTransform m_transform;
    QRegion m_clipRegion;
    bool m_hasClipRegion;
    float m_opacity;

    QRect m_boundingRectMin;
    QRect m_boundingRectMax;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARERENDERABLENODE_H
