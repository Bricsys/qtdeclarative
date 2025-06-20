// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLJSAST_P_H
#define QQMLJSAST_P_H

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

#include "qqmljsastvisitor_p.h"
#include "qqmljsglobal_p.h"

#include <private/qqmljsmemorypool_p.h>

#include <QtCore/qtaggedpointer.h>
#include <QtCore/qversionnumber.h>

#include <type_traits>

QT_BEGIN_NAMESPACE

class QString;

namespace QQmlJS {
    class Parser;
}

#define QQMLJS_DECLARE_AST_NODE(name) \
  enum { K = Kind_##name };

namespace QSOperator // ### rename
{

enum Op {
    Add,
    And,
    InplaceAnd,
    Assign,
    BitAnd,
    BitOr,
    BitXor,
    InplaceSub,
    Div,
    InplaceDiv,
    Equal,
    Exp,
    InplaceExp,
    Ge,
    Gt,
    In,
    InplaceAdd,
    InstanceOf,
    Le,
    LShift,
    InplaceLeftShift,
    Lt,
    Mod,
    InplaceMod,
    Mul,
    InplaceMul,
    NotEqual,
    Or,
    InplaceOr,
    RShift,
    InplaceRightShift,
    StrictEqual,
    StrictNotEqual,
    Sub,
    URShift,
    InplaceURightShift,
    InplaceXor,
    As,
    Coalesce,
    Invalid
};

} // namespace QSOperator

namespace QQmlJS {

namespace AST {

enum class VariableScope {
    NoScope,
    Var,
    Let,
    Const
};

template <typename T1, typename T2>
T1 cast(T2 *ast)
{
    if (ast && ast->kind == std::remove_pointer_t<T1>::K)
        return static_cast<T1>(ast);

    return nullptr;
}

FunctionExpression *asAnonymousFunctionDefinition(AST::Node *n);
ClassExpression *asAnonymousClassDefinition(AST::Node *n);

class QML_PARSER_EXPORT Node: public Managed
{
public:
    enum Kind {
        Kind_Undefined,

        Kind_ArgumentList,
        Kind_ArrayPattern,
        Kind_ArrayMemberExpression,
        Kind_BinaryExpression,
        Kind_Block,
        Kind_BreakStatement,
        Kind_CallExpression,
        Kind_CaseBlock,
        Kind_CaseClause,
        Kind_CaseClauses,
        Kind_Catch,
        Kind_ConditionalExpression,
        Kind_ContinueStatement,
        Kind_DebuggerStatement,
        Kind_DefaultClause,
        Kind_DeleteExpression,
        Kind_DoWhileStatement,
        Kind_ElementList,
        Kind_Elision,
        Kind_EmptyStatement,
        Kind_Expression,
        Kind_ExpressionStatement,
        Kind_FalseLiteral,
        Kind_SuperLiteral,
        Kind_FieldMemberExpression,
        Kind_Finally,
        Kind_ForEachStatement,
        Kind_ForStatement,
        Kind_FormalParameterList,
        Kind_FunctionBody,
        Kind_FunctionDeclaration,
        Kind_FunctionExpression,
        Kind_ClassExpression,
        Kind_ClassDeclaration,
        Kind_IdentifierExpression,
        Kind_IdentifierPropertyName,
        Kind_InitializerExpression,
        Kind_ComputedPropertyName,
        Kind_IfStatement,
        Kind_LabelledStatement,
        Kind_NameSpaceImport,
        Kind_ImportSpecifier,
        Kind_ImportsList,
        Kind_NamedImports,
        Kind_ImportClause,
        Kind_FromClause,
        Kind_ImportDeclaration,
        Kind_Module,
        Kind_ExportSpecifier,
        Kind_ExportsList,
        Kind_ExportClause,
        Kind_ExportDeclaration,
        Kind_NewExpression,
        Kind_NewMemberExpression,
        Kind_NotExpression,
        Kind_NullExpression,
        Kind_YieldExpression,
        Kind_NumericLiteral,
        Kind_NumericLiteralPropertyName,
        Kind_ObjectPattern,
        Kind_PostDecrementExpression,
        Kind_PostIncrementExpression,
        Kind_PreDecrementExpression,
        Kind_PreIncrementExpression,
        Kind_Program,
        Kind_PropertyDefinitionList,
        Kind_PropertyGetterSetter,
        Kind_PropertyName,
        Kind_PropertyNameAndValue,
        Kind_RegExpLiteral,
        Kind_ReturnStatement,
        Kind_StatementList,
        Kind_StringLiteral,
        Kind_StringLiteralPropertyName,
        Kind_SwitchStatement,
        Kind_TemplateLiteral,
        Kind_TaggedTemplate,
        Kind_TypeExpression,
        Kind_ThisExpression,
        Kind_ThrowStatement,
        Kind_TildeExpression,
        Kind_TrueLiteral,
        Kind_TryStatement,
        Kind_TypeOfExpression,
        Kind_UnaryMinusExpression,
        Kind_UnaryPlusExpression,
        Kind_VariableDeclaration,
        Kind_VariableDeclarationList,
        Kind_VariableStatement,
        Kind_VoidExpression,
        Kind_WhileStatement,
        Kind_WithStatement,
        Kind_NestedExpression,
        Kind_ClassElementList,
        Kind_PatternElement,
        Kind_PatternElementList,
        Kind_PatternProperty,
        Kind_PatternPropertyList,
        Kind_Type,
        Kind_TypeArgument,
        Kind_TypeAnnotation,

        Kind_UiArrayBinding,
        Kind_UiImport,
        Kind_UiObjectBinding,
        Kind_UiObjectDefinition,
        Kind_UiInlineComponent,
        Kind_UiObjectInitializer,
        Kind_UiObjectMemberList,
        Kind_UiArrayMemberList,
        Kind_UiPragmaValueList,
        Kind_UiPragma,
        Kind_UiProgram,
        Kind_UiParameterList,
        Kind_UiPropertyAttributes,
        Kind_UiPublicMember,
        Kind_UiQualifiedId,
        Kind_UiScriptBinding,
        Kind_UiSourceElement,
        Kind_UiHeaderItemList,
        Kind_UiEnumDeclaration,
        Kind_UiEnumMemberList,
        Kind_UiVersionSpecifier,
        Kind_UiRequired,
        Kind_UiAnnotation,
        Kind_UiAnnotationList
    };

    inline Node() {}

    // NOTE: node destructors are never called,
    //       instead we block free the memory
    //       (see the NodePool class)
    virtual ~Node() {}

    virtual ExpressionNode *expressionCast();
    virtual BinaryExpression *binaryExpressionCast();
    virtual Statement *statementCast();
    virtual UiObjectMember *uiObjectMemberCast();
    virtual LeftHandSideExpression *leftHandSideExpressionCast();
    virtual Pattern *patternCast();
    // implements the IsFunctionDefinition rules in the spec
    virtual FunctionExpression *asFunctionDefinition();
    virtual ClassExpression *asClassDefinition();

    bool ignoreRecursionDepth() const;

    inline void accept(BaseVisitor *visitor)
    {
        BaseVisitor::RecursionDepthCheck recursionCheck(visitor);

        // Stack overflow is uncommon, ignoreRecursionDepth() only returns true if
        // QV4_CRASH_ON_STACKOVERFLOW is set, and ignoreRecursionDepth() needs to be out of line.
        // Therefore, check for ignoreRecursionDepth() _after_ calling the inline recursionCheck().
        if (recursionCheck() || ignoreRecursionDepth()) {
            if (visitor->preVisit(this))
                accept0(visitor);
            visitor->postVisit(this);
        } else {
            visitor->throwRecursionDepthError();
        }
    }

    inline static void accept(Node *node, BaseVisitor *visitor)
    {
        if (node)
            node->accept(visitor);
    }

    virtual void accept0(BaseVisitor *visitor) = 0;
    virtual SourceLocation firstSourceLocation() const = 0;
    virtual SourceLocation lastSourceLocation() const = 0;

// attributes
    int kind = Kind_Undefined;
};

template<typename T>
T lastListElement(T head)
{
    auto current = head;
    while (current->next)
        current = current->next;
    return current;
}

class QML_PARSER_EXPORT UiQualifiedId: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiQualifiedId)

    UiQualifiedId(QStringView name)
        : next(this), name(name)
    { kind = K; }

    UiQualifiedId(UiQualifiedId *previous, QStringView name)
        : name(name)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiQualifiedId *finish()
    {
        UiQualifiedId *head = next;
        next = nullptr;
        return head;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    {
        return lastListElement(this)->lastOwnSourceLocation();
    }

    SourceLocation lastOwnSourceLocation() const { return identifierToken; }

    QString toString() const
    {
        QString result;
        toString(&result);
        return result;
    }

    void toString(QString *out) const
    {
        for (const UiQualifiedId *it = this; it; it = it->next) {
            out->append(it->name);
            if (it->next)
                out->append(QLatin1Char('.'));
        }
    }

// attributes
    UiQualifiedId *next;
    QStringView name;
    SourceLocation identifierToken;
    SourceLocation dotToken;
};

class QML_PARSER_EXPORT Type: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Type)

    Type(UiQualifiedId *typeId, Type *typeArgument = nullptr)
        : typeId(typeId)
        , typeArgument(typeArgument ? typeArgument->typeId : nullptr)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return typeId->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return typeArgument ? typeArgument->lastSourceLocation() : typeId->lastSourceLocation(); }

    QString toString() const;
    void toString(QString *out) const;

