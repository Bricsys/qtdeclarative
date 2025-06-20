// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscodegenerator_p.h"
#include "qqmljsmetatypes_p.h"
#include "qqmljsregistercontent_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsutils_p.h"

#include <private/qqmljstypepropagator_p.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsutils_p.h>
#include <private/qv4compilerscanfunctions_p.h>
#include <private/qduplicatetracker_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
 * \internal
 * \class QQmlJSCodeGenerator
 *
 * This is a final compile pass that generates C++ code from a function and the
 * annotations produced by previous passes. Such annotations are produced by
 * QQmlJSTypePropagator, and possibly amended by other passes.
 */

#define BYTECODE_UNIMPLEMENTED() Q_ASSERT_X(false, Q_FUNC_INFO, "not implemented");

#define INJECT_TRACE_INFO(function) \
    static const bool injectTraceInfo = true; \
    if (injectTraceInfo) { \
        m_body += u"// "_s + QStringLiteral(#function) + u'\n'; \
    }

#define REJECT \
    return reject

static bool isTypeStorable(const QQmlJSTypeResolver *resolver, const QQmlJSScope::ConstPtr &type)
{
    return !type.isNull() && type != resolver->nullType() && type != resolver->voidType();
}

QString QQmlJSCodeGenerator::castTargetName(const QQmlJSScope::ConstPtr &type) const
{
    return type->augmentedInternalName();
}

QQmlJSCodeGenerator::QQmlJSCodeGenerator(
        const QV4::Compiler::Context *compilerContext,
        const QV4::Compiler::JSUnitGenerator *unitGenerator, const QQmlJSTypeResolver *typeResolver,
        QQmlJSLogger *logger, const BasicBlocks &basicBlocks,
        const InstructionAnnotations &annotations)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger, basicBlocks, annotations)
    , m_context(compilerContext)
{}

QString QQmlJSCodeGenerator::metaTypeFromType(const QQmlJSScope::ConstPtr &type) const
{
    return u"QMetaType::fromType<"_s + type->augmentedInternalName() + u">()"_s;
}

QString QQmlJSCodeGenerator::metaTypeFromName(const QQmlJSScope::ConstPtr &type) const
{
    return u"[]() { static const auto t = QMetaType::fromName(\""_s
            + QString::fromUtf8(QMetaObject::normalizedType(type->augmentedInternalName().toUtf8()))
            + u"\"); return t; }()"_s;
}

QString QQmlJSCodeGenerator::compositeListMetaType(const QString &elementName) const
{
    return u"QQmlPrivate::compositeListMetaType(aotContext->compilationUnit, "_s
            + (m_jsUnitGenerator->hasStringId(elementName)
                       ? QString::number(m_jsUnitGenerator->getStringId(elementName)) + u')'
                       : u"QStringLiteral(\"%1\"))"_s.arg(elementName));
}

QString QQmlJSCodeGenerator::compositeMetaType(const QString &elementName) const
{
    return u"QQmlPrivate::compositeMetaType(aotContext->compilationUnit, "_s
            + (m_jsUnitGenerator->hasStringId(elementName)
                       ? QString::number(m_jsUnitGenerator->getStringId(elementName)) + u')'
                       : u"QStringLiteral(\"%1\"))"_s.arg(elementName));
}

QString QQmlJSCodeGenerator::metaObject(const QQmlJSScope::ConstPtr &objectType)
{
    if (objectType->isComposite()) {
        const QString name = m_typeResolver->nameForType(objectType);
        if (name.isEmpty()) {
            REJECT<QString>(
                    u"retrieving the metaObject of a composite type without an element name."_s);
        }
        return compositeMetaType(name) + u".metaObject()"_s;
    }

    if (objectType->internalName() == u"QObject"_s
            || objectType->internalName() == u"QQmlComponent"_s) {
        return u'&' + objectType->internalName() + u"::staticMetaObject"_s;
    }
    return metaTypeFromName(objectType) + u".metaObject()"_s;
}

QString QQmlJSCodeGenerator::metaType(const QQmlJSScope::ConstPtr &type)
{
    if (type->isComposite()) {
        const QString name = m_typeResolver->nameForType(type);
        if (name.isEmpty()) {
            REJECT<QString>(
                    u"retrieving the metaType of a composite type without an element name."_s);
        }
        return compositeMetaType(name);
    }

    if (type->isListProperty() && type->valueType()->isComposite()) {
        const QString name = m_typeResolver->nameForType(type->valueType());
        Q_ASSERT(!name.isEmpty()); // There can't be a list with anonymous composite value type
        return compositeListMetaType(name);
    }

    return (m_typeResolver->genericType(type) == type)
            ? metaTypeFromType(type)
            : metaTypeFromName(type);
}

QQmlJSAotFunction QQmlJSCodeGenerator::run(const Function *function, bool basicBlocksValidationFailed)
{
    m_function = function;

    if (m_context->contextType == QV4::Compiler::ContextType::Binding
        && m_function->returnType.contains(m_typeResolver->qQmlScriptStringType())) {
        const QString reason = u"binding for property of type QQmlScriptString; nothing to do."_s;
        skip(reason);
        QQmlJSAotFunction result;
        result.skipReason = reason;
        return result;
    }

    QHash<int, int> numRegisterVariablesPerIndex;

    const auto addVariable
            = [&](int registerIndex, int lookupIndex, const QQmlJSScope::ConstPtr &seenType) {
        // Don't generate any variables for registers that are initialized with undefined.
        if (registerIndex == InvalidRegister || !isTypeStorable(m_typeResolver, seenType))
            return;

        const RegisterVariablesKey key = { seenType->internalName(), registerIndex, lookupIndex };


        const auto oldSize = m_registerVariables.size();
        auto &e = m_registerVariables[key];
        if (m_registerVariables.size() != oldSize) {
            e.variableName = u"r%1_%2"_s
                                     .arg(registerIndex)
                                     .arg(numRegisterVariablesPerIndex[registerIndex]++);
            e.storedType = seenType;
        }
        ++e.numTracked;
    };

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wrange-loop-analysis")
    for (const auto &annotation : m_annotations) {
        addVariable(annotation.second.changedRegisterIndex,
                    annotation.second.changedRegister.resultLookupIndex(),
                    annotation.second.changedRegister.storedType());
        for (auto it = annotation.second.typeConversions.begin(),
             end = annotation.second.typeConversions.end();
             it != end; ++it) {
            addVariable(
                    it.key(), it.value().content.resultLookupIndex(),
                    it.value().content.storedType());
        }
    }
QT_WARNING_POP

    // ensure we have m_labels for loops
    for (const auto loopLabel : m_context->labelInfo)
        m_labels.insert(loopLabel, u"label_%1"_s.arg(m_labels.size()));

    // Initialize the first instruction's state to hold the arguments.
    // After this, the arguments (or whatever becomes of them) are carried
    // over into any further basic blocks automatically.
    m_state.State::operator=(initialState(m_function));

    m_pool->setAllocationMode(QQmlJSRegisterContentPool::Temporary);
    const QByteArray byteCode = function->code;
    decode(byteCode.constData(), static_cast<uint>(byteCode.size()));
    m_pool->setAllocationMode(QQmlJSRegisterContentPool::Permanent);

    QQmlJSAotFunction result;
    result.includes.swap(m_includes);

    if (basicBlocksValidationFailed) {
        result.code += "// QV4_BASIC_BLOCK_VALIDATION_FAILED: This file failed compilation "_L1
                       "with basic blocks validation but compiled without it.\n"_L1;
    }

    result.code += u"// %1 at line %2, column %3\n"_s
            .arg(m_context->name).arg(m_context->line).arg(m_context->column);

    for (auto registerIt = m_registerVariables.cbegin(), registerEnd = m_registerVariables.cend();
         registerIt != registerEnd; ++registerIt) {

        const int registerIndex = registerIt.key().registerIndex;
        const bool registerIsArgument = isArgument(registerIndex);

        result.code += registerIt.key().internalName;

        const QQmlJSScope::ConstPtr storedType = registerIt->storedType;
        const bool isPointer
                = (storedType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference);
        if (isPointer)
            result.code += u" *"_s;
        else
            result.code += u' ';

        if (!registerIsArgument
                && registerIndex != Accumulator
                && registerIndex != This
                && !function->registerTypes[registerIndex - firstRegisterIndex()].contains(
                    m_typeResolver->voidType())) {
            result.code += registerIt->variableName + u" = "_s;
            result.code += convertStored(m_typeResolver->voidType(), storedType, QString());
        } else if (registerIsArgument && argumentType(registerIndex).isStoredIn(storedType)) {
            const int argumentIndex = registerIndex - FirstArgument;
            const QQmlJSRegisterContent argument
                    = m_function->argumentTypes[argumentIndex];
            const QQmlJSRegisterContent originalArgument = originalType(argument);

            const bool needsConversion = argument != originalArgument;
            if (!isPointer && registerIt->numTracked == 1 && !needsConversion) {
                // Not a pointer, never written to, and doesn't need any initial conversion.
                // This is a readonly argument.
                //
                // We would like to make the variable a const ref if it's a readonly argument,
                // but due to the various call interfaces accepting non-const values, we can't.
                // We rely on those calls to still not modify their arguments in place.
                result.code += u'&';
            }

            result.code += registerIt->variableName + u" = "_s;

            const auto originalContained = m_typeResolver->originalContainedType(argument);
            QString originalValue;
            const bool needsQVariantWrapping =
                    storedType->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence
                    && !originalContained->isReferenceType()
                    && storedType == m_typeResolver->varType()
                    && originalContained != m_typeResolver->varType();
            if (needsQVariantWrapping) {
                originalValue = u"QVariant(%1, argv[%2])"_s.arg(metaTypeFromName(originalContained))
                                        .arg(QString::number(argumentIndex + 1));
            } else {
                originalValue = u"(*static_cast<"_s + castTargetName(originalArgument.storedType())
                        + u"*>(argv["_s + QString::number(argumentIndex + 1) + u"]))"_s;
            }

            if (needsConversion)
                result.code += conversion(originalArgument, argument, originalValue);
            else
                result.code += originalValue;
        } else {
            result.code += registerIt->variableName;
        }
        result.code += u";\n"_s;
    }

    result.code += m_body;


    QString signature
            = u"    struct { QV4::ExecutableCompilationUnit *compilationUnit; } c { contextUnit };\n"
               "    const auto *aotContext = &c;\n"
               "    Q_UNUSED(aotContext);\n"_s;

    if (function->returnType.isValid()) {
        signature += u"    argTypes[0] = %1;\n"_s.arg(
                metaType(function->returnType.containedType()));
    } else {
        signature += u"    argTypes[0] = QMetaType();\n"_s;
    }
    result.numArguments = function->argumentTypes.length();
    for (qsizetype i = 0; i != result.numArguments; ++i) {
        signature += u"    argTypes[%1] = %2;\n"_s.arg(
                QString::number(i + 1),
                metaType(m_typeResolver->originalContainedType(function->argumentTypes[i])));
    }

    result.signature = std::move(signature);
    return result;
}

