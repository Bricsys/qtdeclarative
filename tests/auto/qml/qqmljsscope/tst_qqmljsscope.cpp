// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qurl.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qscopedpointer.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtGui/qfont.h>

#include <QtQml/private/qqmlirbuilder_p.h>
#include <private/qqmljscompiler_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qqmljslogger_p.h>
#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <private/qqmlcomponent_p.h>

using namespace Qt::StringLiterals;

class tst_qqmljsscope : public QQmlDataTest
{
    Q_OBJECT

    QString loadFile(const QString &file)
    {
        const QFileInfo fi(file);
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly))
            qFatal("Could not open file %s", qPrintable(file));
        QByteArray data = f.readAll();
        return QString::fromUtf8(data);
    }

    QQmlJSScope::ConstPtr run(const QString &unresolved, bool expectErrorsOrWarnings = false)
    {
        const QString resolved = testFile(unresolved);
        QmlIR::Document document(resolved, resolved, false);
        return run(resolved, &document, expectErrorsOrWarnings);
    }

    QQmlJSScope::ConstPtr run(const QString &resolvedFile, QmlIR::Document *document,
                              bool expectErrorsOrWarnings = false)
    {
        const QString sourceCode = loadFile(resolvedFile);
        if (sourceCode.isEmpty())
            return QQmlJSScope::ConstPtr();

        // NB: JS unit generated here is ignored, so use noop function
        QQmlJSSaveFunction noop([](auto &&...) { return true; });
        QQmlJSCompileError error;
        [&]() {
            QVERIFY2(qCompileQmlFile(*document, resolvedFile, noop, nullptr, &error),
                     qPrintable(error.message));
        }();
        if (!error.message.isEmpty())
            return QQmlJSScope::ConstPtr();


        QQmlJSLogger logger;
        logger.setFilePath(resolvedFile);
        logger.setCode(sourceCode);
        logger.setSilent(expectErrorsOrWarnings);
        QQmlJSScope::Ptr target = QQmlJSScope::create();
        target->setOwnModuleName(u"HelloModule"_s);
        QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
        QQmlJSTypeResolver typeResolver { &m_importer };
        typeResolver.init(&visitor, document->program);
        if (!expectErrorsOrWarnings) {
            [&]() {
                QVERIFY2(!logger.hasWarnings(), "Expected no warnings in this test");
                QVERIFY2(!logger.hasErrors(), "Expected no errors in this test");
            }();
        }
        if (QTest::currentTestFailed())
            return QQmlJSScope::ConstPtr();
        return visitor.result();
    }

private Q_SLOTS:
    void initTestCase() override;

    void orderedBindings();
    void signalCreationDifferences();
    void allTypesAvailable();
    void shadowing();
    void requiredAlias();

#ifdef LABS_QML_MODELS_PRESENT
    void componentWrappedObjects();
    void labsQmlModelsSanity();
#endif
    void unknownCppBase();
    void groupedProperties();
    void descriptiveNameOfNull();
    void groupedPropertiesConsistency();
    void groupedPropertySyntax();
    void attachedProperties();
    void scriptIndices();
    void extensions();
    void emptyBlockBinding();
    void hasOwnEnumerationKeys();
    void ownModuleName();
    void resolvedNonUniqueScopes();
    void compilationUnitsAreCompatible();
    void attachedTypeResolution_data();
    void attachedTypeResolution();
    void builtinTypeResolution_data();
    void builtinTypeResolution();
    void methodAndSignalSourceLocation();
    void modulePrefixes();
    void javaScriptBuiltinFlag();
    void isRoot();
    void idLocation();

public:
    tst_qqmljsscope()
        : QQmlDataTest(QT_QMLTEST_DATADIR),
          m_importer(
                  {
                          QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
                          dataDirectory(),
                          // Note: to be able to import the QQmlJSScopeTests
                          // correctly, we need an additional import path. Use
                          // this application's binary directory as done by
                          // QQmlTypeLoader
                          QCoreApplication::applicationDirPath(),
                  },
                  nullptr)
    {
    }

private:
    QQmlJSImporter m_importer;
};

void tst_qqmljsscope::initTestCase()
{
    QQmlDataTest::initTestCase();

    QDirIterator it(dataDirectory(), QDirIterator::FollowSymlinks | QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString url = it.next();
        if (!url.endsWith(u".qml"_s)) // not interesting
            continue;
        const QFileInfo fi(url);
        QVERIFY(fi.exists());
        QFile f(fi.absoluteFilePath());
        QVERIFY(f.open(QIODevice::ReadOnly));
    }

    // test that we can import the module shipped with this test: if we have no
    // errors / warnings, it is a success
    QVERIFY(run(u"importOwnModule.qml"_s));
}

void tst_qqmljsscope::orderedBindings()
{
    QQmlJSScope::ConstPtr root = run(u"orderedBindings.qml"_s);
    QVERIFY(root);

    auto [pBindingsBegin, pBindingsEnd] = root->ownPropertyBindings(u"p"_s);
    QCOMPARE(std::distance(pBindingsBegin, pBindingsEnd), 2);

    // check that the bindings are properly ordered
    QCOMPARE(pBindingsBegin->bindingType(), QQmlSA::BindingType::Object);
    QCOMPARE(std::next(pBindingsBegin)->bindingType(), QQmlSA::BindingType::Interceptor);

    auto [itemsBindingsBegin, itemsBindingsEnd] = root->ownPropertyBindings(u"items"_s);
    QCOMPARE(std::distance(itemsBindingsBegin, itemsBindingsEnd), 2);

    QCOMPARE(itemsBindingsBegin->bindingType(), QQmlSA::BindingType::Object);
    QCOMPARE(std::next(itemsBindingsBegin)->bindingType(), QQmlSA::BindingType::Object);

    QCOMPARE(itemsBindingsBegin->objectType()->baseTypeName(), u"Item"_s);
    QCOMPARE(std::next(itemsBindingsBegin)->objectType()->baseTypeName(), u"Text"_s);
}