// attributes
    UiQualifiedId *typeId;
    UiQualifiedId *typeArgument;
};

class QML_PARSER_EXPORT TypeAnnotation: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(TypeAnnotation)

    TypeAnnotation(Type *type)
        : type(type)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return colonToken; }

    SourceLocation lastSourceLocation() const override
    { return type->lastSourceLocation(); }

// attributes
    Type *type;
    SourceLocation colonToken;
};
class QML_PARSER_EXPORT ExpressionNode: public Node
{
public:
    ExpressionNode() {}

    ExpressionNode *expressionCast() override;
    bool containsOptionalChain() const;

    AST::FormalParameterList *reparseAsFormalParameterList(MemoryPool *pool);

};

class QML_PARSER_EXPORT LeftHandSideExpression : public ExpressionNode
{
    LeftHandSideExpression *leftHandSideExpressionCast() override;
};

class QML_PARSER_EXPORT Statement: public Node
{
public:
    Statement() {}

    Statement *statementCast() override;
};

class QML_PARSER_EXPORT NestedExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(NestedExpression)

    NestedExpression(ExpressionNode *expression)
        : expression(expression)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lparenToken; }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

    FunctionExpression *asFunctionDefinition() override;
    ClassExpression *asClassDefinition() override;


// attributes
    ExpressionNode *expression;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};


class QML_PARSER_EXPORT TypeExpression : public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(TypeExpression)
    TypeExpression(Type *t) : m_type(t) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override {
        return m_type->firstSourceLocation();
    }

    SourceLocation lastSourceLocation() const override {
        return m_type->lastSourceLocation();
    }

    Type *m_type;
};

class QML_PARSER_EXPORT ThisExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(ThisExpression)

    ThisExpression() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return thisToken; }

    SourceLocation lastSourceLocation() const override
    { return thisToken; }

// attributes
    SourceLocation thisToken;
};

class QML_PARSER_EXPORT IdentifierExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(IdentifierExpression)

    IdentifierExpression(QStringView n):
        name (n) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return identifierToken; }

// attributes
    QStringView name;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT NullExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(NullExpression)

    NullExpression() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return nullToken; }

    SourceLocation lastSourceLocation() const override
    { return nullToken; }

// attributes
    SourceLocation nullToken;
};

class QML_PARSER_EXPORT TrueLiteral: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(TrueLiteral)

    TrueLiteral() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return trueToken; }

    SourceLocation lastSourceLocation() const override
    { return trueToken; }

// attributes
    SourceLocation trueToken;
};

class QML_PARSER_EXPORT FalseLiteral: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(FalseLiteral)

    FalseLiteral() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return falseToken; }

    SourceLocation lastSourceLocation() const override
    { return falseToken; }

// attributes
    SourceLocation falseToken;
};

class QML_PARSER_EXPORT SuperLiteral : public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(SuperLiteral)

    SuperLiteral() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return superToken; }

    SourceLocation lastSourceLocation() const override
    { return superToken; }

// attributes
    SourceLocation superToken;
};


class QML_PARSER_EXPORT NumericLiteral: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(NumericLiteral)

    NumericLiteral(double v):
        value(v) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    double value;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT UiVersionSpecifier : public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiVersionSpecifier)

    UiVersionSpecifier(int majorum) : version(QTypeRevision::fromMajorVersion(majorum))
    {
        kind = K;
    }

    UiVersionSpecifier(int majorum, int minorum) : version(QTypeRevision::fromVersion(majorum, minorum))
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override { return majorToken; }

    SourceLocation lastSourceLocation() const override
    {
        return minorToken.isValid() ? minorToken : majorToken;
    }

    // attributes:
    QTypeRevision version;
    SourceLocation majorToken;
    SourceLocation minorToken;
};

class QML_PARSER_EXPORT StringLiteral : public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(StringLiteral)

    StringLiteral(QStringView v):
        value (v) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    QStringView value;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT TemplateLiteral : public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(TemplateLiteral)

    TemplateLiteral(QStringView str, QStringView raw, ExpressionNode *e)
        : value(str), rawValue(raw), expression(e), next(nullptr)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    {
        auto last = lastListElement(this);
        return (last->expression ? last->expression->lastSourceLocation() : last->literalToken);
    }

    void accept0(BaseVisitor *visitor) override;

    bool hasNoSubstitution = false;
    QStringView value;
    QStringView rawValue;
    ExpressionNode *expression;
    TemplateLiteral *next;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT RegExpLiteral: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(RegExpLiteral)

    RegExpLiteral(QStringView p, int f):
        pattern (p), flags (f) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return literalToken; }

    SourceLocation lastSourceLocation() const override
    { return literalToken; }

// attributes:
    QStringView pattern;
    int flags;
    SourceLocation literalToken;
};

class QML_PARSER_EXPORT Pattern : public LeftHandSideExpression
{
public:
    enum ParseMode {
        Literal,
        Binding
    };
    Pattern *patternCast() override;
    virtual bool convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage) = 0;
    ParseMode parseMode = Literal;
};

class QML_PARSER_EXPORT ArrayPattern : public Pattern
{
public:
    QQMLJS_DECLARE_AST_NODE(ArrayPattern)

    ArrayPattern(PatternElementList *elts)
        : elements(elts)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbracketToken; }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

    bool isValidArrayLiteral(SourceLocation *errorLocation = nullptr) const;

    bool convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage) override;

// attributes
    PatternElementList *elements = nullptr;
    SourceLocation lbracketToken;
    SourceLocation commaToken;
    SourceLocation rbracketToken;
};

class QML_PARSER_EXPORT ObjectPattern : public Pattern
{
public:
    QQMLJS_DECLARE_AST_NODE(ObjectPattern)

    ObjectPattern()
        { kind = K; }

    ObjectPattern(PatternPropertyList *plist)
        : properties(plist)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    bool convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage) override;

// attributes
    PatternPropertyList *properties = nullptr;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT Elision: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Elision)

    Elision():
        next (this) { kind = K; }

    Elision(Elision *previous)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return commaToken; }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->commaToken; }

    inline Elision *finish ()
    {
        Elision *front = next;
        next = nullptr;
        return front;
    }

// attributes
    Elision *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT PropertyName: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(PropertyName)

    PropertyName() { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return propertyNameToken; }

    SourceLocation lastSourceLocation() const override
    { return propertyNameToken; }

    virtual QString asString() const = 0;

// attributes
    SourceLocation propertyNameToken;
};

struct QML_PARSER_EXPORT BoundName
{
    enum Type {
        Declared,
        Injected,
    };

    QString id;
    QQmlJS::SourceLocation location;
    QTaggedPointer<TypeAnnotation, Type> typeAnnotation;
    BoundName(const QString &id, const QQmlJS::SourceLocation &location,
              TypeAnnotation *typeAnnotation, Type type = Declared)
        : id(id), location(location), typeAnnotation(typeAnnotation, type)
    {}
    BoundName() = default;

    bool isInjected() const { return typeAnnotation.tag() == Injected; }
};

struct BoundNames : public QVector<BoundName>
{
    int indexOf(const QString &name, int from = 0) const
    {
        auto found = std::find_if(constBegin() + from, constEnd(),
                                  [name](const BoundName &it) { return it.id == name; });
        if (found == constEnd())
            return -1;
        return found - constBegin();
    }

    bool contains(const QString &name) const
    {
        return indexOf(name) != -1;
    }
};

/*!
\internal
This class is needed to pass the information about the equalToken in the parser, and is only needed
during AST construction. It behaves exactly like the expression it contains: that avoids changing
all the usages in qqmljs.g from ExpressionNode to InitializerExpression for every rule expecting a
InitializerOpt_In or InitializerOpt.
*/
class QML_PARSER_EXPORT InitializerExpression : public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(InitializerExpression)

    InitializerExpression(ExpressionNode *e) : expression(e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return equalToken; }

    SourceLocation lastSourceLocation() const override { return expression->lastSourceLocation(); }

    FunctionExpression *asFunctionDefinition() override
    {
        return expression->asFunctionDefinition();
    }

    ClassExpression *asClassDefinition() override { return expression->asClassDefinition(); }

    // attributes
    ExpressionNode *expression;
    SourceLocation equalToken;
};

