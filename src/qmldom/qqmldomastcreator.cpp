// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomastcreator_p.h"
#include "qqmldomconstants_p.h"
#include "qqmldomelements_p.h"
#include "qqmldomitem_p.h"
#include "qqmldompath_p.h"
#include "qqmldomscriptelements_p.h"
#include "qqmldomtop_p.h"
#include "qqmldomerrormessage_p.h"
#include "qqmldomastdumper_p.h"
#include "qqmldomastcreator_p.h"
#include "qqmldom_utils_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQmlCompiler/private/qqmljsutils_p.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopeGuard>
#include <QtCore/QLoggingCategory>

#include <memory>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

Q_STATIC_LOGGING_CATEGORY(creatorLog, "qt.qmldom.astcreator", QtWarningMsg);

/*
   Avoid crashing on files with JS-elements that are not implemented yet.
   Might be removed (definition + usages) once all script elements are implemented.
*/
#define Q_SCRIPTELEMENT_DISABLE()                                                      \
    do {                                                                               \
        qDebug() << "Could not construct the JS DOM at" << __FILE__ << ":" << __LINE__ \
                 << ", skipping JS elements...";                                       \
        disableScriptElements();                                                       \
    } while (false)

#define Q_SCRIPTELEMENT_EXIT_IF(check)            \
    do {                                          \
        if (m_enableScriptExpressions && (check)) { \
            Q_SCRIPTELEMENT_DISABLE();            \
            return;                               \
        }                                         \
    } while (false)

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace AST;

template<typename K, typename V>
V *valueFromMultimap(QMultiMap<K, V> &mmap, const K &key, index_type idx)
{
    if (idx < 0)
        return nullptr;
    auto it = mmap.find(key);
    auto end = mmap.end();
    if (it == end)
        return nullptr;
    auto it2 = it;
    index_type nEl = 0;
    while (it2 != end && it2.key() == key) {
        ++it2;
        ++nEl;
    }
    if (nEl <= idx)
        return nullptr;
    for (index_type i = idx + 1; i < nEl; ++i)
        ++it;
    return &(*it);
}

static ErrorGroups astParseErrors()
{
    static ErrorGroups errs = { { NewErrorGroup("Dom"), NewErrorGroup("QmlFile"),
                                  NewErrorGroup("Parsing") } };
    return errs;
}

static QString toString(const UiQualifiedId *qualifiedId, QChar delimiter = QLatin1Char('.'))
{
    QString result;

    for (const UiQualifiedId *iter = qualifiedId; iter; iter = iter->next) {
        if (iter != qualifiedId)
            result += delimiter;

        result += iter->name;
    }

    return result;
}

static QString typeToString(AST::Type *t)
{
    Q_ASSERT(t);
    QString res = toString(t->typeId);

    if (UiQualifiedId *arg = t->typeArgument)
        res += u'<' + toString(arg) + u'>';

    return res;
}

SourceLocation combineLocations(SourceLocation s1, SourceLocation s2)
{
    return combine(s1, s2);
}

SourceLocation combineLocations(Node *n)
{
    return combineLocations(n->firstSourceLocation(), n->lastSourceLocation());
}

static ScriptElementVariant wrapIntoFieldMemberExpression(const ScriptElementVariant &left,
                                                          const SourceLocation &dotToken,
                                                          const ScriptElementVariant &right)
{
    SourceLocation s1, s2;
    left.visitConst([&s1](auto &&el) { s1 = el->mainRegionLocation(); });
    right.visitConst([&s2](auto &&el) { s2 = el->mainRegionLocation(); });

    auto result = std::make_shared<ScriptElements::BinaryExpression>(s1, s2);
    result->addLocation(OperatorTokenRegion, dotToken);
    result->setOp(ScriptElements::BinaryExpression::FieldMemberAccess);
    result->setLeft(left);
    result->setRight(right);
    return ScriptElementVariant::fromElement(result);
};

/*!
   \internal
    Creates a FieldMemberExpression if the qualified id has dots.
*/
static ScriptElementVariant
fieldMemberExpressionForQualifiedId(const AST::UiQualifiedId *qualifiedId)
{
    ScriptElementVariant bindable;
    bool first = true;
    for (auto exp = qualifiedId; exp; exp = exp->next) {
        const SourceLocation identifierLoc = exp->identifierToken;
        auto id = std::make_shared<ScriptElements::IdentifierExpression>(identifierLoc);
        id->setName(exp->name);
        if (first) {
            first = false;
            bindable = ScriptElementVariant::fromElement(id);
            continue;
        }
        bindable = wrapIntoFieldMemberExpression(bindable, exp->dotToken,
                                                 ScriptElementVariant::fromElement(id));
    }

    return bindable;
}

QQmlDomAstCreator::QmlStackElement &QQmlDomAstCreator::currentQmlObjectOrComponentEl(int idx)
{
    Q_ASSERT_X(idx < nodeStack.size() && idx >= 0, "currentQmlObjectOrComponentEl",
               "Stack does not contain enough elements!");
    int i = nodeStack.size() - idx;
    while (i-- > 0) {
        DomType k = nodeStack.at(i).item.kind;
        if (k == DomType::QmlObject || k == DomType::QmlComponent)
            return nodeStack[i];
    }
    Q_ASSERT_X(false, "currentQmlObjectEl", "No QmlObject or component in stack");
    return nodeStack.last();
}

QQmlDomAstCreator::QmlStackElement &QQmlDomAstCreator::currentNodeEl(int i)
{
    Q_ASSERT_X(i < nodeStack.size() && i >= 0, "currentNode", "Stack does not contain element!");
    return nodeStack[nodeStack.size() - i - 1];
}

QQmlDomAstCreator::ScriptStackElement &QQmlDomAstCreator::currentScriptNodeEl(int i)
{
    Q_ASSERT_X(i < scriptNodeStack.size() && i >= 0, "currentNode",
               "Stack does not contain element!");
    return scriptNodeStack[scriptNodeStack.size() - i - 1];
}

QQmlDomAstCreator::DomValue &QQmlDomAstCreator::currentNode(int i)
{
    Q_ASSERT_X(i < nodeStack.size() && i >= 0, "currentNode",
               "Stack does not contain element!");
    return nodeStack[nodeStack.size() - i - 1].item;
}

void QQmlDomAstCreator::removeCurrentNode(std::optional<DomType> expectedType)
{
    Q_ASSERT_X(!nodeStack.isEmpty(), className, "popCurrentNode() without any node");
    if (expectedType)
        Q_ASSERT(nodeStack.last().item.kind == *expectedType);
    nodeStack.removeLast();
}

void QQmlDomAstCreator::removeCurrentScriptNode(std::optional<DomType> expectedType)
{
    Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
    Q_ASSERT_X(!scriptNodeStack.isEmpty(), className,
               "popCurrentScriptNode() without any node");
    if (expectedType)
        Q_ASSERT(scriptNodeStack.last().kind == *expectedType);
    scriptNodeStack.removeLast();
}

/*!
   \internal
   Prepares a script element DOM representation such that it can be used inside a QML DOM element.
   This recursively sets the pathFromOwner and creates the FileLocations::Tree for all children of
   element.

   Beware that pathFromOwner is appended to ownerFileLocations when creating the FileLocations!

   Make sure to add, for each of its use, a test in tst_qmldomitem:finalizeScriptExpressions, as
   using a wrong pathFromOwner and/or a wrong base might lead to bugs hard to debug and spurious
   crashes.
 */
const ScriptElementVariant &
QQmlDomAstCreator::finalizeScriptExpression(const ScriptElementVariant &element, const Path &pathFromOwner,
                                            const FileLocations::Tree &ownerFileLocations)
{
    auto e = element.base();
    Q_ASSERT(e);

    qCDebug(creatorLog) << "Finalizing script expression with path:"
                        << FileLocations::canonicalPathForTesting(ownerFileLocations)
                                   .append(pathFromOwner.toString());
    e->updatePathFromOwner(pathFromOwner);
    e->createFileLocations(ownerFileLocations);
    return element;
}

FileLocations::Tree QQmlDomAstCreator::createMap(const FileLocations::Tree &base, const Path &p, AST::Node *n)
{
    FileLocations::Tree res = FileLocations::ensure(base, p);
    if (n)
        FileLocations::addRegion(res, MainRegion, combineLocations(n));
    return res;
}

FileLocations::Tree QQmlDomAstCreator::createMap(DomType k, const Path &p, AST::Node *n)
{
    Path relative;
    FileLocations::Tree base;
    switch (k) {
    case DomType::QmlObject:
        switch (currentNode().kind) {
        case DomType::QmlObject:
        case DomType::QmlComponent:
        case DomType::PropertyDefinition:
        case DomType::Binding:
        case DomType::Id:
        case DomType::MethodInfo:
            break;
        default:
            qCWarning(domLog) << "unexpected type" << domTypeToString(currentNode().kind);
            Q_UNREACHABLE();
        }
        base = currentNodeEl().fileLocations;
        if (p.length() > 2) {
            Path p2 = p[p.length() - 2];
            if (p2.headKind() == Path::Kind::Field
                && (p2.checkHeadName(Fields::children) || p2.checkHeadName(Fields::objects)
                    || p2.checkHeadName(Fields::value) || p2.checkHeadName(Fields::annotations)
                    || p2.checkHeadName(Fields::children)))
                relative = p.mid(p.length() - 2, 2);
            else if (p.last().checkHeadName(Fields::value)
                     && p.last().headKind() == Path::Kind::Field)
                relative = p.last();
            else {
                qCWarning(domLog) << "unexpected path to QmlObject in createMap" << p;
                Q_UNREACHABLE();
            }
        } else {
            qCWarning(domLog) << "unexpected path to QmlObject in createMap" << p;
            Q_UNREACHABLE();
        }
        break;
    case DomType::EnumItem:
        relative = p;
        base = currentNodeEl().fileLocations;
        break;
    case DomType::QmlComponent:
    case DomType::Pragma:
    case DomType::Import:
    case DomType::Id:
    case DomType::EnumDecl:
        relative = p;
        base = rootMap;
        break;
    case DomType::Binding:
    case DomType::PropertyDefinition:
    case DomType::MethodInfo:
        base = currentEl<QmlObject>().fileLocations;
        if (p.length() > 3)
            relative = p.mid(p.length() - 3, 3);
        else
            relative = p;
        break;

    default:
        qCWarning(domLog) << "Unexpected type in createMap:" << domTypeToString(k);
        Q_UNREACHABLE();
        break;
    }
    return createMap(base, relative, n);
}

QQmlDomAstCreator::QQmlDomAstCreator(const MutableDomItem &qmlFile)
    : qmlFile(qmlFile),
      qmlFilePtr(qmlFile.ownerAs<QmlFile>()),
      rootMap(qmlFilePtr->fileLocationsTree())
{
}

