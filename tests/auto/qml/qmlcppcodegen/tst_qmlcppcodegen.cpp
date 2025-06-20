// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <data/birthdayparty.h>
#include <data/cppbaseclass.h>
#include <data/druggeljug.h>
#include <data/enumProperty.h>
#include <data/enumproblems.h>
#include <data/getOptionalLookup.h>
#include <data/listprovider.h>
#include <data/objectwithmethod.h>
#include <data/qmlusing.h>
#include <data/resettable.h>
#include <data/takenumber.h>
#include <data/weathermoduleurl.h>
#include <data/withlength.h>

#include <private/qqmlbind_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlpropertycachecreator_p.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QtGui/qcolor.h>
#include <QtGui/qfont.h>
#include <QtGui/qpa/qplatformdialoghelper.h>

#if QT_CONFIG(process)
#include <QtCore/qprocess.h>
#endif

using namespace Qt::StringLiterals;

Q_IMPORT_QML_PLUGIN(TestTypesPlugin)
Q_IMPORT_QML_PLUGIN(ConfusedPlugin)

class tst_QmlCppCodegen : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void accessModelMethodFromOutSide();
    void aliasLookup();
    void ambiguousAs();
    void ambiguousSignals();
    void anchorsFill();
    void anonymousComposite();
    void argumentConversion();
    void array();
    void arrayCtor();
    void asCast();
    void attachedBaseEnum();
    void attachedSelf();
    void attachedType();
    void badSequence();
    void basicBlocksWithBackJump();
    void basicBlocksWithBackJump_infinite();
    void basicDTZ();
    void bindToValueType();
    void bindingExpression();
    void bindingToScriptStringProperty();
    void blockComments();
    void boolCoercions();
    void boolPointerMerge();
    void brokenAs();
    void boundComponents();
    void callContextPropertyLookupResult();
    void callObjectLookupOnNull();
    void callWithSpread();
    void colorAsVariant();
    void colorString();
    void compareOriginals();
    void comparisonTypes();
    void componentReturnType();
    void compositeSingleton();
    void compositeTypeMethod();
    void confusedModule();
    void consoleObject();
    void consoleTrace();
    void construct();
    void contextParam();
    void conversionDecrement();
    void conversionInDeadCode();
    void conversions();
    void convertPrimitiveToVar();
    void convertQJSPrimitiveValueToIntegral();
    void convertToOriginalReadAcumulatorForUnaryOperators();
    void cppMethodListReturnType();
    void cppValueTypeList();
    void dateConstruction();
    void dateConversions();
    void deadShoeSize();
    void destroyAndToString();
    void detachOnAssignment();
    void dialogButtonBox();
    void enumConversion();
    void enumFromBadSingleton();
    void enumLookup();
    void enumMarkedAsFlag();
    void enumProblems();
    void enumScope();
    void enums();
    void enforceSignature();
    void enumsInOtherObject();
    void equalityQObjects();
    void equalityQUrl();
    void equalityTestsWithNullOrUndefined();
    void equalityVarAndNonStorable();
    void equalityVarAndStorable();
    void equalityVarWithOutConversion();
    void equalsUndefined();
    void evadingAmbiguity();
    void exceptionFromInner();
    void excessiveParameters();
    void extendedTypes();
    void failures();
    void fallbackLookups();
    void fileImportsContainCxxTypes();
    void finalProperty();
    void flagEnum();
    void flushBeforeCapture();
    void fromBoolValue();
    void funcWithParams();
    void functionArguments();
    void functionCallOnNamespaced();
    void functionLookup();
    void functionReturningVoid();
    void functionTakingVar();
    void getLookupOfScript();
    void getOptionalLookup();
    void getOptionalLookup_data();
    void getOptionalLookupNonVoidableBase();
    void getOptionalLookupOnQJSValueNonStrict();
    void getOptionalLookupShadowed();
    void globals();
    void idAccess();
    void ignoredFunctionReturn();
    void importsFromImportPath();
    void inPlaceDecrement();
    void inaccessibleProperty();
    void indirectlyShadowable();
    void infinities();
    void infinitiesToInt();
    void innerObjectNonShadowable();
    void insertContextOnInvalidType();
    void intEnumCompare();
    void intOverflow();
    void intToEnum();
    void interceptor();
    void interestingFiles();
    void interestingFiles_data();
    void internalConversion();
    void invalidPropertyType();
    void invalidateCompositeType();
    void invisibleBase();
    void invisibleListElementType();
    void invisibleSingleton();
    void invisibleTypes();
    void iterateUnknownValue();
    void iteration();
    void javaScriptArgument();
    void jsArrayMethods();
    void jsArrayMethodsWithParams();
    void jsArrayMethodsWithParams_data();
    void jsImport();
    void jsMathObject();
    void jsmoduleImport();
    void jsonArrayToStringList();
    void lengthAccessArraySequenceCompat();
    void letAndConst();
    void listAsArgument();
    void listConversion();
    void listIndices();
    void listLength();
    void listOfInvisible();
    void listPropertyAsModel();
    void listToString();
    void lotsOfRegisters();
    void math();
    void mathMinMax();
    void mathOperations();
    void mathStaticProperties();
    void mergedObjectReadWrite();
    void methodOnListLookup();
    void methods();
    void modulePrefix();
    void multiDirectory_data();
    void multiDirectory();
    void multiForeign();
    void multiLookup();
    void multiRedirect();
    void multipleCtors();
    void namespaceWithEnum();
    void noBuiltinsImport();
    void noQQmlData();
    void nonNotifyable();
    void notEqualsInt();
    void notNotString();
    void nullAccess();
    void nullAccessInsideSignalHandler();
    void nullComparison();
    void nullishCoalescing();
    void nullishCoalescing_data();
    void numbersInJsPrimitive();
    void objectInVar();
    void objectLookupOnListElement();
    void objectToString();
    void objectWithStringListMethod();
    void onAssignment();
    void optionalComparison();
    void outOfBoundsArray();
    void overriddenProperty();
    void ownPropertiesNonShadowable();
    void parentProperty();
    void popContextAfterRet();
    void prefixedType();
    void propertyOfParent();
    void qmlUsing();
    void qtfont();
    void reduceWithNullThis();
    void readEnumFromInstance();
    void readonlyListProperty();
    void registerElimination();
    void registerPropagation();
    void renameAdjust();

    void resettableProperty();
    void resettableProperty_data();

    void returnAfterReject();
    void revisions();
    void scopeIdLookup();
    void scopeObjectDestruction();
    void scopeVsObject();
    void scopedEnum();
    void sequenceToIterable();
    void setLookupConversion();
    void setLookupOriginalScope();
    void shadowedAsCasts();
    void shadowedMethod();
    void shadowedPrimitiveCmpEqNull();
    void shifts();
    void signalHandler();
    void signalIndexMismatch();
    void signalsWithLists();
    void signatureIgnored();
    void simpleBinding();
    void storeElementSideEffects();
    void storeMetaEnum();
    void stringArg();
    void stringLength();
    void stringToByteArray();
    void structuredValueType();
    void takeNumbers();
    void takeNumbers_data();
    void testIsnan();
    void thisObject();
    void throwObjectName();
    void topLevelComponent();
    void translation();
    void trigraphs();
    void trivialSignalHandler();
    void typePropagationLoop();
    void typePropertyClash();
    void typedArray();
    void unclearComponentBoundaries();
    void undefinedResets();
    void undefinedToDouble();
    void unknownAttached();
    void unknownParameter();
    void unknownUnderlyingType();
    void unstoredUndefined();
    void unusedAttached();
    void urlString();
    void valueTypeArgument();
    void valueTypeBehavior();
    void valueTypeLists();
    void valueTypeProperty();
    void variantMapLookup();
    void variantReturn();
    void variantlist();
    void variantMap();
    void voidConversion();
    void voidFunction();
    void writeBack();
    void writeVariantMap();
};

static QByteArray arg1()
{
    const QStringList args = QCoreApplication::instance()->arguments();
    return args.size() > 1 ? args[1].toUtf8() : QByteArray("undefined");
}

namespace QmlCacheGeneratedCode {
namespace _qt_qml_TestTypes_failures_qml {
extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
}
}

static void checkColorProperties(QQmlComponent *component)
{
    QVERIFY2(component->isReady(), qPrintable(component->errorString()));
    QScopedPointer<QObject> rootObject(component->create());
    QVERIFY(rootObject);

    const QMetaObject *mo = QMetaType::fromName("QQuickIcon").metaObject();
    QVERIFY(mo != nullptr);

    const QMetaProperty prop = mo->property(mo->indexOfProperty("color"));
    QVERIFY(prop.isValid());

    const QVariant a = rootObject->property("a");
    QVERIFY(a.isValid());

    const QVariant iconColor = prop.readOnGadget(rootObject->property("icon").data());
    QVERIFY(iconColor.isValid());

    const QMetaType colorType = QMetaType::fromName("QColor");
    QVERIFY(colorType.isValid());

    QCOMPARE(a.metaType(), colorType);
    QCOMPARE(iconColor.metaType(), colorType);

    QCOMPARE(iconColor, a);
}

static const double numbers[] = {
    qQNaN(), -qInf(),
    std::numeric_limits<double>::min(),
    std::numeric_limits<float>::min(),
    std::numeric_limits<qint32>::min(),
    -1000.2, -100, -2, -1.333, -1, -0.84, -0.5,

    // -0 and 0 are not different on the QML side. Therefore, don't keep them adjacent.
    // Otherwise the bindings won't get re-evaluated.
    std::copysign(0.0, -1), 1, 0.0,

    0.5, 0.77, 1.4545, 2, 199, 2002.13,
    std::numeric_limits<qint32>::max(),
    std::numeric_limits<quint32>::max(),
    std::numeric_limits<float>::max(),
    std::numeric_limits<double>::max(),
    qInf()
};

class MyCppType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool useListDelegate
                       READ useListDelegate
                       WRITE setUseListDelegate
                       NOTIFY useListDelegateChanged)
public:
    explicit MyCppType(QObject * parent = nullptr) : QObject(parent) {}

    bool useListDelegate() const { return m_useListDelegate; }
    void setUseListDelegate(bool useListDelegate)
    {
        if (useListDelegate != m_useListDelegate) {
            m_useListDelegate = useListDelegate;
            emit useListDelegateChanged();
        }
    }

signals:
    void useListDelegateChanged();

private:
    bool m_useListDelegate = false;
};

class InvisibleListElementType : public QObject
{
    Q_OBJECT
public:
    InvisibleListElementType(QObject *parent = nullptr) : QObject(parent) {}
};

template<typename T>
QString toOperand(double arg);

template<>
QString toOperand<double>(double arg)
{
    if (qIsNull(arg))
        return std::signbit(arg) ? QStringLiteral("(-0)") : QStringLiteral("(0)");

    return u'(' + QJSPrimitiveValue(arg).toString() + u')';
}

template<>
QString toOperand<int>(double arg)
{
    const int iArg = QJSPrimitiveValue(arg).toInteger();
    return u'(' + QJSPrimitiveValue(iArg).toString() + u')';
}

template<>
QString toOperand<bool>(double arg)
{
    const bool bArg = QJSPrimitiveValue(arg).toBoolean();
    return u'(' + QJSPrimitiveValue(bArg).toString() + u')';
}

template<typename T1, typename T2>
double jsEval(double arg1, double arg2, const QString &op, QJSEngine *engine)
{
    auto evalBinary = [&](const QString &jsOp) {
        return engine->evaluate(toOperand<T1>(arg1) + jsOp + toOperand<T2>(arg2)).toNumber();
    };

    auto evalBinaryConst = [&](const QString &jsOp) {
        return engine->evaluate(toOperand<T1>(arg1) + jsOp + u'9').toNumber();
    };

    auto evalUnary = [&](const QString &jsOp) {
        return engine->evaluate(jsOp + toOperand<T1>(arg1)).toNumber();
    };

    auto evalInPlace = [&](const QString &jsOp) {
        return engine->evaluate(
                    u"(function() {var a = "_s + toOperand<T1>(arg1)+ u"; return "_s
                    + jsOp + u"a;})()"_s).toNumber();
    };

    if (op == u"unot")
        return evalUnary(u"!"_s);
    if (op == u"uplus")
        return evalUnary(u"+"_s);
    if (op == u"uminus")
        return evalUnary(u"-"_s);
    if (op == u"ucompl")
        return evalUnary(u"~"_s);

    if (op == u"increment")
        return evalInPlace(u"++"_s);
    if (op == u"decrement")
        return evalInPlace(u"--"_s);

    if (op == u"add")
        return evalBinary(u"+"_s);
    if (op == u"sub")
        return evalBinary(u"-"_s);
    if (op == u"mul")
        return evalBinary(u"*"_s);
    if (op == u"div")
        return evalBinary(u"/"_s);
    if (op == u"exp")
        return evalBinary(u"**"_s);
    if (op == u"mod")
        return evalBinary(u"%"_s);

    if (op == u"bitAnd")
        return evalBinary(u"&"_s);
    if (op == u"bitOr")
        return evalBinary(u"|"_s);
    if (op == u"bitXor")
        return evalBinary(u"^"_s);

    if (op == u"bitAndConst")
        return evalBinaryConst(u"&"_s);
    if (op == u"bitOrConst")
        return evalBinaryConst(u"|"_s);
    if (op == u"bitXorConst")
        return evalBinaryConst(u"^"_s);

    if (op == u"ushr")
        return evalBinary(u">>>"_s);
    if (op == u"shr")
        return evalBinary(u">>"_s);
    if (op == u"shl")
        return evalBinary(u"<<"_s);

    if (op == u"ushrConst")
        return evalBinaryConst(u">>>"_s);
    if (op == u"shrConst")
        return evalBinaryConst(u">>"_s);
    if (op == u"shlConst")
        return evalBinaryConst(u"<<"_s);

    qDebug() << op;
    Q_UNREACHABLE_RETURN(0);
}

static QList<QString> convertToStrings(const QList<qint64> &ints)
{
    QList<QString> strings;
    for (qint64 i : ints)
        strings.append(QString::number(i));
    return strings;
}

static QRegularExpression bindingLoopMessage(const QUrl &url, char var)
{
    // The actual string depends on how many times QObject* was registered with what parameters.
    return QRegularExpression(
                "%1:4:1: QML [^:]+: Binding loop detected for property \"%2\""_L1
                .arg(url.toString()).arg(QLatin1Char(var)));
}

static void listsEqual(QObject *listProp, QObject *array, const char *method)
{
    const QByteArray listPropertyPropertyName = QByteArray("listProperty") + method;
    const QByteArray jsArrayPropertyName = QByteArray("jsArray") + method;

    const QQmlListReference listPropertyProperty(listProp, listPropertyPropertyName.constData());
    const QVariantList jsArrayProperty = array->property(jsArrayPropertyName.constData()).toList();

    const qsizetype listPropertyCount = listPropertyProperty.count();
    QCOMPARE(listPropertyCount, jsArrayProperty.count());

    for (qsizetype i = 0; i < listPropertyCount; ++i)
        QCOMPARE(listPropertyProperty.at(i), jsArrayProperty.at(i).value<QObject *>());
}

void tst_QmlCppCodegen::initTestCase()
{
#ifdef QT_TEST_FORCE_INTERPRETER
    qputenv("QV4_FORCE_INTERPRETER", "1");
#endif
}

void tst_QmlCppCodegen::cleanupTestCase()
{
    // This code checks for basic blocks validation failures in the tests
    QStringList expectedFailures = {
        u"codegen_test_module_basicBlocksWithBackJump_infinite_qml.cpp"_s,
        u"codegen_test_module_verify_basicBlocksWithBackJump_infinite_qml.cpp"_s,
    };

    QString generatedCppFolder = QStringLiteral(GENERATED_CPP_FOLDER);
    QDirIterator dirIterator(generatedCppFolder, { u"*.cpp"_s }, QDir::Files);
    while (dirIterator.hasNext()) {
        QFile file(dirIterator.next());
        if (!file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
            qDebug() << "Couldn't open generated file";
            continue;
        }

        const auto content = file.readAll();
        if (bool validationFailed = content.contains("// QV4_BASIC_BLOCK_VALIDATION_FAILED:"_L1)) {
            if (expectedFailures.contains(dirIterator.fileInfo().fileName())) {
                QEXPECT_FAIL("", "Expected failure", Continue);
            }
            const auto message = file.fileName() + u": Basic blocks validation failed."_s;
            QVERIFY2(!validationFailed, message.toStdString().c_str());
        }
    }
}

void tst_QmlCppCodegen::accessModelMethodFromOutSide()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/AccessModelMethodsFromOutside.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtDebugMsg, "3");
    QTest::ignoreMessage(QtDebugMsg, "Apple");
    QScopedPointer<QObject> object(component.create());

    QCOMPARE(object->property("cost1").toDouble(), 3);
    QCOMPARE(object->property("name1").toString(), u"Orange"_s);
    QCOMPARE(object->property("cost2").toDouble(), 1.95);
    QCOMPARE(object->property("name2").toString(), u"Banana"_s);
}

void tst_QmlCppCodegen::aliasLookup()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/aliasLookup.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const QVariant t = object->property("t");
    QCOMPARE(t.metaType(), QMetaType::fromType<QString>());
    QCOMPARE(t.toString(), u"12"_s);
}

void tst_QmlCppCodegen::ambiguousAs()
{
    QQmlEngine e;
    const QUrl url(u"qrc:/qt/qml/TestTypes/ambiguousAs.qml"_s);
    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("other").value<QObject *>(), o.data());
    o->setProperty("useSelf", QVariant::fromValue(false));
    QCOMPARE(o->property("other").value<QObject *>(), nullptr);
}

void tst_QmlCppCodegen::ambiguousSignals()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ambiguousSignals.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"tomorrow"_s);
    Person *p = qobject_cast<Person *>(o.data());
    QVERIFY(p);
    emit p->ambiguous(12);
    QCOMPARE(o->objectName(), u"12foo"_s);
    emit p->ambiguous();
    QCOMPARE(o->objectName(), u"9foo"_s);
}

void tst_QmlCppCodegen::anchorsFill()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/anchorsFill.qml"_s));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), component.errorString().toUtf8().constData());

    QCOMPARE(object->property("width").toInt(), 234);
    QCOMPARE(object->children().size(), 2);

    QObject *child = object->children().front();
    QVERIFY(child);
    QCOMPARE(child->property("width").toInt(), 234);

    QObject *newParent = object->children().back();
    QVERIFY(newParent);
    QCOMPARE(newParent->property("width").toInt(), 47);

    child->setProperty("parent", QVariant::fromValue(newParent));
    QCOMPARE(child->property("width").toInt(), 47);
}

void tst_QmlCppCodegen::anonymousComposite()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/anonymousCompositeUser.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("b").toInt(), 5);
}

void tst_QmlCppCodegen::argumentConversion()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/argumentConversion.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    auto checkNaN = [&](const char *propName) {
        const QVariant prop = o->property(propName);
        QCOMPARE(prop.metaType(), QMetaType::fromType<double>());
        QVERIFY(qIsNaN(prop.toDouble()));
    };

    checkNaN("a");
    checkNaN("b");
    checkNaN("e");

    QCOMPARE(o->property("c").toDouble(), 3.0);
    QCOMPARE(o->property("d").toDouble(), -1.0);
    QCOMPARE(o->property("f").toDouble(), 10.0);
}

void tst_QmlCppCodegen::array()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/array.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    const QJSValue value1 = object->property("values1").value<QJSValue>();
    QVERIFY(value1.isArray());
    QCOMPARE(value1.property(u"length"_s).toInt(), 3);
    QCOMPARE(value1.property(0).toInt(), 1);
    QCOMPARE(value1.property(1).toInt(), 2);
    QCOMPARE(value1.property(2).toInt(), 3);

    const QJSValue value2 = object->property("values2").value<QJSValue>();
    QVERIFY(value2.isArray());
    QCOMPARE(value2.property(u"length"_s).toInt(), 0);
}

void tst_QmlCppCodegen::arrayCtor()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/arrayCtor.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("defaultCtor"), QVariant::fromValue(QList<int>()));
    QCOMPARE(object->property("oneArgCtor"), QVariant::fromValue(QList<int>(5)));
    QCOMPARE(object->property("multiArgCtor"), QVariant::fromValue(QList<int>({2, 3, 3, 4})));
    QCOMPARE(object->property("arrayTrue"), QVariant::fromValue(QList<bool>({true})));
    QCOMPARE(object->property("arrayFalse"), QVariant::fromValue(QList<bool>({false})));
    QCOMPARE(object->property("arrayNegative"), QVariant::fromValue(QList<double>()));
}

void tst_QmlCppCodegen::asCast()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/asCast.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QQmlContext *context = qmlContext(root.data());
    const QObject *object = context->objectForName(u"object"_s);
    const QObject *item = context->objectForName(u"item"_s);
    const QObject *rectangle = context->objectForName(u"rectangle"_s);
    const QObject *dummy = context->objectForName(u"dummy"_s);

    QCOMPARE(qvariant_cast<QObject *>(root->property("objectAsObject")), object);
    QCOMPARE(qvariant_cast<QObject *>(root->property("objectAsItem")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("objectAsRectangle")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("objectAsDummy")), nullptr);

    QCOMPARE(qvariant_cast<QObject *>(root->property("itemAsObject")), item);
    QCOMPARE(qvariant_cast<QObject *>(root->property("itemAsItem")), item);
    QCOMPARE(qvariant_cast<QObject *>(root->property("itemAsRectangle")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("itemAsDummy")), nullptr);

    QCOMPARE(qvariant_cast<QObject *>(root->property("rectangleAsObject")), rectangle);
    QCOMPARE(qvariant_cast<QObject *>(root->property("rectangleAsItem")), rectangle);
    QCOMPARE(qvariant_cast<QObject *>(root->property("rectangleAsRectangle")), rectangle);
    QCOMPARE(qvariant_cast<QObject *>(root->property("rectangleAsDummy")), nullptr);

    QCOMPARE(qvariant_cast<QObject *>(root->property("dummyAsObject")), dummy);
    QCOMPARE(qvariant_cast<QObject *>(root->property("dummyAsItem")), dummy);
    QCOMPARE(qvariant_cast<QObject *>(root->property("dummyAsRectangle")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("dummyAsDummy")), dummy);

    QCOMPARE(qvariant_cast<QObject *>(root->property("nullAsObject")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("nullAsItem")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("nullAsRectangle")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("nullAsDummy")), nullptr);

    QCOMPARE(qvariant_cast<QObject *>(root->property("undefinedAsObject")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("undefinedAsItem")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("undefinedAsRectangle")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(root->property("undefinedAsDummy")), nullptr);

    QCOMPARE(root->property("stringAsString"), u"a"_s);
    QCOMPARE(root->property("urlAsUrl"), QUrl(u"http://example.com"_s));
    QCOMPARE(root->property("dateAsDate"), QDateTime(QDate(1996, 3, 3), QTime()));
}

