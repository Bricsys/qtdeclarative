// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslintervisitor_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QQmlJS::AST;

namespace QQmlJS {
/*!
   \internal
    \class QQmlJS::LinterVisitor
    Extends QQmlJSImportVisitor with extra warnings that are required for linting but unrelated to
   QQmlJSImportVisitor actual task that is constructing QQmlJSScopes. One example of such warnings
   are purely syntactic checks, or style-checks warnings that don't make sense during compilation.
 */

bool LinterVisitor::visit(StringLiteral *sl)
{
    QQmlJSImportVisitor::visit(sl);
    const QString s = m_logger->code().mid(sl->literalToken.begin(), sl->literalToken.length);

    if (s.contains(QLatin1Char('\r')) || s.contains(QLatin1Char('\n')) || s.contains(QChar(0x2028u))
        || s.contains(QChar(0x2029u))) {
        QString templateString;

        bool escaped = false;
        const QChar stringQuote = s[0];
        for (qsizetype i = 1; i < s.size() - 1; i++) {
            const QChar c = s[i];

            if (c == u'\\') {
                escaped = !escaped;
            } else if (escaped) {
                // If we encounter an escaped quote, unescape it since we use backticks here
                if (c == stringQuote)
                    templateString.chop(1);

                escaped = false;
            } else {
                if (c == u'`')
                    templateString += u'\\';
                if (c == u'$' && i + 1 < s.size() - 1 && s[i + 1] == u'{')
                    templateString += u'\\';
            }

            templateString += c;
        }

        QQmlJSFixSuggestion suggestion = { "Use a template literal instead."_L1, sl->literalToken,
                                           u"`" % templateString % u"`" };
        suggestion.setAutoApplicable();
        m_logger->log(QStringLiteral("String contains unescaped line terminator which is "
                                     "deprecated."),
                      qmlMultilineStrings, sl->literalToken, true, true, suggestion);
    }
    return true;
}

bool LinterVisitor::preVisit(Node *n)
{
    m_ancestryIncludingCurrentNode.push_back(n);
    return true;
}

void LinterVisitor::postVisit(Node *n)
{
    Q_ASSERT(m_ancestryIncludingCurrentNode.back() == n);
    m_ancestryIncludingCurrentNode.pop_back();
}

Node *LinterVisitor::astParentOfVisitedNode() const
{
    if (m_ancestryIncludingCurrentNode.size() < 2)
        return nullptr;
    return m_ancestryIncludingCurrentNode[m_ancestryIncludingCurrentNode.size() - 2];
}

bool LinterVisitor::visit(CommaExpression *expression)
{
    QQmlJSImportVisitor::visit(expression);
    if (!expression->left || !expression->right)
        return true;

    // don't warn about commas in "for" statements
    if (cast<ForStatement *>(astParentOfVisitedNode()))
        return true;

    m_logger->log("Do not use comma expressions."_L1, qmlComma, expression->commaToken);
    return true;
}

static void warnAboutLiteralConstructors(NewMemberExpression *expression, QQmlJSLogger *logger)
{
    static constexpr std::array literals{ "Boolean"_L1, "Function"_L1, "JSON"_L1,
                                          "Math"_L1,    "Number"_L1,   "String"_L1 };

    const IdentifierExpression *identifier = cast<IdentifierExpression *>(expression->base);
    if (!identifier)
        return;

    if (std::find(literals.cbegin(), literals.cend(), identifier->name) != literals.cend()) {
        logger->log("Do not use '%1' as a constructor."_L1.arg(identifier->name),
                    qmlLiteralConstructor, identifier->identifierToken);
    }
}

bool LinterVisitor::visit(NewMemberExpression *expression)
{
    QQmlJSImportVisitor::visit(expression);
    warnAboutLiteralConstructors(expression, m_logger);
    return true;
}

bool LinterVisitor::visit(VoidExpression *ast)
{
    QQmlJSImportVisitor::visit(ast);
    m_logger->log("Do not use void expressions."_L1, qmlVoid, ast->voidToken);
    return true;
}

static SourceLocation confusingPluses(BinaryExpression *exp)
{
    Q_ASSERT(exp->op == QSOperator::Add);

    SourceLocation location = exp->operatorToken;

    // a++ + b
    if (auto increment = cast<PostIncrementExpression *>(exp->left))
        location = combine(increment->incrementToken, location);
    // a + +b
    if (auto unary = cast<UnaryPlusExpression *>(exp->right))
        location = combine(location, unary->plusToken);
    // a + ++b
    if (auto increment = cast<PreIncrementExpression *>(exp->right))
        location = combine(location, increment->incrementToken);

    if (location == exp->operatorToken)
        return SourceLocation{};

    return location;
}

static SourceLocation confusingMinuses(BinaryExpression *exp)
{
    Q_ASSERT(exp->op == QSOperator::Sub);

    SourceLocation location = exp->operatorToken;

    // a-- - b
    if (auto decrement = cast<PostDecrementExpression *>(exp->left))
        location = combine(decrement->decrementToken, location);
    // a - -b
    if (auto unary = cast<UnaryMinusExpression *>(exp->right))
        location = combine(location, unary->minusToken);
    // a - --b
    if (auto decrement = cast<PreDecrementExpression *>(exp->right))
        location = combine(location, decrement->decrementToken);

    if (location == exp->operatorToken)
        return SourceLocation{};

    return location;
}

bool LinterVisitor::visit(BinaryExpression *exp)
{
    QQmlJSImportVisitor::visit(exp);
    switch (exp->op) {
    case QSOperator::Add:
        if (SourceLocation loc = confusingPluses(exp); loc.isValid())
            m_logger->log("Confusing pluses."_L1, qmlConfusingPluses, loc);
        break;
    case QSOperator::Sub:
        if (SourceLocation loc = confusingMinuses(exp); loc.isValid())
            m_logger->log("Confusing minuses."_L1, qmlConfusingMinuses, loc);
        break;
    default:
        break;
    }

    return true;
}

} // namespace QQmlJS

QT_END_NAMESPACE