bool QQmlDomAstCreator::visit(UiProgram *program)
{
    QFileInfo fInfo(qmlFile.canonicalFilePath());
    QString componentName = fInfo.baseName();
    QmlComponent *cPtr;
    Path p = qmlFilePtr->addComponent(QmlComponent(componentName), AddOption::KeepExisting,
                                      &cPtr);
    MutableDomItem newC(qmlFile.item(), p);
    Q_ASSERT_X(newC.item(), className, "could not recover component added with addComponent");
    // QmlFile region == Component region == program span
    // we hide the component span because the component s written after the imports
    FileLocations::addRegion(rootMap, MainRegion, combineLocations(program));
    pushEl(p, *cPtr, program);

    auto envPtr = qmlFile.environment().ownerAs<DomEnvironment>();
    const bool loadDependencies =
            !envPtr->options().testFlag(DomEnvironment::Option::NoDependencies);
    // add implicit directory import and load them in the Dom
    if (!fInfo.canonicalPath().isEmpty()) {
        Import selfDirImport(QmlUri::fromDirectoryString(fInfo.canonicalPath()));
        selfDirImport.implicit = true;
        qmlFilePtr->addImport(selfDirImport);

        if (loadDependencies) {
            const QString currentFile = envPtr->domCreationOption() == Extended
                    ? QQmlJSUtils::qmlBuildPathFromSourcePath(
                              envPtr->semanticAnalysis().m_mapper.get(),
                              qmlFile.canonicalFilePath())
                    : qmlFile.canonicalFilePath();

            const QDir implicitImportDir = QFileInfo(currentFile).dir();
            const QString implicitImportDirPath = implicitImportDir.canonicalPath();
            envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, implicitImportDirPath),
                             DomItem::Callback(), DomType::QmlDirectory);

            // also load the qmldir from the implicit directory, if existing
            if (implicitImportDir.exists(u"qmldir"_s)) {
                const QString implicitImportQmldir = implicitImportDirPath + u"/qmldir"_s;
                envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, implicitImportQmldir),
                                 DomItem::Callback(), DomType::QmldirFile);
            }
        }
    }
    // add implicit imports from the environment (QML, QtQml for example) and load them in the Dom
    for (Import i : qmlFile.environment().ownerAs<DomEnvironment>()->implicitImports()) {
        i.implicit = true;
        qmlFilePtr->addImport(i);

        if (loadDependencies)
            envPtr->loadModuleDependency(i.uri.moduleUri(), i.version, DomItem::Callback());
    }
    if (m_loadFileLazily && loadDependencies) {
        envPtr->loadPendingDependencies();
        envPtr->commitToBase(qmlFile.environment().item());
    }

    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiProgram *)
{
    MutableDomItem newC = qmlFile.path(currentNodeEl().path);
    QmlComponent &comp = current<QmlComponent>();
    for (const Pragma &p : qmlFilePtr->pragmas()) {
        if (p.name.compare(u"singleton", Qt::CaseInsensitive) == 0) {
            comp.setIsSingleton(true);
            comp.setIsCreatable(false); // correct?
        }
    }
    *newC.mutableAs<QmlComponent>() = comp;
    removeCurrentNode(DomType::QmlComponent);
    Q_ASSERT_X(nodeStack.isEmpty(), className, "ui program did not finish node stack");
}

bool QQmlDomAstCreator::visit(UiPragma *el)
{
    QStringList valueList;
    for (auto t = el->values; t; t = t->next)
        valueList << t->value.toString();

    auto fileLocation = createMap(
            DomType::Pragma, qmlFilePtr->addPragma(Pragma(el->name.toString(), valueList)), el);
    FileLocations::addRegion(fileLocation, PragmaKeywordRegion, el->pragmaToken);
    FileLocations::addRegion(fileLocation, IdentifierRegion, el->pragmaIdToken);
    if (el->colonToken.isValid()) {
        FileLocations::addRegion(fileLocation, ColonTokenRegion, el->colonToken);
    }
    int i = 0;
    for (auto t = el->values; t; t = t->next) {
        auto subMap = createMap(fileLocation, Path().withField(Fields::values).withIndex(i), t);
        FileLocations::addRegion(subMap, PragmaValuesRegion, t->location);
        ++i;
    }

    return true;
}

bool QQmlDomAstCreator::visit(UiImport *el)
{
    Version v(Version::Latest, Version::Latest);
    if (el->version && el->version->version.hasMajorVersion())
        v.majorVersion = el->version->version.majorVersion();
    if (el->version && el->version->version.hasMinorVersion())
        v.minorVersion = el->version->version.minorVersion();

    auto envPtr = qmlFile.environment().ownerAs<DomEnvironment>();
    const bool loadDependencies =
            !envPtr->options().testFlag(DomEnvironment::Option::NoDependencies);
    FileLocations::Tree fileLocation;
    if (el->importUri != nullptr) {
        const Import import =
                Import::fromUriString(toString(el->importUri), v, el->importId.toString());
        fileLocation = createMap(DomType::Import, qmlFilePtr->addImport(import), el);

        if (loadDependencies) {
            envPtr->loadModuleDependency(import.uri.moduleUri(), import.version,
                                         DomItem::Callback());
        }
        FileLocations::addRegion(fileLocation, ImportUriRegion, combineLocations(el->importUri));
    } else {
        const Import import =
                Import::fromFileString(el->fileName.toString(), el->importId.toString());
        fileLocation = createMap(DomType::Import, qmlFilePtr->addImport(import), el);

        if (loadDependencies) {
            const QString currentFileDir =
                    QFileInfo(qmlFile.canonicalFilePath()).dir().canonicalPath();
            envPtr->loadFile(FileToLoad::fromFileSystem(
                                     envPtr, import.uri.absoluteLocalPath(currentFileDir)),
                             DomItem::Callback(), DomType::QmlDirectory);
        }
        FileLocations::addRegion(fileLocation, ImportUriRegion, el->fileNameToken);
    }
    if (m_loadFileLazily && loadDependencies) {
        envPtr->loadPendingDependencies();
        envPtr->commitToBase(qmlFile.environment().item());
    }

    if (el->importToken.isValid())
        FileLocations::addRegion(fileLocation, ImportTokenRegion, el->importToken);

    if (el->asToken.isValid())
        FileLocations::addRegion(fileLocation, AsTokenRegion, el->asToken);

    if (el->importIdToken.isValid())
        FileLocations::addRegion(fileLocation, IdNameRegion, el->importIdToken);

    if (el->version)
        FileLocations::addRegion(fileLocation, VersionRegion, combineLocations(el->version));


    return true;
}

bool QQmlDomAstCreator::visit(AST::UiPublicMember *el)
{
    switch (el->type) {
    case AST::UiPublicMember::Signal: {
        MethodInfo m;
        m.name = el->name.toString();
        m.typeName = toString(el->memberType);
        m.isReadonly = el->isReadonly();
        m.access = MethodInfo::Public;
        m.methodType = MethodInfo::Signal;
        m.isList = el->typeModifier == QLatin1String("list");
        MethodInfo *mPtr;
        Path p = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
        pushEl(p, *mPtr, el);

        const auto fileLocations = nodeStack.last().fileLocations;
        FileLocations::addRegion(fileLocations, SignalKeywordRegion, el->propertyToken());
        FileLocations::addRegion(fileLocations, IdentifierRegion, el->identifierToken);
        if (el->lparenToken.isValid())
            FileLocations::addRegion(fileLocations, LeftParenthesisRegion, el->lparenToken);
        if (el->rparenToken.isValid())
            FileLocations::addRegion(fileLocations, RightParenthesisRegion, el->rparenToken);

        MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
        AST::UiParameterList *args = el->parameters;
        while (args) {
            MethodParameter param;
            param.name = args->name.toString();
            param.typeName = args->type ? args->type->toString() : QString();
            index_type idx = index_type(mInfo.parameters.size());
            if (!args->colonToken.isValid())
                param.typeAnnotationStyle = MethodParameter::TypeAnnotationStyle::Prefix;
            mInfo.parameters.append(param);
            auto argLocs = FileLocations::ensure(nodeStack.last().fileLocations,
                                                 Path::fromField(Fields::parameters).withIndex(idx));
            FileLocations::addRegion(argLocs, MainRegion, combineLocations(args));
            FileLocations::addRegion(argLocs, IdentifierRegion, args->identifierToken);
            if (args->type)
                FileLocations::addRegion(argLocs, TypeIdentifierRegion, args->propertyTypeToken);
            args = args->next;
        }
        break;
    }
    case AST::UiPublicMember::Property: {
        PropertyDefinition p;
        p.name = el->name.toString();
        p.typeName = toString(el->memberType);
        p.isReadonly = el->isReadonly();
        p.isDefaultMember = el->isDefaultMember();
        p.isRequired = el->isRequired();
        p.isFinal = el->isFinal();
        p.isList = el->typeModifier == QLatin1String("list");
        p.isFinal = el->isFinal();
        if (!el->typeModifier.isEmpty())
            p.typeName = el->typeModifier.toString() + QChar(u'<') + p.typeName + QChar(u'>');
        PropertyDefinition *pPtr;
        Path pPathFromOwner =
                current<QmlObject>().addPropertyDef(p, AddOption::KeepExisting, &pPtr);
        if (m_enableScriptExpressions) {
            auto qmlObjectType = makeGenericScriptElement(el->memberType, DomType::ScriptType);
            qmlObjectType->insertChild(Fields::typeName,
                                       fieldMemberExpressionForQualifiedId(el->memberType));
            pPtr->setNameIdentifiers(finalizeScriptExpression(
                    ScriptElementVariant::fromElement(qmlObjectType),
                    pPathFromOwner.withField(Fields::nameIdentifiers), rootMap));
            // skip binding identifiers of the binding inside the property definition, if there is
            // one
            m_skipBindingIdentifiers = el->binding;
        }
        pushEl(pPathFromOwner, *pPtr, el);
        FileLocations::addRegion(nodeStack.last().fileLocations, PropertyKeywordRegion,
                                 el->propertyToken());
        FileLocations::addRegion(nodeStack.last().fileLocations, IdentifierRegion,
                                 el->identifierToken);
        FileLocations::addRegion(nodeStack.last().fileLocations, TypeIdentifierRegion,
                                 el->typeToken);
        FileLocations::addRegion(nodeStack.last().fileLocations, ColonTokenRegion, el->colonToken);
        if (el->typeModifierToken.isValid())
            FileLocations::addRegion(nodeStack.last().fileLocations, TypeModifierRegion, el->typeModifierToken);
        if (p.name == u"id")
            qmlFile.addError(std::move(astParseErrors()
                                     .warning(tr("id is a special attribute, that should not be "
                                                 "used as property name"))
                                               .withPath(currentNodeEl().path)));
        if (p.isDefaultMember) {
            FileLocations::addRegion(nodeStack.last().fileLocations, DefaultKeywordRegion,
                                     el->defaultToken());
        }
        if (p.isFinal) {
            FileLocations::addRegion(nodeStack.last().fileLocations, FinalKeywordRegion,
                                     el->finalToken());
        }
        if (p.isRequired) {
            FileLocations::addRegion(nodeStack.last().fileLocations, RequiredKeywordRegion,
                                     el->requiredToken());
        }
        if (p.isReadonly) {
            FileLocations::addRegion(nodeStack.last().fileLocations, ReadonlyKeywordRegion,
                                     el->readonlyToken());
        }
        if (el->statement) {
            BindingType bType = BindingType::Normal;
            SourceLocation loc = combineLocations(el->statement);
            QStringView code = qmlFilePtr->code();

            auto script = std::make_shared<ScriptExpression>(
                    code.mid(loc.offset, loc.length), qmlFilePtr->engine(), el->statement,
                    qmlFilePtr->astComments(), ScriptExpression::ExpressionType::BindingExpression,
                    loc);
            Binding *bPtr;
            Path bPathFromOwner = current<QmlObject>().addBinding(Binding(p.name, script, bType),
                                                                  AddOption::KeepExisting, &bPtr);
            FileLocations::Tree bLoc = createMap(DomType::Binding, bPathFromOwner, el->statement);
            FileLocations::addRegion(bLoc, ColonTokenRegion, el->colonToken);
            FileLocations::Tree valueLoc = FileLocations::ensure(bLoc, Path::fromField(Fields::value));
            FileLocations::addRegion(valueLoc, MainRegion, combineLocations(el->statement));
            // push it also: its needed in endVisit to add the scriptNode to it
            // do not use pushEl to avoid recreating the already created "bLoc" Map
            nodeStack.append({ bPathFromOwner, *bPtr, bLoc });
        }
        break;
    }
    }
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiPublicMember *el)
{
    if (auto &lastEl = currentNode(); lastEl.kind == DomType::Binding) {
        Binding &b = std::get<Binding>(lastEl.value);
        if (m_enableScriptExpressions
            && (scriptNodeStack.size() != 1 || scriptNodeStack.last().isList())) {
            Q_SCRIPTELEMENT_DISABLE();
        }
        if (m_enableScriptExpressions) {
            b.scriptExpressionValue()->setScriptElement(finalizeScriptExpression(
                    currentScriptNodeEl().takeVariant(), Path().withField(Fields::scriptElement),
                    FileLocations::ensure(currentNodeEl().fileLocations,
                                          Path().withField(Fields::value))));
            removeCurrentScriptNode({});
        }

        QmlObject &containingObject = current<QmlObject>();
        Binding *bPtr =
                valueFromMultimap(containingObject.m_bindings, b.name(), currentIndex());
        Q_ASSERT(bPtr);
        removeCurrentNode({});
    }
    Node::accept(el->parameters, this);
    loadAnnotations(el);
    if ((el->binding || el->statement)
        && nodeStack.last().item.kind == DomType::PropertyDefinition) {
        PropertyDefinition &pDef = std::get<PropertyDefinition>(nodeStack.last().item.value);
        if (!pDef.annotations.isEmpty()) {
            QmlObject duplicate;
            duplicate.setName(QLatin1String("duplicate"));
            QmlObject &obj = current<QmlObject>();
            auto it = obj.m_bindings.find(pDef.name);
            if (it != obj.m_bindings.end()) {
                for (QmlObject ann : pDef.annotations) {
                    ann.addAnnotation(duplicate);
                    it->addAnnotation(currentEl<QmlObject>()
                                              .path.withField(Fields::bindings)
                                              .withKey(pDef.name)
                                              .withIndex(obj.m_bindings.values(pDef.name).size() - 1),
                                      ann);
                }
            }
        }
    }
    QmlObject &obj = current<QmlObject>();
    QmlStackElement &sEl = nodeStack.last();
    switch (sEl.item.kind) {
    case DomType::PropertyDefinition: {
        PropertyDefinition pDef = std::get<PropertyDefinition>(sEl.item.value);
        PropertyDefinition *pDefPtr =
                valueFromMultimap(obj.m_propertyDefs, pDef.name, sEl.path.last().headIndex());
        Q_ASSERT(pDefPtr);
        *pDefPtr = pDef;
    } break;
    case DomType::MethodInfo: {
        MethodInfo m = std::get<MethodInfo>(sEl.item.value);
        MethodInfo *mPtr = valueFromMultimap(obj.m_methods, m.name, sEl.path.last().headIndex());
        Q_ASSERT(mPtr);
        *mPtr = m;
    } break;
    default:
        Q_UNREACHABLE();
    }
    removeCurrentNode({});
}