void tst_qqmljsscope::signalCreationDifferences()
{
    QQmlJSScope::ConstPtr root = run(u"signalCreationDifferences.qml"_s, true);
    QVERIFY(root);

    QVERIFY(root->hasOwnProperty(u"myProperty"_s));
    QVERIFY(root->hasOwnProperty(u"conflictingProperty"_s));
    QCOMPARE(root->ownMethods(u"mySignal"_s).size(), 1);

    const auto conflicting = root->ownMethods(u"conflictingPropertyChanged"_s);
    QCOMPARE(conflicting.size(), 2);
    QCOMPARE(conflicting[0].methodType(), QQmlJSMetaMethodType::Signal);
    QCOMPARE(conflicting[1].methodType(), QQmlJSMetaMethodType::Signal);

    const QQmlJSMetaMethod *explicitMethod = nullptr;
    if (conflicting[0].isImplicitQmlPropertyChangeSignal())
        explicitMethod = &conflicting[1];
    else
        explicitMethod = &conflicting[0];
    QCOMPARE(explicitMethod->parameterNames(), QStringList({ u"a"_s, u"c"_s }));
}

void tst_qqmljsscope::allTypesAvailable()
{
        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
            dataDirectory(),
        };

        QQmlJSImporter importer { importPaths, /* resource file mapper */ nullptr };
        const auto imported = importer.importModule(u"QtQml"_s);
        QCOMPARE(imported.contextualTypes().context(), QQmlJS::ContextualTypes::QML);
        const auto types = imported.types();
        QVERIFY(types.contains(u"$internal$.QObject"_s));
        QVERIFY(types.contains(u"QtObject"_s));
        QCOMPARE(types[u"$internal$.QObject"_s].scope, types[u"QtObject"_s].scope);
}

void tst_qqmljsscope::shadowing()
{
    QQmlJSScope::ConstPtr root = run(u"shadowing.qml"_s);
    QVERIFY(root);

    QVERIFY(root->baseType());

    // Check whether properties are properly shadowed
    const auto properties = root->properties();
    QVERIFY(properties.contains(u"property_not_shadowed"_s));
    QVERIFY(properties.contains(u"property_shadowed"_s));

    QCOMPARE(properties[u"property_not_shadowed"_s].typeName(), u"QString"_s);
    QCOMPARE(properties[u"property_shadowed"_s].typeName(), u"int"_s);

    // Check whether methods are properly shadowed
    const auto methods = root->methods();
    QCOMPARE(methods.count(u"method_not_shadowed"_s), 1);
    QCOMPARE(methods.count(u"method_shadowed"_s), 1);

    QCOMPARE(methods[u"method_not_shadowed"_s].parameterNames().size(), 1);
    QCOMPARE(methods[u"method_shadowed"_s].parameterNames().size(), 0);
}

void tst_qqmljsscope::requiredAlias()
{
    QQmlJSScope::ConstPtr root = run(u"requiredAlias.qml"_s);
    QVERIFY(root);

    // Check whether aliases marked as required are required
    QVERIFY(root->isPropertyRequired("sameScopeAlias"));
    QVERIFY(root->isPropertyRequired("innerScopeAlias"));
}

#ifdef LABS_QML_MODELS_PRESENT
void tst_qqmljsscope::componentWrappedObjects()
{
    QQmlJSScope::ConstPtr root = run(u"componentWrappedObjects.qml"_s);
    QVERIFY(root);

    auto children = root->childScopes();
    QCOMPARE(children.size(), 6);

    const auto isGoodType = [](const QQmlJSScope::ConstPtr &type, const QString &propertyName,
                               bool isWrapped) {
        return type->hasOwnProperty(propertyName)
                && type->isWrappedInImplicitComponent() == isWrapped;
    };

    QVERIFY(isGoodType(children[0], u"nonWrapped1"_s, false));
    QVERIFY(isGoodType(children[1], u"nonWrapped2"_s, false));
    QVERIFY(isGoodType(children[2], u"nonWrapped3"_s, false));
    QVERIFY(isGoodType(children[3], u"wrapped"_s, true));
    QCOMPARE(children[4]->childScopes().size(), 3);
    QVERIFY(isGoodType(children[4]->childScopes()[0], u"wrapped"_s, true));

    QCOMPARE(children[4]->childScopes()[1]->childScopes().size(), 1);
    QVERIFY(isGoodType(children[4]->childScopes()[1]->childScopes()[0], u"wrapped2"_s, true));
    QCOMPARE(children[4]->childScopes()[2]->childScopes().size(), 1);
    QVERIFY(isGoodType(children[4]->childScopes()[2]->childScopes()[0], u"wrapped3"_s, false));
}

void tst_qqmljsscope::labsQmlModelsSanity()
{
    QQmlJSScope::ConstPtr root = run(u"labsQmlModelsSanity.qml"_s);
    QVERIFY(root);
    auto children = root->childScopes();
    QCOMPARE(children.size(), 1);

    // DelegateChooser: it inherits QQmlAbstractDelegateComponent (from
    // QmlModels) which inherits QQmlComponent. While
    // QQmlAbstractDelegateComponent has no properties, QQmlComponent does. If
    // the QmlModels dependency is lost, we don't "see" that DelegateChooser
    // inherits QQmlComponent - and so has no properties from it, hence, we can
    // test exactly that:
    QVERIFY(children[0]->hasProperty(u"progress"_s));
    QVERIFY(children[0]->hasProperty(u"status"_s));
    QVERIFY(children[0]->hasProperty(u"url"_s));
}
#endif

void tst_qqmljsscope::unknownCppBase()
{
    QQmlJSScope::ConstPtr root = run(u"unknownCppBaseAssigningToVar.qml"_s, true);
    QVERIFY(root);
    // we should not crash here, then it is a success
}