class QML_PARSER_EXPORT PatternElement : public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(PatternElement)

    enum Type {
        // object literal types
        Literal,
        Method,
        Getter,
        Setter,

        // used by both bindings and literals
        SpreadElement,
        RestElement = SpreadElement,

        // binding types
        Binding,
    };

private:
    /*!
    \internal
    Hide InitializerExpression from the AST. InitializerExpression is only needed during parsing for
    the AST construction, and it is not possible for the parser to directly embed the location of
    equal tokens inside the PatternElement without the InitializerExpression.
    */
    void unwrapInitializer()
    {
        if (auto unwrapped = AST::cast<InitializerExpression *>(initializer)) {
            equalToken = unwrapped->equalToken;
            initializer = unwrapped->expression;
        }
    }
public:

    PatternElement(ExpressionNode *i = nullptr, Type t = Literal)
        : initializer(i), type(t)
    {
        kind = K;
        unwrapInitializer();
    }

    PatternElement(QStringView n, TypeAnnotation *typeAnnotation = nullptr, ExpressionNode *i = nullptr, Type t = Binding)
        : bindingIdentifier(n), initializer(i), type(t)
        , typeAnnotation(typeAnnotation)
    {
        Q_ASSERT(t >= RestElement);
        kind = K;
        unwrapInitializer();
    }

    PatternElement(Pattern *pattern, ExpressionNode *i = nullptr, Type t = Binding)
        : bindingTarget(pattern), initializer(i), type(t)
    {
        Q_ASSERT(t >= RestElement);
        kind = K;
        unwrapInitializer();
    }

    void accept0(BaseVisitor *visitor) override;
    virtual bool convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage);

    SourceLocation firstSourceLocation() const override
    { return identifierToken.isValid() ? identifierToken : (bindingTarget ? bindingTarget->firstSourceLocation() : initializer->firstSourceLocation()); }

    SourceLocation lastSourceLocation() const override
    { return initializer ? initializer->lastSourceLocation() : (bindingTarget ? bindingTarget->lastSourceLocation() : (typeAnnotation ? typeAnnotation->lastSourceLocation() : identifierToken)); }

    ExpressionNode *destructuringTarget() const { return bindingTarget; }
    Pattern *destructuringPattern() const { return bindingTarget ? bindingTarget->patternCast() : nullptr; }
    PatternElementList *elementList() const { ArrayPattern *a = cast<ArrayPattern *>(bindingTarget); return a ? a->elements : nullptr; }
    PatternPropertyList *propertyList() const { ObjectPattern *o = cast<ObjectPattern *>(bindingTarget); return o ? o->properties : nullptr;  }

    bool isVariableDeclaration() const { return scope != VariableScope::NoScope; }
    bool isLexicallyScoped() const { return scope == VariableScope::Let || scope == VariableScope::Const; }

    virtual void boundNames(BoundNames *names);

// attributes
    SourceLocation identifierToken;
    SourceLocation equalToken;
    QStringView bindingIdentifier;
    ExpressionNode *bindingTarget = nullptr;
    ExpressionNode *initializer = nullptr;
    Type type = Literal;
    TypeAnnotation *typeAnnotation = nullptr;
    // when used in a VariableDeclarationList
    SourceLocation declarationKindToken;
    VariableScope scope = VariableScope::NoScope;
    bool isForDeclaration = false;
    bool isInjectedSignalParameter = false;
};

class QML_PARSER_EXPORT PatternElementList : public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(PatternElementList)

    PatternElementList(Elision *elision, PatternElement *element)
        : elision(elision), element(element), next(this)
    { kind = K; }

    PatternElementList *append(PatternElementList *n) {
        n->next = next;
        next = n;
        return n;
    }

    inline PatternElementList *finish ()
    {
        PatternElementList *front = next;
        next = 0;
        return front;
    }

    void accept0(BaseVisitor *visitor) override;

    void boundNames(BoundNames *names);

    SourceLocation firstSourceLocation() const override
    { return elision ? elision->firstSourceLocation() : element->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        auto last = lastListElement(this);
        return last->element ? last->element->lastSourceLocation() : last->elision->lastSourceLocation();
    }

    Elision *elision = nullptr;
    PatternElement *element = nullptr;
    PatternElementList *next;
};

class QML_PARSER_EXPORT PatternProperty : public PatternElement
{
public:
    QQMLJS_DECLARE_AST_NODE(PatternProperty)

    PatternProperty(PropertyName *name, ExpressionNode *i = nullptr, Type t = Literal)
        : PatternElement(i, t), name(name)
    { kind = K; }

    PatternProperty(PropertyName *name, QStringView n, ExpressionNode *i = nullptr)
        : PatternElement(n, /*type annotation*/nullptr, i), name(name)
    { kind = K; }

    PatternProperty(PropertyName *name, Pattern *pattern, ExpressionNode *i = nullptr)
        : PatternElement(pattern, i), name(name)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return name->firstSourceLocation(); }
    SourceLocation lastSourceLocation() const override
    {
        SourceLocation loc = PatternElement::lastSourceLocation();
        return loc.isValid() ? loc : name->lastSourceLocation();
    }

    void boundNames(BoundNames *names) override;
    bool convertLiteralToAssignmentPattern(MemoryPool *pool, SourceLocation *errorLocation, QString *errorMessage) override;

// attributes
    PropertyName *name;
    SourceLocation colonToken;
};


class QML_PARSER_EXPORT PatternPropertyList : public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(PatternPropertyList)

    PatternPropertyList(PatternProperty *property)
        : property(property), next(this)
    { kind = K; }

    PatternPropertyList(PatternPropertyList *previous, PatternProperty *property)
        : property(property), next(this)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    void boundNames(BoundNames *names);

    inline PatternPropertyList *finish ()
    {
        PatternPropertyList *front = next;
        next = 0;
        return front;
    }

    SourceLocation firstSourceLocation() const override
    { return property->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->property->lastSourceLocation(); }

    PatternProperty *property;
    PatternPropertyList *next;
};

class QML_PARSER_EXPORT IdentifierPropertyName: public PropertyName
{
public:
    QQMLJS_DECLARE_AST_NODE(IdentifierPropertyName)

    IdentifierPropertyName(QStringView n):
        id (n) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    QString asString() const override { return id.toString(); }

// attributes
    QStringView id;
};

class QML_PARSER_EXPORT StringLiteralPropertyName: public PropertyName
{
public:
    QQMLJS_DECLARE_AST_NODE(StringLiteralPropertyName)

    StringLiteralPropertyName(QStringView n):
        id (n) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    QString asString() const override { return id.toString(); }

// attributes
    QStringView id;
};

class QML_PARSER_EXPORT NumericLiteralPropertyName: public PropertyName
{
public:
    QQMLJS_DECLARE_AST_NODE(NumericLiteralPropertyName)

    NumericLiteralPropertyName(double n):
        id (n) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    QString asString() const override;

// attributes
    double id;
};

class QML_PARSER_EXPORT ComputedPropertyName : public PropertyName
{
public:
    QQMLJS_DECLARE_AST_NODE(ComputedPropertyName)

    ComputedPropertyName(ExpressionNode *expression)
        : expression(expression)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    QString asString() const override { return QString(); }

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
};


class QML_PARSER_EXPORT ArrayMemberExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(ArrayMemberExpression)

    ArrayMemberExpression(ExpressionNode *b, ExpressionNode *e):
        base (b), expression (e)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

// attributes
    ExpressionNode *base;
    ExpressionNode *expression;
    SourceLocation optionalToken;
    SourceLocation lbracketToken;
    SourceLocation rbracketToken;
    bool isOptional = false;
};

class QML_PARSER_EXPORT FieldMemberExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(FieldMemberExpression)

    FieldMemberExpression(ExpressionNode *b, QStringView n):
        base (b), name (n)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return identifierToken; }

    // attributes
    ExpressionNode *base;
    QStringView name;
    SourceLocation dotToken;
    SourceLocation identifierToken;
    bool isOptional = false;
};

class QML_PARSER_EXPORT TaggedTemplate : public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(TaggedTemplate)

    TaggedTemplate(ExpressionNode *b, TemplateLiteral *t)
        : base (b), templateLiteral(t)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return templateLiteral->lastSourceLocation(); }

    // attributes
    ExpressionNode *base;
    TemplateLiteral *templateLiteral;
};

class QML_PARSER_EXPORT NewMemberExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(NewMemberExpression)

    NewMemberExpression(ExpressionNode *b, ArgumentList *a):
        base (b), arguments (a)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return newToken; }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

    // attributes
    ExpressionNode *base;
    ArgumentList *arguments;
    SourceLocation newToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT NewExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(NewExpression)

    NewExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return newToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation newToken;
};