void QQmlJSCodeGenerator::generateReturnError()
{
    const auto finalizeReturn = qScopeGuard([this]() { m_body += u"return;\n"_s; });

    m_body += u"aotContext->setReturnValueUndefined();\n"_s;
    const auto ret = m_function->returnType;
    if (!ret.isValid() || ret.contains(m_typeResolver->voidType()))
        return;

    m_body += u"if (argv[0]) {\n"_s;

    const auto contained = ret.containedType();
    const auto stored = ret.storedType();
    if (contained->isReferenceType() && stored->isReferenceType()) {
        m_body += u"    *static_cast<"_s
                + stored->augmentedInternalName()
                + u" *>(argv[0]) = nullptr;\n"_s;
    } else if (contained == stored) {
        m_body += u"    *static_cast<"_s + stored->internalName() + u" *>(argv[0]) = "_s
                + stored->internalName() + u"();\n"_s;
    } else {
        m_body += u"    const QMetaType returnType = "_s
                + metaType(ret.containedType()) + u";\n"_s;
        m_body += u"    returnType.destruct(argv[0]);\n"_s;
        m_body += u"    returnType.construct(argv[0]);\n "_s;
    }

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_Ret()
{
    INJECT_TRACE_INFO(generate_Ret);

    const auto finalizeReturn = qScopeGuard([this]() {
        m_body += u"return;\n"_s;
        m_skipUntilNextLabel = true;
        resetState();
    });

    if (!m_function->returnType.isValid())
        return;

    m_body += u"if (argv[0]) {\n"_s;

    const QString signalUndefined = u"aotContext->setReturnValueUndefined();\n"_s;
    const QString in = m_state.accumulatorVariableIn;

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();

    if (in.isEmpty()) {
        if (accumulatorIn.isStoredIn(m_typeResolver->voidType()))
            m_body += signalUndefined;
    } else if (accumulatorIn.isStoredIn(m_typeResolver->varType())) {
        m_body += u"    if (!"_s + in + u".isValid())\n"_s;
        m_body += u"        "_s + signalUndefined;
    } else if (accumulatorIn.isStoredIn(m_typeResolver->jsPrimitiveType())) {
        m_body += u"    if ("_s + in + u".type() == QJSPrimitiveValue::Undefined)\n"_s;
        m_body += u"        "_s + signalUndefined;
    } else if (accumulatorIn.isStoredIn(m_typeResolver->jsValueType())) {
        m_body += u"    if ("_s + in + u".isUndefined())\n"_s;
        m_body += u"        "_s + signalUndefined;
    }

    if (m_function->returnType.contains(m_typeResolver->voidType())) {
        m_body += u"}\n"_s;
        return;
    }

    const auto contained = m_function->returnType.containedType();
    const auto stored = m_function->returnType.storedType();
    if (contained == stored || (contained->isReferenceType() && stored->isReferenceType())) {
        // We can always std::move here, no matter what the optimization pass has detected. The
        // function returns and nothing can access the accumulator register anymore afterwards.
        m_body += u"    *static_cast<"_s
                + stored->augmentedInternalName()
                + u" *>(argv[0]) = "_s
                + conversion(
                          accumulatorIn, m_function->returnType,
                          m_typeResolver->isTriviallyCopyable(accumulatorIn.storedType())
                                  ? in
                                  : u"std::move("_s + in + u')')
                + u";\n"_s;
    } else if (accumulatorIn.contains(contained)) {
        m_body += u"    const QMetaType returnType = "_s + contentType(accumulatorIn, in)
                + u";\n"_s;
        m_body += u"    returnType.destruct(argv[0]);\n"_s;
        m_body += u"    returnType.construct(argv[0], "_s
                + contentPointer(accumulatorIn, in) + u");\n"_s;
    } else {
        m_body += u"    const auto converted = "_s
                + conversion(accumulatorIn, m_function->returnType,
                             consumedAccumulatorVariableIn()) + u";\n"_s;
        m_body += u"    const QMetaType returnType = "_s
                + contentType(m_function->returnType, u"converted"_s)
                + u";\n"_s;
        m_body += u"    returnType.destruct(argv[0]);\n"_s;
        m_body += u"    returnType.construct(argv[0], "_s
                + contentPointer(m_function->returnType, u"converted"_s) + u");\n"_s;
    }

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_Debug()
{
    BYTECODE_UNIMPLEMENTED();
}

static QString toNumericString(double value)
{
    if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
        const int i = value;
        if (i == value)
            return QString::number(i);
    }

    switch (qFpClassify(value)) {
    case FP_INFINITE: {
        const QString inf = u"std::numeric_limits<double>::infinity()"_s;
        return std::signbit(value) ? (u'-' + inf) : inf;
    }
    case FP_NAN:
        return u"std::numeric_limits<double>::quiet_NaN()"_s;
    case FP_ZERO:
        return std::signbit(value) ? u"-0.0"_s : u"0"_s;
    default:
        break;
    }

    return QString::number(value, 'f', std::numeric_limits<double>::max_digits10);
}

void QQmlJSCodeGenerator::generate_LoadConst(int index)
{
    INJECT_TRACE_INFO(generate_LoadConst);

    // You cannot actually get it to generate LoadConst for anything but double. We have
    // a numer of specialized instructions for the other types, after all. However, let's
    // play it safe.

    const QV4::ReturnedValue encodedConst = m_jsUnitGenerator->constant(index);
    const QV4::StaticValue value = QV4::StaticValue::fromReturnedValue(encodedConst);
    const QQmlJSScope::ConstPtr type = m_typeResolver->typeForConst(encodedConst);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    if (type == m_typeResolver->realType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    toNumericString(value.doubleValue()));
    } else if (type == m_typeResolver->int32Type()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    QString::number(value.integerValue()));
    } else if (type == m_typeResolver->boolType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    value.booleanValue() ? u"true"_s : u"false"_s);
    } else if (type == m_typeResolver->voidType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    QString());
    } else if (type == m_typeResolver->nullType()) {
        m_body += conversion(
                    type, m_state.accumulatorOut(),
                    u"nullptr"_s);
    } else {
        REJECT(u"unsupported constant type"_s);
    }

    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadZero()
{
    INJECT_TRACE_INFO(generate_LoadZero);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->int32Type(), m_state.accumulatorOut(), u"0"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadTrue()
{
    INJECT_TRACE_INFO(generate_LoadTrue);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(), u"true"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadFalse()
{
    INJECT_TRACE_INFO(generate_LoadFalse);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s + conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(), u"false"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadNull()
{
    INJECT_TRACE_INFO(generate_LoadNull);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->nullType(), m_state.accumulatorOut(),
                         u"nullptr"_s);
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadUndefined()
{
    INJECT_TRACE_INFO(generate_LoadUndefined);

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->voidType(), m_state.accumulatorOut(),
                         QString());
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadInt(int value)
{
    INJECT_TRACE_INFO(generate_LoadInt);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->int32Type(), m_state.accumulatorOut(),
                         QString::number(value));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveConst(int constIndex, int destTemp)
{
    INJECT_TRACE_INFO(generate_MoveConst);

    Q_ASSERT(destTemp == m_state.changedRegisterIndex());

    auto var = changedRegisterVariable();
    if (var.isEmpty())
        return; // Do not load 'undefined'

    const auto v4Value = QV4::StaticValue::fromReturnedValue(
                m_jsUnitGenerator->constant(constIndex));

    const auto changed = m_state.changedRegister();
    QQmlJSScope::ConstPtr contained;
    QString input;

    m_body += var + u" = "_s;
    if (v4Value.isNull()) {
        contained = m_typeResolver->nullType();
    } else if (v4Value.isUndefined()) {
        contained = m_typeResolver->voidType();
    } else if (v4Value.isBoolean()) {
        contained = m_typeResolver->boolType();
        input = v4Value.booleanValue() ? u"true"_s : u"false"_s;
    } else if (v4Value.isInteger()) {
        contained = m_typeResolver->int32Type();
        input = QString::number(v4Value.int_32());
    } else if (v4Value.isDouble()) {
        contained = m_typeResolver->realType();
        input = toNumericString(v4Value.doubleValue());
    } else {
        REJECT(u"unknown const type"_s);
    }
    m_body += conversion(contained, changed, input) + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadReg(int reg)
{
    INJECT_TRACE_INFO(generate_LoadReg);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(
                registerType(reg), m_state.accumulatorOut(), consumedRegisterVariable(reg));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_StoreReg(int reg)
{
    INJECT_TRACE_INFO(generate_StoreReg);

    Q_ASSERT(m_state.changedRegisterIndex() == reg);
    Q_ASSERT(m_state.accumulatorIn().isValid());
    const QString var = changedRegisterVariable();
    if (var.isEmpty())
        return; // don't store "undefined"
    m_body += var;
    m_body += u" = "_s;
    m_body += conversion(m_state.accumulatorIn(), m_state.changedRegister(),
                         consumedAccumulatorVariableIn());
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveReg(int srcReg, int destReg)
{
    INJECT_TRACE_INFO(generate_MoveReg);

    Q_ASSERT(m_state.changedRegisterIndex() == destReg);
    const QString destRegName = changedRegisterVariable();
    if (destRegName.isEmpty())
        return; // don't store things we cannot store.
    m_body += destRegName;
    m_body += u" = "_s;
    m_body += conversion(
                registerType(srcReg), m_state.changedRegister(), consumedRegisterVariable(srcReg));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadImport(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadLocal(int index)
{
    Q_UNUSED(index);
    REJECT(u"LoadLocal"_s);
}

void QQmlJSCodeGenerator::generate_StoreLocal(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadRuntimeString(int stringId)
{
    INJECT_TRACE_INFO(generate_LoadRuntimeString);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->stringType(), m_state.accumulatorOut(),
                         QQmlJSUtils::toLiteral(m_jsUnitGenerator->stringForIndex(stringId)));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_MoveRegExp(int regExpId, int destReg)
{
    Q_UNUSED(regExpId)
    Q_UNUSED(destReg)
    REJECT(u"MoveRegExp"_s);
}

void QQmlJSCodeGenerator::generate_LoadClosure(int value)
{
    Q_UNUSED(value)
    REJECT(u"LoadClosure"_s);
}

void QQmlJSCodeGenerator::generate_LoadName(int nameIndex)
{
    Q_UNUSED(nameIndex)
    REJECT(u"LoadName"_s);
}

void QQmlJSCodeGenerator::generate_LoadGlobalLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadGlobalLookup);

    AccumulatorConverter registers(this);

    const QString lookup = u"aotContext->loadGlobalLookup("_s + QString::number(index) + u", "_s
            + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
    const QString initialization = u"aotContext->initLoadGlobalLookup("_s
            + QString::number(index) + u", "_s
            + contentType(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
    const QString preparation = getLookupPreparation(
            m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);
    generateLookup(lookup, initialization, preparation);
}

void QQmlJSCodeGenerator::generate_LoadQmlContextPropertyLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadQmlContextPropertyLookup);

    AccumulatorConverter registers(this);

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    if (m_state.accumulatorOut().scope().contains(m_typeResolver->jsGlobalObject())) {
        // This produces a QJSValue. The QQmlJSMetaProperty used to analyze it may have more details
        // but the QQmlJSAotContext API does not reflect them.
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(
                    m_typeResolver->jsValueType(), m_state.accumulatorOut(),
                    u"aotContext->javaScriptGlobalProperty("_s + QString::number(nameIndex) + u")")
                + u";\n"_s;
        return;
    }

    const QString indexString = QString::number(index);
    if (m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ObjectById) {
        const QString lookup = u"aotContext->loadContextIdLookup("_s
                + indexString + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initLoadContextIdLookup("_s
                + indexString + u')';
        generateLookup(lookup, initialization);
        return;
    }

    const bool isProperty = m_state.accumulatorOut().isProperty();
    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    if (isProperty) {
        const QString lookup = u"aotContext->loadScopeObjectPropertyLookup("_s
                + indexString + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initLoadScopeObjectPropertyLookup("_s
                + indexString + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);

        generateLookup(lookup, initialization, preparation);
    } else if (m_state.accumulatorOut().isType() || m_state.accumulatorOut().isImportNamespace()) {
        generateTypeLookup(index);
    } else {
        REJECT(u"lookup of %1"_s.arg(m_state.accumulatorOut().descriptiveName()));
    }
}

void QQmlJSCodeGenerator::generate_StoreNameSloppy(int nameIndex)
{
    INJECT_TRACE_INFO(generate_StoreNameSloppy);

    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent type = m_typeResolver->scopedType(m_function->qmlScope, name);
    Q_ASSERT(type.isProperty());

    switch (type.variant()) {
    case QQmlJSRegisterContent::Property: {
        // Do not convert here. We may intentionally pass the "wrong" type, for example to trigger
        // a property reset.
        m_body += u"aotContext->storeNameSloppy("_s + QString::number(nameIndex)
                + u", "_s
                + contentPointer(m_state.accumulatorIn(), m_state.accumulatorVariableIn)
                + u", "_s
                + contentType(m_state.accumulatorIn(), m_state.accumulatorVariableIn) + u')';
        m_body += u";\n"_s;
        break;
    }
    case QQmlJSRegisterContent::Method:
        REJECT(u"assignment to scope method"_s);
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::generate_StoreNameStrict(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadElement(int base)
{
    INJECT_TRACE_INFO(generate_LoadElement);

    const QQmlJSRegisterContent baseType = registerType(base);

    if (!baseType.isList() && !baseType.isStoredIn(m_typeResolver->stringType()))
        REJECT(u"LoadElement with non-list base type "_s + baseType.descriptiveName());

    const QString voidAssignment = u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
            conversion(literalType(m_typeResolver->voidType()),
                       m_state.accumulatorOut(), QString()) + u";\n"_s;

    AccumulatorConverter registers(this);

    QString indexName = m_state.accumulatorVariableIn;
    QQmlJSScope::ConstPtr indexType;
    if (m_typeResolver->isNumeric(m_state.accumulatorIn())) {
        indexType = m_state.accumulatorIn().containedType();
    } else if (m_state.accumulatorIn().isConversion()) {
        const auto target = m_typeResolver->extractNonVoidFromOptionalType(m_state.accumulatorIn());
        if (m_typeResolver->isNumeric(target)) {
            indexType = target.containedType();
            m_body += u"if (!" + indexName + u".metaType().isValid())\n"
                    + voidAssignment
                    + u"else ";
            indexName = convertStored(
                    m_state.accumulatorIn().storedType(), indexType, indexName);
        } else {
            REJECT(u"LoadElement with non-numeric argument"_s);
        }
    }

    const QString baseName = registerVariable(base);

    if (!m_typeResolver->isNativeArrayIndex(indexType)) {
        m_body += u"if (!QJSNumberCoercion::isArrayIndex("_s + indexName + u"))\n"_s
                + voidAssignment
                + u"else "_s;
    } else if (!m_typeResolver->isUnsignedInteger(indexType)) {
        m_body += u"if ("_s + indexName + u" < 0)\n"_s
                + voidAssignment
                + u"else "_s;
    }

    if (baseType.isStoredIn(m_typeResolver->listPropertyType())) {
        // Our QQmlListProperty only keeps plain QObject*.
        m_body += u"if ("_s + indexName + u" < "_s + baseName
                + u".count(&"_s + baseName + u"))\n"_s;
        m_body += u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
                conversion(m_typeResolver->qObjectType(), m_state.accumulatorOut(),
                           baseName + u".at(&"_s + baseName + u", "_s
                           + indexName + u')') + u";\n"_s;
        m_body += u"else\n"_s
                + voidAssignment;
        return;
    }

    // Since we can do .at() below, we know that we can natively store the element type.
    QQmlJSRegisterContent elementType = m_typeResolver->valueType(baseType);
    elementType = m_pool->storedIn(
            elementType, m_typeResolver->storedType(elementType.containedType()));

    QString access = baseName + u".at("_s + indexName + u')';

    // TODO: Once we get a char type in QML, use it here.
    if (baseType.isStoredIn(m_typeResolver->stringType()))
        access = u"QString("_s + access + u")"_s;
    else if (m_state.isRegisterAffectedBySideEffects(base))
        REJECT(u"LoadElement on a sequence potentially affected by side effects"_s);
    else if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
        REJECT(u"LoadElement on a sequence wrapped in a non-sequence type"_s);

    m_body += u"if ("_s + indexName + u" < "_s + baseName + u".size())\n"_s;
    m_body += u"    "_s + m_state.accumulatorVariableOut + u" = "_s +
            conversion(elementType, m_state.accumulatorOut(), access) + u";\n"_s;
    m_body += u"else\n"_s
            + voidAssignment;
}

void QQmlJSCodeGenerator::generate_StoreElement(int base, int index)
{
    INJECT_TRACE_INFO(generate_StoreElement);

    const QQmlJSRegisterContent baseType = registerType(base);
    const QQmlJSScope::ConstPtr indexType = registerType(index).containedType();

    if (!m_typeResolver->isNumeric(indexType) || !baseType.isList())
        REJECT(u"StoreElement with non-list base type or non-numeric arguments"_s);

    if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
        REJECT(u"indirect StoreElement"_s);

    const QString baseName = registerVariable(base);
    const QString indexName = registerVariable(index);

    const auto valueType = m_typeResolver->valueType(baseType);
    const auto elementType = m_typeResolver->genericType(valueType.containedType());

    addInclude(u"QtQml/qjslist.h"_s);
    if (!m_typeResolver->isNativeArrayIndex(indexType))
        m_body += u"if (QJSNumberCoercion::isArrayIndex("_s + indexName + u")) {\n"_s;
    else if (!m_typeResolver->isUnsignedInteger(indexType))
        m_body += u"if ("_s + indexName + u" >= 0) {\n"_s;
    else
        m_body += u"{\n"_s;

    if (baseType.isStoredIn(m_typeResolver->listPropertyType())) {
        m_body += u"    if ("_s + indexName + u" < "_s + baseName + u".count(&"_s + baseName
                + u"))\n"_s;
        m_body += u"        "_s + baseName + u".replace(&"_s + baseName
                + u", "_s + indexName + u", "_s;
        m_body += conversion(m_state.accumulatorIn(), elementType, m_state.accumulatorVariableIn)
                + u");\n"_s;
        m_body += u"}\n"_s;
        return;
    }

    if (m_state.isRegisterAffectedBySideEffects(base))
        REJECT(u"LoadElement on a sequence potentially affected by side effects"_s);

    m_body += u"    if ("_s + indexName + u" >= " + baseName + u".size())\n"_s;
    m_body += u"        QJSList(&"_s + baseName + u", aotContext->engine).resize("_s
            + indexName + u" + 1);\n"_s;
    m_body += u"    "_s + baseName + u'[' + indexName + u"] = "_s;
    m_body += conversion(m_state.accumulatorIn(), elementType, m_state.accumulatorVariableIn)
            + u";\n"_s;
    m_body += u"}\n"_s;

    generateWriteBack(base);
}

void QQmlJSCodeGenerator::generate_LoadProperty(int nameIndex)
{
    Q_UNUSED(nameIndex)
    REJECT(u"LoadProperty"_s);
}

void QQmlJSCodeGenerator::generate_LoadOptionalProperty(int name, int offset)
{
    Q_UNUSED(name)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generateEnumLookup(int index)
{
    const QString enumMember = m_state.accumulatorOut().enumMember();

    // If we're referring to the type, there's nothing to do.
    // However, we should not get here since no one can ever use the enum metatype.
    // The lookup is dead code and should be optimized away.
    // ... unless you are actually trying to store the metatype itself in a property.
    //     We cannot compile such code.
    if (enumMember.isEmpty())
        REJECT(u"Lookup of enum metatype"_s);

    // If the metaenum has the value, just use it and skip all the rest.
    const QQmlJSMetaEnum metaEnum = m_state.accumulatorOut().enumeration();
    if (metaEnum.hasValues()) {
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + QString::number(metaEnum.value(enumMember));
        m_body += u";\n"_s;
        return;
    }

    const QQmlJSScope::ConstPtr scopeType = m_state.accumulatorOut().scopeType();

    // Otherwise we would have found an enum with values.
    Q_ASSERT(!scopeType->isComposite());

    const QString enumName = metaEnum.isFlag() ? metaEnum.alias() : metaEnum.name();
    if (enumName.isEmpty()) {
        if (metaEnum.isFlag() && !metaEnum.name().isEmpty()) {
            REJECT(u"qmltypes misses name entry for flag; "
                   "did you pass the enum type to Q_FLAG instead of the QFlag type?"
                   "\nType is %1, enum name is %2"_s.arg(scopeType->internalName(), metaEnum.name()));
        }
        REJECT(u"qmltypes misses name entry for enum"_s);
    }
    const QString lookup = u"aotContext->getEnumLookup("_s + QString::number(index)
            + u", &"_s + m_state.accumulatorVariableOut + u')';
    const QString initialization = u"aotContext->initGetEnumLookup("_s
            + QString::number(index) + u", "_s + metaObject(scopeType)
            + u", \""_s + enumName + u"\", \""_s + enumMember
            + u"\")"_s;
    generateLookup(lookup, initialization);
}

void QQmlJSCodeGenerator::generateTypeLookup(int index)
{
    const QString indexString = QString::number(index);
    const QQmlJSRegisterContent accumulatorIn = m_state.registers.value(Accumulator).content;
    const QString namespaceString
            = accumulatorIn.isImportNamespace()
                ? QString::number(accumulatorIn.importNamespace())
                : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_s;

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::Singleton: {
        rejectIfNonQObjectOut(u"non-QObject singleton type"_s);
        const QString lookup = u"aotContext->loadSingletonLookup("_s + indexString
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadSingletonLookup("_s + indexString
                + u", "_s + namespaceString + u')';
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::ModulePrefix:
        break;
    case QQmlJSRegisterContent::Attachment: {
        rejectIfNonQObjectOut(u"non-QObject attached type"_s);
        const QString lookup = u"aotContext->loadAttachedLookup("_s + indexString
                + u", aotContext->qmlScopeObject, &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_s + indexString
                + u", "_s + namespaceString + u", aotContext->qmlScopeObject)"_s;
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::Script:
        REJECT(u"script lookup"_s);
    case QQmlJSRegisterContent::MetaType: {
        if (!m_state.accumulatorOut().isStoredIn(m_typeResolver->metaObjectType())) {
            // TODO: Can we trigger this somehow?
            //       It might be impossible, but we better be safe here.
            REJECT(u"meta-object stored in different type"_s);
        }
        const QString lookup = u"aotContext->loadTypeLookup("_s + indexString
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadTypeLookup("_s + indexString
                + u", "_s + namespaceString + u")"_s;
        generateLookup(lookup, initialization);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::generateVariantEqualityComparison(
        QQmlJSRegisterContent nonStorableContent, const QString &registerName, bool invert)
{
    const auto nonStorableType = nonStorableContent.containedType();
    QQmlJSScope::ConstPtr comparedType = (nonStorableType == m_typeResolver->nullType())
            ? m_typeResolver->nullType()
            : m_typeResolver->voidType();

    // The common operations for both nulltype and voidtype
    m_body += u"if ("_s + registerName
            + u".metaType() == QMetaType::fromType<QJSPrimitiveValue>()) {\n"_s
            + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         u"static_cast<const QJSPrimitiveValue *>("_s + registerName
                                 + u".constData())"_s + u"->type() "_s
                                 + (invert ? u"!="_s : u"=="_s)
                                 + (comparedType == m_typeResolver->nullType()
                                            ? u"QJSPrimitiveValue::Null"_s
                                            : u"QJSPrimitiveValue::Undefined"_s))
            + u";\n} else if ("_s + registerName
            + u".metaType() == QMetaType::fromType<QJSValue>()) {\n"_s
            + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         (invert ? u"!"_s : QString()) + u"static_cast<const QJSValue *>("_s
                                 + registerName + u".constData())"_s + u"->"_s
                                 + (comparedType == m_typeResolver->nullType()
                                            ? u"isNull()"_s
                                            : u"isUndefined()"_s))
            + u";\n}"_s;

    // Generate nullType specific operations (the case when variant contains QObject * or
    // std::nullptr_t)
    if (nonStorableType == m_typeResolver->nullType()) {
        m_body += u"else if ("_s + registerName
                + u".metaType().flags().testFlag(QMetaType::PointerToQObject)) {\n"_s
                + m_state.accumulatorVariableOut + u" = "_s
                + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                             u"*static_cast<QObject *const *>("_s + registerName
                                     + u".constData())"_s + (invert ? u"!="_s : u"=="_s)
                                     + u" nullptr"_s)
                + u";\n} else if ("_s + registerName
                + u".metaType() == QMetaType::fromType<std::nullptr_t>()) {\n"_s
                + m_state.accumulatorVariableOut + u" = "_s
                + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                             (invert ? u"false"_s : u"true"_s))
                + u";\n}\n"_s;
    }

    // fallback case (if variant contains a different type, then it is not null or undefined)
    m_body += u"else {\n"_s + m_state.accumulatorVariableOut + u" = "_s
            + conversion(m_typeResolver->boolType(), m_state.accumulatorOut(),
                         (invert ? (registerName + u".isValid() ? true : false"_s)
                                 : (registerName + u".isValid() ? false : true"_s)))
            + u";\n}"_s;
}

void QQmlJSCodeGenerator::generateVariantEqualityComparison(
        QQmlJSRegisterContent storableContent, const QString &typedRegisterName,
        const QString &varRegisterName, bool invert)
{
    // enumerations are ===-equal to their underlying type and they are stored as such.
    // Therefore, use the underlying type right away.
    const QQmlJSScope::ConstPtr contained = storableContent.isEnumeration()
              ? storableContent.storedType()
              : storableContent.containedType();

    const QQmlJSScope::ConstPtr boolType = m_typeResolver->boolType();
    if (contained->isReferenceType()) {
        const QQmlJSScope::ConstPtr comparable = m_typeResolver->qObjectType();
        const QString cmpExpr = (invert ? u"!"_s : QString()) + u"(("
                + varRegisterName + u".metaType().flags() & QMetaType::PointerToQObject) "_s
                + u" && "_s + conversion(storableContent, comparable, typedRegisterName) + u" == "_s
                + conversion(m_typeResolver->varType(), comparable, varRegisterName) + u')';

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(boolType, m_state.accumulatorOut(), cmpExpr) + u";\n";
        return;
    }

    if (m_typeResolver->isPrimitive(contained)) {
        const QQmlJSScope::ConstPtr comparable = m_typeResolver->jsPrimitiveType();
        const QString cmpExpr = (invert ? u"!"_s : QString())
                + conversion(storableContent, comparable, typedRegisterName)
                + u".strictlyEquals("_s
                + conversion(m_typeResolver->varType(), comparable, varRegisterName) + u')';

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(boolType, m_state.accumulatorOut(), cmpExpr) + u";\n"_s;
        return;
    }

    REJECT(u"comparison of non-primitive, non-object type to var"_s);
}

void QQmlJSCodeGenerator::generateArrayInitializer(int argc, int argv)
{
    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    const QQmlJSScope::ConstPtr value = stored->valueType();
    Q_ASSERT(value);

    QStringList initializer;
    for (int i = 0; i < argc; ++i) {
        initializer += convertStored(
                registerType(argv + i).storedType(), value,
                consumedRegisterVariable(argv + i));
    }

    m_body += m_state.accumulatorVariableOut + u" = "_s + stored->internalName() + u'{';
    m_body += initializer.join(u", "_s);
    m_body += u"};\n";
}

void QQmlJSCodeGenerator::generateWriteBack(int registerIndex)
{
    QString writeBackRegister = registerVariable(registerIndex);
    bool writeBackAffectedBySideEffects = m_state.isRegisterAffectedBySideEffects(registerIndex);

    for (QQmlJSRegisterContent writeBack = registerType(registerIndex);
         !writeBack.storedType()->isReferenceType();) {
        if (writeBackAffectedBySideEffects)
            REJECT(u"write-back of value affected by side effects"_s);

        if (writeBack.isConversion())
            REJECT(u"write-back of converted value"_s);

        const int lookupIndex = writeBack.resultLookupIndex();

        // This is essential for the soundness of the type system.
        //
        // If a value or a list is returned from a function, we cannot know
        // whether it is a copy or a reference. Therefore, we cannot know whether
        // we have to write it back and so we have to REJECT any write on it.
        //
        // Only if we are sure that the value is locally created we can be sure
        // we don't have to write it back. In this latter case we could allow
        // a modification that doesn't write back.
        if (lookupIndex == -1)
            REJECT(u"write-back of non-lookup"_s);

        const QString writeBackIndexString = QString::number(lookupIndex);

        const QQmlJSRegisterContent::ContentVariant variant = writeBack.variant();
        if (variant == QQmlJSRegisterContent::Property && isQmlScopeObject(writeBack.scope())) {
            const QString lookup = u"aotContext->writeBackScopeObjectPropertyLookup("_s
                    + writeBackIndexString
                    + u", "_s + contentPointer(writeBack, writeBackRegister) + u')';
            const QString initialization = u"aotContext->initLoadScopeObjectPropertyLookup("_s
                    + writeBackIndexString + u')';
            generateLookup(lookup, initialization);
            break;
        }


        QQmlJSRegisterContent outerContent;
        QString outerRegister;
        bool outerAffectedBySideEffects = false;
        for (auto it = m_state.lookups.constBegin(), end = m_state.lookups.constEnd();
             it != end; ++it) {
            if (it.value().content.resultLookupIndex() == writeBack.baseLookupIndex()) {
                outerContent = it.value().content;
                outerRegister = lookupVariable(outerContent.resultLookupIndex());
                outerAffectedBySideEffects = it.value().affectedBySideEffects;
                break;
            }
        }

        // If the lookup doesn't exist, it was killed by state merge.
        if (!outerContent.isValid())
            REJECT(u"write-back of lookup across jumps or merges."_s);

        Q_ASSERT(!outerRegister.isEmpty());

        switch (writeBack.variant()) {
        case QQmlJSRegisterContent::Property:
            if (writeBack.scopeType()->isReferenceType()) {
                const QString lookup = u"aotContext->writeBackObjectLookup("_s
                        + writeBackIndexString
                        + u", "_s + outerRegister
                        + u", "_s + contentPointer(writeBack, writeBackRegister) + u')';

                const QString initialization = (m_state.registers[registerIndex].isShadowable
                                        ? u"aotContext->initGetObjectLookupAsVariant("_s
                                        : u"aotContext->initGetObjectLookup("_s)
                        + writeBackIndexString + u", "_s + outerRegister + u')';

                generateLookup(lookup, initialization);
            } else {
                const QString valuePointer = contentPointer(outerContent, outerRegister);
                const QString lookup = u"aotContext->writeBackValueLookup("_s
                        + writeBackIndexString
                        + u", "_s + valuePointer
                        + u", "_s + contentPointer(writeBack, writeBackRegister) + u')';
                const QString initialization = u"aotContext->initGetValueLookup("_s
                        + writeBackIndexString
                        + u", "_s + metaObject(writeBack.scopeType()) + u')';
                generateLookup(lookup, initialization);
            }
            break;
        default:
            REJECT(u"SetLookup on value types (because of missing write-back)"_s);
        }

        writeBackRegister = outerRegister;
        writeBack = outerContent;
        writeBackAffectedBySideEffects = outerAffectedBySideEffects;
    }
}

void QQmlJSCodeGenerator::rejectIfNonQObjectOut(const QString &error)
{
    if (m_state.accumulatorOut().storedType()->accessSemantics()
        != QQmlJSScope::AccessSemantics::Reference) {
        REJECT(error);
    }
}

void QQmlJSCodeGenerator::rejectIfBadArray()
{
    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    if (stored->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence) {
        // This rejects any attempt to store the list into a QVariant.
        // Therefore, we don't have to adjust the contained type below.

        REJECT(u"storing an array in a non-sequence type"_s);
    } else if (stored->isListProperty()) {
        // We can, technically, generate code for this. But it's dangerous:
        //
        // const QString storage = m_state.accumulatorVariableOut + u"_storage"_s;
        // m_body += stored->internalName() + u"::ListType " + storage
        //         + u" = {"_s + initializer.join(u", "_s) + u"};\n"_s;
        // m_body += m_state.accumulatorVariableOut
        //         + u" = " + stored->internalName() + u"(nullptr, &"_s + storage + u");\n"_s;

        REJECT(u"creating a QQmlListProperty not backed by a property"_s);

    }
}

/*!
 * \internal
 *
 * generates a check for the content pointer to be valid.
 * Returns true if the content pointer needs to be retrieved from a QVariant, or
 * false if the variable can be used as-is.
 */
bool QQmlJSCodeGenerator::generateContentPointerCheck(
        const QQmlJSScope::ConstPtr &required, QQmlJSRegisterContent actual,
        const QString &variable, const QString &errorMessage)
{
    const QQmlJSScope::ConstPtr scope = required;
    const QQmlJSScope::ConstPtr input = actual.containedType();
    if (QQmlJSUtils::searchBaseAndExtensionTypes(input,
            [&](const QQmlJSScope::ConstPtr &base) { return base == scope; })) {
        return false;
    }

    if (!m_typeResolver->canHold(input, scope)) {
        REJECT<bool>(u"lookup of members of %1 in %2"_s
                             .arg(scope->internalName(), input->internalName()));
    }

    bool needsVarContentConversion = false;
    QString processedErrorMessage;
    if (actual.storedType()->isReferenceType()) {
        // Since we have verified the type in qqmljstypepropagator.cpp we now know
        // that we can only have either null or the actual type here. Therefore,
        // it's enough to check the pointer for null.
        m_body += u"if ("_s + variable + u" == nullptr) {\n    "_s;
        processedErrorMessage = errorMessage.arg(u"null");
    } else if (actual.isStoredIn(m_typeResolver->varType())) {
        // Since we have verified the type in qqmljstypepropagator.cpp we now know
        // that we can only have either undefined or the actual type here. Therefore,
        // it's enough to check the QVariant for isValid().
        m_body += u"if (!"_s + variable + u".isValid()) {\n    "_s;
        needsVarContentConversion = true;
        processedErrorMessage = errorMessage.arg(u"undefined");
    } else {
        REJECT<bool>(u"retrieving metatype from %1"_s.arg(actual.descriptiveName()));
    }

    generateSetInstructionPointer();
    m_body += u"    aotContext->engine->throwError(QJSValue::TypeError, "_s;
    m_body += u"QLatin1String(\"%1\"));\n"_s.arg(processedErrorMessage);
    generateReturnError();
    m_body += u"}\n"_s;
    return needsVarContentConversion;
}

QString QQmlJSCodeGenerator::generateCallConstructor(
        const QQmlJSMetaMethod &ctor, const QList<QQmlJSRegisterContent> &argumentTypes,
        const QStringList &arguments, const QString &metaType, const QString &metaObject)
{
    const auto parameterTypes = ctor.parameters();
    Q_ASSERT(parameterTypes.length() == argumentTypes.length());

    // We need to store the converted arguments in a temporaries because they might not be lvalues.
    QStringList argPointers;

    QString result = u"[&](){\n"_s;
    for (qsizetype i = 0, end = parameterTypes.length(); i < end; ++i) {
        const QQmlJSRegisterContent argumentType = argumentTypes[i];
        const QQmlJSScope::ConstPtr parameterType = parameterTypes[i].type();
        const QString argument = arguments[i];
        const QString arg = u"arg"_s + QString::number(i);

        result += u"    auto "_s + arg + u" = "_s;
        if (argumentType.contains(parameterType)) {
            result += argument;
            argPointers.append(contentPointer(argumentType, arg));
        } else {
            const QQmlJSRegisterContent parameterTypeConversion
                    = m_pool->storedIn(
                        m_typeResolver->convert(argumentType, parameterType),
                        m_typeResolver->genericType(parameterType));
            result += conversion(argumentType, parameterTypeConversion, argument);
            argPointers.append(contentPointer(parameterTypeConversion, arg));
        }
        result += u";\n"_s;
    }

    result += u"    void *args[] = {"_s + argPointers.join(u',') + u"};\n"_s;
    result += u"    return aotContext->constructValueType("_s + metaType + u", "_s + metaObject
            + u", "_s + QString::number(int(ctor.constructorIndex())) + u", args);\n"_s;

    return result + u"}()"_s;
}

QString QQmlJSCodeGenerator::resolveValueTypeContentPointer(
        const QQmlJSScope::ConstPtr &required, QQmlJSRegisterContent actual,
        const QString &variable, const QString &errorMessage)
{
    if (generateContentPointerCheck(required, actual, variable, errorMessage))
        return variable + u".data()"_s;
    return contentPointer(actual, variable);
}

QString QQmlJSCodeGenerator::resolveQObjectPointer(
        const QQmlJSScope::ConstPtr &required, QQmlJSRegisterContent actual,
        const QString &variable, const QString &errorMessage)
{
    if (generateContentPointerCheck(required, actual, variable, errorMessage))
        return u"*static_cast<QObject *const *>("_s + variable + u".constData())"_s;
    return variable;
}

void QQmlJSCodeGenerator::generate_GetLookup(int index)
{
    INJECT_TRACE_INFO(generate_GetLookup);
    generate_GetLookupHelper(index);
}

void QQmlJSCodeGenerator::generate_GetLookupHelper(int index)
{
    if (m_state.accumulatorOut().isMethod())
        REJECT(u"lookup of function property."_s);

    if (m_state.accumulatorOut().scope().contains(m_typeResolver->mathObject())) {
        QString name = m_jsUnitGenerator->lookupName(index);

        double value{};
        if (name == u"E") {
            value = std::exp(1.0);
        } else if (name == u"LN10") {
            value = log(10.0);
        } else if (name == u"LN2") {
            value = log(2.0);
        } else if (name == u"LOG10E") {
            value = log10(std::exp(1.0));
        } else if (name == u"LOG2E") {
            value = log2(std::exp(1.0));
        } else if (name == u"PI") {
            value = 3.14159265358979323846;
        } else if (name == u"SQRT1_2") {
            value = std::sqrt(0.5);
        } else if (name == u"SQRT2") {
            value = std::sqrt(2.0);
        } else {
            Q_UNREACHABLE();
        }

        m_body += m_state.accumulatorVariableOut + u" = "_s
                  + conversion(m_typeResolver->realType(), m_state.accumulatorOut(), toNumericString(value))
                  + u";\n"_s;
        return;
    }

    if (m_state.accumulatorOut().isImportNamespace()) {
        Q_ASSERT(m_state.accumulatorOut().variant() == QQmlJSRegisterContent::ModulePrefix);
        // If we have an object module prefix, we need to pass through the original object.
        if (m_state.accumulatorVariableIn != m_state.accumulatorVariableOut) {
            m_body += m_state.accumulatorVariableOut + u" = "_s
                    + conversion(m_state.accumulatorIn(), m_state.accumulatorOut(),
                                 m_state.accumulatorVariableIn)
                    + u";\n"_s;
        }
        return;
    }

    AccumulatorConverter registers(this);

    if (m_state.accumulatorOut().isEnumeration()) {
        generateEnumLookup(index);
        return;
    }

    const QString indexString = QString::number(index);
    const QString namespaceString = m_state.accumulatorIn().isImportNamespace()
            ? QString::number(m_state.accumulatorIn().importNamespace())
            : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_s;
    const auto accumulatorIn = m_state.accumulatorIn();
    const QQmlJSRegisterContent scope = m_state.accumulatorOut().scope();
    const bool isReferenceType = scope.containedType()->isReferenceType();

    switch (m_state.accumulatorOut().variant()) {
    case QQmlJSRegisterContent::Attachment: {
        if (isQmlScopeObject(m_state.accumulatorOut().attachee())) {
            generateTypeLookup(index);
            return;
        }
        if (!isReferenceType) {
            // This can happen on incomplete type information. We contextually know that the
            // type must be a QObject, but we cannot construct the inheritance chain. Then we
            // store it in a generic type. Technically we could even convert it to QObject*, but
            // that would be expensive.
            REJECT(u"attached object for non-QObject type"_s);
        }

        if (!m_state.accumulatorIn().storedType()->isReferenceType()) {
            // This can happen if we retroactively determine that the property might not be
            // what we think it is (ie, it can be shadowed).
            REJECT(u"attached object of potentially non-QObject base"_s);
        }

        rejectIfNonQObjectOut(u"non-QObject attached type"_s);

        const QString lookup = u"aotContext->loadAttachedLookup("_s + indexString
                + u", "_s + m_state.accumulatorVariableIn
                + u", &"_s + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_s
                + indexString + u", "_s + namespaceString + u", "_s
                + m_state.accumulatorVariableIn + u')';
        generateLookup(lookup, initialization);
        return;
    }
    case QQmlJSRegisterContent::Singleton:
    case QQmlJSRegisterContent::Script:
    case QQmlJSRegisterContent::MetaType: {
        generateTypeLookup(index);
        return;
    }
    default:
        break;
    }

    Q_ASSERT(m_state.accumulatorOut().isProperty());

    if (accumulatorIn.isStoredIn(m_typeResolver->jsValueType())) {
        REJECT(u"lookup in QJSValue"_s);
    } else if (isReferenceType) {
        const QString inputPointer = resolveQObjectPointer(
                    scope.containedType(), accumulatorIn, m_state.accumulatorVariableIn,
                    u"Cannot read property '%1' of %2"_s.arg(
                        m_jsUnitGenerator->lookupName(index)));
        const QString lookup = u"aotContext->getObjectLookup("_s + indexString
                + u", "_s + inputPointer + u", "_s
                + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut) + u')';
        const QString initialization = (m_state.isShadowable()
                                                ? u"aotContext->initGetObjectLookupAsVariant("_s
                                                : u"aotContext->initGetObjectLookup("_s)
                + indexString + u", "_s + inputPointer + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
    } else if ((scope.containedType()->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
                    || scope.contains(m_typeResolver->stringType()))
               && m_jsUnitGenerator->lookupName(index) == u"length"_s) {
        const QQmlJSScope::ConstPtr stored = accumulatorIn.storedType();
        if (stored->isListProperty()) {
            m_body += m_state.accumulatorVariableOut + u" = "_s;
            m_body += conversion(
                        originalType(m_state.accumulatorOut()),
                        m_state.accumulatorOut(),
                        m_state.accumulatorVariableIn + u".count("_s + u'&'
                            + m_state.accumulatorVariableIn + u')');
            m_body += u";\n"_s;
        } else if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
                   || stored == m_typeResolver->stringType()) {
            m_body += m_state.accumulatorVariableOut + u" = "_s
                    + conversion(originalType(m_state.accumulatorOut()),
                                 m_state.accumulatorOut(),
                                 m_state.accumulatorVariableIn + u".length()"_s)
                    + u";\n"_s;
        } else {
            REJECT(u"access to 'length' property of sequence wrapped in non-sequence"_s);
        }
    } else if (accumulatorIn.isStoredIn(m_typeResolver->variantMapType())) {
        QString mapLookup = m_state.accumulatorVariableIn + u"["_s
                + QQmlJSUtils::toLiteral(m_jsUnitGenerator->lookupName(index)) + u"]"_s;
        m_body += m_state.accumulatorVariableOut + u" = "_s;
        m_body += conversion(m_typeResolver->varType(), m_state.accumulatorOut(), mapLookup);
        m_body += u";\n"_s;
    } else {
        if (m_state.isRegisterAffectedBySideEffects(Accumulator))
            REJECT(u"reading from a value that's potentially affected by side effects"_s);

        const QString inputContentPointer = resolveValueTypeContentPointer(
                    scope.containedType(), accumulatorIn, m_state.accumulatorVariableIn,
                    u"Cannot read property '%1' of %2"_s.arg(
                        m_jsUnitGenerator->lookupName(index)));

        const QString lookup = u"aotContext->getValueLookup("_s + indexString
                + u", "_s + inputContentPointer
                + u", "_s + contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut)
                + u')';
        const QString initialization = u"aotContext->initGetValueLookup("_s
                + indexString + u", "_s
                + metaObject(scope.containedType()) + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut(), m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
    }
}

void QQmlJSCodeGenerator::generate_GetOptionalLookup(int index, int offset)
{
    INJECT_TRACE_INFO(generate_GetOptionalLookup);

    const QQmlJSRegisterContent accumulatorIn = m_state.accumulatorIn();
    QString accumulatorVarIn = m_state.accumulatorVariableIn;

    const auto &annotation = m_annotations[currentInstructionOffset()];
    if (accumulatorIn.storedType()->isReferenceType()) {
        m_body += u"if (!%1)\n"_s.arg(accumulatorVarIn);
        generateJumpCodeWithTypeConversions(offset);
    } else if (accumulatorIn.isStoredIn(m_typeResolver->varType())) {
        m_body += u"if (!%1.isValid() || ((%1.metaType().flags() & QMetaType::PointerToQObject) "
                  "&& %1.value<QObject *>() == nullptr))\n"_s.arg(accumulatorVarIn);
        generateJumpCodeWithTypeConversions(offset);
    } else if (accumulatorIn.isStoredIn(m_typeResolver->jsPrimitiveType())) {
        m_body += u"if (%1.equals(QJSPrimitiveUndefined()) "
                  "|| %1.equals(QJSPrimitiveNull()))\n"_s.arg(accumulatorVarIn);
        generateJumpCodeWithTypeConversions(offset);
    } else if (annotation.changedRegisterIndex == Accumulator
               && annotation.changedRegister.variant() == QQmlJSRegisterContent::Enum) {
        // Nothing
    } else if (accumulatorIn.isStoredIn(m_typeResolver->jsValueType())) {
        m_body += u"if (%1.isNull() || %1.isUndefined())\n"_s.arg(accumulatorVarIn);
        generateJumpCodeWithTypeConversions(offset);
    } else if (!m_typeResolver->canHoldUndefined(accumulatorIn.storage())) {
        // The base cannot hold undefined and isn't a reference type, generate a regular get lookup
    } else {
        Q_UNREACHABLE(); // No other accumulatorIn stored types should be possible
    }

    generate_GetLookupHelper(index);
}

void QQmlJSCodeGenerator::generate_StoreProperty(int nameIndex, int baseReg)
{
    Q_UNUSED(nameIndex)
    Q_UNUSED(baseReg)
    REJECT(u"StoreProperty"_s);
}

void QQmlJSCodeGenerator::generate_SetLookup(int index, int baseReg)
{
    INJECT_TRACE_INFO(generate_SetLookup);

    const QString indexString = QString::number(index);
    const QQmlJSScope::ConstPtr valueType = m_state.accumulatorIn().storedType();
    const QQmlJSRegisterContent property = m_state.readAccumulator();
    Q_ASSERT(property.isConversion());
    const QQmlJSScope::ConstPtr originalScope
        = m_typeResolver->original(property.conversionResultScope()).containedType();

    if (property.storedType().isNull()) {
        REJECT(u"SetLookup. Could not find property "
               + m_jsUnitGenerator->lookupName(index)
               + u" on type "
               + originalScope->internalName());
    }

    const QString object = registerVariable(baseReg);
    m_body += u"{\n"_s;
    QString variableIn;
    if (!m_state.accumulatorIn().contains(property.containedType())) {
        m_body += property.storedType()->augmentedInternalName() + u" converted = "_s
                + conversion(m_state.accumulatorIn(), property, consumedAccumulatorVariableIn())
                + u";\n"_s;
        variableIn = contentPointer(property, u"converted"_s);
    } else {
        variableIn = contentPointer(property, m_state.accumulatorVariableIn);
    }

    switch (originalScope->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::Reference: {
        const QString basePointer = resolveQObjectPointer(
                    originalScope, registerType(baseReg), object,
                    u"TypeError: Value is %1 and could not be converted to an object"_s);

        const QString lookup = u"aotContext->setObjectLookup("_s + indexString
                + u", "_s + basePointer + u", "_s + variableIn + u')';

        // We use the asVariant lookup also for non-shadowable properties if the input can hold
        // undefined since that may be a reset. See QQmlJSTypePropagator::generate_StoreProperty().
        const QString initialization
                = (property.contains(m_typeResolver->varType())
                                                ? u"aotContext->initSetObjectLookupAsVariant("_s
                                                : u"aotContext->initSetObjectLookup("_s)
                + indexString + u", "_s + basePointer + u')';
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSScope::AccessSemantics::Sequence: {
        const QString propertyName = m_jsUnitGenerator->lookupName(index);
        if (propertyName != u"length"_s)
            REJECT(u"setting non-length property on a sequence type"_s);

        if (!originalScope->isListProperty())
            REJECT(u"resizing sequence types (because of missing write-back)"_s);

        // We can resize without write back on a list property because it's actually a reference.
        m_body += u"const int begin = "_s + object + u".count(&" + object + u");\n"_s;
        m_body += u"const int end = "_s
                + (variableIn.startsWith(u'&') ? variableIn.mid(1) : (u'*' + variableIn))
                + u";\n"_s;
        m_body += u"for (int i = begin; i < end; ++i)\n"_s;
        m_body += u"    "_s + object + u".append(&"_s + object + u", nullptr);\n"_s;
        m_body += u"for (int i = begin; i > end; --i)\n"_s;
        m_body += u"    "_s + object + u".removeLast(&"_s + object + u')'
                + u";\n"_s;
        break;
    }
    case QQmlJSScope::AccessSemantics::Value: {
        const QQmlJSRegisterContent base = registerType(baseReg);
        const QString baseContentPointer = resolveValueTypeContentPointer(
                    originalScope, base, object,
                    u"TypeError: Value is %1 and could not be converted to an object"_s);

        const QString lookup = u"aotContext->setValueLookup("_s + indexString
                + u", "_s + baseContentPointer
                + u", "_s + variableIn + u')';

        // We use the asVariant lookup also for non-shadowable properties if the input can hold
        // undefined since that may be a reset. See QQmlJSTypePropagator::generate_StoreProperty().
        const QString initialization
                = (property.contains(m_typeResolver->varType())
                           ? u"aotContext->initSetValueLookupAsVariant("_s
                           : u"aotContext->initSetValueLookup("_s)
                + indexString + u", "_s + metaObject(originalScope) + u')';

        generateLookup(lookup, initialization);
        generateWriteBack(baseReg);

        break;
    }
    case QQmlJSScope::AccessSemantics::None:
        Q_UNREACHABLE();
        break;
    }

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Yield()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_YieldStar()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Resume(int)
{
    BYTECODE_UNIMPLEMENTED();
}

QString QQmlJSCodeGenerator::initAndCall(
        int argc, int argv, const QString &callMethodTemplate, const QString &initMethodTemplate,
        QString *outVar)
{
    QString args;

    if (m_state.changedRegisterIndex() == InvalidRegister ||
            m_state.accumulatorOut().contains(m_typeResolver->voidType())) {
        args = u"nullptr"_s;
    } else {
        *outVar = u"callResult"_s;
        const QQmlJSScope::ConstPtr outType = m_state.accumulatorOut().storedType();
        m_body += outType->augmentedInternalName() + u' ' + *outVar;
        m_body += u";\n";

        args = contentPointer(m_state.accumulatorOut(), *outVar);
    }

    // We may need to convert the arguments to the function call so that they match what the
    // function expects. They are passed as void* after all. We try to convert them where they
    // are created, but if they are read as different types in multiple places, we can't.
    QString argumentPreparation;
    for (int i = 0; i < argc; ++i) {
        const QQmlJSRegisterContent content = registerType(argv + i);
        const QQmlJSRegisterContent read = m_state.readRegister(argv + i);
        if (read.contains(content.containedType())) {
            args += u", "_s + contentPointer(read, registerVariable(argv + i));
        } else {
            const QString var = u"arg"_s + QString::number(i);
            argumentPreparation +=
                    u"    "_s + read.storedType()->augmentedInternalName() + u' ' + var + u" = "_s
                    + conversion(content, read, consumedRegisterVariable(argv + i)) + u";\n";
            args += u", "_s + contentPointer(read, var);
        }
    }

    QString initMethod;

    if (m_state.isShadowable()) {
        initMethod = initMethodTemplate;
    } else {
        const QQmlJSMetaMethod method = m_state.accumulatorOut().methodCall();
        Q_ASSERT(!method.isConstructor());

        const QQmlJSMetaMethod::RelativeFunctionIndex relativeMethodIndex =
                method.isJavaScriptFunction() ? method.jsFunctionIndex() : method.methodIndex();
        initMethod = initMethodTemplate.arg(int(relativeMethodIndex));
    }

    return u"const auto doCall = [&]() {\n"_s
            + argumentPreparation
            + u"    void *args[] = {" + args + u"};\n"_s
            + u"    return aotContext->"_s + callMethodTemplate.arg(u"args"_s).arg(argc) + u";\n"
            + u"};\n"_s
            + u"const auto doInit = [&]() {\n"_s
            + u"    aotContext->"_s + initMethod + u";\n"
            + u"};\n"_s;
}

void QQmlJSCodeGenerator::generateMoveOutVar(const QString &outVar)
{
    if (m_state.accumulatorVariableOut.isEmpty() || outVar.isEmpty())
        return;

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += u"std::move(" + outVar + u");\n";
}

void QQmlJSCodeGenerator::generate_CallValue(int name, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallProperty(int nameIndex, int baseReg, int argc, int argv)
{
    Q_UNUSED(nameIndex);
    Q_UNUSED(baseReg);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    REJECT(u"CallProperty"_s);
}

bool QQmlJSCodeGenerator::inlineStringMethod(const QString &name, int base, int argc, int argv)
{
    if (name != u"arg"_s || argc != 1)
        return false;

    const auto arg = [&](const QQmlJSScope::ConstPtr &type) {
        return convertStored(registerType(argv).storedType(), type, consumedRegisterVariable(argv));
    };

    const auto ret = [&](const QString &arg) {
        const QString expression = convertStored(
                    registerType(base).storedType(), m_typeResolver->stringType(),
                    consumedRegisterVariable(base)) + u".arg("_s + arg + u')';
        return conversion(
                m_typeResolver->stringType(), m_state.accumulatorOut(), expression);
    };

    const QQmlJSRegisterContent input = m_state.readRegister(argv);
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    if (m_typeResolver->isNumeric(input))
        m_body += ret(arg(input.containedType()));
    else if (input.contains(m_typeResolver->boolType()))
        m_body += ret(arg(m_typeResolver->boolType()));
    else
        m_body += ret(arg(m_typeResolver->stringType()));
    m_body += u";\n"_s;
    return true;
}

bool QQmlJSCodeGenerator::inlineTranslateMethod(const QString &name, int argc, int argv)
{
    addInclude(u"QtCore/qcoreapplication.h"_s);

    const auto arg = [&](int i, const QQmlJSScope::ConstPtr &type) {
        Q_ASSERT(i < argc);
        return convertStored(registerType(argv + i).storedType(), type,
                             consumedRegisterVariable(argv + i));
    };

    const auto stringArg = [&](int i) {
        return i < argc
                ? (arg(i, m_typeResolver->stringType()) + u".toUtf8().constData()"_s)
                : u"\"\""_s;
    };

    const auto intArg = [&](int i) {
        return i < argc ? arg(i, m_typeResolver->int32Type()) : u"-1"_s;
    };

    const auto stringRet = [&](const QString &expression) {
        return conversion(
                m_typeResolver->stringType(), m_state.accumulatorOut(), expression);
    };

    const auto capture = [&]() {
        m_body += u"aotContext->captureTranslation();\n"_s;
    };

    if (name == u"QT_TRID_NOOP"_s || name == u"QT_TR_NOOP"_s) {
        Q_ASSERT(argc > 0);
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(arg(0, m_typeResolver->stringType())) + u";\n"_s;
        return true;
    }

    if (name == u"QT_TRANSLATE_NOOP"_s) {
        Q_ASSERT(argc > 1);
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(arg(1, m_typeResolver->stringType())) + u";\n"_s;
        return true;
    }

    if (name == u"qsTrId"_s) {
        capture();
        // We inline qtTrId() here because in the !QT_CONFIG(translation) case it's unavailable.
        // QCoreApplication::translate() is always available in some primitive form.
        // Also, this saves a function call.
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate(nullptr, "_s + stringArg(0) +
                            u", nullptr, "_s + intArg(1) + u")"_s) + u";\n"_s;
        return true;
    }

    if (name == u"qsTr"_s) {
        capture();
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate("_s
                            + u"aotContext->translationContext().toUtf8().constData(), "_s
                            + stringArg(0) + u", "_s + stringArg(1) + u", "_s
                            + intArg(2) + u")"_s) + u";\n"_s;
        return true;
    }

    if (name == u"qsTranslate"_s) {
        capture();
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + stringRet(u"QCoreApplication::translate("_s
                            + stringArg(0) + u", "_s + stringArg(1) + u", "_s
                            + stringArg(2) + u", "_s + intArg(3) + u")"_s) + u";\n"_s;
        return true;
    }

    return false;
}

static QString maxExpression(int argc)
{
    Q_ASSERT_X(argc >= 2, Q_FUNC_INFO, "max() expects at least two arguments.");

    QString expression =
            u"[&]() { \nauto  tmpMax = (qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == 1) ? arg2 : ((arg2 > arg1 || std::isnan(arg2)) ? arg2 : arg1);\n"_s;
    for (int i = 2; i < argc; i++) {
        expression +=
                "\ttmpMax = (qIsNull(%1) && qIsNull(tmpMax) && std::copysign(1.0, %1) == 1) ? arg2 : ((%1 > tmpMax || std::isnan(%1)) ? %1 : tmpMax);\n"_L1
                        .arg("arg"_L1 + QString::number(i + 1));
    }
    expression += "return tmpMax;\n}()"_L1;

    return expression;
}

static QString minExpression(int argc)
{
    Q_ASSERT_X(argc >= 2, Q_FUNC_INFO, "min() expects at least two arguments.");

    QString expression =
            u"[&]() { \nauto  tmpMin = (qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == -1) ? arg2 : ((arg2 < arg1 || std::isnan(arg2)) ? arg2 : arg1);\n"_s;
    for (int i = 2; i < argc; i++) {
        expression +=
                "tmpMin = (qIsNull(%1) && qIsNull(tmpMin) && std::copysign(1.0, %1) == -1) ? arg2 : ((%1 < tmpMin || std::isnan(%1)) ? %1 : tmpMin);\n"_L1
                        .arg("arg"_L1 + QString::number(i + 1));
    }
    expression += "return tmpMin;\n}()"_L1;

    return expression;
}

bool QQmlJSCodeGenerator::inlineMathMethod(const QString &name, int argc, int argv)
{
    addInclude(u"cmath"_s);
    addInclude(u"limits"_s);
    addInclude(u"QtCore/qalgorithms.h"_s);
    addInclude(u"QtCore/qrandom.h"_s);
    addInclude(u"QtQml/qjsprimitivevalue.h"_s);

    // If the result is not stored, we don't need to generate any code. All the math methods are
    // conceptually pure functions.
    if (m_state.changedRegisterIndex() != Accumulator)
        return true;

    m_body += u"{\n"_s;
    for (int i = 0; i < argc; ++i) {
        m_body += u"const double arg%1 = "_s.arg(i + 1) + convertStored(
                        registerType(argv + i).storedType(),
                        m_typeResolver->realType(), consumedRegisterVariable(argv + i))
                + u";\n"_s;
    }

    const QString qNaN = u"std::numeric_limits<double>::quiet_NaN()"_s;
    const QString inf = u"std::numeric_limits<double>::infinity()"_s;
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    QString expression;

    if (name == u"abs" && argc == 1) {
        expression = u"(qIsNull(arg1) ? 0 : (arg1 < 0.0 ? -arg1 : arg1))"_s;
    } else if (name == u"acos"_s && argc == 1) {
        expression = u"arg1 > 1.0 ? %1 : std::acos(arg1)"_s.arg(qNaN);
    } else if (name == u"acosh"_s && argc == 1) {
        expression = u"arg1 < 1.0 ? %1 : std::acosh(arg1)"_s.arg(qNaN);
    } else if (name == u"asin"_s && argc == 1) {
        expression = u"arg1 > 1.0 ? %1 : std::asin(arg1)"_s.arg(qNaN);
    } else if (name == u"asinh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::asinh(arg1)"_s;
    } else if (name == u"atan"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::atan(arg1)"_s;
    } else if (name == u"atanh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::atanh(arg1)"_s;
    } else if (name == u"atan2"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"cbrt"_s && argc == 1) {
        expression = u"std::cbrt(arg1)"_s;
    } else if (name == u"ceil"_s && argc == 1) {
        expression = u"(arg1 < 0.0 && arg1 > -1.0) ? std::copysign(0.0, -1.0) : std::ceil(arg1)"_s;
    } else if (name == u"clz32"_s && argc == 1) {
        expression = u"qint32(qCountLeadingZeroBits(quint32(QJSNumberCoercion::toInteger(arg1))))"_s;
    } else if (name == u"cos"_s && argc == 1) {
        expression = u"std::cos(arg1)"_s;
    } else if (name == u"cosh"_s && argc == 1) {
        expression = u"std::cosh(arg1)"_s;
    } else if (name == u"exp"_s && argc == 1) {
        expression = u"std::isinf(arg1) "
                "? (std::copysign(1.0, arg1) == -1 ? 0.0 : %1) "
                ": std::exp(arg1)"_s.arg(inf);
    } else if (name == u"expm1"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"floor"_s && argc == 1) {
        expression = u"std::floor(arg1)"_s;
    } else if (name == u"fround"_s && argc == 1) {
        expression = u"(std::isnan(arg1) || std::isinf(arg1) || qIsNull(arg1)) "
                "? arg1 "
                ": double(float(arg1))"_s;
    } else if (name == u"hypot"_s) {
        // TODO: complicated
        return false;
    } else if (name == u"imul"_s && argc == 2) {
        expression = u"qint32(quint32(QJSNumberCoercion::toInteger(arg1)) "
                "* quint32(QJSNumberCoercion::toInteger(arg2)))"_s;
    } else if (name == u"log"_s && argc == 1) {
        expression = u"arg1 < 0.0 ? %1 : std::log(arg1)"_s.arg(qNaN);
    } else if (name == u"log10"_s && argc == 1) {
        expression = u"arg1 < 0.0 ? %1 : std::log10(arg1)"_s.arg(qNaN);
    } else if (name == u"log1p"_s && argc == 1) {
        expression = u"arg1 < -1.0 ? %1 : std::log1p(arg1)"_s.arg(qNaN);
    } else if (name == u"log2"_s && argc == 1) {
        expression = u"arg1 < -0.0 ? %1 : std::log2(arg1)"_s.arg(qNaN);
    } else if (name == u"max"_s && argc >= 2) {
        expression = maxExpression(argc);
    } else if (name == u"min"_s && argc >= 2) {
        expression = minExpression(argc);
    } else if (name == u"pow"_s) {
        expression = u"QQmlPrivate::jsExponentiate(arg1, arg2)"_s;
    } else if (name == u"random"_s && argc == 0) {
        expression = u"QRandomGenerator::global()->generateDouble()"_s;
    } else if (name == u"round"_s && argc == 1) {
        expression = u"std::isfinite(arg1) "
                "? ((arg1 < 0.5 && arg1 >= -0.5) "
                    "? std::copysign(0.0, arg1) "
                    ": std::floor(arg1 + 0.5)) "
                ": arg1"_s;
    } else if (name == u"sign"_s && argc == 1) {
        expression = u"std::isnan(arg1) "
                "? %1 "
                ": (qIsNull(arg1) "
                    "? arg1 "
                    ": (std::signbit(arg1) ? -1.0 : 1.0))"_s.arg(qNaN);
    } else if (name == u"sin"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::sin(arg1)"_s;
    } else if (name == u"sinh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::sinh(arg1)"_s;
    } else if (name == u"sqrt"_s && argc == 1) {
        expression = u"std::sqrt(arg1)"_s;
    } else if (name == u"tan"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::tan(arg1)"_s;
    } else if (name == u"tanh"_s && argc == 1) {
        expression = u"qIsNull(arg1) ? arg1 : std::tanh(arg1)"_s;
    } else if (name == u"trunc"_s && argc == 1) {
        expression = u"std::trunc(arg1)"_s;
    } else {
        return false;
    }

    m_body += conversion(m_typeResolver->realType(), m_state.accumulatorOut(), expression);

    m_body += u";\n"_s;
    m_body += u"}\n"_s;
    return true;
}

static QString messageTypeForMethod(const QString &method)
{
    if (method == u"log" || method == u"debug")
        return u"QtDebugMsg"_s;
    if (method == u"info")
        return u"QtInfoMsg"_s;
    if (method == u"warn")
        return u"QtWarningMsg"_s;
    if (method == u"error")
        return u"QtCriticalMsg"_s;
    return QString();
}

bool QQmlJSCodeGenerator::inlineConsoleMethod(const QString &name, int argc, int argv)
{
    const QString type = messageTypeForMethod(name);
    if (type.isEmpty())
        return false;

    addInclude(u"QtCore/qloggingcategory.h"_s);

    m_body += u"{\n";
    m_body += u"    bool firstArgIsCategory = false;\n";
    const QQmlJSRegisterContent firstArg = argc > 0 ? registerType(argv) : QQmlJSRegisterContent();

    // We could check whether the first argument is a QQmlLoggingCategoryBase here, and we should
    // because QQmlLoggingCategoryBase is now a builtin.
    // TODO: The run time check for firstArg is obsolete.
    const bool firstArgIsReference = argc > 0
            && firstArg.containedType()->isReferenceType();

    if (firstArgIsReference) {
        m_body += u"    QObject *firstArg = ";
        m_body += convertStored(
                    firstArg.storedType(),
                    m_typeResolver->genericType(firstArg.storedType()),
                    registerVariable(argv));
        m_body += u";\n";
    }

    m_body += u"    const QLoggingCategory *category = aotContext->resolveLoggingCategory(";
    m_body += firstArgIsReference ? u"firstArg"_sv : u"nullptr"_sv;
    m_body += u", &firstArgIsCategory);\n";
    m_body += u"    if (category && category->isEnabled(" + type + u")) {\n";

    m_body += u"        const QString message = ";

    const auto stringConversion = [&](int i) -> QString {
        const QQmlJSScope::ConstPtr read = m_state.readRegister(argv + i).storedType();
        const QQmlJSScope::ConstPtr actual = registerType(argv + i).storedType();
        if (read == m_typeResolver->stringType()) {
            return convertStored(actual, read, consumedRegisterVariable(argv + i));
        } else if (actual->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence) {
            addInclude(u"QtQml/qjslist.h"_s);
            return u"u'[' + QJSList(&"_s + registerVariable(argv + i)
                    + u", aotContext->engine).toString() + u']'"_s;
        } else {
            REJECT<QString>(u"converting arguments for console method to string"_s);
        }
    };

    if (argc > 0) {
        if (firstArgIsReference) {
            const QString firstArgStringConversion = convertStored(
                    registerType(argv).storedType(),
                    m_typeResolver->stringType(), registerVariable(argv));
            m_body += u"(firstArgIsCategory ? QString() : (" + firstArgStringConversion;
            if (argc > 1)
                m_body += u".append(QLatin1Char(' ')))).append(";
            else
                m_body += u"))";
        } else {
            m_body += stringConversion(0);
            if (argc > 1)
                m_body += u".append(QLatin1Char(' ')).append(";
        }

        for (int i = 1; i < argc; ++i) {
            if (i > 1)
                m_body += u".append(QLatin1Char(' ')).append("_s;
            m_body += stringConversion(i) + u')';
        }
    } else {
        m_body += u"QString()";
    }
    m_body += u";\n        ";
    generateSetInstructionPointer();
    m_body += u"        aotContext->writeToConsole(" + type + u", message, category);\n";
    m_body += u"    }\n";
    m_body += u"}\n";
    return true;
}

bool QQmlJSCodeGenerator::inlineArrayMethod(const QString &name, int base, int argc, int argv)
{
    const auto intType = m_typeResolver->int32Type();
    const auto valueType = registerType(base).storedType()->valueType();
    const auto boolType = m_typeResolver->boolType();
    const auto stringType = m_typeResolver->stringType();
    const auto baseType = registerType(base);

    const QString baseVar = registerVariable(base);
    const QString qjsListMethod = u"QJSList(&"_s + baseVar + u", aotContext->engine)."
            + name + u"(";

    addInclude(u"QtQml/qjslist.h"_s);

    if (name == u"includes" && argc > 0 && argc < 3) {
        QString call = qjsListMethod
                + convertStored(registerType(argv).storedType(), valueType,
                                consumedRegisterVariable(argv));
        if (argc == 2) {
            call += u", " + convertStored(registerType(argv + 1).storedType(), intType,
                                              consumedRegisterVariable(argv + 1));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(boolType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    if (name == u"toString" || (name == u"join" && argc < 2)) {
        QString call = qjsListMethod;
        if (argc == 1) {
            call += convertStored(registerType(argv).storedType(), stringType,
                                  consumedRegisterVariable(argv));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(stringType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    if (name == u"slice" && argc < 3) {
        QString call = qjsListMethod;
        for (int i = 0; i < argc; ++i) {
            if (i > 0)
                call += u", ";
            call += convertStored(registerType(argv + i).storedType(), intType,
                                   consumedRegisterVariable(argv + i));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s;
        if (baseType.storedType()->isListProperty())
            m_body += conversion(m_typeResolver->qObjectListType(), m_state.accumulatorOut(), call);
        else
            m_body += conversion(baseType, m_state.accumulatorOut(), call);
        m_body += u";\n"_s;

        return true;
    }

    if ((name == u"indexOf" || name == u"lastIndexOf") && argc > 0 && argc < 3) {
        QString call = qjsListMethod
                + convertStored(registerType(argv).storedType(), valueType,
                                consumedRegisterVariable(argv));
        if (argc == 2) {
            call += u", " + convertStored(registerType(argv + 1).storedType(), intType,
                                          consumedRegisterVariable(argv + 1));
        }
        call += u")";

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(intType, m_state.accumulatorOut(), call) + u";\n"_s;
        return true;
    }

    return false;
}

void QQmlJSCodeGenerator::generate_CallPropertyLookup(int index, int base, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallPropertyLookup);

    const QQmlJSRegisterContent scopeContent = m_state.accumulatorOut().scope();
    const QQmlJSScope::ConstPtr scope = scopeContent.containedType();

    AccumulatorConverter registers(this);

    const QQmlJSRegisterContent baseType = registerType(base);
    const QString name = m_jsUnitGenerator->lookupName(index);

    if (scope == m_typeResolver->mathObject()) {
        if (inlineMathMethod(name, argc, argv))
            return;
    } else if (scope == m_typeResolver->consoleObject()) {
        if (inlineConsoleMethod(name, argc, argv))
            return;
    } else if (scope == m_typeResolver->stringType()) {
        if (inlineStringMethod(name, base, argc, argv))
            return;
    } else if (baseType.storedType()->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence) {
        if (inlineArrayMethod(name, base, argc, argv))
            return;
    }

    if (m_state.accumulatorOut().isJavaScriptReturnValue())
        REJECT(u"call to untyped JavaScript function"_s);

    m_body += u"{\n"_s;
    QString outVar;

    if (scope->isReferenceType()) {
        const QString inputPointer = resolveQObjectPointer(
                scope, baseType, registerVariable(base),
                u"Cannot call method '%1' of %2"_s.arg(name));

        const QString initMethodTemplate = m_state.isShadowable()
                ? u"initCallObjectPropertyLookupAsVariant(%1, %2)"_s
                : u"initCallObjectPropertyLookup(%1, %2, %3)"_s;

        m_body += initAndCall(
                argc, argv,
                u"callObjectPropertyLookup(%1, %2, %3, %4)"_s.arg(index).arg(inputPointer),
                initMethodTemplate.arg(index).arg(inputPointer), &outVar);
    } else {
        const QQmlJSScope::ConstPtr originalScope
                = m_typeResolver->original(scopeContent).containedType();
        const QString inputPointer = resolveValueTypeContentPointer(
                originalScope, baseType, registerVariable(base),
                u"Cannot call method '%1' of %2"_s.arg(name));

        m_body += initAndCall(
                argc, argv,
                u"callValueLookup(%1, %2, %3, %4)"_s.arg(index).arg(inputPointer),
                u"initCallValueLookup(%1, %2, %3)"_s
                        .arg(index).arg(metaObject(originalScope)),
                &outVar);
    }

    const QString lookup = u"doCall()"_s;
    const QString initialization = u"doInit()"_s;
    const QString preparation = getLookupPreparation(m_state.accumulatorOut(), outVar, index);
    generateLookup(lookup, initialization, preparation);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_s;

    if (scope->isReferenceType())
        return;

    const QQmlJSMetaMethod method = m_state.accumulatorOut().methodCall();
    if (!method.isConst())
        generateWriteBack(base);
}

void QQmlJSCodeGenerator::generate_CallName(int name, int argc, int argv)
{
    Q_UNUSED(name);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    REJECT(u"CallName"_s);
}

void QQmlJSCodeGenerator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallGlobalLookup(int index, int argc, int argv)
{
    Q_UNUSED(index);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    REJECT(u"CallGlobalLookup"_s);
}

void QQmlJSCodeGenerator::generate_CallQmlContextPropertyLookup(int index, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallQmlContextPropertyLookup);

    if (m_state.accumulatorOut().scope().contains(m_typeResolver->jsGlobalObject())) {
        const QString name = m_jsUnitGenerator->stringForIndex(
                m_jsUnitGenerator->lookupNameIndex(index));
        if (inlineTranslateMethod(name, argc, argv))
            return;
    }

    if (m_state.accumulatorOut().isJavaScriptReturnValue())
        REJECT(u"call to untyped JavaScript function"_s);

    AccumulatorConverter registers(this);

    m_body += u"{\n"_s;
    QString outVar;
    m_body += initAndCall(
            argc, argv, u"callQmlContextPropertyLookup(%1, %2, %3)"_s.arg(index),
            u"initCallQmlContextPropertyLookup(%1, %2)"_s.arg(index), &outVar);

    const QString lookup = u"doCall()"_s;
    const QString initialization = u"doInit()"_s;
    const QString preparation = getLookupPreparation(m_state.accumulatorOut(), outVar, index);
    generateLookup(lookup, initialization, preparation);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Construct(int func, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_Construct);
    Q_UNUSED(func);

    const auto originalResult = originalType(m_state.accumulatorOut());

    if (originalResult.contains(m_typeResolver->dateTimeType())) {
        m_body += m_state.accumulatorVariableOut + u" = ";
        if (argc == 0) {
            m_body += conversion(
                    m_typeResolver->dateTimeType(), m_state.accumulatorOut(),
                    u"QDateTime::currentDateTime()"_s) + u";\n";
            return;
        }

        if (argc == 1 && m_state.readRegister(argv).contains(m_typeResolver->dateTimeType())) {
            m_body += conversion(
                        registerType(argv), m_state.readRegister(argv), registerVariable(argv))
                    + u";\n";
            return;
        }

        QString ctorArgs;
        constexpr int maxArgc = 7; // year, month, day, hours, minutes, seconds, milliseconds
        for (int i = 0; i < std::min(argc, maxArgc); ++i) {
            if (i > 0)
                ctorArgs += u", ";
            ctorArgs += conversion(
                    registerType(argv + i), m_state.readRegister(argv + i),
                    registerVariable(argv + i));
        }
        m_body += conversion(
                m_typeResolver->dateTimeType(), m_state.accumulatorOut(),
                u"aotContext->constructDateTime("_s + ctorArgs + u')') + u";\n";
        return;
    }

    if (originalResult.contains(m_typeResolver->variantListType())) {
        rejectIfBadArray();

        if (argc == 1 && m_state.readRegister(argv).contains(m_typeResolver->realType())) {
            addInclude(u"QtQml/qjslist.h"_s);

            const QString error = u"    aotContext->engine->throwError(QJSValue::RangeError, "_s
                    + u"QLatin1String(\"Invalid array length\"));\n"_s;

            const QString indexName = registerVariable(argv);
            const auto indexType = registerType(argv).containedType();
            if (!m_typeResolver->isNativeArrayIndex(indexType)) {
                m_body += u"if (!QJSNumberCoercion::isArrayIndex("_s + indexName + u")) {\n"_s
                        + error;
                generateReturnError();
                m_body += u"}\n"_s;
            } else if (!m_typeResolver->isUnsignedInteger(indexType)) {
                m_body += u"if ("_s + indexName + u" < 0) {\n"_s
                        + error;
                generateReturnError();
                m_body += u"}\n"_s;
            }

            m_body += m_state.accumulatorVariableOut + u" = "_s
                    + m_state.accumulatorOut().storedType()->internalName() + u"();\n"_s;
            m_body += u"QJSList(&"_s + m_state.accumulatorVariableOut
                    + u", aotContext->engine).resize("_s
                    + convertStored(
                              registerType(argv).storedType(), m_typeResolver->sizeType(),
                              consumedRegisterVariable(argv))
                    + u");\n"_s;
        } else if (!m_logger->currentFunctionHasCompileError()) {
            generateArrayInitializer(argc, argv);
        }
        return;
    }

    const QQmlJSScope::ConstPtr originalContained = originalResult.containedType();
    if (originalContained->isValueType() && originalResult.isMethodCall()) {
        const QQmlJSMetaMethod ctor = originalResult.methodCall();
        if (ctor.isJavaScriptFunction())
            REJECT(u"calling JavaScript constructor "_s + ctor.methodName());

        QList<QQmlJSRegisterContent> argumentTypes;
        QStringList arguments;
        for (int i = 0; i < argc; ++i) {
            argumentTypes.append(registerType(argv + i));
            arguments.append(consumedRegisterVariable(argv + i));
        }

        const QQmlJSScope::ConstPtr extension = originalContained->extensionType().scope;
        const QString result = generateCallConstructor(
                ctor, argumentTypes, arguments, metaType(originalContained),
                metaObject(extension ? extension : originalContained));

        m_body += m_state.accumulatorVariableOut + u" = "_s
                + conversion(m_pool->storedIn(originalResult, m_typeResolver->varType()),
                             m_state.accumulatorOut(), result)
                + u";\n"_s;

        return;
    }


    REJECT(u"Construct"_s);
}

void QQmlJSCodeGenerator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetUnwindHandler(int offset)
{
    Q_UNUSED(offset)
    REJECT(u"SetUnwindHandler"_s);
}

void QQmlJSCodeGenerator::generate_UnwindDispatch()
{
    REJECT(u"UnwindDispatch"_s);
}

void QQmlJSCodeGenerator::generate_UnwindToLabel(int level, int offset)
{
    Q_UNUSED(level)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeadTemporalZoneCheck(int name)
{
    Q_UNUSED(name)
    INJECT_TRACE_INFO(generate_DeadTemporalZoneCheck);
    // Nothing to do here. If we have statically asserted the dtz check in the type propagator
    // the value cannot be empty. Otherwise we can't get here.
}

void QQmlJSCodeGenerator::generate_ThrowException()
{
    INJECT_TRACE_INFO(generate_ThrowException);

    generateSetInstructionPointer();
    m_body += u"aotContext->engine->throwError("_s + conversion(
                    m_state.accumulatorIn(),
                    m_typeResolver->jsValueType(),
                    m_state.accumulatorVariableIn) + u");\n"_s;
    generateReturnError();
    m_skipUntilNextLabel = true;
    resetState();
}

void QQmlJSCodeGenerator::generate_GetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateCallContext()
{
    INJECT_TRACE_INFO(generate_CreateCallContext);

    m_body += u"{\n"_s;
}

void QQmlJSCodeGenerator::generate_PushCatchContext(int index, int nameIndex)
{
    Q_UNUSED(index)
    Q_UNUSED(nameIndex)
    REJECT(u"PushCatchContext"_s);
}

void QQmlJSCodeGenerator::generate_PushWithContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushBlockContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CloneBlockContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushScriptContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopScriptContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopContext()
{
    INJECT_TRACE_INFO(generate_PopContext);

    // Add an empty block before the closing brace, in case there was a bare label before it.
    m_body += u"{}\n}\n"_s;
}

void QQmlJSCodeGenerator::generate_GetIterator(int iterator)
{
    INJECT_TRACE_INFO(generate_GetIterator);

    addInclude(u"QtQml/qjslist.h"_s);
    const QQmlJSRegisterContent listType = m_state.accumulatorIn();
    if (!listType.isList())
        REJECT(u"iterator on non-list type"_s);

    const QQmlJSRegisterContent iteratorType = m_state.accumulatorOut();
    if (!iteratorType.isProperty())
        REJECT(u"using non-iterator as iterator"_s);

    const QString identifier = QString::number(iteratorType.baseLookupIndex());
    const QString iteratorName = m_state.accumulatorVariableOut + u"Iterator" + identifier;
    const QString listName = m_state.accumulatorVariableOut + u"List" + identifier;

    m_body += u"QJSListFor"_s
            + (iterator == int(QQmlJS::AST::ForEachType::In) ? u"In"_s : u"Of"_s)
            + u"Iterator "_s + iteratorName + u";\n";
    m_body += m_state.accumulatorVariableOut + u" = &" + iteratorName + u";\n";

    m_body += m_state.accumulatorVariableOut + u"->init(";
    if (iterator == int(QQmlJS::AST::ForEachType::In)) {
        if (!iteratorType.isStoredIn(m_typeResolver->forInIteratorPtr()))
            REJECT(u"using non-iterator as iterator"_s);
        m_body += u"QJSList(&" + m_state.accumulatorVariableIn + u", aotContext->engine)";
    }
    m_body += u");\n";

    if (iterator == int(QQmlJS::AST::ForEachType::Of)) {
        if (!iteratorType.isStoredIn(m_typeResolver->forOfIteratorPtr()))
            REJECT(u"using non-iterator as iterator"_s);
        m_body += u"const auto &" // Rely on life time extension for const refs
                + listName + u" = " + consumedAccumulatorVariableIn();
    }
}

void QQmlJSCodeGenerator::generate_IteratorNext(int value, int offset)
{
    INJECT_TRACE_INFO(generate_IteratorNext);

    Q_ASSERT(value == m_state.changedRegisterIndex());
    const QQmlJSRegisterContent iteratorContent = m_state.accumulatorIn();
    if (!iteratorContent.isProperty())
        REJECT(u"using non-iterator as iterator"_s);

    const QQmlJSScope::ConstPtr iteratorType = iteratorContent.storedType();
    const QString listName = m_state.accumulatorVariableIn
            + u"List" + QString::number(iteratorContent.baseLookupIndex());
    QString qjsList;
    if (iteratorType == m_typeResolver->forOfIteratorPtr())
        qjsList = u"QJSList(&" + listName + u", aotContext->engine)";
    else if (iteratorType != m_typeResolver->forInIteratorPtr())
        REJECT(u"using non-iterator as iterator"_s);

    m_body += u"if (" + m_state.accumulatorVariableIn + u"->hasNext(" + qjsList + u")) {\n    ";

    // We know that this works because we can do ->next() below.
    QQmlJSRegisterContent iteratorValue = m_typeResolver->extractNonVoidFromOptionalType(
            m_typeResolver->original(m_state.changedRegister()));
    iteratorValue = m_pool->storedIn(iteratorValue, iteratorValue.containedType());

    m_body += changedRegisterVariable() + u" = "
            + conversion(
                      iteratorValue, m_state.changedRegister(),
                      m_state.accumulatorVariableIn + u"->next(" + qjsList + u')')
            + u";\n";
    m_body += u"} else {\n    ";
    m_body += changedRegisterVariable() + u" = "
            + conversion(m_typeResolver->voidType(), m_state.changedRegister(), QString());
    m_body += u";\n    ";
    generateJumpCodeWithTypeConversions(offset);
    m_body += u"\n}"_s;
}

void QQmlJSCodeGenerator::generate_IteratorNextForYieldStar(int iterator, int object, int offset)
{
    Q_UNUSED(iterator)
    Q_UNUSED(object)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorClose()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DestructureRestElement()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteProperty(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteName(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TypeofName(int name)
{
    Q_UNUSED(name);
    REJECT(u"TypeofName"_s);
}

void QQmlJSCodeGenerator::generate_TypeofValue()
{
    REJECT(u"TypeofValue"_s);
}

void QQmlJSCodeGenerator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DefineArray(int argc, int args)
{
    INJECT_TRACE_INFO(generate_DefineArray);

    rejectIfBadArray();
    if (!m_logger->currentFunctionHasCompileError())
        generateArrayInitializer(argc, args);
}

void QQmlJSCodeGenerator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    INJECT_TRACE_INFO(generate_DefineObjectLiteral);

    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut().storedType();
    if (stored->accessSemantics() != QQmlJSScope::AccessSemantics::Value)
        REJECT(u"storing an object literal in a non-value type"_s);

    const QQmlJSScope::ConstPtr contained = m_state.accumulatorOut().containedType();

    const int classSize = m_jsUnitGenerator->jsClassSize(internalClassId);
    Q_ASSERT(argc >= classSize);

    const auto createVariantMap = [&]() {
        QString result;
        result += u"QVariantMap {\n";
        const QQmlJSScope::ConstPtr propType = m_typeResolver->varType();
        for (int i = 0; i < classSize; ++i) {
            result += u"{ "_s
                    + QQmlJSUtils::toLiteral(m_jsUnitGenerator->jsClassMember(internalClassId, i))
                    + u", "_s;
            const int currentArg = args + i;
            const QQmlJSScope::ConstPtr argType = registerType(currentArg).storedType();
            const QString consumedArg = consumedRegisterVariable(currentArg);
            result += convertStored(argType, propType, consumedArg) + u" },\n";
        }

        for (int i = classSize; i < argc; i += 3) {
            const int nameArg = args + i + 1;
            result += u"{ "_s
                    + conversion(
                              registerType(nameArg),
                              m_typeResolver->stringType(),
                              consumedRegisterVariable(nameArg))
                    + u", "_s;

            const int valueArg = args + i + 2;
            result += convertStored(
                              registerType(valueArg).storedType(),
                              propType,
                              consumedRegisterVariable(valueArg))
                    + u" },\n";
        }

        result += u"}";
        return result;
    };

    if (contained == m_typeResolver->varType() || contained == m_typeResolver->variantMapType()) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + createVariantMap() + u";\n"_s;
        return;
    }

    if (contained == m_typeResolver->jsValueType()) {
        m_body += m_state.accumulatorVariableOut + u" = aotContext->engine->toScriptValue("_s
                + createVariantMap() + u");\n"_s;
        return;
    }

    m_body += m_state.accumulatorVariableOut + u" = "_s + stored->augmentedInternalName();
    const bool isVariantOrPrimitive = (stored == m_typeResolver->varType())
            || (stored == m_typeResolver->jsPrimitiveType());

    if (m_state.accumulatorOut().contains(stored)) {
        m_body += u"()";
    } else if (isVariantOrPrimitive) {
        m_body += u'(' + metaType(m_state.accumulatorOut().containedType()) + u')';
    } else {
        REJECT(u"storing an object literal in an unsupported container %1"_s
                               .arg(stored->internalName()));
    }
    m_body += u";\n";

    if (argc == 0)
        return;

    bool isExtension = false;
    if (!m_typeResolver->canPopulate(contained, m_typeResolver->variantMapType(), &isExtension))
        REJECT(u"storing an object literal in a non-structured value type"_s);

    const QQmlJSScope::ConstPtr accessor = isExtension
            ? contained->extensionType().scope
            : contained;

    m_body += u"{\n";
    m_body += u"    const QMetaObject *meta = ";
    if (!isExtension && isVariantOrPrimitive)
        m_body += m_state.accumulatorVariableOut + u".metaType().metaObject()";
    else
        m_body += metaObject(accessor);
    m_body += u";\n";

    for (int i = 0; i < classSize; ++i) {
        m_body += u"    {\n";
        const QString propName = m_jsUnitGenerator->jsClassMember(internalClassId, i);
        const int currentArg = args + i;
        const QQmlJSRegisterContent propType = m_state.readRegister(currentArg);
        const QQmlJSRegisterContent argType = registerType(currentArg);
        const QQmlJSMetaProperty property = contained->property(propName);
        const QString consumedArg = consumedRegisterVariable(currentArg);
        QString argument = conversion(argType, propType, consumedArg);

        if (argument == consumedArg) {
            argument = registerVariable(currentArg);
        } else {
            m_body += u"        "_s + propType.storedType()->augmentedInternalName()
                    + u" arg = "_s + argument + u";\n";
            argument = u"arg"_s;
        }

        int index = property.index();
        if (index == -1)
            continue;

        const QString indexString = QString::number(index);
        m_body += u"        void *argv[] = { %1, nullptr };\n"_s
                          .arg(contentPointer(propType, argument));
        m_body += u"        meta->property("_s + indexString;
        m_body += u").enclosingMetaObject()->d.static_metacall(reinterpret_cast<QObject *>(";
        m_body += contentPointer(m_state.accumulatorOut(), m_state.accumulatorVariableOut);
        m_body += u"), QMetaObject::WriteProperty, " + indexString + u", argv);\n";
        m_body += u"    }\n";
    }

    // This is not implemented because we cannot statically determine the type of the value and we
    // don't want to rely on QVariant::convert() since that may give different results than
    // the JavaScript coercion. We might still make it work by querying the QMetaProperty
    // for its type at run time and runtime coercing to that, but we don't know whether that
    // still pays off.
    if (argc > classSize)
        REJECT(u"non-literal keys of object literals"_s);

    m_body += u"}\n";

}

void QQmlJSCodeGenerator::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    Q_UNUSED(classIndex)
    Q_UNUSED(heritage)
    Q_UNUSED(computedNames)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateMappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateUnmappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateRestParameter(int argIndex)
{
    Q_UNUSED(argIndex)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ConvertThisToObject()
{
    INJECT_TRACE_INFO(generate_ConvertThisToObject);

    m_body += changedRegisterVariable() + u" = "_s
            + conversion(m_typeResolver->qObjectType(), m_state.changedRegister(),
                         u"aotContext->thisObject()"_s)
            + u";\n"_s;
}

void QQmlJSCodeGenerator::generate_LoadSuperConstructor()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ToObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Jump(int offset)
{
    INJECT_TRACE_INFO(generate_Jump);

    generateJumpCodeWithTypeConversions(offset);
    m_skipUntilNextLabel = true;
    resetState();
}

void QQmlJSCodeGenerator::generate_JumpTrue(int offset)
{
    INJECT_TRACE_INFO(generate_JumpTrue);

    m_body += u"if ("_s;
    m_body += convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->boolType(),
                            m_state.accumulatorVariableIn);
    m_body += u") "_s;
    generateJumpCodeWithTypeConversions(offset);
}

void QQmlJSCodeGenerator::generate_JumpFalse(int offset)
{
    INJECT_TRACE_INFO(generate_JumpFalse);

    m_body += u"if (!"_s;
    m_body += convertStored(m_state.accumulatorIn().storedType(), m_typeResolver->boolType(),
                            m_state.accumulatorVariableIn);
    m_body += u") "_s;
    generateJumpCodeWithTypeConversions(offset);
}

void QQmlJSCodeGenerator::generate_JumpNoException(int offset)
{
    INJECT_TRACE_INFO(generate_JumpNoException);

    m_body += u"if (!context->engine->hasException()) "_s;
    generateJumpCodeWithTypeConversions(offset);
}

void QQmlJSCodeGenerator::generate_JumpNotUndefined(int offset)
{
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CheckException()
{
    INJECT_TRACE_INFO(generate_CheckException);

    generateExceptionCheck();
}

void QQmlJSCodeGenerator::generate_CmpEqNull()
{
    INJECT_TRACE_INFO(generate_CmpEqNull);
    generateEqualityOperation(literalType(m_typeResolver->nullType()), QString(), u"equals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpNeNull()
{
    INJECT_TRACE_INFO(generate_CmlNeNull);
    generateEqualityOperation(literalType(m_typeResolver->nullType()), QString(), u"equals"_s, true);
}

QString QQmlJSCodeGenerator::getLookupPreparation(
        QQmlJSRegisterContent content, const QString &var, int lookup)
{
    if (content.contains(content.storedType()))
        return QString();

    if (content.isStoredIn(m_typeResolver->varType())) {
        return var + u" = QVariant(aotContext->lookupResultMetaType("_s
                + QString::number(lookup) + u"))"_s;
    }

    if (content.isStoredIn(m_typeResolver->jsPrimitiveType())) {
        return var + u" = QJSPrimitiveValue(aotContext->lookupResultMetaType("_s
                + QString::number(lookup) + u"))"_s;
    }

    // TODO: We could make sure they're compatible, for example QObject pointers.
    return QString();
}

QString QQmlJSCodeGenerator::contentPointer(QQmlJSRegisterContent content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    if (content.contains(stored))
        return u'&' + var;

    if (content.isStoredIn(m_typeResolver->varType())
            || content.isStoredIn(m_typeResolver->jsPrimitiveType())) {
        return var + u".data()"_s;
    }

    if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return u'&' + var;

    if (m_typeResolver->isNumeric(content.storedType())
        && content.containedType()->scopeType() == QQmlSA::ScopeType::EnumScope) {
        return u'&' + var;
    }

    if (stored->isListProperty() && content.containedType()->isListProperty())
        return u'&' + var;

    REJECT<QString>(
            u"content pointer of unsupported wrapper type "_s + content.descriptiveName());
}

QString QQmlJSCodeGenerator::contentType(QQmlJSRegisterContent content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    const QQmlJSScope::ConstPtr contained = content.containedType();
    if (contained == stored)
        return metaTypeFromType(stored);

    if (stored == m_typeResolver->varType() || stored == m_typeResolver->jsPrimitiveType())
        return var + u".metaType()"_s; // We expect the container to be initialized

    if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return metaType(contained);

    const QQmlJSScope::ConstPtr nonComposite = QQmlJSScope::nonCompositeBaseType(contained);
    if (m_typeResolver->isNumeric(stored) && nonComposite->scopeType() == QQmlSA::ScopeType::EnumScope)
        return metaTypeFromType(nonComposite->baseType());

    if (stored->isListProperty() && contained->isListProperty())
        return metaType(contained);

    REJECT<QString>(
            u"content type of unsupported wrapper type "_s + content.descriptiveName());
}

void QQmlJSCodeGenerator::generate_CmpEqInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpEqInt);

    generateEqualityOperation(
            literalType(m_typeResolver->int32Type()), QString::number(lhsConst), u"equals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpNeInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpNeInt);

    generateEqualityOperation(
            literalType(m_typeResolver->int32Type()), QString::number(lhsConst), u"equals"_s, true);
}

void QQmlJSCodeGenerator::generate_CmpEq(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpEq);
    generateEqualityOperation(registerType(lhs), registerVariable(lhs), u"equals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpNe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpNe);
    generateEqualityOperation(registerType(lhs), registerVariable(lhs), u"equals"_s, true);
}

void QQmlJSCodeGenerator::generate_CmpGt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGt);
    generateCompareOperation(lhs, u">"_s);
}

void QQmlJSCodeGenerator::generate_CmpGe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGe);
    generateCompareOperation(lhs, u">="_s);
}

void QQmlJSCodeGenerator::generate_CmpLt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLt);
    generateCompareOperation(lhs, u"<"_s);
}

void QQmlJSCodeGenerator::generate_CmpLe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLe);
    generateCompareOperation(lhs, u"<="_s);
}

void QQmlJSCodeGenerator::generate_CmpStrictEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictEqual);
    generateEqualityOperation(registerType(lhs), registerVariable(lhs), u"strictlyEquals"_s, false);
}

void QQmlJSCodeGenerator::generate_CmpStrictNotEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictNotEqual);
    generateEqualityOperation(registerType(lhs), registerVariable(lhs), u"strictlyEquals"_s, true);
}

void QQmlJSCodeGenerator::generate_CmpIn(int lhs)
{
    Q_UNUSED(lhs)
    REJECT(u"CmpIn"_s);
}

void QQmlJSCodeGenerator::generate_CmpInstanceOf(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_As(int lhs)
{
    INJECT_TRACE_INFO(generate_As);

    const QString input = registerVariable(lhs);
    const QQmlJSRegisterContent inputContent = m_state.readRegister(lhs);
    const QQmlJSRegisterContent outputContent = m_state.accumulatorOut();

    // If the originalType output is a conversion, we're supposed to check for the contained
    // type and if it doesn't match, set the result to null or undefined.
    const QQmlJSRegisterContent originalContent = originalType(outputContent);
    QQmlJSScope::ConstPtr target;
    if (originalContent.containedType()->isReferenceType())
        target = originalContent.containedType();
    else if (originalContent.isConversion())
        target = m_typeResolver->extractNonVoidFromOptionalType(originalContent).containedType();
    else if (originalContent.variant() == QQmlJSRegisterContent::Cast)
        target = originalContent.containedType();

    if (!target)
        REJECT(u"type assertion to unknown type"_s);

    const bool isTrivial = m_typeResolver->inherits(
            m_typeResolver->originalContainedType(inputContent), target);

    m_body += m_state.accumulatorVariableOut + u" = "_s;

    if (!isTrivial && target->isReferenceType()) {
        const QQmlJSScope::ConstPtr genericContained = m_typeResolver->genericType(target);
        const QString inputConversion = inputContent.storedType()->isReferenceType()
                ? input
                : convertStored(inputContent.storedType(), genericContained, input);

        if (target->isComposite()
                && m_state.accumulatorIn().isStoredIn(m_typeResolver->metaObjectType())) {
            m_body += conversion(
                        genericContained, outputContent,
                        m_state.accumulatorVariableIn + u"->cast("_s + inputConversion + u')');
        } else {
            m_body += conversion(
                        genericContained, outputContent,
                        u'(' + metaObject(target) + u")->cast("_s + inputConversion + u')');
        }
        m_body += u";\n"_s;
        return;
    }

    if (inputContent.isStoredIn(m_typeResolver->varType())
            || inputContent.isStoredIn(m_typeResolver->jsPrimitiveType())) {

        const auto source = m_typeResolver->extractNonVoidFromOptionalType(
                originalType(inputContent)).containedType();

        if (source && source == target) {
            m_body += input + u".metaType() == "_s + metaType(target)
                    + u" ? " + conversion(inputContent, outputContent, input)
                    + u" : " + conversion(
                                  literalType(m_typeResolver->voidType()), outputContent, QString());
            m_body += u";\n"_s;
            return;
        }
    }

    if (isTrivial) {
        // No actual conversion necessary. The 'as' is a no-op
        m_body += conversion(inputContent, m_state.accumulatorOut(), input) + u";\n"_s;
        return;
    }

    REJECT(u"non-trivial value type assertion"_s);
}

void QQmlJSCodeGenerator::generate_UNot()
{
    INJECT_TRACE_INFO(generate_UNot);
    generateUnaryOperation(u"!"_s);
}

void QQmlJSCodeGenerator::generate_UPlus()
{
    INJECT_TRACE_INFO(generate_UPlus);
    generateUnaryOperation(u"+"_s);
}

void QQmlJSCodeGenerator::generate_UMinus()
{
    INJECT_TRACE_INFO(generate_UMinus);
    generateUnaryOperation(u"-"_s);
}

void QQmlJSCodeGenerator::generate_UCompl()
{
    INJECT_TRACE_INFO(generate_UCompl);
    generateUnaryOperation(u"~"_s);
}

void QQmlJSCodeGenerator::generate_Increment()
{
    INJECT_TRACE_INFO(generate_Increment);
    generateInPlaceOperation(u"++"_s);
}

void QQmlJSCodeGenerator::generate_Decrement()
{
    INJECT_TRACE_INFO(generate_Decrement);
    generateInPlaceOperation(u"--"_s);
}

void QQmlJSCodeGenerator::generate_Add(int lhs)
{
    INJECT_TRACE_INFO(generate_Add);
    generateArithmeticOperation(lhs, u"+"_s);
}

void QQmlJSCodeGenerator::generate_BitAnd(int lhs)
{
    INJECT_TRACE_INFO(generate_BitAnd);
    generateArithmeticOperation(lhs, u"&"_s);
}

void QQmlJSCodeGenerator::generate_BitOr(int lhs)
{
    INJECT_TRACE_INFO(generate_BitOr);
    generateArithmeticOperation(lhs, u"|"_s);
}

void QQmlJSCodeGenerator::generate_BitXor(int lhs)
{
    INJECT_TRACE_INFO(generate_BitXor);
    generateArithmeticOperation(lhs, u"^"_s);
}

void QQmlJSCodeGenerator::generate_UShr(int lhs)
{
    INJECT_TRACE_INFO(generate_BitUShr);
    generateShiftOperation(lhs, u">>"_s);
}

void QQmlJSCodeGenerator::generate_Shr(int lhs)
{
    INJECT_TRACE_INFO(generate_Shr);
    generateShiftOperation(lhs, u">>"_s);
}

void QQmlJSCodeGenerator::generate_Shl(int lhs)
{
    INJECT_TRACE_INFO(generate_Shl);
    generateShiftOperation(lhs, u"<<"_s);
}

void QQmlJSCodeGenerator::generate_BitAndConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitAndConst);
    generateArithmeticConstOperation(rhs, u"&"_s);
}

void QQmlJSCodeGenerator::generate_BitOrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitOrConst);
    generateArithmeticConstOperation(rhs, u"|"_s);
}

void QQmlJSCodeGenerator::generate_BitXorConst(int rhs)
{
    INJECT_TRACE_INFO(generate_BitXorConst);
    generateArithmeticConstOperation(rhs, u"^"_s);
}

void QQmlJSCodeGenerator::generate_UShrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_UShrConst);
    generateArithmeticConstOperation(rhs & 0x1f, u">>"_s);
}

void QQmlJSCodeGenerator::generate_ShrConst(int rhs)
{
    INJECT_TRACE_INFO(generate_ShrConst);
    generateArithmeticConstOperation(rhs & 0x1f, u">>"_s);
}

void QQmlJSCodeGenerator::generate_ShlConst(int rhs)
{
    INJECT_TRACE_INFO(generate_ShlConst);
    generateArithmeticConstOperation(rhs & 0x1f, u"<<"_s);
}

void QQmlJSCodeGenerator::generate_Exp(int lhs)
{
    INJECT_TRACE_INFO(generate_Exp);

    const QString lhsString = conversion(
                registerType(lhs), m_state.readRegister(lhs), consumedRegisterVariable(lhs));
    const QString rhsString = conversion(
                m_state.accumulatorIn(), m_state.readAccumulator(),
                consumedAccumulatorVariableIn());

    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !lhsString.isEmpty());
    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !rhsString.isEmpty());

    const QQmlJSRegisterContent originalOut = originalType(m_state.accumulatorOut());
    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(
                originalOut, m_state.accumulatorOut(),
                u"QQmlPrivate::jsExponentiate("_s + lhsString + u", "_s + rhsString + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_Mul(int lhs)
{
    INJECT_TRACE_INFO(generate_Mul);
    generateArithmeticOperation(lhs, u"*"_s);
}

void QQmlJSCodeGenerator::generate_Div(int lhs)
{
    INJECT_TRACE_INFO(generate_Div);
    generateArithmeticOperation(lhs, u"/"_s);
}

void QQmlJSCodeGenerator::generate_Mod(int lhs)
{
    INJECT_TRACE_INFO(generate_Mod);

    const auto lhsVar = convertStored(
                registerType(lhs).storedType(), m_typeResolver->jsPrimitiveType(),
                consumedRegisterVariable(lhs));
    const auto rhsVar = convertStored(
                m_state.accumulatorIn().storedType(), m_typeResolver->jsPrimitiveType(),
                consumedAccumulatorVariableIn());
    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !lhsVar.isEmpty());
    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !rhsVar.isEmpty());

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    m_body += conversion(m_typeResolver->jsPrimitiveType(), m_state.accumulatorOut(),
                       u'(' + lhsVar + u" % "_s + rhsVar + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generate_Sub(int lhs)
{
    INJECT_TRACE_INFO(generate_Sub);
    generateArithmeticOperation(lhs, u"-"_s);
}

void QQmlJSCodeGenerator::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    Q_UNUSED(firstReg)
    Q_UNUSED(count)
    // Ignore. We reject uninitialized values anyway.
}

void QQmlJSCodeGenerator::generate_ThrowOnNullOrUndefined()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_GetTemplateObject(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSCodeGenerator::startInstruction(
        QV4::Moth::Instr::Type type)
{
    m_state.State::operator=(nextStateFromAnnotations(m_state, m_annotations));
    const auto accumulatorIn = m_state.registers.find(Accumulator);
    if (accumulatorIn != m_state.registers.end()
            && isTypeStorable(m_typeResolver, accumulatorIn.value().content.storedType())) {
        QQmlJSRegisterContent content = accumulatorIn.value().content;
        m_state.accumulatorVariableIn = m_registerVariables.value(RegisterVariablesKey {
            content.storedType()->internalName(),
            Accumulator,
            content.resultLookupIndex()
        }).variableName;
        Q_ASSERT(!m_state.accumulatorVariableIn.isEmpty());
    } else {
        m_state.accumulatorVariableIn.clear();
    }

    auto labelIt = m_labels.constFind(currentInstructionOffset());
    if (labelIt != m_labels.constEnd()) {
        m_body += *labelIt + u":;\n"_s;
        m_skipUntilNextLabel = false;
    } else if (m_skipUntilNextLabel && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    if (m_state.changedRegisterIndex() == Accumulator)
        m_state.accumulatorVariableOut = changedRegisterVariable();
    else
        m_state.accumulatorVariableOut.clear();

    // If the accumulator type is valid, we want an accumulator variable.
    // If not, we don't want one.
    Q_ASSERT(m_state.changedRegisterIndex() == Accumulator
             || m_state.accumulatorVariableOut.isEmpty());
    Q_ASSERT(m_state.changedRegisterIndex() != Accumulator
             || !m_state.accumulatorVariableOut.isEmpty()
             || !isTypeStorable(m_typeResolver, m_state.changedRegister().storedType()));

    // If the instruction has no side effects and doesn't write any register, it's dead.
    // We might still need the label, though, and the source code comment.
    if (!m_state.hasSideEffects() && changedRegisterVariable().isEmpty()) {
        generateJumpCodeWithTypeConversions(0);
        return SkipInstruction;
    }

    return ProcessInstruction;
}

void QQmlJSCodeGenerator::endInstruction(QV4::Moth::Instr::Type)
{
    if (!m_skipUntilNextLabel)
        generateJumpCodeWithTypeConversions(0);
    m_pool->clearTemporaries();
}

void QQmlJSCodeGenerator::generateSetInstructionPointer()
{
    m_body += u"aotContext->setInstructionPointer("_s
        + QString::number(nextInstructionOffset()) + u");\n"_s;
}

void QQmlJSCodeGenerator::generateExceptionCheck()
{
    m_body += u"if (aotContext->engine->hasError()) {\n"_s;
    generateReturnError();
    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generateEqualityOperation(
        QQmlJSRegisterContent lhsContent, QQmlJSRegisterContent rhsContent,
        const QString &lhsName, const QString &rhsName, const QString &function, bool invert)
{
    const bool lhsIsOptional = m_typeResolver->isOptionalType(lhsContent);
    const bool rhsIsOptional = m_typeResolver->isOptionalType(rhsContent);

    const QQmlJSScope::ConstPtr rhsContained = rhsIsOptional
            ? m_typeResolver->extractNonVoidFromOptionalType(rhsContent).containedType()
            : rhsContent.containedType();

    const QQmlJSScope::ConstPtr lhsContained = lhsIsOptional
            ? m_typeResolver->extractNonVoidFromOptionalType(lhsContent).containedType()
            : lhsContent.containedType();

    const bool isStrict = function == "strictlyEquals"_L1;
    const bool strictlyComparableWithVar
            = isStrict && canStrictlyCompareWithVar(m_typeResolver, lhsContained, rhsContained);
    auto isComparable = [&]() {
        if (m_typeResolver->isPrimitive(lhsContent) && m_typeResolver->isPrimitive(rhsContent))
            return true;
        if (m_typeResolver->isNumeric(lhsContent) && m_typeResolver->isNumeric(rhsContent))
            return true;
        if (m_typeResolver->isNumeric(lhsContent) && rhsContent.isEnumeration())
            return true;
        if (m_typeResolver->isNumeric(rhsContent) && lhsContent.isEnumeration())
            return true;
        if (strictlyComparableWithVar)
            return true;
        if (canCompareWithQObject(m_typeResolver, lhsContained, rhsContained))
            return true;
        if (canCompareWithQUrl(m_typeResolver, lhsContained, rhsContained))
            return true;
        return false;
    };

    const auto retrieveOriginal = [this](QQmlJSRegisterContent content) {
        const auto contained = content.containedType();
        const auto originalContent = originalType(content);
        const auto containedOriginal = originalContent.containedType();

        if (originalContent.isStoredIn(m_typeResolver->genericType(containedOriginal))) {
            // The original type doesn't need any wrapping.
            return originalContent;
        } else if (contained == containedOriginal) {
            if (originalContent.isConversion()) {
                // The original conversion origins are more accurate
                return m_pool->storedIn(originalContent, content.storedType());
            }
        } else if (m_typeResolver->canHold(contained, containedOriginal)) {
            return m_pool->storedIn(originalContent, content.storedType());
        }

        return content;
    };

    const QQmlJSScope::ConstPtr lhsType = lhsContent.storedType();
    const QQmlJSScope::ConstPtr rhsType = rhsContent.storedType();

    if (!isComparable()) {
        QQmlJSRegisterContent lhsOriginal = retrieveOriginal(lhsContent);
        QQmlJSRegisterContent rhsOriginal = retrieveOriginal(rhsContent);
        if (lhsOriginal.containedType() != lhsContent.containedType()
                || lhsOriginal.storedType() != lhsType
                || rhsOriginal.containedType() != rhsContent.containedType()
                || rhsOriginal.storedType() != rhsType) {
            // If either side is simply a wrapping of a specific type into a more general one, we
            // can compare the original types instead. You can't nest wrappings after all.
            generateEqualityOperation(lhsOriginal, rhsOriginal,
                                      conversion(lhsType, lhsOriginal, lhsName),
                                      conversion(rhsType, rhsOriginal, rhsName),
                                      function, invert);
            return;
        }

        REJECT(u"incomparable types %1 and %2"_s.arg(
                rhsContent.descriptiveName(), lhsContent.descriptiveName()));
    }

    if (strictlyComparableWithVar) {
        // Determine which side is holding a storable type
        if (!lhsName.isEmpty() && rhsName.isEmpty()) {
            // lhs register holds var type and rhs is not storable
            generateVariantEqualityComparison(rhsContent, lhsName, invert);
            return;
        }

        if (!rhsName.isEmpty() && lhsName.isEmpty()) {
            // lhs content is not storable and rhs is var type
            generateVariantEqualityComparison(lhsContent, rhsName, invert);
            return;
        }

        if (lhsContent.contains(m_typeResolver->varType())) {
            generateVariantEqualityComparison(rhsContent, rhsName, lhsName, invert);
            return;
        }

        if (rhsContent.contains(m_typeResolver->varType())) {
            generateVariantEqualityComparison(lhsContent, lhsName, rhsName, invert);
            return;
        }

        // It shouldn't be possible to get here because optional null should be stored in
        // QJSPrimitiveValue, not in QVariant. But let's rather be safe than sorry.
        REJECT(u"comparison of optional null"_s);
    }

    const auto comparison = [&]() -> QString {
        const auto primitive = m_typeResolver->jsPrimitiveType();
        const QString sign = invert ? u" != "_s : u" == "_s;

        if (lhsType == rhsType && lhsType != primitive && lhsType != m_typeResolver->varType()) {

            // Straight forward comparison of equal types,
            // except QJSPrimitiveValue which has two comparison functions.

            if (isTypeStorable(m_typeResolver, lhsType))
                return lhsName + sign + rhsName;

            // null === null and undefined === undefined
            return invert ? u"false"_s : u"true"_s;
        }

        if (canCompareWithQObject(m_typeResolver, lhsType, rhsType)) {
            // Comparison of QObject-derived with nullptr or different QObject-derived.
            return (isTypeStorable(m_typeResolver, lhsType) ? lhsName : u"nullptr"_s)
                    + sign
                    + (isTypeStorable(m_typeResolver, rhsType) ? rhsName : u"nullptr"_s);
        }

        if (canCompareWithQObject(m_typeResolver, lhsContained, rhsContained)) {
            // Comparison of optional QObject-derived with nullptr or different QObject-derived.
            // Mind that null == undefined but null !== undefined
            // Therefore the isStrict dance.

            QString result;
            if (isStrict) {
                if (lhsIsOptional) {
                    if (rhsIsOptional) {
                        // If both are invalid we're fine
                        result += u"(!"_s
                                + lhsName + u".isValid() && !"_s
                                + rhsName + u".isValid()) || "_s;
                    }

                    result += u'(' + lhsName + u".isValid() && "_s;
                } else {
                    result += u'(';
                }

                if (rhsIsOptional) {
                    result += rhsName + u".isValid() && "_s;
                }
            } else {
                result += u'(';
            }

            // We do not implement comparison with explicit undefined, yet. Only with null.
            Q_ASSERT(lhsType != m_typeResolver->voidType());
            Q_ASSERT(rhsType != m_typeResolver->voidType());

            const auto resolvedName = [&](const QString name) -> QString {
                // If isStrict we check validity already before.
                const QString content = u"*static_cast<QObject **>("_s + name + u".data())"_s;
                return isStrict
                        ? content
                        : u'(' + name + u".isValid() ? "_s + content + u" : nullptr)"_s;
            };

            const QString lhsResolved = lhsIsOptional ? resolvedName(lhsName) : lhsName;
            const QString rhsResolved = rhsIsOptional ? resolvedName(rhsName) : rhsName;

            return (invert ? u"!("_s : u"("_s) + result
                    + (isTypeStorable(m_typeResolver, lhsType) ? lhsResolved : u"nullptr"_s)
                    + u" == "_s
                    + (isTypeStorable(m_typeResolver, rhsType) ? rhsResolved : u"nullptr"_s)
                    + u"))"_s;
        }

        if ((m_typeResolver->isUnsignedInteger(rhsType)
                    && m_typeResolver->isUnsignedInteger(lhsType))
                || (m_typeResolver->isSignedInteger(rhsType)
                    && m_typeResolver->isSignedInteger(lhsType))) {
            // Both integers of same signedness: Let the C++ compiler perform the type promotion
            return lhsName + sign + rhsName;
        }

        if (rhsType == m_typeResolver->boolType() && m_typeResolver->isIntegral(lhsType)) {
            // Integral and bool: We can promote the bool to the integral type
            return lhsName + sign + convertStored(rhsType, lhsType, rhsName);
        }

        if (lhsType == m_typeResolver->boolType() && m_typeResolver->isIntegral(rhsType)) {
            // Integral and bool: We can promote the bool to the integral type
            return convertStored(lhsType, rhsType, lhsName) + sign + rhsName;
        }

        if (m_typeResolver->isNumeric(lhsType) && m_typeResolver->isNumeric(rhsType)) {
            // Both numbers: promote them to double
            return convertStored(lhsType, m_typeResolver->realType(), lhsName)
                    + sign
                    + convertStored(rhsType, m_typeResolver->realType(), rhsName);
        }

        // If none of the above matches, we have to use QJSPrimitiveValue
        return (invert ? u"!"_s : QString())
                + convertStored(lhsType, primitive, lhsName)
                + u'.' + function + u'(' + convertStored(rhsType, primitive, rhsName) + u')';
    };

    m_body += m_state.accumulatorVariableOut + u" = "_s;
    m_body += conversion(m_typeResolver->boolType(), m_state.accumulatorOut(), comparison());
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateCompareOperation(int lhs, const QString &cppOperator)
{
    m_body += m_state.accumulatorVariableOut + u" = "_s;

    const auto lhsType = registerType(lhs);
    const QQmlJSScope::ConstPtr compareType =
            m_typeResolver->isNumeric(lhsType) && m_typeResolver->isNumeric(m_state.accumulatorIn())
                ? m_typeResolver->merge(lhsType.storedType(), m_state.accumulatorIn().storedType())
                : m_typeResolver->jsPrimitiveType();

    m_body += conversion(
                m_typeResolver->boolType(), m_state.accumulatorOut(),
                convertStored(registerType(lhs).storedType(), compareType,
                              consumedRegisterVariable(lhs))
                    + u' ' + cppOperator + u' '
                    + convertStored(m_state.accumulatorIn().storedType(), compareType,
                                    consumedAccumulatorVariableIn()));
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateArithmeticOperation(int lhs, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(registerType(lhs), m_state.readRegister(lhs),
                           consumedRegisterVariable(lhs)),
                conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()),
                cppOperator);
}

void QQmlJSCodeGenerator::generateShiftOperation(int lhs, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(registerType(lhs), m_state.readRegister(lhs),
                           consumedRegisterVariable(lhs)),
                u'(' + conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()) + u" & 0x1f)"_s,
                cppOperator);
}

void QQmlJSCodeGenerator::generateArithmeticOperation(
        const QString &lhs, const QString &rhs, const QString &cppOperator)
{
    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !lhs.isEmpty());
    Q_ASSERT(m_logger->currentFunctionHasCompileError() || !rhs.isEmpty());

    const QQmlJSRegisterContent originalOut = originalType(m_state.accumulatorOut());
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_s;
    const QString explicitCast
            = originalOut.isStoredIn(m_typeResolver->stringType())
                ? originalOut.storedType()->internalName()
                : QString();
    m_body += conversion(
                originalOut, m_state.accumulatorOut(),
                explicitCast + u'(' + lhs + u' ' + cppOperator + u' ' + rhs + u')');
    m_body += u";\n"_s;
}

void QQmlJSCodeGenerator::generateArithmeticConstOperation(int rhsConst, const QString &cppOperator)
{
    generateArithmeticOperation(
                conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                           consumedAccumulatorVariableIn()),
                conversion(literalType(m_typeResolver->int32Type()),
                           m_state.readAccumulator(), QString::number(rhsConst)),
                cppOperator);
}

void QQmlJSCodeGenerator::generateUnaryOperation(const QString &cppOperator)
{
    const auto var = conversion(m_state.accumulatorIn(),
                                originalType(m_state.readAccumulator()),
                                consumedAccumulatorVariableIn());

    if (var == m_state.accumulatorVariableOut) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + cppOperator + var + u";\n"_s;
        return;
    }

    const auto originalResult = originalType(m_state.accumulatorOut());
    if (m_state.accumulatorOut() == originalResult) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + var + u";\n"_s;
        m_body += m_state.accumulatorVariableOut + u" = "_s
                + cppOperator + m_state.accumulatorVariableOut + u";\n"_s;
        return;
    }

    m_body += m_state.accumulatorVariableOut + u" = "_s + conversion(
                originalResult, m_state.accumulatorOut(), cppOperator + var) + u";\n"_s;
}

void QQmlJSCodeGenerator::generateInPlaceOperation(const QString &cppOperator)
{
    {
        // If actually in place, we cannot consume the variable.
        const QString var = conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                                       m_state.accumulatorVariableIn);
        if (var == m_state.accumulatorVariableOut) {
            m_body += cppOperator + var + u";\n"_s;
            return;
        }
    }

    const QString var = conversion(m_state.accumulatorIn(), m_state.readAccumulator(),
                                   consumedAccumulatorVariableIn());

    const auto originalResult = originalType(m_state.accumulatorOut());
    if (m_state.accumulatorOut() == originalResult) {
        m_body += m_state.accumulatorVariableOut + u" = "_s + var + u";\n"_s;
        m_body += cppOperator + m_state.accumulatorVariableOut + u";\n"_s;
        return;
    }

    m_body += u"{\n"_s;
    m_body += u"auto converted = "_s + var + u";\n"_s;
    m_body += m_state.accumulatorVariableOut + u" = "_s + conversion(
                originalResult, m_state.accumulatorOut(), u'('
                + cppOperator + u"converted)"_s)  + u";\n"_s;
    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generateLookup(const QString &lookup, const QString &initialization,
                                        const QString &resultPreparation)
{
    m_body += u"#ifndef QT_NO_DEBUG\n"_s;
    generateSetInstructionPointer();
    m_body += u"#endif\n"_s;

    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_s;
    m_body += u"while (!"_s + lookup + u") {\n"_s;

    m_body += u"#ifdef QT_NO_DEBUG\n"_s;
    generateSetInstructionPointer();
    m_body += u"#endif\n"_s;

    m_body += initialization + u";\n"_s;
    generateExceptionCheck();
    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_s;
    m_body += u"}\n"_s;
}

void QQmlJSCodeGenerator::generateJumpCodeWithTypeConversions(int relativeOffset)
{
    QString conversionCode;
    const int absoluteOffset = nextInstructionOffset() + relativeOffset;
    const auto annotation = m_annotations.find(absoluteOffset);
    if (static_cast<InstructionAnnotations::const_iterator>(annotation) != m_annotations.constEnd()) {
        const auto &conversions = annotation->second.typeConversions;

        for (auto regIt = conversions.constBegin(), regEnd = conversions.constEnd();
             regIt != regEnd; ++regIt) {
            const QQmlJSRegisterContent targetType = regIt.value().content;
            if (!targetType.isValid() || !isTypeStorable(m_typeResolver, targetType.storedType()))
                continue;

            const int registerIndex = regIt.key();
            const auto variable = m_registerVariables.constFind(RegisterVariablesKey {
                    targetType.storedType()->internalName(),
                    registerIndex,
                    targetType.resultLookupIndex()
            });

            if (variable == m_registerVariables.constEnd())
                continue;

            QQmlJSRegisterContent currentType;
            QString currentVariable;
            if (registerIndex == m_state.changedRegisterIndex()) {
                currentVariable = changedRegisterVariable();
                if (variable->variableName == currentVariable)
                    continue;

                currentType = m_state.changedRegister();
                currentVariable = u"std::move("_s + currentVariable + u')';
            } else {
                const auto it = m_state.registers.find(registerIndex);
                if (it == m_state.registers.end()
                        || variable->variableName == registerVariable(registerIndex)) {
                    continue;
                }

                currentType = it.value().content;
                currentVariable = consumedRegisterVariable(registerIndex);
            }

            // Actually == here. We want the jump code also for equal types
            if (currentType == targetType)
                continue;

            conversionCode += variable->variableName;
            conversionCode += u" = "_s;
            conversionCode += conversion(currentType, targetType, currentVariable);
            conversionCode += u";\n"_s;
        }
    }

    if (relativeOffset) {
        auto labelIt = m_labels.find(absoluteOffset);
        if (labelIt == m_labels.end())
            labelIt = m_labels.insert(absoluteOffset, u"label_%1"_s.arg(m_labels.size()));
        conversionCode += u"    goto "_s + *labelIt + u";\n"_s;
    }

    m_body += u"{\n"_s + conversionCode + u"}\n"_s;
}

QString QQmlJSCodeGenerator::registerVariable(int index) const
{
    QQmlJSRegisterContent content = registerType(index);
    const auto it = m_registerVariables.constFind(RegisterVariablesKey {
        content.storedType()->internalName(),
        index,
        content.resultLookupIndex()
    });
    if (it != m_registerVariables.constEnd())
        return it->variableName;

    return QString();
}

QString QQmlJSCodeGenerator::lookupVariable(int lookupIndex) const
{
    for (auto it = m_registerVariables.constBegin(), end = m_registerVariables.constEnd(); it != end; ++it) {
        if (it.key().lookupIndex == lookupIndex)
            return it->variableName;
    }
    return QString();
}

QString QQmlJSCodeGenerator::consumedRegisterVariable(int index) const
{
    const QString var = registerVariable(index);
    if (var.isEmpty() || !shouldMoveRegister(index))
        return var;
    return u"std::move(" + var + u")";
}

QString QQmlJSCodeGenerator::consumedAccumulatorVariableIn() const
{
    return shouldMoveRegister(Accumulator)
            ? u"std::move(" + m_state.accumulatorVariableIn + u")"
            : m_state.accumulatorVariableIn;
}

QString QQmlJSCodeGenerator::changedRegisterVariable() const
{
    QQmlJSRegisterContent changedRegister = m_state.changedRegister();

    const QQmlJSScope::ConstPtr storedType = changedRegister.storedType();
    if (storedType.isNull())
        return QString();

    return m_registerVariables.value(RegisterVariablesKey {
        storedType->internalName(),
        m_state.changedRegisterIndex(),
        changedRegister.resultLookupIndex()
    }).variableName;
}

QQmlJSRegisterContent QQmlJSCodeGenerator::registerType(int index) const
{
    auto it = m_state.registers.find(index);
    if (it != m_state.registers.end())
        return it.value().content;

    return QQmlJSRegisterContent();
}

QQmlJSRegisterContent QQmlJSCodeGenerator::lookupType(int lookupIndex) const
{
    auto it = m_state.lookups.find(lookupIndex);
    if (it != m_state.lookups.end())
        return it.value().content;

    return QQmlJSRegisterContent();
}

bool QQmlJSCodeGenerator::shouldMoveRegister(int index) const
{
    return m_state.canMoveReadRegister(index)
            && !m_typeResolver->isTriviallyCopyable(m_state.readRegister(index).storedType());
}

QString QQmlJSCodeGenerator::conversion(
        QQmlJSRegisterContent from, QQmlJSRegisterContent to, const QString &variable)
{
    const QQmlJSScope::ConstPtr contained = to.containedType();

    // If from is QJSPrimitiveValue and to contains a primitive we coerce using QJSPrimitiveValue
    if (from.isStoredIn(m_typeResolver->jsPrimitiveType()) && m_typeResolver->isPrimitive(to)) {

        QString primitive = [&]() -> QString {
            if (contained == m_typeResolver->jsPrimitiveType())
                return variable;

            const QString conversion = variable + u".to<QJSPrimitiveValue::%1>()"_s;
            if (contained == m_typeResolver->boolType())
                return conversion.arg(u"Boolean"_s);
            if (m_typeResolver->isIntegral(to))
                return conversion.arg(u"Integer"_s);
            if (m_typeResolver->isNumeric(to))
                return conversion.arg(u"Double"_s);
            if (contained == m_typeResolver->stringType())
                return conversion.arg(u"String"_s);
            REJECT<QString>(
                    u"Conversion of QJSPrimitiveValue to "_s + contained->internalName());
        }();

        if (primitive.isEmpty())
            return primitive;

        return convertStored(m_typeResolver->jsPrimitiveType(), to.storedType(), primitive);
    }

    if (to.isStoredIn(contained)
            || m_typeResolver->isNumeric(to.storedType())
            || to.storedType()->isReferenceType()
            || from.contains(contained)) {
        // If:
        // * the output is not actually wrapped at all, or
        // * the output is stored in a numeric type (as there are no internals to a number), or
        // * the output is a QObject pointer, or
        // * we merely wrap the value into a new container,
        // we can convert by stored type.
        return convertStored(from.storedType(), to.storedType(), variable);
    } else {
        return convertContained(from, to, variable);
    }
}

QString QQmlJSCodeGenerator::convertStored(
        const QQmlJSScope::ConstPtr &from, const QQmlJSScope::ConstPtr &to, const QString &variable)
{
    // TODO: most values can be moved, which is much more efficient with the common types.
    //       add a move(from, to, variable) function that implements the moves.
    Q_ASSERT(!to->isComposite()); // We cannot directly convert to composites.

    const auto jsValueType = m_typeResolver->jsValueType();
    const auto varType = m_typeResolver->varType();
    const auto jsPrimitiveType = m_typeResolver->jsPrimitiveType();
    const auto boolType = m_typeResolver->boolType();

    auto zeroBoolOrInt = [&](const QQmlJSScope::ConstPtr &to) {
        if (to == boolType)
            return u"false"_s;
        if (m_typeResolver->isSignedInteger(to))
            return u"0"_s;
        if (m_typeResolver->isUnsignedInteger(to))
            return u"0u"_s;
        return QString();
    };

    if (from == m_typeResolver->voidType()) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
        const QString zero = zeroBoolOrInt(to);
        if (!zero.isEmpty())
            return zero;
        if (to == m_typeResolver->floatType())
            return u"std::numeric_limits<float>::quiet_NaN()"_s;
        if (to == m_typeResolver->realType())
            return u"std::numeric_limits<double>::quiet_NaN()"_s;
        if (to == m_typeResolver->stringType())
            return QQmlJSUtils::toLiteral(u"undefined"_s);
        if (to == m_typeResolver->varType())
            return u"QVariant()"_s;
        if (to == m_typeResolver->jsValueType())
            return u"QJSValue();"_s;
        if (to == m_typeResolver->jsPrimitiveType())
            return u"QJSPrimitiveValue()"_s;
        if (from == to)
            return QString();
    }

    if (from == m_typeResolver->nullType()) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
        if (to == jsValueType)
            return u"QJSValue(QJSValue::NullValue)"_s;
        if (to == jsPrimitiveType)
            return u"QJSPrimitiveValue(QJSPrimitiveNull())"_s;
        if (to == varType)
            return u"QVariant::fromValue<std::nullptr_t>(nullptr)"_s;
        const QString zero = zeroBoolOrInt(to);
        if (!zero.isEmpty())
            return zero;
        if (to == m_typeResolver->floatType())
            return u"0.0f"_s;
        if (to == m_typeResolver->realType())
            return u"0.0"_s;
        if (to == m_typeResolver->stringType())
            return QQmlJSUtils::toLiteral(u"null"_s);
        if (from == to)
            return QString();
        REJECT<QString>(u"Conversion from null to %1"_s.arg(to->internalName()));
    }

    if (from == to)
        return variable;

    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            // Compare internalName here. The same C++ type can be exposed muliple times in
            // different QML types. However, the C++ names have to be unique. We can always
            // static_cast to those.

            for (QQmlJSScope::ConstPtr base = from; base; base = base->baseType()) {
                // We still have to cast as other execution paths may result in different types.
                if (base->internalName() == to->internalName())
                    return u"static_cast<"_s + to->internalName() + u" *>("_s + variable + u')';
            }
            for (QQmlJSScope::ConstPtr base = to; base; base = base->baseType()) {
                if (base->internalName() == from->internalName())
                    return u"static_cast<"_s + to->internalName() + u" *>("_s + variable + u')';
            }
        } else if (to == m_typeResolver->boolType()) {
            return u'(' + variable + u" != nullptr)"_s;
        }
    }

    auto isJsValue = [&](const QQmlJSScope::ConstPtr &candidate) {
        return candidate == jsValueType || candidate->isScript();
    };

    if (isJsValue(from) && isJsValue(to))
        return variable;

    const auto isBoolOrNumber = [&](const QQmlJSScope::ConstPtr &type) {
        return m_typeResolver->isNumeric(type)
                || type == m_typeResolver->boolType()
                || type->scopeType() == QQmlSA::ScopeType::EnumScope;
    };

    if (from == m_typeResolver->realType() || from == m_typeResolver->floatType()) {
        if (to == m_typeResolver->int64Type() || to == m_typeResolver->uint64Type()) {
            return to->internalName() + u"(QJSNumberCoercion::roundTowards0("_s
                    + variable + u"))"_s;
        }

        if (m_typeResolver->isSignedInteger(to))
            return u"QJSNumberCoercion::toInteger("_s + variable + u')';
        if (m_typeResolver->isUnsignedInteger(to))
            return u"uint(QJSNumberCoercion::toInteger("_s + variable + u"))"_s;
        if (to == m_typeResolver->boolType())
            return u"[](double moved){ return moved && !std::isnan(moved); }("_s + variable + u')';
    }

    if (isBoolOrNumber(from) && isBoolOrNumber(to))
        return to->internalName() + u'(' + variable + u')';


    if (from == jsPrimitiveType) {
        if (to == m_typeResolver->realType())
            return variable + u".toDouble()"_s;
        if (to == boolType)
            return variable + u".toBoolean()"_s;
        if (to == m_typeResolver->int64Type() || to == m_typeResolver->uint64Type())
            return u"%1(%2.toDouble())"_s.arg(to->internalName(), variable);
        if (m_typeResolver->isIntegral(to))
            return u"%1(%2.toInteger())"_s.arg(to->internalName(), variable);
        if (to == m_typeResolver->stringType())
            return variable + u".toString()"_s;
        if (to == jsValueType)
            return u"QJSValue(QJSPrimitiveValue("_s + variable + u"))"_s;
        if (to == varType)
            return variable + u".toVariant()"_s;
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_s + to->internalName() + u" *>(nullptr)"_s;
    }

    if (isJsValue(from)) {
        if (to == jsPrimitiveType)
            return variable + u".toPrimitive()"_s;
        if (to == varType)
            return variable + u".toVariant(QJSValue::RetainJSObjects)"_s;
        return u"qjsvalue_cast<"_s + castTargetName(to) + u">("_s + variable + u')';
    }

    if (to == jsPrimitiveType) {
        // null and undefined have been handled above already
        Q_ASSERT(from != m_typeResolver->nullType());
        Q_ASSERT(from != m_typeResolver->voidType());

        if (from == m_typeResolver->boolType()
                || from == m_typeResolver->int32Type()
                || from == m_typeResolver->realType()
                || from == m_typeResolver->stringType()) {
            return u"QJSPrimitiveValue("_s + variable + u')';
        } else if (from == m_typeResolver->int16Type()
                   || from == m_typeResolver->int8Type()
                   || from == m_typeResolver->uint16Type()
                   || from == m_typeResolver->uint8Type()) {
            return u"QJSPrimitiveValue(int("_s + variable + u"))"_s;
        } else if (m_typeResolver->isNumeric(from)) {
            return u"QJSPrimitiveValue(double("_s + variable + u"))"_s;
        }
    }

    if (to == jsValueType)
        return u"aotContext->engine->toScriptValue("_s + variable + u')';

    if (from == varType) {
        if (to == m_typeResolver->listPropertyType())
            return u"QQmlListReference("_s + variable + u", aotContext->qmlEngine())"_s;
        return u"aotContext->engine->fromVariant<"_s + castTargetName(to) + u">("_s
                + variable + u')';
    }

    if (to == varType)
        return u"QVariant::fromValue("_s + variable + u')';

    if (from == m_typeResolver->urlType() && to == m_typeResolver->stringType())
        return variable + u".toString()"_s;

    if (from == m_typeResolver->stringType() && to == m_typeResolver->urlType())
        return u"QUrl("_s + variable + u')';

    if (from == m_typeResolver->byteArrayType() && to == m_typeResolver->stringType())
        return u"QString::fromUtf8("_s + variable + u')';

    if (from == m_typeResolver->stringType() && to == m_typeResolver->byteArrayType())
        return variable + u".toUtf8()"_s;

    for (const auto &originType : {
         m_typeResolver->dateTimeType(),
         m_typeResolver->dateType(),
         m_typeResolver->timeType()}) {
        if (from == originType) {
            for (const auto &targetType : {
                 m_typeResolver->dateTimeType(),
                 m_typeResolver->dateType(),
                 m_typeResolver->timeType(),
                 m_typeResolver->stringType(),
                 m_typeResolver->realType()}) {
                if (to == targetType) {
                    return u"aotContext->engine->coerceValue<%1, %2>(%3)"_s.arg(
                                originType->internalName(), targetType->internalName(), variable);
                }
            }
            break;
        }
    }

    const auto retrieveFromPrimitive = [&](
            const QQmlJSScope::ConstPtr &type, const QString &expression) -> QString
    {
        if (type == m_typeResolver->boolType())
            return expression + u".toBoolean()"_s;
        if (m_typeResolver->isSignedInteger(type))
            return expression + u".toInteger()"_s;
        if (m_typeResolver->isUnsignedInteger(type))
            return u"uint("_s + expression + u".toInteger())"_s;
        if (type == m_typeResolver->realType())
            return expression + u".toDouble()"_s;
        if (type == m_typeResolver->floatType())
            return u"float("_s + expression + u".toDouble())"_s;
        if (type == m_typeResolver->stringType())
            return expression + u".toString()"_s;
        return QString();
    };

    if (!retrieveFromPrimitive(from, u"x"_s).isEmpty()) {
        const QString retrieve = retrieveFromPrimitive(
                    to, convertStored(from, m_typeResolver->jsPrimitiveType(), variable));
        if (!retrieve.isEmpty())
            return retrieve;
    }

    if (from->isReferenceType() && to == m_typeResolver->stringType()) {
        return u"aotContext->engine->coerceValue<"_s + castTargetName(from) + u", "
                + castTargetName(to) + u">("_s + variable + u')';
    }

    // Any value type is a non-null JS 'object' and therefore coerces to true.
    if (to == m_typeResolver->boolType()) {
        // All the interesting cases are already handled above:
        Q_ASSERT(from != m_typeResolver->nullType());
        Q_ASSERT(from != m_typeResolver->voidType());
        Q_ASSERT(retrieveFromPrimitive(from, u"x"_s).isEmpty());
        Q_ASSERT(!isBoolOrNumber(from));

        return u"true"_s;
    }

    if (m_typeResolver->areEquivalentLists(from, to))
        return variable;

    if (from->isListProperty()
            && to->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence
            && to->valueType()->isReferenceType()
            && !to->isListProperty()) {
        return variable + u".toList<"_s + to->internalName() + u">()"_s;
    }

    bool isExtension = false;
    if (m_typeResolver->canPopulate(to, from, &isExtension)) {
        REJECT<QString>(
                u"populating "_s + to->internalName() + u" from "_s + from->internalName());
    } else if (const auto ctor = m_typeResolver->selectConstructor(to, from, &isExtension);
               ctor.isValid()) {
        const auto argumentTypes = ctor.parameters();
        return (isExtension ? to->extensionType().scope->internalName() : to->internalName())
                + u"("_s + convertStored(from, argumentTypes[0].type(), variable) + u")"_s;
    }

    if (to == m_typeResolver->stringType()
            && from->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence) {
        addInclude(u"QtQml/qjslist.h"_s);

        // Extend the life time of whatever variable is across the call to toString().
        // variable may be an rvalue.
        return u"[&](auto &&l){ return QJSList(&l, aotContext->engine).toString(); }("_s
                + variable + u')';
    }

    // TODO: add more conversions

    REJECT<QString>(
            u"conversion from "_s + from->internalName() + u" to "_s + to->internalName());
}

