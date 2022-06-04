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

#ifndef QQMLXMLHTTPREQUEST_P_H
#define QQMLXMLHTTPREQUEST_P_H

#include <QtQml/qjsengine.h>
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

#include <QtCore/qglobal.h>
#include <private/qqmlglobal_p.h>

QT_REQUIRE_CONFIG(qml_xml_http_request);

QT_BEGIN_NAMESPACE

void *qt_add_qmlxmlhttprequest(QV4::ExecutionEngine *engine);
void qt_rem_qmlxmlhttprequest(QV4::ExecutionEngine *engine, void *);

QT_END_NAMESPACE

#endif // QQMLXMLHTTPREQUEST_P_H