class QML_PARSER_EXPORT CallExpression: public LeftHandSideExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(CallExpression)

    CallExpression(ExpressionNode *b, ArgumentList *a):
        base (b), arguments (a)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return rparenToken; }

// attributes
    ExpressionNode *base;
    ArgumentList *arguments;
    SourceLocation optionalToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    bool isOptional = false;
};

class QML_PARSER_EXPORT ArgumentList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ArgumentList)

    ArgumentList(ExpressionNode *e):
        expression (e), next (this)
        { kind = K; }

    ArgumentList(ArgumentList *previous, ExpressionNode *e):
        expression (e)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return expression->lastSourceLocation();
    }

    inline ArgumentList *finish ()
    {
        ArgumentList *front = next;
        next = nullptr;
        return front;
    }

// attributes
    ExpressionNode *expression;
    ArgumentList *next;
    SourceLocation commaToken;
    bool isSpreadElement = false;
};

class QML_PARSER_EXPORT PostIncrementExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(PostIncrementExpression)

    PostIncrementExpression(ExpressionNode *b):
        base (b) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return incrementToken; }

// attributes
    ExpressionNode *base;
    SourceLocation incrementToken;
};

class QML_PARSER_EXPORT PostDecrementExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(PostDecrementExpression)

    PostDecrementExpression(ExpressionNode *b):
        base (b) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return base->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return decrementToken; }

// attributes
    ExpressionNode *base;
    SourceLocation decrementToken;
};

class QML_PARSER_EXPORT DeleteExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(DeleteExpression)

    DeleteExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return deleteToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation deleteToken;
};

class QML_PARSER_EXPORT VoidExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(VoidExpression)

    VoidExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return voidToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation voidToken;
};

class QML_PARSER_EXPORT TypeOfExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(TypeOfExpression)

    TypeOfExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return typeofToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation typeofToken;
};

class QML_PARSER_EXPORT PreIncrementExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(PreIncrementExpression)

    PreIncrementExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return incrementToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation incrementToken;
};

class QML_PARSER_EXPORT PreDecrementExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(PreDecrementExpression)

    PreDecrementExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return decrementToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation decrementToken;
};

class QML_PARSER_EXPORT UnaryPlusExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(UnaryPlusExpression)

    UnaryPlusExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return plusToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation plusToken;
};

class QML_PARSER_EXPORT UnaryMinusExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(UnaryMinusExpression)

    UnaryMinusExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return minusToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation minusToken;
};

class QML_PARSER_EXPORT TildeExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(TildeExpression)

    TildeExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return tildeToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation tildeToken;
};

class QML_PARSER_EXPORT NotExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(NotExpression)

    NotExpression(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return notToken; }

    SourceLocation lastSourceLocation() const override
    { return expression->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    SourceLocation notToken;
};

class QML_PARSER_EXPORT BinaryExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(BinaryExpression)

    BinaryExpression(ExpressionNode *l, int o, ExpressionNode *r):
        left (l), op (o), right (r)
        { kind = K; }

    BinaryExpression *binaryExpressionCast() override;

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return left->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return right->lastSourceLocation(); }

// attributes
    ExpressionNode *left;
    int op;
    ExpressionNode *right;
    SourceLocation operatorToken;
};

class QML_PARSER_EXPORT ConditionalExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(ConditionalExpression)

    ConditionalExpression(ExpressionNode *e, ExpressionNode *t, ExpressionNode *f):
        expression (e), ok (t), ko (f)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return ko->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    ExpressionNode *ok;
    ExpressionNode *ko;
    SourceLocation questionToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT Expression: public ExpressionNode // ### rename
{
public:
    QQMLJS_DECLARE_AST_NODE(Expression)

    Expression(ExpressionNode *l, ExpressionNode *r):
        left (l), right (r) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return left->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return right->lastSourceLocation(); }

// attributes
    ExpressionNode *left;
    ExpressionNode *right;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT Block: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(Block)

    Block(StatementList *slist):
        statements (slist) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    // attributes
    StatementList *statements;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT StatementList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(StatementList)

    // ### This should be a Statement, but FunctionDeclaration currently doesn't inherit it.
    StatementList(Node *stmt)
        : statement(stmt), next (this)
    { kind = K; }

    StatementList *append(StatementList *n) {
        n->next = next;
        next = n;
        return n;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return statement->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        return lastListElement(this)->statement->lastSourceLocation();
    }

    inline StatementList *finish ()
    {
        StatementList *front = next;
        next = nullptr;
        return front;
    }

// attributes
    Node *statement = nullptr;
    StatementList *next;
};

class QML_PARSER_EXPORT VariableDeclarationList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(VariableDeclarationList)

    VariableDeclarationList(PatternElement *decl)
        : declaration(decl), next(this)
    { kind = K; }

    VariableDeclarationList(VariableDeclarationList *previous, PatternElement *decl)
        : declaration(decl)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return declaration->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return declaration->lastSourceLocation();
    }

    inline VariableDeclarationList *finish(VariableScope s)
    {
        VariableDeclarationList *front = next;
        next = nullptr;
        VariableDeclarationList *vdl;
        for (vdl = front; vdl != nullptr; vdl = vdl->next) {
            vdl->declaration->scope = s;
        }
        return front;
    }

// attributes
    PatternElement *declaration;
    VariableDeclarationList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT VariableStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(VariableStatement)

    VariableStatement(VariableDeclarationList *vlist):
        declarations (vlist)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return declarationKindToken; }

    SourceLocation lastSourceLocation() const override
    { return declarations->lastSourceLocation(); }

// attributes
    VariableDeclarationList *declarations;
    SourceLocation declarationKindToken;
};

class QML_PARSER_EXPORT EmptyStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(EmptyStatement)

    EmptyStatement() { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return semicolonToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT ExpressionStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ExpressionStatement)

    ExpressionStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return expression->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    ExpressionNode *expression;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT IfStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(IfStatement)

    IfStatement(ExpressionNode *e, Statement *t, Statement *f = nullptr):
        expression (e), ok (t), ko (f)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return ifToken; }

    SourceLocation lastSourceLocation() const override
    {
        if (ko)
            return ko->lastSourceLocation();

        return ok->lastSourceLocation();
    }

// attributes
    ExpressionNode *expression;
    Statement *ok;
    Statement *ko;
    SourceLocation ifToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation elseToken;
};

class QML_PARSER_EXPORT DoWhileStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(DoWhileStatement)

    DoWhileStatement(Statement *stmt, ExpressionNode *e):
        statement (stmt), expression (e)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return doToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    Statement *statement;
    ExpressionNode *expression;
    SourceLocation doToken;
    SourceLocation whileToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT WhileStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(WhileStatement)

    WhileStatement(ExpressionNode *e, Statement *stmt):
        expression (e), statement (stmt)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return whileToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation whileToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT ForStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ForStatement)

    ForStatement(ExpressionNode *i, ExpressionNode *c, ExpressionNode *e, Statement *stmt):
        initialiser (i), condition (c), expression (e), statement (stmt)
        { kind = K; }

    ForStatement(VariableDeclarationList *vlist, ExpressionNode *c, ExpressionNode *e, Statement *stmt):
        declarations (vlist), condition (c), expression (e), statement (stmt)
        { kind = K; }


    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *initialiser = nullptr;
    VariableDeclarationList *declarations = nullptr;
    ExpressionNode *condition;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation firstSemicolonToken;
    SourceLocation secondSemicolonToken;
    SourceLocation rparenToken;
};

enum class ForEachType {
    In,
    Of
};

class QML_PARSER_EXPORT ForEachStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ForEachStatement)

    ForEachStatement(ExpressionNode *i, ExpressionNode *e, Statement *stmt)
        : lhs(i), expression(e), statement(stmt)
    { kind = K; }
    ForEachStatement(PatternElement *v, ExpressionNode *e, Statement *stmt)
        : lhs(v), expression(e), statement(stmt)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return forToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

    PatternElement *declaration() const {
        return AST::cast<PatternElement *>(lhs);
    }

// attributes
    Node *lhs;
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation forToken;
    SourceLocation lparenToken;
    SourceLocation inOfToken;
    SourceLocation rparenToken;
    ForEachType type;
};

