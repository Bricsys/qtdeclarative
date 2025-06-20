// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKBASICTHEME_P_H
#define QQUICKBASICTHEME_P_H

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

#include <QtQuickControls2Basic/qtquickcontrols2basicexports.h>

QT_BEGIN_NAMESPACE

class QQuickTheme;

class Q_QUICKCONTROLS2BASIC_EXPORT QQuickBasicTheme
{
public:
    static void initialize(QQuickTheme *theme);
    static void updateTheme();
};

QT_END_NAMESPACE

#endif // QQUICKBASICTHEME_P_H
