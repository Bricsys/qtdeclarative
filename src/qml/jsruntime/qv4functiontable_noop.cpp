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

#include "qv4functiontable_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

void generateFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef)
{
    Q_UNUSED(function);
    Q_UNUSED(codeRef);
}

void destroyFunctionTable(Function *function, JSC::MacroAssemblerCodeRef *codeRef)
{
    Q_UNUSED(function);
    Q_UNUSED(codeRef);
}

size_t exceptionHandlerSize()
{
    return 0;
}

} // QV4

QT_END_NAMESPACE