class QML_PARSER_EXPORT ContinueStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ContinueStatement)

    ContinueStatement(QStringView l = QStringView()):
        label (l) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return continueToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    QStringView label;
    SourceLocation continueToken;
    SourceLocation identifierToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT BreakStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(BreakStatement)

    BreakStatement(QStringView l):
        label (l) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return breakToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

    // attributes
    QStringView label;
    SourceLocation breakToken;
    SourceLocation identifierToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT ReturnStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ReturnStatement)

    ReturnStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return returnToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    ExpressionNode *expression;
    SourceLocation returnToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT YieldExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(YieldExpression)

    YieldExpression(ExpressionNode *e = nullptr):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return yieldToken; }

    SourceLocation lastSourceLocation() const override
    { return expression ? expression->lastSourceLocation() : yieldToken; }

// attributes
    ExpressionNode *expression;
    bool isYieldStar = false;
    SourceLocation yieldToken;
};

class QML_PARSER_EXPORT WithStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(WithStatement)

    WithStatement(ExpressionNode *e, Statement *stmt):
        expression (e), statement (stmt)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return withToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    ExpressionNode *expression;
    Statement *statement;
    SourceLocation withToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT CaseBlock: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(CaseBlock)

    CaseBlock(CaseClauses *c, DefaultClause *d = nullptr, CaseClauses *r = nullptr):
        clauses (c), defaultClause (d), moreClauses (r)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    CaseClauses *clauses;
    DefaultClause *defaultClause;
    CaseClauses *moreClauses;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT SwitchStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(SwitchStatement)

    SwitchStatement(ExpressionNode *e, CaseBlock *b):
        expression (e), block (b)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return switchToken; }

    SourceLocation lastSourceLocation() const override
    { return block->rbraceToken; }

// attributes
    ExpressionNode *expression;
    CaseBlock *block;
    SourceLocation switchToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT CaseClause: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(CaseClause)

    CaseClause(ExpressionNode *e, StatementList *slist):
        expression (e), statements (slist)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return caseToken; }

    SourceLocation lastSourceLocation() const override
    { return statements ? statements->lastSourceLocation() : colonToken; }

// attributes
    ExpressionNode *expression;
    StatementList *statements;
    SourceLocation caseToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT CaseClauses: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(CaseClauses)

    CaseClauses(CaseClause *c):
        clause (c), next (this)
        { kind = K; }

    CaseClauses(CaseClauses *previous, CaseClause *c):
        clause (c)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return clause->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        return lastListElement(this)->clause->lastSourceLocation();
    }

    inline CaseClauses *finish ()
    {
        CaseClauses *front = next;
        next = nullptr;
        return front;
    }

//attributes
    CaseClause *clause;
    CaseClauses *next;
};

class QML_PARSER_EXPORT DefaultClause: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(DefaultClause)

    DefaultClause(StatementList *slist):
        statements (slist)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return defaultToken; }

    SourceLocation lastSourceLocation() const override
    { return statements ? statements->lastSourceLocation() : colonToken; }

// attributes
    StatementList *statements;
    SourceLocation defaultToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT LabelledStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(LabelledStatement)

    LabelledStatement(QStringView l, Statement *stmt):
        label (l), statement (stmt)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    QStringView label;
    Statement *statement;
    SourceLocation identifierToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT ThrowStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ThrowStatement)

    ThrowStatement(ExpressionNode *e):
        expression (e) { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return throwToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

    // attributes
    ExpressionNode *expression;
    SourceLocation throwToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT Catch: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Catch)

    Catch(PatternElement *p, Block *stmt)
        : patternElement(p), statement(stmt)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return catchToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

// attributes
    PatternElement *patternElement;
    Block *statement;
    SourceLocation catchToken;
    SourceLocation lparenToken;
    SourceLocation identifierToken;
    SourceLocation rparenToken;
};

class QML_PARSER_EXPORT Finally: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Finally)

    Finally(Block *stmt):
        statement (stmt)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return finallyToken; }

    SourceLocation lastSourceLocation() const override
    { return statement ? statement->lastSourceLocation() : finallyToken; }

// attributes
    Block *statement;
    SourceLocation finallyToken;
};

class QML_PARSER_EXPORT TryStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(TryStatement)

    TryStatement(Statement *stmt, Catch *c, Finally *f):
        statement (stmt), catchExpression (c), finallyExpression (f)
        { kind = K; }

    TryStatement(Statement *stmt, Finally *f):
        statement (stmt), catchExpression (nullptr), finallyExpression (f)
        { kind = K; }

    TryStatement(Statement *stmt, Catch *c):
        statement (stmt), catchExpression (c), finallyExpression (nullptr)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return tryToken; }

    SourceLocation lastSourceLocation() const override
    {
        if (finallyExpression)
            return finallyExpression->statement->rbraceToken;
        else if (catchExpression)
            return catchExpression->statement->rbraceToken;

        return statement->lastSourceLocation();
    }

// attributes
    Statement *statement;
    Catch *catchExpression;
    Finally *finallyExpression;
    SourceLocation tryToken;
};

class QML_PARSER_EXPORT FunctionExpression: public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(FunctionExpression)

    FunctionExpression(QStringView n, FormalParameterList *f, StatementList *b, TypeAnnotation *typeAnnotation = nullptr):
        name (n), formals (f), body (b),
        typeAnnotation(typeAnnotation)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return functionToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    FunctionExpression *asFunctionDefinition() override;

// attributes
    QStringView name;
    bool isArrowFunction = false;
    bool isGenerator = false;
    FormalParameterList *formals;
    StatementList *body;
    TypeAnnotation *typeAnnotation;
    SourceLocation functionToken;
    // for generators:
    SourceLocation starToken;
    SourceLocation identifierToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT FunctionDeclaration: public FunctionExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(FunctionDeclaration)

    FunctionDeclaration(QStringView n, FormalParameterList *f, StatementList *b, TypeAnnotation *typeAnnotation = nullptr):
        FunctionExpression(n, f, b, typeAnnotation)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;
};

class QML_PARSER_EXPORT FormalParameterList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(FormalParameterList)

    FormalParameterList(FormalParameterList *previous, PatternElement *e)
        : element(e)
    {
        kind = K;
        if (previous) {
            next = previous->next;
            previous->next = this;
        } else {
            next = this;
        }
    }

    FormalParameterList *append(FormalParameterList *n) {
        n->next = next;
        next = n;
        return n;
    }

    bool isSimpleParameterList()
    {
        AST::FormalParameterList *formals = this;
        while (formals) {
            PatternElement *e = formals->element;
            if (e && e->type == PatternElement::RestElement)
                return false;
            if (e && (e->initializer || e->bindingTarget))
                return false;
            formals = formals->next;
        }
        return true;
    }

    int length()
    {
        // the length property of Function objects
        int l = 0;
        AST::FormalParameterList *formals = this;
        while (formals) {
            PatternElement *e = formals->element;
            if (!e || e->initializer)
                break;
            if (e->type == PatternElement::RestElement)
                break;
            ++l;
            formals = formals->next;
        }
        return l;
    }

    bool containsName(const QString &name) const {
        for (const FormalParameterList *it = this; it; it = it->next) {
            PatternElement *b = it->element;
            // ### handle binding patterns
            if (b && b->bindingIdentifier == name)
                return true;
        }
        return false;
    }

    BoundNames formals() const;

    BoundNames boundNames() const;

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return element->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        return lastListElement(this)->element->lastSourceLocation();
    }

    FormalParameterList *finish(MemoryPool *pool);

// attributes
    PatternElement *element = nullptr;
    FormalParameterList *next;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT ClassExpression : public ExpressionNode
{
public:
    QQMLJS_DECLARE_AST_NODE(ClassExpression)

    ClassExpression(QStringView n, ExpressionNode *heritage, ClassElementList *elements)
        : name(n), heritage(heritage), elements(elements)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return classToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    ClassExpression *asClassDefinition() override;

// attributes
    QStringView name;
    ExpressionNode *heritage;
    ClassElementList *elements;
    SourceLocation classToken;
    SourceLocation identifierToken;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT ClassDeclaration: public ClassExpression
{
public:
    QQMLJS_DECLARE_AST_NODE(ClassDeclaration)

    ClassDeclaration(QStringView n, ExpressionNode *heritage, ClassElementList *elements)
        : ClassExpression(n, heritage, elements)
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;
};


class QML_PARSER_EXPORT ClassElementList : public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ClassElementList)

    ClassElementList(PatternProperty *property, bool isStatic)
        : isStatic(isStatic), property(property)
    {
        kind = K;
        next = this;
    }

    ClassElementList *append(ClassElementList *n) {
        n->next = next;
        next = n;
        return n;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return property->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    {
        if (next)
            return next->lastSourceLocation();
        return property->lastSourceLocation();
    }

    ClassElementList *finish();

    bool isStatic;
    ClassElementList *next;
    PatternProperty *property;
};

class QML_PARSER_EXPORT Program: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Program)

    Program(StatementList *statements)
        : statements(statements)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return statements ? statements->firstSourceLocation() : SourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return statements ? statements->lastSourceLocation() : SourceLocation(); }