void tst_qqmljsscope::groupedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"groupProperties.qml"_s);
    QVERIFY(root);

    QVERIFY(root->hasProperty(u"anchors"_s));
    const auto anchorBindings = root->propertyBindings(u"anchors"_s);
    QVERIFY(!anchorBindings.isEmpty());
    QCOMPARE(anchorBindings.size(), 2); // from type itself and from the base type

    const auto getBindingsWithinGroup =
            [&](QMultiHash<QString, QQmlJSMetaPropertyBinding> *bindings, qsizetype index) -> void {
        const auto &binding = anchorBindings[index];
        QCOMPARE(binding.bindingType(), QQmlSA::BindingType::GroupProperty);
        auto anchorScope = binding.groupType();
        QVERIFY(anchorScope);
        *bindings = anchorScope->ownPropertyBindings();
    };

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfType;
    getBindingsWithinGroup(&bindingsOfType, 0);
    QCOMPARE(bindingsOfType.size(), 2);
    QCOMPARE(value(bindingsOfType, u"left"_s).bindingType(), QQmlSA::BindingType::Script);
    QCOMPARE(value(bindingsOfType, u"leftMargin"_s).bindingType(),
             QQmlSA::BindingType::NumberLiteral);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinGroup(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"top"_s).bindingType(), QQmlSA::BindingType::Script);
}

void tst_qqmljsscope::descriptiveNameOfNull()
{
    QQmlJSRegisterContentPool pool;
    QQmlJSRegisterContent nullContent;
    QCOMPARE(nullContent.descriptiveName(), u"(invalid type)"_s);

    QQmlJSScope::Ptr stored = QQmlJSScope::create();
    stored->setInternalName(u"bar"_s);
    QQmlJSMetaProperty property;
    property.setPropertyName(u"foo"_s);
    property.setTypeName(u"baz"_s);
    QQmlJSRegisterContent unscoped = pool.storedIn(pool.createProperty(
            property, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::InvalidLookupIndex, QQmlJSRegisterContent::Property,
            QQmlJSRegisterContent()), stored);
    QCOMPARE(unscoped.descriptiveName(), u"(invalid type)::foo with type baz (stored as bar)"_s);
}

void tst_qqmljsscope::groupedPropertiesConsistency()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"groupPropertiesConsistency.qml"_s));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        QFont font = qvariant_cast<QFont>(root->property("font"));
        QCOMPARE(font.pixelSize(), 22);
    }

    {
        QQmlJSScope::ConstPtr root = run(u"groupPropertiesConsistency.qml"_s);
        QVERIFY(root);

        const auto fontBindings = root->propertyBindings(u"font"_s);
        QCOMPARE(fontBindings.size(), 2);

        // The binding order in QQmlJSScope case is "reversed": first come
        // bindings on the leaf type, followed by the bindings on the base type
        QCOMPARE(fontBindings[0].bindingType(), QQmlSA::BindingType::GroupProperty);
        QCOMPARE(fontBindings[1].bindingType(), QQmlSA::BindingType::Script);
    }
}

void tst_qqmljsscope::groupedPropertySyntax()
{
    QQmlJSScope::ConstPtr root = run(u"groupPropertySyntax.qml"_s);
    QVERIFY(root);

    const auto fontBindings = root->propertyBindings(u"font"_s);
    QCOMPARE(fontBindings.size(), 1);

    // The binding order in QQmlJSScope case is "reversed": first come
    // bindings on the leaf type, followed by the bindings on the base type
    QCOMPARE(fontBindings[0].bindingType(), QQmlSA::BindingType::GroupProperty);
    auto fontScope = fontBindings[0].groupType();
    QVERIFY(fontScope);
    QCOMPARE(fontScope->accessSemantics(), QQmlJSScope::AccessSemantics::Value);
    auto subbindings = fontScope->ownPropertyBindings();
    QCOMPARE(subbindings.size(), 2);

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QCOMPARE(value(subbindings, u"pixelSize"_s).bindingType(), QQmlSA::BindingType::NumberLiteral);
    QCOMPARE(value(subbindings, u"bold"_s).bindingType(), QQmlSA::BindingType::BoolLiteral);
}

void tst_qqmljsscope::attachedProperties()
{
    QQmlJSScope::ConstPtr root = run(u"attachedProperties.qml"_s);
    QVERIFY(root);

    const auto keysBindings = root->propertyBindings(u"Keys"_s);
    QVERIFY(!keysBindings.isEmpty());
    QCOMPARE(keysBindings.size(), 2); // from type itself and from the base type

    const auto getBindingsWithinAttached =
            [&](QMultiHash<QString, QQmlJSMetaPropertyBinding> *bindings, qsizetype index) -> void {
        const auto &binding = keysBindings[index];
        QCOMPARE(binding.bindingType(), QQmlSA::BindingType::AttachedProperty);
        auto keysScope = binding.attachedType();
        QVERIFY(keysScope);
        QCOMPARE(keysScope->accessSemantics(), QQmlJSScope::AccessSemantics::Reference);
        *bindings = keysScope->ownPropertyBindings();
    };

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfType;
    getBindingsWithinAttached(&bindingsOfType, 0);
    QCOMPARE(bindingsOfType.size(), 2);
    QCOMPARE(value(bindingsOfType, u"enabled"_s).bindingType(), QQmlSA::BindingType::BoolLiteral);
    QCOMPARE(value(bindingsOfType, u"forwardTo"_s).bindingType(), QQmlSA::BindingType::Script);

    QMultiHash<QString, QQmlJSMetaPropertyBinding> bindingsOfBaseType;
    getBindingsWithinAttached(&bindingsOfBaseType, 1);
    QCOMPARE(bindingsOfBaseType.size(), 1);
    QCOMPARE(value(bindingsOfBaseType, u"priority"_s).bindingType(), QQmlSA::BindingType::Script);
}

