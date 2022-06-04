/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLPROPERTYCACHEMETODARGUMENTS_P_H
#define QQMLPROPERTYCACHEMETODARGUMENTS_P_H

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

#include <QtCore/qlist.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qtaggedpointer.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QString;
class QQmlPropertyCacheMethodArguments
{
public:
    QQmlPropertyCacheMethodArguments *next;
    QList<QByteArray> *names;
    QMetaType types[1]; // First one is return type
};

QT_END_NAMESPACE

#endif // QQMLPROPERTYCACHEMETODARGUMENTS_P_H