QString QQmlJSCodeGenerator::convertContained(QQmlJSRegisterContent from, QQmlJSRegisterContent to, const QString &variable)
{
    const QQmlJSScope::ConstPtr containedFrom = from.containedType();
    const QQmlJSScope::ConstPtr containedTo = to.containedType();

    // Those should be handled before, by convertStored().
    Q_ASSERT(!to.storedType()->isReferenceType());
    Q_ASSERT(!to.isStoredIn(containedTo));
    Q_ASSERT(containedFrom != containedTo);

    if (!to.isStoredIn(m_typeResolver->varType())
            && !to.isStoredIn(m_typeResolver->jsPrimitiveType())) {
        REJECT<QString>(u"internal conversion into unsupported wrapper type."_s);
    }

    bool isExtension = false;
    if (m_typeResolver->canPopulate(containedTo, containedFrom, &isExtension)) {
        REJECT<QString>(u"populating "_s + containedTo->internalName()
                                + u" from "_s + containedFrom->internalName());
    } else if (const auto ctor = m_typeResolver->selectConstructor(
                containedTo, containedFrom, &isExtension); ctor.isValid()) {
        return generateCallConstructor(
                ctor, {from}, {variable}, metaType(containedTo),
                metaObject(isExtension ? containedTo->extensionType().scope : containedTo));
    }

    const auto originalFrom = originalType(from);
    const auto containedOriginalFrom = originalFrom.containedType();
    if (containedFrom != containedOriginalFrom
            && m_typeResolver->canHold(containedFrom, containedOriginalFrom)) {
        // If from is simply a wrapping of a specific type into a more general one, we can convert
        // the original type instead. You can't nest wrappings after all.
        return conversion(m_pool->storedIn(originalFrom, from.storedType()), to, variable);
    }

    if (m_typeResolver->isPrimitive(containedFrom) && m_typeResolver->isPrimitive(containedTo)) {
        const QQmlJSRegisterContent intermediate
                = m_pool->storedIn(from, m_typeResolver->jsPrimitiveType());
        return conversion(intermediate, to, conversion(from, intermediate, variable));
    }

    REJECT<QString>(
            u"internal conversion with incompatible or ambiguous types: %1 -> %2"_s
                    .arg(from.descriptiveName(), to.descriptiveName()));
}