void tst_QmlCppCodegen::attachedBaseEnum()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/attachedBaseEnum.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QObject *drag = qvariant_cast<QObject *>(object->property("drag"));
    QVERIFY(drag);

    // Drag.YAxis is 2, but we cannot #include it here.
    bool ok = false;
    QCOMPARE(drag->property("axis").toInt(&ok), 2);
    QVERIFY(ok);
}

void tst_QmlCppCodegen::attachedSelf()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/SelectionRectangle.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QObject *handle = qvariant_cast<QObject *>(o->property("aa"));
    QVERIFY(handle);
    QVERIFY(qvariant_cast<QObject *>(handle->property("rect")) != nullptr);
}

void tst_QmlCppCodegen::attachedType()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/text.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("dayz").toDateTime(), QDateTime(QDate(1911, 3, 4), QTime()));
    QCOMPARE(object->property("oParty").toDateTime(), QDateTime(QDate(1911, 3, 4), QTime()));

    QObject *party = qvariant_cast<QObject *>(object->property("party"));
    QVERIFY(party);
    QCOMPARE(party->property("eee").toInt(), 21);
    QCOMPARE(party->property("fff").toInt(), 33);
    QCOMPARE(object->property("ggg").toInt(), 37);
}

void tst_QmlCppCodegen::badSequence()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/badSequence.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    Person *self = qobject_cast<Person *>(o.data());
    QVERIFY(self);
    QVERIFY(self->barzles().isEmpty());
    QVERIFY(self->cousins().isEmpty());

    Person *other = o->property("other").value<Person *>();
    QVERIFY(other);

    QVERIFY(other->barzles().isEmpty());
    QVERIFY(other->cousins().isEmpty());

    Barzle f1;
    Barzle f2;
    const QList<Barzle *> barzles { &f1, &f2 };
    const QList<Person *> cousins { self, other };

    other->setBarzles(barzles);
    QCOMPARE(self->barzles(), barzles);
    QCOMPARE(self->property("l").toInt(), 2);

    other->setCousins(cousins);
    QCOMPARE(self->cousins(), cousins);
    QCOMPARE(self->property("m").toInt(), 2);

    QQmlListProperty<Person> others
            = self->property("others").value<QQmlListProperty<Person>>();
    QCOMPARE(others.count(&others), 2);
    QCOMPARE(others.at(&others, 0), cousins[0]);
    QCOMPARE(others.at(&others, 1), cousins[1]);

    QQmlListProperty<Person> momsCousins
            = self->property("momsCousins").value<QQmlListProperty<Person>>();
    QCOMPARE(momsCousins.count(&momsCousins), 2);
    QCOMPARE(momsCousins.at(&momsCousins, 0), cousins[0]);
    QCOMPARE(momsCousins.at(&momsCousins, 1), cousins[1]);

    QQmlListProperty<Person> dadsCousins
            = self->property("dadsCousins").value<QQmlListProperty<Person>>();
    QCOMPARE(dadsCousins.count(&dadsCousins), 1);
    QCOMPARE(dadsCousins.at(&dadsCousins, 0), other);
}

static bool expectingMessage = false;
static void handler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    QVERIFY(expectingMessage);
    QCOMPARE(type, QtDebugMsg);
    QCOMPARE(message, u"false");
    expectingMessage = false;
}

void tst_QmlCppCodegen::basicBlocksWithBackJump()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/basicBlocksWithBackJump.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    const auto oldHandler = qInstallMessageHandler(&handler);
    const auto guard = qScopeGuard([oldHandler]() { qInstallMessageHandler(oldHandler); });

    // t1 does not log anything
    QMetaObject::invokeMethod(o.data(), "t1");

    // t2 logs "false" exactly once
    expectingMessage = true;
    QMetaObject::invokeMethod(o.data(), "t2");
    QVERIFY(!expectingMessage);

    // t3 logs "false" exactly once
    expectingMessage = true;
    QMetaObject::invokeMethod(o.data(), "t3");
    QVERIFY(!expectingMessage);
}

void tst_QmlCppCodegen::basicBlocksWithBackJump_infinite()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/basicBlocksWithBackJump_infinite.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::basicDTZ()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/basicDTZ.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("title").toString(), u"none");

    QMetaObject::invokeMethod(o.data(), "t1");
    QMetaObject::invokeMethod(o.data(), "t2");
    QMetaObject::invokeMethod(o.data(), "t3");

    QCOMPARE(o->property("title").toString(), u"Baz 41");
}

void tst_QmlCppCodegen::bindToValueType()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/bindToValueType.qml"_s));
    checkColorProperties(&component);
}

void tst_QmlCppCodegen::bindingExpression()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/BindingExpression.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QObject *child = qmlContext(object.data())->objectForName(u"child"_s);

    double width = 200;
    double y = 10;
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(object->property("width").toDouble(), width);
        QCOMPARE(object->property("height").toDouble(), width);
        QCOMPARE(object->property("y").toDouble(), y);

        const double childY = y + (width - 100) / 2;
        QCOMPARE(child->property("y").toDouble(), childY);
        QCOMPARE(object->property("mass"), childY > 100 ? u"heavy"_s : u"light"_s);
        QCOMPARE(object->property("test_division").toDouble(), width / 1000 + 50);
        QCOMPARE(object->property("test_ternary").toDouble(), 2.2);

        const int test_switch = object->property("test_switch").toInt();
        switch (int(width) % 3) {
        case 0:
            QCOMPARE(test_switch, 130);
            break;
        case 1:
            QCOMPARE(test_switch, 380);
            break;
        case 2:
            QCOMPARE(test_switch, 630);
            break;
        }

        width = 200 * i;
        y = 10 + i;
        object->setProperty("width", width);
        object->setProperty("y", y);
    }
}

void tst_QmlCppCodegen::bindingToScriptStringProperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/BindingToScriptStringProperty.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    const auto *bottomRect = o->findChild<QObject *>("bottom");
    QCOMPARE(bottomRect->property("y").toInt(), 150);
    o->setProperty("c", true);
    QCOMPARE(bottomRect->property("y").toInt(), 300);
}

void tst_QmlCppCodegen::blockComments()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/blockComments.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(o->property("implicitHeight").toDouble(), 8.0);
}

void tst_QmlCppCodegen::boolCoercions()
{
    QQmlEngine e;
    const QUrl url(u"qrc:/qt/qml/TestTypes/boolCoercions.qml"_s);
    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + ":41:5: Unable to assign [undefined] to bool"_L1));
    QScopedPointer<QObject> o(c.create());

    for (char p = '1'; p <= '8'; ++p) {
        const QVariant t = o->property((u"t"_s + QLatin1Char(p)).toLatin1());
        QCOMPARE(t.metaType(), QMetaType::fromType<bool>());
        QVERIFY(t.toBool());
    }

    for (char p = '1'; p <= '5'; ++p) {
        const QVariant f = o->property((u"f"_s + QLatin1Char(p)).toLatin1());
        QCOMPARE(f.metaType(), QMetaType::fromType<bool>());
        QVERIFY(!f.toBool());
    }
}

void tst_QmlCppCodegen::boolPointerMerge()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/boolPointerMerge.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QObject *item = o->property("item").value<QObject *>();
    QVERIFY(item);
    QCOMPARE(item->property("ppp").toInt(), -99);
}

void tst_QmlCppCodegen::brokenAs()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/brokenAs.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QVERIFY(o->property("a").value<QObject *>() != nullptr);
    QCOMPARE(o->property("b").value<QObject *>(), nullptr);
}

void tst_QmlCppCodegen::boundComponents()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/boundComponents.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QObject *c1o = o->property("o").value<QObject *>();
    QVERIFY(c1o != nullptr);
    QCOMPARE(c1o->objectName(), u"bar"_s);

    QObject *c2o = c1o->property("o").value<QObject *>();
    QVERIFY(c2o != nullptr);
    QCOMPARE(c2o->objectName(), u"bar12"_s);
}

void tst_QmlCppCodegen::callContextPropertyLookupResult()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/callContextPropertyLookupResult.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QVERIFY(qvariant_cast<QQmlComponent *>(o->property("c")) != nullptr);
}

void tst_QmlCppCodegen::callObjectLookupOnNull()
{
    QQmlEngine engine;
    const QString urlString = u"qrc:/qt/qml/TestTypes/callObjectLookupOnNull.qml"_s;
    QQmlComponent c(&engine, QUrl(urlString));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->objectName(), u"horst"_s);

    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(urlString + u":5:5: TypeError: Cannot call method 'getName' of null"));
    o->setProperty("person", QVariant::fromValue<Person *>(nullptr));

    QCOMPARE(o->objectName(), QString());
}

void tst_QmlCppCodegen::callWithSpread()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/callWithSpread.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(QtCriticalMsg, "That is great!");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::colorAsVariant()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/colorAsVariant.qml"_s));
    checkColorProperties(&component);
}

void tst_QmlCppCodegen::colorString()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/colorString.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(qvariant_cast<QColor>(rootObject->property("c")), QColor::fromRgb(0xdd, 0xdd, 0xdd));
    QCOMPARE(qvariant_cast<QColor>(rootObject->property("d")), QColor::fromRgb(0xaa, 0xaa, 0xaa));
    QCOMPARE(qvariant_cast<QColor>(rootObject->property("e")), QColor::fromRgb(0x11, 0x22, 0x33));
}

void tst_QmlCppCodegen::compareOriginals()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/compareOriginals.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("compareOriginals").toInt(), 5);
    QVERIFY(object->property("optionalThis").toBool());
}

void tst_QmlCppCodegen::comparisonTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/comparisonTypes.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("found").toInt(), 1);
    QCOMPARE(object->property("foundStrict").toInt(), 0);

    QCOMPARE(object->property("foundNot").toInt(), 2);
    QCOMPARE(object->property("foundNotStrict").toInt(), 10);
}

void tst_QmlCppCodegen::componentReturnType()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/componentReturnType.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QCOMPARE(object->property("count").toInt(), 10);
    QCOMPARE(QQmlListReference(object.data(), "children").count(), 11);
}

void tst_QmlCppCodegen::compositeSingleton()
{
    QQmlEngine engine;
    engine.addImportPath(u":/qt/qml/TestTypes/imports/"_s);
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/compositesingleton.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> o(component.create());
    QCOMPARE(o->property("x").toDouble(), 4.5);
    QCOMPARE(o->property("y").toDouble(), 10.0);
    QCOMPARE(o->property("smooth").toBool(), true);
}

void tst_QmlCppCodegen::compositeTypeMethod()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/compositeTypeMethod.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QSignalSpy spy(object.data(), SIGNAL(foo()));
    QTRY_VERIFY(spy.size() > 0);
}

void tst_QmlCppCodegen::confusedModule()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/Confused/Main.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QTest::ignoreMessage(QtDebugMsg, "Hello from Test");
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QQmlComponent component2(&engine, QUrl(u"qrc:/qt/qml/Confused/Main2.qml"_s));
    QVERIFY2(!component2.isError(), component2.errorString().toUtf8());

    // TODO: We would like to have a better error here, but we currently cannot propagate it.
    QTest::ignoreMessage(
            QtWarningMsg,
            "qrc:/qt/qml/Confused/Main2.qml:5: "
            "TypeError: Property 'Print' of object Broken is not a function");

    QScopedPointer<QObject> object2(component2.create());
    QVERIFY(!object2.isNull());
}

void tst_QmlCppCodegen::consoleObject()
{
    QQmlEngine engine;
    static const QString urlString = u"qrc:/qt/qml/TestTypes/consoleObject.qml"_s;
    QQmlComponent c(&engine, QUrl(urlString));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtDebugMsg, "b 4.55");
    QTest::ignoreMessage(QtDebugMsg, "b 4.55");
    QTest::ignoreMessage(QtInfoMsg, "b 4.55");
    QTest::ignoreMessage(QtWarningMsg, "b 4.55");
    QTest::ignoreMessage(QtCriticalMsg, "b 4.55");

    // Unfortunately we cannot check the logging category with QTest::ignoreMessage
    QTest::ignoreMessage(QtDebugMsg, "b 4.55");
    QTest::ignoreMessage(QtDebugMsg, "b 4.55");
    QTest::ignoreMessage(QtInfoMsg, "b 4.55");
    QTest::ignoreMessage(QtWarningMsg, "b 4.55");
    QTest::ignoreMessage(QtCriticalMsg, "b 4.55");

    const QRegularExpression re(u"QQmlComponentAttached\\(0x[0-9a-f]+\\) b 4\\.55"_s);
    QTest::ignoreMessage(QtDebugMsg, re);
    QTest::ignoreMessage(QtDebugMsg, re);
    QTest::ignoreMessage(QtInfoMsg, re);
    QTest::ignoreMessage(QtWarningMsg, re);
    QTest::ignoreMessage(QtCriticalMsg, re);

    QTest::ignoreMessage(QtDebugMsg, "a undefined b false null 7");
    QTest::ignoreMessage(QtDebugMsg, "");
    QTest::ignoreMessage(QtDebugMsg, "4");
    QTest::ignoreMessage(QtDebugMsg, "");

    const QRegularExpression re2(u"QQmlComponentAttached\\(0x[0-9a-f]+\\)"_s);
    QTest::ignoreMessage(QtDebugMsg, re2);

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    auto oldHandler = qInstallMessageHandler(
            [](QtMsgType, const QMessageLogContext &ctxt, const QString &) {
        QCOMPARE(ctxt.file, urlString.toUtf8());
        QCOMPARE(ctxt.function, QByteArray("expression for onCompleted"));
        QVERIFY(ctxt.line > 0);
    });
    const auto guard = qScopeGuard([oldHandler]() { qInstallMessageHandler(oldHandler); });

    QScopedPointer<QObject> p(c.create());
    QVERIFY(!p.isNull());
}

void tst_QmlCppCodegen::consoleTrace()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/consoleTrace.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    // We always get line numbers for the first call since we need to do the "init" step.

    QTest::ignoreMessage(QtDebugMsg, R"(c (qrc:/qt/qml/TestTypes/consoleTrace.qml:6)
b (qrc:/qt/qml/TestTypes/consoleTrace.qml:5)
a (qrc:/qt/qml/TestTypes/consoleTrace.qml:4)
expression for onCompleted (qrc:/qt/qml/TestTypes/consoleTrace.qml:7))");

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
}

void tst_QmlCppCodegen::construct()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/construct.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const QJSManagedValue v = engine.toManagedValue(object->property("foo"));
    QVERIFY(v.isError());
    QCOMPARE(v.toString(), u"Error: bar"_s);

    QCOMPARE(object->property("aaa").toInt(), 12);
    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/construct.qml:9: Error: ouch");
    object->metaObject()->invokeMethod(object.data(), "ouch");
    QCOMPARE(object->property("aaa").toInt(), 13);
}

void tst_QmlCppCodegen::contextParam()
{
    // The compiler cannot resolve context parameters.
    // Make sure the binding is interpreted.

    QQmlEngine engine;

    QVariantMap m;
    m.insert(u"foo"_s, 10);
    engine.rootContext()->setContextProperty(u"contextParam"_s, m);

    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/contextParam.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("foo").toInt(), 10);
}

void tst_QmlCppCodegen::conversionDecrement()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/conversionDecrement.qml"_s));

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("currentPageIndex").toInt(), 0);
    o->setProperty("pages", 5);
    QCOMPARE(o->property("currentPageIndex").toInt(), 3);
    o->setProperty("pages", 4);
    QCOMPARE(o->property("currentPageIndex").toInt(), 0);
    o->setProperty("pages", 6);
    QCOMPARE(o->property("currentPageIndex").toInt(), 4);
    o->setProperty("pages", 60);
    QCOMPARE(o->property("currentPageIndex").toInt(), 3);
}

void tst_QmlCppCodegen::conversionInDeadCode()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/conversionInDeadCode.qml"_s));

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("a").toInt(), -4);
    QCOMPARE(o->property("b").toInt(), 12);
    QCOMPARE(o->property("c").toInt(), 10);
    QCOMPARE(o->property("d").toInt(), 10);
    QCOMPARE(o->property("e").toInt(), 10);
    QCOMPARE(o->property("f").toInt(), 20);
    QCOMPARE(o->property("g").toInt(), 33);
}

void tst_QmlCppCodegen::conversions()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/conversions.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/conversions.qml:42: TypeError: Type error");

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QVERIFY(object->property("nullIsNull").toBool());
    QVERIFY(!object->property("intIsNull").toBool());
    QVERIFY(!object->property("zeroIsNull").toBool());
    QVERIFY(!object->property("arrayIsNull").toBool());
    QVERIFY(!object->property("objectIsNull").toBool());
    QVERIFY(!object->property("nullIsNotNull").toBool());
    QVERIFY(object->property("intIsNotNull").toBool());
    QVERIFY(object->property("zeroIsNotNull").toBool());
    QVERIFY(object->property("arrayIsNotNull").toBool());
    QVERIFY(object->property("objectIsNotNull").toBool());
    QVERIFY(!object->property("neNull").toBool());
    QVERIFY(object->property("eqNull").toBool());

    QVERIFY(!object->property("boolEqualsBool").toBool());
    QVERIFY(object->property("boolNotEqualsBool").toBool());

    QCOMPARE(object->property("cmpEqInt").toInt(), 17);

    QCOMPARE(object->property("undefinedType").toString(), u"undefined"_s);
    QCOMPARE(object->property("booleanType").toString(), u"boolean"_s);
    QCOMPARE(object->property("numberType").toString(), u"number"_s);
    QCOMPARE(object->property("stringType").toString(), u"string"_s);
    QCOMPARE(object->property("objectType").toString(), u"object"_s);
    QCOMPARE(object->property("symbolType").toString(), u"symbol"_s);

    QJSManagedValue obj = engine.toManagedValue(object->property("anObject"));
    QCOMPARE(obj.property(u"a"_s).toInt(), 12);
    QCOMPARE(obj.property(u"b"_s).toInt(), 14);
    QCOMPARE(obj.property(u"c"_s).toString(), u"somestring"_s);

    QVERIFY(object->property("aInObject").toBool());
    QVERIFY(object->property("lengthInArray").toBool());
    QVERIFY(!object->property("fooInNumber").isValid());
    QVERIFY(!object->property("numberInObject").toBool());
    QVERIFY(object->property("numberInArray").toBool());

    QCOMPARE(object->property("varPlusVar").toDouble(), 2.0);
    QVERIFY(qIsNaN(object->property("varMinusVar").toDouble()));
    QVERIFY(qIsNaN(object->property("varTimesVar").toDouble()));
    QCOMPARE(object->property("varDivVar").toDouble(), 0.0);

    const QVariant stringPlusString = object->property("stringPlusString");
    QCOMPARE(stringPlusString.typeId(), QMetaType::QString);
    QCOMPARE(stringPlusString.toString(), u"1220"_s);

    const QVariant stringMinusString = object->property("stringMinusString");
    QCOMPARE(stringMinusString.typeId(), QMetaType::Double);
    QCOMPARE(stringMinusString.toDouble(), -8.0);

    const QVariant stringTimesString = object->property("stringTimesString");
    QCOMPARE(stringTimesString.typeId(), QMetaType::Double);
    QCOMPARE(stringTimesString.toDouble(), 240.0);

    const QVariant stringDivString = object->property("stringDivString");
    QCOMPARE(stringDivString.typeId(), QMetaType::Double);
    QCOMPARE(stringDivString.toDouble(), 0.6);

    QCOMPARE(object->property("uglyString").toString(),
             u"with\nnewlinewith\"quotwith\\slashes"_s);

    QCOMPARE(qvariant_cast<QObject *>(object->property("nullObject1")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(object->property("nullObject2")), object.data());

    QCOMPARE(object->property("passObjectLiteral").toInt(), 11);
    QCOMPARE(object->property("passArrayLiteral").toInt(), 4);

    QCOMPARE(object->property("doneStuff").toInt(), 19);

    QVariantList modulos = object->property("modulos").toList();
    QCOMPARE(modulos.size(), 7);

    QCOMPARE(modulos[0].userType(), QMetaType::Double);
    QCOMPARE(modulos[0].toDouble(), 0.0);

    QCOMPARE(modulos[1].userType(), QMetaType::Int);
    QCOMPARE(modulos[1].toInt(), 0);

    QCOMPARE(modulos[2].userType(), QMetaType::Double);
    QCOMPARE(modulos[2].toDouble(), 2.4);

    QCOMPARE(modulos[3].userType(), QMetaType::Double);
    QCOMPARE(modulos[3].toDouble(), -0.4);

    QCOMPARE(modulos[4].userType(), QMetaType::Double);
    QCOMPARE(modulos[4].toDouble(), 0.0);

    QCOMPARE(modulos[5].userType(), QMetaType::Double);
    QVERIFY(qIsNaN(modulos[5].toDouble()));

    QCOMPARE(modulos[6].userType(), QMetaType::Double);
    QVERIFY(qIsNaN(modulos[6].toDouble()));

    QVariantList unaryOps = object->property("unaryOps").toList();
    QCOMPARE(unaryOps.size(), 6);

    QCOMPARE(unaryOps[0].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[0].toDouble(), 1221);

    QCOMPARE(unaryOps[1].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[1].toDouble(), 1219);

    QCOMPARE(unaryOps[2].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[2].toDouble(), 1220);

    QCOMPARE(unaryOps[3].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[3].toDouble(), -1220);

    QCOMPARE(unaryOps[4].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[4].toDouble(), 1220);

    QCOMPARE(unaryOps[5].userType(), QMetaType::Double);
    QCOMPARE(unaryOps[5].toDouble(), 1220);

    QVariant undef;
    QVERIFY(object->metaObject()->invokeMethod(object.data(), "retUndefined",
                                               Q_RETURN_ARG(QVariant, undef)));
    QVERIFY(!undef.isValid());
}

void tst_QmlCppCodegen::convertPrimitiveToVar()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/convertPrimitiveToVar.qml"_s));

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("offsetValue").toInt(), 41);
}

void tst_QmlCppCodegen::convertQJSPrimitiveValueToIntegral()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/convertQJSPrimitiveValueToIntegral.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::convertToOriginalReadAcumulatorForUnaryOperators()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/convertToOriginalReadAcumulatorForUnaryOperators.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::cppMethodListReturnType()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/CppMethodListReturnType.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("list").toList()[2].toInt(), 2);
}

void tst_QmlCppCodegen::cppValueTypeList()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Test.qml"_s));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), component.errorString().toUtf8().constData());
    QCOMPARE(object->property("a").toInt(), 16);
    QMetaObject::invokeMethod(object.data(), "incA");
    QCOMPARE(object->property("a").toInt(), 17);

    QCOMPARE(object->property("b").toDouble(), 0.25);
    QMetaObject::invokeMethod(object.data(), "incB");
    QCOMPARE(object->property("b").toDouble(), 13.5);
}