inline QString getScopeName(const QQmlJSScope::ConstPtr &scope)
{
    Q_ASSERT(scope);
    QQmlJSScope::ScopeType type = scope->scopeType();
    if (type == QQmlSA::ScopeType::GroupedPropertyScope
        || type == QQmlSA::ScopeType::AttachedPropertyScope)
        return scope->internalName();
    return scope->baseTypeName();
}

struct FunctionOrExpressionIdentifier
{
    QString name;
    QQmlJS::SourceLocation loc = QQmlJS::SourceLocation {}; // source location of owning scope
    int index = -1; // relative or absolute script index
    FunctionOrExpressionIdentifier() = default;
    FunctionOrExpressionIdentifier(const QString &name, const QQmlJS::SourceLocation &l, int i)
        : name(name), loc(l), index(i)
    {
    }
    FunctionOrExpressionIdentifier(const QString &name, const QV4::CompiledData::Location &l, int i)
        : name(name), loc(QQmlJS::SourceLocation { 0, 0, l.line(), l.column() }), index(i)
    {
    }

    friend bool operator<(const FunctionOrExpressionIdentifier &x,
                          const FunctionOrExpressionIdentifier &y)
    {
        // source location is taken from the owner scope so would be non-unique,
        // while name must be unique within that scope. so: if source locations
        // match, compare by name
        if (x.loc == y.loc)
            return x.name < y.name;

        // otherwise, compare by start line first, if they match, then by column
        if (x.loc.startLine == y.loc.startLine)
            return x.loc.startColumn < y.loc.startColumn;
        return x.loc.startLine < y.loc.startLine;
    }
    friend bool operator==(const FunctionOrExpressionIdentifier &x,
                           const FunctionOrExpressionIdentifier &y)
    {
        // equal-compare by name and index
        return x.index == y.index && x.name == y.name;
    }
    friend bool operator!=(const FunctionOrExpressionIdentifier &x,
                           const FunctionOrExpressionIdentifier &y)
    {
        return !(x == y);
    }
    friend QDebug &operator<<(QDebug &stream, const FunctionOrExpressionIdentifier &x)
    {
        const QString dump = u"(%1, %2, loc %3:%4)"_s.arg(x.name, QString::number(x.index),
                                                          QString::number(x.loc.startLine),
                                                          QString::number(x.loc.startColumn));
        stream << dump;
        return stream.maybeSpace();
    }
};

void tst_qqmljsscope::scriptIndices()
{
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.loadUrl(testFileUrl(u"functionAndBindingIndices.qml"_s));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
    }

    const QString file = testFile(u"functionAndBindingIndices.qml"_s);
    QmlIR::Document document(file, file, false); // we need QmlIR information here
    QQmlJSScope::ConstPtr root = run(file, &document);
    QVERIFY(root);
    QVERIFY(document.javaScriptCompilationUnit->unitData());

    // compare QQmlJSScope and QmlIR:

    // {property, function}Name and relative (per-object) function table index
    QList<FunctionOrExpressionIdentifier> orderedJSScopeExpressionsRelative;
    QList<FunctionOrExpressionIdentifier> orderedQmlIrExpressionsRelative;
    // {property, function}Name and absolute (per-document) function table index
    QList<FunctionOrExpressionIdentifier> orderedJSScopeExpressionsAbsolute;
    QList<FunctionOrExpressionIdentifier> orderedQmlIrExpressionsAbsolute;

    const auto populateQQmlJSScopeArrays =
            [&](const QQmlJSScope::ConstPtr &scope, const QString &name,
                QQmlJSMetaMethod::RelativeFunctionIndex relativeIndex) {
                orderedJSScopeExpressionsRelative.emplaceBack(name, scope->sourceLocation(),
                                                              static_cast<int>(relativeIndex));
                auto absoluteIndex = scope->ownRuntimeFunctionIndex(relativeIndex);
                orderedJSScopeExpressionsAbsolute.emplaceBack(name, scope->sourceLocation(),
                                                              static_cast<int>(absoluteIndex));
            };

    const auto populateQmlIRArrays = [&](const QmlIR::Object *irObject, const QString &name,
                                         int relative) {
        orderedQmlIrExpressionsRelative.emplaceBack(name, irObject->location, relative);
        auto absolute = irObject->runtimeFunctionIndices.at(relative);
        orderedQmlIrExpressionsAbsolute.emplaceBack(name, irObject->location, absolute);
    };

    const auto suitableScope = [](const QQmlJSScope::ConstPtr &scope) {
        const auto type = scope->scopeType();
        return type == QQmlSA::ScopeType::QMLScope
                || type == QQmlSA::ScopeType::GroupedPropertyScope
                || type == QQmlSA::ScopeType::AttachedPropertyScope;
    };

    QList<QQmlJSScope::ConstPtr> queue;
    queue.push_back(root);
    while (!queue.isEmpty()) {
        auto current = queue.front();
        queue.pop_front();

        if (suitableScope(current)) {
            const auto methods = current->ownMethods();
            for (const auto &method : methods) {
                if (method.methodType() == QQmlJSMetaMethodType::Signal)
                    continue;
                QString name = method.methodName();
                auto relativeIndex = method.jsFunctionIndex();
                QVERIFY2(static_cast<int>(relativeIndex) >= 0,
                         qPrintable(QStringLiteral("Method %1 from %2 has no index")
                                            .arg(name, getScopeName(current))));

                populateQQmlJSScopeArrays(current, name, relativeIndex);
            }

            const auto bindings = current->ownPropertyBindings();
            for (const auto &binding : bindings) {
                if (binding.bindingType() != QQmlSA::BindingType::Script)
                    continue;
                QString name = binding.propertyName();
                auto relativeIndex = binding.scriptIndex();
                QVERIFY2(static_cast<int>(relativeIndex) >= 0,
                         qPrintable(QStringLiteral("Binding on property %1 from %2 has no index")
                                            .arg(name, getScopeName(current))));

                populateQQmlJSScopeArrays(current, name, relativeIndex);
            }
        }

        const auto children = current->childScopes();
        for (const auto &c : children)
            queue.push_back(c);
    }

    for (const QmlIR::Object *irObject : std::as_const(document.objects)) {
        const QString objectName = document.stringAt(irObject->inheritedTypeNameIndex);
        for (auto it = irObject->functionsBegin(); it != irObject->functionsEnd(); ++it) {
            QString name = document.stringAt(it->nameIndex);
            populateQmlIRArrays(irObject, name, it->index);
        }
        for (auto it = irObject->bindingsBegin(); it != irObject->bindingsEnd(); ++it) {
            if (it->type() != QmlIR::Binding::Type_Script)
                continue;
            QString name = document.stringAt(it->propertyNameIndex);
            int index = it->value.compiledScriptIndex;
            populateQmlIRArrays(irObject, name, index);
        }
    }

    std::sort(orderedJSScopeExpressionsRelative.begin(), orderedJSScopeExpressionsRelative.end());
    std::sort(orderedQmlIrExpressionsRelative.begin(), orderedQmlIrExpressionsRelative.end());

    std::sort(orderedJSScopeExpressionsAbsolute.begin(), orderedJSScopeExpressionsAbsolute.end());
    std::sort(orderedQmlIrExpressionsAbsolute.begin(), orderedQmlIrExpressionsAbsolute.end());
    QCOMPARE(orderedJSScopeExpressionsRelative, orderedQmlIrExpressionsRelative);
    QCOMPARE(orderedJSScopeExpressionsAbsolute, orderedQmlIrExpressionsAbsolute);
}