void QQmlDomAstCreator::endVisit(AST::FormalParameterList *list)
{
    endVisitForLists(list);
}

bool QQmlDomAstCreator::visit(AST::FunctionExpression *)
{
    ++m_nestedFunctionDepth;
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

ScriptElementVariant QQmlDomAstCreator::prepareBodyForFunction(AST::FunctionExpression *fExpression)
{
    Q_ASSERT(!scriptNodeStack.isEmpty() || !fExpression->body);

    if (fExpression->body) {
        if (currentScriptNodeEl().isList()) {
            // It is more intuitive to have functions with a block as a body instead of a
            // list.
            auto body = std::make_shared<ScriptElements::BlockStatement>(
                    combineLocations(fExpression->lbraceToken, fExpression->rbraceToken));
            body->setStatements(currentScriptNodeEl().takeList());
            if (auto semanticScope = body->statements().semanticScope())
                body->setSemanticScope(semanticScope);
            auto result = ScriptElementVariant::fromElement(body);
            removeCurrentScriptNode({});
            return result;
        } else {
            auto result = currentScriptNodeEl().takeVariant();
            removeCurrentScriptNode({});
            return result;
        }
        Q_UNREACHABLE_RETURN({});
    }

    // for convenience purposes: insert an empty BlockStatement
    auto body = std::make_shared<ScriptElements::BlockStatement>(
            combineLocations(fExpression->lbraceToken, fExpression->rbraceToken));
    return ScriptElementVariant::fromElement(body);
}

void QQmlDomAstCreator::endVisit(AST::FunctionExpression *fExpression)
{
    --m_nestedFunctionDepth;
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(fExpression, DomType::ScriptFunctionExpression);
    if (fExpression->identifierToken.isValid())
        current->addLocation(IdentifierRegion, fExpression->identifierToken);
    if (fExpression->functionToken.isValid())
        current->addLocation(FunctionKeywordRegion, fExpression->functionToken);
    if (fExpression->starToken.isValid())
        current->addLocation(StarTokenRegion, fExpression->starToken);
    if (fExpression->lparenToken.isValid())
        current->addLocation(LeftParenthesisRegion, fExpression->lparenToken);
    if (fExpression->rparenToken.isValid())
        current->addLocation(RightParenthesisRegion, fExpression->rparenToken);
    if (fExpression->lbraceToken.isValid())
        current->addLocation(LeftBraceRegion, fExpression->lbraceToken);
    if (fExpression->rbraceToken.isValid())
        current->addLocation(RightBraceRegion, fExpression->rbraceToken);
    if (fExpression->typeAnnotation) {
        current->addLocation(TypeIdentifierRegion,
                             combineLocations(fExpression->typeAnnotation->type));
    }

    Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty() && fExpression->body);
    current->insertChild(Fields::body, prepareBodyForFunction(fExpression));

    if (fExpression->typeAnnotation) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::returnType, currentScriptNodeEl().takeVariant());
        scriptNodeStack.removeLast();
    }
    if (fExpression->formals) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::parameters, currentScriptNodeEl().takeList());
        scriptNodeStack.removeLast();
    }

    if (!fExpression->name.isEmpty())
        current->insertValue(Fields::name, fExpression->name);

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::FunctionDeclaration *fDef)
{
    // Treat nested functions as (named) lambdas instead of Qml Object methods.
    if (m_nestedFunctionDepth > 0) {
        return visit(static_cast<FunctionExpression *>(fDef));
    }
    ++m_nestedFunctionDepth;
    const QStringView code(qmlFilePtr->code());
    MethodInfo m;
    m.name = fDef->name.toString();
    if (AST::TypeAnnotation *tAnn = fDef->typeAnnotation) {
        if (AST::Type *t = tAnn->type)
            m.typeName = typeToString(t);
    }
    m.access = MethodInfo::Public;
    m.methodType = MethodInfo::Method;

    SourceLocation bodyLoc = fDef->body ? combineLocations(fDef->body)
                                        : combineLocations(fDef->lbraceToken, fDef->rbraceToken);
    SourceLocation methodLoc = combineLocations(fDef);
    QStringView preCode = code.mid(methodLoc.begin(), bodyLoc.begin() - methodLoc.begin());
    QStringView postCode = code.mid(bodyLoc.end(), methodLoc.end() - bodyLoc.end());
    m.body = std::make_shared<ScriptExpression>(
            code.mid(bodyLoc.offset, bodyLoc.length), qmlFilePtr->engine(), fDef->body,
            qmlFilePtr->astComments(), ScriptExpression::ExpressionType::FunctionBody, bodyLoc, 0,
            preCode, postCode);

    if (fDef->typeAnnotation) {
        SourceLocation typeLoc = combineLocations(fDef->typeAnnotation);
        m.returnType = std::make_shared<ScriptExpression>(
                code.mid(typeLoc.offset, typeLoc.length), qmlFilePtr->engine(),
                fDef->typeAnnotation, qmlFilePtr->astComments(),
                ScriptExpression::ExpressionType::ReturnType, typeLoc, 0, u"", u"");
    }

    MethodInfo *mPtr;
    Path mPathFromOwner = current<QmlObject>().addMethod(m, AddOption::KeepExisting, &mPtr);
    pushEl(mPathFromOwner, *mPtr,
           fDef); // add at the start and use the normal recursive visit?
    FileLocations::Tree &fLoc = nodeStack.last().fileLocations;
    if (fDef->identifierToken.isValid())
        FileLocations::addRegion(fLoc, IdentifierRegion, fDef->identifierToken);
    auto bodyTree = FileLocations::ensure(fLoc, Path::fromField(Fields::body));
    FileLocations::addRegion(bodyTree, MainRegion, bodyLoc);
    if (fDef->functionToken.isValid())
        FileLocations::addRegion(fLoc, FunctionKeywordRegion, fDef->functionToken);
    if (fDef->starToken.isValid())
        FileLocations::addRegion(fLoc, StarTokenRegion, fDef->starToken);
    if (fDef->lparenToken.length != 0)
        FileLocations::addRegion(fLoc, LeftParenthesisRegion, fDef->lparenToken);
    if (fDef->rparenToken.length != 0)
        FileLocations::addRegion(fLoc, RightParenthesisRegion, fDef->rparenToken);
    if (fDef->lbraceToken.length != 0)
        FileLocations::addRegion(fLoc, LeftBraceRegion, fDef->lbraceToken);
    if (fDef->rbraceToken.length != 0)
        FileLocations::addRegion(fLoc, RightBraceRegion, fDef->rbraceToken);
    if (fDef->typeAnnotation)
        FileLocations::addRegion(fLoc, TypeIdentifierRegion, combineLocations(fDef->typeAnnotation->type));
    MethodInfo &mInfo = std::get<MethodInfo>(currentNode().value);
    AST::FormalParameterList *args = fDef->formals;
    while (args) {
        MethodParameter param;
        param.name = args->element->bindingIdentifier.toString();
        if (AST::TypeAnnotation *tAnn = args->element->typeAnnotation) {
            if (AST::Type *t = tAnn->type)
                param.typeName = typeToString(t);
        }
        if (args->element->initializer) {
            SourceLocation loc = combineLocations(args->element->initializer);
            auto script = std::make_shared<ScriptExpression>(
                    code.mid(loc.offset, loc.length), qmlFilePtr->engine(),
                    args->element->initializer, qmlFilePtr->astComments(),
                    ScriptExpression::ExpressionType::ArgInitializer, loc);
            param.defaultValue = script;
        }
        if (args->element->type == AST::PatternElement::SpreadElement)
            param.isRestElement = true;
        SourceLocation parameterLoc = combineLocations(args->element);
        param.value = std::make_shared<ScriptExpression>(
                code.mid(parameterLoc.offset, parameterLoc.length), qmlFilePtr->engine(),
                args->element, qmlFilePtr->astComments(),
                ScriptExpression::ExpressionType::ArgumentStructure, parameterLoc);

        index_type idx = index_type(mInfo.parameters.size());
        mInfo.parameters.append(param);
        auto argLocs = FileLocations::ensure(nodeStack.last().fileLocations,
                                             Path::fromField(Fields::parameters).withIndex(idx));
        FileLocations::addRegion(argLocs, MainRegion, combineLocations(args));
        if (args->element->identifierToken.isValid())
            FileLocations::addRegion(argLocs, IdentifierRegion, args->element->identifierToken);
        if (args->element->typeAnnotation)
            FileLocations::addRegion(argLocs, TypeIdentifierRegion, combineLocations(args->element->typeAnnotation->type));
        args = args->next;
    }
    return true;
}

bool QQmlDomAstCreator::visit(AST::UiSourceElement *el)
{
    if (!cast<FunctionDeclaration *>(el->sourceElement)) {
        qCWarning(creatorLog) << "unhandled source el:" << static_cast<AST::Node *>(el);
        Q_UNREACHABLE();
    }
    return true;
}

static void setFormalParameterKind(ScriptElementVariant &variant)
{
    if (auto data = variant.data()) {
        if (auto genericElement =
                    std::get_if<std::shared_ptr<ScriptElements::GenericScriptElement>>(&*data)) {
            (*genericElement)->setKind(DomType::ScriptFormalParameter);
        }
    }
}

void QQmlDomAstCreator::endVisit(AST::FunctionDeclaration *fDef)
{
    // Treat nested functions as (named) lambdas instead of Qml Object methods.
    if (m_nestedFunctionDepth > 1) {
        endVisit(static_cast<FunctionExpression *>(fDef));
        return;
    }
    --m_nestedFunctionDepth;
    MethodInfo &m = std::get<MethodInfo>(currentNode().value);
    const FileLocations::Tree bodyTree =
            FileLocations::ensure(currentNodeEl().fileLocations, Path().withField(Fields::body));
    const Path bodyPath = Path().withField(Fields::scriptElement);

    if (!m_enableScriptExpressions)
        return;

    Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty() && fDef->body);
    m.body->setScriptElement(
            finalizeScriptExpression(prepareBodyForFunction(fDef), bodyPath, bodyTree));

    if (fDef->typeAnnotation) {
        auto argLoc = FileLocations::ensure(nodeStack.last().fileLocations,
                                            Path().withField(Fields::returnType));
        const Path pathToReturnType = Path().withField(Fields::scriptElement);

        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        ScriptElementVariant variant = currentScriptNodeEl().takeVariant();
        finalizeScriptExpression(variant, pathToReturnType, argLoc);
        m.returnType->setScriptElement(variant);
        removeCurrentScriptNode({});
    }
    if (fDef->formals) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        const auto parameterList = scriptNodeStack.takeLast().takeList();
        const auto &parameterQList = parameterList.qList();
        size_t size = (size_t)parameterQList.size();
        for (size_t idx = size - 1; idx < size; --idx) {
            auto argLoc = FileLocations::ensure(
                    nodeStack.last().fileLocations,
                    Path().withField(Fields::parameters).withIndex(idx).withField(Fields::value));
            const Path pathToArgument = Path().withField(Fields::scriptElement);

            ScriptElementVariant variant = parameterQList[idx];
            setFormalParameterKind(variant);
            finalizeScriptExpression(variant, pathToArgument, argLoc);
            m.parameters[idx].value->setScriptElement(variant);
        }
    }

    // there should be no more uncollected script elements
    if (m_enableScriptExpressions && !scriptNodeStack.empty()) {
        Q_SCRIPTELEMENT_DISABLE();
    }
}