void tst_QmlCppCodegen::dateConstruction()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/dateConstruction.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QDateTime now = QDateTime::currentDateTime();
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QVERIFY(o->property("now").value<QDateTime>().toMSecsSinceEpoch() >= now.toMSecsSinceEpoch());
    QCOMPARE(o->property("now2"), o->property("now"));
    QCOMPARE(o->property("fromString").value<QDateTime>(),
             QDateTime(QDate(1995, 12, 17), QTime(3, 24), QTimeZone::LocalTime));
    QCOMPARE(o->property("fromNumber").value<QDateTime>().toMSecsSinceEpoch(), 777);
    QCOMPARE(o->property("fromPrimitive").value<QDateTime>().toMSecsSinceEpoch(), 57);
    o->setObjectName("foo"_L1);
    QCOMPARE(o->property("fromPrimitive").value<QDateTime>(),
             QDateTime(QDate(1997, 2, 13), QTime(13, 4, 12), QTimeZone::LocalTime));

    QCOMPARE(o->property("from2").value<QDateTime>(),
             QDateTime(QDate(1996, 2, 1), QTime(), QTimeZone::LocalTime));
    QCOMPARE(o->property("from3").value<QDateTime>(),
             QDateTime(QDate(1996, 3, 3), QTime(), QTimeZone::LocalTime));
    QCOMPARE(o->property("from4").value<QDateTime>(),
             QDateTime(QDate(1996, 4, 4), QTime(5, 0), QTimeZone::LocalTime));
    QCOMPARE(o->property("from5").value<QDateTime>(),
             QDateTime(QDate(1996, 5, 5), QTime(6, 7), QTimeZone::LocalTime));
    QCOMPARE(o->property("from6").value<QDateTime>(),
             QDateTime(QDate(1996, 6, 6), QTime(7, 8, 9), QTimeZone::LocalTime));
    QCOMPARE(o->property("from7").value<QDateTime>(),
             QDateTime(QDate(1996, 7, 7), QTime(8, 9, 10, 11), QTimeZone::LocalTime));
    QCOMPARE(o->property("from8").value<QDateTime>(),
             QDateTime(QDate(1996, 8, 8), QTime(9, 10, 11, 12), QTimeZone::LocalTime));

    QCOMPARE(o->property("withUnderflow").value<QDateTime>(),
             QDateTime(QDate(-6, 7, 24), QTime(16, 51, 50, 990), QTimeZone::LocalTime));
    QCOMPARE(o->property("invalid").value<QDateTime>(), QDateTime());
}

void tst_QmlCppCodegen::dateConversions()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/dateConversions.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    Druggeljug *ref = engine.singletonInstance<Druggeljug *>("TestTypes", "Druggeljug");

    const QDateTime refDate = engine.coerceValue<QDate, QDateTime>(ref->myDate());
    const QDateTime refTime = engine.coerceValue<QTime, QDateTime>(ref->myTime());

    QCOMPARE(o->property("date").value<QDateTime>(), refDate);
    QCOMPARE(o->property("time").value<QDateTime>(), refTime);

    QCOMPARE(o->property("dateString").toString(),
             (engine.coerceValue<QDateTime, QString>(refDate)));
    QCOMPARE(o->property("dateNumber").toDouble(),
             (engine.coerceValue<QDateTime, double>(refDate)));
    QCOMPARE(o->property("timeString").toString(),
             (engine.coerceValue<QDateTime, QString>(refTime)));
    QCOMPARE(o->property("timeNumber").toDouble(),
             (engine.coerceValue<QDateTime, double>(refTime)));

    QMetaObject::invokeMethod(o.data(), "shuffle");

    QCOMPARE(ref->myDate(), (engine.coerceValue<QDateTime, QDate>(refDate)));
    QCOMPARE(ref->myTime(), (engine.coerceValue<QDateTime, QTime>(refTime)));

    const QDate date = ref->myDate();
    const QTime time = ref->myTime();

    QCOMPARE(o->property("dateString").toString(), (engine.coerceValue<QDate, QString>(date)));
    QCOMPARE(o->property("dateNumber").toDouble(), (engine.coerceValue<QDate, double>(date)));
    QCOMPARE(o->property("timeString").toString(), (engine.coerceValue<QTime, QString>(time)));
    QCOMPARE(o->property("timeNumber").toDouble(), (engine.coerceValue<QTime, double>(time)));

    QMetaObject::invokeMethod(o.data(), "fool");

    QCOMPARE(ref->myDate(), (engine.coerceValue<QTime, QDate>(time)));
    QCOMPARE(ref->myTime(), (engine.coerceValue<QDate, QTime>(date)));

    QMetaObject::invokeMethod(o.data(), "invalidate");
    QMetaObject::invokeMethod(o.data(), "shuffle");

    QCOMPARE(o->property("dateString").toString(), "Invalid Date"_L1);
    QVERIFY(qIsNaN(o->property("dateNumber").toDouble()));
    QCOMPARE(o->property("timeString").toString(), "Invalid Date"_L1);
    QVERIFY(qIsNaN(o->property("timeNumber").toDouble()));

}

void tst_QmlCppCodegen::deadShoeSize()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/deadShoeSize.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/deadShoeSize.qml:5: Error: ouch");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(o->property("shoeSize").toInt(), 0);
}

void tst_QmlCppCodegen::destroyAndToString()
{
    QQmlEngine engine;
    const QString url = u"qrc:/qt/qml/TestTypes/destroyAndToString.qml"_s;
    QQmlComponent c(&engine, QUrl(url));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    const QString objectName = u"me, myself, and I"_s;
    QCOMPARE(o->objectName(), objectName);

    auto verifyName = [&](const char *property, const QString &name) {
        const QRegularExpression regexp(u"^QObject[^(]*\\(0x[0-9a-f]+, \"%1\"\\)$"_s.arg(name));
        const QString value = o->property(property).toString();
        const auto match = regexp.match(value);
        QVERIFY2(match.hasMatch(), qPrintable(value + u" does not match " + regexp.pattern()));
    };

    verifyName("stringed", objectName);
    verifyName("selfStringed", objectName);
    verifyName("immediateShadowableStringed", u"immediateShadowable"_s);
    verifyName("delayedShadowableStringed", u"delayedShadowable"_s);

    QVERIFY(o->property("delayedShadowable").value<QObject *>());
    QVERIFY(o->property("immediateShadowable").value<QObject *>());
    QVERIFY(o->property("delayedDestroyable").value<QObject *>());
    QVERIFY(o->property("immediateDestroyable").value<QObject *>());
    QVERIFY(o->property("scopedImmediateDestroyable").value<QObject *>());
    QVERIFY(o->property("scopedDelayedDestroyable").value<QObject *>());

    QMetaObject::invokeMethod(o.data(), "explode");

    QVERIFY(o->property("delayedShadowable").value<QObject *>());
    QVERIFY(o->property("delayedDestroyable").value<QObject *>());
    QVERIFY(o->property("scopedDelayedDestroyable").value<QObject *>());

    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url + u":36: TypeError: Cannot call method 'toString' of null"));
    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url + u":37: TypeError: Cannot call method 'toString' of null"));

    QTRY_VERIFY(!o->property("immediateShadowable").value<QObject *>());
    QTRY_VERIFY(!o->property("immediateDestroyable").value<QObject *>());
    QTRY_VERIFY(!o->property("scopedImmediateDestroyable").value<QObject *>());
    QTRY_VERIFY(!o->property("delayedShadowable").value<QObject *>());
    QTRY_VERIFY(!o->property("delayedDestroyable").value<QObject *>());
    QTRY_VERIFY(!o->property("scopedDelayedDestroyable").value<QObject *>());

    verifyName("stringed", objectName);
    verifyName("selfStringed", objectName);

    // Since the bindings threw errors, the values were not changed
    verifyName("immediateShadowableStringed", u"immediateShadowable"_s);
    verifyName("delayedShadowableStringed", u"delayedShadowable"_s);

    QVERIFY(o->property("overrides").value<QObject *>() != nullptr);
    QString result;
    QMetaObject::invokeMethod(o.data(), "callOverridden", Q_RETURN_ARG(QString, result));
    QCOMPARE(result, u"yes"_s);

    QCOMPARE(o->objectName(), objectName);
    QTRY_COMPARE(o->property("overrides").value<QObject *>(), nullptr);
}

void tst_QmlCppCodegen::detachOnAssignment()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/detachOnAssignment.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    BirthdayPartyAttached *b = static_cast<BirthdayPartyAttached *>(
            qmlAttachedPropertiesObject<BirthdayParty>(o.data()));
    QVERIFY(b);
    QCOMPARE(o->property("d").value<QDateTime>().date().year(), 1997);
    QCOMPARE(b->rsvp().date().year(), 2001);

    Person *p = qvariant_cast<Person *>(o->property("person"));
    QVERIFY(p);

    QCOMPARE(o->property("r").value<QRectF>(), QRectF(2, 3, 4, 5));
    QCOMPARE(p->area(), QRectF(22, 3, 4, 5));

    QCOMPARE(o->property("v").value<QVariantList>()[0], QStringLiteral("a"));
    QCOMPARE(p->things()[0], QStringLiteral("c"));
}

void tst_QmlCppCodegen::dialogButtonBox()
{
    QQmlEngine engine;
    const QUrl copy(u"qrc:/qt/qml/TestTypes/dialogButtonBox.qml"_s);
    QQmlComponent c(&engine, copy);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QObject *footer = o->property("footer").value<QObject *>();
    QVERIFY(footer);

    QCOMPARE(footer->property("standardButtons").value<QPlatformDialogHelper::StandardButton>(),
             QPlatformDialogHelper::Ok | QPlatformDialogHelper::Cancel);
}

void tst_QmlCppCodegen::enumConversion()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumConversion.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(o->property("test").toInt(), 0x04);
    QCOMPARE(o->property("test_1").toBool(), true);
    QCOMPARE(o->objectName(), u"0m"_s);
}

void tst_QmlCppCodegen::enumFromBadSingleton()
{
    QQmlEngine e;
    const QUrl url(u"qrc:/qt/qml/TestTypes/enumFromBadSingleton.qml"_s);
    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

#if QT_DEPRECATED_SINCE(6,4)
    QTest::ignoreMessage(
                QtWarningMsg, qPrintable(
                    url.toString()
                    + u":5:5: TypeError: Cannot read property 'TestA' of undefined"_s));
#else
    QTest::ignoreMessage(
                QtWarningMsg, qPrintable(
                    url.toString()
                    + u":5:5: ReferenceError: DummyObjekt is not defined"_s));
#endif

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QVERIFY(o->objectName().isEmpty());
}

void tst_QmlCppCodegen::enumLookup()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumLookup.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("ready").toBool(), true);
    QCOMPARE(o->property("enumFromGadget2").toInt(), 3);
}

void tst_QmlCppCodegen::enumMarkedAsFlag()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumMarkedAsFlag.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("flagValue").toInt(), 3);
}

void tst_QmlCppCodegen::enumProblems()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumProblems.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> outer(c.create());
    QVERIFY(!outer.isNull());
    QObject *inner = outer->property("o").value<QObject *>();
    QVERIFY(inner);

    Foo *bar = inner->property("bar").value<Foo *>();
    QVERIFY(bar);
    QCOMPARE(bar->type(), Foo::Component);

    Foo *fighter = inner->property("fighter").value<Foo *>();
    QVERIFY(fighter);
    QCOMPARE(fighter->type(), Foo::Fighter);

    QCOMPARE(outer->property("a").toInt(), FooFactory::B);
    QCOMPARE(outer->property("b").toInt(), FooFactory::C);
    QCOMPARE(outer->property("c").toInt(), FooFactory::D);
    QCOMPARE(outer->property("d").toInt(), FooFactory::E);
}

void tst_QmlCppCodegen::enumScope()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumScope.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QCOMPARE(object->property("flow").toInt(), 1);
}

void tst_QmlCppCodegen::enums()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Enums.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/Enums.qml:4:1: "
                                       "QML Enums: Layout attached property must be attached to an object deriving from Item");
    QScopedPointer<QObject> object(component.create());

    QVERIFY(!object.isNull());
    bool ok = false;
    QCOMPARE(object->property("appState").toInt(&ok), 2);
    QVERIFY(ok);
    QCOMPARE(object->property("color").toString(), u"blue"_s);

    QTRY_COMPARE(object->property("appState").toInt(&ok), 1);
    QVERIFY(ok);
    QCOMPARE(object->property("color").toString(), u"green"_s);

    const auto func = qmlAttachedPropertiesFunction(
            object.data(), QMetaType::fromName("QQuickLayout*").metaObject());

    QObject *attached = qmlAttachedPropertiesObject(object.data(), func);

    const QVariant prop = attached->property("alignment");
    QVERIFY(prop.isValid());
    QCOMPARE(qvariant_cast<Qt::Alignment>(prop), Qt::AlignCenter);

}

void tst_QmlCppCodegen::enforceSignature()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enforceSignature.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const QVariant a = object->property("a");
    QCOMPARE(a.metaType(), QMetaType::fromType<QObject *>());
    QCOMPARE(a.value<QObject *>(), nullptr);

    const QVariant b = object->property("b");
    QCOMPARE(b.metaType(), QMetaType::fromType<QObject *>());
    QCOMPARE(b.value<QObject *>(), nullptr);
}

void tst_QmlCppCodegen::enumsInOtherObject()
{
    QQmlEngine engine;
    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/enumsInOtherObject.qml:4:25: "
                                       "QML Enums: Layout attached property must be attached to an object deriving from Item");
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumsInOtherObject.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("color").toString(), u"blue"_s);
    QTRY_COMPARE(object->property("color").toString(), u"green"_s);
}

void tst_QmlCppCodegen::equalityQObjects()
{
    QQmlEngine engine;
    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/equalityQObjects.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));
    QScopedPointer<QObject> object(c1.create());
    QVERIFY(!object.isNull() && !c1.isError());

    QVERIFY(object->property("derivedIsNotNull").toBool());
    QVERIFY(object->property("nullObjectIsNull").toBool());
    QVERIFY(object->property("nonNullObjectIsNotNull").toBool());
    QVERIFY(object->property("compareSameObjects").toBool());
    QVERIFY(object->property("compareDifferentObjects").toBool());
    QVERIFY(object->property("compareObjectWithNullObject").toBool());

    QVERIFY(object->property("nonStrict_derivedIsNotNull").toBool());
    QVERIFY(object->property("nonStrict_nullObjectIsNull").toBool());
    QVERIFY(object->property("nonStrict_nonNullObjectIsNotNull").toBool());
    QVERIFY(object->property("nonStrict_compareSameObjects").toBool());
    QVERIFY(object->property("nonStrict_compareDifferentObjects").toBool());
    QVERIFY(object->property("nonStrict_compareObjectWithNullObject").toBool());
}

void tst_QmlCppCodegen::equalityQUrl()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/equalityQUrl.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> object(c1.create());
    QVERIFY(!object.isNull() && !c1.isError());
    QVERIFY(object->property("emptyUrlStrict").toBool());
    QVERIFY(object->property("emptyUrlWeak").toBool());
    QVERIFY(object->property("sourceUrlStrict").toBool());
    QVERIFY(object->property("sourceUrlWeak").toBool());
    QVERIFY(object->property("sourceIsNotEmptyStrict").toBool());
    QVERIFY(object->property("sourceIsNotEmptyWeak").toBool());
}

void tst_QmlCppCodegen::equalityTestsWithNullOrUndefined()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/equalityTestsWithNullOrUndefined.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);
}

void tst_QmlCppCodegen::equalityVarAndNonStorable()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/equalityVarAndNonStorable.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> object(c1.create());
    QVERIFY(!object.isNull() && !c1.isError());
    QVERIFY(!object->property("aIsNull").toBool());
    QVERIFY(object->property("aIsNotNull").toBool());
    QVERIFY(object->property("aIsNotUndefined").toBool());
    QVERIFY(object->property("objectIsNotNull").toBool());
    QVERIFY(!object->property("typedArrayIsNull").toBool());
    QVERIFY(object->property("isUndefined").toBool());
    QVERIFY(!object->property("derivedIsNull").toBool());

    QVERIFY(object->property("primitiveIsNull").toBool());
    QVERIFY(object->property("primitiveIsDefined").toBool());
    QVERIFY(object->property("primitiveIsUndefined").toBool());

    QVERIFY(object->property("jsValueIsNull").toBool());
    QVERIFY(object->property("jsValueIsDefined").toBool());
    QVERIFY(object->property("jsValueIsUndefined").toBool());

    QVERIFY(object->property("nullVarIsUndefined").toBool());
    QVERIFY(object->property("nullIsUndefined").toBool());
    QVERIFY(object->property("nullVarIsNull").toBool());
    QVERIFY(object->property("nullIsNotUndefined").toBool());
}

void tst_QmlCppCodegen::equalityVarAndStorable()
{
    QQmlEngine engine;
    QQmlComponent planner(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Planner.qml"_s));
    QVERIFY2(!planner.isError(), qPrintable(planner.errorString()));
    QScopedPointer<QObject> p(planner.create());
    QVERIFY(!p.isNull());

    QQmlComponent variable(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Variable.qml"_s));
    QVERIFY2(!variable.isError(), qPrintable(variable.errorString()));
    QScopedPointer<QObject> v(variable.create());
    QVERIFY(!v.isNull());

    QVERIFY(p->objectName().isEmpty());
    QMetaObject::invokeMethod(p.data(), "typeErasedRemoveOne", v.data());
    QCOMPARE(p->objectName(), u"n"_s);

    v->setProperty("value", 1);
    QMetaObject::invokeMethod(p.data(), "typeErasedRemoveOne", v.data());
    QCOMPARE(p->objectName(), u"nd"_s);

    QQmlComponent constraint(&engine, QUrl(u"qrc:/qt/qml/TestTypes/BaseConstraint.qml"_s));
    QVERIFY2(!constraint.isError(), qPrintable(constraint.errorString()));
    QScopedPointer<QObject> c(constraint.create());
    QVERIFY(!c.isNull());

    c->setProperty("output", QVariant::fromValue(v.data()));
    QCOMPARE(v->property("mark").toInt(), 0);
    QMetaObject::invokeMethod(p.data(), "typeErasedRun", c.data());
    QCOMPARE(v->property("mark").toInt(), 5);

    QTest::ignoreMessage(QtDebugMsg, "success");
    QMetaObject::invokeMethod(p.data(), "verify", 10);

    QTest::ignoreMessage(QtCriticalMsg, "failed 10 11");
    QMetaObject::invokeMethod(p.data(), "verify", 11);
}

void tst_QmlCppCodegen::equalityVarWithOutConversion()
{
    QQmlEngine engine;
    QUrl url(u"qrc:/qt/qml/TestTypes/equalityVarWithOutConversion.qml"_s);
    QQmlComponent component(&engine, url);
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    auto ss = o->property("ss").value<QQmlScriptString>();
    QQmlExpression expr(ss);
    expr.evaluate();
}

void tst_QmlCppCodegen::equalsUndefined()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/equalsUndefined.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("a").toInt(), 50);
    QCOMPARE(object->property("b").toInt(), 5000);
}

void tst_QmlCppCodegen::evadingAmbiguity()
{
    QQmlEngine engine;

    // We need to add an import path here because we cannot namespace the implicit import.
    // The implicit import is what we use for all the other tests, even if we explicitly
    // import TestTypes. That is because the TestTypes module is in a subdirectory "data".
    engine.addImportPath(u":/"_s);

    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ambiguous1/Ambiguous.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));
    QScopedPointer<QObject> o1(c1.create());
    QCOMPARE(o1->objectName(), QStringLiteral("Ambiguous"));
    QCOMPARE(o1->property("i").toString(), QStringLiteral("Ambiguous1"));

    QQmlComponent c2(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ambiguous2/Ambiguous.qml"_s));
    QVERIFY2(c2.isReady(), qPrintable(c2.errorString()));
    QScopedPointer<QObject> o2(c2.create());
    QCOMPARE(o2->objectName(), QStringLiteral("Ambiguous"));
    QCOMPARE(o2->property("i").toString(), QStringLiteral("Ambiguous2"));
}

void tst_QmlCppCodegen::exceptionFromInner()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/exceptionFromInner.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QTest::ignoreMessage(
        QtWarningMsg,
        "qrc:/qt/qml/TestTypes/exceptionFromInner.qml:7: TypeError: "
        "Cannot read property 'objectName' of null");
    QMetaObject::invokeMethod(object.data(), "disbelieveFail");
}

void tst_QmlCppCodegen::excessiveParameters()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/excessiveParameters.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QSignalSpy spy(object.data(), SIGNAL(foo()));
    QTRY_VERIFY(spy.size() > 0);
}

void tst_QmlCppCodegen::extendedTypes()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/extendedTypes.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtDebugMsg, "6 QSizeF(10, 20) 30");
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("a").toInt(), 6);
    QCOMPARE(qvariant_cast<QSizeF>(object->property("b")), QSizeF(10, 20));
    QCOMPARE(object->property("c").toInt(), 30);
    QCOMPARE(object->property("d").toString(), u"QSizeF(10, 20)"_s);

    QCOMPARE(object->property("e").toInt(), 2);
}

void tst_QmlCppCodegen::failures()
{
    const auto &aotFailure
            = QmlCacheGeneratedCode::_qt_qml_TestTypes_failures_qml::aotBuiltFunctions[0];
    QVERIFY(!aotFailure.functionPtr);
    QCOMPARE(aotFailure.functionIndex, 0);
}

void tst_QmlCppCodegen::fallbackLookups()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/fallbacklookups.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->objectName(), QString());
    int result = 0;

    QMetaObject::invokeMethod(o.data(), "withContext", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 16);
    QCOMPARE(o->objectName(), QStringLiteral("aa93"));

    QMetaObject::invokeMethod(o.data(), "withId", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 17);
    QCOMPARE(o->objectName(), QStringLiteral("bb94"));

    QObject *singleton = nullptr;
    QMetaObject::invokeMethod(o.data(), "getSingleton", Q_RETURN_ARG(QObject*, singleton));
    QVERIFY(singleton);

    QMetaObject::invokeMethod(o.data(), "withSingleton", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 18);
    QCOMPARE(singleton->objectName(), QStringLiteral("cc95"));

    QMetaObject::invokeMethod(o.data(), "withProperty", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 19);
    QCOMPARE(singleton->objectName(), QStringLiteral("dd96"));
}

void tst_QmlCppCodegen::fileImportsContainCxxTypes()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/usingCxxTypesFromFileImports.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"horst guenther"_s);
}

void tst_QmlCppCodegen::finalProperty()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/finalUser.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("f").toInt(), 12);
}