void tst_qqmljsscope::extensions()
{
    QQmlJSScope::ConstPtr root = run(u"extensions.qml"_s);
    QVERIFY(root);
    QVERIFY(root->isFullyResolved());

    const auto childScopes = root->childScopes();
    QCOMPARE(childScopes.size(), 5);

    QCOMPARE(childScopes[0]->baseTypeName(), u"Extended"_s);
    QCOMPARE(childScopes[1]->baseTypeName(), u"ExtendedIndirect"_s);
    QCOMPARE(childScopes[2]->baseTypeName(), u"ExtendedTwice"_s);
    QCOMPARE(childScopes[3]->baseTypeName(), u"NamespaceExtended"_s);
    QCOMPARE(childScopes[4]->baseTypeName(), u"NonNamespaceExtended"_s);
    QVERIFY(childScopes[0]->isFullyResolved());
    QVERIFY(childScopes[1]->isFullyResolved());
    QVERIFY(childScopes[2]->isFullyResolved());
    QVERIFY(childScopes[3]->isFullyResolved());
    QVERIFY(childScopes[4]->isFullyResolved());

    QCOMPARE(childScopes[0]->property(u"count"_s).typeName(), u"int"_s);
    QCOMPARE(childScopes[1]->property(u"count"_s).typeName(), u"double"_s);
    QCOMPARE(childScopes[2]->property(u"count"_s).typeName(), u"int"_s);
    QCOMPARE(childScopes[2]->property(u"str"_s).typeName(), u"QString"_s);

    QVERIFY(!childScopes[3]->hasProperty(u"count"_s));
    QVERIFY(!childScopes[3]->property(u"count"_s).isValid());
    QVERIFY(!childScopes[3]->hasProperty(u"p"_s));
    QVERIFY(!childScopes[3]->property(u"p"_s).isValid());
    QVERIFY(!childScopes[3]->hasMethod(u"someMethod"_s));
    QVERIFY(childScopes[3]->hasEnumeration(u"ExtensionEnum"_s));
    QVERIFY(childScopes[3]->hasEnumerationKey(u"Value1"_s));
    QVERIFY(childScopes[3]->enumeration(u"ExtensionEnum"_s).isValid());
    QCOMPARE(childScopes[3]->defaultPropertyName(), u"objectName"_s);
    QCOMPARE(childScopes[3]->parentPropertyName(), u"p"_s);
    QVERIFY(!childScopes[3]->hasInterface(u"QQmlParserStatus"_s));
    QCOMPARE(childScopes[3]->attachedTypeName(), QString());
    QVERIFY(!childScopes[3]->attachedType());

    QVERIFY(!childScopes[3]->extensionIsNamespace());
    QVERIFY(childScopes[3]->baseType()->extensionIsNamespace());

    QVERIFY(childScopes[4]->hasProperty(u"count"_s));
    QVERIFY(childScopes[4]->property(u"count"_s).isValid());
    QVERIFY(childScopes[4]->hasProperty(u"p"_s));
    QVERIFY(childScopes[4]->property(u"p"_s).isValid());
    QVERIFY(childScopes[4]->hasMethod(u"someMethod"_s));
    QVERIFY(childScopes[4]->hasEnumeration(u"ExtensionEnum"_s));
    QVERIFY(childScopes[4]->hasEnumerationKey(u"Value1"_s));
    QVERIFY(childScopes[4]->enumeration(u"ExtensionEnum"_s).isValid());
    QCOMPARE(childScopes[4]->defaultPropertyName(), u"objectName"_s);
    QCOMPARE(childScopes[4]->parentPropertyName(), u"p"_s);
    QVERIFY(!childScopes[4]->hasInterface(u"QQmlParserStatus"_s));
    QCOMPARE(childScopes[4]->attachedTypeName(), QString());
    QVERIFY(!childScopes[4]->attachedType());

    auto [owner, ownerKind] = QQmlJSScope::ownerOfProperty(childScopes[4], u"count"_s);
    QVERIFY(owner);
    QCOMPARE(ownerKind, QQmlJSScope::ExtensionType);
    QCOMPARE(owner, childScopes[4]->baseType()->extensionType().scope);
}

