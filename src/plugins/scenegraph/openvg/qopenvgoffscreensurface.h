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

#ifndef QOPENVGOFFSCREENSURFACE_H
#define QOPENVGOFFSCREENSURFACE_H

#include "qopenvgcontext_p.h"

QT_BEGIN_NAMESPACE

class QOpenVGOffscreenSurface
{
public:
    QOpenVGOffscreenSurface(const QSize &size);
    ~QOpenVGOffscreenSurface();

    void makeCurrent();
    void doneCurrent();
    void swapBuffers();

    VGImage image() { return m_image; }
    QSize size() const { return m_size; }

    QImage readbackQImage();

private:
    VGImage m_image;
    QSize m_size;
    EGLContext m_context;
    EGLSurface m_renderTarget;
    EGLContext m_previousContext = EGL_NO_CONTEXT;
    EGLSurface m_previousReadSurface = EGL_NO_SURFACE;
    EGLSurface m_previousDrawSurface = EGL_NO_SURFACE;
    EGLDisplay m_display;
};

QT_END_NAMESPACE

#endif // QOPENVGOFFSCREENSURFACE_H