void tst_QmlCppCodegen::flagEnum()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/flagEnum.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlCommunicationPermission *p = qobject_cast<QQmlCommunicationPermission *>(o.data());
    QCOMPARE(p->communicationModes(), CommunicationPermission::Access);
}

void tst_QmlCppCodegen::flushBeforeCapture()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/noBindingLoop.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("deviation").toDouble(), 9.0 / 3.3333);
    QCOMPARE(o->property("samples").toInt(), 16);
    QCOMPARE(o->property("radius").toDouble(), 8.0);
}

void tst_QmlCppCodegen::fromBoolValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/fromBoolValue.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("a").toBool(), true);
    o->setProperty("x", 100);
    QCOMPARE(o->property("a").toBool(), false);

    QCOMPARE(o->property("width").toInt(), 100);
    QCOMPARE(o->property("b").toBool(), false);

    QScopedPointer<QObject> parent(c.create());
    o->setProperty("parent", QVariant::fromValue(parent.data()));
    QCOMPARE(o->property("width").toInt(), 100);
    QCOMPARE(o->property("b").toBool(), false);

    o->setProperty("state", QVariant::fromValue(u"foo"_s));
    QCOMPARE(o->property("width").toInt(), 0);
    QCOMPARE(o->property("b").toBool(), false);
}

void tst_QmlCppCodegen::funcWithParams()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/funcWithParams.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("bar").toInt(), 30);
}

// QML-generated types have no C++ names, but we want to call a method that
// expects a pointer to a QML-generated type as argument.
//
// We force the QML engine to assign a specific name to our type and declare
// an incomplete dummy class of the same name here. The dummy class does not
// have a proper metatype by itself. Therefore, when we want to pass a (null)
// pointer of it to invokeMethod(), usually invokeMethod() would complain that
// the metatype of the argument we pass does not match the metatype of the
// argument the method expects. In order to work around it, we specialize
// qMetaTypeInterfaceForType() and produce a "correct" metatype this way.
class Dummy2_QMLTYPE_0;

// We set this to the actual value retrieved from an actual instance of the QML
// type before retrieving the metatype interface for the first time.
static const QtPrivate::QMetaTypeInterface *dummyMetaTypeInterface = nullptr;
template<>
const QtPrivate::QMetaTypeInterface *QtPrivate::qMetaTypeInterfaceForType<Dummy2_QMLTYPE_0 *>() {
    return dummyMetaTypeInterface;
}

void tst_QmlCppCodegen::functionArguments()
{
    qmlClearTypeRegistrations();

    QQmlEngine engine;
    QQmlComponent preheat(&engine);
    preheat.setData(R"(
        import QtQml
        import TestTypes
        QtObject {
            objectName: Style.objectName
        }
    )", QUrl(u"qrc:/qt/qml/TestTypes/preheat.qml"_s));
    QVERIFY2(preheat.isReady(), qPrintable(preheat.errorString()));
    QScopedPointer<QObject> hot(preheat.create());
    QVERIFY(!hot.isNull());

    // Ensure that Dummy gets counter value 1. Don't do that at home
    QScopedValueRollback<QAtomicInt> rb(QQmlPropertyCacheCreatorBase::classIndexCounter, 0);

    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Dummy2.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    const QMetaObject *metaObject = object->metaObject();
    dummyMetaTypeInterface = metaObject->metaType().iface();
    const QByteArray className = QByteArray(metaObject->className());
    QCOMPARE(className, "Dummy2_QMLTYPE_0");

    int result;
    int a = 1;
    bool b = false;
    Dummy2_QMLTYPE_0 *c = nullptr;
    double d = -1.2;
    int e = 3;

    metaObject->invokeMethod(
                object.data(), "someFunction", Q_RETURN_ARG(int, result),
                Q_ARG(int, a), Q_ARG(bool, b), Q_ARG(Dummy2_QMLTYPE_0 *, c),
                Q_ARG(double, d), Q_ARG(int, e));
    QCOMPARE(result, 42);

    QString astr = u"foo"_s;
    QString bstr = u"bar"_s;
    QString concatenated;
    metaObject->invokeMethod(
                object.data(), "concat", Q_RETURN_ARG(QString, concatenated),
                Q_ARG(QString, astr), Q_ARG(QString, bstr));
    QCOMPARE(concatenated, u"foobar"_s);
}

void tst_QmlCppCodegen::functionCallOnNamespaced()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/themergood.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->property("i").toInt(), 12);
    }

    {
        QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/themerbad.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->property("r"), QVariant::fromValue(QRectF(5.0, 10.0, 1.0, 1.0)));
    }
}

void tst_QmlCppCodegen::functionLookup()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/functionLookup.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    const QVariant foo = o->property("bar");
    QCOMPARE(foo.metaType(), QMetaType::fromType<QJSValue>());
    const QJSManagedValue method(engine.toScriptValue(foo), &engine);
    QVERIFY(method.isFunction());
    const QJSValue result = method.call();
    QVERIFY(result.isString());
    QCOMPARE(result.toString(), QStringLiteral("a99"));
}

void tst_QmlCppCodegen::functionReturningVoid()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/functionReturningVoid.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    // It should be able to call the methods and wrap the void values into invalid QVariants,
    // without crashing.
    QVERIFY(o->metaObject()->indexOfProperty("aa") >= 0);
    QVERIFY(o->metaObject()->indexOfProperty("bb") >= 0);
    QVERIFY(!o->property("aa").isValid());
    QVERIFY(!o->property("bb").isValid());
}

void tst_QmlCppCodegen::functionTakingVar()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/functionTakingVar.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QVERIFY(!o->property("c").isValid());

    int value = 11;
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(&engine);
    void *args[] = { nullptr, reinterpret_cast<void *>(std::addressof(value)) };
    QMetaType types[] = { QMetaType::fromType<void>(), QMetaType::fromType<std::decay_t<int>>() };
    e->executeRuntimeFunction(document, 0, o.data(), 1, args, types);

    QCOMPARE(o->property("c"), QVariant::fromValue<int>(11));
}

void tst_QmlCppCodegen::getLookupOfScript()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/NotificationItem.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"heading"_s);
}

void tst_QmlCppCodegen::getOptionalLookup_data()
{
    QTest::addColumn<QString>("propertyName");
    QTest::addColumn<QVariant>("expected");

    // Objects
    QTest::addRow("int on object") << u"to1"_s << QVariant(5);
    QTest::addRow("string on object") << u"to2"_s << QVariant(u"6"_s);
    QTest::addRow("object on object") << u"to3"_s << QVariant::fromValue<QObject *>(nullptr);
    QTest::addRow("int on null") << u"to4"_s << QVariant(); // undefined
    QTest::addRow("any on undefined as object") << u"to5"_s << QVariant(); // undefined
    QTest::addRow("int on string on object") << u"to6"_s << QVariant(1);

    // Value Types
    QTest::addRow("int on rect") << u"tv1"_s << QVariant(50);
    QTest::addRow("int on point") << u"tv2"_s << QVariant(-10);
    QTest::addRow("int on undefined as point") << u"tv4"_s << QVariant(); // undefined

    // Enums
    QTest::addRow("enum on object") << u"te1"_s << QVariant(1);
    QTest::addRow("enum on type") << u"te2"_s << QVariant(1);
    QTest::addRow("enums comparison 1") << u"te3"_s << QVariant(false);
    QTest::addRow("enums comparison 2") << u"te4"_s << QVariant(true);

    // Complex chains
    QTest::addRow("mixed 1") << u"tc1"_s << QVariant(-10);
    QTest::addRow("mixed 2") << u"tc2"_s << QVariant(0);
    QTest::addRow("early out 1") << u"tc4"_s << QVariant(); // undefined
    QTest::addRow("early out 2") << u"tc5"_s << QVariant(); // undefined
}

void tst_QmlCppCodegen::getOptionalLookup()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/getOptionalLookup.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QFETCH(QString, propertyName);
    QFETCH(QVariant, expected);

    QVariant actual = o->property(propertyName.toLocal8Bit());
    QCOMPARE(actual, expected);
}

void tst_QmlCppCodegen::getOptionalLookupNonVoidableBase()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/GetOptionalLookupNonVoidableBase.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
}

void tst_QmlCppCodegen::getOptionalLookupOnQJSValueNonStrict()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/GetOptionalLookupOnQJSValueNonStrict.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QVERIFY(o->property("b").toBool());
}

void tst_QmlCppCodegen::getOptionalLookupShadowed()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/GetOptionalLookupShadowed.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("res").toString(), u"a"_s);
}

void tst_QmlCppCodegen::globals()
{
    QQmlEngine engine;
    int exitCode = -1;
    QObject::connect(&engine, &QQmlEngine::exit, [&](int code) { exitCode = code; });
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/globals.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    const QByteArray message = QByteArray("Start 2 ") + arg1();
    QTest::ignoreMessage(QtDebugMsg, message.constData());

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QTRY_COMPARE(exitCode, 0);

    QObject *application = qvariant_cast<QObject *>(object->property("application"));
    QVERIFY(application);
    QCOMPARE(QString::fromUtf8(application->metaObject()->className()),
             u"QQuickApplication"_s);

    QTest::ignoreMessage(QtDebugMsg, "End");
    QMetaObject::invokeMethod(application, "aboutToQuit");

    const QVariant somewhere = object->property("somewhere");
    QCOMPARE(somewhere.userType(), QMetaType::QUrl);
    QCOMPARE(qvariant_cast<QUrl>(somewhere).toString(), u"qrc:/somewhere/else.qml"_s);

    const QVariant somewhereString = object->property("somewhereString");
    QCOMPARE(somewhereString.userType(), QMetaType::QString);
    QCOMPARE(somewhereString.toString(), u"qrc:/somewhere/else.qml"_s);

    const QVariant plain = object->property("plain");
    QCOMPARE(plain.userType(), QMetaType::QUrl);
    QCOMPARE(qvariant_cast<QUrl>(plain).toString(), u"/not/here.qml"_s);
}

void tst_QmlCppCodegen::idAccess()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/idAccess.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QVERIFY(object->property("y").toInt() != 48);
    QCOMPARE(object->property("y").toInt(), 12);
    object->setProperty("z", 13);
    QCOMPARE(object->property("y").toInt(), 13);
    object->setProperty("x", QVariant::fromValue(333));
    QCOMPARE(object->property("y").toInt(), 48);

    // The binding was broken by setting the property
    object->setProperty("z", 14);
    QCOMPARE(object->property("y").toInt(), 48);

    QObject *ttt = qmlContext(object.data())->objectForName(u"ttt"_s);
    QFont f = qvariant_cast<QFont>(ttt->property("font"));
    QCOMPARE(f.pointSize(), 22);

    QObject::connect(object.data(), &QObject::objectNameChanged, ttt, [&](){
        ttt->setParent(nullptr);
        QJSEngine::setObjectOwnership(ttt, QJSEngine::CppOwnership);
        object.reset(ttt);
    });

    QVERIFY(object->objectName().isEmpty());
    QVERIFY(ttt->objectName().isEmpty());
    ttt->setProperty("text", u"kill"_s);
    QCOMPARE(object.data(), ttt);
    QCOMPARE(ttt->objectName(), u"context"_s);
}

void tst_QmlCppCodegen::ignoredFunctionReturn()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ignoredFunctionReturn.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
}

void tst_QmlCppCodegen::importsFromImportPath()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/importsFromImportPath.qml"_s));

    // We might propagate the import path, eventually, but for now instantiating is not important.
    // If the compiler accepts the file, it's probably fine.
    QVERIFY(component.isError());
    QCOMPARE(component.errorString(),
             u"qrc:/qt/qml/TestTypes/importsFromImportPath.qml:1 module \"Module\" is not installed\n"_s);
}

void tst_QmlCppCodegen::inPlaceDecrement()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/dialog.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QObject *header = qvariant_cast<QObject *>(object->property("header"));
    QVERIFY(header);
    QObject *background = qvariant_cast<QObject *>(header->property("background"));
    QObject *parent = qvariant_cast<QObject *>(background->property("parent"));

    QCOMPARE(background->property("width").toInt(), parent->property("width").toInt() + 1);
    QCOMPARE(background->property("height").toInt(), parent->property("height").toInt() - 1);

    QVERIFY(object->setProperty("width", QVariant::fromValue(17)));
    QVERIFY(parent->property("width").toInt() > 0);
    QVERIFY(object->setProperty("height", QVariant::fromValue(53)));
    QVERIFY(parent->property("height").toInt() > 0);

    QCOMPARE(background->property("width").toInt(), parent->property("width").toInt() + 1);
    QCOMPARE(background->property("height").toInt(), parent->property("height").toInt() - 1);

    QCOMPARE(object->property("a").toInt(), 1024);
}

void tst_QmlCppCodegen::inaccessibleProperty()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/versionmismatch.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("c").toInt(), 5);
}

void tst_QmlCppCodegen::indirectlyShadowable()
{
    QQmlEngine engine;

    const QString url = u"qrc:/qt/qml/TestTypes/indirectlyShadowable.qml"_s;
    QQmlComponent c(&engine, QUrl(url));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    const auto verifyShadowable = [&](const QString &objectName) {
        QObject *outer = o->property("outer").value<QObject *>();
        QVERIFY(outer);
        QObject *inner = outer->property("inner").value<QObject *>();
        QVERIFY(inner);
        QObject *shadowable = inner->property("shadowable").value<QObject *>();
        QVERIFY(shadowable);
        QCOMPARE(shadowable->objectName(), objectName);
    };

    const auto verifyNotShadowable = [&](const QString &objectName) {
        QObject *notShadowable = o->property("notShadowable").value<QObject *>();
        QCOMPARE(notShadowable->objectName(), objectName);
    };

    const auto verifyEvil = [&]() {
        QObject *outer = o->property("outer").value<QObject *>();
        QVERIFY(outer);
        QCOMPARE(outer->property("inner").toString(), u"evil"_s);
    };

    verifyShadowable(u"shadowable"_s);
    verifyNotShadowable(u"notShadowable"_s);

    QMetaObject::invokeMethod(o.data(), "setInnerShadowable");

    verifyShadowable(u"self"_s);
    verifyNotShadowable(u"notShadowable"_s);

    QMetaObject::invokeMethod(o.data(), "getInnerShadowable");

    verifyShadowable(u"self"_s);
    verifyNotShadowable(u"self"_s);

    QMetaObject::invokeMethod(o.data(), "turnEvil");

    verifyEvil();
    verifyNotShadowable(u"self"_s);

    // Does not produce an error message because JavaScript.
    QMetaObject::invokeMethod(o.data(), "setInnerShadowable");

    verifyEvil();
    verifyNotShadowable(u"self"_s);

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + u":29: Error: Cannot assign [undefined] to QObject*"_s));
    QMetaObject::invokeMethod(o.data(), "getInnerShadowable");

    verifyEvil();
    verifyNotShadowable(u"self"_s);
}

void tst_QmlCppCodegen::infinities()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/infinities.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("positiveInfinity").toDouble(), std::numeric_limits<double>::infinity());
    QCOMPARE(o->property("negativeInfinity").toDouble(), -std::numeric_limits<double>::infinity());

    const double positiveZero = o->property("positiveZero").toDouble();
    QCOMPARE(positiveZero, 0.0);
    QVERIFY(!std::signbit(positiveZero));

    const double negativeZero = o->property("negativeZero").toDouble();
    QCOMPARE(negativeZero, -0.0);
    QVERIFY(std::signbit(negativeZero));

    QVERIFY(qIsNaN(o->property("naN").toDouble()));
}

void tst_QmlCppCodegen::infinitiesToInt()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/infinitiesToInt.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    const char *props[] = {"a", "b", "c"};
    for (const char *prop : props) {
        const QVariant i = o->property(prop);
        QCOMPARE(i.metaType(), QMetaType::fromType<int>());
        bool ok = false;
        QCOMPARE(i.toInt(&ok), 0);
        QVERIFY(ok);
    }
}

void tst_QmlCppCodegen::innerObjectNonShadowable()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ownProperty.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(rootObject->objectName(), u"foo"_s);
}

class HandleHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *handle MEMBER m_handle)

private:
    QQmlComponent *m_handle = nullptr;
};

void tst_QmlCppCodegen::insertContextOnInvalidType()
{
    qmlRegisterType<HandleHandler>("Handlerei", 1, 0, "HandleHandler");

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/insertContextOnInvalidType.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg,
            "qrc:/qt/qml/TestTypes/insertContextOnInvalidType.qml:5: "
            "ReferenceError: handleDelegate is not defined");
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    const char *outter = "handleDelegateOutter";
    QVERIFY(rootObject->metaObject()->indexOfProperty(outter) != -1);
    QVERIFY(!rootObject->property(outter).isValid());
}

void tst_QmlCppCodegen::intEnumCompare()
{
    QQmlEngine engine;
    {
        QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/intEnumCompare.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->property("a").toBool(), true);
        QCOMPARE(o->property("b").toBool(), false);
        QCOMPARE(o->property("c").toBool(), true);
        QCOMPARE(o->property("d").toBool(), false);
    }

    {
        // We cannot use Qt.red in QML because it's lower case.
        QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/enumInvalid.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);
        QCOMPARE(o->property("c").toBool(), true);
        QCOMPARE(o->property("d").toBool(), false);
    }
}

void tst_QmlCppCodegen::intOverflow()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/intOverflow.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("a").toDouble(), 1.09951162778e+12);
    QCOMPARE(object->property("b").toInt(), 5);
}

void tst_QmlCppCodegen::intToEnum()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/intToEnum.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    MyType *m = qobject_cast<MyType *>(o.data());
    QCOMPARE(m->a(), MyType::D);
    QCOMPARE(m->property("b").toInt(), 24);
}

void tst_QmlCppCodegen::interceptor()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/interceptor.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("x").toInt(), 100);
    QCOMPARE(object->property("y").toInt(), 100);

    QVERIFY(object->property("width").toInt() != 200);
    QVERIFY(object->property("height").toInt() != 200);
    QVERIFY(object->property("qProperty1").toInt() != 300);
    QVERIFY(object->property("qProperty2").toInt() != 300);
    QTRY_COMPARE(object->property("width").toInt(), 200);
    QTRY_COMPARE(object->property("height").toInt(), 200);
    QTRY_COMPARE(object->property("qProperty1").toInt(), 300);
    QTRY_COMPARE(object->property("qProperty2").toInt(), 300);
}

void tst_QmlCppCodegen::interestingFiles()
{
    QFETCH(QString, file);
    QFETCH(bool, isValid);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/%1"_s.arg(file)));
    if (isValid) {
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.create());
        QVERIFY(!object.isNull());
    } else {
        QVERIFY(component.isError());
    }
}

void tst_QmlCppCodegen::interestingFiles_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("isValid");

    QTest::addRow("conversions2") << u"conversions2.qml"_s << true;
    QTest::addRow("TestCase") << u"TestCase.qml"_s << true;
    QTest::addRow("layouts") << u"layouts.qml"_s << true;
    QTest::addRow("interactive") << u"interactive.qml"_s << true;
    QTest::addRow("Panel") << u"Panel.qml"_s << true;
    QTest::addRow("ProgressBar") << u"ProgressBar/ProgressBar.ui.qml"_s << true;
    QTest::addRow("Root") << u"ProgressBar/Root.qml"_s << true;
    QTest::addRow("noscope") << u"noscope.qml"_s << true;
    QTest::addRow("dynamicscene") << u"dynamicscene.qml"_s << true;
    QTest::addRow("curlygrouped") << u"curlygrouped.qml"_s << true;
    QTest::addRow("cycleHead") << u"cycleHead.qml"_s << false;
    QTest::addRow("deadStoreLoop") << u"deadStoreLoop.qml"_s << true;
    QTest::addRow("moveRegVoid") << u"moveRegVoid.qml"_s << true;
}

void tst_QmlCppCodegen::internalConversion()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/internalConversion.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QObject *offset = obj->property("offset").value<QObject *>();
    QVERIFY(offset);

    QCOMPARE(offset->objectName(), "hello"_L1);
    QCOMPARE(offset->property("mark").toString(), "hello"_L1);
}

void tst_QmlCppCodegen::invalidPropertyType()
{
    // Invisible on purpose
    qmlRegisterType<MyCppType>("App", 1, 0, "MyCppType");

    QQmlEngine engine;
    QQmlComponent okComponent(&engine, QUrl(u"qrc:/qt/qml/TestTypes/OkType.qml"_s));
    QVERIFY2(okComponent.isReady(), qPrintable(okComponent.errorString()));
    QScopedPointer<QObject> picker(okComponent.create());
    QVERIFY2(!picker.isNull(), qPrintable(okComponent.errorString()));
    QObject *inner = qmlContext(picker.data())->objectForName(u"inner"_s);
    QVERIFY(inner);
    MyCppType *myCppType = qobject_cast<MyCppType *>(inner);
    QVERIFY(myCppType);
    QVERIFY(!myCppType->useListDelegate());

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/BadType.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.createWithInitialProperties(
            QVariantMap {{u"picker"_s, QVariant::fromValue(picker.data())}}));
    QVERIFY2(!o.isNull(), qPrintable(c.errorString()));
    QVERIFY(!myCppType->useListDelegate());

    o->setProperty("useListDelegate", QVariant::fromValue<bool>(true));
    QVERIFY(myCppType->useListDelegate());
}

void tst_QmlCppCodegen::invalidateCompositeType()
{
    for (int i = 0; i < 3; ++i) {
        QQmlEngine engine;
        QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/invalidateCompositeType.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(o);

        QObject *d = o->property("d").value<QObject *>();
        QVERIFY(d);
        QCOMPARE(o->property("d2").value<QObject *>(), d);

        QObject *group = d->property("child2").value<QObject *>();
        QVERIFY(group);
        QCOMPARE(o->property("d3").value<QObject *>(), group);
    }
}

void tst_QmlCppCodegen::invisibleBase()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/invisibleBase.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(qvariant_cast<QObject *>(o->property("n")), o.data());
}

void tst_QmlCppCodegen::invisibleListElementType()
{
    qmlRegisterType<InvisibleListElementType>("Invisible", 1, 0, "InvisibleListElement");
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/invisibleListElementType.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *a = o->property("a").value<QObject *>();
    QVERIFY(a);

    const QVariant x = a->property("x");
    QCOMPARE(x.metaType(), QMetaType::fromType<QQmlListReference>());
    const QQmlListReference ref = x.value<QQmlListReference>();
    QVERIFY(ref.isValid());
    QCOMPARE(ref.size(), 0);
}

void tst_QmlCppCodegen::invisibleSingleton()
{
    // We may have seen Style.qml as singleton before, which would make the assignment pass.
    // So let's flush the type registry.
    qmlClearTypeRegistrations();

    QQmlEngine engine;
    const QUrl copy(u"qrc:/qt/qml/HiddenTestTypes/hidden/Main.qml"_s);
    QQmlComponent c(&engine, copy);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
                QtWarningMsg,
                "qrc:/qt/qml/HiddenTestTypes/hidden/Main.qml:4:5: "
                "Unable to assign [undefined] to QColor");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("c"), QVariant(QMetaType::fromName("QColor")));
}