void tst_qqmljsscope::emptyBlockBinding()
{
    QQmlJSScope::ConstPtr root = run(u"emptyBlockBinding.qml"_s);
    QVERIFY(root);
    QVERIFY(root->hasOwnPropertyBindings(u"x"_s));
    QVERIFY(root->hasOwnPropertyBindings(u"y"_s));
}

void tst_qqmljsscope::hasOwnEnumerationKeys()
{
    QQmlJSScope::ConstPtr root = run(u"extensions.qml"_s);
    QVERIFY(root);
    QQmlJSScope::ConstPtr extendedDerived = root->childScopes().front();
    QVERIFY(extendedDerived);
    // test that enumeration keys from base cannot be found
    QVERIFY(!extendedDerived->hasOwnEnumerationKey(u"ThisIsTheEnumFromExtended"_s));
    QVERIFY(!extendedDerived->hasOwnEnumerationKey(u"ThisIsTheFlagFromExtended"_s));

    QQmlJSScope::ConstPtr extended = extendedDerived->baseType();
    QVERIFY(extended);

    QVERIFY(extended->hasOwnEnumerationKey(u"ThisIsTheEnumFromExtended"_s));
    QVERIFY(extended->hasOwnEnumerationKey(u"ThisIsTheFlagFromExtended"_s));
    QVERIFY(!extended->hasOwnEnumerationKey(u"ThisIsTheEnumFromExtension"_s));
    QVERIFY(!extended->hasOwnEnumerationKey(u"ThisIsTheFlagFromExtension"_s));
}

void tst_qqmljsscope::ownModuleName()
{
    const QString moduleName = u"HelloModule"_s;
    QQmlJSScope::ConstPtr root = run(u"ownModuleName.qml"_s);
    QVERIFY(root);
    QCOMPARE(root->moduleName(), moduleName);
    QCOMPARE(root->ownModuleName(), moduleName);

    QCOMPARE(root->childScopes().size(), 2);
    QQmlJSScope::ConstPtr child = root->childScopes().front();
    QVERIFY(child);
    // only root and inline components have own module names, but the child should be able to query
    // its component's module Name via moduleName()
    QCOMPARE(child->ownModuleName(), QString());
    QCOMPARE(child->moduleName(), moduleName);

    QQmlJSScope::ConstPtr ic = root->childScopes()[1];
    QVERIFY(ic);
    QCOMPARE(ic->ownModuleName(), moduleName);
    QCOMPARE(ic->moduleName(), moduleName);

    QQmlJSScope::ConstPtr icChild = ic->childScopes().front();
    QVERIFY(icChild);
    QCOMPARE(icChild->ownModuleName(), QString());
    QCOMPARE(icChild->moduleName(), moduleName);
}

void tst_qqmljsscope::resolvedNonUniqueScopes()
{
    QQmlJSScope::ConstPtr root = run(u"resolvedNonUniqueScope.qml"_s);
    QVERIFY(root);

    const auto value = [](const QMultiHash<QString, QQmlJSMetaPropertyBinding> &bindings,
                          const QString &key) {
        return bindings.value(key, QQmlJSMetaPropertyBinding(QQmlJS::SourceLocation {}));
    };

    {
        auto topLevelBindings = root->propertyBindings(u"Component"_s);
        QCOMPARE(topLevelBindings.size(), 1);
        QCOMPARE(topLevelBindings[0].bindingType(), QQmlSA::BindingType::AttachedProperty);
        auto componentScope = topLevelBindings[0].attachedType();
        auto componentBindings = componentScope->ownPropertyBindings();
        QCOMPARE(componentBindings.size(), 2);
        auto onCompletedBinding = value(componentBindings, u"onCompleted"_s);
        QVERIFY(onCompletedBinding.isValid());
        QCOMPARE(onCompletedBinding.bindingType(), QQmlSA::BindingType::Script);
        QCOMPARE(onCompletedBinding.scriptKind(), QQmlSA::ScriptBindingKind::SignalHandler);
        auto onDestructionBinding = value(componentBindings, u"onDestruction"_s);
        QVERIFY(onDestructionBinding.isValid());
        QCOMPARE(onDestructionBinding.bindingType(), QQmlSA::BindingType::Script);
        QCOMPARE(onDestructionBinding.scriptKind(),
                 QQmlSA::ScriptBindingKind::SignalHandler);
    }

    {
        auto topLevelBindings = root->propertyBindings(u"p"_s);
        QCOMPARE(topLevelBindings.size(), 1);
        QCOMPARE(topLevelBindings[0].bindingType(), QQmlSA::BindingType::GroupProperty);
        auto pScope = topLevelBindings[0].groupType();
        auto pBindings = pScope->ownPropertyBindings();
        QCOMPARE(pBindings.size(), 1);
        auto onXChangedBinding = value(pBindings, u"onXChanged"_s);
        QVERIFY(onXChangedBinding.isValid());
        QCOMPARE(onXChangedBinding.bindingType(), QQmlSA::BindingType::Script);
        QCOMPARE(onXChangedBinding.scriptKind(), QQmlSA::ScriptBindingKind::SignalHandler);
    }
}

static void
getRuntimeInfoFromCompilationUnit(const QV4::CompiledData::Unit *unit,
                                  QList<const QV4::CompiledData::Function *> &runtimeFunctions)
{
    QVERIFY(unit);
    for (uint i = 0; i < unit->functionTableSize; ++i) {
        const QV4::CompiledData::Function *function = unit->functionAt(i);
        QVERIFY(function);
        runtimeFunctions << function;
    }
}

