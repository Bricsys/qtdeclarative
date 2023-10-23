// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQMLDEBUGCLIENT_P_P_H
#define QQMLDEBUGCLIENT_P_P_H

#include "qqmldebugclient_p.h"
#include <private/qobject_p.h>

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

QT_BEGIN_NAMESPACE

class QQmlDebugClientPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlDebugClient)
public:
    QQmlDebugClientPrivate(const QString &name, QQmlDebugConnection *connection);
    void addToConnection();

    QString name;
    QPointer<QQmlDebugConnection> connection;
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCLIENT_P_P_H