void tst_QmlCppCodegen::invisibleTypes()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/invisibleTypes.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QObject *singleton = qvariant_cast<QObject *>(o->property("singleton"));
    QVERIFY(singleton != nullptr);
    QCOMPARE(singleton->metaObject()->className(), "SingletonModel");

    QObject *attached = qvariant_cast<QObject *>(o->property("attached"));
    QVERIFY(attached != nullptr);
    QCOMPARE(attached->metaObject()->className(), "AttachedAttached");

// TODO: This doesn't work in interpreted mode:
//    const QMetaObject *meta = qvariant_cast<const QMetaObject *>(o->property("metaobject"));
//    QVERIFY(meta != nullptr);
//    QCOMPARE(meta->className(), "DerivedFromInvisible");
}

void tst_QmlCppCodegen::iterateUnknownValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/iterateUnknownValue.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtDebugMsg, "one");
    QTest::ignoreMessage(QtDebugMsg, "two");
    QTest::ignoreMessage(QtDebugMsg, "three");

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::iteration()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/iteration.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->objectName(), "a345b345c345"_L1);
}

void tst_QmlCppCodegen::javaScriptArgument()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/javaScriptArgument.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("a").toDouble(), 4.0);
    QCOMPARE(o->property("b").toDouble(), 9.0);
    QCOMPARE(o->property("c").toString(), u"5t-1"_s);
    QCOMPARE(o->property("d").toString(), u"9"_s);
    QCOMPARE(o->property("e").toString(), u"10"_s);
    QCOMPARE(o->property("f").toString(), u"-10"_s);

    const QStringList scales {
        u"0 "_s, u"1 "_s, u"10 "_s, u"100 "_s, u"1000 "_s, u"9.77k"_s, u"97.7k"_s, u"977k"_s,
        u"9.54M"_s, u"95.4M"_s, u"954M"_s, u"9.31G"_s, u"93.1G"_s, u"931G"_s, u"9.09T"_s, u"-1 "_s,
        u"-10 "_s, u"-100 "_s, u"-1000 "_s, u"-9.77k"_s, u"-97.7k"_s, u"-977k"_s, u"-9.54M"_s,
        u"-95.4M"_s, u"-954M"_s, u"-9.31G"_s, u"-93.1G"_s, u"-931G"_s, u"-9.09T"_s
    };

    QCOMPARE(o->property("scales").value<QStringList>(), scales);

    double thing = 12.0;
    QString result;
    QMetaObject::invokeMethod(
                o.data(), "forwardArg", Q_RETURN_ARG(QString, result), Q_ARG(double, thing));
    QCOMPARE(result, u"12 ");
}

void tst_QmlCppCodegen::jsArrayMethods()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsArrayMethods.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QQmlComponent untyped(&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsArrayMethodsUntyped.qml"_s));
    QVERIFY2(untyped.isReady(), qPrintable(untyped.errorString()));
    QScopedPointer<QObject> check(untyped.create());
    QVERIFY(!check.isNull());

    check->setProperty("l1", object->property("l1"));
    check->setProperty("l2", object->property("l2"));
    check->setProperty("l3", object->property("l3"));

    QCOMPARE(object->property("listPropertyToString"), object->property("jsArrayToString"));
    QCOMPARE(object->property("listPropertyToString"), check->property("jsArrayToString"));

    QCOMPARE(object->property("listPropertyIncludes"), object->property("jsArrayIncludes"));
    QVERIFY(object->property("listPropertyIncludes").toBool());

    QCOMPARE(object->property("listPropertyJoin"), object->property("jsArrayJoin"));
    QCOMPARE(object->property("listPropertyJoin"), check->property("jsArrayJoin"));
    QVERIFY(object->property("listPropertyJoin").toString().contains(QStringLiteral("klaus")));

    QCOMPARE(object->property("listPropertyIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("listPropertyIndexOf").toInt(), 1);

    QCOMPARE(object->property("listPropertyLastIndexOf"), object->property("jsArrayLastIndexOf"));
    QCOMPARE(object->property("listPropertyLastIndexOf").toInt(), 5);

    QCOMPARE(check->property("pushAndJoin").toString(), QStringLiteral("A+B"));
}

void tst_QmlCppCodegen::jsArrayMethodsWithParams()
{
    QFETCH(int, i);
    QFETCH(int, j);
    QFETCH(int, k);
    QQmlEngine engine;
    QQmlComponent component
            (&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsArrayMethodsWithParams.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QQmlComponent untyped(
            &engine, QUrl(u"qrc:/qt/qml/TestTypes/jsArrayMethodsWithParamsUntyped.qml"_s));
    QVERIFY2(untyped.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.createWithInitialProperties({
            {QStringLiteral("i"), i},
            {QStringLiteral("j"), j},
            {QStringLiteral("k"), k}
    }));
    QVERIFY(!object.isNull());
    QScopedPointer<QObject> check(untyped.createWithInitialProperties({
            {QStringLiteral("i"), i},
            {QStringLiteral("j"), j},
            {QStringLiteral("k"), k}
    }));
    QVERIFY(!check.isNull());
    check->setProperty("l1", object->property("l1"));
    check->setProperty("l2", object->property("l2"));
    check->setProperty("l3", object->property("l3"));

    listsEqual(object.data(), object.data(), "Slice");
    listsEqual(object.data(), check.data(), "Slice");
    QCOMPARE(object->property("listPropertyIndexOf"), object->property("jsArrayIndexOf"));
    QCOMPARE(object->property("listPropertyIndexOf"), check->property("jsArrayIndexOf"));
    QCOMPARE(object->property("listPropertyLastIndexOf"), object->property("jsArrayLastIndexOf"));
    QCOMPARE(object->property("listPropertyLastIndexOf"), check->property("jsArrayLastIndexOf"));
}

void tst_QmlCppCodegen::jsArrayMethodsWithParams_data()
{
    QTest::addColumn<int>("i");
    QTest::addColumn<int>("j");
    QTest::addColumn<int>("k");

    const int indices[] = {
        std::numeric_limits<int>::min(),
        -10, -3, -2, -1, 0, 1, 2, 3, 10,
        std::numeric_limits<int>::max(),
    };

    // We cannot test the full cross product. So, take a random sample instead.
    const qsizetype numIndices = sizeof(indices) / sizeof(int);
    qsizetype seed = QRandomGenerator::global()->generate();
    const int numSamples = 4;
    for (int i = 0; i < numSamples; ++i) {
        seed = qHash(i, seed);
        const int vi = indices[qAbs(seed) % numIndices];
        for (int j = 0; j < numSamples; ++j) {
            seed = qHash(j, seed);
            const int vj = indices[qAbs(seed) % numIndices];
            for (int k = 0; k < numSamples; ++k) {
                seed = qHash(k, seed);
                const int vk = indices[qAbs(seed) % numIndices];
                const QString tag = QLatin1String("%1/%2/%3").arg(
                        QString::number(vi), QString::number(vj), QString::number(vk));
                QTest::newRow(qPrintable(tag)) << vi << vj << vk;

                // output all the tags so that we can find out
                // what combination caused a test to hang.
                qDebug().noquote() << "scheduling" << tag;
            }
        }
    }
}

void tst_QmlCppCodegen::jsImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsimport.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("value").toInt(), 42);
}

void tst_QmlCppCodegen::jsMathObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsMathObject.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QJSManagedValue math(engine.globalObject().property(QStringLiteral("Math")), &engine);

    const QMetaObject *metaObject = o->metaObject();

    QString name;
    for (double a : numbers) {
        for (double b : numbers) {
            o->setProperty("a", a);
            o->setProperty("b", b);
            for (int i = 0, end = metaObject->propertyCount(); i != end; ++i) {
                const QMetaProperty prop = metaObject->property(i);
                name = QString::fromUtf8(prop.name());

                if (!math.hasProperty(name))
                    continue;

                const double result = prop.read(o.data()).toDouble();
                if (name == QStringLiteral("random")) {
                    QVERIFY(result >= 0.0 && result < 1.0);
                    continue;
                }

                QJSManagedValue jsMethod(math.property(name), &engine);
                const double expected = jsMethod.call(QJSValueList {a, b}).toNumber();
                QCOMPARE(result, expected);
            }
        }
    }

    if (QTest::currentTestFailed())
        qDebug() << name << "failed.";
}

void tst_QmlCppCodegen::jsmoduleImport()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/jsmoduleimport.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("ok").toBool(), true);
    QVariant okFunc = object->property("okFunc");
    QCOMPARE(okFunc.metaType(), QMetaType::fromType<QJSValue>());
    QJSValue val = engine.toScriptValue(okFunc);
    QJSValue result = val.call();
    QVERIFY(result.isBool());
    QVERIFY(result.toBool());
}

void tst_QmlCppCodegen::jsonArrayToStringList()
{
    QQmlEngine engine;
    const QString urlString = u"qrc:/qt/qml/TestTypes/jsonArrayToStringList.qml"_s;
    QQmlComponent component(&engine, QUrl(urlString));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(
            QtWarningMsg,
            "Converting array value at position 0 from double to QStringList via QJSValue even "
            "though they are not directly convertible");
    QTest::ignoreMessage(
            QtWarningMsg,
            "Converting array value at position 2 from double to QStringList via QJSValue even "
            "though they are not directly convertible");
    QTest::ignoreMessage(
            QtWarningMsg,
            "Converting array value at position 3 from std::nullptr_t to QStringList via QJSValue "
            "even though they are not directly convertible");
    QTest::ignoreMessage(
            QtWarningMsg,
            "Converting array value at position 4 from std::nullptr_t to QStringList via QJSValue "
            "even though they are not directly convertible");

    QTest::ignoreMessage(QtDebugMsg, "json [1,aa,2,null,null]");
    QTest::ignoreMessage(QtDebugMsg, "strings [1,aa,2,null,null]");
    QTest::ignoreMessage(QtDebugMsg, "strings2 [1,aa,2,null,null]");

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QListProvider *provider = qobject_cast<QListProvider *>(object.data());
    QCOMPARE(provider->json(), QJsonDocument::fromJson("[1,\"aa\",2,null,null]").array());
    QCOMPARE(provider->strings(), (QStringList { u"1"_s, u"aa"_s, u"2"_s, u"null"_s, u"null"_s }));
    QCOMPARE(provider->property("strings2").toStringList(),
             (QStringList { u"1"_s, u"aa"_s, u"2"_s, u"null"_s, u"null"_s }));

    QCOMPARE(provider->stringStrings(), (QList<QStringList> {
        QStringList(),
        QStringList { u"aa"_s }, // QVariant::convert auto-converts QString to QStringList ...
        QStringList(),
        QStringList(),
        QStringList(),
    }));

    // Note1: Converting a generic array to string is basically ArrayPrototype.join().
    //        This produces an empty string for null. Try [null, null].join() for that.
    //        Converting a single null to string (in an element-by-element operation)
    //        produces the string "null". Try (null + "") for that. The inconsistency is
    //        in JavaScript itself, not in our implementation.
    // Note2: Converting an array to string produces an un-delimited string. If you
    //        convert a nested array to string, you don't see the boundaries anymore.
    //        Try [[1, 2],[3, 4]].join() for that. Again, that's JavaScript itself.

    provider->setJson(QJsonDocument::fromJson(R"([
        ["c", null, [99], {"x": 12}],
        [1, 2, 3, "bbb", [16, "a", null]]
    ])").array());

    QCOMPARE(provider->strings(),
             (QStringList { u"c,,99,[object Object]"_s, u"1,2,3,bbb,16,a,"_s }));
    QCOMPARE(provider->property("strings2").toStringList(),
             (QStringList { u"c,,99,[object Object]"_s, u"1,2,3,bbb,16,a,"_s }));
    QCOMPARE(provider->stringStrings(), (QList<QStringList> {
        QStringList { u"c"_s, u"null"_s, u"99"_s, u"[object Object]"_s },
        QStringList { u"1"_s, u"2"_s, u"3"_s, u"bbb"_s, u"16,a,"_s}
    }));
}

void tst_QmlCppCodegen::lengthAccessArraySequenceCompat()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ArraySequenceLengthInterop.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("length").toInt(), 100);
}

void tst_QmlCppCodegen::letAndConst()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/letAndConst.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"ab"_s);
}

void tst_QmlCppCodegen::listAsArgument()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listAsArgument.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->property("i").toInt(), 4);
    QCOMPARE(o->property("j").toInt(), 2);
    QCOMPARE(o->property("i1").toInt(), 2);
    QCOMPARE(o->property("i2").toInt(), 4);
    QCOMPARE(o->property("d").value<QObject *>()->objectName(), u"this one"_s);

    int singleInt = 0;
    QList<int> moreInts;
    QMetaObject::invokeMethod(o.data(), "returnInts1", Q_RETURN_ARG(QList<int>, moreInts));
    QCOMPARE(moreInts, QList<int>({5, 4, 3, 2, 1}));
    QMetaObject::invokeMethod(o.data(), "selectSecondInt", Q_RETURN_ARG(int, singleInt), Q_ARG(QList<int>, moreInts));
    QCOMPARE(singleInt, 4);
}

void tst_QmlCppCodegen::listConversion()
{
    QQmlEngine e;
    QQmlComponent component(&e, QUrl(u"qrc:/qt/qml/TestTypes/listConversion.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(u"Cannot append QObject\\(0x[0-9a-f]+, name = \"a\"\\) "
                                   "to a QML list of QQmlBind\\*"_s));
    QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(u"Cannot append QObject\\(0x[0-9a-f]+, name = \"b\"\\) "
                                   "to a QML list of QQmlBind\\*"_s));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQmlListProperty<QObject> list = o->property("o").value<QQmlListProperty<QObject>>();
    QCOMPARE(list.count(&list), 3);
    for (int i = 0; i < 3; ++i) {
        QObject *entry = list.at(&list, i);
        Person *person = qobject_cast<Person *>(entry);
        QVERIFY(person);
        QCOMPARE(person->name(), u"Horst %1"_s.arg(i + 1));
    }

    QStringList strings = o->property("s").value<QStringList>();
    QCOMPARE(strings, QStringList({u"Horst 1"_s, u"Horst 2"_s, u"Horst 3"_s}));

    QVariantList vars = o->property("v").toList();
    QCOMPARE(vars, QVariantList({
        QString(),
        QVariant::fromValue<qsizetype>(3),
        QVariant::fromValue<Person *>(nullptr)
    }));

    QCOMPARE(o->property("numbers").value<QList<int>>(), (QList<int>{1, 2}));
    auto objects = o->property("objects").value<QQmlListProperty<QObject>>();
    QCOMPARE(objects.count(&objects), 2);

    const QObject *a = objects.at(&objects, 0);
    QVERIFY(a);
    QCOMPARE(a->objectName(), u"a"_s);

    const QObject *b = objects.at(&objects, 1);
    QVERIFY(b);
    QCOMPARE(b->objectName(), u"b"_s);

    auto bindings = o->property("bindings").value<QQmlListProperty<QQmlBind>>();
    QCOMPARE(bindings.count(&bindings), 2);

    const QQmlBind *c = bindings.at(&bindings, 0);
    QVERIFY(c);
    QCOMPARE(c->objectName(), u"c"_s);

    const QQmlBind *d = bindings.at(&bindings, 1);
    QVERIFY(d);
    QCOMPARE(d->objectName(), u"d"_s);

    auto objectsFromBindings = o->property("objectsFromBindings").value<QQmlListProperty<QObject>>();
    QCOMPARE(objectsFromBindings.count(&objectsFromBindings), 2);
    QCOMPARE(objectsFromBindings.at(&objectsFromBindings, 0), c);
    QCOMPARE(objectsFromBindings.at(&objectsFromBindings, 1), d);

    auto nulls = o->property("nulls").value<QQmlListProperty<QQmlBind>>();
    QCOMPARE(nulls.count(&nulls), 2);
    QCOMPARE(nulls.at(&nulls, 0), nullptr);
    QCOMPARE(nulls.at(&nulls, 1), nullptr);

    QObject *src = o->property("src").value<QObject *>();
    QVERIFY(src);

    QCOMPARE(src->property("numbers").value<QList<int>>(), (QList<int>{1, 2}));

    QQmlListProperty<QObject> srcObjects
            = src->property("objects").value<QQmlListProperty<QObject>>();
    QVERIFY(srcObjects.object != objects.object);
    const QObjectList oldObjects = objects.toList<QObjectList>();
    QCOMPARE(srcObjects.toList<QObjectList>(), oldObjects);

    QQmlListProperty<QQmlBind> srcBindings
            = src->property("bindings").value<QQmlListProperty<QQmlBind>>();
    QVERIFY(srcBindings.object != bindings.object);
    const QObjectList oldBindings = bindings.toList<QObjectList>();
    QCOMPARE(srcBindings.toList<QObjectList>(), oldBindings);

    QMetaObject::invokeMethod(o.data(), "shuffle");

    QCOMPARE(o->property("numbers").value<QList<int>>(), (QList<int>{1, 3}));

    // List properties stay intact
    QCOMPARE(objects.count(&objects), 2);
    QCOMPARE(objects.at(&objects, 1), o.data());
    QCOMPARE(bindings.count(&bindings), 2);
    QCOMPARE(bindings.at(&bindings, 1), nullptr);
    QCOMPARE(objectsFromBindings.count(&objectsFromBindings), 2);
    QCOMPARE(objectsFromBindings.at(&objectsFromBindings, 1), o.data());
    QCOMPARE(nulls.count(&nulls), 2);
    QCOMPARE(nulls.at(&nulls, 1), nullptr);

    // Changes did not propagate back to the source
    QCOMPARE(src->property("numbers").value<QList<int>>(), (QList<int>{1, 2}));
    QCOMPARE(srcObjects.toList<QObjectList>(), oldObjects);
    QCOMPARE(srcBindings.toList<QObjectList>(), oldBindings);
}

void tst_QmlCppCodegen::listIndices()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listIndices.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QQmlListReference list(o.data(), "items");
    QCOMPARE(list.count(), 3);
    for (int i = 0; i < 3; ++i)
        QCOMPARE(list.at(i), o.data());
    QCOMPARE(o->property("numItems").toInt(), 3);
    QCOMPARE(qvariant_cast<QObject *>(o->property("fractional")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(o->property("negativeZero")), o.data());
    QCOMPARE(qvariant_cast<QObject *>(o->property("infinity")), nullptr);
    QCOMPARE(qvariant_cast<QObject *>(o->property("nan")), nullptr);
}

void tst_QmlCppCodegen::listLength()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listlength.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("l").toInt(), 2);
}

void tst_QmlCppCodegen::listOfInvisible()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listOfInvisible.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("width").toDouble(), 27.0);
}

void tst_QmlCppCodegen::listPropertyAsModel()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listPropertyAsModel.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlListReference children(o.data(), "children");
    QCOMPARE(children.count(), 5);
}

void tst_QmlCppCodegen::listToString()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/listToString.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtDebugMsg, "[one,two]");
    QTest::ignoreMessage(QtDebugMsg, "one,two");
    QTest::ignoreMessage(QtDebugMsg, "[1,2]");
    QTest::ignoreMessage(QtDebugMsg, "1,2");
    QTest::ignoreMessage(
            QtDebugMsg,
            QRegularExpression(u"\\[QObject_QML_[0-9]+\\(0x[0-9a-f]+\\),"_s
                               u"QObject_QML_[0-9]+\\(0x[0-9a-f]+\\)\\]"_s));
    QTest::ignoreMessage(
            QtDebugMsg,
            QRegularExpression(u"QObject_QML_[0-9]+\\(0x[0-9a-f]+\\),"_s
                               u"QObject_QML_[0-9]+\\(0x[0-9a-f]+\\)"_s));

    QTest::ignoreMessage(QtDebugMsg, "[a,b]");

    QScopedPointer<QObject> o(c.create());
}

void tst_QmlCppCodegen::lotsOfRegisters()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/page.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const auto compare = [&]() {
        const qreal implicitBackgroundWidth = object->property("implicitBackgroundWidth").toDouble();
        const qreal leftInset = object->property("leftInset").toDouble();
        const qreal rightInset = object->property("rightInset").toDouble();
        const qreal implicitContentWidth = object->property("implicitContentWidth").toDouble();
        const qreal leftPadding = object->property("leftPadding").toDouble();
        const qreal rightPadding = object->property("rightPadding").toDouble();
        const qreal implicitFooterWidth = object->property("implicitFooterWidth").toDouble();
        const qreal implicitHeaderWidth = object->property("implicitHeaderWidth").toDouble();

        const qreal implicitWidth = object->property("implicitWidth").toDouble();
        QCOMPARE(implicitWidth, qMax(qMax(implicitBackgroundWidth + leftInset + rightInset,
                                     implicitContentWidth + leftPadding + rightPadding),
                                     qMax(implicitHeaderWidth, implicitFooterWidth)));
    };

    compare();

    const QList<const char *> props = {
        "leftInset", "rightInset", "contentWidth", "leftPadding", "rightPadding"
    };

    for (int i = 0; i < 100; ++i) {
        QVERIFY(object->setProperty(props[i % props.size()], (i * 17) % 512));
        compare();
    }
}

void tst_QmlCppCodegen::math()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/math.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("a").toInt(), 9);
    QCOMPARE(object->property("b").toDouble(), 50.0 / 22.0);
    QCOMPARE(object->property("c").toDouble(), std::atan(1.0) * 8.0);
}