void QQmlDomAstCreator::endVisit(AST::UiSourceElement *el)
{
    MethodInfo &m = std::get<MethodInfo>(currentNode().value);
    loadAnnotations(el);
    QmlObject &obj = current<QmlObject>();
    MethodInfo *mPtr =
            valueFromMultimap(obj.m_methods, m.name, nodeStack.last().path.last().headIndex());
    Q_ASSERT(mPtr);
    *mPtr = m;
    removeCurrentNode(DomType::MethodInfo);
}

bool QQmlDomAstCreator::visit(AST::UiObjectDefinition *el)
{
    QmlObject scope;
    scope.setName(toString(el->qualifiedTypeNameId));
    scope.addPrototypePath(Paths::lookupTypePath(scope.name()));
    QmlObject *sPtr = nullptr;
    Path sPathFromOwner;
    if (!arrayBindingLevels.isEmpty() && nodeStack.size() == arrayBindingLevels.last()) {
        if (currentNode().kind == DomType::Binding) {
            QList<QmlObject> *vals = std::get<Binding>(currentNode().value).arrayValue();
            if (vals) {
                int idx = vals->size();
                vals->append(scope);
                sPathFromOwner = currentNodeEl().path.withField(Fields::value).withIndex(idx);
                sPtr = &((*vals)[idx]);
                sPtr->updatePathFromOwner(sPathFromOwner);
            } else {
                Q_ASSERT_X(false, className,
                           "expected an array binding with a valid QList<QmlScope> as value");
            }
        } else {
            Q_ASSERT_X(false, className, "expected an array binding as last node on the stack");
        }
    } else {
        DomValue &containingObject = currentQmlObjectOrComponentEl().item;
        switch (containingObject.kind) {
        case DomType::QmlComponent:
            sPathFromOwner = std::get<QmlComponent>(containingObject.value).addObject(scope, &sPtr);
            break;
        case DomType::QmlObject:
            sPathFromOwner = std::get<QmlObject>(containingObject.value).addChild(scope, &sPtr);
            break;
        default:
            Q_UNREACHABLE();
        }
        Path pathFromContainingObject = sPathFromOwner.mid(currentNodeEl().path.length());
        FileLocations::Tree fLoc =
                FileLocations::ensure(currentNodeEl().fileLocations, pathFromContainingObject);
        FileLocations::addRegion(fLoc, IdentifierRegion,
                                 el->qualifiedTypeNameId->identifierToken);
    }
    Q_ASSERT_X(sPtr, className, "could not recover new scope");

    if (m_enableScriptExpressions) {
        auto qmlObjectType = makeGenericScriptElement(el->qualifiedTypeNameId, DomType::ScriptType);
        qmlObjectType->insertChild(Fields::typeName,
                                   fieldMemberExpressionForQualifiedId(el->qualifiedTypeNameId));
        sPtr->setNameIdentifiers(
                finalizeScriptExpression(ScriptElementVariant::fromElement(qmlObjectType),
                                         sPathFromOwner.withField(Fields::nameIdentifiers), rootMap));
    }
    pushEl(sPathFromOwner, *sPtr, el);

    if (m_enableScriptExpressions && el->initializer) {
        FileLocations::addRegion(nodeStack.last().fileLocations, LeftBraceRegion,
                                 el->initializer->lbraceToken);
        FileLocations::addRegion(nodeStack.last().fileLocations, RightBraceRegion,
                                 el->initializer->rbraceToken);
    }
    loadAnnotations(el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiObjectDefinition *)
{
    QmlObject &obj = current<QmlObject>();
    int idx = currentIndex();
    if (!arrayBindingLevels.isEmpty() && nodeStack.size() == arrayBindingLevels.last() + 1) {
        if (currentNode(1).kind == DomType::Binding) {
            Binding &b = std::get<Binding>(currentNode(1).value);
            QList<QmlObject> *vals = b.arrayValue();
            Q_ASSERT_X(vals, className,
                       "expected an array binding with a valid QList<QmlScope> as value");
            (*vals)[idx] = obj;
        } else {
            Q_ASSERT_X(false, className, "expected an array binding as last node on the stack");
        }
    } else {
        DomValue &containingObject = currentNodeEl(1).item;
        Path p = currentNodeEl().path;
        switch (containingObject.kind) {
        case DomType::QmlComponent:
            if (p[p.length() - 2] == Path::fromField(Fields::objects))
                std::get<QmlComponent>(containingObject.value).m_objects[idx] = obj;
            else
                Q_UNREACHABLE();
            break;
        case DomType::QmlObject:
            if (p[p.length() - 2] == Path::fromField(Fields::children))
                std::get<QmlObject>(containingObject.value).m_children[idx] = obj;
            else
                Q_UNREACHABLE();
            break;
        default:
            Q_UNREACHABLE();
        }
    }
    removeCurrentNode(DomType::QmlObject);
}

void QQmlDomAstCreator::setBindingIdentifiers(const Path &pathFromOwner,
                                              const UiQualifiedId *identifiers, Binding *bindingPtr)
{
    const bool skipBindingIdentifiers = std::exchange(m_skipBindingIdentifiers, false);
    if (!m_enableScriptExpressions || skipBindingIdentifiers)
        return;

    ScriptElementVariant bindable = fieldMemberExpressionForQualifiedId(identifiers);
    bindingPtr->setBindingIdentifiers(finalizeScriptExpression(
            bindable, pathFromOwner.withField(Fields::bindingIdentifiers), rootMap));
}

bool QQmlDomAstCreator::visit(AST::UiObjectBinding *el)
{
    BindingType bType = (el->hasOnToken ? BindingType::OnBinding : BindingType::Normal);
    QmlObject value;
    value.setName(toString(el->qualifiedTypeNameId));
    Binding *bPtr;
    Path bPathFromOwner = current<QmlObject>().addBinding(
            Binding(toString(el->qualifiedId), value, bType), AddOption::KeepExisting, &bPtr);
    if (bPtr->name() == u"id")
        qmlFile.addError(std::move(astParseErrors()
                                 .warning(tr("id attributes should only be a lower case letter "
                                             "followed by letters, numbers or underscore, "
                                             "assuming they refer to an id property"))
                                           .withPath(bPathFromOwner)));
    setBindingIdentifiers(bPathFromOwner, el->qualifiedId, bPtr);

    pushEl(bPathFromOwner, *bPtr, el);
    if (el->hasOnToken)
        FileLocations::addRegion(nodeStack.last().fileLocations, OnTokenRegion, el->colonToken);
    else
        FileLocations::addRegion(nodeStack.last().fileLocations, ColonTokenRegion, el->colonToken);
    FileLocations::addRegion(nodeStack.last().fileLocations, IdentifierRegion, combineLocations(el->qualifiedId));
    loadAnnotations(el);
    QmlObject *objValue = bPtr->objectValue();
    Q_ASSERT_X(objValue, className, "could not recover objectValue");
    objValue->setName(toString(el->qualifiedTypeNameId));

    if (m_enableScriptExpressions) {
        auto qmlObjectType = makeGenericScriptElement(el->qualifiedTypeNameId, DomType::ScriptType);
        qmlObjectType->insertChild(Fields::typeName,
                                   fieldMemberExpressionForQualifiedId(el->qualifiedTypeNameId));
        objValue->setNameIdentifiers(finalizeScriptExpression(
                ScriptElementVariant::fromElement(qmlObjectType),
                bPathFromOwner.withField(Fields::value).withField(Fields::nameIdentifiers), rootMap));
    }

    objValue->addPrototypePath(Paths::lookupTypePath(objValue->name()));
    pushEl(bPathFromOwner.withField(Fields::value), *objValue, el->initializer);
    if (m_enableScriptExpressions && el->initializer) {
        FileLocations::addRegion(nodeStack.last().fileLocations, LeftBraceRegion,
                                 el->initializer->lbraceToken);
        FileLocations::addRegion(nodeStack.last().fileLocations, RightBraceRegion,
                                 el->initializer->rbraceToken);
    }
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiObjectBinding *)
{
    QmlObject &objValue = current<QmlObject>();
    QmlObject &containingObj = current<QmlObject>(1);
    Binding &b = std::get<Binding>(currentNode(1).value);
    QmlObject *objPtr = b.objectValue();
    Q_ASSERT(objPtr);
    *objPtr = objValue;
    index_type idx = currentNodeEl(1).path.last().headIndex();
    Binding *bPtr = valueFromMultimap(containingObj.m_bindings, b.name(), idx);
    Q_ASSERT(bPtr);
    *bPtr = b;
    removeCurrentNode(DomType::QmlObject);
    removeCurrentNode(DomType::Binding);
}

bool QQmlDomAstCreator::visit(AST::UiScriptBinding *el)
{
    ++m_nestedFunctionDepth;
    QStringView code = qmlFilePtr->code();
    SourceLocation loc = combineLocations(el->statement);
    auto script = std::make_shared<ScriptExpression>(
            code.mid(loc.offset, loc.length), qmlFilePtr->engine(), el->statement,
            qmlFilePtr->astComments(), ScriptExpression::ExpressionType::BindingExpression, loc);
    Binding bindingV(toString(el->qualifiedId), script, BindingType::Normal);
    Binding *bindingPtr = nullptr;
    Id *idPtr = nullptr;
    Path pathFromOwner;
    if (bindingV.name() == u"id") {
        Node *exp = script->ast();
        if (ExpressionStatement *eStat = cast<ExpressionStatement *>(script->ast()))
            exp = eStat->expression;
        if (IdentifierExpression *iExp = cast<IdentifierExpression *>(exp)) {
            QmlStackElement &containingObjectEl = currentEl<QmlObject>();
            QmlObject &containingObject = std::get<QmlObject>(containingObjectEl.item.value);
            QString idName = iExp->name.toString();
            Id idVal(idName, qmlFile.canonicalPath().withPath(containingObject.pathFromOwner()));
            idVal.value = script;
            containingObject.setIdStr(idName);
            FileLocations::addRegion(containingObjectEl.fileLocations, IdTokenRegion,
                                     combineLocations(el->qualifiedId));
            FileLocations::addRegion(containingObjectEl.fileLocations, IdColonTokenRegion,
                                     el->colonToken);
            FileLocations::addRegion(containingObjectEl.fileLocations, IdNameRegion,
                                     combineLocations(el->statement));
            QmlComponent &comp = current<QmlComponent>();
            pathFromOwner = comp.addId(idVal, AddOption::KeepExisting, &idPtr);
            QRegularExpression idRe(QRegularExpression::anchoredPattern(
                    QStringLiteral(uR"([[:lower:]][[:lower:][:upper:]0-9_]*)")));
            auto m = idRe.matchView(iExp->name);
            if (!m.hasMatch()) {
                qmlFile.addError(std::move(
                        astParseErrors()
                                .warning(tr("id attributes should only be a lower case letter "
                                            "followed by letters, numbers or underscore, not %1")
                                                 .arg(iExp->name))
                                .withPath(pathFromOwner)));
            }
        } else {
            pathFromOwner =
                    current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
            Q_ASSERT_X(bindingPtr, className, "binding could not be retrieved");
            qmlFile.addError(std::move(
                    astParseErrors()
                            .warning(tr("id attributes should only be a lower case letter "
                                        "followed by letters, numbers or underscore, not %1 "
                                        "%2, assuming they refer to a property")
                                             .arg(script->code(), script->astRelocatableDump()))
                            .withPath(pathFromOwner)));
        }
    } else {
        pathFromOwner =
                current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
        QmlStackElement &containingObjectEl = currentEl<QmlObject>();
        // remove the containingObjectEl.path prefix from pathFromOwner
        Path pathFromContainingObject = pathFromOwner.mid(containingObjectEl.path.length());
        auto bindingFileLocation =
                FileLocations::ensure(containingObjectEl.fileLocations, pathFromContainingObject);
        FileLocations::addRegion(bindingFileLocation, IdentifierRegion,
                                 el->qualifiedId->identifierToken);
        FileLocations::addRegion(bindingFileLocation, ColonTokenRegion, el->colonToken);

        setBindingIdentifiers(pathFromOwner, el->qualifiedId, bindingPtr);

        Q_ASSERT_X(bindingPtr, className, "binding could not be retrieved");
    }
    if (bindingPtr)
        pushEl(pathFromOwner, *bindingPtr, el);
    else if (idPtr)
        pushEl(pathFromOwner, *idPtr, el);
    else
        Q_UNREACHABLE();
    loadAnnotations(el);
    // avoid duplicate colon location for id?
    FileLocations::addRegion(nodeStack.last().fileLocations, ColonTokenRegion, el->colonToken);
    return true;
}

void QQmlDomAstCreator::setScriptExpression (const std::shared_ptr<ScriptExpression>& value)
{
    if (m_enableScriptExpressions
        && (scriptNodeStack.size() != 1 || currentScriptNodeEl().isList()))
        Q_SCRIPTELEMENT_DISABLE();
    if (m_enableScriptExpressions) {
        FileLocations::Tree valueLoc = FileLocations::ensure(currentNodeEl().fileLocations,
                                                             Path().withField(Fields::value));
        value->setScriptElement(finalizeScriptExpression(currentScriptNodeEl().takeVariant(),
                                                         Path().withField(Fields::scriptElement),
                                                         valueLoc));
        removeCurrentScriptNode({});
    }
};

void QQmlDomAstCreator::endVisit(AST::UiScriptBinding *)
{
    --m_nestedFunctionDepth;
    DomValue &lastEl = currentNode();
    index_type idx = currentIndex();
    if (lastEl.kind == DomType::Binding) {
        Binding &b = std::get<Binding>(lastEl.value);

        setScriptExpression(b.scriptExpressionValue());

        QmlObject &containingObject = current<QmlObject>();
        Binding *bPtr = valueFromMultimap(containingObject.m_bindings, b.name(), idx);
        Q_ASSERT(bPtr);
        *bPtr = b;
    } else if (lastEl.kind == DomType::Id) {
        Id &id = std::get<Id>(lastEl.value);

        setScriptExpression(id.value);

        QmlComponent &comp = current<QmlComponent>();
        Id *idPtr = valueFromMultimap(comp.m_ids, id.name, idx);
        *idPtr = id;
    } else {
        Q_UNREACHABLE();
    }

    // there should be no more uncollected script elements
    if (m_enableScriptExpressions && !scriptNodeStack.empty()) {
        Q_SCRIPTELEMENT_DISABLE();
    }
    removeCurrentNode({});
}

bool QQmlDomAstCreator::visit(AST::UiArrayBinding *el)
{
    QList<QmlObject> value;
    Binding bindingV(toString(el->qualifiedId), value, BindingType::Normal);
    Binding *bindingPtr;
    Path bindingPathFromOwner =
            current<QmlObject>().addBinding(bindingV, AddOption::KeepExisting, &bindingPtr);
    if (bindingV.name() == u"id")
        qmlFile.addError(std::move(
                astParseErrors()
                        .error(tr("id attributes should have only simple strings as values"))
                        .withPath(bindingPathFromOwner)));

    setBindingIdentifiers(bindingPathFromOwner, el->qualifiedId, bindingPtr);

    pushEl(bindingPathFromOwner, *bindingPtr, el);
    FileLocations::addRegion(currentNodeEl().fileLocations, ColonTokenRegion, el->colonToken);
    loadAnnotations(el);
    FileLocations::Tree arrayList =
            createMap(currentNodeEl().fileLocations, Path::fromField(Fields::value), nullptr);
    FileLocations::addRegion(arrayList, LeftBracketRegion, el->lbracketToken);
    FileLocations::addRegion(arrayList, RightBracketRegion, el->rbracketToken);
    arrayBindingLevels.append(nodeStack.size());
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiArrayBinding *)
{
    index_type idx = currentIndex();
    Binding &b = std::get<Binding>(currentNode().value);
    Binding *bPtr = valueFromMultimap(current<QmlObject>().m_bindings, b.name(), idx);
    *bPtr = b;
    arrayBindingLevels.removeLast();
    removeCurrentNode(DomType::Binding);
}

void QQmlDomAstCreator::endVisit(AST::ArgumentList *list)
{
    endVisitForLists(list);
}

bool QQmlDomAstCreator::visit(AST::UiParameterList *)
{
    return false; // do not create script node for Ui stuff
}

void QQmlDomAstCreator::endVisit(AST::PatternElementList *list)
{
    endVisitForLists<AST::PatternElementList>(list, [](AST::PatternElementList *current) {
        int toCollect = 0;
        toCollect += bool(current->elision);
        toCollect += bool(current->element);
        return toCollect;
    });
}

void QQmlDomAstCreator::endVisit(AST::PatternPropertyList *list)
{
    endVisitForLists(list);
}

/*!
   \internal
   Implementing the logic of this method in \c QQmlDomAstCreator::visit(AST::UiQualifiedId *)
   would create scriptelements at places where there are not needed. This is mainly because
   UiQualifiedId's appears inside and outside of script parts.
*/
ScriptElementVariant QQmlDomAstCreator::scriptElementForQualifiedId(AST::UiQualifiedId *expression)
{
    auto id = std::make_shared<ScriptElements::IdentifierExpression>(
            expression->firstSourceLocation(), expression->lastSourceLocation());
    id->setName(expression->toString());

    return ScriptElementVariant::fromElement(id);
}

bool QQmlDomAstCreator::visit(AST::UiQualifiedId *)
{
    if (!m_enableScriptExpressions)
        return false;

    return false;
}

bool QQmlDomAstCreator::visit(AST::UiEnumDeclaration *el)
{
    EnumDecl eDecl;
    eDecl.setName(el->name.toString());
    EnumDecl *ePtr;
    Path enumPathFromOwner =
            current<QmlComponent>().addEnumeration(eDecl, AddOption::KeepExisting, &ePtr);
    pushEl(enumPathFromOwner, *ePtr, el);
    FileLocations::addRegion(nodeStack.last().fileLocations, EnumKeywordRegion, el->enumToken);
    FileLocations::addRegion(nodeStack.last().fileLocations, IdentifierRegion, el->identifierToken);
    loadAnnotations(el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiEnumDeclaration *)
{
    EnumDecl &e = std::get<EnumDecl>(currentNode().value);
    EnumDecl *ePtr =
            valueFromMultimap(current<QmlComponent>().m_enumerations, e.name(), currentIndex());
    Q_ASSERT(ePtr);
    *ePtr = e;
    removeCurrentNode(DomType::EnumDecl);
}

bool QQmlDomAstCreator::visit(AST::UiEnumMemberList *el)
{
    EnumItem it(el->member.toString(), el->value,
                el->valueToken.isValid() ? EnumItem::ValueKind::ExplicitValue
                                         : EnumItem::ValueKind::ImplicitValue);
    EnumDecl &eDecl = std::get<EnumDecl>(currentNode().value);
    Path itPathFromDecl = eDecl.addValue(it);
    const auto map = createMap(DomType::EnumItem, itPathFromDecl, nullptr);
    FileLocations::addRegion(map, MainRegion, combine(el->memberToken, el->valueToken));
    if (el->memberToken.isValid())
        FileLocations::addRegion(map, IdentifierRegion, el->memberToken);
    if (el->valueToken.isValid())
        FileLocations::addRegion(map, EnumValueRegion, el->valueToken);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiEnumMemberList *el)
{
    Node::accept(el->next, this); // put other enum members at the same level as this one...
}

bool QQmlDomAstCreator::visit(AST::UiInlineComponent *el)
{
    QStringList els = current<QmlComponent>().name().split(QLatin1Char('.'));
    els.append(el->name.toString());
    QString cName = els.join(QLatin1Char('.'));
    QmlComponent *compPtr;
    Path p = qmlFilePtr->addComponent(QmlComponent(cName), AddOption::KeepExisting, &compPtr);

    if (m_enableScriptExpressions) {
        auto inlineComponentType =
                makeGenericScriptElement(el->identifierToken, DomType::ScriptType);

        auto typeName = std::make_shared<ScriptElements::IdentifierExpression>(el->identifierToken);
        typeName->setName(el->name);
        inlineComponentType->insertChild(Fields::typeName,
                                         ScriptElementVariant::fromElement(typeName));
        compPtr->setNameIdentifiers(
                finalizeScriptExpression(ScriptElementVariant::fromElement(inlineComponentType),
                                         p.withField(Fields::nameIdentifiers), rootMap));
    }

    pushEl(p, *compPtr, el);
    FileLocations::addRegion(nodeStack.last().fileLocations, ComponentKeywordRegion,
                             el->componentToken);
    FileLocations::addRegion(nodeStack.last().fileLocations, IdentifierRegion, el->identifierToken);
    loadAnnotations(el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiInlineComponent *)
{
    QmlComponent &component = std::get<QmlComponent>(currentNode().value);
    QStringList nameEls = component.name().split(QChar::fromLatin1('.'));
    QString key = nameEls.mid(1).join(QChar::fromLatin1('.'));
    QmlComponent *cPtr = valueFromMultimap(qmlFilePtr->lazyMembers().m_components, key, currentIndex());
    Q_ASSERT(cPtr);
    *cPtr = component;
    removeCurrentNode(DomType::QmlComponent);
}

bool QQmlDomAstCreator::visit(UiRequired *el)
{
    PropertyDefinition pDef;
    pDef.name = el->name.toString();
    pDef.isRequired = true;
    PropertyDefinition *pDefPtr;
    Path pathFromOwner =
            current<QmlObject>().addPropertyDef(pDef, AddOption::KeepExisting, &pDefPtr);
    createMap(DomType::PropertyDefinition, pathFromOwner, el);
    return false;
}

bool QQmlDomAstCreator::visit(AST::UiAnnotation *el)
{
    QmlObject a;
    a.setName(QStringLiteral(u"@") + toString(el->qualifiedTypeNameId));
    // add annotation prototype?
    DomValue &containingElement = currentNode();
    Path pathFromOwner;
    QmlObject *aPtr = nullptr;
    switch (containingElement.kind) {
    case DomType::QmlObject:
        pathFromOwner = std::get<QmlObject>(containingElement.value).addAnnotation(a, &aPtr);
        break;
    case DomType::Binding:
        pathFromOwner = std::get<Binding>(containingElement.value)
                                .addAnnotation(currentNodeEl().path, a, &aPtr);
        break;
    case DomType::Id:
        pathFromOwner =
                std::get<Id>(containingElement.value).addAnnotation(currentNodeEl().path, a, &aPtr);
        break;
    case DomType::PropertyDefinition:
        pathFromOwner = std::get<PropertyDefinition>(containingElement.value)
                                .addAnnotation(currentNodeEl().path, a, &aPtr);
        break;
    case DomType::MethodInfo:
        pathFromOwner = std::get<MethodInfo>(containingElement.value)
                                .addAnnotation(currentNodeEl().path, a, &aPtr);
        break;
    default:
        qCWarning(domLog) << "Unexpected container object for annotation:"
                          << domTypeToString(containingElement.kind);
        Q_UNREACHABLE();
    }
    pushEl(pathFromOwner, *aPtr, el);
    return true;
}

void QQmlDomAstCreator::endVisit(AST::UiAnnotation *)
{
    DomValue &containingElement = currentNode(1);
    Path pathFromOwner;
    QmlObject &a = std::get<QmlObject>(currentNode().value);
    switch (containingElement.kind) {
    case DomType::QmlObject:
        std::get<QmlObject>(containingElement.value).m_annotations[currentIndex()] = a;
        break;
    case DomType::Binding:
        std::get<Binding>(containingElement.value).m_annotations[currentIndex()] = a;
        break;
    case DomType::Id:
        std::get<Id>(containingElement.value).annotations[currentIndex()] = a;
        break;
    case DomType::PropertyDefinition:
        std::get<PropertyDefinition>(containingElement.value).annotations[currentIndex()] = a;
        break;
    case DomType::MethodInfo:
        std::get<MethodInfo>(containingElement.value).annotations[currentIndex()] = a;
        break;
    default:
        Q_UNREACHABLE();
    }
    removeCurrentNode(DomType::QmlObject);
}

void QQmlDomAstCreator::throwRecursionDepthError()
{
    qmlFile.addError(astParseErrors().error(
            tr("Maximum statement or expression depth exceeded in QmlDomAstCreator")));
}

void QQmlDomAstCreator::endVisit(AST::StatementList *list)
{
    endVisitForLists(list);
}

bool QQmlDomAstCreator::visit(AST::BinaryExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::BinaryExpression *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BinaryExpression>(exp);
    current->addLocation(OperatorTokenRegion, exp->operatorToken);
    Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
    current->setRight(currentScriptNodeEl().takeVariant());
    removeCurrentScriptNode({});
    Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
    current->setLeft(currentScriptNodeEl().takeVariant());
    removeCurrentScriptNode({});

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::Block *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::Block *block)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BlockStatement>(block);

    if (block->statements) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->setStatements(currentScriptNodeEl().takeList());
        removeCurrentScriptNode(DomType::List);
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ForStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ForStatement *forStatement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::ForStatement>(forStatement);
    current->addLocation(FileLocationRegion::ForKeywordRegion, forStatement->forToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, forStatement->lparenToken);
    current->addLocation(FileLocationRegion::FirstSemicolonTokenRegion,
                         forStatement->firstSemicolonToken);
    current->addLocation(FileLocationRegion::SecondSemicolonRegion,
                         forStatement->secondSemicolonToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, forStatement->rparenToken);

    if (forStatement->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setBody(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setExpression(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->condition) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setCondition(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }

    if (forStatement->declarations) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        auto variableDeclaration = makeGenericScriptElement(forStatement->declarations,
                                                            DomType::ScriptVariableDeclaration);

        ScriptElements::ScriptList list = currentScriptNodeEl().takeList();
        list.replaceKindForGenericChildren(DomType::ScriptPattern,
                                           DomType::ScriptVariableDeclarationEntry);
        variableDeclaration->insertChild(Fields::declarations, std::move(list));
        removeCurrentScriptNode({});

        current->setDeclarations(ScriptElementVariant::fromElement(variableDeclaration));

        if (auto pe = forStatement->declarations->declaration;
            pe && pe->declarationKindToken.isValid()) {
            current->addLocation(FileLocationRegion::TypeIdentifierRegion,
                                 pe->declarationKindToken);
        }
    }

    if (forStatement->initialiser) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setInitializer(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode(std::nullopt);
    }
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::IdentifierExpression *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::IdentifierExpression>(expression);
    current->setName(expression->name);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::NumericLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(expression->value);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::StringLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    pushScriptElement(makeStringLiteral(expression->value, expression));
    return true;
}

bool QQmlDomAstCreator::visit(AST::NullExpression *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(nullptr);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::TrueLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(true);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::FalseLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(false);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::IdentifierPropertyName *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::IdentifierExpression>(expression);
    current->setName(expression->id);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::StringLiteralPropertyName *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    pushScriptElement(makeStringLiteral(expression->id, expression));
    return true;
}

bool QQmlDomAstCreator::visit(AST::TypeAnnotation *)
{
    if (!m_enableScriptExpressions)
        return false;

    // do nothing: the work is done in (end)visit(AST::Type*).
    return true;
}

bool QQmlDomAstCreator::visit(AST::RegExpLiteral *literal)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeGenericScriptElement(literal, DomType::ScriptRegExpLiteral);
    current->insertValue(Fields::regExpPattern, literal->pattern);
    current->insertValue(Fields::regExpFlags, literal->flags);
    pushScriptElement(current);

    return true;
}

bool QQmlDomAstCreator::visit(AST::ThisExpression *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeGenericScriptElement(expression, DomType::ScriptThisExpression);
    if (expression->thisToken.isValid())
        current->addLocation(ThisKeywordRegion, expression->thisToken);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::SuperLiteral *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeGenericScriptElement(expression, DomType::ScriptSuperLiteral);
    if (expression->superToken.isValid())
        current->addLocation(SuperKeywordRegion, expression->superToken);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::NumericLiteralPropertyName *expression)
{
    if (!m_enableScriptExpressions)
        return false;

    auto current = makeScriptElement<ScriptElements::Literal>(expression);
    current->setLiteralValue(expression->id);
    pushScriptElement(current);
    return true;
}

bool QQmlDomAstCreator::visit(AST::ComputedPropertyName *)
{
    if (!m_enableScriptExpressions)
        return false;

    // nothing to do, just forward the underlying expression without changing/wrapping it
    return true;
}

template<typename T>
void QQmlDomAstCreator::endVisitForLists(T *list,
                                         const std::function<int(T *)> &scriptElementsPerEntry)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptList(list);
    for (auto it = list; it; it = it->next) {
        const int entriesToCollect = scriptElementsPerEntry ? scriptElementsPerEntry(it) : 1;
        for (int i = 0; i < entriesToCollect; ++i) {
            Q_SCRIPTELEMENT_EXIT_IF(scriptNodeStack.isEmpty());
            auto last = scriptNodeStack.takeLast();
            if (last.isList())
                current.append(last.takeList());
            else
                current.append(last.takeVariant());
        }
    }

    current.reverse();
    pushScriptElement(current);
}

void QQmlDomAstCreator::endVisit(AST::VariableDeclarationList *list)
{
    endVisitForLists(list);
}

bool QQmlDomAstCreator::visit(AST::Elision *list)
{
    if (!m_enableScriptExpressions)
        return false;

    auto currentList = makeScriptList(list);

    for (auto it = list; it; it = it->next) {
        auto current = makeGenericScriptElement(it->commaToken, DomType::ScriptElision);
        currentList.append(ScriptElementVariant::fromElement(current));
    }
    pushScriptElement(currentList);

    return false; // return false because we already iterated over the children using the custom
                  // iteration above
}

bool QQmlDomAstCreator::visit(AST::PatternElement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

/*!
   \internal
    Avoid code-duplication, reuse this code when doing endVisit on types inheriting from
    AST::PatternElement.
*/
void QQmlDomAstCreator::endVisitHelper(
        AST::PatternElement *pe,
        const std::shared_ptr<ScriptElements::GenericScriptElement> &current)
{
    if (pe->equalToken.isValid())
        current->addLocation(FileLocationRegion::EqualTokenRegion, pe->equalToken);

    if (pe->identifierToken.isValid() && !pe->bindingIdentifier.isEmpty()) {
        auto identifier =
                std::make_shared<ScriptElements::IdentifierExpression>(pe->identifierToken);
        identifier->setName(pe->bindingIdentifier);
        current->insertChild(Fields::identifier, ScriptElementVariant::fromElement(identifier));
    }
    if (pe->initializer) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::initializer, scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }
    if (pe->typeAnnotation) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::type, scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }
    if (pe->bindingTarget) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::bindingElement, scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }
}