// attributes
    StatementList *statements;
};

class QML_PARSER_EXPORT ImportSpecifier: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ImportSpecifier)

    ImportSpecifier(QStringView importedBinding)
        : importedBinding(importedBinding)
    {
        kind = K;
    }

    ImportSpecifier(QStringView identifier, QStringView importedBinding)
        : identifier(identifier), importedBinding(importedBinding)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifier.isNull() ? importedBindingToken : identifierToken; }
    SourceLocation lastSourceLocation() const override
    { return importedBindingToken; }

// attributes
    SourceLocation identifierToken;
    SourceLocation importedBindingToken;
    QStringView identifier;
    QStringView importedBinding;
};

class QML_PARSER_EXPORT ImportsList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ImportsList)

    ImportsList(ImportSpecifier *importSpecifier)
        : importSpecifier(importSpecifier)
    {
        kind = K;
        next = this;
    }

    ImportsList(ImportsList *previous, ImportSpecifier *importSpecifier)
        : importSpecifier(importSpecifier)
    {
        kind = K;
        if (previous) {
            next = previous->next;
            previous->next = this;
        } else {
            next = this;
        }
    }

    ImportsList *finish()
    {
        ImportsList *head = next;
        next = nullptr;
        return head;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return importSpecifierToken; }

    SourceLocation lastSourceLocation() const override
    {
        return lastListElement(this)->importSpecifierToken;
    }

// attributes
    SourceLocation importSpecifierToken;
    ImportSpecifier *importSpecifier;
    ImportsList *next = this;
};

class QML_PARSER_EXPORT NamedImports: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(NamedImports)

    NamedImports()
    {
        kind = K;
    }

    NamedImports(ImportsList *importsList)
        : importsList(importsList)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return leftBraceToken; }
    SourceLocation lastSourceLocation() const override
    { return rightBraceToken; }

// attributes
    SourceLocation leftBraceToken;
    SourceLocation rightBraceToken;
    ImportsList *importsList = nullptr;
};

class QML_PARSER_EXPORT NameSpaceImport: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(NameSpaceImport)

    NameSpaceImport(QStringView importedBinding)
        : importedBinding(importedBinding)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    virtual SourceLocation firstSourceLocation() const override
    { return starToken; }
    virtual SourceLocation lastSourceLocation() const override
    { return importedBindingToken; }

// attributes
    SourceLocation starToken;
    SourceLocation importedBindingToken;
    QStringView importedBinding;
};

class QML_PARSER_EXPORT ImportClause: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ImportClause)

    ImportClause(QStringView importedDefaultBinding)
        : importedDefaultBinding(importedDefaultBinding)
    {
        kind = K;
    }

    ImportClause(NameSpaceImport *nameSpaceImport)
        : nameSpaceImport(nameSpaceImport)
    {
        kind = K;
    }

    ImportClause(NamedImports *namedImports)
        : namedImports(namedImports)
    {
        kind = K;
    }

    ImportClause(QStringView importedDefaultBinding, NameSpaceImport *nameSpaceImport)
        : importedDefaultBinding(importedDefaultBinding)
        , nameSpaceImport(nameSpaceImport)
    {
        kind = K;
    }

    ImportClause(QStringView importedDefaultBinding, NamedImports *namedImports)
        : importedDefaultBinding(importedDefaultBinding)
        , namedImports(namedImports)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    virtual SourceLocation firstSourceLocation() const override
    { return importedDefaultBinding.isNull() ? (nameSpaceImport ? nameSpaceImport->firstSourceLocation() : namedImports->firstSourceLocation()) :  importedDefaultBindingToken; }
    virtual SourceLocation lastSourceLocation() const override
    { return importedDefaultBinding.isNull() ? (nameSpaceImport ? nameSpaceImport->lastSourceLocation() : namedImports->lastSourceLocation()) : importedDefaultBindingToken; }

// attributes
    SourceLocation importedDefaultBindingToken;
    QStringView importedDefaultBinding;
    NameSpaceImport *nameSpaceImport = nullptr;
    NamedImports *namedImports = nullptr;
};

class QML_PARSER_EXPORT FromClause: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(FromClause)

    FromClause(QStringView moduleSpecifier)
        : moduleSpecifier(moduleSpecifier)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return fromToken; }

    SourceLocation lastSourceLocation() const override
    { return moduleSpecifierToken; }

// attributes
    SourceLocation fromToken;
    SourceLocation moduleSpecifierToken;
    QStringView moduleSpecifier;
};

class QML_PARSER_EXPORT ImportDeclaration: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ImportDeclaration)

    ImportDeclaration(ImportClause *importClause, FromClause *fromClause)
        : importClause(importClause), fromClause(fromClause)
    {
        kind = K;
    }

    ImportDeclaration(QStringView moduleSpecifier)
        : moduleSpecifier(moduleSpecifier)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return importToken; }

    SourceLocation lastSourceLocation() const override
    { return moduleSpecifier.isNull() ? fromClause->lastSourceLocation() : moduleSpecifierToken; }

// attributes
    SourceLocation importToken;
    SourceLocation moduleSpecifierToken;
    QStringView moduleSpecifier;
    ImportClause *importClause = nullptr;
    FromClause *fromClause = nullptr;
};

class QML_PARSER_EXPORT ExportSpecifier: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ExportSpecifier)

    ExportSpecifier(QStringView identifier)
        : identifier(identifier), exportedIdentifier(identifier)
    {
        kind = K;
    }

    ExportSpecifier(QStringView identifier, QStringView exportedIdentifier)
        : identifier(identifier), exportedIdentifier(exportedIdentifier)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return identifierToken; }
    SourceLocation lastSourceLocation() const override
    { return exportedIdentifierToken.isValid() ? exportedIdentifierToken : identifierToken; }

// attributes
    SourceLocation identifierToken;
    SourceLocation exportedIdentifierToken;
    QStringView identifier;
    QStringView exportedIdentifier;
};

class QML_PARSER_EXPORT ExportsList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ExportsList)

    ExportsList(ExportSpecifier *exportSpecifier)
        : exportSpecifier(exportSpecifier)
    {
        kind = K;
        next = this;
    }

    ExportsList(ExportsList *previous, ExportSpecifier *exportSpecifier)
        : exportSpecifier(exportSpecifier)
    {
        kind = K;
        if (previous) {
            next = previous->next;
            previous->next = this;
        } else {
            next = this;
        }
    }

    ExportsList *finish()
    {
        ExportsList *head = next;
        next = nullptr;
        return head;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return exportSpecifier->firstSourceLocation(); }
    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->exportSpecifier->lastSourceLocation(); }

// attributes
    ExportSpecifier *exportSpecifier;
    ExportsList *next;
};

class QML_PARSER_EXPORT ExportClause: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(ExportClause)

    ExportClause()
    {
        kind = K;
    }

    ExportClause(ExportsList *exportsList)
        : exportsList(exportsList)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return leftBraceToken; }
    SourceLocation lastSourceLocation() const override
    { return rightBraceToken; }

// attributes
    SourceLocation leftBraceToken;
    SourceLocation rightBraceToken;
    ExportsList *exportsList = nullptr;
};

class QML_PARSER_EXPORT ExportDeclaration: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(ExportDeclaration)

    ExportDeclaration(FromClause *fromClause)
        : fromClause(fromClause)
    {
        kind = K;
    }

    ExportDeclaration(ExportClause *exportClause, FromClause *fromClause)
        : exportClause(exportClause), fromClause(fromClause)
    {
        kind = K;
    }

    ExportDeclaration(ExportClause *exportClause)
        : exportClause(exportClause)
    {
        kind = K;
    }

    ExportDeclaration(bool exportDefault, Node *variableStatementOrDeclaration)
        : variableStatementOrDeclaration(variableStatementOrDeclaration)
        , exportDefault(exportDefault)
    {
        kind = K;
    }

    bool exportsAll() const
    {
        return fromClause && !exportClause;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return exportToken; }
    SourceLocation lastSourceLocation() const override
    { return fromClause ? fromClause->lastSourceLocation() : (exportClause ? exportClause->lastSourceLocation() : variableStatementOrDeclaration->lastSourceLocation()); }

// attributes
    SourceLocation exportToken;
    ExportClause *exportClause = nullptr;
    FromClause *fromClause = nullptr;
    Node *variableStatementOrDeclaration = nullptr;
    bool exportDefault = false;
};

class QML_PARSER_EXPORT ESModule: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(Module)

    ESModule(StatementList *body)
        : body(body)
    {
        kind = K;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return body ? body->firstSourceLocation() : SourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return body ? body->lastSourceLocation() : SourceLocation(); }

// attributes
    StatementList *body;
};

class QML_PARSER_EXPORT DebuggerStatement: public Statement
{
public:
    QQMLJS_DECLARE_AST_NODE(DebuggerStatement)