void tst_QmlCppCodegen::mathMinMax()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/mathMinMax.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    // Math.max()
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "2");
    QTest::ignoreMessage(QtDebugMsg, "2");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "-1");

    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "2");
    QTest::ignoreMessage(QtDebugMsg, "2");
    QTest::ignoreMessage(QtDebugMsg, "9");

    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "0.002");
    QTest::ignoreMessage(QtDebugMsg, "5.4");
    QTest::ignoreMessage(QtDebugMsg, "NaN");
    QTest::ignoreMessage(QtDebugMsg, "Infinity");
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "0.08");
    QTest::ignoreMessage(QtDebugMsg, "Infinity");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "NaN");

    // Math.min()
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "1");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-1");

    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "-2");
    QTest::ignoreMessage(QtDebugMsg, "-2");
    QTest::ignoreMessage(QtDebugMsg, "0");

    QTest::ignoreMessage(QtDebugMsg, "0");
    QTest::ignoreMessage(QtDebugMsg, "-0.001");
    QTest::ignoreMessage(QtDebugMsg, "0.002");
    QTest::ignoreMessage(QtDebugMsg, "NaN");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-1");
    QTest::ignoreMessage(QtDebugMsg, "-8");
    QTest::ignoreMessage(QtDebugMsg, "NaN");

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::mathOperations()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/mathOperations.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    const QMetaObject *metaObject = o->metaObject();

    char t1;
    char t2;
    QString name;
    const auto guard = qScopeGuard([&]() {
        if (QTest::currentTestFailed()) {
            qDebug() << t1 << t2 << name << "failed on:";
            qDebug() << "doubles" << o->property("a").toDouble() << o->property("b").toDouble();
            qDebug() << "integers" << o->property("ia").toInt() << o->property("ib").toInt();
            qDebug() << "booleans" << o->property("ba").toBool() << o->property("bb").toBool();
        }
    });

    for (double a : numbers) {
        for (double b : numbers) {
            o->setProperty("a", a);
            o->setProperty("b", b);
            for (int i = 0, end = metaObject->propertyCount(); i != end; ++i) {
                const QMetaProperty prop = metaObject->property(i);
                const QByteArray propName = prop.name();

                if (propName.size() < 3 || propName == "objectName")
                    continue;

                t1 = propName[0];
                t2 = propName[1];
                name = QString::fromUtf8(propName.mid(2));

                double expected;

                switch (t2) {
                case 'd':
                case '_':
                    switch (t1) {
                    case 'd':
                        expected = jsEval<double, double>(a, b, name, &engine);
                        break;
                    case 'i':
                        expected = jsEval<int, double>(a, b, name, &engine);
                        break;
                    case 'b':
                        expected = jsEval<bool, double>(a, b, name, &engine);
                        break;
                    }
                    break;
                case 'i':
                    switch (t1) {
                    case 'd':
                        expected = jsEval<double, int>(a, b, name, &engine);
                        break;
                    case 'i':
                        expected = jsEval<int, int>(a, b, name, &engine);
                        break;
                    case 'b':
                        expected = jsEval<bool, int>(a, b, name, &engine);
                        break;
                    }
                    break;
                case 'b':
                    switch (t1) {
                    case 'd':
                        expected = jsEval<double, bool>(a, b, name, &engine);
                        break;
                    case 'i':
                        expected = jsEval<int, bool>(a, b, name, &engine);
                        break;
                    case 'b':
                        expected = jsEval<bool, bool>(a, b, name, &engine);
                        break;
                    }
                    break;
                }

                const double result = prop.read(o.data()).toDouble();
                QCOMPARE(result, expected);
            }
        }
    }
}

void tst_QmlCppCodegen::mathStaticProperties()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/mathStaticProperties.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    // Approximate values based on
    // https://262.ecma-international.org/14.0/#sec-value-properties-of-the-math-object
    QCOMPARE(object->property("e").toDouble(), 2.7182818284590452354);
    QCOMPARE(object->property("ln10").toDouble(), 2.302585092994046);
    QCOMPARE(object->property("ln2").toDouble(), 0.6931471805599453);
    QCOMPARE(object->property("log10e").toDouble(), 0.4342944819032518);
    QCOMPARE(object->property("log2e").toDouble(), 1.4426950408889634);
    QCOMPARE(object->property("pi").toDouble(), 3.1415926535897932);
    QCOMPARE(object->property("sqrt1_2").toDouble(), 0.7071067811865476);
    QCOMPARE(object->property("sqrt2").toDouble(), 1.4142135623730951);
}

void tst_QmlCppCodegen::mergedObjectReadWrite()
{
    QQmlEngine e;
    {
        QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/mergedObjectRead.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QTest::ignoreMessage(QtDebugMsg, "null");
        QTest::ignoreMessage(
                QtWarningMsg, QRegularExpression(u"TypeError: Cannot read property 'x' of null"_s));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
    }

    {
        QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/mergedObjectWrite.qml"_s));
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(
                        u"TypeError: Value is null and could not be converted to an object"_s));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
    }
}

void tst_QmlCppCodegen::methodOnListLookup()
{
    QQmlEngine engine;
    const QUrl url(u"qrc:/qt/qml/TestTypes/methodOnListLookup.qml"_s);
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->objectName(), u"no one");

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString()
                       + ":14: TypeError: Cannot call method 'getName' of undefined"_L1));
    QMetaObject::invokeMethod(o.data(), "boom");
}

void tst_QmlCppCodegen::methods()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/methods.qml"_s));
    QVERIFY(component.isReady());

    QTest::ignoreMessage(QtDebugMsg, "The Bar");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(u"TypeError: .* is not a function"_s));
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(obj);
    BirthdayParty *party(qobject_cast<BirthdayParty *>(obj.data()));

    QVERIFY(party && party->host());
    QCOMPARE(party->guestCount(), 5);

    bool foundGreen = false;
    bool foundFoo = false;
    for (int ii = 0; ii < party->guestCount(); ++ii) {
        if (party->guest(ii)->name() == u"William Green"_s)
            foundGreen = true;
        if (party->guest(ii)->name() == u"The Foo"_s)
            foundFoo = true;
    }

    QVERIFY(foundGreen);
    QVERIFY(foundFoo);

    QCOMPARE(obj->property("n1").toString(), u"onGurk"_s);
    QCOMPARE(obj->property("n2").toString(), u"onSemmeln"_s);
    QCOMPARE(obj->property("n3"), QVariant());

    {
        QVariant ret;
        obj->metaObject()->invokeMethod(obj.data(), "retrieveVar", Q_RETURN_ARG(QVariant, ret));
        QCOMPARE(ret.typeId(), QMetaType::QString);
        QCOMPARE(ret.toString(), u"Jack Smith"_s);
    }

    {
        QString ret;
        obj->metaObject()->invokeMethod(obj.data(), "retrieveString", Q_RETURN_ARG(QString, ret));
        QCOMPARE(ret, u"Jack Smith"_s);
    }

    QCOMPARE(party->host()->shoeSize(), 12);
    obj->metaObject()->invokeMethod(obj.data(), "storeElement");
    QCOMPARE(party->host()->shoeSize(), 13);
    QJSManagedValue v = engine.toManagedValue(obj->property("dresses"));
    QVERIFY(v.isArray());

    QJSManagedValue inner(v.property(2), &engine);
    QVERIFY(inner.isArray());
    QCOMPARE(inner.property(0).toInt(), 1);
    QCOMPARE(inner.property(1).toInt(), 2);
    QCOMPARE(inner.property(2).toInt(), 3);

    QCOMPARE(obj->property("enumValue").toInt(), 2);
}

void tst_QmlCppCodegen::modulePrefix()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/modulePrefix.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(rootObject->property("foo").toDateTime(), QDateTime(QDate(1911, 3, 4), QTime()));
    QCOMPARE(rootObject->property("bar").toDateTime(), QDateTime(QDate(1911, 3, 4), QTime()));
    QCOMPARE(rootObject->property("baz").toString(), QStringLiteral("ItIsTheSingleton"));
}

void tst_QmlCppCodegen::multiDirectory_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addRow("from qt_add_qml_module")
            << QUrl(u"qrc:/qt/qml/TestTypes/extra/extra.qml"_s);
#ifndef VERY_OLD_CMAKE
    QTest::addRow("from qt_target_qml_sources")
            << QUrl(u"qrc:/qt/qml/TestTypes/extra2/extra.qml"_s);
#endif
}

void tst_QmlCppCodegen::multiDirectory()
{
    QFETCH(QUrl, url);
    QQmlEngine engine;
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(rootObject->property("r").value<QRectF>(), QRectF(4, 6, 8, 10));
}

void tst_QmlCppCodegen::multiForeign()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/multiforeign.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"not here and not there"_s);
}

void tst_QmlCppCodegen::multiLookup()
{
    // Multiple lookups of singletons (Qt in this case) don't clash with one another.
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/immediateQuit.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    const QByteArray message = QByteArray("End: ") + arg1();
    QTest::ignoreMessage(QtDebugMsg, message.constData());

    QSignalSpy quitSpy(&engine, &QQmlEngine::quit);
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(quitSpy.size(), 1);
}

void tst_QmlCppCodegen::multiRedirect()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/multiRedirect.qml"_s));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->objectName(), u"green"_s);
}

void tst_QmlCppCodegen::multipleCtors()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/multipleCtors.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("wr").value<ValueTypeWithLength>().length(), 3);
    QCOMPARE(o->property("wp").value<ValueTypeWithLength>().length(), 11);
    QCOMPARE(o->property("wi").value<ValueTypeWithLength>().length(), 17);
    QCOMPARE(o->property("wo").value<ValueTypeWithLength>().length(), 5);
    QCOMPARE(o->property("wz").value<ValueTypeWithLength>().length(), 8);
}

void tst_QmlCppCodegen::namespaceWithEnum()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/namespaceWithEnum.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("i").toInt(), 2);
}

void tst_QmlCppCodegen::noBuiltinsImport()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/noBuiltinsImport.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("textureSize").value<QSizeF>(), QSizeF(1024, 1024));
}

void tst_QmlCppCodegen::noQQmlData()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/noQQmlData.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/noQQmlData.qml:7: TypeError: "
                                       "Cannot read property 'name' of null");
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    BirthdayParty *party = qobject_cast<BirthdayParty *>(root.data());
    QVERIFY(party != nullptr);

    QCOMPARE(party->host(), nullptr);
    QCOMPARE(party->property("n").toString(), QString());

    Person *host1 = new Person(party);
    party->setHost(host1);
    QCOMPARE(party->property("n").toString(), u"Bart in da house!"_s);
    host1->setName(u"Marge"_s);
    QCOMPARE(party->property("n").toString(), u"Marge in da house!"_s);

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/noQQmlData.qml:7: TypeError: "
                                       "Cannot read property 'name' of null");

    // Doesn't crash
    party->setHost(nullptr);

    // Lookups are initialized now, and we introduce an object without QQmlData
    Person *host2 = new Person(party);
    party->setHost(host2);
    QCOMPARE(party->property("n").toString(), u"Bart in da house!"_s);
    host2->setName(u"Homer"_s);
    QCOMPARE(party->property("n").toString(), u"Homer in da house!"_s);

    QMetaObject::invokeMethod(party, "burn");
    engine.collectGarbage();

    // Does not crash
    party->setProperty("inDaHouse", u" burns!"_s);

    // Mr Burns may or may not burn, depending on whether we use lookups.
    // If using lookups, the binding is aborted when we find the isQueuedForDeletion flag.
    // If reading the property directly, we don't have to care about it.
    QVERIFY(party->property("n").toString().startsWith(u"Mr Burns"_s));
}

void tst_QmlCppCodegen::nonNotifyable()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/nonNotifyable.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(qvariant_cast<QDateTime>(object->property("dayz")),
             QDateTime(QDate(2121, 1, 12), QTime()));
    QCOMPARE(qvariant_cast<QDateTime>(object->property("oParty")),
             QDateTime(QDate(2111, 12, 11), QTime()));
}

void tst_QmlCppCodegen::notEqualsInt()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/notEqualsInt.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QObject *t = qmlContext(o.data())->objectForName(u"t"_s);
    QVERIFY(t);
    QCOMPARE(t->property("text").toString(), u"Foo"_s);
    QMetaObject::invokeMethod(o.data(), "foo");
    QCOMPARE(t->property("text").toString(), u"Bar"_s);
}

void tst_QmlCppCodegen::notNotString()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/notNotString.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("notNotString").value<bool>(), false);
    o->setObjectName(u"a"_s);
    QCOMPARE(o->property("notNotString").value<bool>(), true);
}

void tst_QmlCppCodegen::nullAccess()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/nullAccess.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/qt/qml/TestTypes/nullAccess.qml:4:5: TypeError: "
                         "Cannot read property 'width' of null");
    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/qt/qml/TestTypes/nullAccess.qml:5:5: TypeError: "
                         "Cannot read property 'height' of null");
    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/qt/qml/TestTypes/nullAccess.qml:6: TypeError: Value is null and "
                         "could not be converted to an object");
    QScopedPointer<QObject> object(component.create());

    QCOMPARE(object->property("width").toDouble(), 0.0);
    QCOMPARE(object->property("height").toDouble(), 0.0);
}

void tst_QmlCppCodegen::nullAccessInsideSignalHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/nullAccessInsideSignalHandler.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QTest::ignoreMessage(QtWarningMsg,
                         "qrc:/qt/qml/TestTypes/nullAccessInsideSignalHandler.qml:15: ReferenceError: "
                         "text is not defined");
    QScopedPointer<QObject> object(component.create());
    QSignalSpy spy(object.data(), SIGNAL(say_hello()));
    QTRY_VERIFY(spy.size() > 0);
}


void tst_QmlCppCodegen::nullComparison()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/nullComparison.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("v").toInt(), 1);
    QCOMPARE(o->property("w").toInt(), 3);
    QCOMPARE(o->property("x").toInt(), 1);
    QCOMPARE(o->property("y").toInt(), 5);
    QCOMPARE(o->property("z").toInt(), 18);
}

void tst_QmlCppCodegen::nullishCoalescing_data()
{
    QTest::addColumn<QString>("propertyName");
    QTest::addColumn<QVariant>("expected");

    const auto undefinedValue = QVariant();
    const auto nullValue = QVariant::fromMetaType(QMetaType::fromType<std::nullptr_t>(), nullptr);

    QTest::addRow("trivial-good-int") << "p1" << QVariant(5);
    QTest::addRow("trivial-good-string") << "p2" << QVariant(u"6"_s);

    QTest::addRow("trivial-bad-undefined-undefined") << "p3" << undefinedValue;
    QTest::addRow("trivial-bad-undefined-null") << "p4" << nullValue;
    QTest::addRow("trivial-bad-undefined-int") << "p5" << QVariant(-1);
    QTest::addRow("trivial-bad-undefined-string") << "p6" << QVariant(u"-1"_s);

    QTest::addRow("trivial-bad-null-undefined") << "p7" << undefinedValue;
    QTest::addRow("trivial-bad-null-null") << "p8" << nullValue;
    QTest::addRow("trivial-bad-null-int") << "p9" << QVariant(-1);
    QTest::addRow("trivial-bad-null-string") << "p10" << QVariant(u"-1"_s);

    QTest::addRow("enum1") << "p11" << QVariant(1);

    QTest::addRow("multiple ?? int") << "p12" << QVariant(1);
    QTest::addRow("multiple ?? string") << "p13" << QVariant(u"1"_s);
    QTest::addRow("multiple ?? mixed2") << "p14" << QVariant(u"2"_s);
    QTest::addRow("multiple ?? mixed3") << "p15" << QVariant(1);

    QTest::addRow("optional + nullish bad") << "p16" << QVariant(-1);
    QTest::addRow("optional + nullish good") << "p17" << QVariant(5);
}

void tst_QmlCppCodegen::nullishCoalescing()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/nullishCoalescing.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QFETCH(QString, propertyName);
    QFETCH(QVariant, expected);

    QVariant actual = o->property(propertyName.toLocal8Bit());
    QCOMPARE(actual, expected);
}

void tst_QmlCppCodegen::numbersInJsPrimitive()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/numbersInJsPrimitive.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    const QList<qint64> zeroes
            = {0, 0, 0, 0, 0, 0, 0, 0};
    const QList<qint64> written
            = {35, 36, 37, 38, 39, 40, 41, 42};
    const QList<qint64> writtenNegative
            = {-35, 220, -37, 65498, -39, 4294967256, -41, 4294967254};
    const QList<QList<qint64>> writtenShuffled = {
        { -36, 219, -38, 65497, -40, 4294967255, -42, 4294967260 },
        { -37, 218, -39, 65496, -41, 4294967254, -36, 4294967259 },
        { -38, 217, -40, 65495, -42, 4294967260, -37, 4294967258 },
        { -39, 216, -41, 65494, -36, 4294967259, -38, 4294967257 },
        { -40, 215, -42, 65500, -37, 4294967258, -39, 4294967256 },
        { -41, 214, -36, 65499, -38, 4294967257, -40, 4294967255 },
        { -42, 220, -37, 65498, -39, 4294967256, -41, 4294967254 },
        { -36, 219, -38, 65497, -40, 4294967255, -42, 4294967260 },
    };

    const QList<qint64> stored
            = {50, 51, 1332, 1333, 1334, 1335, 1336, 1337};
    const QList<qint64> storedNegative
            = {-50, 205, -1332, 64203, -1334, 4294965961, -1336, 4294965959};
    const QList<QList<qint64>> storedShuffled = {
        { -51, 204, -1333, 64202, -1335, 4294965960, -1337, 4294967245 },
        { -52, 203, -1334, 64201, -1336, 4294965959, -51,   4294967244 },
        { -53, 202, -1335, 64200, -1337, 4294967245, -52,   4294967243 },
        { -54, 201, -1336, 64199, -51,   4294967244, -53,   4294967242 },
        { -55, 200, -1337, 65485, -52,   4294967243, -54,   4294967241 },
        { -56, 199, -51,   65484, -53,   4294967242, -55,   4294967240 },
        { -57, 205, -52,   65483, -54,   4294967241, -56,   4294967239 },
        { -51, 204, -53,   65482, -55,   4294967240, -57,   4294967245 },
    };

    QStringList asStrings(8);

    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(
                    o.data(), "readValueAsString",
                    Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
    }
    QCOMPARE(asStrings, convertToStrings(zeroes));

    QMetaObject::invokeMethod(o.data(), "writeValues");
    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(
                    o.data(), "readValueAsString",
                    Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
    }
    QCOMPARE(asStrings, convertToStrings(written));

    QMetaObject::invokeMethod(o.data(), "negateValues");
    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(
                    o.data(), "readValueAsString",
                    Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
    }
    QCOMPARE(asStrings, convertToStrings(writtenNegative));

    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(o.data(), "shuffleValues");
        for (int i = 0; i < 8; ++i) {
            QMetaObject::invokeMethod(
                        o.data(), "readValueAsString",
                        Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
        }
        QCOMPARE(asStrings, convertToStrings(writtenShuffled[i]));
    }

    QMetaObject::invokeMethod(o.data(), "storeValues");
    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(
                    o.data(), "readValueAsString",
                    Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
    }
    QCOMPARE(asStrings, convertToStrings(stored));

    QMetaObject::invokeMethod(o.data(), "negateValues");
    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(
                    o.data(), "readValueAsString",
                    Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
    }
    QCOMPARE(asStrings, convertToStrings(storedNegative));

    for (int i = 0; i < 8; ++i) {
        QMetaObject::invokeMethod(o.data(), "shuffleValues");
        for (int i = 0; i < 8; ++i) {
            QMetaObject::invokeMethod(
                        o.data(), "readValueAsString",
                        Q_RETURN_ARG(QString, asStrings[i]), Q_ARG(int, i));
        }
        QCOMPARE(asStrings, convertToStrings(storedShuffled[i]));
    }
}

void tst_QmlCppCodegen::objectInVar()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/objectInVar.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(qvariant_cast<QObject*>(o->property("thing")), o.data());

    bool result = false;
    QVERIFY(QMetaObject::invokeMethod(o.data(), "doThing", Q_RETURN_ARG(bool, result)));
    QVERIFY(result);

    o->setProperty("thing", QVariant::fromValue<std::nullptr_t>(nullptr));
    QVERIFY(QMetaObject::invokeMethod(o.data(), "doThing", Q_RETURN_ARG(bool, result)));
    QVERIFY(!result);
}

void tst_QmlCppCodegen::objectLookupOnListElement()
{
    QQmlEngine engine;

    const QUrl url(u"qrc:/qt/qml/TestTypes/objectLookupOnListElement.qml"_s);
    QQmlComponent c1(&engine, url);
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> object(c1.create());
    QVERIFY(!object.isNull());

    QList<int> zOrders;
    QMetaObject::invokeMethod(object.data(), "zOrders", Q_RETURN_ARG(QList<int>, zOrders));
    QCOMPARE(zOrders, (QList<int>{1, 0, 0}));
    object->setProperty("current", 1);
    QMetaObject::invokeMethod(object.data(), "zOrders", Q_RETURN_ARG(QList<int>, zOrders));
    QCOMPARE(zOrders, (QList<int>{0, 1, 0}));

    QMetaObject::invokeMethod(object.data(), "clearChildren");
    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString()
                           + u":21: TypeError: Cannot read property 'z' of undefined"_s));
    QMetaObject::invokeMethod(object.data(), "zOrders", Q_RETURN_ARG(QList<int>, zOrders));
    QCOMPARE(zOrders, (QList<int>()));
}

void tst_QmlCppCodegen::objectToString()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/toString.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/toString.qml:6: no");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("yes").toString(), u"yes yes"_s);
    QCOMPARE(o->property("no").toString(), u" no"_s); // throws, but that is ignored
}

void tst_QmlCppCodegen::objectWithStringListMethod()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/objectWithStringListMethod.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QTest::ignoreMessage(QtDebugMsg, "2");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::onAssignment()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/pressAndHoldButton.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QCOMPARE(object->property("pressed").toBool(), false);
    QCOMPARE(object->property("scale").toDouble(), 1.0);

    object->metaObject()->invokeMethod(object.data(), "press");
    QTRY_COMPARE(object->property("pressed").toBool(), true);
    QCOMPARE(object->property("scale").toDouble(), 0.9);

    object->metaObject()->invokeMethod(object.data(), "release");
    QCOMPARE(object->property("pressed").toBool(), false);
    QCOMPARE(object->property("scale").toDouble(), 1.0);
}

void tst_QmlCppCodegen::optionalComparison()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/optionalComparison.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("found").toInt(), 1);
    QCOMPARE(object->property("foundStrict").toInt(), 1);
    QCOMPARE(object->property("foundNot").toInt(), 2);
    QCOMPARE(object->property("foundStrictNot").toInt(), 2);

    // this === this, null === null (x4), undefined === undefined
    QCOMPARE(object->property("undefinedEqualsUndefined").toInt(), 6);

    QCOMPARE(object->property("optionalNull").toBool(), true);
    object->setObjectName("foo"_L1);
    QCOMPARE(object->property("optionalNull").toBool(), false);
}

void tst_QmlCppCodegen::outOfBoundsArray()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/outOfBounds.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(QtDebugMsg, "oob undefined");
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QVERIFY(object->metaObject()->indexOfProperty("oob") > 0);
    QVERIFY(!object->property("oob").isValid());
    const QVariant oob2 = object->property("oob2");
    QCOMPARE(oob2.metaType(), QMetaType::fromType<QObject *>());
    QCOMPARE(oob2.value<QObject *>(), nullptr);
}