void QQmlDomAstCreator::endVisit(AST::PatternElement *pe)
{
    if (!m_enableScriptExpressions)
        return;

    auto element = makeGenericScriptElement(pe, DomType::ScriptPattern);
    endVisitHelper(pe, element);
    // check if helper disabled scriptexpressions
    if (!m_enableScriptExpressions)
        return;

    pushScriptElement(element);
}

bool QQmlDomAstCreator::visit(AST::IfStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::IfStatement *ifStatement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::IfStatement>(ifStatement);
    current->addLocation(LeftParenthesisRegion, ifStatement->lparenToken);
    current->addLocation(RightParenthesisRegion, ifStatement->rparenToken);
    current->addLocation(ElseKeywordRegion, ifStatement->elseToken);
    current->addLocation(IfKeywordRegion, ifStatement->ifToken);

    if (ifStatement->ko) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setAlternative(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }

    if (ifStatement->ok) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setConsequence(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }
    if (ifStatement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setCondition(scriptNodeStack.last().takeVariant());
        scriptNodeStack.removeLast();
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ReturnStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ReturnStatement *returnStatement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::ReturnStatement>(returnStatement);
    current->addLocation(ReturnKeywordRegion, returnStatement->returnToken);

    if (returnStatement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setExpression(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::YieldExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::YieldExpression *yExpression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(yExpression, DomType::ScriptYieldExpression);
    current->addLocation(YieldKeywordRegion, yExpression->yieldToken);

    if (yExpression->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::FieldMemberExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::FieldMemberExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BinaryExpression>(expression);
    current->setOp(ScriptElements::BinaryExpression::FieldMemberAccess);
    current->addLocation(FileLocationRegion::OperatorTokenRegion, expression->dotToken);

    if (expression->base) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setLeft(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    auto scriptIdentifier =
            std::make_shared<ScriptElements::IdentifierExpression>(expression->identifierToken);
    scriptIdentifier->setName(expression->name);
    current->setRight(ScriptElementVariant::fromElement(scriptIdentifier));

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ArrayMemberExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ArrayMemberExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BinaryExpression>(expression);
    current->setOp(ScriptElements::BinaryExpression::ArrayMemberAccess);
    current->addLocation(FileLocationRegion::OperatorTokenRegion, expression->lbracketToken);

    if (expression->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        // if scriptNodeStack.last() is fieldmember expression, add expression to it instead of
        // creating new one
        current->setRight(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (expression->base) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setLeft(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::CallExpression *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::CallExpression *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptCallExpression);
    current->addLocation(LeftParenthesisRegion, exp->lparenToken);
    current->addLocation(RightParenthesisRegion, exp->rparenToken);

    if (exp->arguments) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::arguments, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    } else {
        // insert empty list
        current->insertChild(Fields::arguments,
                             ScriptElements::ScriptList(exp->lparenToken, exp->rparenToken));
    }

    if (exp->base) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::callee, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ArrayPattern *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ArrayPattern *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptArray);

    if (exp->elements) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        ScriptElements::ScriptList list = currentScriptNodeEl().takeList();
        list.replaceKindForGenericChildren(DomType::ScriptPattern, DomType::ScriptArrayEntry);
        current->insertChild(Fields::elements, std::move(list));

        removeCurrentScriptNode({});
    } else {
        // insert empty list
        current->insertChild(Fields::elements,
                             ScriptElements::ScriptList(exp->lbracketToken, exp->rbracketToken));
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ObjectPattern *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ObjectPattern *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptObject);

    if (exp->properties) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::properties, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    } else {
        // insert empty list
        current->insertChild(Fields::properties,
                             ScriptElements::ScriptList(exp->lbraceToken, exp->rbraceToken));
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PatternProperty *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::PatternProperty *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptProperty);

    // handle the stuff from PatternProperty's base class PatternElement
    endVisitHelper(static_cast<PatternElement *>(exp), current);

    // check if helper disabled scriptexpressions
    if (!m_enableScriptExpressions)
        return;

    if (exp->name) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::name, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::VariableStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::VariableStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptVariableDeclaration);
    current->addLocation(FileLocationRegion::TypeIdentifierRegion, statement->declarationKindToken);

    if (statement->declarations) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());

        ScriptElements::ScriptList list = currentScriptNodeEl().takeList();
        list.replaceKindForGenericChildren(DomType::ScriptPattern,
                                           DomType::ScriptVariableDeclarationEntry);
        current->insertChild(Fields::declarations, std::move(list));

        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::Type *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::Type *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptType);

    if (exp->typeArgument) {
        current->insertChild(Fields::typeArgumentName,
                             fieldMemberExpressionForQualifiedId(exp->typeArgument));
        current->addLocation(FileLocationRegion::IdentifierRegion, combineLocations(exp->typeArgument));
    }

    if (exp->typeId) {
        current->insertChild(Fields::typeName, fieldMemberExpressionForQualifiedId(exp->typeId));
        current->addLocation(FileLocationRegion::TypeIdentifierRegion, combineLocations(exp->typeId));
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::DefaultClause *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::DefaultClause *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptDefaultClause);
    current->addLocation(DefaultKeywordRegion, exp->defaultToken);
    current->addLocation(ColonTokenRegion, exp->colonToken);

    if (exp->statements) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::statements, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::CaseClause *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::CaseClause *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptCaseClause);
    current->addLocation(FileLocationRegion::CaseKeywordRegion, exp->caseToken);
    current->addLocation(FileLocationRegion::ColonTokenRegion, exp->colonToken);

    if (exp->statements) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::statements, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    }

    if (exp->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::CaseClauses *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::CaseClauses *list)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptList(list);

    for (auto it = list; it; it = it->next) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current.append(scriptNodeStack.takeLast().takeVariant());
    }

    current.reverse();
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::CaseBlock *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::CaseBlock *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptCaseBlock);
    current->addLocation(FileLocationRegion::LeftBraceRegion, exp->lbraceToken);
    current->addLocation(FileLocationRegion::RightBraceRegion, exp->rbraceToken);

    if (exp->moreClauses) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::moreCaseClauses, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    }

    if (exp->defaultClause) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::defaultClause, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (exp->clauses) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::caseClauses, currentScriptNodeEl().takeList());
        removeCurrentScriptNode({});
    }
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::SwitchStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::SwitchStatement *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptSwitchStatement);
    current->addLocation(FileLocationRegion::SwitchKeywordRegion, exp->switchToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, exp->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, exp->rparenToken);

    if (exp->block) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::caseBlock, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }
    if (exp->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::WhileStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::WhileStatement *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptWhileStatement);
    current->addLocation(FileLocationRegion::WhileKeywordRegion, exp->whileToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, exp->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, exp->rparenToken);

    if (exp->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::body, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (exp->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::DoWhileStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::DoWhileStatement *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptDoWhileStatement);
    current->addLocation(FileLocationRegion::DoKeywordRegion, exp->doToken);
    current->addLocation(FileLocationRegion::WhileKeywordRegion, exp->whileToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, exp->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, exp->rparenToken);

    if (exp->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (exp->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::body, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ForEachStatement *)
{
    if (!m_enableScriptExpressions)
        return false;

    return true;
}

void QQmlDomAstCreator::endVisit(AST::ForEachStatement *exp)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(exp, DomType::ScriptForEachStatement);
    current->addLocation(FileLocationRegion::ForKeywordRegion, exp->forToken);
    current->addLocation(FileLocationRegion::InOfTokenRegion, exp->inOfToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, exp->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, exp->rparenToken);

    if (exp->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::body, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }
    if (exp->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (exp->lhs) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::bindingElement, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});

        if (auto pe = AST::cast<PatternElement *>(exp->lhs);
            pe && pe->declarationKindToken.isValid()) {
            current->addLocation(FileLocationRegion::TypeIdentifierRegion,
                                 pe->declarationKindToken);
        }
    }

    pushScriptElement(current);
}


