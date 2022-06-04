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

#ifndef QQUICKRENDERCONTROL_H
#define QQUICKRENDERCONTROL_H

#include <QtCore/qobject.h>
#include <QtQuick/qtquickglobal.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class QQuickWindow;
class QOpenGLContext;
class QQuickRenderControlPrivate;
class QThread;

class Q_QUICK_EXPORT QQuickRenderControl : public QObject
{
    Q_OBJECT

public:
    explicit QQuickRenderControl(QObject *parent = nullptr);
    ~QQuickRenderControl() override;

    void prepareThread(QThread *targetThread);

    void setSamples(int sampleCount);
    int samples() const;

    bool initialize();

    void invalidate();

    void beginFrame();
    void endFrame();

    void polishItems();
    bool sync();
    void render();

    static QWindow *renderWindowFor(QQuickWindow *win, QPoint *offset = nullptr);
    virtual QWindow *renderWindow(QPoint *offset) { Q_UNUSED(offset); return nullptr; }

    QQuickWindow *window() const;

protected:
    QQuickRenderControl(QQuickRenderControlPrivate &dd, QObject * parent);

Q_SIGNALS:
    void renderRequested();
    void sceneChanged();

private:
    Q_DECLARE_PRIVATE(QQuickRenderControl)
};

QT_END_NAMESPACE

#endif // QQUICKRENDERCONTROL_H