void tst_QmlCppCodegen::overriddenProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/childobject.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QObject *child = object->property("child").value<QObject *>();
    QVERIFY(child);

    QCOMPARE(object->objectName(), u"kraut"_s);
    QCOMPARE(object->property("doneThing").toInt(), 5);
    QCOMPARE(object->property("usingFinal").toInt(), 5);

    auto checkAssignment = [&]() {
        const QString newName = u"worscht"_s;
        QMetaObject::invokeMethod(object.data(), "setChildObjectName", Q_ARG(QString, newName));
        QCOMPARE(object->objectName(), newName);
    };
    checkAssignment();

    QMetaObject::invokeMethod(child, "doString");
    QCOMPARE(child->objectName(), u"string"_s);
    QMetaObject::invokeMethod(child, "doNumber");
    QCOMPARE(child->objectName(), u"double"_s);
    QMetaObject::invokeMethod(child, "doArray");
    QCOMPARE(child->objectName(), u"javaScript"_s);

    QMetaObject::invokeMethod(child, "doString2");
    QCOMPARE(child->objectName(), u"string"_s);
    QMetaObject::invokeMethod(child, "doNumber2");
    QCOMPARE(child->objectName(), u"double"_s);
    QMetaObject::invokeMethod(child, "doArray2");
    QCOMPARE(child->objectName(), u"javaScript"_s);

    QMetaObject::invokeMethod(child, "doFoo");
    QCOMPARE(child->objectName(), u"ObjectWithMethod"_s);

    ObjectWithMethod *benign = new ObjectWithMethod(object.data());
    benign->theThing = 10;
    benign->setObjectName(u"cabbage"_s);
    object->setProperty("child", QVariant::fromValue(benign));
    QCOMPARE(object->objectName(), u"cabbage"_s);
    checkAssignment();
    QCOMPARE(object->property("doneThing").toInt(), 10);
    QCOMPARE(object->property("usingFinal").toInt(), 10);

    OverriddenObjectName *evil = new OverriddenObjectName(object.data());
    QTest::ignoreMessage(QtWarningMsg,
                         "Final member fff is overridden in class OverriddenObjectName. "
                         "The override won't be used.");
    object->setProperty("child", QVariant::fromValue(evil));

    QCOMPARE(object->objectName(), u"borschtsch"_s);

    checkAssignment();
    QCOMPARE(object->property("doneThing").toInt(), 7);
    QCOMPARE(object->property("usingFinal").toInt(), 5);
}

void tst_QmlCppCodegen::ownPropertiesNonShadowable()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/overriddenMember.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(rootObject->property("ppp").toInt(), 16);
    QCOMPARE(rootObject->property("ppp2").toInt(), 9);
    QCOMPARE(rootObject->property("ppp3").toInt(), 12);
}

void tst_QmlCppCodegen::parentProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/parentProp.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("c").toInt(), 11);
    QCOMPARE(object->property("i").toInt(), 22);
    object->setProperty("a", QVariant::fromValue(22));
    QCOMPARE(object->property("c").toInt(), 28);
    object->setProperty("implicitWidth", QVariant::fromValue(14));
    QCOMPARE(object->property("i").toInt(), 26);

    QObject *child = qmlContext(object.data())->objectForName(u"child"_s);
    QObject *sibling = qmlContext(object.data())->objectForName(u"sibling"_s);
    QObject *evil = qmlContext(object.data())->objectForName(u"evil"_s);

    child->setProperty("parent", QVariant::fromValue(sibling));

    QCOMPARE(child->property("b").toInt(), 0);
    QCOMPARE(child->property("i").toInt(), 28);
    QCOMPARE(object->property("i").toInt(), 56);

    child->setProperty("parent", QVariant::fromValue(evil));

    QCOMPARE(child->property("b").toInt(), 5994);
    QCOMPARE(object->property("c").toInt(), 5996);

    QCOMPARE(child->property("i").toInt(), 443);
    QCOMPARE(object->property("i").toInt(), 886);

    {
        QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/specificParent.qml"_s));

        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> rootObject(component.create());
        QVERIFY(rootObject);

        QCOMPARE(rootObject->property("a").toReal(), 77.0);
    }
}

void tst_QmlCppCodegen::popContextAfterRet()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/popContextAfterRet.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->objectName(), QString());
    o->setProperty("stackViewDepth", 1);
    QCOMPARE(o->objectName(), u"backgroundImage"_s);
    o->setProperty("stackViewDepth", 2);
    QCOMPARE(o->objectName(), u"backgroundBlur"_s);
    o->setProperty("stackViewDepth", 1);
    QCOMPARE(o->objectName(), u"backgroundImage"_s);
}

void tst_QmlCppCodegen::prefixedType()
{
    QQmlEngine engine;

    // We need to add an import path here because we cannot namespace the implicit import.
    // The implicit import is what we use for all the other tests, even if we explicitly
    // import TestTypes. That is because the TestTypes module is in a subdirectory "data".
    engine.addImportPath(u":/"_s);

    const QUrl document(u"qrc:/qt/qml/TestTypes/prefixedMetaType.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("state").toInt(), 2);
    QVERIFY(qvariant_cast<QObject *>(o->property("a")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(o->property("b")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(o->property("c")) == nullptr);

    QVERIFY(qvariant_cast<QObject *>(o->property("d")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(o->property("e")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(o->property("f")) == nullptr);

    QVERIFY(qvariant_cast<QObject *>(o->property("g")) != nullptr);
    QVERIFY(qvariant_cast<QObject *>(o->property("h")) != nullptr);

    QCOMPARE(o->property("countG").toInt(), 11);
    QCOMPARE(o->property("countH").toInt(), 11);
}

void tst_QmlCppCodegen::propertyOfParent()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/RootWithoutId.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QObject *child = qmlContext(object.data())->objectForName(u"item"_s);

    bool expected = false;

    for (int i = 0; i < 3; ++i) {
        const QVariant foo = object->property("foo");
        QCOMPARE(foo.metaType(), QMetaType::fromType<bool>());
        QCOMPARE(foo.toBool(), expected);

        const QVariant bar = object->property("bar");
        QCOMPARE(bar.metaType(), QMetaType::fromType<bool>());
        QCOMPARE(bar.toBool(), expected);

        const QVariant visible = child->property("visible");
        QCOMPARE(visible.metaType(), QMetaType::fromType<bool>());
        QCOMPARE(visible.toBool(), expected);

        expected = !expected;
        object->setProperty("foo", expected);
    }
}

void tst_QmlCppCodegen::qmlUsing()
{
    QQmlEngine engine;
    const QString url = u"qrc:/qt/qml/TestTypes/qmlUsing.qml"_s;
    QQmlComponent component(&engine, QUrl(url));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    UsingUserObject *u = qobject_cast<UsingUserObject *>(object.data());
    QVERIFY(u);

    QCOMPARE(u->a(), 7);
    QCOMPARE(u->getB(), 5);
    QCOMPARE(u->val().a(), 24);
    QCOMPARE(u->val().getB(), 25);
    QCOMPARE(u->property("valA").toInt(), 24);
    QCOMPARE(u->property("valB").toInt(), 25);
    QCOMPARE(u->property("myA").toInt(), 7);
    QCOMPARE(u->property("myB").toInt(), 5);
    QCOMPARE(u->property("myA2").toInt(), 7);
    QCOMPARE(u->property("myB2").toInt(), 5);
    QCOMPARE(u->setValCalls(), 0); // getters do not cause write back

    QCOMPARE(u->u(), 9u);
    QCOMPARE(u->val().u(), 26u);
    QCOMPARE(u->property("valU").toUInt(), 26u);
    QCOMPARE(u->property("myU").toUInt(), 9u);
    QCOMPARE(u->property("myU2").toUInt(), 9u);

    QList<int> as;
    QList<int> bs;
    QList<uint> us;
    QObject::connect(u, &UsingUserObject::aChanged, this, [&]() {
        as.append(u->a());
        bs.append(u->getB());
    });

    QObject::connect(u, &UsingUserObject::uChanged, this, [&]() {
        us.append(u->u());
    });

    QMetaObject::invokeMethod(object.data(), "twiddle");

    const QList<int> expectedA = { 57, 59 };
    QCOMPARE(as, expectedA);

    const QList<int> expectedB = { 5, 58 };
    QCOMPARE(bs, expectedB);

    const QList<uint> expectedU = { 62, 63 };
    QCOMPARE(us, expectedU);

    QCOMPARE(u->a(), 59);
    QCOMPARE(u->getB(), 60);
    QCOMPARE(u->val().a(), 55);
    QCOMPARE(u->val().getB(), 56);
    QCOMPARE(u->property("valA").toInt(), 55);
    QCOMPARE(u->property("valB").toInt(), 56); // Updated, because val changes
    QCOMPARE(u->property("myA").toInt(), 59);
    QCOMPARE(u->property("myB").toInt(), 5);  // Remains 5, due to lack of signaling
    QCOMPARE(u->property("myA2").toInt(), 59);
    QCOMPARE(u->property("myB2").toInt(), 5); // Remains 5, due to lack of signaling

    QCOMPARE(u->u(), 63u);
    QCOMPARE(u->val().u(), 61u);
    QCOMPARE(u->property("valU").toUInt(), 61u);
    QCOMPARE(u->property("myU").toUInt(), 63u);
    QCOMPARE(u->property("myU2").toUInt(), 63u);

    QCOMPARE(u->setValCalls(), 3); // assignment to val.a and val.u, and call of val.setB()

    QMetaObject::invokeMethod(object.data(), "burn");

    const uint huge = 4294967295;

    const QList<uint> expectedU2 = { 62, 63, huge, 64, huge };
    QCOMPARE(us, expectedU2);

    QCOMPARE(u->u(), huge);
    QCOMPARE(u->val().u(), huge);
    QCOMPARE(u->property("valU").toUInt(), huge);
    QCOMPARE(u->property("myU").toUInt(), huge);
    QCOMPARE(u->property("myU2").toUInt(), huge);

    QMetaObject::invokeMethod(object.data(), "twiddle");

    const QString iError = u" Error: Cannot assign QString to TransparentWrapper<int,int_tag>"_s;
    const QString uError = u" Error: Cannot assign QString to TransparentWrapper<uint,int_tag>"_s;

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":46:"_s + iError));
    QMetaObject::invokeMethod(object.data(), "impossibleValA");
    QCOMPARE(u->val().a(), 55);
    QCOMPARE(u->property("valA").toInt(), 55);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":50:"_s + iError));
    QMetaObject::invokeMethod(object.data(), "impossibleA");
    QCOMPARE(u->a(), 59);
    QCOMPARE(u->property("myA").toInt(), 59);
    QCOMPARE(u->property("myA2").toInt(), 59);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":54:"_s + iError));
    QMetaObject::invokeMethod(object.data(), "impossibleSelfA");
    QCOMPARE(u->a(), 59);
    QCOMPARE(u->property("myA").toInt(), 59);
    QCOMPARE(u->property("myA2").toInt(), 59);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":58:"_s + uError));
    QMetaObject::invokeMethod(object.data(), "impossibleValU");
    QCOMPARE(u->val().u(), 61u);
    QCOMPARE(u->property("valU").toUInt(), 61u);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":62:"_s + uError));
    QMetaObject::invokeMethod(object.data(), "impossibleU");
    QCOMPARE(u->u(), 63u);
    QCOMPARE(u->property("myU").toUInt(), 63u);
    QCOMPARE(u->property("myU2").toUInt(), 63u);

    QTest::ignoreMessage(QtWarningMsg, qPrintable(url + u":66:"_s + uError));
    QMetaObject::invokeMethod(object.data(), "impossibleSelfU");
    QCOMPARE(u->u(), 63u);
    QCOMPARE(u->property("myU").toUInt(), 63u);
    QCOMPARE(u->property("myU2").toUInt(), 63u);
}

void tst_QmlCppCodegen::qtfont()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/qtfont.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const QVariant font = object->property("font");
    QCOMPARE(font.metaType(), QMetaType::fromType<QFont>());

    QFont expected;
    expected.setFamily(u"Arial"_s);
    expected.setWeight(QFont::Normal);
    expected.setItalic(false);
    expected.setPixelSize(32);

    QCOMPARE(font.value<QFont>(), expected);
}

void tst_QmlCppCodegen::reduceWithNullThis()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/reduceWithNullThis.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("preferredHeight").toDouble(), 28.0);
    QCOMPARE(object->property("preferredHeight2").toDouble(), 28.0);
}

void tst_QmlCppCodegen::readEnumFromInstance()
{
    QQmlEngine engine;

    const QString url = u"qrc:/qt/qml/TestTypes/readEnumFromInstance.qml"_s;

    QQmlComponent component(&engine, QUrl(url));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + ":7:5: Unable to assign [undefined] to int"_L1));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("priority"), QVariant::fromValue<int>(0));
    QCOMPARE(object->property("prop2"), QVariant::fromValue<int>(1));
    QCOMPARE(object->property("priorityIsVeryHigh"), QVariant::fromValue<bool>(false));

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + ":13: Error: Cannot assign [undefined] to int"_L1));

    int result = 0;
    QMetaObject::invokeMethod(object.data(), "cyclePriority", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 0);
}

void tst_QmlCppCodegen::readonlyListProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/readonlyListProperty.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("l").toInt(), 4);
}

void tst_QmlCppCodegen::registerElimination()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/registerelimination.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    // Increment of 23 hits both 0 and 460
    for (int input = -23; input < 700; input += 23) {
        object->setProperty("input", input);
        if (input <= 0 || input >= 460)
            QCOMPARE(object->property("output").toInt(), 459);
        else
            QCOMPARE(object->property("output").toInt(), input);
    }
}

void tst_QmlCppCodegen::registerPropagation()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/registerPropagation.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    int result = 0;
    QMetaObject::invokeMethod(o.data(), "test", Q_RETURN_ARG(int, result));
    QCOMPARE(result, 1);

    QString undefined;
    QMetaObject::invokeMethod(o.data(), "produceUndefined1", Q_RETURN_ARG(QString, undefined));
    QCOMPARE(undefined, u"undefined"_s);

    undefined.clear();
    QMetaObject::invokeMethod(o.data(), "produceUndefined2", Q_RETURN_ARG(QString, undefined));
    QCOMPARE(undefined, u"undefined"_s);
}

void tst_QmlCppCodegen::renameAdjust()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/renameAdjust.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtDebugMsg, "success");
    QTest::ignoreMessage(QtCriticalMsg, "failed 10 11");

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
}

void tst_QmlCppCodegen::resettableProperty()
{
    QFETCH(QString, url);

    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(url));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + u":10:5: Unable to assign [undefined] to double"_s));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("value").toDouble(), 999);
    QMetaObject::invokeMethod(o.data(), "doReset");
    QCOMPARE(o->property("value").toDouble(), 0);

    o->setProperty("value", double(82));
    QCOMPARE(o->property("value").toDouble(), 82);
    QMetaObject::invokeMethod(o.data(), "doReset2");
    QCOMPARE(o->property("value").toDouble(), 0);

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + u":18: Error: Cannot assign [undefined] to double"_s));
    QCOMPARE(o->property("notResettable").toDouble(), 10);
    QMetaObject::invokeMethod(o.data(), "doNotReset");
    QCOMPARE(o->property("notResettable").toDouble(), 10);
    QCOMPARE(o->property("notResettable2").toDouble(), 0); // not NaN

    o->setObjectName(u"namename"_s);
    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(url + u":22: Error: Cannot assign [undefined] to QString"_s));
    QMetaObject::invokeMethod(o.data(), "aaa");
    QCOMPARE(o->objectName(), u"namename"_s);
}

void tst_QmlCppCodegen::resettableProperty_data()
{
    QTest::addColumn<QString>("url");
    QTest::addRow("object lookups") << u"qrc:/qt/qml/TestTypes/resettable.qml"_s;
    QTest::addRow("fallback lookups") << u"qrc:/qt/qml/TestTypes/fallbackresettable.qml"_s;
}

void tst_QmlCppCodegen::returnAfterReject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/returnAfterReject.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QCOMPARE(o->property("bar").toInt(), 123);
}

void tst_QmlCppCodegen::revisions()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/revisions.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("delayed").toBool(), true);
    QCOMPARE(o->property("gotten").toInt(), 5);
}

void tst_QmlCppCodegen::scopeIdLookup()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/scopeIdLookup.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("objectName").toString(), u"outer"_s);
}

void tst_QmlCppCodegen::scopeObjectDestruction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/fileDialog.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QObject *dialog = rootObject->property("dialog").value<QObject *>();
    QVERIFY(dialog);

    // We cannot check the warning messages. The AOT compiled code complains about reading the
    // "parent" property of an object scheduled for deletion. The runtime silently returns undefined
    // at that point and then complains about not being able to read a property on undefined.

    // Doesn't crash, even though it triggers bindings on scope objects scheduled for deletion.
    QMetaObject::invokeMethod(dialog, "open");
}

void tst_QmlCppCodegen::scopeVsObject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/scopeVsObject.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("objectName").toString(), u"foobar"_s);
}

void tst_QmlCppCodegen::scopedEnum()
{
    QQmlEngine engine;
    const QString url = u"qrc:/qt/qml/TestTypes/scopedEnum.qml"_s;
    QQmlComponent component(&engine, QUrl(url));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url + u":6:5: Unable to assign [undefined] to int"_s));
    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url + u":8: TypeError: Cannot read property 'C' of undefined"_s));
    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url + u":14: TypeError: Cannot read property 'C' of undefined"_s));

    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("good").toInt(), 27);
    QCOMPARE(object->property("bad").toInt(), 0);
    QCOMPARE(object->property("wrong").toInt(), 0);
    QCOMPARE(object->property("right").toInt(), 7);

    QCOMPARE(object->property("notgood").toInt(), 26);
    QCOMPARE(object->property("notbad").toInt(), 26);
    QCOMPARE(object->property("notwrong").toInt(), 0);
    QCOMPARE(object->property("notright").toInt(), 6);

    QCOMPARE(object->property("passable").toInt(), 2);
    QCOMPARE(object->property("wild").toInt(), 1);
}

void tst_QmlCppCodegen::sequenceToIterable()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/sequenceToIterable.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("c").toInt(), 11);

    QQmlListReference children(object.data(), "children");
    QCOMPARE(children.count(), 11);
    static const QRegularExpression name(u"Entry\\(0x[0-9a-f]+, \"Item ([0-9])\"\\): ([0-9])"_s);
    for (int i = 0; i < 10; ++i) {
        const auto match = name.match(children.at(i)->objectName());
        QVERIFY(match.hasMatch());
        QCOMPARE(match.captured(1), QString::number(i));
    }
}

void tst_QmlCppCodegen::setLookupConversion()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/setLookupConversion.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QVERIFY(o->objectName().isEmpty());
    QMetaObject::invokeMethod(o.data(), "t");
    QCOMPARE(o->objectName(), u"a"_s);
    QCOMPARE(o->property("value").toInt(), 9);
}

void tst_QmlCppCodegen::setLookupOriginalScope()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/setLookupOriginalScope.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *v = o->property("variable").value<QObject *>();
    QCOMPARE(v->property("value").toInt(), 0);

    QMetaObject::invokeMethod(o.data(), "trigger");
    QObject *edit = o->property("edit").value<QObject *>();
    QVERIFY(edit);
    QCOMPARE(edit->property("myOutput").value<QObject *>(), v);
    QCOMPARE(v->property("value").toInt(), 55);
}

void tst_QmlCppCodegen::shadowedAsCasts()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/shadowedAsCasts.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QObject *shadowed1 = obj->property("shadowed1").value<QObject *>();
    QVERIFY(shadowed1);
    QVERIFY(shadowed1->objectName().isEmpty());
    const QVariant name1 = shadowed1->property("objectName");
    QCOMPARE(name1.metaType(), QMetaType::fromType<int>());
    QCOMPARE(name1.toInt(), 43);

    QObject *shadowed2 = obj->property("shadowed2").value<QObject *>();
    QVERIFY(shadowed2);
    QVERIFY(shadowed2->objectName().isEmpty());
    const QVariant name2 = shadowed2->property("objectName");
    QCOMPARE(name2.metaType(), QMetaType::fromType<int>());
    QCOMPARE(name2.toInt(), 42);

    QObject *shadowed3 = obj->property("shadowed3").value<QObject *>();
    QVERIFY(shadowed3);
    QVERIFY(shadowed3->objectName().isEmpty());
    const QVariant name3 = shadowed3->property("objectName");
    QCOMPARE(name3.metaType(), QMetaType::fromType<double>());
    QCOMPARE(name3.toDouble(), 41.0);
}

void tst_QmlCppCodegen::shadowedMethod()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/shadowedMethod.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("athing"), QVariant::fromValue<bool>(false));
    QCOMPARE(o->property("bthing"), QVariant::fromValue(u"b"_s));
    QCOMPARE(o->property("cthing"), QVariant::fromValue(u"c"_s));
}

void tst_QmlCppCodegen::shadowedPrimitiveCmpEqNull()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/shadowedPrimitiveCmpEqNull.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
}

void tst_QmlCppCodegen::shifts()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/shifts.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    QCOMPARE(object->property("a").toInt(), 9728);
    QCOMPARE(object->property("b").toInt(), 4864);
    QCOMPARE(object->property("c").toInt(), 19448);
    QCOMPARE(object->property("d").toInt(), 9731);
    QCOMPARE(object->property("e").toInt(), 0);
}

void tst_QmlCppCodegen::signalHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/signal.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->objectName(), QString());
    QCOMPARE(object->property("ff").toInt(), 4);

    object->setObjectName(u"foo"_s);
    QCOMPARE(object->property("ff").toInt(), 12);
}

void tst_QmlCppCodegen::signalIndexMismatch()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/signalIndexMismatch.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> item(c1.create());
    const auto visualIndexBeforeMoveList = item->property("visualIndexBeforeMove").toList();
    const auto visualIndexAfterMoveList = item->property("visualIndexAfterMove").toList();

    QCOMPARE(visualIndexBeforeMoveList, QList<QVariant>({ 0, 1, 2 }));
    QCOMPARE(visualIndexAfterMoveList, QList<QVariant>({ 0, 1, 2 }));
}

void tst_QmlCppCodegen::signalsWithLists()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/signalsWithLists.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QVariantList varlist = o->property("varlist").toList();
    QCOMPARE(varlist.size(), 5);
    QCOMPARE(varlist[0], QVariant::fromValue(1));
    QCOMPARE(varlist[1], QVariant::fromValue(u"foo"_s));
    QCOMPARE(varlist[2], QVariant::fromValue(o.data()));
    QCOMPARE(varlist[3], QVariant());
    QCOMPARE(varlist[4], QVariant::fromValue(true));

    QQmlListProperty<QObject> objlist = o->property("objlist").value<QQmlListProperty<QObject>>();
    QCOMPARE(objlist.count(&objlist), 3);
    QCOMPARE(objlist.at(&objlist, 0), o.data());
    QCOMPARE(objlist.at(&objlist, 1), nullptr);
    QCOMPARE(objlist.at(&objlist, 2), o.data());

    QCOMPARE(o->property("happening").toInt(), 0);
    o->metaObject()->invokeMethod(o.data(), "sendSignals");
    QCOMPARE(o->property("happening").toInt(), 8);
}