bool QQmlDomAstCreator::visit(AST::ClassExpression *)
{
    // TODO: Add support for js expressions in classes
    // For now, turning off explicitly to avoid unwanted problems
    if (m_enableScriptExpressions)
        Q_SCRIPTELEMENT_DISABLE();
    return true;
}

void QQmlDomAstCreator::endVisit(AST::ClassExpression *)
{
}

void QQmlDomAstCreator::endVisit(AST::TaggedTemplate *literal)
{
    if (!m_enableScriptExpressions)
        return;
    auto current = makeGenericScriptElement(literal, DomType::ScriptTaggedTemplate);
    Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
    current->insertChild(Fields::templateLiteral, scriptNodeStack.takeLast().takeVariant());
    Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
    current->insertChild(Fields::callee, scriptNodeStack.takeLast().takeVariant());
    pushScriptElement(current);
}

/*!
\internal
Denotes the position of a template part in a template string. For example, in \c{`a${b}c${d}`}, \c a
is \c AtBeginning and \c{${d}} is \c AtEnd while the others are \c InMiddle, and in \c{`a`}, \c a is
\c AtBeginning and \c AtEnd.
 */
enum TemplatePartPosition : quint8 {
    InMiddle = 0,
    AtBeginning = 0x1,
    AtEnd = 0x2,
};