// Note: this test is here because we never explicitly test qCompileQmlFile()
void tst_qqmljsscope::compilationUnitsAreCompatible()
{
    QList<const QV4::CompiledData::Function *> componentFunctions;
    QList<const QV4::CompiledData::Function *> cachegenFunctions;

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl(u"compilationUnitsCompatibility.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQmlComponentPrivate *cPriv = QQmlComponentPrivate::get(&component);
    QVERIFY(cPriv);
    auto unit = cPriv->compilationUnit();
    QVERIFY(unit);
    QVERIFY(unit->unitData());
    getRuntimeInfoFromCompilationUnit(unit->unitData(), componentFunctions);

    if (QTest::currentTestFailed())
        return;

    const QString file = testFile(u"compilationUnitsCompatibility.qml"_s);
    QmlIR::Document document(file, file, false); // we need QmlIR information here
    QVERIFY(run(file, &document));
    QVERIFY(document.javaScriptCompilationUnit->unitData());
    getRuntimeInfoFromCompilationUnit(document.javaScriptCompilationUnit->unitData(),
                                      cachegenFunctions);
    if (QTest::currentTestFailed())
        return;

    QCOMPARE(cachegenFunctions.size(), componentFunctions.size());
    // name index should be fairly unique to distinguish different functions
    // within a document. their order must be the same for both qmlcachegen and
    // qqmltypecompiler (runtime)
    for (qsizetype i = 0; i < cachegenFunctions.size(); ++i)
        QCOMPARE(uint(cachegenFunctions[i]->nameIndex), uint(componentFunctions[i]->nameIndex));
}

void tst_qqmljsscope::attachedTypeResolution_data()
{
    QTest::addColumn<bool>("creatable");
    QTest::addColumn<QString>("moduleName");
    QTest::addColumn<QString>("typeName");
    QTest::addColumn<QString>("attachedTypeName");
    QTest::addColumn<QString>("propertyOnSelf");
    QTest::addColumn<QString>("propertyOnAttached");

    QTest::addRow("ListView") << true
                              << "QtQuick"
                              << "ListView"
                              << "QQuickListViewAttached"
                              << "orientation"
                              << "";
    QTest::addRow("Keys") << false
                          << "QtQuick"
                          << "Keys"
                          << "QQuickKeysAttached"
                          << "priority"
                          << "priority";
}

class TestPass : public QQmlSA::ElementPass
{
public:
    TestPass(QQmlSA::PassManager *manager) : QQmlSA::ElementPass(manager) { }
    bool shouldRun(const QQmlSA::Element &) override { return true; }
    void run(const QQmlSA::Element &) override { }
};

using PassManagerPtr = std::unique_ptr<
        QQmlSA::PassManager, decltype(&QQmlSA::PassManagerPrivate::deletePassManager)>;

void tst_qqmljsscope::attachedTypeResolution()
{
    QFETCH(bool, creatable);
    QFETCH(QString, moduleName);
    QFETCH(QString, typeName);
    QFETCH(QString, attachedTypeName);
    QFETCH(QString, propertyOnSelf);
    QFETCH(QString, propertyOnAttached);

    std::unique_ptr<QQmlJSLogger> logger = std::make_unique<QQmlJSLogger>();
    QFile qmlFile("data/attachedTypeResolution.qml");
    if (!qmlFile.open(QIODevice::ReadOnly | QIODevice::Text))
        QSKIP("Unable to open qml file");

    logger->setCode(qmlFile.readAll());
    logger->setFilePath(QString(qmlFile.filesystemFileName().string().c_str()));
    QQmlJSImporter importer{ { "data" }, nullptr, UseOptionalImports };
    QStringList defaultImportPaths =
            QStringList{ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) };
    importer.setImportPaths(defaultImportPaths);
    QQmlJSTypeResolver resolver(&importer);
    const auto &implicitImportDirectory = QQmlJSImportVisitor::implicitImportDirectory(
            logger->filePath(), importer.resourceFileMapper());
    QQmlJSImportVisitor v{
        QQmlJSScope::create(), &importer, logger.get(), implicitImportDirectory, {}
    };

    PassManagerPtr manager(
            QQmlSA::PassManagerPrivate::createPassManager(&v, &resolver),
            &QQmlSA::PassManagerPrivate::deletePassManager);

    TestPass pass{ manager.get() };
    const auto &resolved = pass.resolveType(moduleName, typeName);

    QVERIFY(!resolved.isNull());
    const auto &attachedType = pass.resolveAttached(moduleName, typeName);
    QVERIFY(!attachedType.isNull());
    QCOMPARE(attachedType.name(), attachedTypeName);

    if (propertyOnAttached != "") {
        QEXPECT_FAIL("Keys", "Keys and QQuickKeysAttached have the same properties", Continue);
        QVERIFY(!resolved.hasProperty(propertyOnAttached));
        QVERIFY(attachedType.hasProperty(propertyOnAttached));
    }
    if (propertyOnSelf != "") {
        QEXPECT_FAIL("Keys", "Keys and QQuickKeysAttached have the same properties", Continue);
        QVERIFY(!attachedType.hasProperty(propertyOnSelf));
    }

    if (creatable)
        QVERIFY(resolved.hasProperty(propertyOnSelf));
}



void tst_qqmljsscope::builtinTypeResolution_data()
{
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QString>("typeName");

    QTest::addRow("global_QtObject") << true  << "Qt";
    QTest::addRow("function")        << true  << "function";
    QTest::addRow("Array")           << true  << "Array";
    QTest::addRow("invalid")         << false << "foobar";
    QTest::addRow("Number")          << true  << "Number";
    QTest::addRow("bool")            << true  << "bool";
    QTest::addRow("QString")         << true  << "QString";
}

