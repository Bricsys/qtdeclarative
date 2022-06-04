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

#ifndef QQUICKSMOOTHEDANIMATION_H
#define QQUICKSMOOTHEDANIMATION_H

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

#include <qqml.h>
#include "qquickanimation_p.h"

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

class QQmlProperty;
class QQuickSmoothedAnimationPrivate;
class Q_QUICK_PRIVATE_EXPORT QQuickSmoothedAnimation : public QQuickNumberAnimation
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickSmoothedAnimation)

    Q_PROPERTY(qreal velocity READ velocity WRITE setVelocity NOTIFY velocityChanged)
    Q_PROPERTY(ReversingMode reversingMode READ reversingMode WRITE setReversingMode NOTIFY reversingModeChanged)
    Q_PROPERTY(qreal maximumEasingTime READ maximumEasingTime WRITE setMaximumEasingTime NOTIFY maximumEasingTimeChanged)
    QML_NAMED_ELEMENT(SmoothedAnimation)
    QML_ADDED_IN_VERSION(2, 0)

public:
    enum ReversingMode { Eased, Immediate, Sync };
    Q_ENUM(ReversingMode)

    QQuickSmoothedAnimation(QObject *parent = nullptr);
    ~QQuickSmoothedAnimation();

    ReversingMode reversingMode() const;
    void setReversingMode(ReversingMode);

    int duration() const override;
    void setDuration(int) override;

    qreal velocity() const;
    void setVelocity(qreal);

    int maximumEasingTime() const;
    void setMaximumEasingTime(int);

    QAbstractAnimationJob* transition(QQuickStateActions &actions,
                            QQmlProperties &modified,
                            TransitionDirection direction,
                            QObject *defaultTarget = nullptr) override;
Q_SIGNALS:
    void velocityChanged();
    void reversingModeChanged();
    void maximumEasingTimeChanged();
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickSmoothedAnimation)

#endif // QQUICKSMOOTHEDANIMATION_H
