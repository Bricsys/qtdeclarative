// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGINTERNALTEXTNODE_P_H
#define QSGINTERNALTEXTNODE_P_H

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

#include "qsgtextnode.h"
#include "qquicktext_p.h"
#include <qglyphrun.h>

#include <QtGui/qcolor.h>
#include <QtGui/qtextlayout.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QSGGlyphNode;
class QTextBlock;
class QColor;
class QTextDocument;
class QSGContext;
class QRawFont;
class QSGInternalRectangleNode;
class QSGClipNode;
class QSGTexture;
class QSGRenderContext;

class QQuickTextNodeEngine;

class Q_QUICK_PRIVATE_EXPORT QSGInternalTextNode : public QSGTextNode
{
public:
    QSGInternalTextNode(QSGRenderContext *renderContext);
    ~QSGInternalTextNode();

    static bool isComplexRichText(QTextDocument *);

    void addTextLayout(const QPointF &position, QTextLayout *textLayout,
                       int selectionStart = -1, int selectionEnd = -1,
                       int lineStart = 0, int lineCount = -1) override;
    void addTextDocument(const QPointF &position, QTextDocument *textDocument,
                         int selectionStart = -1, int selectionEnd = -1) override;

    void setColor(const QColor &color) override
    {
        m_color = color;
    }

    QColor color() const override
    {
        return m_color;
    }

    void setTextStyle(TextStyle textStyle) override
    {
        m_textStyle = textStyle;
    }

    TextStyle textStyle() override
    {
        return m_textStyle;
    }

    void setStyleColor(const QColor &styleColor) override
    {
        m_styleColor = styleColor;
    }

    QColor styleColor() const override
    {
        return m_styleColor;
    }

    void setAnchorColor(const QColor &anchorColor) override
    {
        m_anchorColor = anchorColor;
    }

    QColor anchorColor() const override
    {
        return m_anchorColor;
    }

    void setSelectionColor(const QColor &selectionColor) override
    {
        m_selectionColor = selectionColor;
    }

    QColor selectionColor() const override
    {
        return m_selectionColor;
    }

    void setSelectionTextColor(const QColor &selectionTextColor) override
    {
        m_selectionTextColor = selectionTextColor;
    }

    QColor selectionTextColor() const override
    {
        return m_selectionTextColor;
    }

    void setRenderTypeQuality(int renderTypeQuality) override
    {
        m_renderTypeQuality = renderTypeQuality;
    }
    int renderTypeQuality() const override
    {
        return m_renderTypeQuality;
    }

    void setRenderType(RenderType renderType) override
    {
        m_renderType = renderType;
    }

    RenderType renderType() const override
    {
        return m_renderType;
    }

    void setSmooth(bool smooth) override
    {
        m_smooth = smooth;
    }

    bool smooth() const override
    {
        return m_smooth;
    }

    void setViewport(const QRectF &viewport) override
    {
        m_viewport = viewport;
    }

    QRectF viewport() const override
    {
        return m_viewport;
    }

    void setCursor(const QRectF &rect, const QColor &color);
    void clearCursor();

    void addRectangleNode(const QRectF &rect, const QColor &color);
    void addImage(const QRectF &rect, const QImage &image);
    void deleteContent();
    QSGGlyphNode *addGlyphs(const QPointF &position, const QGlyphRun &glyphs, const QColor &color,
                            QQuickText::TextStyle style = QQuickText::Normal, const QColor &styleColor = QColor(),
                            QSGNode *parentNode = 0);

    QSGInternalRectangleNode *cursorNode() const { return m_cursorNode; }
    QPair<int, int> renderedLineRange() const { return { m_firstLineInViewport, m_firstLinePastViewport }; }

private:
    QSGInternalRectangleNode *m_cursorNode = nullptr;
    QList<QSGTexture *> m_textures;
    QSGRenderContext *m_renderContext = nullptr;
    RenderType m_renderType = QtRendering;
    TextStyle m_textStyle = Normal;
    QRectF m_viewport;
    QColor m_color = QColor(0, 0, 0);
    QColor m_styleColor = QColor(0, 0, 0);
    QColor m_anchorColor = QColor(0, 0, 255);
    QColor m_selectionColor = QColor(0, 0, 128);
    QColor m_selectionTextColor = QColor(255, 255, 255);
    bool m_smooth = false;
    int m_renderTypeQuality = -1;
    int m_firstLineInViewport = -1;
    int m_firstLinePastViewport = -1;

    friend class QQuickTextEdit;
    friend class QQuickTextEditPrivate;
};

QT_END_NAMESPACE

#endif // QSGINTERNALTEXTNODE_P_H