void tst_QmlCppCodegen::signatureIgnored()
{
    QQmlEngine engine;

    QQmlComponent c1(&engine, QUrl(u"qrc:/qt/qml/TestTypes/signatureIgnored.qml"_s));
    QVERIFY2(c1.isReady(), qPrintable(c1.errorString()));

    QScopedPointer<QObject> ignored(c1.create());
    QCOMPARE(ignored->property("l").toInt(), 5);
    QCOMPARE(ignored->property("m").toInt(), 77);
    QCOMPARE(ignored->property("n").toInt(), 67);
}

void tst_QmlCppCodegen::simpleBinding()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/Test.qml"_s));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), component.errorString().toUtf8().constData());
    QCOMPARE(object->property("foo").toInt(), int(3));

    {
        CppBaseClass *base = qobject_cast<CppBaseClass *>(object.data());
        Q_ASSERT(base);
        QVERIFY(!base->cppProp.hasBinding());
        QCOMPARE(base->cppProp.value(), 7);
        QVERIFY(base->cppProp2.hasBinding());
        QCOMPARE(base->cppProp2.value(), 14);
        base->cppProp.setValue(9);
        QCOMPARE(base->cppProp.value(), 9);
        QCOMPARE(base->cppProp2.value(), 18);
    }
}

void tst_QmlCppCodegen::storeElementSideEffects()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/storeElementSideEffects.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    const QJSValue prop = o->property("myItem").value<QJSValue>();
    QVERIFY(prop.isArray());
    QCOMPARE(prop.property(0).toInt(), 10);
}

void tst_QmlCppCodegen::storeMetaEnum()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/StoreMetaEnum.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("bar").toInt(), 0);
    QCOMPARE(o->property("baz").toInt(), 1);
}

void tst_QmlCppCodegen::stringArg()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/stringArg.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("stringArg"), u"a foozly thing"_s);
    QCOMPARE(o->property("falseArg"), u"a 0 thing"_s);
    QCOMPARE(o->property("trueArg"), u"a 1 thing"_s);
    QCOMPARE(o->property("zeroArg"), u"a 0 thing"_s);
    QCOMPARE(o->property("intArg"), u"a 11 thing"_s);
    QCOMPARE(o->property("realArg"), u"a 12.25 thing"_s);
}

void tst_QmlCppCodegen::stringLength()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/stringLength.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("stringLength").toInt(), 8);
}

void tst_QmlCppCodegen::stringToByteArray()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/stringToByteArray.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    Person *person = qobject_cast<Person *>(o.data());
    QVERIFY(person);

    QCOMPARE(person->dataBindable().value(), QByteArray("some data"));
    QCOMPARE(person->name(), u"some data"_s);
}

void tst_QmlCppCodegen::structuredValueType()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/structuredValueType.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QCOMPARE(o->property("r").value<QRectF>(), QRectF(1, 2, 3, 4));
    QCOMPARE(o->property("r2").value<QRectF>(), QRectF(42, 0, 0, 0));
    QCOMPARE(o->property("r3").value<QRectF>(), QRectF(4, 3, 2, 1));
    QCOMPARE(o->property("r4").value<QRectF>(), QRectF(43, 0, 0, 0));

    WeatherModelUrl w;
    w.setStrings(QStringList({u"one"_s, u"two"_s, u"three"_s}));

    QCOMPARE(o->property("w").value<WeatherModelUrl>(), w);

    WeatherModelUrlDerived w1;
    w1.setStrings(QStringList({u"four"_s, u"five"_s, u"six"_s}));
    QCOMPARE(o->property("w1").value<WeatherModelUrlDerived>(), w1);

    WeatherModelUrl w2;
    w2.setStrings(QStringList({u"three"_s, u"two"_s, u"one"_s}));
    QCOMPARE(o->property("w2").value<WeatherModelUrl>(), w2);
}

void tst_QmlCppCodegen::takeNumbers()
{
    QFETCH(QByteArray, method);
    QFETCH(int, expectedInt);
    QFETCH(qsizetype, expectedSizeType);
    QFETCH(qlonglong, expectedLongLong);

    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/takenumber.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    TakeNumber *takeNumber = qobject_cast<TakeNumber *>(o.data());
    QVERIFY(takeNumber != nullptr);

    o->metaObject()->invokeMethod(o.data(), method);

    QCOMPARE(takeNumber->takenInt, expectedInt);
    QCOMPARE(takeNumber->takenNegativeInt, -expectedInt);
    QCOMPARE(takeNumber->takenQSizeType, expectedSizeType);
    QCOMPARE(takeNumber->takenQLongLong, expectedLongLong);
    QCOMPARE(takeNumber->takenNumbers,
             (Numbers {expectedInt, -expectedInt, expectedSizeType, expectedLongLong}));
    QCOMPARE(takeNumber->propertyInt, expectedInt);
    QCOMPARE(takeNumber->propertyNegativeInt, -expectedInt);
    QCOMPARE(takeNumber->propertyQSizeType, expectedSizeType);
    QCOMPARE(takeNumber->propertyQLongLong, expectedLongLong);
    QCOMPARE(takeNumber->propertyNumbers,
             (Numbers {expectedInt, -expectedInt, expectedSizeType, expectedLongLong}));
}

void tst_QmlCppCodegen::takeNumbers_data()
{
    QTest::addColumn<QByteArray>("method");
    QTest::addColumn<int>("expectedInt");
    QTest::addColumn<qsizetype>("expectedSizeType");
    QTest::addColumn<qlonglong>("expectedLongLong");

    QTest::addRow("literal0") << "literal0"_ba << 0 << qsizetype(0) << 0ll;
    QTest::addRow("literal56") << "literal56"_ba << 56 << qsizetype(56) << 56ll;

    QTest::addRow("variable0") << "variable0"_ba << 0 << qsizetype(0) << 0ll;
    QTest::addRow("variable484") << "variable484"_ba << 484 << qsizetype(484) << 484ll;

    QTest::addRow("literal3B")
            << "literal3B"_ba << int(3000000000ll) << qsizetype(3000000000ll) << 3000000000ll;
    QTest::addRow("variable3B")
            << "variable3B"_ba << int(3000000000ll) << qsizetype(3000000000ll) << 3000000000ll;

}


void tst_QmlCppCodegen::testIsnan()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/isnan.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("good").toDouble(), 10.1);
    QVERIFY(qIsNaN(o->property("bad").toDouble()));

    const QVariant a = o->property("a");
    QCOMPARE(a.metaType(), QMetaType::fromType<bool>());
    QVERIFY(!a.toBool());

    const QVariant b = o->property("b");
    QCOMPARE(b.metaType(), QMetaType::fromType<bool>());
    QVERIFY(b.toBool());
}

void tst_QmlCppCodegen::thisObject()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/thisObject.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("warned").value<QObject *>(), o.data());
}

void tst_QmlCppCodegen::throwObjectName()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/throwObjectName.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(QtWarningMsg, "qrc:/qt/qml/TestTypes/throwObjectName.qml:5:5: ouch");
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QVERIFY(o->objectName().isEmpty());
}

void tst_QmlCppCodegen::topLevelComponent()
{
    // TODO: Once we stop accepting top level Component elements, this test can be removed.

    QQmlEngine e;

    const QUrl url(u"qrc:/qt/qml/TestTypes/topLevelComponent.qml"_s);
    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString() + u":4:1: Using a Component as the root of a QML document "
                                        "is deprecated: types defined in qml documents are "
                                        "automatically wrapped into Components when needed."_s));

    QQmlComponent c(&e, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlComponent *inner = qobject_cast<QQmlComponent *>(o.data());
    QVERIFY(inner);

    QScopedPointer<QObject> o2(inner->create());
    QCOMPARE(o2->objectName(), u"foo"_s);
}

void tst_QmlCppCodegen::translation()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/translation.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("translate2"), u"s"_s);
    QCOMPARE(o->property("translate3"), u"s"_s);
    QCOMPARE(o->property("translate4"), u"s"_s);

    QCOMPARE(o->property("translateNoop2"), u"s"_s);
    QCOMPARE(o->property("translateNoop3"), u"s"_s);

    QCOMPARE(o->property("tr1"), u"s"_s);
    QCOMPARE(o->property("tr2"), u"s"_s);
    QCOMPARE(o->property("tr3"), u"s"_s);

    QCOMPARE(o->property("trNoop1"), u"s"_s);
    QCOMPARE(o->property("trNoop2"), u"s"_s);

    QCOMPARE(o->property("trId1"), u"s"_s);
    QCOMPARE(o->property("trId2"), u"s"_s);

    QCOMPARE(o->property("trIdNoop1"), u"s"_s);
}

void tst_QmlCppCodegen::trigraphs()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/trigraphs.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->objectName(), u"?""?= ?""?/ ?""?' ?""?( ?""?) ?""?! ?""?< ?""?> ?""?-"_s);
}

void tst_QmlCppCodegen::trivialSignalHandler()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/trivialSignalHandler.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("a").toString(), u"no"_s);
    QCOMPARE(o->property("b").toInt(), -1);
    QCOMPARE(o->property("b").toDouble(), -1.0);

    o->setObjectName(u"yes"_s);
    QCOMPARE(o->property("a").toString(), u"yes"_s);
    QCOMPARE(o->property("b").toInt(), 5);
    QCOMPARE(o->property("c").toDouble(), 2.5);
}

void tst_QmlCppCodegen::typePropagationLoop()
{
    QQmlEngine engine;

    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/typePropagationLoop.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(o->property("j").toInt(), 3);
}

void tst_QmlCppCodegen::typePropertyClash()
{
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(u"size"_s, 5);
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/typePropertyClash.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), u"Size: 5"_s);
}

void tst_QmlCppCodegen::typedArray()
{
    QQmlEngine engine;
    const QUrl document(u"qrc:/qt/qml/TestTypes/typedArray.qml"_s);
    QQmlComponent c(&engine, document);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QDateTime date;
    QVERIFY(qvariant_cast<QList<int>>(o->property("values2")).isEmpty());
    QCOMPARE(qvariant_cast<QList<int>>(o->property("values3")),
             QList<int>({1, 2, 3, 4}));
    QCOMPARE(qvariant_cast<QList<QDateTime>>(o->property("values4")),
             QList<QDateTime>({date, date, date}));
    {
        const QList<double> actual
                = qvariant_cast<QList<double>>(o->property("values5"));
        const QList<double> expected
                = QList<double>({1, 2, 3.4, 30, std::numeric_limits<double>::quiet_NaN(), 0});
        QCOMPARE(actual.size(), expected.size());
        for (qsizetype i = 0, end = actual.size(); i != end; ++i) {
            if (std::isnan(expected[i]))
                QVERIFY(std::isnan(actual[i]));
            else
                QCOMPARE(actual[i], expected[i]);
        }
    }
    date = QDateTime::currentDateTime();
    o->setProperty("aDate", date);
    QCOMPARE(qvariant_cast<QList<QDateTime>>(o->property("values4")),
             QList<QDateTime>({date, date, date}));

    QQmlListProperty<QObject> values6
            = qvariant_cast<QQmlListProperty<QObject>>(o->property("values6"));
    QCOMPARE(values6.count(&values6), 3);
    for (int i = 0; i < 3; ++i)
        QCOMPARE(values6.at(&values6, i), o.data());

    QCOMPARE(o->property("inIntList").toInt(), 2);
    QCOMPARE(qvariant_cast<QDateTime>(o->property("inDateList")), date);
    QCOMPARE(o->property("inRealList").toDouble(), 30.0);
    QCOMPARE(o->property("inCharList").toString(), QStringLiteral("f"));

    const QMetaObject *metaObject = o->metaObject();
    QMetaMethod method = metaObject->method(metaObject->indexOfMethod("stringAt10(QString)"));
    QVERIFY(method.isValid());

    // If LoadElement threw an exception the function would certainly return neither 10 nor 20.
    int result = 0;
    method.invoke(
                o.data(), Q_RETURN_ARG(int, result), Q_ARG(QString, QStringLiteral("a")));
    QCOMPARE(result, 10);
    method.invoke(
                o.data(), Q_RETURN_ARG(int, result), Q_ARG(QString, QStringLiteral("aaaaaaaaaaa")));
    QCOMPARE(result, 20);
}

void tst_QmlCppCodegen::unclearComponentBoundaries()
{
    QQmlEngine engine;
    QQmlComponent component(
                &engine, QUrl(u"qrc:/qt/qml/TestTypes/unclearComponentBoundaries.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(rootObject->objectName(), u"outer"_s);
}

void tst_QmlCppCodegen::undefinedResets()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/undefinedResets.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    Person *person = qobject_cast<Person *>(rootObject.data());
    QVERIFY(person);
    QCOMPARE(person->shoeSize(), 0);
    QCOMPARE(person->name(), u"Marge"_s);

    person->setShoeSize(11);

    QCOMPARE(person->shoeSize(), 11);
    QCOMPARE(person->name(), u"Bart"_s);

    person->setShoeSize(10);
    QCOMPARE(person->shoeSize(), 10);
    QCOMPARE(person->name(), u"Marge"_s);

    person->setName(u"no one"_s);
    QCOMPARE(person->name(), u"no one"_s);

    person->setObjectName(u"the one"_s);
    QCOMPARE(person->name(), u"Bart"_s);
}

void tst_QmlCppCodegen::undefinedToDouble()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/undefinedToDouble.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    const QVariant d = o->property("d");
    QCOMPARE(d.metaType(), QMetaType::fromType<double>());
    QVERIFY(std::isnan(d.toDouble()));
}

void tst_QmlCppCodegen::unknownAttached()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/unknownAttached.qml"_s));
    QVERIFY(c.isError());
}

void tst_QmlCppCodegen::unknownParameter()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/unknownParameter.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("cppProp").toInt(), 18);
}

void tst_QmlCppCodegen::unknownUnderlyingType()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/unknownUnderlyingType.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QCOMPARE(object->property("foo").toInt(), 11);
}

void tst_QmlCppCodegen::unstoredUndefined()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/unstoredUndefined.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QCOMPARE(o->objectName(), u"NaN"_s);
}

void tst_QmlCppCodegen::unusedAttached()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/unusedAttached.qml"_s));
    QVERIFY2(!component.isError(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    const auto func = qmlAttachedPropertiesFunction(
                object.data(), QMetaType::fromName("QQuickKeyNavigationAttached*").metaObject());
    QObject *attached = qmlAttachedPropertiesObject(object.data(), func);
    const QVariant prop = attached->property("priority");
    QVERIFY(prop.isValid());
    QCOMPARE(QByteArray(prop.metaType().name()), "QQuickKeyNavigationAttached::Priority");
    bool ok = false;
    QCOMPARE(prop.toInt(&ok), 0);
    QVERIFY(ok);
}

void tst_QmlCppCodegen::urlString()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/urlString.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);

    QCOMPARE(qvariant_cast<QUrl>(rootObject->property("c")), QUrl(u"http://dddddd.com"_s));
    QCOMPARE(qvariant_cast<QUrl>(rootObject->property("d")), QUrl(u"http://aaaaaa.com"_s));
    QCOMPARE(qvariant_cast<QUrl>(rootObject->property("e")), QUrl(u"http://a112233.de"_s));
    QCOMPARE(rootObject->objectName(), QLatin1String("http://dddddd.com"));
}

void tst_QmlCppCodegen::valueTypeArgument()
{
    QTest::ignoreMessage(QtMsgType::QtDebugMsg, "Reading l.i=5");

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/ValueTypeArgument.qml"_s));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject);
}

void tst_QmlCppCodegen::valueTypeBehavior()
{
    QQmlEngine engine;

    {
        const QUrl url(u"qrc:/qt/qml/TestTypes/valueTypeCopy.qml"_s);
        QQmlComponent c(&engine, url);
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'e'));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'f'));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
        QCOMPARE(o->property("e").toDouble(), 45.0);
        QCOMPARE(o->property("f").toDouble(), 1.0);
    }

    {
        const QUrl url(u"qrc:/qt/qml/TestTypes/valueTypeReference.qml"_s);
        QQmlComponent c(&engine, url);
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'e'));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'f'));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
        QVERIFY(qIsNaN(o->property("e").toDouble()));
        QCOMPARE(o->property("f").toDouble(), 5.0);
    }

    {
        const QUrl url(u"qrc:/qt/qml/TestTypes/valueTypeDefault.qml"_s);
        QQmlComponent c(&engine, url);
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'e'));
        QTest::ignoreMessage(QtWarningMsg, bindingLoopMessage(url, 'f'));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());
        QVERIFY(qIsNaN(o->property("e").toDouble()));
        QCOMPARE(o->property("f").toDouble(), 5.0);
    }

    {
        const QUrl url(u"qrc:/qt/qml/TestTypes/valueTypeCast.qml"_s);
        QQmlComponent c(&engine, url);
        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer o(c.create());
        QVERIFY(!o.isNull());
        QCOMPARE(o->property("x"), 10);

        QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString()
                           + u":8: TypeError: Cannot read property 'x' of undefined"_s));
        o->setProperty("v", QLatin1String("not a rect"));

        // If the binding throws an exception, the value doesn't change.
        QCOMPARE(o->property("x"), 10);

        QCOMPARE(o->property("tv3"), 5);
        QCOMPARE(o->property("tc3"), 5);
        QCOMPARE(o->property("tc6"), QVariant());
        QCOMPARE(o->property("tc7"), QVariant());
        QCOMPARE(o->property("tc8"), 2);

        // The default greeting is never applied because undefined can be coerced to string
        QCOMPARE(o->property("greeting1"), QLatin1String("undefined"));
        QCOMPARE(o->property("greeting2"), QLatin1String("Custom Greeting"));
    }
}

void tst_QmlCppCodegen::valueTypeLists()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/valueTypeLists.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QCOMPARE(qvariant_cast<QRectF>(o->property("rectInBounds")), QRectF(1, 2, 3, 4));
    QVERIFY(o->metaObject()->indexOfProperty("rectOutOfBounds") > 0);
    QVERIFY(!o->property("rectOutOfBounds").isValid());

    QCOMPARE(qvariant_cast<QString>(o->property("stringInBounds")), QStringLiteral("bbb"));
    QVERIFY(o->metaObject()->indexOfProperty("stringOutOfBounds") > 0);
    QVERIFY(!o->property("stringOutOfBounds").isValid());

    QCOMPARE(qvariant_cast<int>(o->property("intInBounds")), 7);
    QVERIFY(o->metaObject()->indexOfProperty("intOutOfBounds") > 0);
    QVERIFY(!o->property("intOutOfBounds").isValid());

    QCOMPARE(qvariant_cast<QString>(o->property("charInBounds")), QStringLiteral("d"));
    QVERIFY(o->metaObject()->indexOfProperty("charOutOfBounds") > 0);
    QVERIFY(!o->property("charOutOfBounds").isValid());
}

void tst_QmlCppCodegen::valueTypeProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/valueTypeProperty.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());

    QFont font = qvariant_cast<QFont>(object->property("font"));
    QCOMPARE(object->property("foo").toString(), font.family());
    font.setFamily(u"Bar"_s);
    object->setProperty("font", QVariant::fromValue(font));
    QCOMPARE(object->property("foo").toString(), u"Bar"_s);
}

void tst_QmlCppCodegen::variantMapLookup()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/variantMapLookup.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->property("i"), 42);
}

void tst_QmlCppCodegen::variantReturn()
{
    QQmlEngine e;
    QQmlComponent c(&e, QUrl(u"qrc:/qt/qml/TestTypes/variantReturn.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QObject *a = o->property("a").value<QObject *>();
    QVERIFY(a);
    const QVariant x = a->property("x");
    const QMetaObject *meta = x.metaType().metaObject();
    QVERIFY(meta);
    const QMetaProperty property = meta->property(meta->indexOfProperty("timeIndex"));
    QVERIFY(property.isValid());
    const QVariant timeIndex = property.readOnGadget(x.data());
    QCOMPARE(timeIndex.metaType(), QMetaType::fromType<qsizetype>());
    QCOMPARE(timeIndex.value<qsizetype>(), qsizetype(1));

    QObject *b = o->property("b").value<QObject *>();
    QVERIFY(b);
    QCOMPARE(b->property("z").toInt(), 2);
}

void tst_QmlCppCodegen::variantlist()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/variantlist.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    const QVariantList things = qvariant_cast<QVariantList>(o->property("things"));
    QCOMPARE(things.size(), 2);
    QCOMPARE(things[0].toString(), u"thing"_s);
    QCOMPARE(things[1].toInt(), 30);
}

void tst_QmlCppCodegen::variantMap()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, QUrl(u"qrc:/qt/qml/TestTypes/variantMap.qml"_s));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->objectName(), "a b"_L1);
    QCOMPARE(o->property("r"), QVariant::fromValue(QRectF(12, 13, 14, 15)));

    const QVariantMap expected = QVariantMap {
        { u"1"_s, QVariant::fromValue<std::nullptr_t>(nullptr) },
        { u"19"_s, QVariant::fromValue(u"19"_s) },
        { u"25"_s, QVariant() }
    };

    QCOMPARE(o->property("v").toMap(), expected);
}

void tst_QmlCppCodegen::voidConversion()
{
    QQmlEngine engine;
    const QUrl url(u"qrc:/qt/qml/TestTypes/voidConversion.qml"_s);
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString() + u":8: Error: Cannot assign [undefined] to QPointF"_s));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);

    QCOMPARE(o->property("p"), QPointF(20, 10));
}

void tst_QmlCppCodegen::voidFunction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/voidfunction.qml"_s));
    QVERIFY2(component.isReady(), component.errorString().toUtf8());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());
    QVERIFY(object->objectName().isEmpty());
    object->metaObject()->invokeMethod(object.data(), "doesNotReturnValue");
    QCOMPARE(object->objectName(), u"barbar"_s);
}

void tst_QmlCppCodegen::writeBack()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/TestTypes/writeback.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    Person *person = qobject_cast<Person *>(object.data());
    QVERIFY(person);
    QCOMPARE(person->area(), QRectF(4, 5, 16, 17));
    QCOMPARE(person->property("inner").toInt(), 99);

    Person *shadowable = person->property("shadowable").value<Person *>();
    QVERIFY(shadowable);
    QCOMPARE(shadowable->area(), QRectF(40, 50, 16, 17));

    QCOMPARE(person->property("ints"), QVariant::fromValue(QList<int>({12, 22, 2, 1, 0, 0, 33})));
}

void tst_QmlCppCodegen::writeVariantMap()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl(u"qrc:/qt/qml/StringBuilderTestTypes/writeVariantMap.qml"_s));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    const QVariantMap v = object->property("data").toMap();
    QCOMPARE(v.size(), 1);
    const QVariant textPlain = v[u"text/plain"_s];
    QCOMPARE(textPlain.metaType(), QMetaType::fromType<QString>());
    QCOMPARE(textPlain.toString(), u"%Drag Me%"_s);

}

QTEST_MAIN(tst_QmlCppCodegen)

#include "tst_qmlcppcodegen.moc"