void tst_qqmljsscope::builtinTypeResolution()
{
    QFETCH(bool, valid);
    QFETCH(QString, typeName);

    QQmlJSImporter importer{ { "data" }, nullptr, UseOptionalImports };
    QStringList defaultImportPaths =
            QStringList{ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) };
    importer.setImportPaths(defaultImportPaths);
    QQmlJSTypeResolver resolver(&importer);
    const auto &implicitImportDirectory = QQmlJSImportVisitor::implicitImportDirectory({}, nullptr);
    QQmlJSLogger logger;
    QQmlJSImportVisitor v{
        QQmlJSScope::create(), &importer, &logger, implicitImportDirectory, {}
    };

    PassManagerPtr manager(
            QQmlSA::PassManagerPrivate::createPassManager(&v, &resolver),
            &QQmlSA::PassManagerPrivate::deletePassManager);

    TestPass pass{ manager.get() };
    auto element = pass.resolveBuiltinType(typeName);
    QCOMPARE(element.isNull(), !valid);
}

void tst_qqmljsscope::methodAndSignalSourceLocation()
{
    auto jsscope = run(u"methodAndSignalSourceLocation.qml"_s, false);

    std::array<std::array<int, 9>, 2> offsetsByLineEnding = {
        std::array{ 29, 51, 74, 102, 128, 160, 219, 235, 257 }, // 1 char line endings
        std::array{ 32, 55, 79, 108, 135, 168, 231, 248, 271 }  // 2 char line endinds
    };

    // Try to detect the size of line endings as they lead to different source locations
    auto offset1 = jsscope->methods("f1")[0].sourceLocation().offset;
    QVERIFY(offset1 == 29 || offset1 == 32);
    bool oneCharEndings = offset1 == 29;
    std::array<int, 9> &offsets = oneCharEndings ? offsetsByLineEnding[0] : offsetsByLineEnding[1];

    using namespace QQmlJS;
    QCOMPARE(jsscope->methods("f1")[0].sourceLocation(), SourceLocation(offsets[0], 17, 4, 5));
    QCOMPARE(jsscope->methods("f2")[0].sourceLocation(), SourceLocation(offsets[1], 18, 5, 5));
    QCOMPARE(jsscope->methods("f3")[0].sourceLocation(), SourceLocation(offsets[2], 23, 6, 5));
    QCOMPARE(jsscope->methods("f4")[0].sourceLocation(), SourceLocation(offsets[3], 21, 7, 5));
    QCOMPARE(jsscope->methods("f5")[0].sourceLocation(), SourceLocation(offsets[4], 27, 8, 5));
    QCOMPARE(jsscope->methods("f6")[0].sourceLocation(), SourceLocation(offsets[5], oneCharEndings ? 53 : 55, 9, 5));

    QCOMPARE(jsscope->methods("s1")[0].sourceLocation(), SourceLocation(offsets[6], 11, 13, 5));
    QCOMPARE(jsscope->methods("s2")[0].sourceLocation(), SourceLocation(offsets[7], 17, 14, 5));
    QCOMPARE(jsscope->methods("s3")[0].sourceLocation(), SourceLocation(offsets[8], 28, 15, 5));
}

void tst_qqmljsscope::modulePrefixes()
{
    const auto url = testFile("modulePrefixes.qml");
    const QString sourceCode = loadFile(url);
    QQmlJSLogger logger;
    logger.setFilePath(url);
    logger.setCode(sourceCode);

    QQmlJSScope::Ptr target = QQmlJSScope::create();
    QmlIR::Document document(url, url, false);
    QQmlJSSaveFunction noop([](auto &&...) { return true; });
    QQmlJSCompileError error;
    [&]() {
        QVERIFY2(qCompileQmlFile(document, url, noop, nullptr, &error), qPrintable(error.message));
    }();
    if (!error.message.isEmpty())
        return;

    QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
    QQmlJSTypeResolver typeResolver{ &m_importer };
    typeResolver.init(&visitor, document.program);

    const auto prefixes = typeResolver.seenModuleQualifiers();
    QVERIFY(prefixes.contains("QML"_L1));
    QVERIFY(prefixes.contains("CD"_L1));
    QVERIFY(prefixes.contains("QQ"_L1));
}

void tst_qqmljsscope::javaScriptBuiltinFlag()
{
    const auto url = testFile("ComponentType.qml");
    QQmlJSLogger logger;
    logger.setFilePath(url);
    logger.setCode(loadFile(url));

    QQmlJSScope::Ptr target = QQmlJSScope::create();
    QmlIR::Document document(url, url, false);
    QQmlJSSaveFunction noop([](auto &&...) { return true; });
    QQmlJSCompileError error;
    [&]() {
        QVERIFY2(qCompileQmlFile(document, url, noop, nullptr, &error), qPrintable(error.message));
    }();
    if (!error.message.isEmpty())
        return;

    QQmlJSImportVisitor visitor(target, &m_importer, &logger, dataDirectory());
    QQmlJSTypeResolver typeResolver{ &m_importer };
    typeResolver.init(&visitor, document.program);


    QVERIFY(typeResolver.mathObject()->isJavaScriptBuiltin()); // JS
    QVERIFY(!typeResolver.typeForName("ComponentType")->isJavaScriptBuiltin()); // QML
    QVERIFY(!typeResolver.varType()->isJavaScriptBuiltin()); // C++
}

void tst_qqmljsscope::isRoot()
{
    auto jsscope = run(u"isRoot.qml"_s, false);

    QVERIFY(jsscope->isFileRootComponent());
    QVERIFY(jsscope->property(u"isRoot"_s).isValid());

    const auto children = jsscope->childScopes();
    QCOMPARE(children.size(), 2);
    for (const auto &child : children) {
        QVERIFY(!child->isFileRootComponent());
        QVERIFY(child->property(u"isNotRoot"_s).isValid());
    }
}

void tst_qqmljsscope::idLocation()
{
    auto jsscope = run(u"idSourceLocation.qml"_s, false);
    const QQmlJS::SourceLocation actual = jsscope->idSourceLocation();
    QVERIFY(actual.isValid());
    QCOMPARE(actual.startLine, 7);
    QCOMPARE(actual.startColumn, 25);
    QCOMPARE(actual.length, 10);
}

QTEST_MAIN(tst_qqmljsscope)
#include "tst_qqmljsscope.moc"
