// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLINTERVISITOR_P_H
#define QQMLJSLINTERVISITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljsimportvisitor_p.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

/*!
   \internal
    Extends QQmlJSImportVisitor with extra warnings that are required for linting but unrelated to
   QQmlJSImportVisitor actual task that is constructing QQmlJSScopes. One example of such warnings
   are purely syntactic checks, or warnings that don't affect compilation.
 */
class LinterVisitor final : public QQmlJSImportVisitor
{
public:
    using QQmlJSImportVisitor::QQmlJSImportVisitor;

protected:
    using QQmlJSImportVisitor::endVisit;
    using QQmlJSImportVisitor::visit;

    bool preVisit(QQmlJS::AST::Node *) override;
    void postVisit(QQmlJS::AST::Node *) override;
    QQmlJS::AST::Node *astParentOfVisitedNode() const;

    bool visit(QQmlJS::AST::StringLiteral *) override;
    bool visit(AST::CommaExpression *) override;

private:
    std::vector<QQmlJS::AST::Node *> m_ancestryIncludingCurrentNode;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSLINTERVISITOR_P_H