    DebuggerStatement()
        { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return debuggerToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    SourceLocation debuggerToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiImport: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiImport)

    UiImport(QStringView fileName)
        : fileName(fileName), importUri(nullptr)
    { kind = K; }

    UiImport(UiQualifiedId *uri)
        : importUri(uri)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return importToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    QStringView fileName;
    UiQualifiedId *importUri;
    QStringView importId;
    SourceLocation importToken;
    SourceLocation fileNameToken;
    SourceLocation asToken;
    SourceLocation importIdToken;
    SourceLocation semicolonToken;
    UiVersionSpecifier *version = nullptr;
};

class QML_PARSER_EXPORT UiObjectMember: public Node
{
public:
    SourceLocation firstSourceLocation() const override = 0;
    SourceLocation lastSourceLocation() const override = 0;

    UiObjectMember *uiObjectMemberCast() override;

// attributes
    UiAnnotationList *annotations = nullptr;
};

class QML_PARSER_EXPORT UiObjectMemberList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiObjectMemberList)

    UiObjectMemberList(UiObjectMember *member)
        : next(this), member(member)
    { kind = K; }

    UiObjectMemberList(UiObjectMemberList *previous, UiObjectMember *member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return member->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->member->lastSourceLocation(); }

    UiObjectMemberList *finish()
    {
        UiObjectMemberList *head = next;
        next = nullptr;
        return head;
    }

// attributes
    UiObjectMemberList *next;
    UiObjectMember *member;
};

class QML_PARSER_EXPORT UiPragmaValueList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiPragmaValueList)

    UiPragmaValueList(QStringView value)
        : value(value)
        , next(this)
    {
        kind = K;
    }

    UiPragmaValueList(UiPragmaValueList *previous, QStringView value)
        : value(value)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return location; }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->location; }

    UiPragmaValueList *finish()
    {
        UiPragmaValueList *head = next;
        next = nullptr;
        return head;
    }

    QStringView value;
    UiPragmaValueList *next;
    SourceLocation location;
};

class QML_PARSER_EXPORT UiPragma: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiPragma)

    UiPragma(QStringView name, UiPragmaValueList *values = nullptr)
        : name(name), values(values)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return pragmaToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

// attributes
    QStringView name;
    UiPragmaValueList *values;
    SourceLocation pragmaToken;
    SourceLocation pragmaIdToken;
    SourceLocation colonToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiRequired: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiRequired)

    UiRequired(QStringView name)
        :name(name)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return requiredToken; }

    SourceLocation lastSourceLocation() const override
    { return semicolonToken; }

    QStringView name;
    SourceLocation requiredToken;
    SourceLocation semicolonToken;
};

class QML_PARSER_EXPORT UiHeaderItemList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiHeaderItemList)

    UiHeaderItemList(UiImport *import)
        : headerItem(import), next(this)
    { kind = K; }

    UiHeaderItemList(UiPragma *pragma)
        : headerItem(pragma), next(this)
    { kind = K; }

    UiHeaderItemList(UiHeaderItemList *previous, UiImport *import)
        : headerItem(import)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiHeaderItemList(UiHeaderItemList *previous, UiPragma *pragma)
        : headerItem(pragma)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    UiHeaderItemList *finish()
    {
        UiHeaderItemList *head = next;
        next = nullptr;
        return head;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return headerItem->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->headerItem->lastSourceLocation(); }

// attributes
    Node *headerItem;
    UiHeaderItemList *next;
};

class QML_PARSER_EXPORT UiProgram: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiProgram)

    UiProgram(UiHeaderItemList *headers, UiObjectMemberList *members)
        : headers(headers), members(members)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    {
        if (headers)
            return headers->firstSourceLocation();
        else if (members)
            return members->firstSourceLocation();
        return SourceLocation();
    }

    SourceLocation lastSourceLocation() const override
    {
        if (members)
            return members->lastSourceLocation();
        else if (headers)
            return headers->lastSourceLocation();
        return SourceLocation();
    }

// attributes
    UiHeaderItemList *headers;
    UiObjectMemberList *members;
};

class QML_PARSER_EXPORT UiArrayMemberList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiArrayMemberList)

    UiArrayMemberList(UiObjectMember *member)
        : next(this), member(member)
    { kind = K; }

    UiArrayMemberList(UiArrayMemberList *previous, UiObjectMember *member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return member->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->member->lastSourceLocation(); }

    UiArrayMemberList *finish()
    {
        UiArrayMemberList *head = next;
        next = nullptr;
        return head;
    }

// attributes
    UiArrayMemberList *next;
    UiObjectMember *member;
    SourceLocation commaToken;
};

class QML_PARSER_EXPORT UiObjectInitializer: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiObjectInitializer)

    UiObjectInitializer(UiObjectMemberList *members)
        : members(members)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return lbraceToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

// attributes
    SourceLocation lbraceToken;
    UiObjectMemberList *members;
    SourceLocation rbraceToken;
};

class QML_PARSER_EXPORT UiParameterList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiParameterList)

    UiParameterList(Type *t, QStringView n):
        type (t), name (n), next (this)
        { kind = K; }

    UiParameterList(UiParameterList *previous, Type *t, QStringView n):
        type (t), name (n)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *) override;

    SourceLocation firstSourceLocation() const override
    { return colonToken.isValid() ? identifierToken : propertyTypeToken; }

    SourceLocation lastSourceLocation() const override
    {
        auto last = lastListElement(this);
        return last->lastOwnSourceLocation();
    }

    SourceLocation lastOwnSourceLocation() const
    {
        return (colonToken.isValid() ? propertyTypeToken : identifierToken);
    }

    inline UiParameterList *finish ()
    {
        UiParameterList *front = next;
        next = nullptr;
        return front;
    }

// attributes
    Type *type;
    QStringView name;
    UiParameterList *next;
    SourceLocation commaToken;
    SourceLocation propertyTypeToken;
    SourceLocation identifierToken;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT UiPropertyAttributes : public Node
{
    QQMLJS_DECLARE_AST_NODE(UiPropertyAttributes)
public:
    UiPropertyAttributes() { kind = K; }

    SourceLocation defaultToken() const { return m_defaultToken; }
    bool isDefaultMember() const { return defaultToken().isValid(); }
    SourceLocation requiredToken() const { return m_requiredToken; }
    bool isRequired() const { return requiredToken().isValid(); }
    SourceLocation readonlyToken() const { return m_readonlyToken; }
    bool isReadonly() const { return readonlyToken().isValid(); }
    SourceLocation finalToken() const { return m_finalToken; }
    bool isFinal() const { return finalToken().isValid(); }

    SourceLocation propertyToken() const { return m_propertyToken; }

    template <bool InvalidIsLargest = true>
    static bool compareLocationsByBegin(const SourceLocation * const &lhs,
                                        const SourceLocation * const &rhs)
    {
        if (lhs->isValid() && rhs->isValid())
            return lhs->begin() < rhs->begin();
        else if (lhs->isValid())
            return InvalidIsLargest;
        else
            return !InvalidIsLargest;
    }

    void accept0(BaseVisitor *) override {} // intentionally do nothing

    SourceLocation firstSourceLocation() const override;

    SourceLocation lastSourceLocation() const override;

private:
    friend class QQmlJS::Parser;
    SourceLocation m_defaultToken;
    SourceLocation m_readonlyToken;
    SourceLocation m_requiredToken;
    SourceLocation m_propertyToken;
    SourceLocation m_finalToken;
};

class QML_PARSER_EXPORT UiPublicMember: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiPublicMember)

    UiPublicMember(UiQualifiedId *memberType,
                   QStringView name)
        : type(Property), memberType(memberType), name(name), statement(nullptr), binding(nullptr), parameters(nullptr)
    { kind = K; }

    UiPublicMember(UiQualifiedId *memberType,
                   QStringView name,
                   Statement *statement)
        : type(Property), memberType(memberType), name(name), statement(statement), binding(nullptr), parameters(nullptr)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    {
      if (hasAttributes)
          return m_attributes->firstSourceLocation();
      else
        return m_propertyToken;
    }

    SourceLocation lastSourceLocation() const override
    {
      if (binding)
          return binding->lastSourceLocation();
      if (statement)
          return statement->lastSourceLocation();

      return semicolonToken;
    }

    SourceLocation defaultToken() const
    {
        return hasAttributes ? m_attributes->defaultToken() : SourceLocation {};
    }
    bool isDefaultMember() const { return defaultToken().isValid(); }

    SourceLocation requiredToken() const
    {
        return hasAttributes ? m_attributes->requiredToken() : SourceLocation {};
    }
    bool isRequired() const { return requiredToken().isValid(); }

    SourceLocation readonlyToken() const
    {
        return hasAttributes ? m_attributes->readonlyToken() : SourceLocation {};
    }
    bool isReadonly() const { return readonlyToken().isValid(); }

    SourceLocation finalToken() const
    {
        return hasAttributes ? m_attributes->finalToken() : SourceLocation {};
    }
    bool isFinal() const { return finalToken().isValid(); }

    void setAttributes(UiPropertyAttributes *attributes)
    {
        m_attributes = attributes;
        hasAttributes = true;
    }

    SourceLocation propertyToken() const
    {
        return hasAttributes ? m_attributes->propertyToken() : m_propertyToken;
    }

    void setPropertyToken(SourceLocation token)
    {
        m_propertyToken = token;
        hasAttributes = false;
    }

