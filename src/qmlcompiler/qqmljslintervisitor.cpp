// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslintervisitor_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
/*!
   \internal
    \class QQmlJS::LinterVisitor
    Extends QQmlJSImportVisitor with extra warnings that are required for linting but unrelated to
   QQmlJSImportVisitor actual task that is constructing QQmlJSScopes. One example of such warnings
   are purely syntactic checks, or style-checks warnings that don't make sense during compilation.
 */

} // namespace QQmlJS

QT_END_NAMESPACE