Q_DECLARE_FLAGS(TemplatePartPositions, TemplatePartPosition)

/*!
\internal
Sets the DollarLeftBraceTokenRegion sourcelocation in currentExpression if templatePartLocation
claims that toBeSplit ends in \c{${}.
*/
static void extractDollarBraceSourceLocationInto(
        const std::shared_ptr<ScriptElements::GenericScriptElement> &currentExpression,
        const SourceLocation &toBeSplit, QStringView code,
        TemplatePartPositions templatePartLocation)
{
    if (templatePartLocation & AtEnd || !currentExpression)
        return;

    const auto offset = toBeSplit.offset + toBeSplit.length - 2;
    constexpr auto length = quint32(std::char_traits<char>::length("${"));
    const auto [row, column] = SourceLocation::rowAndColumnFrom(code, offset, toBeSplit);
    currentExpression->addLocation(FileLocationRegion::DollarLeftBraceTokenRegion,
                                   SourceLocation{ offset, length, row, column });
    return;
}

/*!
\internal
See also \l extractDollarBraceSourceLocationInto.
*/
static void extractRightBacktickSourceLocationInto(
        const std::shared_ptr<ScriptElements::GenericScriptElement> &currentTemplate,
        const SourceLocation &toBeSplit, QStringView code,
        TemplatePartPositions templatePartLocation)
{
    if (!(templatePartLocation & AtEnd))
        return;

    const auto offset = toBeSplit.offset + toBeSplit.length - 1;
    constexpr auto length = quint32(std::char_traits<char>::length("`"));
    const auto [row, column] = SourceLocation::rowAndColumnFrom(code, offset, toBeSplit);
    currentTemplate->addLocation(FileLocationRegion::RightBacktickTokenRegion,
                                 SourceLocation{ offset, length, row, column });
}

/*!
\internal
See also \l extractDollarBraceSourceLocationInto.
*/
static void extractLeftBacktickSourceLocationInto(
        const std::shared_ptr<ScriptElements::GenericScriptElement> &currentTemplate,
        const SourceLocation &toBeSplit, TemplatePartPositions templatePartLocation)
{
    if (!(templatePartLocation & AtBeginning))
        return;

    constexpr auto length = quint32(std::char_traits<char>::length("`"));
    const QQmlJS::SourceLocation leftBacktick{ toBeSplit.offset, length, toBeSplit.startLine,
                                               toBeSplit.startColumn };
    currentTemplate->addLocation(FileLocationRegion::LeftBacktickTokenRegion, leftBacktick);
}

/*!
\internal
See also \l extractDollarBraceSourceLocationInto, but returns the extracted right brace instead of
inserting right away.
*/
static SourceLocation extractRightBraceSourceLocation(const SourceLocation &toBeSplit,
                                                      TemplatePartPositions templatePartLocation)
{
    if (templatePartLocation & AtBeginning)
        return SourceLocation{};

    // extract } at the beginning and insert in next loop iteration
    return SourceLocation{ toBeSplit.offset, 1, toBeSplit.startLine, toBeSplit.startColumn };
}

/*!
\internal
Cleans the toBeSplit sourcelocation from potential backticks, dollar braces and right braces to only
contain the location of the string part.
*/
static SourceLocation extractStringLocation(const SourceLocation &toBeSplit, QStringView code,
                                            TemplatePartPositions location)
{

    // remove "`" or "}" at beginning and "`" or "${" at the end of this location.
    const quint32 length = toBeSplit.length - (location & AtEnd ? 2 : 3);
    const quint32 offset = toBeSplit.offset + 1;
    const auto [row, column] = SourceLocation::rowAndColumnFrom(code, offset, toBeSplit);
    return SourceLocation{ offset, length, row, column };
}

void QQmlDomAstCreator::endVisit(AST::TemplateLiteral *literal)
{
    if (!m_enableScriptExpressions)
        return;

    // AST::TemplateLiteral is a list and a TemplateLiteral at the same time:
    // in the Dom representation wrap the list into a separate TemplateLiteral Item.
    auto currentList = makeScriptList(literal);
    auto currentTemplate = makeGenericScriptElement(literal, DomType::ScriptTemplateLiteral);

    const auto children = [&literal]() {
        std::vector<AST::TemplateLiteral *> result;
        for (auto it = literal; it; it = it->next) {
            result.push_back(it);
        }
        return result;
    }();

    SourceLocation rightBrace;
    for (auto it = children.crbegin(); it != children.crend(); ++it) {
        // literalToken contains "`", "${", "}", for example "`asdf${" or "}asdf${"
        const QQmlJS::SourceLocation toBeSplit = (*it)->literalToken;
        std::shared_ptr<ScriptElements::GenericScriptElement> currentExpression;

        if ((*it)->expression) {
            Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
            currentExpression = makeGenericScriptElement((*it)->expression,
                                                         DomType::ScriptTemplateExpressionPart);

            currentExpression->insertChild(Fields::expression,
                                           scriptNodeStack.takeLast().takeVariant());
            if (rightBrace.isValid()) {
                currentExpression->addLocation(FileLocationRegion::RightBraceRegion,
                                               std::exchange(rightBrace, SourceLocation{}));
            }
            currentList.append(ScriptElementVariant::fromElement(currentExpression));
        }

        if (!toBeSplit.isValid())
            continue;

        const TemplatePartPositions location = [&it, &children]() {
            TemplatePartPositions result;
            if (it == children.crbegin())
                result |= AtEnd;
            if (it == std::prev(children.crend()))
                result |= AtBeginning;
            return result;
        }();

        extractRightBacktickSourceLocationInto(currentTemplate, toBeSplit, qmlFilePtr->code(),
                                               location);
        extractLeftBacktickSourceLocationInto(currentTemplate, toBeSplit, location);

        extractDollarBraceSourceLocationInto(currentExpression, toBeSplit, qmlFilePtr->code(),
                                             location);
        rightBrace = extractRightBraceSourceLocation(toBeSplit, location);

        if ((*it)->rawValue.isEmpty())
            continue;

        const SourceLocation stringLocation =
                extractStringLocation(toBeSplit, qmlFilePtr->code(), location);
        auto currentString =
                makeGenericScriptElement(stringLocation, DomType::ScriptTemplateStringPart);
        currentString->insertValue(Fields::value, (*it)->rawValue);

        currentList.append(ScriptElementVariant::fromElement(currentString));
    }
    currentList.reverse();

    currentTemplate->insertChild(Fields::components, currentList);
    pushScriptElement(currentTemplate);
}

bool QQmlDomAstCreator::visit(AST::TryStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::TryStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptTryCatchStatement);
    current->addLocation(FileLocationRegion::TryKeywordRegion, statement->tryToken);

    if (auto exp = statement->finallyExpression) {
        current->addLocation(FileLocationRegion::FinallyKeywordRegion, exp->finallyToken);

        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::finallyBlock, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (auto exp = statement->catchExpression) {
        current->addLocation(FileLocationRegion::CatchKeywordRegion, exp->catchToken);
        current->addLocation(FileLocationRegion::LeftParenthesisRegion, exp->lparenToken);
        current->addLocation(FileLocationRegion::RightParenthesisRegion, exp->rparenToken);

        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::catchBlock, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::catchParameter, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (statement->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::block, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::Catch *)
{
    // handled in visit(AST::TryStatement* )
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::Catch *)
{
    // handled in endVisit(AST::TryStatement* )
}

bool QQmlDomAstCreator::visit(AST::Finally *)
{
    // handled in visit(AST::TryStatement* )
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::Finally *)
{
    // handled in endVisit(AST::TryStatement* )
}

bool QQmlDomAstCreator::visit(AST::ThrowStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::ThrowStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptThrowStatement);
    current->addLocation(FileLocationRegion::ThrowKeywordRegion, statement->throwToken);

    if (statement->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::LabelledStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::LabelledStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptLabelledStatement);
    current->addLocation(FileLocationRegion::ColonTokenRegion, statement->colonToken);

    auto label = std::make_shared<ScriptElements::IdentifierExpression>(statement->identifierToken);
    label->setName(statement->label);
    current->insertChild(Fields::label, ScriptElementVariant::fromElement(label));


    if (statement->statement) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::statement, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::BreakStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::BreakStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptBreakStatement);
    current->addLocation(FileLocationRegion::BreakKeywordRegion, statement->breakToken);

    if (!statement->label.isEmpty()) {
        auto label =
                std::make_shared<ScriptElements::IdentifierExpression>(statement->identifierToken);
        label->setName(statement->label);
        current->insertChild(Fields::label, ScriptElementVariant::fromElement(label));
    }

    pushScriptElement(current);
}