// attributes
    enum : bool { Signal, Property } type;
    bool hasAttributes = false;
    QStringView typeModifier;
    UiQualifiedId *memberType;
    QStringView name;
    Statement *statement; // initialized with a JS expression
    UiObjectMember *binding; // initialized with a QML object or array.
    UiParameterList *parameters;
    // TODO: merge source locations
    SourceLocation typeModifierToken;
    SourceLocation typeToken;
    SourceLocation identifierToken;
    SourceLocation colonToken;
    SourceLocation semicolonToken;
    SourceLocation lparenToken;
    SourceLocation rparenToken;
private:
    union {
        SourceLocation m_propertyToken = SourceLocation {};
        UiPropertyAttributes *m_attributes;
    };
};

class QML_PARSER_EXPORT UiObjectDefinition: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiObjectDefinition)

    UiObjectDefinition(UiQualifiedId *qualifiedTypeNameId,
                       UiObjectInitializer *initializer)
        : qualifiedTypeNameId(qualifiedTypeNameId), initializer(initializer)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return qualifiedTypeNameId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return initializer->rbraceToken; }

// attributes
    UiQualifiedId *qualifiedTypeNameId;
    UiObjectInitializer *initializer;
};

class QML_PARSER_EXPORT UiInlineComponent: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiInlineComponent)

    UiInlineComponent(QStringView inlineComponentName, UiObjectDefinition* inlineComponent)
        : name(inlineComponentName), component(inlineComponent)
    { kind = K; }

    SourceLocation lastSourceLocation() const override
    {return component->lastSourceLocation();}

    SourceLocation firstSourceLocation() const override
    {return componentToken;}

    void accept0(BaseVisitor *visitor) override;

    // attributes
    QStringView name;
    UiObjectDefinition* component;
    SourceLocation componentToken;
    SourceLocation identifierToken;
};

class QML_PARSER_EXPORT UiSourceElement: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiSourceElement)

    UiSourceElement(Node *sourceElement)
        : sourceElement(sourceElement)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    {
      if (FunctionExpression *funDecl = sourceElement->asFunctionDefinition())
        return funDecl->firstSourceLocation();
      else if (VariableStatement *varStmt = cast<VariableStatement *>(sourceElement))
        return varStmt->firstSourceLocation();

      return SourceLocation();
    }

    SourceLocation lastSourceLocation() const override
    {
      if (FunctionExpression *funDecl = sourceElement->asFunctionDefinition())
        return funDecl->lastSourceLocation();
      else if (VariableStatement *varStmt = cast<VariableStatement *>(sourceElement))
        return varStmt->lastSourceLocation();

      return SourceLocation();
    }

    void accept0(BaseVisitor *visitor) override;


// attributes
    Node *sourceElement;
};

class QML_PARSER_EXPORT UiObjectBinding: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiObjectBinding)

    UiObjectBinding(UiQualifiedId *qualifiedId,
                    UiQualifiedId *qualifiedTypeNameId,
                    UiObjectInitializer *initializer)
        : qualifiedId(qualifiedId),
          qualifiedTypeNameId(qualifiedTypeNameId),
          initializer(initializer),
          hasOnToken(false)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    {
        if (hasOnToken && qualifiedTypeNameId)
            return qualifiedTypeNameId->identifierToken;

        return qualifiedId->identifierToken;
    }

    SourceLocation lastSourceLocation() const override
    { return initializer->rbraceToken; }

    void accept0(BaseVisitor *visitor) override;


// attributes
    UiQualifiedId *qualifiedId;
    UiQualifiedId *qualifiedTypeNameId;
    UiObjectInitializer *initializer;
    SourceLocation colonToken;
    bool hasOnToken;
};

class QML_PARSER_EXPORT UiScriptBinding: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiScriptBinding)

    UiScriptBinding(UiQualifiedId *qualifiedId,
                    Statement *statement)
        : qualifiedId(qualifiedId),
          statement(statement)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return qualifiedId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return statement->lastSourceLocation(); }

    void accept0(BaseVisitor *visitor) override;

// attributes
    UiQualifiedId *qualifiedId;
    Statement *statement;
    SourceLocation colonToken;
};

class QML_PARSER_EXPORT UiArrayBinding: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiArrayBinding)

    UiArrayBinding(UiQualifiedId *qualifiedId,
                   UiArrayMemberList *members)
        : qualifiedId(qualifiedId),
          members(members)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { Q_ASSERT(qualifiedId); return qualifiedId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return rbracketToken; }

    void accept0(BaseVisitor *visitor) override;

// attributes
    UiQualifiedId *qualifiedId;
    UiArrayMemberList *members;
    SourceLocation colonToken;
    SourceLocation lbracketToken;
    SourceLocation rbracketToken;
};

class QML_PARSER_EXPORT UiEnumMemberList: public Node
{
    QQMLJS_DECLARE_AST_NODE(UiEnumMemberList)
public:
    UiEnumMemberList(QStringView member, double v = 0.0)
        : next(this), member(member), value(v)
    { kind = K; }

    UiEnumMemberList(UiEnumMemberList *previous, QStringView member)
        : member(member)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
        value = previous->value + 1;
    }

    UiEnumMemberList(UiEnumMemberList *previous, QStringView member, double v)
        : member(member), value(v)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    SourceLocation firstSourceLocation() const override
    { return memberToken; }

    SourceLocation lastSourceLocation() const override
    {
        auto last = lastListElement(this);
        return last->valueToken.isValid() ? last->valueToken : last->memberToken;
    }

    void accept0(BaseVisitor *visitor) override;

    UiEnumMemberList *finish()
    {
        UiEnumMemberList *head = next;
        next = nullptr;
        return head;
    }

// attributes
    UiEnumMemberList *next;
    QStringView member;
    double value;
    SourceLocation memberToken;
    SourceLocation valueToken;
};

class QML_PARSER_EXPORT UiEnumDeclaration: public UiObjectMember
{
public:
    QQMLJS_DECLARE_AST_NODE(UiEnumDeclaration)

    UiEnumDeclaration(QStringView name,
                      UiEnumMemberList *members)
        : name(name)
        , members(members)
    { kind = K; }

    SourceLocation firstSourceLocation() const override
    { return enumToken; }

    SourceLocation lastSourceLocation() const override
    { return rbraceToken; }

    void accept0(BaseVisitor *visitor) override;

// attributes
    SourceLocation enumToken;
    SourceLocation identifierToken;
    SourceLocation lbraceToken;
    SourceLocation rbraceToken;
    QStringView name;
    UiEnumMemberList *members;
};

class QML_PARSER_EXPORT UiAnnotation: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiAnnotation)

    UiAnnotation(UiQualifiedId *qualifiedTypeNameId,
                       UiObjectInitializer *initializer)
        : qualifiedTypeNameId(qualifiedTypeNameId), initializer(initializer)
    { kind = K; }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return qualifiedTypeNameId->identifierToken; }

    SourceLocation lastSourceLocation() const override
    { return initializer->rbraceToken; }

// attributes
    UiQualifiedId *qualifiedTypeNameId;
    UiObjectInitializer *initializer;
};

class QML_PARSER_EXPORT UiAnnotationList: public Node
{
public:
    QQMLJS_DECLARE_AST_NODE(UiAnnotationList)

    UiAnnotationList(UiAnnotation *annotation)
        : next(this), annotation(annotation)
    { kind = K; }

    UiAnnotationList(UiAnnotationList *previous, UiAnnotation *annotation)
        : annotation(annotation)
    {
        kind = K;
        next = previous->next;
        previous->next = this;
    }

    void accept0(BaseVisitor *visitor) override;

    SourceLocation firstSourceLocation() const override
    { return annotation->firstSourceLocation(); }

    SourceLocation lastSourceLocation() const override
    { return lastListElement(this)->annotation->lastSourceLocation(); }

    UiAnnotationList *finish()
    {
        UiAnnotationList *head = next;
        next = nullptr;
        return head;
    }

// attributes
    UiAnnotationList *next;
    UiAnnotation *annotation;
};

} } // namespace AST


QT_END_NAMESPACE

#endif