void QQmlJSCodeGenerator::reject(const QString &thing)
{
    addError(u"Cannot generate efficient code for %1"_s.arg(thing));
}


void QQmlJSCodeGenerator::skip(const QString &thing)
{
    addSkip(u"Skipped code generation for %1"_s.arg(thing));
}

QQmlJSCodeGenerator::AccumulatorConverter::AccumulatorConverter(QQmlJSCodeGenerator *generator)
    : accumulatorOut(generator->m_state.accumulatorOut())
    , accumulatorVariableIn(generator->m_state.accumulatorVariableIn)
    , accumulatorVariableOut(generator->m_state.accumulatorVariableOut)
    , generator(generator)
{
    if (accumulatorVariableOut.isEmpty())
        return;

    const QQmlJSTypeResolver *resolver = generator->m_typeResolver;
    const QQmlJSScope::ConstPtr origContained = resolver->originalContainedType(accumulatorOut);
    const QQmlJSRegisterContent storage = accumulatorOut.storage();
    const QQmlJSScope::ConstPtr stored = storage.containedType();
    const QQmlJSScope::ConstPtr origStored = resolver->original(storage).containedType();


    // If the stored type differs or if we store in QVariant and the contained type differs,
    // then we have to use a temporary ...
    if (origStored != stored
            || (origContained != accumulatorOut.containedType() && stored == resolver->varType())) {

        const bool storable = isTypeStorable(resolver, origStored);
        generator->m_state.accumulatorVariableOut = storable ? u"retrieved"_s : QString();
        generator->m_state.setRegister(Accumulator, generator->originalType(accumulatorOut));
        generator->m_body += u"{\n"_s;
        if (storable) {
            generator->m_body += origStored->augmentedInternalName() + u' '
                    + generator->m_state.accumulatorVariableOut + u";\n";
        }
    } else if (generator->m_state.accumulatorVariableIn == generator->m_state.accumulatorVariableOut
               && generator->m_state.readsRegister(Accumulator)
               && generator->m_state.accumulatorOut().isStoredIn(resolver->varType())) {
        // If both m_state.accumulatorIn and m_state.accumulatorOut are QVariant, we will need to
        // prepare the output QVariant, and afterwards use the input variant. Therefore we need to
        // move the input out of the way first.
        generator->m_state.accumulatorVariableIn
                = generator->m_state.accumulatorVariableIn + u"_moved"_s;
        generator->m_body += u"{\n"_s;
        generator->m_body += u"QVariant "_s + generator->m_state.accumulatorVariableIn
                + u" = std::move("_s + generator->m_state.accumulatorVariableOut + u");\n"_s;
    }
}

QQmlJSCodeGenerator::AccumulatorConverter::~AccumulatorConverter()
{
    if (accumulatorVariableOut != generator->m_state.accumulatorVariableOut) {
        generator->m_body += accumulatorVariableOut + u" = "_s + generator->conversion(
                    generator->m_state.accumulatorOut(), accumulatorOut,
                    u"std::move("_s + generator->m_state.accumulatorVariableOut + u')') + u";\n"_s;
        generator->m_body += u"}\n"_s;
        generator->m_state.setRegister(Accumulator, accumulatorOut);
        generator->m_state.accumulatorVariableOut = accumulatorVariableOut;
    } else if (accumulatorVariableIn != generator->m_state.accumulatorVariableIn) {
        generator->m_body += u"}\n"_s;
        generator->m_state.accumulatorVariableIn = accumulatorVariableIn;
    }
}


QT_END_NAMESPACE