// note: thats for comma expressions
bool QQmlDomAstCreator::visit(AST::Expression *)
{
    return m_enableScriptExpressions;
}

// note: thats for comma expressions
void QQmlDomAstCreator::endVisit(AST::Expression *commaExpression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeScriptElement<ScriptElements::BinaryExpression>(commaExpression);
    current->addLocation(OperatorTokenRegion, commaExpression->commaToken);

    if (commaExpression->right) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setRight(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (commaExpression->left) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->setLeft(currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ConditionalExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::ConditionalExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(expression, DomType::ScriptConditionalExpression);
    current->addLocation(FileLocationRegion::QuestionMarkTokenRegion, expression->questionToken);
    current->addLocation(FileLocationRegion::ColonTokenRegion, expression->colonToken);

    if (expression->ko) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::alternative, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (expression->ok) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::consequence, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    if (expression->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::condition, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::ContinueStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::ContinueStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptContinueStatement);
    current->addLocation(FileLocationRegion::ContinueKeywordRegion, statement->continueToken);

    if (!statement->label.isEmpty()) {
        auto label =
                std::make_shared<ScriptElements::IdentifierExpression>(statement->identifierToken);
        label->setName(statement->label);
        current->insertChild(Fields::label, ScriptElementVariant::fromElement(label));
    }

    pushScriptElement(current);
}

/*!
   \internal
   Helper to create unary expressions from AST nodes.
   \sa makeGenericScriptElement
 */
std::shared_ptr<ScriptElements::GenericScriptElement>
QQmlDomAstCreator::makeUnaryExpression(AST::Node *expression, QQmlJS::SourceLocation operatorToken,
                                       bool hasExpression, UnaryExpressionKind kind)
{
    const DomType type = [&kind]() {
        switch (kind) {
        case Prefix:
            return DomType::ScriptUnaryExpression;
        case Postfix:
            return DomType::ScriptPostExpression;
        }
        Q_UNREACHABLE_RETURN(DomType::ScriptUnaryExpression);
    }();

    auto current = makeGenericScriptElement(expression, type);
    current->addLocation(FileLocationRegion::OperatorTokenRegion, operatorToken);

    if (hasExpression) {
        if (!stackHasScriptVariant()) {
            Q_SCRIPTELEMENT_DISABLE();
            return {};
        }
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    return current;
}

bool QQmlDomAstCreator::visit(AST::UnaryMinusExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::UnaryMinusExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->minusToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::UnaryPlusExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::UnaryPlusExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->plusToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::TildeExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::TildeExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->tildeToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::NotExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::NotExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->notToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::TypeOfExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::TypeOfExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->typeofToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::DeleteExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::DeleteExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->deleteToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::VoidExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::VoidExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->voidToken, statement->expression, Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PostDecrementExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::PostDecrementExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->decrementToken, statement->base, Postfix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PostIncrementExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::PostIncrementExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current =
            makeUnaryExpression(statement, statement->incrementToken, statement->base, Postfix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PreIncrementExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::PreIncrementExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeUnaryExpression(statement, statement->incrementToken, statement->expression,
                                       Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::EmptyStatement *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::EmptyStatement *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(statement, DomType::ScriptEmptyStatement);
    current->addLocation(FileLocationRegion::SemicolonTokenRegion, statement->semicolonToken);
    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::NestedExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::NestedExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(expression, DomType::ScriptParenthesizedExpression);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, expression->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, expression->rparenToken);

    if (expression->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::NewExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::NewExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(expression, DomType::ScriptNewExpression);
    current->addLocation(FileLocationRegion::NewKeywordRegion, expression->newToken);

    if (expression->expression) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::expression, currentScriptNodeEl().takeVariant());
        removeCurrentScriptNode({});
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::NewMemberExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::NewMemberExpression *expression)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeGenericScriptElement(expression, DomType::ScriptNewMemberExpression);
    current->addLocation(FileLocationRegion::NewKeywordRegion, expression->newToken);
    current->addLocation(FileLocationRegion::LeftParenthesisRegion, expression->lparenToken);
    current->addLocation(FileLocationRegion::RightParenthesisRegion, expression->rparenToken);

    if (expression->arguments) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptList());
        current->insertChild(Fields::arguments, scriptNodeStack.takeLast().takeList());
    }
    if (expression->base) {
        Q_SCRIPTELEMENT_EXIT_IF(!stackHasScriptVariant());
        current->insertChild(Fields::base, scriptNodeStack.takeLast().takeVariant());
    }

    pushScriptElement(current);
}

bool QQmlDomAstCreator::visit(AST::PreDecrementExpression *)
{
    return m_enableScriptExpressions;
}

void QQmlDomAstCreator::endVisit(AST::PreDecrementExpression *statement)
{
    if (!m_enableScriptExpressions)
        return;

    auto current = makeUnaryExpression(statement, statement->decrementToken, statement->expression,
                                       Prefix);
    if (!current)
        return;

    pushScriptElement(current);
}

static const DomEnvironment *environmentFrom(MutableDomItem &qmlFile)
{
    auto top = qmlFile.top();
    if (!top) {
        return {};
    }
    auto domEnvironment = top.as<DomEnvironment>();
    if (!domEnvironment) {
        return {};
    }
    return domEnvironment;
}

static QStringList qmldirFilesFrom(MutableDomItem &qmlFile)
{
    if (auto env = environmentFrom(qmlFile))
        return env->qmldirFiles();

    return {};
}

QQmlDomAstCreatorWithQQmlJSScope::QQmlDomAstCreatorWithQQmlJSScope(const QQmlJSScope::Ptr &current,
                                                                   MutableDomItem &qmlFile,
                                                                   QQmlJSLogger *logger,
                                                                   QQmlJSImporter *importer)
    : m_root(current),
      m_logger(logger),
      m_importer(importer),
      m_implicitImportDirectory(QQmlJSImportVisitor::implicitImportDirectory(
              m_logger->filePath(), m_importer->resourceFileMapper())),
      m_scopeCreator(m_root, m_importer, m_logger, m_implicitImportDirectory,
                     qmldirFilesFrom(qmlFile)),
      m_domCreator(qmlFile)
{
}

#define X(name)                                                 \
    bool QQmlDomAstCreatorWithQQmlJSScope::visit(name *node)    \
    {                                                           \
        return visitT(node);                                    \
    }                                                           \
    void QQmlDomAstCreatorWithQQmlJSScope::endVisit(name *node) \
    {                                                           \
        endVisitT(node);                                        \
    }
QQmlJSASTClassListToVisit
#undef X

void QQmlDomAstCreatorWithQQmlJSScope::setScopeInDomAfterEndvisit()
{
    const QQmlJSScope::ConstPtr scope = m_scopeCreator.m_currentScope;
    if (!m_domCreator.scriptNodeStack.isEmpty()) {
        auto topOfStack = m_domCreator.currentScriptNodeEl();
        switch (topOfStack.kind) {
        case DomType::ScriptBlockStatement:
        case DomType::ScriptForStatement:
        case DomType::ScriptForEachStatement:
        case DomType::ScriptDoWhileStatement:
        case DomType::ScriptWhileStatement:
        case DomType::List:
            m_domCreator.currentScriptNodeEl().setSemanticScope(scope);
            break;
        case DomType::ScriptFunctionExpression: {
            // Put the body's scope into the function expression: function expressions will contain
            // their parents scope instead of their own without this
            auto element = m_domCreator.currentScriptNodeEl().value;
            auto scriptElementVariant = std::get_if<ScriptElementVariant>(&element);
            if (!scriptElementVariant || !scriptElementVariant->data())
                break;
            scriptElementVariant->visit([](auto &&e) {
                using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                if (e->kind() != DomType::ScriptFunctionExpression)
                    return;

                if constexpr (std::is_same_v<U,
                                             ScriptElement::PointerType<
                                                     ScriptElements::GenericScriptElement>>) {
                    if (auto bodyPtr = e->elementChild(Fields::body)) {
                        const auto bodyScope = bodyPtr.base()->semanticScope();
                        e->setSemanticScope(bodyScope);
                    }
                }
            });
            break;
        }

        // TODO: find which script elements also have a scope and implement them here
        default:
            break;
        };
    } else if (!m_domCreator.nodeStack.isEmpty()) {
        std::visit(
                [&scope](auto &&e) {
                    using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                    // TODO: find which dom elements also have a scope and implement them here
                    if constexpr (std::is_same_v<U, QmlObject>) {
                        e.setSemanticScope(scope);
                    } else if constexpr (std::is_same_v<U, QmlComponent>) {
                        e.setSemanticScope(scope);
                    } else if constexpr (std::is_same_v<U, MethodInfo>) {
                        if (e.body) {
                            if (auto scriptElement = e.body->scriptElement()) {
                                scriptElement.base()->setSemanticScope(scope);
                            }
                        }
                        e.setSemanticScope(scope);
                    }
                },
                m_domCreator.currentNodeEl().item.value);
    }
}

void QQmlDomAstCreatorWithQQmlJSScope::setScopeInDomBeforeEndvisit()
{
    const QQmlJSScope::ConstPtr scope = m_scopeCreator.m_currentScope;

    // depending whether the property definition has a binding, the property definition might be
    // either at the last position in the stack or at the position before the last position.
    if (m_domCreator.nodeStack.size() > 1
        && m_domCreator.nodeStack.last().item.kind == DomType::Binding) {
        std::visit(
                [&scope](auto &&e) {
                    using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                    if constexpr (std::is_same_v<U, PropertyDefinition>) {
                        // Make sure to use the property definition scope instead of the binding
                        // scope. If the current scope is a binding scope (this happens when the
                        // property definition has a binding, like `property int i: 45` for
                        // example), then the property definition scope is the parent of the current
                        // scope.
                        const bool usePropertyDefinitionScopeInsteadOfTheBindingScope =
                                scope->scopeType() == QQmlSA::ScopeType::JSFunctionScope
                                && scope->parentScope()
                                && scope->parentScope()->scopeType() == QQmlSA::ScopeType::QMLScope;
                        e.setSemanticScope(usePropertyDefinitionScopeInsteadOfTheBindingScope
                                                   ? scope->parentScope()
                                                   : scope);
                    }
                },
                m_domCreator.currentNodeEl(1).item.value);
    }
    if (m_domCreator.nodeStack.size() > 0) {
        std::visit(
                [&scope](auto &&e) {
                    using U = std::remove_cv_t<std::remove_reference_t<decltype(e)>>;
                    if constexpr (std::is_same_v<U, PropertyDefinition>) {
                        e.setSemanticScope(scope);
                        Q_ASSERT(e.semanticScope());
                    } else if constexpr (std::is_same_v<U, MethodInfo>) {
                        if (e.methodType == MethodInfo::Signal) {
                            e.setSemanticScope(scope);
                        }
                    }
                },
                m_domCreator.currentNodeEl().item.value);
    }
}

void QQmlDomAstCreatorWithQQmlJSScope::throwRecursionDepthError()
{
}

} // end namespace Dom
} // end namespace QQmlJS

#undef Q_SCRIPTELEMENT_DISABLE
#undef Q_SCRIPTELEMENT_EXIT_IF

QT_END_NAMESPACE
