// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtTest/QTest>
#include <QtTest/QSignalSpy>

#include <private/qqmldata_p.h>
#include <qjsengine.h>
#include <qjsvalueiterator.h>
#include <qstandarditemmodel.h>
#include <QtCore/qnumeric.h>
#include <qqmlengine.h>
#include <qqmlcomponent.h>
#include <stdlib.h>
#include <private/qv4alloca_p.h>
#include <private/qjsvalue_p.h>
#include <QScopeGuard>
#include <QUrl>
#include <QModelIndex>
#include <QtQml/qqmllist.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <private/qv4functionobject_p.h>
#include <QItemSelection>
#include <QItemSelectionRange>
#include <QJsonArray>
#include <QQueue>
#include <QStack>
#include <QTranslator>

#ifdef Q_CC_MSVC
#define NO_INLINE __declspec(noinline)
#else
#define NO_INLINE __attribute__((noinline))
#endif

using namespace Qt::StringLiterals;

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QObjectList)

class DateTimeHolder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDateTime dateTime MEMBER m_dateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(QDate date MEMBER m_date NOTIFY dateChanged)
    Q_PROPERTY(QTime time MEMBER m_time NOTIFY timeChanged)
    Q_PROPERTY(QString string MEMBER m_string NOTIFY stringChanged)

signals:
    void dateTimeChanged();
    void dateChanged();
    void timeChanged();
    void stringChanged();

public:
    QDateTime m_dateTime;
    QDate m_date;
    QTime m_time;
    QString m_string;
};

class tst_QJSEngine : public QObject
{
    Q_OBJECT

public:
    tst_QJSEngine();
    virtual ~tst_QJSEngine();

private slots:
    void callQObjectSlot();
    void constructWithParent();
    void newObject();
    void newArray();
    void newArray_HooliganTask218092();
    void newArray_HooliganTask233836();
    void toScriptValueBuiltin_data();
    void toScriptValueBuiltin();
    void toScriptValueQmlBuiltin_data();
    void toScriptValueQmlBuiltin();
    void toScriptValueQtQml_data();
    void toScriptValueQtQml();
    void toScriptValuenotroundtripped_data();
    void toScriptValuenotroundtripped();
    void newVariant();
    void newVariant_valueOfToString();
    void newVariant_valueOfEnum();
    void newRegExp();
    void jsRegExp();
    void newDate();
    void jsParseDate();
    void newQObject();
    void newQObjectRace();
    void newQObject_ownership();
    void newQObject_deletedEngine();
    void newQObjectPropertyCache();
    void newQMetaObject();
    void exceptionInSlot();
    void globalObjectProperties();
    void globalObjectEquals();
    void globalObjectProperties_enumerate();
    void createGlobalObjectProperty();
    void globalObjectWithCustomPrototype();
    void builtinFunctionNames_data();
    void builtinFunctionNames();
    void evaluate_data();
    void evaluate();
    void errorMessage_QT679();
    void valueConversion_basic();
    void valueConversion_QVariant();
    void valueConversion_basic2();
    void valueConversion_dateTime();
    void valueConversion_RegularExpression();
    void castWithMultipleInheritance();
    void collectGarbage();
    void collectGarbageNestedWrappersTwoEngines();
    void gcWithNestedDataStructure();
    void stacktrace();
    void unshiftAndSort();
    void unshiftAndPushAndSort();
    void numberParsing_data();
    void numberParsing();
    void automaticSemicolonInsertion();
    void errorConstructors();
    void argumentsProperty_globalContext();
    void argumentsProperty_JS();
    void jsNumberClass();
    void jsForInStatement_simple();
    void jsForInStatement_prototypeProperties();
    void jsForInStatement_mutateWhileIterating();
    void jsForInStatement_arrays();
    void jsForInStatement_constant();
    void with_constant();
    void stringObjects();
    void jsStringPrototypeReplaceBugs();
    void getterSetterThisObject_global();
    void getterSetterThisObject_plain();
    void getterSetterThisObject_prototypeChain();
    void jsContinueInSwitch();
    void jsShadowReadOnlyPrototypeProperty();
    void jsReservedWords_data();
    void jsReservedWords();
    void jsFutureReservedWords_data();
    void jsFutureReservedWords();
    void jsThrowInsideWithStatement();
    void reentrancy_globalObjectProperties();
    void reentrancy_Array();
    void reentrancy_objectCreation();
    void jsIncDecNonObjectProperty();
    void JSON_Parse();
    void JSON_Stringify_data();
    void JSON_Stringify();
    void JSON_Stringify_WithReplacer_QTBUG_95324();
    void arraySort();
    void lookupOnDisappearingProperty();
    void arrayConcat();
    void recursiveBoundFunctions();

    void qRegularExpressionImport_data();
    void qRegularExpressionImport();
    void qRegularExpressionExport_data();
    void qRegularExpressionExport();
    void dateRoundtripJSQtJS();
    void dateRoundtripQtJSQt();
    void dateConversionJSQt();
    void dateConversionQtJS();
    void functionPrototypeExtensions();
    void threadedEngine();

    void functionDeclarationsInConditionals();

    void arrayPop_QTBUG_35979();
    void array_unshift_QTBUG_52065();
    void array_join_QTBUG_53672();

    void regexpLastMatch();
    void regexpLastIndex();
    void indexedAccesses();

    void prototypeChainGc();
    void prototypeChainGc_QTBUG38299();

    void dynamicProperties();

    void scopeOfEvaluate();

    void callConstants();

    void installTranslationFunctions();
    void translateScript_data();
    void translateScript();
    void translateScript_crossScript();
    void translateScript_trNoOp();
    void translateScript_callQsTrFromCpp();
    void translateWithInvalidArgs_data();
    void translateWithInvalidArgs();
    void translationContext_data();
    void translationContext();
    void translateScriptIdBased();
    void translateScriptUnicode_data();
    void translateScriptUnicode();
    void translateScriptUnicodeIdBased_data();
    void translateScriptUnicodeIdBased();
    void translateFromBuiltinCallback();
    void translationFilePath_data();
    void translationFilePath();
    void translationFileName();

    void installConsoleFunctions();
    void logging();
    void tracing();
    void asserts();
    void exceptions();

    void exceptionReporting();

    void installGarbageCollectionFunctions();

    void installAllExtensions();

    void privateMethods();

    void engineForObject();
    void intConversion_QTBUG43309();
#ifdef QT_DEPRECATED
    void toFixed();
#endif

    void argumentEvaluationOrder();

    void v4FunctionWithoutQML();

    void withNoContext();
    void holeInPropertyData();

    void basicBlockMergeAfterLoopPeeling();

    void modulusCrash();
    void malformedExpression();

    void scriptScopes();

    void binaryNumbers();
    void octalNumbers();

    void incrementAfterNewline();

    void deleteInsideForIn();

    void functionToString_data();
    void functionToString();

    void stringReplace();

    void protoChanges_QTBUG68369();
    void multilineStrings();

    void throwError();
    void throwErrorObject();
    void returnError();
    void catchError();
    void mathMinMax();
    void mathNegativeZero();

    void importModule();
    void importModuleRelative();
    void importModuleWithLexicallyScopedVars();
    void importExportErrors();

    void registerModule();
    void registerModuleQObject();
    void registerModuleNamedError();

    void equality();
    void aggressiveGc();
    void noAccumulatorInTemplateLiteral();

    void interrupt_data();
    void interrupt();

    void triggerBackwardJumpWithDestructuring();
    void arrayConcatOnSparseArray();
    void concatAfterUnshift();
    void sortSparseArray();
    void compileBrokenRegexp();
    void sortNonStringArray();
    void iterateInvalidProxy();
    void applyOnHugeArray();
    void reflectApplyOnHugeArray();
    void jsonStringifyHugeArray();

    void tostringRecursionCheck();
    void arrayIncludesWithLargeArray();
    void printCircularArray();
    void typedArraySet();
    void dataViewCtor();

    void uiLanguage();
    void urlObject();
    void thisInConstructor();
    void forOfAndGc();
    void jsExponentiate();
    void arrayBuffer();
    void staticInNestedClasses();
    void callElement();

    void functionCtorGeneratedCUIsNotCollectedByGc();

    void tdzViolations_data();
    void tdzViolations();

    void coerceValue();

    void coerceDateTime_data();
    void coerceDateTime();

    void callWithSpreadOnElement();
    void spreadNoOverflow();

    void symbolToVariant();

    void garbageCollectedObjectMethodBase();

    void optionalChainWithElementLookup();

    void deleteDefineCycle();
    void deleteFromSparseArray();

    void emptyStringLiteralEvaluatesToANonNullString();

    void consoleLogSequence();

    void generatorFunctionInTailCallPosition();
    void generatorMethodInTailCallPosition();

    void generatorStackOverflow_data();
    void generatorStackOverflow();
    void generatorInfiniteRecursion();

    void setDeleteDuringForEach();
    void mapDeleteDuringForEach();

    void multiMatchingRegularExpression();

#if QT_CONFIG(icu)
    void toLocaleLowerCase_data();
    void toLocaleLowerCase();
    void toLocaleLowerStringWithQLocale();

    void toLocaleUpperCase_data();
    void toLocaleUpperCase();
    void toLocaleUpperStringWithQLocale();
#endif

public:
    Q_INVOKABLE QJSValue throwingCppMethod1();
    Q_INVOKABLE void throwingCppMethod2();
    Q_INVOKABLE QJSValue throwingCppMethod3();

signals:
    void testSignal();
};

tst_QJSEngine::tst_QJSEngine()
{
    qmlRegisterType<DateTimeHolder>("Test", 1, 0, "DateTimeHolder");
}

tst_QJSEngine::~tst_QJSEngine()
{
}

Q_DECLARE_METATYPE(Qt::KeyboardModifier)
Q_DECLARE_METATYPE(Qt::KeyboardModifiers)

class OverloadedSlots : public QObject
{
    Q_OBJECT
public:
    OverloadedSlots()
    {
    }

signals:
    void slotWithoutArgCalled();
    void slotWithSingleArgCalled(const QString &arg);
    void slotWithArgumentsCalled(const QString &arg1, const QString &arg2, const QString &arg3);
    void slotWithOverloadedArgumentsCalled(const QString &arg, Qt::KeyboardModifier modifier, Qt::KeyboardModifiers moreModifiers);
    void slotWithTwoOverloadedArgumentsCalled(const QString &arg, Qt::KeyboardModifiers moreModifiers, Qt::KeyboardModifier modifier);

public slots:
    void slotToCall() { emit slotWithoutArgCalled(); }
    void slotToCall(const QString &arg) { emit slotWithSingleArgCalled(arg); }
    void slotToCall(const QString &arg, const QString &arg2, const QString &arg3 = QString())
    {
        slotWithArgumentsCalled(arg, arg2, arg3);
    }
    void slotToCall(const QString &arg, Qt::KeyboardModifier modifier, Qt::KeyboardModifiers blah = Qt::ShiftModifier)
    {
        emit slotWithOverloadedArgumentsCalled(arg, modifier, blah);
    }
    void slotToCallTwoDefault(const QString &arg, Qt::KeyboardModifiers modifiers = Qt::ShiftModifier | Qt::ControlModifier, Qt::KeyboardModifier modifier = Qt::AltModifier)
    {
        emit slotWithTwoOverloadedArgumentsCalled(arg, modifiers, modifier);
    }
};

void tst_QJSEngine::callQObjectSlot()
{
    OverloadedSlots dummy;
    QJSEngine eng;
    eng.globalObject().setProperty("dummy", eng.newQObject(&dummy));
    QQmlEngine::setObjectOwnership(&dummy, QQmlEngine::CppOwnership);

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithoutArgCalled()));
        eng.evaluate("dummy.slotToCall();");
        QCOMPARE(spy.size(), 1);
    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithSingleArgCalled(QString)));
        eng.evaluate("dummy.slotToCall('arg');");

        QCOMPARE(spy.size(), 1);
        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithArgumentsCalled(QString,QString,QString)));
        eng.evaluate("dummy.slotToCall('arg', 'arg2');");
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(arguments.at(1).toString(), QString("arg2"));
        QCOMPARE(arguments.at(2).toString(), QString());
    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithArgumentsCalled(QString,QString,QString)));
        eng.evaluate("dummy.slotToCall('arg', 'arg2', 'arg3');");
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(arguments.at(1).toString(), QString("arg2"));
        QCOMPARE(arguments.at(2).toString(), QString("arg3"));
    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithOverloadedArgumentsCalled(QString,Qt::KeyboardModifier,Qt::KeyboardModifiers)));
        eng.evaluate(QStringLiteral("dummy.slotToCall('arg', %1);").arg(QString::number(Qt::ControlModifier)));
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.first();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(arguments.at(1).toInt(), int(Qt::ControlModifier));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifiers>(arguments.at(2))), int(Qt::ShiftModifier));

    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithTwoOverloadedArgumentsCalled(QString,Qt::KeyboardModifiers,Qt::KeyboardModifier)));
        QJSValue v = eng.evaluate(QStringLiteral("dummy.slotToCallTwoDefault('arg', %1);").arg(QString::number(Qt::MetaModifier | Qt::KeypadModifier)));
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.first();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifiers>(arguments.at(1))), int(Qt::MetaModifier | Qt::KeypadModifier));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifier>(arguments.at(2))), int(Qt::AltModifier));
    }

    QJSValue jsArray = eng.newArray();
    jsArray.setProperty(QStringLiteral("MetaModifier"), QJSValue(Qt::MetaModifier));
    jsArray.setProperty(QStringLiteral("ShiftModifier"), QJSValue(Qt::ShiftModifier));
    jsArray.setProperty(QStringLiteral("ControlModifier"), QJSValue(Qt::ControlModifier));
    jsArray.setProperty(QStringLiteral("KeypadModifier"), QJSValue(Qt::KeypadModifier));

    QJSValue value = eng.newQObject(new QObject);
    value.setPrototype(jsArray);
    eng.globalObject().setProperty(QStringLiteral("Qt"), value);

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithOverloadedArgumentsCalled(QString,Qt::KeyboardModifier,Qt::KeyboardModifiers)));
        QJSValue v = eng.evaluate(QStringLiteral("dummy.slotToCall('arg', Qt.ControlModifier);"));
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.first();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(arguments.at(1).toInt(), int(Qt::ControlModifier));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifiers>(arguments.at(2))), int(Qt::ShiftModifier));
    }

    {
        QSignalSpy spy(&dummy, SIGNAL(slotWithTwoOverloadedArgumentsCalled(QString,Qt::KeyboardModifiers,Qt::KeyboardModifier)));
        QJSValue v = eng.evaluate(QStringLiteral("dummy.slotToCallTwoDefault('arg', Qt.MetaModifier | Qt.KeypadModifier);"));
        QCOMPARE(spy.size(), 1);

        const QList<QVariant> arguments = spy.first();
        QCOMPARE(arguments.at(0).toString(), QString("arg"));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifiers>(arguments.at(1))), int(Qt::MetaModifier | Qt::KeypadModifier));
        QCOMPARE(int(qvariant_cast<Qt::KeyboardModifier>(arguments.at(2))), int(Qt::AltModifier));
    }

}

void tst_QJSEngine::constructWithParent()
{
    QPointer<QJSEngine> ptr;
    {
        QObject obj;
        QJSEngine *engine = new QJSEngine(&obj);
        ptr = engine;
    }
    QVERIFY(ptr.isNull());
}

void tst_QJSEngine::newObject()
{
    QJSEngine eng;
    QJSValue object = eng.newObject();
    QVERIFY(!object.isUndefined());
    QCOMPARE(object.isObject(), true);
    QCOMPARE(object.isCallable(), false);
    // prototype should be Object.prototype
    QVERIFY(!object.prototype().isUndefined());
    QCOMPARE(object.prototype().isObject(), true);
    QCOMPARE(object.prototype().strictlyEquals(eng.evaluate("Object.prototype")), true);
}

void tst_QJSEngine::newArray()
{
    QJSEngine eng;
    QJSValue array = eng.newArray();
    QVERIFY(!array.isUndefined());
    QCOMPARE(array.isArray(), true);
    QCOMPARE(array.isObject(), true);
    QVERIFY(!array.isCallable());
    // prototype should be Array.prototype
    QVERIFY(!array.prototype().isUndefined());
    QCOMPARE(array.prototype().isArray(), true);
    QCOMPARE(array.prototype().strictlyEquals(eng.evaluate("Array.prototype")), true);
}

void tst_QJSEngine::newArray_HooliganTask218092()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("[].splice(0, 0, 'a')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 0);
    }
    {
        QJSValue ret = eng.evaluate("['a'].splice(0, 1, 'b')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 1);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b'].splice(0, 1, 'c')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 1);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b', 'c'].splice(0, 2, 'd')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 2);
    }
    {
        QJSValue ret = eng.evaluate("['a', 'b', 'c'].splice(1, 2, 'd', 'e', 'f')");
        QVERIFY(ret.isArray());
        QCOMPARE(ret.property("length").toInt(), 2);
    }
}

void tst_QJSEngine::newArray_HooliganTask233836()
{
    QJSEngine eng;
    {
        // According to ECMA-262, this should cause a RangeError.
        QJSValue ret = eng.evaluate("a = new Array(4294967295); a.push('foo')");
        QVERIFY(ret.isError() && ret.toString().contains(QLatin1String("RangeError")));
    }
    {
        QJSValue ret = eng.newArray(0xFFFFFFFF);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        ret.setProperty(0xFFFFFFFF, 123);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        QVERIFY(ret.property(0xFFFFFFFF).isNumber());
        QCOMPARE(ret.property(0xFFFFFFFF).toInt(), 123);
        ret.setProperty(123, 456);
        QCOMPARE(ret.property("length").toUInt(), uint(0xFFFFFFFF));
        QVERIFY(ret.property(123).isNumber());
        QCOMPARE(ret.property(123).toInt(), 456);
    }
}

void tst_QJSEngine::toScriptValueBuiltin_data()
{
    QTest::addColumn<QVariant>("input");

    QTest::newRow("UnknownType") << QVariant(QMetaType(QMetaType::UnknownType), nullptr);
    QTest::newRow("Nullptr") << QVariant(QMetaType(QMetaType::Nullptr), nullptr);
    QTest::newRow("true") << QVariant(true);
    QTest::newRow("false") << QVariant(false);
    QTest::newRow("int") << QVariant(int(42));
    QTest::newRow("uint") << QVariant(uint(42));
    QTest::newRow("longlong") << QVariant(qlonglong(4242));
    QTest::newRow("ulonglong") << QVariant(qulonglong(4242));
    QTest::newRow("double") << QVariant(double(42.42));
    QTest::newRow("float") << QVariant(float(42.42));
    QTest::newRow("qstring") << QVariant(QString::fromLatin1("hello"));
    QTest::newRow("qbytearray") << QVariant(QByteArray("hello"));
    QTest::newRow("short") << QVariant(short('r'));
    QTest::newRow("ushort") << QVariant(short('b'));
    QTest::newRow("char") << QVariant(char('r'));
    QTest::newRow("uchar") << QVariant(uchar('b'));
    QTest::newRow("qchar") << QVariant(QString::fromUtf8("å").at(0));
    QTest::newRow("qdate") << QVariant(QDate(1925, 5, 8));
    QTest::newRow("qtime") << QVariant(QTime(4, 5, 6));
    QTest::newRow("qregularexpression") << QVariant(QRegularExpression(".*"));
    QTest::newRow("qpointf") << QVariant(QPointF(42, 24));
    QTest::newRow("qvariantlist") << QVariant(QVariantList() << 42.24 << 5 << "hello");
    QTest::newRow("qvariantlist_point") << QVariant(QVariantList() << 42.24 << QPointF(42.24, 24.42) << QPointF(24.42, 42.24));
    QVariantMap vm; vm.insert("test", 55); vm.insert("abc", 42.42);
    QTest::newRow("qvariantmap") << QVariant(vm);
    vm.clear(); vm.insert("point1", QPointF(42.24, 24.42)); vm.insert("point2", QPointF(42.24, 24.42));
    QTest::newRow("qvariantmap_point") << QVariant(vm);
    QTest::newRow("qvariant") << QVariant(QVariant(42));
    QTest::newRow("QList<QString>") << QVariant::fromValue(QVector<QString>() << "1" << "2" << "3" << "4");
    QTest::newRow("QStringList") << QVariant::fromValue(QStringList() << "1" << "2" << "3" << "4");
    QTest::newRow("QMap<QString, QString>") << QVariant::fromValue(QMap<QString, QString>{{ "1", "2" }, { "3", "4" }});
    QTest::newRow("QHash<QString, QString>") << QVariant::fromValue(QHash<QString, QString>{{ "1", "2" }, { "3", "4" }});
    QTest::newRow("QMap<QString, QPointF>") << QVariant::fromValue(QMap<QString, QPointF>{{ "1", { 42.24, 24.42 } }, { "3", { 24.42, 42.24 } }});
    QTest::newRow("QHash<QString, QPointF>") << QVariant::fromValue(QHash<QString, QPointF>{{ "1", { 42.24, 24.42 } }, { "3", { 24.42, 42.24 } }});
}

void tst_QJSEngine::toScriptValueBuiltin()
{
    QFETCH(QVariant, input);

    QJSEngine engine;
    QJSValue outputJS = engine.toScriptValue(input);
    QVariant output = engine.fromScriptValue<QVariant>(outputJS);

    if (input.metaType().id() == QMetaType::QChar) {
        if (!input.convert(QMetaType(QMetaType::QString)))
            QFAIL("cannot convert to the original value");
    } else if (!output.convert(input.metaType()) && input.isValid())
        QFAIL("cannot convert to the original value");
    QCOMPARE(input, output);
}

void tst_QJSEngine::toScriptValueQmlBuiltin_data()
{
    QTest::addColumn<QVariant>("input");

    QTest::newRow("QList<QVariant>") << QVariant(QList<QVariant>{true, 5, 13.2f, 42.24, QString("world"), QUrl("htt://a.com"), QDateTime::currentDateTime(), QRegularExpression("a*b*c"), QByteArray("hello")});
    QTest::newRow("QList<bool>") << QVariant::fromValue(QList<bool>{true, false, true, false});
    QTest::newRow("QList<int>") << QVariant::fromValue(QList<int>{1, 2, 3, 4});
    QTest::newRow("QList<float>") << QVariant::fromValue(QList<float>{1.1f, 2.2f, 3.3f, 4.4f});
    QTest::newRow("QList<double>") << QVariant::fromValue(QList<double>{1.1, 2.2, 3.3, 4.4});
    QTest::newRow("QList<QString>") << QVariant::fromValue(QList<QString>{"a", "b", "c", "d"});
    QTest::newRow("QList<QUrl>") << QVariant::fromValue(QList<QUrl>{QUrl("htt://a.com"), QUrl("file:///tmp/b/"), QUrl("c.foo"), QUrl("/some/d")});
    QTest::newRow("QList<QDateTime>") << QVariant::fromValue(QList<QDateTime>{QDateTime::currentDateTime(), QDateTime::fromMSecsSinceEpoch(300), QDateTime()});
    QTest::newRow("QList<QRegularExpression>") << QVariant::fromValue(QList<QRegularExpression>{QRegularExpression("abcd"), QRegularExpression("a[b|c]d$"), QRegularExpression("a*b*d")});
    QTest::newRow("QList<QByteArray>") << QVariant::fromValue(QList<QByteArray>{QByteArray("aaa"), QByteArray("bbb"), QByteArray("ccc")});
}

void tst_QJSEngine::toScriptValueQmlBuiltin()
{
    QFETCH(QVariant, input);

    // We need the type registrations in QQmlEngine::init() for this.
    QQmlEngine engine;

    QJSValue outputJS = engine.toScriptValue(input);
    QVariant output = engine.fromScriptValue<QVariant>(outputJS);

    QVERIFY(output.convert(input.metaType()));
    QCOMPARE(input, output);
}

void tst_QJSEngine::toScriptValueQtQml_data()
{
    QTest::addColumn<QVariant>("input");

    QTest::newRow("std::vector<qreal>") << QVariant::fromValue(std::vector<qreal>{.1, .2, .3, .4});
    QTest::newRow("QList<qreal>") << QVariant::fromValue(QList<qreal>{.1, .2, .3, .4});

    QTest::newRow("std::vector<int>") << QVariant::fromValue(std::vector<int>{1, 2, 3, 4});
    QTest::newRow("std::vector<bool>") << QVariant::fromValue(std::vector<bool>{true, false, true, false});
    QTest::newRow("std::vector<QString>") << QVariant::fromValue(std::vector<QString>{"a", "b", "c", "d"});
    QTest::newRow("std::vector<QUrl>") << QVariant::fromValue(std::vector<QUrl>{QUrl("htt://a.com"), QUrl("file:///tmp/b/"), QUrl("c.foo"), QUrl("/some/d")});

    QTest::newRow("QList<QPoint>") << QVariant::fromValue(QList<QPointF>() << QPointF(42.24, 24.42) << QPointF(42.24, 24.42));

    static const QStandardItemModel model(4, 4);
    QTest::newRow("QModelIndexList") << QVariant::fromValue(QModelIndexList{ model.index(1, 2), model.index(2, 3), model.index(3, 1), model.index(3, 2)});
    QTest::newRow("std::vector<QModelIndex>") << QVariant::fromValue(std::vector<QModelIndex>{ model.index(1, 2), model.index(2, 3), model.index(3, 1), model.index(3, 2)});

    // QVariant wants to implicitly convert this to a QList<QItemSelectionRange>. Prevent that by
    // keeping the instance on the stack, and explicitly instantiating the template below.
    QItemSelection selection;

    // It doesn't have an initializer list ctor ...
    selection << QItemSelectionRange(model.index(1, 2), model.index(2, 3))
              << QItemSelectionRange(model.index(3, 1), model.index(3, 2))
              << QItemSelectionRange(model.index(2, 2), model.index(3, 3))
              << QItemSelectionRange(model.index(1, 1), model.index(2, 2));

    QTest::newRow("QItemSelection") << QVariant::fromValue<QItemSelection>(selection);
}

void tst_QJSEngine::toScriptValueQtQml()
{
    QFETCH(QVariant, input);

    // Import QtQml, to enable the sequential containers defined there.
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QtQml\nQtObject{}", QUrl());
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());

    QJSValue outputJS = engine.toScriptValue(input);
    QVariant output = engine.fromScriptValue<QVariant>(outputJS);

    if (input.metaType().id() == QMetaType::QChar) {
        if (!input.convert(QMetaType(QMetaType::QString)))
            QFAIL("cannot convert to the original value");
    } else if (!output.convert(input.metaType()) && input.isValid())
        QFAIL("cannot convert to the original value");
    QCOMPARE(input, output);
}

void tst_QJSEngine::toScriptValuenotroundtripped_data()
{
    QTest::addColumn<QVariant>("input");
    QTest::addColumn<QVariant>("output");

    QTest::newRow("QList<QObject*>") << QVariant::fromValue(QList<QObject*>() << this) << QVariant(QVariantList() << QVariant::fromValue<QObject *>(this));
    QTest::newRow("QObjectList") << QVariant::fromValue(QObjectList() << this) << QVariant(QVariantList() << QVariant::fromValue<QObject *>(this));
    QTest::newRow("VoidStar") << QVariant(QMetaType(QMetaType::VoidStar), nullptr) << QVariant(QMetaType(QMetaType::Nullptr), nullptr);
}

// This is almost the same as toScriptValue, but the inputs don't roundtrip to
// exactly the same value.
void tst_QJSEngine::toScriptValuenotroundtripped()
{
    QFETCH(QVariant, input);
    QFETCH(QVariant, output);

    QJSEngine engine;
    QJSValue outputJS = engine.toScriptValue(input);
    QVariant actualOutput = engine.fromScriptValue<QVariant>(outputJS);

    QCOMPARE(actualOutput, output);
}

void tst_QJSEngine::newVariant()
{
    QJSEngine eng;
    {
        QJSValue opaque = eng.toScriptValue(QVariant(QPoint(1, 2)));
        QVERIFY(!opaque.isUndefined());
        QCOMPARE(opaque.isVariant(), false);
        QVERIFY(!opaque.isCallable());
        QCOMPARE(opaque.isObject(), true);
        QVERIFY(!opaque.prototype().isUndefined());
        QCOMPARE(opaque.prototype().isVariant(), false);
        QVERIFY(opaque.property("valueOf").callWithInstance(opaque).equals(opaque));
    }
}

void tst_QJSEngine::newVariant_valueOfToString()
{
    // valueOf() and toString()
    QJSEngine eng;
    {
        QJSValue object = eng.toScriptValue(QVariant(QPoint(10, 20)));
        QJSValue value = object.property("valueOf").callWithInstance(object);
        QVERIFY(value.isObject());
        QVERIFY(value.strictlyEquals(object));
        QCOMPARE(object.toString(), QString::fromLatin1("QPoint(10, 20)"));
    }
}

void tst_QJSEngine::newVariant_valueOfEnum()
{
    QJSEngine eng;
    {
        QJSManagedValue object = eng.toManagedValue(QVariant::fromValue(Qt::ControlModifier));
        QJSValue value = object.property("valueOf").callWithInstance(object.toJSValue());
        QVERIFY(value.isNumber());
        QCOMPARE(value.toInt(), static_cast<qint32>(Qt::ControlModifier));
    }
}

void tst_QJSEngine::newRegExp()
{
    QJSEngine eng;
    QJSValue rexp = eng.toScriptValue(QRegularExpression("foo"));
    QVERIFY(!rexp.isUndefined());
    QCOMPARE(rexp.isRegExp(), true);
    QCOMPARE(rexp.isObject(), true);
    QCOMPARE(rexp.isCallable(), false);
    // prototype should be RegExp.prototype
    QVERIFY(!rexp.prototype().isUndefined());
    QCOMPARE(rexp.prototype().isObject(), true);
    // Get [[Class]] internal property of RegExp Prototype Object.
    // See ECMA-262 Section 8.6.2, "Object Internal Properties and Methods".
    // See ECMA-262 Section 15.10.6, "Properties of the RegExp Prototype Object".
    QJSValue r = eng.evaluate("Object.prototype.toString.call(RegExp.prototype)");
    QCOMPARE(r.toString(), QString::fromLatin1("[object Object]"));
    QCOMPARE(rexp.prototype().strictlyEquals(eng.evaluate("RegExp.prototype")), true);

    QCOMPARE(qjsvalue_cast<QRegularExpression>(rexp).pattern(), QRegularExpression("foo").pattern());
}

void tst_QJSEngine::jsRegExp()
{
    // See ECMA-262 Section 15.10, "RegExp Objects".
    // These should really be JS-only tests, as they test the implementation's
    // ECMA-compliance, not the C++ API. Compliance should already be covered
    // by the Mozilla tests (qscriptjstestsuite).
    // We can consider updating the expected results of this test if the
    // RegExp implementation changes.

    QJSEngine eng;
    QJSValue r = eng.evaluate("/foo/gim");
    QVERIFY(r.isRegExp());
    QCOMPARE(r.toString(), QString::fromLatin1("/foo/gim"));

    QJSValue rxCtor = eng.globalObject().property("RegExp");
    QJSValue r2 = rxCtor.call(QJSValueList() << r);
    QVERIFY(r2.isRegExp());
    QVERIFY(r2.strictlyEquals(r));

    QJSValue r3 = rxCtor.call(QJSValueList() << r << "gim");
    QVERIFY(!r3.isError());

    QJSValue r4 = rxCtor.call(QJSValueList() << "foo" << "gim");
    QVERIFY(r4.isRegExp());

    QJSValue r5 = rxCtor.callAsConstructor(QJSValueList() << r);
    QVERIFY(r5.isRegExp());
    QCOMPARE(r5.toString(), QString::fromLatin1("/foo/gim"));
    QVERIFY(r5.strictlyEquals(r));

    // See ECMA-262 Section 15.10.4.1, "new RegExp(pattern, flags)".
    QJSValue r6 = rxCtor.callAsConstructor(QJSValueList() << "foo" << "bar");
    QVERIFY(r6.isError());
    QVERIFY(r6.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r7 = eng.evaluate("/foo/gimp");
    QVERIFY(r7.isError());
    QVERIFY(r7.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r8 = eng.evaluate("/foo/migmigmig");
    QVERIFY(r8.isError());
    QVERIFY(r8.toString().contains(QString::fromLatin1("SyntaxError"))); // Invalid regular expression flag

    QJSValue r9 = rxCtor.callAsConstructor();
    QVERIFY(r9.isRegExp());
    QCOMPARE(r9.toString(), QString::fromLatin1("/(?:)/"));

    QJSValue r10 = rxCtor.callAsConstructor(QJSValueList() << "" << "gim");
    QVERIFY(r10.isRegExp());
    QCOMPARE(r10.toString(), QString::fromLatin1("/(?:)/gim"));

    QJSValue r11 = rxCtor.callAsConstructor(QJSValueList() << "{1.*}" << "g");
    QVERIFY(r11.isRegExp());
    QCOMPARE(r11.toString(), QString::fromLatin1("/{1.*}/g"));
}

void tst_QJSEngine::newDate()
{
    QJSEngine eng;

    {
        QJSValue date = eng.evaluate("new Date(0)");
        QVERIFY(!date.isUndefined());
        QCOMPARE(date.isDate(), true);
        QCOMPARE(date.isObject(), true);
        QVERIFY(!date.isCallable());
    }

    {
        QDateTime dt = QDateTime(QDate(1, 2, 3), QTime(4, 5, 6, 7));
        QJSValue date = eng.toScriptValue(dt);
        QVERIFY(!date.isUndefined());
        QCOMPARE(date.isDate(), true);
        QCOMPARE(date.isObject(), true);

        QCOMPARE(date.toDateTime(), dt);
    }

    {
        QDateTime dt = QDateTime(QDate(1, 2, 3), QTime(4, 5, 6, 7), QTimeZone::UTC);
        QJSValue date = eng.toScriptValue(dt);
        // toDateTime() result should be in local time
        QCOMPARE(date.toDateTime(), dt.toLocalTime());
    }
}

void tst_QJSEngine::jsParseDate()
{
    QJSEngine eng;
    // Date.parse() should return NaN when it fails
    {
        QJSValue ret = eng.evaluate("Date.parse()");
        QVERIFY(ret.isNumber());
        QVERIFY(qIsNaN(ret.toNumber()));
    }

    // Date.parse() should be able to parse the output of Date().toString()
    {
        QJSValue ret = eng.evaluate("var x = new Date(); var s = x.toString(); s == new Date(Date.parse(s)).toString()");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), true);
    }
}

void tst_QJSEngine::newQObject()
{
    QJSEngine eng;
    QObject temp;

    {
        QJSValue qobject = eng.newQObject(nullptr);
        QCOMPARE(qobject.isNull(), true);
        QCOMPARE(qobject.isObject(), false);
        QCOMPARE(qobject.toQObject(), (QObject *)nullptr);
    }
    {
        QJSValue qobject = eng.newQObject(&temp);
        QVERIFY(!qobject.isUndefined());
        QCOMPARE(qobject.isQObject(), true);
        QCOMPARE(qobject.isObject(), true);
        QCOMPARE(qobject.toQObject(), (QObject *)&temp);
        QVERIFY(!qobject.isCallable());
        // prototype should be QObject.prototype
        QCOMPARE(qobject.prototype().isObject(), true);
        QEXPECT_FAIL("", "FIXME: newly created QObject's prototype is an JS Object", Continue);
        QCOMPARE(qobject.prototype().isQObject(), true);
    }
}

void tst_QJSEngine::newQObjectRace()
{
    class Thread : public QThread
    {
        void run() override
        {
            int newObjectCount = 1000;
#if defined(Q_OS_QNX)
            newObjectCount = 128;
#endif
            for (int i=0;i<newObjectCount;++i)
            {
                QJSEngine e;
                auto obj = e.newQObject(new QObject);
            }
        }
    };


    Thread threads[8];
    for (auto& t : threads)
        t.start(); // should not crash
    for (auto& t : threads)
        t.wait();
}

void tst_QJSEngine::newQObject_ownership()
{
    QJSEngine eng;
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr != nullptr);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        gc(*eng.handle(), GCFlags::DontSendPostedEvents);
        if (ptr)
            QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
        QVERIFY(ptr.isNull());
    }
    {
        QPointer<QObject> ptr = new QObject(this);
        QVERIFY(ptr != nullptr);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        QObject *before = ptr;
        gc(*eng.handle(), GCFlags::DontSendPostedEvents);
        QCOMPARE(ptr.data(), before);
        delete ptr;
    }
    {
        std::unique_ptr<QObject> parent = std::make_unique<QObject>();
        QObject *child = new QObject(parent.get());
        QJSValue v = eng.newQObject(child);
        QCOMPARE(v.toQObject(), child);
        parent.reset();
        QCOMPARE(v.toQObject(), (QObject *)nullptr);
    }
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr != nullptr);
        {
            QJSValue v = eng.newQObject(ptr);
        }
        gc(*eng.handle(), GCFlags::DontSendPostedEvents);
        // no parent, so it should be like ScriptOwnership
        if (ptr)
            QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
        QVERIFY(ptr.isNull());
    }
    {
        std::unique_ptr<QObject> parent = std::make_unique<QObject>();
        QPointer<QObject> child = new QObject(parent.get());
        QVERIFY(child != nullptr);
        {
            QJSValue v = eng.newQObject(child);
        }
        gc(*eng.handle(), GCFlags::DontSendPostedEvents);
        // has parent, so it should be like QtOwnership
        QVERIFY(child != nullptr);
    }
    {
        QPointer<QObject> ptr = new QObject();
        QVERIFY(ptr != nullptr);
        {
            QQmlEngine::setObjectOwnership(ptr.data(), QQmlEngine::CppOwnership);
            QJSValue v = eng.newQObject(ptr);
        }
        gc(*eng.handle(), GCFlags::DontSendPostedEvents);
        if (ptr)
            QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
        QVERIFY(!ptr.isNull());
        delete ptr.data();
    }
}

void tst_QJSEngine::newQObject_deletedEngine()
{
    QJSValue object;
    QObject *ptr = new QObject();
    QSignalSpy spy(ptr, SIGNAL(destroyed()));
    {
        QJSEngine engine;
        object = engine.newQObject(ptr);
        engine.globalObject().setProperty("obj", object);
    }
    QTRY_VERIFY(spy.size());
}

class TestQMetaObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(int called READ called)
public:
    enum Enum1 {
        Zero = 0,
        One,
        Two
    };
    enum Enum2 {
        A = 0,
        B,
        C
    };
    Q_ENUMS(Enum1 Enum2)

    Q_INVOKABLE TestQMetaObject() {}
    Q_INVOKABLE TestQMetaObject(int)
        : m_called(2) {}
    Q_INVOKABLE TestQMetaObject(QString)
        : m_called(3) {}
    Q_INVOKABLE TestQMetaObject(QString, int)
        : m_called(4) {}
    int called() const {
        return m_called;
    }
private:
    int m_called = 1;
};

class TestQMetaObject2 : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE TestQMetaObject2(int a) : m_called(a) {}
    int called() const { return m_called; }

private:
    int m_called = 1;
};

void tst_QJSEngine::newQObjectPropertyCache()
{
    QScopedPointer<QObject> obj(new QObject);
    QQmlEngine::setObjectOwnership(obj.data(), QQmlEngine::CppOwnership);

    {
        QJSEngine engine;
        engine.newQObject(obj.data());
        QVERIFY(QQmlData::get(obj.data())->propertyCache);
    }
    QVERIFY(!QQmlData::get(obj.data())->propertyCache);
}

void tst_QJSEngine::newQMetaObject() {
    {
        QJSEngine engine;
        QJSValue metaObject = engine.newQMetaObject(&TestQMetaObject::staticMetaObject);
        QCOMPARE(metaObject.isNull(), false);
        QCOMPARE(metaObject.isObject(), true);
        QCOMPARE(metaObject.isQObject(), false);
        QCOMPARE(metaObject.isCallable(), true);
        QCOMPARE(metaObject.isQMetaObject(), true);

        QCOMPARE(metaObject.toQMetaObject(), &TestQMetaObject::staticMetaObject);

        QVERIFY(metaObject.strictlyEquals(engine.newQMetaObject<TestQMetaObject>()));
        QVERIFY(!metaObject.strictlyEquals(engine.newArray()));


        {
        auto result = metaObject.callAsConstructor();
        if (result.isError())
            qDebug() << result.toString();
        QCOMPARE(result.isError(), false);
        QCOMPARE(result.isNull(), false);
        QCOMPARE(result.isObject(), true);
        QCOMPARE(result.isQObject(), true);
        QVERIFY(result.property("constructor").strictlyEquals(metaObject));
        QVERIFY(result.prototype().strictlyEquals(metaObject));


        QCOMPARE(result.property("called").toInt(), 1);

        }

        QJSValue integer(42);
        QJSValue string("foo");

        {
            auto result = metaObject.callAsConstructor({integer});
            QCOMPARE(result.property("called").toInt(), 2);
        }

        {
            auto result = metaObject.callAsConstructor({string});
            QCOMPARE(result.property("called").toInt(), 3);
        }

        {
            auto result = metaObject.callAsConstructor({string, integer});
            QCOMPARE(result.property("called").toInt(), 4);
        }
    }

    {
        QJSEngine engine;
        QJSValue metaObject = engine.newQMetaObject(&TestQMetaObject::staticMetaObject);
        QCOMPARE(metaObject.property("Zero").toInt(), 0);
        QCOMPARE(metaObject.property("One").toInt(), 1);
        QCOMPARE(metaObject.property("Two").toInt(), 2);
        QCOMPARE(metaObject.property("A").toInt(), 0);
        QCOMPARE(metaObject.property("B").toInt(), 1);
        QCOMPARE(metaObject.property("C").toInt(), 2);
    }

    {
        QJSEngine engine;
        const QJSValue metaObject = engine.newQMetaObject(&TestQMetaObject2::staticMetaObject);
        engine.globalObject().setProperty("Example"_L1, metaObject);

        const QJSValue invalid = engine.evaluate("new Example()"_L1);
        QVERIFY(invalid.isError());
        QCOMPARE(invalid.toString(), "Error: Insufficient arguments"_L1);

        const QJSValue valid = engine.evaluate("new Example(123)"_L1);
        QCOMPARE(qjsvalue_cast<TestQMetaObject2 *>(valid)->called(), 123);
    }
}

void tst_QJSEngine::exceptionInSlot()
{
    QJSEngine engine;
    QJSValue wrappedThis = engine.newQObject(this);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine.globalObject().setProperty("testCase", wrappedThis);
    engine.evaluate(
            "var called = false\n"
            "function throwingSlot() {\n"
            "    called = true\n"
            "    throw 42;\n"
            "}\n"
            "testCase.testSignal.connect(throwingSlot)\n"
            );
    QCOMPARE(engine.globalObject().property("called").toBool(), false);
    emit testSignal();
    QCOMPARE(engine.globalObject().property("called").toBool(), true);
}

void tst_QJSEngine::globalObjectProperties()
{
    // See ECMA-262 Section 15.1, "The Global Object".

    QJSEngine eng;
    QJSValue global = eng.globalObject();

    QVERIFY(global.property("NaN").isNumber());
    QVERIFY(qIsNaN(global.property("NaN").toNumber()));

    QVERIFY(global.property("Infinity").isNumber());
    QVERIFY(qIsInf(global.property("Infinity").toNumber()));

    QVERIFY(global.property("undefined").isUndefined());

    QVERIFY(global.property("eval").isCallable());

    QVERIFY(global.property("parseInt").isCallable());

    QVERIFY(global.property("parseFloat").isCallable());

    QVERIFY(global.property("isNaN").isCallable());

    QVERIFY(global.property("isFinite").isCallable());

    QVERIFY(global.property("decodeURI").isCallable());

    QVERIFY(global.property("decodeURIComponent").isCallable());

    QVERIFY(global.property("encodeURI").isCallable());

    QVERIFY(global.property("encodeURIComponent").isCallable());

    QVERIFY(global.property("Object").isCallable());
    QVERIFY(global.property("Function").isCallable());
    QVERIFY(global.property("Array").isCallable());
    QVERIFY(global.property("String").isCallable());
    QVERIFY(global.property("Boolean").isCallable());
    QVERIFY(global.property("Number").isCallable());
    QVERIFY(global.property("Date").isCallable());
    QVERIFY(global.property("RegExp").isCallable());
    QVERIFY(global.property("Error").isCallable());
    QVERIFY(global.property("EvalError").isCallable());
    QVERIFY(global.property("RangeError").isCallable());
    QVERIFY(global.property("ReferenceError").isCallable());
    QVERIFY(global.property("SyntaxError").isCallable());
    QVERIFY(global.property("TypeError").isCallable());
    QVERIFY(global.property("URIError").isCallable());
    QVERIFY(global.property("Math").isObject());
    QVERIFY(!global.property("Math").isCallable());
}

void tst_QJSEngine::globalObjectEquals()
{
    QJSEngine eng;
    QJSValue o = eng.globalObject();
    QVERIFY(o.strictlyEquals(eng.globalObject()));
    QVERIFY(o.equals(eng.globalObject()));
}

void tst_QJSEngine::globalObjectProperties_enumerate()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();

    QSet<QString> expectedNames;
    expectedNames
        << "isNaN"
        << "parseFloat"
        << "String"
        << "EvalError"
        << "URIError"
        << "Math"
        << "encodeURIComponent"
        << "RangeError"
        << "eval"
        << "isFinite"
        << "ReferenceError"
        << "Infinity"
        << "Function"
        << "RegExp"
        << "Number"
        << "parseInt"
        << "Object"
        << "decodeURI"
        << "TypeError"
        << "Boolean"
        << "encodeURI"
        << "NaN"
        << "Error"
        << "decodeURIComponent"
        << "Date"
        << "Array"
        << "Symbol"
        << "escape"
        << "unescape"
        << "SyntaxError"
        << "undefined"
        << "JSON"
        << "ArrayBuffer"
        << "SharedArrayBuffer"
        << "DataView"
        << "Int8Array"
        << "Uint8Array"
        << "Uint8ClampedArray"
        << "Int16Array"
        << "Uint16Array"
        << "Int32Array"
        << "Uint32Array"
        << "Float32Array"
        << "Float64Array"
        << "WeakSet"
        << "Set"
        << "WeakMap"
        << "Map"
        << "Reflect"
        << "Proxy"
        << "Atomics"
        << "Promise"
        << "URL"
        << "URLSearchParams"
        ;
    QSet<QString> actualNames;
    {
        QJSValueIterator it(global);
        while (it.hasNext()) {
            it.next();
            actualNames.insert(it.name());
        }
    }

    QSet<QString> remainingNames = actualNames;
    {
        QSet<QString>::const_iterator it;
        for (it = expectedNames.constBegin(); it != expectedNames.constEnd(); ++it) {
            QString name = *it;
            QVERIFY(actualNames.contains(name));
            remainingNames.remove(name);
        }
    }
    QVERIFY(remainingNames.isEmpty());
}

void tst_QJSEngine::createGlobalObjectProperty()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();
    // create property with no attributes
    {
        QString name = QString::fromLatin1("foo");
        QVERIFY(global.property(name).isUndefined());
        QJSValue val(123);
        global.setProperty(name, val);
        QVERIFY(global.property(name).equals(val));
        global.deleteProperty(name);
        QVERIFY(global.property(name).isUndefined());
    }
}

void tst_QJSEngine::globalObjectWithCustomPrototype()
{
    QJSEngine engine;
    QJSValue proto = engine.newObject();
    proto.setProperty("protoProperty", 123);
    QJSValue global = engine.globalObject();
    QJSValue originalProto = global.prototype();
    global.setPrototype(proto);
    {
        QJSValue ret = engine.evaluate("protoProperty");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("protoProperty")));
    }
    {
        QJSValue ret = engine.evaluate("this.protoProperty");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("protoProperty")));
    }
    {
        QJSValue ret = engine.evaluate("this.hasOwnProperty('protoProperty')");
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
    }

    // Custom prototype set from JS
    {
        QJSValue ret = engine.evaluate("this.__proto__ = { 'a': 123 }; a");
        QVERIFY(ret.isNumber());
        QVERIFY(ret.strictlyEquals(global.property("a")));
    }
}

void tst_QJSEngine::builtinFunctionNames_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedName");

    // See ECMA-262 Chapter 15, "Standard Built-in ECMAScript Objects".

    QTest::newRow("parseInt") << QString("parseInt") << QString("parseInt");
    QTest::newRow("parseFloat") << QString("parseFloat") << QString("parseFloat");
    QTest::newRow("isNaN") << QString("isNaN") << QString("isNaN");
    QTest::newRow("isFinite") << QString("isFinite") << QString("isFinite");
    QTest::newRow("decodeURI") << QString("decodeURI") << QString("decodeURI");
    QTest::newRow("decodeURIComponent") << QString("decodeURIComponent") << QString("decodeURIComponent");
    QTest::newRow("encodeURI") << QString("encodeURI") << QString("encodeURI");
    QTest::newRow("encodeURIComponent") << QString("encodeURIComponent") << QString("encodeURIComponent");
    QTest::newRow("escape") << QString("escape") << QString("escape");
    QTest::newRow("unescape") << QString("unescape") << QString("unescape");

    QTest::newRow("Array") << QString("Array") << QString("Array");
    QTest::newRow("Array.prototype.toString") << QString("Array.prototype.toString") << QString("toString");
    QTest::newRow("Array.prototype.toLocaleString") << QString("Array.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Array.prototype.concat") << QString("Array.prototype.concat") << QString("concat");
    QTest::newRow("Array.prototype.find") << QString("Array.prototype.find") << QString("find");
    QTest::newRow("Array.prototype.findIndex") << QString("Array.prototype.findIndex") << QString("findIndex");
    QTest::newRow("Array.prototype.join") << QString("Array.prototype.join") << QString("join");
    QTest::newRow("Array.prototype.pop") << QString("Array.prototype.pop") << QString("pop");
    QTest::newRow("Array.prototype.push") << QString("Array.prototype.push") << QString("push");
    QTest::newRow("Array.prototype.reverse") << QString("Array.prototype.reverse") << QString("reverse");
    QTest::newRow("Array.prototype.shift") << QString("Array.prototype.shift") << QString("shift");
    QTest::newRow("Array.prototype.slice") << QString("Array.prototype.slice") << QString("slice");
    QTest::newRow("Array.prototype.sort") << QString("Array.prototype.sort") << QString("sort");
    QTest::newRow("Array.prototype.splice") << QString("Array.prototype.splice") << QString("splice");
    QTest::newRow("Array.prototype.unshift") << QString("Array.prototype.unshift") << QString("unshift");

    QTest::newRow("Boolean") << QString("Boolean") << QString("Boolean");
    QTest::newRow("Boolean.prototype.toString") << QString("Boolean.prototype.toString") << QString("toString");

    QTest::newRow("Date") << QString("Date") << QString("Date");
    QTest::newRow("Date.prototype.toString") << QString("Date.prototype.toString") << QString("toString");
    QTest::newRow("Date.prototype.toDateString") << QString("Date.prototype.toDateString") << QString("toDateString");
    QTest::newRow("Date.prototype.toTimeString") << QString("Date.prototype.toTimeString") << QString("toTimeString");
    QTest::newRow("Date.prototype.toLocaleString") << QString("Date.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Date.prototype.toLocaleDateString") << QString("Date.prototype.toLocaleDateString") << QString("toLocaleDateString");
    QTest::newRow("Date.prototype.toLocaleTimeString") << QString("Date.prototype.toLocaleTimeString") << QString("toLocaleTimeString");
    QTest::newRow("Date.prototype.valueOf") << QString("Date.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Date.prototype.getTime") << QString("Date.prototype.getTime") << QString("getTime");
    QTest::newRow("Date.prototype.getYear") << QString("Date.prototype.getYear") << QString("getYear");
    QTest::newRow("Date.prototype.getFullYear") << QString("Date.prototype.getFullYear") << QString("getFullYear");
    QTest::newRow("Date.prototype.getUTCFullYear") << QString("Date.prototype.getUTCFullYear") << QString("getUTCFullYear");
    QTest::newRow("Date.prototype.getMonth") << QString("Date.prototype.getMonth") << QString("getMonth");
    QTest::newRow("Date.prototype.getUTCMonth") << QString("Date.prototype.getUTCMonth") << QString("getUTCMonth");
    QTest::newRow("Date.prototype.getDate") << QString("Date.prototype.getDate") << QString("getDate");
    QTest::newRow("Date.prototype.getUTCDate") << QString("Date.prototype.getUTCDate") << QString("getUTCDate");
    QTest::newRow("Date.prototype.getDay") << QString("Date.prototype.getDay") << QString("getDay");
    QTest::newRow("Date.prototype.getUTCDay") << QString("Date.prototype.getUTCDay") << QString("getUTCDay");
    QTest::newRow("Date.prototype.getHours") << QString("Date.prototype.getHours") << QString("getHours");
    QTest::newRow("Date.prototype.getUTCHours") << QString("Date.prototype.getUTCHours") << QString("getUTCHours");
    QTest::newRow("Date.prototype.getMinutes") << QString("Date.prototype.getMinutes") << QString("getMinutes");
    QTest::newRow("Date.prototype.getUTCMinutes") << QString("Date.prototype.getUTCMinutes") << QString("getUTCMinutes");
    QTest::newRow("Date.prototype.getSeconds") << QString("Date.prototype.getSeconds") << QString("getSeconds");
    QTest::newRow("Date.prototype.getUTCSeconds") << QString("Date.prototype.getUTCSeconds") << QString("getUTCSeconds");
    QTest::newRow("Date.prototype.getMilliseconds") << QString("Date.prototype.getMilliseconds") << QString("getMilliseconds");
    QTest::newRow("Date.prototype.getUTCMilliseconds") << QString("Date.prototype.getUTCMilliseconds") << QString("getUTCMilliseconds");
    QTest::newRow("Date.prototype.getTimezoneOffset") << QString("Date.prototype.getTimezoneOffset") << QString("getTimezoneOffset");
    QTest::newRow("Date.prototype.setTime") << QString("Date.prototype.setTime") << QString("setTime");
    QTest::newRow("Date.prototype.setMilliseconds") << QString("Date.prototype.setMilliseconds") << QString("setMilliseconds");
    QTest::newRow("Date.prototype.setUTCMilliseconds") << QString("Date.prototype.setUTCMilliseconds") << QString("setUTCMilliseconds");
    QTest::newRow("Date.prototype.setSeconds") << QString("Date.prototype.setSeconds") << QString("setSeconds");
    QTest::newRow("Date.prototype.setUTCSeconds") << QString("Date.prototype.setUTCSeconds") << QString("setUTCSeconds");
    QTest::newRow("Date.prototype.setMinutes") << QString("Date.prototype.setMinutes") << QString("setMinutes");
    QTest::newRow("Date.prototype.setUTCMinutes") << QString("Date.prototype.setUTCMinutes") << QString("setUTCMinutes");
    QTest::newRow("Date.prototype.setHours") << QString("Date.prototype.setHours") << QString("setHours");
    QTest::newRow("Date.prototype.setUTCHours") << QString("Date.prototype.setUTCHours") << QString("setUTCHours");
    QTest::newRow("Date.prototype.setDate") << QString("Date.prototype.setDate") << QString("setDate");
    QTest::newRow("Date.prototype.setUTCDate") << QString("Date.prototype.setUTCDate") << QString("setUTCDate");
    QTest::newRow("Date.prototype.setMonth") << QString("Date.prototype.setMonth") << QString("setMonth");
    QTest::newRow("Date.prototype.setUTCMonth") << QString("Date.prototype.setUTCMonth") << QString("setUTCMonth");
    QTest::newRow("Date.prototype.setYear") << QString("Date.prototype.setYear") << QString("setYear");
    QTest::newRow("Date.prototype.setFullYear") << QString("Date.prototype.setFullYear") << QString("setFullYear");
    QTest::newRow("Date.prototype.setUTCFullYear") << QString("Date.prototype.setUTCFullYear") << QString("setUTCFullYear");
    QTest::newRow("Date.prototype.toUTCString") << QString("Date.prototype.toUTCString") << QString("toUTCString");
    QTest::newRow("Date.prototype.toGMTString") << QString("Date.prototype.toGMTString") << QString("toUTCString"); // yes, this is per spec

    QTest::newRow("Error") << QString("Error") << QString("Error");
//    QTest::newRow("Error.prototype.backtrace") << QString("Error.prototype.backtrace") << QString("backtrace");
    QTest::newRow("Error.prototype.toString") << QString("Error.prototype.toString") << QString("toString");

    QTest::newRow("EvalError") << QString("EvalError") << QString("EvalError");
    QTest::newRow("RangeError") << QString("RangeError") << QString("RangeError");
    QTest::newRow("ReferenceError") << QString("ReferenceError") << QString("ReferenceError");
    QTest::newRow("SyntaxError") << QString("SyntaxError") << QString("SyntaxError");
    QTest::newRow("TypeError") << QString("TypeError") << QString("TypeError");
    QTest::newRow("URIError") << QString("URIError") << QString("URIError");

    QTest::newRow("Function") << QString("Function") << QString("Function");
    QTest::newRow("Function.prototype.toString") << QString("Function.prototype.toString") << QString("toString");
    QTest::newRow("Function.prototype.apply") << QString("Function.prototype.apply") << QString("apply");
    QTest::newRow("Function.prototype.call") << QString("Function.prototype.call") << QString("call");
/*  In V8, those function are only there for signals
    QTest::newRow("Function.prototype.connect") << QString("Function.prototype.connect") << QString("connect");
    QTest::newRow("Function.prototype.disconnect") << QString("Function.prototype.disconnect") << QString("disconnect");*/

    QTest::newRow("Math.abs") << QString("Math.abs") << QString("abs");
    QTest::newRow("Math.acos") << QString("Math.acos") << QString("acos");
    QTest::newRow("Math.asin") << QString("Math.asin") << QString("asin");
    QTest::newRow("Math.atan") << QString("Math.atan") << QString("atan");
    QTest::newRow("Math.atan2") << QString("Math.atan2") << QString("atan2");
    QTest::newRow("Math.ceil") << QString("Math.ceil") << QString("ceil");
    QTest::newRow("Math.cos") << QString("Math.cos") << QString("cos");
    QTest::newRow("Math.exp") << QString("Math.exp") << QString("exp");
    QTest::newRow("Math.floor") << QString("Math.floor") << QString("floor");
    QTest::newRow("Math.log") << QString("Math.log") << QString("log");
    QTest::newRow("Math.max") << QString("Math.max") << QString("max");
    QTest::newRow("Math.min") << QString("Math.min") << QString("min");
    QTest::newRow("Math.pow") << QString("Math.pow") << QString("pow");
    QTest::newRow("Math.random") << QString("Math.random") << QString("random");
    QTest::newRow("Math.round") << QString("Math.round") << QString("round");
    QTest::newRow("Math.sign") << QString("Math.sign") << QString("sign");
    QTest::newRow("Math.sin") << QString("Math.sin") << QString("sin");
    QTest::newRow("Math.sqrt") << QString("Math.sqrt") << QString("sqrt");
    QTest::newRow("Math.tan") << QString("Math.tan") << QString("tan");

    QTest::newRow("Number") << QString("Number") << QString("Number");
    QTest::newRow("Number.isFinite") << QString("Number.isFinite") << QString("isFinite");
    QTest::newRow("Number.isNaN") << QString("Number.isNaN") << QString("isNaN");
    QTest::newRow("Number.prototype.toString") << QString("Number.prototype.toString") << QString("toString");
    QTest::newRow("Number.prototype.toLocaleString") << QString("Number.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Number.prototype.valueOf") << QString("Number.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Number.prototype.toFixed") << QString("Number.prototype.toFixed") << QString("toFixed");
    QTest::newRow("Number.prototype.toExponential") << QString("Number.prototype.toExponential") << QString("toExponential");
    QTest::newRow("Number.prototype.toPrecision") << QString("Number.prototype.toPrecision") << QString("toPrecision");

    QTest::newRow("Object") << QString("Object") << QString("Object");
    QTest::newRow("Object.prototype.toString") << QString("Object.prototype.toString") << QString("toString");
    QTest::newRow("Object.prototype.toLocaleString") << QString("Object.prototype.toLocaleString") << QString("toLocaleString");
    QTest::newRow("Object.prototype.valueOf") << QString("Object.prototype.valueOf") << QString("valueOf");
    QTest::newRow("Object.prototype.hasOwnProperty") << QString("Object.prototype.hasOwnProperty") << QString("hasOwnProperty");
    QTest::newRow("Object.prototype.isPrototypeOf") << QString("Object.prototype.isPrototypeOf") << QString("isPrototypeOf");
    QTest::newRow("Object.prototype.propertyIsEnumerable") << QString("Object.prototype.propertyIsEnumerable") << QString("propertyIsEnumerable");
    QTest::newRow("Object.prototype.__defineGetter__") << QString("Object.prototype.__defineGetter__") << QString("__defineGetter__");
    QTest::newRow("Object.prototype.__defineSetter__") << QString("Object.prototype.__defineSetter__") << QString("__defineSetter__");

    QTest::newRow("RegExp") << QString("RegExp") << QString("RegExp");
    QTest::newRow("RegExp.prototype.exec") << QString("RegExp.prototype.exec") << QString("exec");
    QTest::newRow("RegExp.prototype.test") << QString("RegExp.prototype.test") << QString("test");
    QTest::newRow("RegExp.prototype.toString") << QString("RegExp.prototype.toString") << QString("toString");

    QTest::newRow("String") << QString("String") << QString("String");
    QTest::newRow("String.prototype.toString") << QString("String.prototype.toString") << QString("toString");
    QTest::newRow("String.prototype.valueOf") << QString("String.prototype.valueOf") << QString("valueOf");
    QTest::newRow("String.prototype.charAt") << QString("String.prototype.charAt") << QString("charAt");
    QTest::newRow("String.prototype.charCodeAt") << QString("String.prototype.charCodeAt") << QString("charCodeAt");
    QTest::newRow("String.prototype.concat") << QString("String.prototype.concat") << QString("concat");
    QTest::newRow("String.prototype.endsWith") << QString("String.prototype.endsWith") << QString("endsWith");
    QTest::newRow("String.prototype.includes") << QString("String.prototype.includes") << QString("includes");
    QTest::newRow("String.prototype.indexOf") << QString("String.prototype.indexOf") << QString("indexOf");
    QTest::newRow("String.prototype.lastIndexOf") << QString("String.prototype.lastIndexOf") << QString("lastIndexOf");
    QTest::newRow("String.prototype.localeCompare") << QString("String.prototype.localeCompare") << QString("localeCompare");
    QTest::newRow("String.prototype.match") << QString("String.prototype.match") << QString("match");
    QTest::newRow("String.prototype.repeat") << QString("String.prototype.repeat") << QString("repeat");
    QTest::newRow("String.prototype.replace") << QString("String.prototype.replace") << QString("replace");
    QTest::newRow("String.prototype.search") << QString("String.prototype.search") << QString("search");
    QTest::newRow("String.prototype.slice") << QString("String.prototype.slice") << QString("slice");
    QTest::newRow("String.prototype.split") << QString("String.prototype.split") << QString("split");
    QTest::newRow("String.prototype.startsWith") << QString("String.prototype.startsWith") << QString("startsWith");
    QTest::newRow("String.prototype.substring") << QString("String.prototype.substring") << QString("substring");
    QTest::newRow("String.prototype.toLowerCase") << QString("String.prototype.toLowerCase") << QString("toLowerCase");
    QTest::newRow("String.prototype.toLocaleLowerCase") << QString("String.prototype.toLocaleLowerCase") << QString("toLocaleLowerCase");
    QTest::newRow("String.prototype.toUpperCase") << QString("String.prototype.toUpperCase") << QString("toUpperCase");
    QTest::newRow("String.prototype.toLocaleUpperCase") << QString("String.prototype.toLocaleUpperCase") << QString("toLocaleUpperCase");
}

void tst_QJSEngine::builtinFunctionNames()
{
    QFETCH(QString, expression);
    QFETCH(QString, expectedName);
    QJSEngine eng;
    // The "name" property is actually non-standard, but JSC supports it.
    QJSValue ret = eng.evaluate(QString::fromLatin1("%0.name").arg(expression));
    QVERIFY(ret.isString());
    QCOMPARE(ret.toString(), expectedName);
}

void tst_QJSEngine::evaluate_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("lineNumber");
    QTest::addColumn<bool>("expectHadError");
    QTest::addColumn<int>("expectErrorLineNumber");

    QTest::newRow("(newline)") << QString("\n") << -1 << false << -1;
    QTest::newRow("0 //")   << QString("0 //") << -1 << false << -1;
    QTest::newRow("/* */")   << QString("/* */") << -1 << false << -1;
    QTest::newRow("//") << QString("//") << -1 << false << -1;
    QTest::newRow("(spaces)")  << QString("  ") << -1 << false << -1;
    QTest::newRow("(empty)")   << QString("") << -1 << false << -1;
    QTest::newRow("0")     << QString("0")       << -1 << false << -1;
    QTest::newRow("0=1")   << QString("\n0=1;\n") << -1 << true  << 2;
    QTest::newRow("a=1")   << QString("a=1\n")   << -1 << false << -1;
    QTest::newRow("a=1;K") << QString("a=1;\nK") << -1 << true  << 2;

    QTest::newRow("f()") << QString("function f()\n"
                                    "{\n"
                                    "  var a;\n"
                                    "  var b=\";\n" // here's the error
                                    "}\n"
                                    "f();\n")
                         << -1 << true << 4;

    QTest::newRow("0")     << QString("0")       << 10 << false << -1;
    QTest::newRow("0=1")   << QString("\n\n0=1\n") << 10 << true  << 12;
    QTest::newRow("a=1")   << QString("a=1\n")   << 10 << false << -1;
    QTest::newRow("a=1;K") << QString("a=1;\n\nK") << 10 << true  << 12;

    QTest::newRow("f()") << QString("function f()\n"
                                    "{\n"
                                    "  var a;\n"
                                    "\n\n"
                                    "  var b=\";\n" // here's the error
                                    "}\n"
                                    "f();\n")
                         << 10 << true << 15;
    QTest::newRow("functionThatDoesntExist()")
        << QString(";\n;\n;\nfunctionThatDoesntExist()")
        << -1 << true << 4;
    QTest::newRow("for (var p in this) { continue labelThatDoesntExist; }")
        << QString("for (var p in this) {\ncontinue labelThatDoesntExist; }")
        << 4 << true << 5;
    QTest::newRow("duplicateLabel: { duplicateLabel: ; }")
        << QString("duplicateLabel: { duplicateLabel: ; }")
        << 12 << true << 12;

    QTest::newRow("/=/") << QString("/=/") << -1 << false << -1;
    QTest::newRow("/=/g") << QString("/=/g") << -1 << false << -1;
    QTest::newRow("/a/") << QString("/a/") << -1 << false << -1;
    QTest::newRow("/a/g") << QString("/a/g") << -1 << false << -1;
    QTest::newRow("/a/gim") << QString("/a/gim") << -1 << false << -1;
    QTest::newRow("/a/gimp") << QString("/a/gimp") << 1 << true << 1;
    QTest::newRow("empty-array-concat") << QString("var a = []; var b = [1]; var c = a.concat(b); ") << 1 << false << -1;
    QTest::newRow("object-literal") << QString("var a = {\"0\":\"#\",\"2\":\"#\",\"5\":\"#\",\"8\":\"#\",\"6\":\"#\",\"12\":\"#\",\"13\":\"#\",\"16\":\"#\",\"18\":\"#\",\"39\":\"#\",\"40\":\"#\"}") << 1 << false << -1;
}

void tst_QJSEngine::evaluate()
{
    QFETCH(QString, code);
    QFETCH(int, lineNumber);
    QFETCH(bool, expectHadError);
    QFETCH(int, expectErrorLineNumber);

    QJSEngine eng;
    QJSValue ret;
    if (lineNumber != -1)
        ret = eng.evaluate(code, /*fileName =*/QString(), lineNumber);
    else
        ret = eng.evaluate(code);
    QCOMPARE(ret.isError(), expectHadError);
    if (ret.isError()) {
        QVERIFY(ret.property("lineNumber").strictlyEquals(eng.toScriptValue(expectErrorLineNumber)));
    }
}

void tst_QJSEngine::errorMessage_QT679()
{
    QJSEngine engine;
    engine.globalObject().setProperty("foo", 15);
    QJSValue error = engine.evaluate("'hello world';\nfoo.bar.blah");
    QVERIFY(error.isError());
    QVERIFY(error.toString().startsWith(QString::fromLatin1("TypeError: ")));
}

struct Foo {
public:
    int x = -1, y = -1;
    Foo() {}
};

Q_DECLARE_METATYPE(Foo)
Q_DECLARE_METATYPE(Foo*)

Q_DECLARE_METATYPE(QList<Foo>)
Q_DECLARE_METATYPE(QVector<QChar>)
Q_DECLARE_METATYPE(QStack<int>)
Q_DECLARE_METATYPE(QQueue<char>)

void tst_QJSEngine::valueConversion_basic()
{
    QJSEngine eng;
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(num.isNumber(), true);
        QCOMPARE(num.strictlyEquals(eng.toScriptValue(123)), true);

        int inum = eng.fromScriptValue<int>(num);
        QCOMPARE(inum, 123);

        QString snum = eng.fromScriptValue<QString>(num);
        QCOMPARE(snum, QLatin1String("123"));
    }
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(num.isNumber(), true);
        QCOMPARE(num.strictlyEquals(eng.toScriptValue(123)), true);

        int inum = eng.fromScriptValue<int>(num);
        QCOMPARE(inum, 123);

        QString snum = eng.fromScriptValue<QString>(num);
        QCOMPARE(snum, QLatin1String("123"));
    }
    {
        QJSValue num = eng.toScriptValue(123);
        QCOMPARE(eng.fromScriptValue<char>(num), char(123));
        QCOMPARE(eng.fromScriptValue<unsigned char>(num), (unsigned char)(123));
        QCOMPARE(eng.fromScriptValue<short>(num), short(123));
        QCOMPARE(eng.fromScriptValue<unsigned short>(num), (unsigned short)(123));
        QCOMPARE(eng.fromScriptValue<float>(num), float(123));
        QCOMPARE(eng.fromScriptValue<double>(num), double(123));
        QCOMPARE(eng.fromScriptValue<long>(num), long(123));
        QCOMPARE(eng.fromScriptValue<ulong>(num), ulong(123));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), qlonglong(123));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), qulonglong(123));
    }
    {
        QJSValue num(123);
        QCOMPARE(eng.fromScriptValue<char>(num), char(123));
        QCOMPARE(eng.fromScriptValue<unsigned char>(num), (unsigned char)(123));
        QCOMPARE(eng.fromScriptValue<short>(num), short(123));
        QCOMPARE(eng.fromScriptValue<unsigned short>(num), (unsigned short)(123));
        QCOMPARE(eng.fromScriptValue<float>(num), float(123));
        QCOMPARE(eng.fromScriptValue<double>(num), double(123));
        QCOMPARE(eng.fromScriptValue<long>(num), long(123));
        QCOMPARE(eng.fromScriptValue<ulong>(num), ulong(123));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), qlonglong(123));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), qulonglong(123));
    }

    {
        QJSValue num = eng.toScriptValue(Q_INT64_C(0x100000000));
        QCOMPARE(eng.fromScriptValue<qlonglong>(num), Q_INT64_C(0x100000000));
        QCOMPARE(eng.fromScriptValue<qulonglong>(num), Q_UINT64_C(0x100000000));
    }

    {
        QChar c = QLatin1Char('c');
        QJSValue str = eng.toScriptValue(QString::fromLatin1("ciao"));
        QCOMPARE(eng.fromScriptValue<QChar>(str), c);
        QJSValue code = eng.toScriptValue(c.unicode());
        QCOMPARE(eng.fromScriptValue<QChar>(code), c);
        QCOMPARE(eng.fromScriptValue<QChar>(eng.toScriptValue(c)), c);
    }

    {
        QList<QObject *> list = {this};
        QQmlListProperty<QObject> prop(this, &list);
        QJSValue jsVal = eng.toScriptValue(prop);
        QCOMPARE(eng.fromScriptValue<QQmlListProperty<QObject>>(jsVal), prop);
    }

    QVERIFY(eng.toScriptValue(static_cast<void *>(nullptr)).isNull());
}

void tst_QJSEngine::valueConversion_QVariant()
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    QJSEngine eng;
    // qScriptValueFromValue() should be "smart" when the argument is a QVariant
    {
        QJSValue val = eng.toScriptValue(QVariant());
        QVERIFY(!val.isVariant());
        QVERIFY(val.isUndefined());
    }
    // Checking nested QVariants
    {
        // ### Qt 7: QVariant nesting is evil; we should check if we can get rid of it
        // main use case for it was QSignalSpy
        QVariant tmp1;
        QVariant tmp2(QMetaType::fromType<QVariant>(), &tmp1);
        QCOMPARE(QMetaType::Type(tmp2.userType()), QMetaType::QVariant);

        QJSValue val1 = eng.toScriptValue(tmp1);
        QJSValue val2 = eng.toScriptValue(tmp2);
        QVERIFY(val1.isUndefined());
        QVERIFY(!val2.isUndefined());
        QVERIFY(!val1.isVariant());
        QVERIFY(val2.isVariant());
    }
    {
        QVariant tmp1(123);
        QVariant tmp2(QMetaType::fromType<QVariant>(), &tmp1);
        QVariant tmp3(QMetaType::fromType<QVariant>(), &tmp2);
        QCOMPARE(QMetaType::Type(tmp1.userType()), QMetaType::Int);
        QCOMPARE(QMetaType::Type(tmp2.userType()), QMetaType::QVariant);
        QCOMPARE(QMetaType::Type(tmp3.userType()), QMetaType::QVariant);

        QJSValue val1 = eng.toScriptValue(tmp2);
        QJSValue val2 = eng.toScriptValue(tmp3);
        QVERIFY(!val1.isUndefined());
        QVERIFY(!val2.isUndefined());
        QVERIFY(val1.isVariant());
        QVERIFY(val2.isVariant());
        QCOMPARE(val1.toVariant(), tmp2);
        QCOMPARE(val2.toVariant(), tmp3);
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(true));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isBool());
        QCOMPARE(val.toBool(), true);
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(int(123)));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isNumber());
        QCOMPARE(val.toNumber(), qreal(123));
    }
    {
        QJSValue val = eng.toScriptValue(QVariant(qreal(1.25)));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isNumber());
        QCOMPARE(val.toNumber(), qreal(1.25));
    }
    {
        QString str = QString::fromLatin1("ciao");
        QJSValue val = eng.toScriptValue(QVariant(str));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isString());
        QCOMPARE(val.toString(), str);
    }
    {
        QJSValue val = eng.toScriptValue(QVariant::fromValue((QObject*)this));
        QVERIFY(!val.isVariant());
        QVERIFY(val.isQObject());
        QCOMPARE(val.toQObject(), (QObject*)this);
    }
    {
        QVariant var = QVariant::fromValue(QPoint(123, 456));
        QJSValue val = eng.toScriptValue(var);
        QVERIFY(!val.isVariant());
        QCOMPARE(val.toVariant(), var);
    }

    QCOMPARE(qjsvalue_cast<QVariant>(QJSValue(123)), QVariant(123));

    QVERIFY(eng.toScriptValue(QVariant(QMetaType::fromType<void *>(), nullptr)).isNull());
    QVERIFY(eng.toScriptValue(QVariant::fromValue(nullptr)).isNull());

    {
        QVariantMap map;
        map.insert("42", "the answer to life the universe and everything");
        QJSValue val = eng.toScriptValue(map);
        QVERIFY(val.isObject());
        QCOMPARE(val.property(42).toString(), map.value(QStringLiteral("42")).toString());
    }
    QT_WARNING_POP
}

void tst_QJSEngine::valueConversion_basic2()
{
    QJSEngine eng;
    // more built-in types
    {
        QJSValue val = eng.toScriptValue(uint(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(qulonglong(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(float(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(short(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(ushort(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(char(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
    {
        QJSValue val = eng.toScriptValue(uchar(123));
        QVERIFY(val.isNumber());
        QCOMPARE(val.toInt(), 123);
    }
}

void tst_QJSEngine::valueConversion_dateTime()
{
    QJSEngine eng;
    {
        QDateTime in = QDateTime::currentDateTime();
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isDate());
        QCOMPARE(val.toDateTime(), in);
    }
    {
        QDate in = QDate::currentDate();
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isDate());
        QCOMPARE(val.toDateTime().date(), in);
    }
}

void tst_QJSEngine::valueConversion_RegularExpression()
{
    QJSEngine eng;
    {
        QRegularExpression in = QRegularExpression("foo");
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isRegExp());
        QRegularExpression out = qjsvalue_cast<QRegularExpression>(val);
        QCOMPARE(out.pattern(), in.pattern());
        QCOMPARE(out.patternOptions(), in.patternOptions());
    }
    {
        QRegularExpression in = QRegularExpression("foo",
                                                   QRegularExpression::CaseInsensitiveOption);
        QJSValue val = eng.toScriptValue(in);
        QVERIFY(val.isRegExp());
        QCOMPARE(qjsvalue_cast<QRegularExpression>(val), in);
        QRegularExpression out = qjsvalue_cast<QRegularExpression>(val);
        QCOMPARE(out.patternOptions(), in.patternOptions());
    }
}

class Klazz : public QObject,
              public QStandardItem,
              public QQmlParserStatus
{
    Q_INTERFACES(QQmlParserStatus)
    Q_OBJECT
public:
    Klazz(QObject *parent = nullptr) : QObject(parent) { }
    void classBegin() override {}
    void componentComplete() override {}
};

Q_DECLARE_METATYPE(Klazz*)
Q_DECLARE_METATYPE(QStandardItem*)

void tst_QJSEngine::castWithMultipleInheritance()
{
    QJSEngine eng;
    Klazz klz;
    QJSValue v = eng.newQObject(&klz);

    QCOMPARE(qjsvalue_cast<Klazz*>(v), &klz);
    QCOMPARE(qjsvalue_cast<QQmlParserStatus*>(v), (QQmlParserStatus *)&klz);
    QCOMPARE(qjsvalue_cast<QObject*>(v), (QObject *)&klz);
    QCOMPARE(qjsvalue_cast<QStandardItem*>(v), (QStandardItem *)&klz);
}


void tst_QJSEngine::collectGarbage()
{
    QJSEngine eng;
    eng.evaluate("a = new Object(); a = new Object(); a = new Object()");
    QJSValue a = eng.newObject();
    a = eng.newObject();
    a = eng.newObject();
    QPointer<QObject> ptr = new QObject();
    QVERIFY(ptr != nullptr);
    (void)eng.newQObject(ptr);
    gc(*eng.handle(), GCFlags::DontSendPostedEvents);
    if (ptr)
        QGuiApplication::sendPostedEvents(ptr, QEvent::DeferredDelete);
    QVERIFY(ptr.isNull());
}

class TestObjectContainer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *dummy MEMBER m_dummy CONSTANT)

public:
    TestObjectContainer() : m_dummy(new QObject(this)) {}

private:
    QObject *m_dummy;
};

void tst_QJSEngine::collectGarbageNestedWrappersTwoEngines()
{
    QJSEngine engine1;
    QJSEngine engine2;

    TestObjectContainer container;
    QQmlEngine::setObjectOwnership(&container, QQmlEngine::CppOwnership);

    engine1.globalObject().setProperty("foobar", engine1.newQObject(&container));
    engine2.globalObject().setProperty("foobar", engine2.newQObject(&container));

    engine1.evaluate("foobar.dummy.baz = 42");
    engine2.evaluate("foobar.dummy.baz = 43");

    QCOMPARE(engine1.evaluate("foobar.dummy.baz").toInt(), 42);
    QCOMPARE(engine2.evaluate("foobar.dummy.baz").toInt(), 43);

    gc(*engine1.handle());
    gc(*engine2.handle());

    // The GC should not collect dummy object wrappers neither in engine1 nor engine2, we
    // verify that by checking whether the baz property still has its previous value.
    QCOMPARE(engine1.evaluate("foobar.dummy.baz").toInt(), 42);
    QCOMPARE(engine2.evaluate("foobar.dummy.baz").toInt(), 43);
}

void tst_QJSEngine::gcWithNestedDataStructure()
{
    // The GC must be able to traverse deeply nested objects, otherwise this
    // test would crash.
    QJSEngine eng;
    eng.installExtensions(QJSEngine::GarbageCollectionExtension);

    QJSValue ret = eng.evaluate(
        "function makeList(size)"
        "{"
        "  var head = { };"
        "  var l = head;"
        "  for (var i = 0; i < size; ++i) {"
        "    l.data = i + \"\";"
        "    l.next = { }; l = l.next;"
        "  }"
        "  l.next = null;"
        "  return head;"
        "}");
    QVERIFY(!ret.isError());
    const int size = 200;
    QJSValue head = eng.evaluate(QString::fromLatin1("makeList(%0)").arg(size));
    QVERIFY(!head.isError());
    for (int x = 0; x < 2; ++x) {
        if (x == 1)
            eng.evaluate("gc()");
        QJSValue l = head;
        // Make sure all the nodes are still alive.
        for (int i = 0; i < 200; ++i) {
            QCOMPARE(l.property("data").toString(), QString::number(i));
            l = l.property("next");
        }
    }
}

void tst_QJSEngine::stacktrace()
{
    QString script = QString::fromLatin1(
        "function foo(counter) {\n"
        "    switch (counter) {\n"
        "        case 0: foo(counter+1); break;\n"
        "        case 1: foo(counter+1); break;\n"
        "        case 2: foo(counter+1); break;\n"
        "        case 3: foo(counter+1); break;\n"
        "        case 4: foo(counter+1); break;\n"
        "        default:\n"
        "        throw new Error('blah');\n"
        "    }\n"
        "}\n"
        "foo(0);");

    const QString fileName("testfile");

    QStringList backtrace;
    backtrace << "foo(5)@testfile:9"
              << "foo(4)@testfile:7"
              << "foo(3)@testfile:6"
              << "foo(2)@testfile:5"
              << "foo(1)@testfile:4"
              << "foo(0)@testfile:3"
              << "<global>()@testfile:12";

    QJSEngine eng;
    QJSValue result = eng.evaluate(script, fileName);
    QVERIFY(result.isError());

    QJSValue stack = result.property("stack");

    QJSValueIterator it(stack);
    int counter = 5;
    while (it.hasNext()) {
        it.next();
        QJSValue obj = it.value();
        QJSValue frame = obj.property("frame");

        QCOMPARE(obj.property("fileName").toString(), fileName);
        if (counter >= 0) {
            QJSValue callee = frame.property("arguments").property("callee");
            QVERIFY(callee.strictlyEquals(eng.globalObject().property("foo")));
            QCOMPARE(obj.property("functionName").toString(), QString("foo"));
            int line = obj.property("lineNumber").toInt();
            if (counter == 5)
                QCOMPARE(line, 9);
            else
                QCOMPARE(line, 3 + counter);
        } else {
            QVERIFY(frame.strictlyEquals(eng.globalObject()));
            QVERIFY(obj.property("functionName").toString().isEmpty());
        }

        --counter;
    }

    // throw something that isn't an Error object
    // ###FIXME: No uncaughtExceptionBacktrace: QVERIFY(eng.uncaughtExceptionBacktrace().isEmpty());
    QString script2 = QString::fromLatin1(
        "function foo(counter) {\n"
        "    switch (counter) {\n"
        "        case 0: foo(counter+1); break;\n"
        "        case 1: foo(counter+1); break;\n"
        "        case 2: foo(counter+1); break;\n"
        "        case 3: foo(counter+1); break;\n"
        "        case 4: foo(counter+1); break;\n"
        "        default:\n"
        "        throw 'just a string';\n"
        "    }\n"
        "}\n"
        "foo(0);");

    QJSValue result2 = eng.evaluate(script2, fileName);
    QVERIFY(!result2.isError());
    QVERIFY(result2.isString());

    {
        QString script3 = QString::fromLatin1(
                    "'use strict'\n"
                    "function throwUp() { throw new Error('up') }\n"
                    "function indirectlyThrow() { return throwUp() }\n"
                    "indirectlyThrow()\n"
                    );
        QJSValue result3 = eng.evaluate(script3);
        QVERIFY(result3.isError());
        QJSValue stack = result3.property("stack");
        QVERIFY(stack.isString());
        QString stackTrace = stack.toString();
        QVERIFY(!stackTrace.contains(QStringLiteral("indirectlyThrow")));
        QVERIFY(stackTrace.contains(QStringLiteral("elide")));
    }
}

void tst_QJSEngine::unshiftAndSort()
{
    QJSEngine engine;
    QJSValue func = engine.evaluate(R"""(
    (function (objectArr, currIdx) {
        objectArr.unshift({"sortIndex": currIdx});
        objectArr.sort(function(a, b) {
            if (a.sortIndex > b.sortIndex)
                return 1;
            if (a.sortIndex < b.sortIndex)
                return -1;
            return 0;
        });
        return objectArr;
    })
    )""");
    QVERIFY(func.isCallable());
    QJSValue objectArr = engine.newArray();

    for (int i = 0; i < 5; ++i) {
        objectArr = func.call({objectArr, i});
        QVERIFY2(!objectArr.isError(), qPrintable(objectArr.toString()));
        const int length = objectArr.property("length").toInt();

        // It did add one element
        QCOMPARE(length, i + 1);

        for (int x = 0; x < length; ++x) {
            // We didn't sort cruft into the array.
            QVERIFY(!objectArr.property(x).isUndefined());

            // The array is actually sorted.
            QCOMPARE(objectArr.property(x).property("sortIndex").toInt(), x);
        }
    }
}

void tst_QJSEngine::unshiftAndPushAndSort()
{
    QJSEngine engine;
    QJSValue func = engine.evaluate(R"""(
    (function (objectArr, currIdx) {
        objectArr.unshift({"sortIndex": currIdx});
        objectArr.push({"sortIndex": currIdx + 1});
        objectArr.sort(function(a, b) {
            if (a.sortIndex > b.sortIndex)
                return 1;
            if (a.sortIndex < b.sortIndex)
                return -1;
            return 0;
        });
        return objectArr;
    })
    )""");
    QVERIFY(func.isCallable());
    QJSValue objectArr = engine.newArray();

    for (int i = 0; i < 20; i += 2) {
        objectArr = func.call({objectArr, i});
        QVERIFY2(!objectArr.isError(), qPrintable(objectArr.toString()));
        const int length = objectArr.property("length").toInt();

        // It did add 2 elements
        QCOMPARE(length, i + 2);

        for (int x = 0; x < length; ++x) {
            // We didn't sort cruft into the array.
            QVERIFY(!objectArr.property(x).isUndefined());

            // The array is actually sorted.
            QCOMPARE(objectArr.property(x).property("sortIndex").toInt(), x);
        }
    }
}

void tst_QJSEngine::numberParsing_data()
{
    QTest::addColumn<QString>("string");
    QTest::addColumn<qreal>("expect");

    QTest::newRow("decimal 0") << QString("0") << qreal(0);
//    QTest::newRow("octal 0") << QString("00") << qreal(00);
    QTest::newRow("hex 0") << QString("0x0") << qreal(0x0);
    QTest::newRow("decimal 100") << QString("100") << qreal(100);
    QTest::newRow("hex 100") << QString("0x100") << qreal(0x100);
//    QTest::newRow("octal 100") << QString("0100") << qreal(0100);
    QTest::newRow("decimal 4G") << QString("4294967296") << qreal(Q_UINT64_C(4294967296));
    QTest::newRow("hex 4G") << QString("0x100000000") << qreal(Q_UINT64_C(0x100000000));
//    QTest::newRow("octal 4G") << QString("040000000000") << qreal(Q_UINT64_C(040000000000));
    QTest::newRow("0.5") << QString("0.5") << qreal(0.5);
    QTest::newRow("1.5") << QString("1.5") << qreal(1.5);
    QTest::newRow("1e2") << QString("1e2") << qreal(100);
}

void tst_QJSEngine::numberParsing()
{
    QFETCH(QString, string);
    QFETCH(qreal, expect);

    QJSEngine eng;
    QJSValue ret = eng.evaluate(string);
    QVERIFY(ret.isNumber());
    qreal actual = ret.toNumber();
    QCOMPARE(actual, expect);
}

// see ECMA-262, section 7.9
// This is testing ECMA compliance, not our C++ API, but it's important that
// the back-end is conformant in this regard.
void tst_QJSEngine::automaticSemicolonInsertion()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("{ 1 2 } 3");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("{ 1\n2 } 3");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("for (a; b\n)");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("(function() { return\n1 + 2 })()");
        QVERIFY(ret.isUndefined());
    }
    {
        eng.evaluate("c = 2; b = 1");
        QJSValue ret = eng.evaluate("a = b\n++c");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("if (a > b)\nelse c = d");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        eng.evaluate("function c() { return { foo: function() { return 5; } } }");
        eng.evaluate("b = 1; d = 2; e = 3");
        QJSValue ret = eng.evaluate("a = b + c\n(d + e).foo()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("throw\n1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains("SyntaxError"));
    }
    {
        QJSValue ret = eng.evaluate("a = Number(1)\n++a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 2);
    }

    // "a semicolon is never inserted automatically if the semicolon
    // would then be parsed as an empty statement"
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0)\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0)\n --a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if ((0))\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if ((0))\n --a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0\n)\n ++a; a");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0\n ++a; a");
        QVERIFY(ret.isError());
    }
    {
        eng.evaluate("a = 123");
        QJSValue ret = eng.evaluate("if (0))\n ++a; a");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("n = 0; for (i = 0; i < 10; ++i)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; for (i = 0; i < 10; ++i)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("n = 0; for (var i = 0; i < 10; ++i)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; for (var i = 0; i < 10; ++i)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("n = 0; i = 0; while (i++ < 10)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 30; i = 0; while (i++ < 10)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 20);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 0; for (i in o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 9; for (i in o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 0; for (var i in o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    {
        QJSValue ret = eng.evaluate("o = { a: 0, b: 1, c: 2 }; n = 9; for (var i in o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 6);
    }
    {
        QJSValue ret = eng.evaluate("o = { n: 3 }; n = 5; with (o)\n ++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 5);
    }
    {
        QJSValue ret = eng.evaluate("o = { n: 3 }; n = 10; with (o)\n --n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    {
        QJSValue ret = eng.evaluate("n = 5; i = 0; do\n ++n; while (++i < 10); n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 15);
    }
    {
        QJSValue ret = eng.evaluate("n = 20; i = 0; do\n --n; while (++i < 10); n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }

    {
        QJSValue ret = eng.evaluate("n = 1; i = 0; if (n) i\n++n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 2);
    }
    {
        QJSValue ret = eng.evaluate("n = 1; i = 0; if (n) i\n--n; n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 0);
    }

    {
        QJSValue ret = eng.evaluate("if (0)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("n = 0; if (1) --n;else\n ++n;\n n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), -1);
    }
    {
        QJSValue ret = eng.evaluate("while (0)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("for (;;)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("for (p in this)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("with (this)");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("do");
        QVERIFY(ret.isError());
    }
}

void tst_QJSEngine::errorConstructors()
{
    QJSEngine eng;
    QStringList prefixes;
    prefixes << "" << "Eval" << "Range" << "Reference" << "Syntax" << "Type" << "URI";
    for (int x = 0; x < 3; ++x) {
        for (int i = 0; i < prefixes.size(); ++i) {
            QString name = prefixes.at(i) + QLatin1String("Error");
            QString code = QString(i+1, QLatin1Char('\n'));
            if (x == 0)
                code += QLatin1String("throw ");
            else if (x == 1)
                code += QLatin1String("new ");
            code += name + QLatin1String("()");
            QJSValue ret = eng.evaluate(code);
            QVERIFY(ret.isError());
            QVERIFY(ret.toString().startsWith(name));
            qDebug() << ret.property("stack").toString();
            QCOMPARE(ret.property("lineNumber").toInt(), i+2);
        }
    }
}

void tst_QJSEngine::argumentsProperty_globalContext()
{
    QJSEngine eng;
    {
        // Unlike function calls, the global context doesn't have an
        // arguments property.
        QJSValue ret = eng.evaluate("arguments");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    eng.evaluate("arguments = 10");
    {
        QJSValue ret = eng.evaluate("arguments");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 10);
    }
    QVERIFY(eng.evaluate("delete arguments").toBool());
    QVERIFY(eng.globalObject().property("arguments").isUndefined());
}

void tst_QJSEngine::argumentsProperty_JS()
{
    {
        QJSEngine eng;
        eng.evaluate("o = { arguments: 123 }");
        QJSValue ret = eng.evaluate("with (o) { arguments; }");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        QJSEngine eng;
        QVERIFY(eng.globalObject().property("arguments").isUndefined());
        // This is testing ECMA-262 compliance. In function calls, "arguments"
        // appears like a local variable, and it can be replaced.
        QJSValue ret = eng.evaluate("(function() { arguments = 456; return arguments; })()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 456);
        QVERIFY(eng.globalObject().property("arguments").isUndefined());
    }
}

void tst_QJSEngine::jsNumberClass()
{
    // See ECMA-262 Section 15.7, "Number Objects".

    QJSEngine eng;

    QJSValue ctor = eng.globalObject().property("Number");
    QVERIFY(ctor.property("length").isNumber());
    QCOMPARE(ctor.property("length").toNumber(), qreal(1));
    QJSValue proto = ctor.property("prototype");
    QVERIFY(proto.isObject());
    {
        QVERIFY(ctor.property("MAX_VALUE").isNumber());
        QVERIFY(ctor.property("MIN_VALUE").isNumber());
        QVERIFY(ctor.property("NaN").isNumber());
        QVERIFY(ctor.property("NEGATIVE_INFINITY").isNumber());
        QVERIFY(ctor.property("POSITIVE_INFINITY").isNumber());
        QVERIFY(ctor.property("EPSILON").isNumber());
    }
    QCOMPARE(proto.toNumber(), qreal(0));
    QVERIFY(proto.property("constructor").strictlyEquals(ctor));

    {
        QJSValue ret = eng.evaluate("Number()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(0));
    }
    {
        QJSValue ret = eng.evaluate("Number(123)");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    {
        QJSValue ret = eng.evaluate("Number('456')");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(456));
    }
    {
        QJSValue ret = eng.evaluate("new Number()");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(0));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123)");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    {
        QJSValue ret = eng.evaluate("new Number('456')");
        QVERIFY(!ret.isNumber());
        QVERIFY(ret.isObject());
        QCOMPARE(ret.toNumber(), qreal(456));
    }

    QVERIFY(ctor.property("isFinite").isCallable());
    {
        QJSValue ret = eng.evaluate("Number.isFinite()");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }
    {
        QJSValue ret = eng.evaluate("Number.isFinite(NaN)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }
    {
        QJSValue ret = eng.evaluate("Number.isFinite(Infinity)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }
    {
        QJSValue ret = eng.evaluate("Number.isFinite(-Infinity)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }
    {
        QJSValue ret = eng.evaluate("Number.isFinite(123)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), true);
    }

    QVERIFY(ctor.property("isNaN").isCallable());
    {
        QJSValue ret = eng.evaluate("Number.isNaN()");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }
    {
        QJSValue ret = eng.evaluate("Number.isNaN(NaN)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), true);
    }
    {
        QJSValue ret = eng.evaluate("Number.isNaN(123)");
        QVERIFY(ret.isBool());
        QCOMPARE(ret.toBool(), false);
    }

    QVERIFY(proto.property("toString").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toString()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123).toString(8)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("173"));
    }
    {
        QJSValue ret = eng.evaluate("new Number(123).toString(16)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("7b"));
    }
    QVERIFY(proto.property("toLocaleString").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toLocaleString()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
    }
    QVERIFY(proto.property("valueOf").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).valueOf()");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toNumber(), qreal(123));
    }
    QVERIFY(proto.property("toExponential").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toExponential()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("1.23e+2"));
        ret = eng.evaluate("new Number(123).toExponential(1)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("1.2e+2"));
        ret = eng.evaluate("new Number(123).toExponential(2)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("1.23e+2"));
        ret = eng.evaluate("new Number(123).toExponential(3)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("1.230e+2"));
    }
    QVERIFY(proto.property("toFixed").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toFixed()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
        ret = eng.evaluate("new Number(123).toFixed(1)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123.0"));
        ret = eng.evaluate("new Number(123).toFixed(2)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123.00"));
    }
    QVERIFY(proto.property("toPrecision").isCallable());
    {
        QJSValue ret = eng.evaluate("new Number(123).toPrecision()");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("123"));
        ret = eng.evaluate("new Number(42).toPrecision(1)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("4e+1"));
        ret = eng.evaluate("new Number(42).toPrecision(2)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("42"));
        ret = eng.evaluate("new Number(42).toPrecision(3)");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("42.0"));
    }
}

void tst_QJSEngine::jsForInStatement_simple()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("o = { }; r = []; for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QVERIFY(lst.isEmpty());
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123, q: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
        QCOMPARE(lst.at(1), QString::fromLatin1("q"));
    }
}

void tst_QJSEngine::jsForInStatement_prototypeProperties()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("o = { }; o.__proto__ = { p: 123 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; o.__proto__ = { q: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
        QCOMPARE(lst.at(1), QString::fromLatin1("q"));
    }
    {
        // shadowed property
        QJSValue ret = eng.evaluate("o = { p: 123 }; o.__proto__ = { p: 456 }; r = [];"
                                        "for (var p in o) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

}

void tst_QJSEngine::jsForInStatement_mutateWhileIterating()
{
    QJSEngine eng;
    // deleting property during enumeration
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; delete r[p]; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }
    {
        QJSValue ret = eng.evaluate("o = { p: 123, q: 456 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; delete o.q; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 1);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

    // adding property during enumeration
    {
        QJSValue ret = eng.evaluate("o = { p: 123 }; r = [];"
                                        "for (var p in o) { r[r.length] = p; o.q = 456; } r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("p"));
    }

}

void tst_QJSEngine::jsForInStatement_arrays()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 2);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
    }
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; a.foo = 'bar'; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 3);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
        QCOMPARE(lst.at(2), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("a = [123, 456]; a.foo = 'bar';"
                                        "b = [111, 222, 333]; b.bar = 'baz';"
                                        "a.__proto__ = b; r = [];"
                                        "for (var p in a) r[r.length] = p; r");
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), 5);
        QCOMPARE(lst.at(0), QString::fromLatin1("0"));
        QCOMPARE(lst.at(1), QString::fromLatin1("1"));
        QCOMPARE(lst.at(2), QString::fromLatin1("foo"));
        QCOMPARE(lst.at(3), QString::fromLatin1("2"));
        QCOMPARE(lst.at(4), QString::fromLatin1("bar"));
    }
}

void tst_QJSEngine::jsForInStatement_constant()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("r = true; for (var p in undefined) r = false; r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
    {
        QJSValue ret = eng.evaluate("r = true; for (var p in null) r = false; r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
    {
        QJSValue ret = eng.evaluate("r = false; for (var p in 1) r = true; r");
        QVERIFY(ret.isBool());
        QVERIFY(!ret.toBool());
    }
    {
        QJSValue ret = eng.evaluate("r = false; for (var p in 'abc') r = true; r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
}

void tst_QJSEngine::with_constant()
{
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("r = false; with(null) { r= true; } r");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("r = false; with(undefined) { r= true; } r");
        QVERIFY(ret.isError());
    }
    {
        QJSValue ret = eng.evaluate("r = false; with(1) { r= true; } r");
        QVERIFY(ret.isBool());
        QVERIFY(ret.toBool());
    }
}

void tst_QJSEngine::stringObjects()
{
    // See ECMA-262 Section 15.5, "String Objects".

    QJSEngine eng;
    QString str("ciao");
    // in C++
    {
        QJSValue obj = eng.evaluate(QString::fromLatin1("new String('%0')").arg(str));
        QCOMPARE(obj.property("length").toInt(), str.size());
        for (int i = 0; i < str.size(); ++i) {
            QString pname = QString::number(i);
            QVERIFY(obj.property(pname).isString());
            QCOMPARE(obj.property(pname).toString(), QString(str.at(i)));
            QVERIFY(!obj.deleteProperty(pname));
            obj.setProperty(pname, 123);
            QVERIFY(obj.property(pname).isString());
            QCOMPARE(obj.property(pname).toString(), QString(str.at(i)));
        }
        QVERIFY(obj.property("-1").isUndefined());
        QVERIFY(obj.property(QString::number(str.size())).isUndefined());

        QJSValue val = eng.toScriptValue(123);
        obj.setProperty("-1", val);
        QVERIFY(obj.property("-1").strictlyEquals(val));
        obj.setProperty("100", val);
        QVERIFY(obj.property("100").strictlyEquals(val));
    }

    {
        QJSValue ret = eng.evaluate("s = new String('ciao'); r = []; for (var p in s) r.push(p); r");
        QVERIFY(ret.isArray());
        QStringList lst = qjsvalue_cast<QStringList>(ret);
        QCOMPARE(lst.size(), str.size());
        for (int i = 0; i < str.size(); ++i)
            QCOMPARE(lst.at(i), QString::number(i));

        QJSValue ret2 = eng.evaluate("s[0] = 123; s[0]");
        QVERIFY(ret2.isString());
        QCOMPARE(ret2.toString().size(), 1);
        QCOMPARE(ret2.toString().at(0), str.at(0));

        QJSValue ret3 = eng.evaluate("s[-1] = 123; s[-1]");
        QVERIFY(ret3.isNumber());
        QCOMPARE(ret3.toInt(), 123);

        QJSValue ret4 = eng.evaluate("s[s.length] = 456; s[s.length]");
        QVERIFY(ret4.isNumber());
        QCOMPARE(ret4.toInt(), 456);

        QJSValue ret5 = eng.evaluate("delete s[0]");
        QVERIFY(ret5.isBool());
        QVERIFY(!ret5.toBool());

        QJSValue ret6 = eng.evaluate("delete s[-1]");
        QVERIFY(ret6.isBool());
        QVERIFY(ret6.toBool());

        QJSValue ret7 = eng.evaluate("delete s[s.length]");
        QVERIFY(ret7.isBool());
        QVERIFY(ret7.toBool());
    }
}

void tst_QJSEngine::jsStringPrototypeReplaceBugs()
{
    QJSEngine eng;
    // task 212440
    {
        QJSValue ret = eng.evaluate("replace_args = []; \"a a a\".replace(/(a)/g, function() { replace_args.push(arguments); }); replace_args");
        QVERIFY(ret.isArray());
        int len = ret.property("length").toInt();
        QCOMPARE(len, 3);
        for (int i = 0; i < len; ++i) {
            QJSValue args = ret.property(i);
            QCOMPARE(args.property("length").toInt(), 4);
            QCOMPARE(args.property(0).toString(), QString::fromLatin1("a")); // matched string
            QCOMPARE(args.property(1).toString(), QString::fromLatin1("a")); // capture
            QVERIFY(args.property(2).isNumber());
            QCOMPARE(args.property(2).toInt(), i*2); // index of match
            QCOMPARE(args.property(3).toString(), QString::fromLatin1("a a a"));
        }
    }
    // task 212501
    {
        QJSValue ret = eng.evaluate("\"foo\".replace(/a/g, function() {})");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
}

void tst_QJSEngine::getterSetterThisObject_global()
{
    {
        QJSEngine eng;
        // read
        eng.evaluate("__defineGetter__('x', function() { return this; });");
        {
            QJSValue ret = eng.evaluate("x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("(function() { return x; })()");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("with (this) x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("with ({}) x");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        {
            QJSValue ret = eng.evaluate("(function() { with ({}) return x; })()");
            QVERIFY(ret.equals(eng.globalObject()));
        }
        // write
        eng.evaluate("__defineSetter__('x', function() { return this; });");
        {
            QJSValue ret = eng.evaluate("x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("(function() { return x = 'foo'; })()");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("with (this) x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("with ({}) x = 'foo'");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
        {
            QJSValue ret = eng.evaluate("(function() { with ({}) return x = 'foo'; })()");
            // SpiderMonkey says setter return value, JSC says RHS.
            QVERIFY(ret.isString());
            QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
        }
    }
}

void tst_QJSEngine::getterSetterThisObject_plain()
{
    {
        QJSEngine eng;
        eng.evaluate("o = {}");
        // read
        eng.evaluate("o.__defineGetter__('x', function() { return this; })");
        QVERIFY(eng.evaluate("o.x === o").toBool());
        QVERIFY(eng.evaluate("with (o) x").equals(eng.evaluate("o")));
        QVERIFY(eng.evaluate("(function() { with (o) return x; })() === o").toBool());
        eng.evaluate("q = {}; with (o) with (q) x").equals(eng.evaluate("o"));
        // write
        eng.evaluate("o.__defineSetter__('x', function() { return this; });");
        // SpiderMonkey says setter return value, JSC says RHS.
        QVERIFY(eng.evaluate("(o.x = 'foo') === 'foo'").toBool());
        QVERIFY(eng.evaluate("with (o) x = 'foo'").equals("foo"));
        QVERIFY(eng.evaluate("with (o) with (q) x = 'foo'").equals("foo"));
    }
}

void tst_QJSEngine::getterSetterThisObject_prototypeChain()
{
    {
        QJSEngine eng;
        eng.evaluate("o = {}; p = {}; o.__proto__ = p");
        // read
        eng.evaluate("p.__defineGetter__('x', function() { return this; })");
        QVERIFY(eng.evaluate("o.x === o").toBool());
        QVERIFY(eng.evaluate("with (o) x").equals(eng.evaluate("o")));
        QVERIFY(eng.evaluate("(function() { with (o) return x; })() === o").toBool());
        eng.evaluate("q = {}; with (o) with (q) x").equals(eng.evaluate("o"));
        eng.evaluate("with (q) with (o) x").equals(eng.evaluate("o"));
        // write
        eng.evaluate("o.__defineSetter__('x', function() { return this; });");
        // SpiderMonkey says setter return value, JSC says RHS.
        QVERIFY(eng.evaluate("(o.x = 'foo') === 'foo'").toBool());
        QVERIFY(eng.evaluate("with (o) x = 'foo'").equals("foo"));
        QVERIFY(eng.evaluate("with (o) with (q) x = 'foo'").equals("foo"));
    }
}

void tst_QJSEngine::jsContinueInSwitch()
{
    // This is testing ECMA-262 compliance, not C++ API.

    QJSEngine eng;
    // switch - continue
    {
        QJSValue ret = eng.evaluate("switch (1) { default: continue; }");
        QVERIFY(ret.isError());
    }
    // for - switch - case - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1: ++j; continue;\n"
                                        "    case 100: ++j; continue;\n"
                                        "    case 1000: ++j; continue;\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    // for - switch - case - default - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1: ++j; continue;\n"
                                        "    case 100: ++j; continue;\n"
                                        "    case 1000: ++j; continue;\n"
                                        "    default: if (i < 50000) break; else continue;\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
    // switch - for - continue
    {
        QJSValue ret = eng.evaluate("j = 123; switch (j) {\n"
                                        "  case 123:\n"
                                        "  for (i = 0; i < 100000; ++i) {\n"
                                        "    continue;\n"
                                        "  }\n"
                                        "}; i\n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 100000);
    }
    // switch - switch - continue
    {
        QJSValue ret = eng.evaluate("i = 1; switch (i) { default: switch (i) { case 1: continue; } }");
        QVERIFY(ret.isError());
    }
    // for - switch - switch - continue
    {
        QJSValue ret = eng.evaluate("j = 0; for (i = 0; i < 100000; ++i) {\n"
                                        "  switch (i) {\n"
                                        "    case 1:\n"
                                        "    switch (i) {\n"
                                        "      case 1: ++j; continue;\n"
                                        "    }\n"
                                        "  }\n"
                                        "}; j");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 1);
    }
    // switch - for - switch - continue
    {
        QJSValue ret = eng.evaluate("j = 123; switch (j) {\n"
                                        "  case 123:\n"
                                        "  for (i = 0; i < 100000; ++i) {\n"
                                        "    switch (i) {\n"
                                        "      case 1:\n"
                                        "      ++j; continue;\n"
                                        "    }\n"
                                        "  }\n"
                                        "}; i\n");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 100000);
    }
}

void tst_QJSEngine::jsShadowReadOnlyPrototypeProperty()
{
    QJSEngine eng;
    QVERIFY(eng.evaluate("o = {}; o.__proto__ = parseInt; o.length").isNumber());
    QVERIFY(eng.evaluate("o.length = 123; o.length").toInt() != 123);
    QVERIFY(!eng.evaluate("o.hasOwnProperty('length')").toBool());
}

void tst_QJSEngine::jsReservedWords_data()
{
    QTest::addColumn<QString>("word");
    QTest::newRow("break") << QString("break");
    QTest::newRow("case") << QString("case");
    QTest::newRow("catch") << QString("catch");
    QTest::newRow("continue") << QString("continue");
    QTest::newRow("default") << QString("default");
    QTest::newRow("delete") << QString("delete");
    QTest::newRow("do") << QString("do");
    QTest::newRow("else") << QString("else");
    QTest::newRow("false") << QString("false");
    QTest::newRow("finally") << QString("finally");
    QTest::newRow("for") << QString("for");
    QTest::newRow("function") << QString("function");
    QTest::newRow("if") << QString("if");
    QTest::newRow("in") << QString("in");
    QTest::newRow("instanceof") << QString("instanceof");
    QTest::newRow("new") << QString("new");
    QTest::newRow("null") << QString("null");
    QTest::newRow("return") << QString("return");
    QTest::newRow("switch") << QString("switch");
    QTest::newRow("this") << QString("this");
    QTest::newRow("throw") << QString("throw");
    QTest::newRow("true") << QString("true");
    QTest::newRow("try") << QString("try");
    QTest::newRow("typeof") << QString("typeof");
    QTest::newRow("var") << QString("var");
    QTest::newRow("void") << QString("void");
    QTest::newRow("while") << QString("while");
    QTest::newRow("with") << QString("with");
}

void tst_QJSEngine::jsReservedWords()
{
    // See ECMA-262 Section 7.6.1, "Reserved Words".
    // We prefer that the implementation is less strict than the spec; e.g.
    // it's good to allow reserved words as identifiers in object literals,
    // and when accessing properties using dot notation.

    QFETCH(QString, word);
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate(word + " = 123");
        QVERIFY(ret.isError());
        QString str = ret.toString();
        QVERIFY(str.startsWith("SyntaxError") || str.startsWith("ReferenceError"));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("var " + word + " = 123");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().startsWith("SyntaxError"));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = {}; o." + word + " = 123");
        // in the old back-end, in SpiderMonkey and in v8, this is allowed, but not in JSC
        QVERIFY(!ret.isError());
        QVERIFY(ret.strictlyEquals(eng.evaluate("o." + word)));
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = { " + word + ": 123 }");
        // in the old back-end, in SpiderMonkey and in v8, this is allowed, but not in JSC
        QVERIFY(!ret.isError());
        QVERIFY(ret.property(word).isNumber());
    }
    {
        // SpiderMonkey allows this, but we don't
        QJSEngine eng;
        QJSValue ret = eng.evaluate("function " + word + "() {}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().startsWith("SyntaxError"));
    }
}

void tst_QJSEngine::jsFutureReservedWords_data()
{
    QTest::addColumn<QString>("word");
    QTest::addColumn<bool>("allowed");
    QTest::newRow("abstract") << QString("abstract") << true;
    QTest::newRow("boolean") << QString("boolean") << true;
    QTest::newRow("byte") << QString("byte") << true;
    QTest::newRow("char") << QString("char") << true;
    QTest::newRow("class") << QString("class") << false;
    QTest::newRow("const") << QString("const") << false;
    QTest::newRow("debugger") << QString("debugger") << false;
    QTest::newRow("double") << QString("double") << true;
    QTest::newRow("enum") << QString("enum") << false;
    QTest::newRow("export") << QString("export") << false;
    QTest::newRow("extends") << QString("extends") << false;
    QTest::newRow("final") << QString("final") << true;
    QTest::newRow("float") << QString("float") << true;
    QTest::newRow("goto") << QString("goto") << true;
    QTest::newRow("implements") << QString("implements") << true;
    QTest::newRow("import") << QString("import") << false;
    QTest::newRow("int") << QString("int") << true;
    QTest::newRow("interface") << QString("interface") << true;
    QTest::newRow("long") << QString("long") << true;
    QTest::newRow("native") << QString("native") << true;
    QTest::newRow("package") << QString("package") << true;
    QTest::newRow("private") << QString("private") << true;
    QTest::newRow("protected") << QString("protected") << true;
    QTest::newRow("public") << QString("public") << true;
    QTest::newRow("short") << QString("short") << true;
    QTest::newRow("static") << QString("static") << true;
    QTest::newRow("super") << QString("super") << false;
    QTest::newRow("synchronized") << QString("synchronized") << true;
    QTest::newRow("throws") << QString("throws") << true;
    QTest::newRow("transient") << QString("transient") << true;
    QTest::newRow("volatile") << QString("volatile") << true;
}

void tst_QJSEngine::jsFutureReservedWords()
{
    // See ECMA-262 Section 7.6.1.2, "Future Reserved Words".
    // In real-world implementations, most of these words are
    // actually allowed as normal identifiers.

    QFETCH(QString, word);
    QFETCH(bool, allowed);
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate(word + " = 123");
        QCOMPARE(!ret.isError(), allowed);
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("var " + word + " = 123");
        QCOMPARE(!ret.isError(), allowed);
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = {}; o." + word + " = 123");

        QCOMPARE(ret.isNumber(), true);
        QCOMPARE(!ret.isError(), true);
    }
    {
        QJSEngine eng;
        QJSValue ret = eng.evaluate("o = { " + word + ": 123 }");
        QCOMPARE(!ret.isError(), true);
    }
}

void tst_QJSEngine::jsThrowInsideWithStatement()
{
    // This is testing ECMA-262 compliance, not C++ API.

    // task 209988
    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate(
            "try {"
            "  o = { bad : \"bug\" };"
            "  with (o) {"
            "    throw 123;"
            "  }"
            "} catch (e) {"
            "  bad;"
            "}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    {
        QJSValue ret = eng.evaluate(
            "try {"
            "  o = { bad : \"bug\" };"
            "  with (o) {"
            "    throw 123;"
            "  }"
            "} finally {"
            "  bad;"
            "}");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("ReferenceError")));
    }
    {
        QJSValue ret = eng.evaluate(
            "o = { bug : \"no bug\" };"
            "with (o) {"
            "  try {"
            "    throw 123;"
            "  } finally {"
            "    bug;"
            "  }"
            "}");
        QVERIFY(!ret.isError());
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 123);
    }
    {
        QJSValue ret = eng.evaluate(
            "o = { bug : \"no bug\" };"
            "with (o) {"
            "    throw 123;"
            "}");
        QVERIFY(ret.isNumber());
        QJSValue ret2 = eng.evaluate("bug");
        QVERIFY(ret2.isError());
        QVERIFY(ret2.toString().contains(QString::fromLatin1("ReferenceError")));
    }
}

void tst_QJSEngine::reentrancy_globalObjectProperties()
{
    QJSEngine eng1;
    QJSEngine eng2;
    QVERIFY(eng2.globalObject().property("a").isUndefined());
    eng1.evaluate("a = 10");
    QVERIFY(eng1.globalObject().property("a").isNumber());
    QVERIFY(eng2.globalObject().property("a").isUndefined());
    eng2.evaluate("a = 20");
    QVERIFY(eng2.globalObject().property("a").isNumber());
    QCOMPARE(eng1.globalObject().property("a").toInt(), 10);
}

void tst_QJSEngine::reentrancy_Array()
{
    // weird bug with JSC backend
    {
        QJSEngine eng;
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
        eng.evaluate("Array.prototype.toString");
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
    }
    {
        QJSEngine eng;
        QCOMPARE(eng.evaluate("Array()").toString(), QString());
    }
}

void tst_QJSEngine::reentrancy_objectCreation()
{
    // Owned by JS engine, as newQObject() sets JS ownership explicitly
    QObject *temp = new QObject();

    QJSEngine eng1;
    QJSEngine eng2;
    {
        QDateTime dt = QDateTime::currentDateTime();
        QJSValue d1 = eng1.toScriptValue(dt);
        QJSValue d2 = eng2.toScriptValue(dt);
        QCOMPARE(d1.toDateTime(), d2.toDateTime());
        QCOMPARE(d2.toDateTime(), d1.toDateTime());
    }
    {
        QJSValue r1 = eng1.evaluate("new RegExp('foo', 'gim')");
        QJSValue r2 = eng2.evaluate("new RegExp('foo', 'gim')");
        QCOMPARE(qjsvalue_cast<QRegularExpression>(r1), qjsvalue_cast<QRegularExpression>(r2));
        QCOMPARE(qjsvalue_cast<QRegularExpression>(r2), qjsvalue_cast<QRegularExpression>(r1));
    }
    {
        QJSValue o1 = eng1.newQObject(temp);
        QJSValue o2 = eng2.newQObject(temp);
        QCOMPARE(o1.toQObject(), o2.toQObject());
        QCOMPARE(o2.toQObject(), o1.toQObject());
    }
}

void tst_QJSEngine::jsIncDecNonObjectProperty()
{
    // This is testing ECMA-262 compliance, not C++ API.

    QJSEngine eng;
    {
        QJSValue ret = eng.evaluate("var a; a.n++");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n--");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = null; a.n++");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = null; a.n--");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; ++a.n");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; --a.n");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n += 1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a; a.n -= 1");
        QVERIFY(ret.isError());
        QVERIFY(ret.toString().contains(QString::fromLatin1("TypeError")));
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; a.length++");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 4);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; a.length--");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 4);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; ++a.length");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 5);
    }
    {
        QJSValue ret = eng.evaluate("var a = 'ciao'; --a.length");
        QVERIFY(ret.isNumber());
        QCOMPARE(ret.toInt(), 3);
    }
}

void tst_QJSEngine::JSON_Parse()
{
    QJSEngine eng;
    QJSValue ret = eng.evaluate("var json=\"{\\\"1\\\": null}\"; JSON.parse(json);");
    QVERIFY(ret.isObject());
}

void tst_QJSEngine::JSON_Stringify_data()
{
    QTest::addColumn<QString>("object");
    QTest::addColumn<QString>("json");

    // Basic "smoke" test. More tests are provided by test262 suite.
    // Don't test with multiple key-value pairs on the same level,
    // because serialization order might not be deterministic.
    // Note: parenthesis are required, otherwise objects will be interpretted as code blocks.
    QTest::newRow("empty")          << "({})"               << "{}";
    QTest::newRow("string")         << "({a: 'b'})"         << "{\"a\":\"b\"}";
    QTest::newRow("number")         << "({c: 42})"          << "{\"c\":42}";
    QTest::newRow("boolean")        << "({d: true})"        << "{\"d\":true}";
    QTest::newRow("key is array")   << "({[[12, 34]]: 56})" << "{\"12,34\":56}";
    QTest::newRow("value is date")  << "({d: new Date('2000-01-20T12:00:00.000Z')})"  << "{\"d\":\"2000-01-20T12:00:00.000Z\"}";
}

void tst_QJSEngine::JSON_Stringify()
{
    QFETCH(QString, object);
    QFETCH(QString, json);

    QJSEngine eng;

    QJSValue obj = eng.evaluate(object);
    QVERIFY(obj.isObject());

    QJSValue func = eng.evaluate("(function(obj) { return JSON.stringify(obj); })");
    QVERIFY(func.isCallable());

    QJSValue ret = func.call(QJSValueList{obj});
    QVERIFY(ret.isString());
    QCOMPARE(ret.toString(), json);
}

void tst_QJSEngine::JSON_Stringify_WithReplacer_QTBUG_95324()
{
    QJSEngine eng;
    QJSValue json = eng.evaluate(R"(
        function replacer(k, v) {
          if (this[k] instanceof Date) {
            return Math.floor(this[k].getTime() / 1000.0);
          }
          return v;
        }
        const obj = {d: new Date('2000-01-20T12:00:00.000Z')};
        JSON.stringify(obj, replacer);
    )");
    QVERIFY(json.isString());
    QCOMPARE(json.toString(), QString::fromLatin1("{\"d\":948369600}"));
}

void tst_QJSEngine::arraySort()
{
    // tests that calling Array.sort with a bad sort function doesn't cause issues
    // Using std::sort is e.g. not safe when used with a bad sort function and causes
    // crashes
    QJSEngine eng;
    eng.evaluate("function crashMe() {"
                 "    var data = [];"
                 "    for (var i = 0; i < 50; i++) {"
                 "        data[i] = 'whatever';"
                 "    }"
                 "    data.sort(function(a, b) {"
                 "        return -1;"
                 "    });"
                 "}"
                 "crashMe();");
}

void tst_QJSEngine::lookupOnDisappearingProperty()
{
    QJSEngine eng;
    QJSValue func = eng.evaluate("(function(){\"use strict\"; return eval(\"(function(obj) { return obj.someProperty; })\")})()");
    QVERIFY(func.isCallable());

    QJSValue o = eng.newObject();
    o.setProperty(QStringLiteral("someProperty"), 42);

    QCOMPARE(func.call(QJSValueList()<< o).toInt(), 42);

    o = eng.newObject();
    QVERIFY(func.call(QJSValueList()<< o).isUndefined());
    QVERIFY(func.call(QJSValueList()<< o).isUndefined());
}

void tst_QJSEngine::arrayConcat()
{
    QJSEngine eng;
    QJSValue v = eng.evaluate("var x = [1, 2, 3, 4, 5, 6];"
                              "var y = [];"
                              "for (var i = 0; i < 5; ++i)"
                              "    x.shift();"
                              "for (var i = 10; i < 13; ++i)"
                              "   x.push(i);"
                              "x.toString();");
    QCOMPARE(v.toString(), QString::fromLatin1("6,10,11,12"));
}

void tst_QJSEngine::recursiveBoundFunctions()
{

    QJSEngine eng;
    QJSValue v = eng.evaluate("function foo(x, y, z)"
                              "{ return this + x + y + z; }"
                              "var bar = foo.bind(-1, 10);"
                              "var baz = bar.bind(-2, 20);"
                              "baz(30)");
    QCOMPARE(v.toInt(), 59);
}

void tst_QJSEngine::qRegularExpressionImport_data()
{
    QTest::addColumn<QRegularExpression>("rx");
    QTest::addColumn<QString>("string");

    QTest::newRow("normal")            << QRegularExpression("(test|foo)") << "test _ foo _ test _ Foo";
    QTest::newRow("normal2")           << QRegularExpression("(Test|Foo)") << "test _ foo _ test _ Foo";
    QTest::newRow("case insensitive")  << QRegularExpression("(test|foo)", QRegularExpression::CaseInsensitiveOption) << "test _ foo _ test _ Foo";
    QTest::newRow("case insensitive2") << QRegularExpression("(Test|Foo)", QRegularExpression::CaseInsensitiveOption) << "test _ foo _ test _ Foo";
    QTest::newRow("b(a*)(b*)")         << QRegularExpression("b(a*)(b*)", QRegularExpression::CaseInsensitiveOption) << "aaabbBbaAabaAaababaaabbaaab";
    QTest::newRow("greedy")            << QRegularExpression("a*(a*)", QRegularExpression::CaseInsensitiveOption) << "aaaabaaba";
    QTest::newRow("wildcard")          << QRegularExpression(".*\\.txt") << "file.txt";
    QTest::newRow("wildcard 2")        << QRegularExpression("a.b\\.txt") << "ab.txt abb.rtc acb.txt";
    QTest::newRow("slash")             << QRegularExpression("g/.*/s", QRegularExpression::CaseInsensitiveOption) << "string/string/string";
    QTest::newRow("slash2")            << QRegularExpression("g / .* / s", QRegularExpression::CaseInsensitiveOption) << "string / string / string";
    QTest::newRow("fixed")             << QRegularExpression("a\\*aa\\.a\\(ba\\)\\*a\\\\ba", QRegularExpression::CaseInsensitiveOption) << "aa*aa.a(ba)*a\\ba";
    QTest::newRow("fixed insensitive") << QRegularExpression("A\\*A", QRegularExpression::CaseInsensitiveOption) << "a*A A*a A*A a*a";
    QTest::newRow("fixed sensitive")   << QRegularExpression("A\\*A") << "a*A A*a A*A a*a";
    QTest::newRow("html")              << QRegularExpression("<b>(.*)</b>") << "<b>bold</b><i>italic</i><b>bold</b>";
    QTest::newRow("html minimal")      << QRegularExpression("^<b>(.*)</b>$") << "<b>bold</b><i>italic</i><b>bold</b>";
    QTest::newRow("aaa")               << QRegularExpression("a{2,5}") << "aAaAaaaaaAa";
    QTest::newRow("aaa minimal")       << QRegularExpression("^a{2,5}$") << "aAaAaaaaaAa";
    QTest::newRow("minimal")           << QRegularExpression("^.*\\} [*8]$") << "}?} ?} *";
    QTest::newRow(".? minimal")        << QRegularExpression("^.?$") << ".?";
    QTest::newRow(".+ minimal")        << QRegularExpression("^.+$") << ".+";
    QTest::newRow("[.?] minimal")      << QRegularExpression("^[.?]$") << ".?";
    QTest::newRow("[.+] minimal")      << QRegularExpression("^[.+]$") << ".+";
    QTest::newRow("aaa inverted greedyness")  << QRegularExpression("a{2,5}", QRegularExpression::InvertedGreedinessOption) << "aAaAaaaaaAa";
    QTest::newRow("inverted greedyness")  << QRegularExpression(".*\\} [*8]", QRegularExpression::InvertedGreedinessOption) << "}?} ?} *";
    QTest::newRow(".? inverted greedyness")  << QRegularExpression(".?", QRegularExpression::InvertedGreedinessOption) << ".?";
    QTest::newRow(".+ inverted greedyness")  << QRegularExpression(".+", QRegularExpression::InvertedGreedinessOption) << ".+";
    QTest::newRow("[.?] inverted greedyness")  << QRegularExpression("[.?]", QRegularExpression::InvertedGreedinessOption) << ".?";
    QTest::newRow("[.+] inverted greedyness")  << QRegularExpression("[.+]", QRegularExpression::InvertedGreedinessOption) << ".+";
    QTest::newRow("two lines")  << QRegularExpression("^.*$") << "abc\ndef";
    QTest::newRow("multiline")  << QRegularExpression("^.*$", QRegularExpression::MultilineOption) << "abc\ndef";
}

void tst_QJSEngine::qRegularExpressionImport()
{
    QFETCH(QRegularExpression, rx);
    QFETCH(QString, string);

    QJSEngine eng;
    QJSValue rexp;
    rexp = eng.toScriptValue(rx);

    QCOMPARE(rexp.isRegExp(), true);
    QCOMPARE(rexp.isCallable(), false);

    QJSValue func = eng.evaluate("(function(string, regexp) { return string.match(regexp); })");
    QJSValue result = func.call(QJSValueList() << string << rexp);

    const QRegularExpressionMatch match = rx.match(string);
    for (int i = 0; i <= match.lastCapturedIndex(); i++)
        QCOMPARE(result.property(i).toString(), match.captured(i));
}

void tst_QJSEngine::qRegularExpressionExport_data()
{
    QTest::addColumn<QString>("js");
    QTest::addColumn<QRegularExpression>("regularexpression");

    QTest::newRow("normal")            << "/(test|foo)/" << QRegularExpression("(test|foo)");
    QTest::newRow("normal2")           << "/(Test|Foo)/" << QRegularExpression("(Test|Foo)");
    QTest::newRow("case insensitive")  << "/(test|foo)/i" << QRegularExpression("(test|foo)", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("case insensitive2") << "/(Test|Foo)/i" << QRegularExpression("(Test|Foo)", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("b(a*)(b*)")         << "/b(a*)(b*)/i" << QRegularExpression("b(a*)(b*)", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("greedy")            << "/a*(a*)/i" << QRegularExpression("a*(a*)", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("wildcard")          << "/.*\\.txt/" << QRegularExpression(".*\\.txt");
    QTest::newRow("wildcard 2")        << "/a.b\\.txt/" << QRegularExpression("a.b\\.txt");
    QTest::newRow("slash")             << "/g\\/.*\\/s/i" << QRegularExpression("g\\/.*\\/s", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("slash2")            << "/g \\/ .* \\/ s/i" << QRegularExpression("g \\/ .* \\/ s", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("fixed")             << "/a\\*aa\\.a\\(ba\\)\\*a\\\\ba/i" << QRegularExpression("a\\*aa\\.a\\(ba\\)\\*a\\\\ba", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("fixed insensitive") << "/A\\*A/i" << QRegularExpression("A\\*A", QRegularExpression::CaseInsensitiveOption);
    QTest::newRow("fixed sensitive")   << "/A\\*A/" << QRegularExpression("A\\*A");
    QTest::newRow("html")              << "/<b>(.*)<\\/b>/" << QRegularExpression("<b>(.*)<\\/b>");
    QTest::newRow("html minimal")      << "/^<b>(.*)<\\/b>$/" << QRegularExpression("^<b>(.*)<\\/b>$");
    QTest::newRow("aaa")               << "/a{2,5}/" << QRegularExpression("a{2,5}");
    QTest::newRow("aaa minimal")       << "/^a{2,5}$/" << QRegularExpression("^a{2,5}$");
    QTest::newRow("minimal")           << "/^.*\\} [*8]$/" << QRegularExpression("^.*\\} [*8]$");
    QTest::newRow(".? minimal")        << "/^.?$/" << QRegularExpression("^.?$");
    QTest::newRow(".+ minimal")        << "/^.+$/" << QRegularExpression("^.+$");
    QTest::newRow("[.?] minimal")      << "/^[.?]$/" << QRegularExpression("^[.?]$");
    QTest::newRow("[.+] minimal")      << "/^[.+]$/" << QRegularExpression("^[.+]$");
    QTest::newRow("multiline")  << "/^.*$/m" << QRegularExpression("^.*$", QRegularExpression::MultilineOption);
}

void tst_QJSEngine::qRegularExpressionExport()
{
    QFETCH(QString, js);
    QFETCH(QRegularExpression, regularexpression);

    QJSEngine eng;
    QJSValue rexp;
    rexp = eng.evaluate(js);

    QCOMPARE(rexp.isRegExp(), true);
    QCOMPARE(rexp.isCallable(), false);

    QRegularExpression rx = qjsvalue_cast<QRegularExpression>(rexp);
    QCOMPARE(rx, regularexpression);
}

// QScriptValue::toDateTime() returns a local time, whereas JS dates
// are always stored as UTC. Qt Script must respect the current time
// zone, and correctly adjust for daylight saving time that may be in
// effect at a given date (QTBUG-9770).
void tst_QJSEngine::dateRoundtripJSQtJS()
{
    qint64 secs = QDate(2009, 1, 1).startOfDay(QTimeZone::UTC).toSecsSinceEpoch();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.evaluate(QString::fromLatin1("new Date(%0)").arg(secs * 1000.0));
        QDateTime qtDate = jsDate.toDateTime();
        QJSValue jsDate2 = eng.toScriptValue(qtDate);
        if (jsDate2.toNumber() != jsDate.toNumber())
            QFAIL(qPrintable(jsDate.toString()));
        secs += 2*60*60;
    }
}

void tst_QJSEngine::dateRoundtripQtJSQt()
{
    QDateTime qtDate = QDate(2009, 1, 1).startOfDay();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.toScriptValue(qtDate);
        QDateTime qtDate2 = jsDate.toDateTime();
        if (qtDate2 != qtDate)
            QFAIL(qPrintable(qtDate.toString()));
        qtDate = qtDate.addSecs(2*60*60);
    }
}

void tst_QJSEngine::dateConversionJSQt()
{
    qint64 secs = QDate(2009, 1, 1).startOfDay(QTimeZone::UTC).toSecsSinceEpoch();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.evaluate(QString::fromLatin1("new Date(%0)").arg(secs * 1000.0));
        QDateTime qtDate = jsDate.toDateTime();
        QString qtUTCDateStr = qtDate.toUTC().toString(Qt::ISODate);
        QString jsUTCDateStr = jsDate.property("toISOString").callWithInstance(jsDate).toString();
        jsUTCDateStr.remove(jsUTCDateStr.size() - 5, 4); // get rid of milliseconds (".000")
        if (qtUTCDateStr != jsUTCDateStr)
            QFAIL(qPrintable(jsDate.toString()));
        secs += 2*60*60;
    }
}

void tst_QJSEngine::dateConversionQtJS()
{
    QDateTime qtDate = QDate(2009, 1, 1).startOfDay();
    QJSEngine eng;
    for (int i = 0; i < 8000; ++i) {
        QJSValue jsDate = eng.toScriptValue(qtDate);
        QString jsUTCDateStr = jsDate.property("toISOString").callWithInstance(jsDate).toString();
        QString qtUTCDateStr = qtDate.toUTC().toString(Qt::ISODate);
        jsUTCDateStr.remove(jsUTCDateStr.size() - 5, 4); // get rid of milliseconds (".000")
        if (jsUTCDateStr != qtUTCDateStr)
            QFAIL(qPrintable(qtDate.toString()));
        qtDate = qtDate.addSecs(2*60*60);
    }
}

void tst_QJSEngine::functionPrototypeExtensions()
{
    // QJS adds connect and disconnect properties to Function.prototype.
    QJSEngine eng;
    QJSValue funProto = eng.globalObject().property("Function").property("prototype");
    QVERIFY(funProto.isCallable());
    QVERIFY(funProto.property("connect").isCallable());
    QVERIFY(funProto.property("disconnect").isCallable());

    // No properties should appear in for-in statements.
    QJSValue props = eng.evaluate("props = []; for (var p in Function.prototype) props.push(p); props");
    QVERIFY(props.isArray());
    QCOMPARE(props.property("length").toInt(), 0);
}

class ThreadedTestEngine : public QThread {
    Q_OBJECT;

public:
    int result = 0;

    ThreadedTestEngine() {}

    void run() override {
        QJSEngine firstEngine;
        QJSEngine secondEngine;
        QJSValue value = firstEngine.evaluate("1");
        result = secondEngine.evaluate("1 + " + QString::number(value.toInt())).toInt();
    }
};

void tst_QJSEngine::threadedEngine()
{
    ThreadedTestEngine thread1;
    ThreadedTestEngine thread2;
    thread1.start();
    thread2.start();
    thread1.wait();
    thread2.wait();
    QCOMPARE(thread1.result, 2);
    QCOMPARE(thread2.result, 2);
}

void tst_QJSEngine::functionDeclarationsInConditionals()
{
    // Visibility of function declarations inside blocks is limited to the block
    QJSEngine eng;
    QJSValue result = eng.evaluate("if (true) {\n"
                                   "    function blah() { return false; }\n"
                                   "} else {\n"
                                   "    function blah() { return true; }\n"
                                   "}\n"
                                   "blah();");
    QVERIFY(result.isError());
}

void tst_QJSEngine::arrayPop_QTBUG_35979()
{
    QJSEngine eng;
    QJSValue result = eng.evaluate(""
            "var x = [1, 2]\n"
            "x.pop()\n"
            "x[1] = 3\n"
            "x.toString()\n");
    QCOMPARE(result.toString(), QString("1,3"));
}

void tst_QJSEngine::array_unshift_QTBUG_52065()
{
    QJSEngine eng;
    QJSValue result = eng.evaluate("[1, 2, 3, 4, 5, 6, 7, 8, 9]");
    QJSValue unshift = result.property(QStringLiteral("unshift"));
    unshift.callWithInstance(result, QJSValueList() << QJSValue(0));

    int len = result.property(QStringLiteral("length")).toInt();
    QCOMPARE(len, 10);

    for (int i = 0; i < len; ++i)
        QCOMPARE(result.property(i).toInt(), i);
}

void tst_QJSEngine::array_join_QTBUG_53672()
{
    QJSEngine eng;
    QJSValue result = eng.evaluate("Array.prototype.join.call(0)");
    QVERIFY(result.isString());
    QCOMPARE(result.toString(), QString(""));
}

void tst_QJSEngine::regexpLastMatch()
{
    QJSEngine eng;

    QCOMPARE(eng.evaluate("RegExp.input").toString(), QString());

    QJSValue hasProperty;

    for (int i = 1; i < 9; ++i) {
        hasProperty = eng.evaluate("RegExp.hasOwnProperty(\"$" + QString::number(i) + "\")");
        QVERIFY(hasProperty.isBool());
        QVERIFY(hasProperty.toBool());
    }

    hasProperty = eng.evaluate("RegExp.hasOwnProperty(\"$0\")");
    QVERIFY(hasProperty.isBool());
    QVERIFY(!hasProperty.toBool());

    hasProperty = eng.evaluate("RegExp.hasOwnProperty(\"$10\")");
    QVERIFY(!hasProperty.toBool());

    hasProperty = eng.evaluate("RegExp.hasOwnProperty(\"lastMatch\")");
    QVERIFY(hasProperty.toBool());
    hasProperty = eng.evaluate("RegExp.hasOwnProperty(\"$&\")");
    QVERIFY(hasProperty.toBool());

    QJSValue result = eng.evaluate(""
            "var re = /h(el)l(o)/\n"
            "var text = \"blah hello world\"\n"
            "text.match(re)\n");
    QVERIFY(!result.isError());
    QJSValue match = eng.evaluate("RegExp.$1");
    QCOMPARE(match.toString(), QString("el"));
    match = eng.evaluate("RegExp.$2");
    QCOMPARE(match.toString(), QString("o"));
    for (int i = 3; i <= 9; ++i) {
        match = eng.evaluate("RegExp.$" + QString::number(i));
        QVERIFY(match.isString());
        QCOMPARE(match.toString(), QString());
    }
    QCOMPARE(eng.evaluate("RegExp.input").toString(), QString("blah hello world"));
    QCOMPARE(eng.evaluate("RegExp.lastParen").toString(), QString("o"));
    QCOMPARE(eng.evaluate("RegExp.leftContext").toString(), QString("blah "));
    QCOMPARE(eng.evaluate("RegExp.rightContext").toString(), QString(" world"));

    QCOMPARE(eng.evaluate("RegExp.lastMatch").toString(), QString("hello"));

    result = eng.evaluate(""
            "var re = /h(ello)/\n"
            "var text = \"hello\"\n"
            "text.match(re)\n");
    QVERIFY(!result.isError());
    match = eng.evaluate("RegExp.$1");
    QCOMPARE(match.toString(), QString("ello"));
    for (int i = 2; i <= 9; ++i) {
        match = eng.evaluate("RegExp.$" + QString::number(i));
        QVERIFY(match.isString());
        QCOMPARE(match.toString(), QString());
    }

}

void tst_QJSEngine::regexpLastIndex()
{
    QJSEngine eng;
    QJSValue result;
    result = eng.evaluate("function test(text, rx) {"
                          "    var res;"
                          "    while (res = rx.exec(text)) { "
                          "        return true;"
                          "    }"
                          "    return false;"
                          " }"
                          "function tester(text) {"
                          "    return test(text, /,\\s*/g);"
                          "}");
    QVERIFY(!result.isError());

    result = eng.evaluate("tester(\",  \\n\");");
    QVERIFY(result.toBool());
    result = eng.evaluate("tester(\",  \\n\");");
    QVERIFY(result.toBool());
}

void tst_QJSEngine::indexedAccesses()
{
    QJSEngine engine;
    QJSValue v = engine.evaluate("function foo() { return 1[1] } foo()");
    QVERIFY(v.isUndefined());
    v = engine.evaluate("function foo() { return /x/[1] } foo()");
    QVERIFY(v.isUndefined());
    v = engine.evaluate("function foo() { return \"xy\"[1] } foo()");
    QVERIFY(v.isString());
    v = engine.evaluate("function foo() { return \"xy\"[2] } foo()");
    QVERIFY(v.isUndefined());
}

void tst_QJSEngine::prototypeChainGc()
{
    QJSEngine engine;

    QJSValue getProto = engine.evaluate("Object.getPrototypeOf");

    QJSValue factory = engine.evaluate("(function() { return Object.create(Object.create({})); })");
    QVERIFY(factory.isCallable());
    QJSValue obj = factory.call();
    gc(*engine.handle());

    QJSValue proto = getProto.call(QJSValueList() << obj);
    proto = getProto.call(QJSValueList() << proto);
    QVERIFY(proto.isObject());
}

void tst_QJSEngine::prototypeChainGc_QTBUG38299()
{
    QJSEngine engine;
    engine.evaluate("var mapping = {"
                    "'prop1': \"val1\",\n"
                    "'prop2': \"val2\"\n"
                    "}\n"
                    "\n"
                    "delete mapping.prop2\n"
                    "delete mapping.prop1\n"
                    "\n");
    // Don't hang!
    gc(*engine.handle());
}

void tst_QJSEngine::dynamicProperties()
{
    {
        QJSEngine engine;
        QScopedPointer<QObject> obj(new QObject);
        QJSValue wrapper = engine.newQObject(obj.data());
        wrapper.setProperty("someRandomProperty", 42);
        QCOMPARE(wrapper.property("someRandomProperty").toInt(), 42);
        QVERIFY(!qmlContext(obj.data()));
    }
    {
        QQmlEngine qmlEngine;
        QQmlComponent component(&qmlEngine);
        component.setData("import QtQml 2.0; QtObject { property QtObject subObject: QtObject {} }", QUrl());
        QScopedPointer<QObject> root(component.create(nullptr));
        QVERIFY(root);
        QVERIFY(qmlContext(root.data()));

        QJSValue wrapper = qmlEngine.newQObject(root.data());
        wrapper.setProperty("someRandomProperty", 42);
        QVERIFY(!wrapper.hasProperty("someRandomProperty"));

        QObject *subObject = qvariant_cast<QObject*>(root->property("subObject"));
        QVERIFY(subObject);
        QVERIFY(qmlContext(subObject));

        wrapper = qmlEngine.newQObject(subObject);
        wrapper.setProperty("someRandomProperty", 42);
        QVERIFY(!wrapper.hasProperty("someRandomProperty"));
    }
}

class EvaluateWrapper : public QObject
{
    Q_OBJECT
public:
    EvaluateWrapper(QJSEngine *engine)
        : engine(engine)
    {}

public slots:
    QJSValue cppEvaluate(const QString &program)
    {
        return engine->evaluate(program);
    }

private:
    QJSEngine *engine;
};

void tst_QJSEngine::scopeOfEvaluate()
{
    QJSEngine engine;
    QJSValue wrapper = engine.newQObject(new EvaluateWrapper(&engine));

    engine.evaluate("testVariable = 42");

    QJSValue function = engine.evaluate("(function(evalWrapper){\n"
                                        "var testVariable = 100; \n"
                                        "try { \n"
                                        "    return evalWrapper.cppEvaluate(\"(function() { return testVariable; })\")\n"
                                        "           ()\n"
                                        "} catch (e) {}\n"
                                        "})");
    QVERIFY(function.isCallable());
    QJSValue result = function.call(QJSValueList() << wrapper);
    QCOMPARE(result.toInt(), 42);
}

void tst_QJSEngine::callConstants()
{
    QJSEngine engine;
    engine.evaluate("function f() {\n"
                    "  var one; one();\n"
                    "  var two = null; two();\n"
                    "}\n");
    QJSValue exceptionResult = engine.evaluate("true()");
    QCOMPARE(exceptionResult.toString(), QString("TypeError: true is not a function"));
}

void tst_QJSEngine::installTranslationFunctions()
{
    QJSEngine eng;
    QJSValue global = eng.globalObject();
    QVERIFY(global.property("qsTranslate").isUndefined());
    QVERIFY(global.property("QT_TRANSLATE_NOOP").isUndefined());
    QVERIFY(global.property("qsTr").isUndefined());
    QVERIFY(global.property("QT_TR_NOOP").isUndefined());
    QVERIFY(global.property("qsTrId").isUndefined());
    QVERIFY(global.property("QT_TRID_NOOP").isUndefined());

    eng.installExtensions(QJSEngine::TranslationExtension);
    QVERIFY(global.property("qsTranslate").isCallable());
    QVERIFY(global.property("QT_TRANSLATE_NOOP").isCallable());
    QVERIFY(global.property("qsTr").isCallable());
    QVERIFY(global.property("QT_TR_NOOP").isCallable());
    QVERIFY(global.property("qsTrId").isCallable());
    QVERIFY(global.property("QT_TRID_NOOP").isCallable());

    {
        QJSValue ret = eng.evaluate("qsTr('foo')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("qsTranslate('foo', 'bar')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("bar"));
    }
    {
        QJSValue ret = eng.evaluate("QT_TR_NOOP('foo')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("QT_TRANSLATE_NOOP('foo', 'bar')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("bar"));
    }

    {
        QJSValue ret = eng.evaluate("qsTrId('foo')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("QT_TRID_NOOP('foo')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
    {
        QJSValue ret = eng.evaluate("qsTr('%1').arg('foo')");
        QVERIFY(ret.isString());
        QCOMPARE(ret.toString(), QString::fromLatin1("foo"));
    }
    QVERIFY(eng.evaluate("QT_TRID_NOOP()").isUndefined());
}

class TranslationScope
{
public:
    TranslationScope(const QString &fileName)
    {
        if (!translator.load(fileName))
            QFAIL("failed to load translation");
        QCoreApplication::instance()->installTranslator(&translator);
    }
    ~TranslationScope()
    {
        QCoreApplication::instance()->removeTranslator(&translator);
    }

private:
    QTranslator translator;
};

void tst_QJSEngine::translateScript_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedTranslation");

    QString fileName = QString::fromLatin1("translatable.js");
    // Top-level
    QTest::newRow("qsTr('One')@translatable.js")
            << QString::fromLatin1("qsTr('One')") << fileName << QString::fromLatin1("En");
    QTest::newRow("qsTr('Hello')@translatable.js")
            << QString::fromLatin1("qsTr('Hello')") << fileName << QString::fromLatin1("Hallo");
    // From function
    QTest::newRow("(function() { return qsTr('One'); })()@translatable.js")
            << QString::fromLatin1("(function() { return qsTr('One'); })()") << fileName << QString::fromLatin1("En");
    QTest::newRow("(function() { return qsTr('Hello'); })()@translatable.js")
            << QString::fromLatin1("(function() { return qsTr('Hello'); })()") << fileName << QString::fromLatin1("Hallo");
    // Plural
    QTest::newRow("qsTr('%n message(s) saved', '', 1)@translatable.js")
            << QString::fromLatin1("qsTr('%n message(s) saved', '', 1)") << fileName << QString::fromLatin1("1 melding lagret");
    QTest::newRow("qsTr('%n message(s) saved', '', 3).arg@translatable.js")
            << QString::fromLatin1("qsTr('%n message(s) saved', '', 3)") << fileName << QString::fromLatin1("3 meldinger lagret");

    // Top-level
    QTest::newRow("qsTranslate('FooContext', 'Two')@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'Two')") << fileName << QString::fromLatin1("To");
    QTest::newRow("qsTranslate('FooContext', 'Goodbye')@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'Goodbye')") << fileName << QString::fromLatin1("Farvel");
    // From eval
    QTest::newRow("eval('qsTranslate(\\'FooContext\\', \\'Two\\')')@translatable.js")
            << QString::fromLatin1("eval('qsTranslate(\\'FooContext\\', \\'Two\\')')") << fileName << QString::fromLatin1("To");
    QTest::newRow("eval('qsTranslate(\\'FooContext\\', \\'Goodbye\\')')@translatable.js")
            << QString::fromLatin1("eval('qsTranslate(\\'FooContext\\', \\'Goodbye\\')')") << fileName << QString::fromLatin1("Farvel");

    QTest::newRow("qsTranslate('FooContext', 'Goodbye', '')@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'Goodbye', '')") << fileName << QString::fromLatin1("Farvel");

    QTest::newRow("qsTranslate('FooContext', 'Goodbye', '', 42)@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'Goodbye', '', 42)") << fileName << QString::fromLatin1("Goodbye");

    QTest::newRow("qsTr('One', 'not the same one')@translatable.js")
            << QString::fromLatin1("qsTr('One', 'not the same one')") << fileName << QString::fromLatin1("Enda en");

    QTest::newRow("qsTr('One', 'not the same one', 42)@translatable.js")
            << QString::fromLatin1("qsTr('One', 'not the same one', 42)") << fileName << QString::fromLatin1("One");

    // Plural
    QTest::newRow("qsTranslate('FooContext', '%n fooish bar(s) found', '', 1)@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', '%n fooish bar(s) found', '', 1)") << fileName << QString::fromLatin1("1 fooaktig bar funnet");
    QTest::newRow("qsTranslate('FooContext', '%n fooish bar(s) found', '', 2)@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', '%n fooish bar(s) found', '', 2)") << fileName << QString::fromLatin1("2 fooaktige barer funnet");

    // Don't exist in translation
    QTest::newRow("qsTr('Three')@translatable.js")
            << QString::fromLatin1("qsTr('Three')") << fileName << QString::fromLatin1("Three");
    QTest::newRow("qsTranslate('FooContext', 'So long')@translatable.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'So long')") << fileName << QString::fromLatin1("So long");
    QTest::newRow("qsTranslate('BarContext', 'Goodbye')@translatable.js")
            << QString::fromLatin1("qsTranslate('BarContext', 'Goodbye')") << fileName << QString::fromLatin1("Goodbye");

    // Translate strings from the second script (translatable2.js)

    QString fileName2 = QString::fromLatin1("translatable2.js");
    QTest::newRow("qsTr('Three')@translatable2.js")
            << QString::fromLatin1("qsTr('Three')") << fileName2 << QString::fromLatin1("Tre");
    QTest::newRow("qsTr('Happy birthday!')@translatable2.js")
            << QString::fromLatin1("qsTr('Happy birthday!')") << fileName2 << QString::fromLatin1("Gratulerer med dagen!");

    // Not translated because translation is only in translatable.js
    QTest::newRow("qsTr('One')@translatable2.js")
            << QString::fromLatin1("qsTr('One')") << fileName2 << QString::fromLatin1("One");
    QTest::newRow("(function() { return qsTr('One'); })()@translatable2.js")
            << QString::fromLatin1("(function() { return qsTr('One'); })()") << fileName2 << QString::fromLatin1("One");

    // For qsTranslate() the filename shouldn't matter
    QTest::newRow("qsTranslate('FooContext', 'Two')@translatable2.js")
            << QString::fromLatin1("qsTranslate('FooContext', 'Two')") << fileName2 << QString::fromLatin1("To");
    QTest::newRow("qsTranslate('BarContext', 'Congratulations!')@translatable.js")
            << QString::fromLatin1("qsTranslate('BarContext', 'Congratulations!')") << fileName << QString::fromLatin1("Gratulerer!");
}

void tst_QJSEngine::translateScript()
{
    QFETCH(QString, expression);
    QFETCH(QString, fileName);
    QFETCH(QString, expectedTranslation);

    QJSEngine engine;

    TranslationScope tranScope(":/translations/translatable_la");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QCOMPARE(engine.evaluate(expression, fileName).toString(), expectedTranslation);
}

void tst_QJSEngine::translateScript_crossScript()
{
    QJSEngine engine;
    TranslationScope tranScope(":/translations/translatable_la");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QString fileName = QString::fromLatin1("translatable.js");
    QString fileName2 = QString::fromLatin1("translatable2.js");
    // qsTr() should use the innermost filename as context
    engine.evaluate("function foo(s) { return bar(s); }", fileName);
    engine.evaluate("function bar(s) { return qsTr(s); }", fileName2);
    QCOMPARE(engine.evaluate("bar('Three')", fileName2).toString(), QString::fromLatin1("Tre"));
    QCOMPARE(engine.evaluate("bar('Three')", fileName).toString(), QString::fromLatin1("Tre"));
    QCOMPARE(engine.evaluate("bar('One')", fileName2).toString(), QString::fromLatin1("One"));

    engine.evaluate("function foo(s) { return bar(s); }", fileName2);
    engine.evaluate("function bar(s) { return qsTr(s); }", fileName);
    QCOMPARE(engine.evaluate("bar('Three')", fileName2).toString(), QString::fromLatin1("Three"));
    QCOMPARE(engine.evaluate("bar('One')", fileName).toString(), QString::fromLatin1("En"));
    QCOMPARE(engine.evaluate("bar('One')", fileName2).toString(), QString::fromLatin1("En"));
}

void tst_QJSEngine::translateScript_trNoOp()
{
    QJSEngine engine;
    TranslationScope tranScope(":/translations/translatable_la");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QVERIFY(engine.evaluate("QT_TR_NOOP()").isUndefined());
    QCOMPARE(engine.evaluate("QT_TR_NOOP('One')").toString(), QString::fromLatin1("One"));

    QVERIFY(engine.evaluate("QT_TRANSLATE_NOOP()").isUndefined());
    QVERIFY(engine.evaluate("QT_TRANSLATE_NOOP('FooContext')").isUndefined());
    QCOMPARE(engine.evaluate("QT_TRANSLATE_NOOP('FooContext', 'Two')").toString(), QString::fromLatin1("Two"));
}

void tst_QJSEngine::translateScript_callQsTrFromCpp()
{
    QJSEngine engine;
    TranslationScope tranScope(":/translations/translatable_la");
    engine.installExtensions(QJSEngine::TranslationExtension);

    // There is no context, but it shouldn't crash
    QCOMPARE(engine.globalObject().property("qsTr").call(QJSValueList() << "One").toString(), QString::fromLatin1("One"));
}

void tst_QJSEngine::translateWithInvalidArgs_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedError");

    QTest::newRow("qsTr()")  << "qsTr()" << "Error: qsTr() requires at least one argument";
    QTest::newRow("qsTr(123)")  << "qsTr(123)" << "Error: qsTr(): first argument (sourceText) must be a string";
    QTest::newRow("qsTr('foo', 123)")  << "qsTr('foo', 123)" << "Error: qsTr(): second argument (disambiguation) must be a string";
    QTest::newRow("qsTr('foo', 'bar', 'baz')")  << "qsTr('foo', 'bar', 'baz')" << "Error: qsTr(): third argument (n) must be a number";
    QTest::newRow("qsTr('foo', 'bar', true)")  << "qsTr('foo', 'bar', true)" << "Error: qsTr(): third argument (n) must be a number";

    QTest::newRow("qsTranslate()")  << "qsTranslate()" << "Error: qsTranslate() requires at least two arguments";
    QTest::newRow("qsTranslate('foo')")  << "qsTranslate('foo')" << "Error: qsTranslate() requires at least two arguments";
    QTest::newRow("qsTranslate(123, 'foo')")  << "qsTranslate(123, 'foo')" << "Error: qsTranslate(): first argument (context) must be a string";
    QTest::newRow("qsTranslate('foo', 123)")  << "qsTranslate('foo', 123)" << "Error: qsTranslate(): second argument (sourceText) must be a string";
    QTest::newRow("qsTranslate('foo', 'bar', 123)")  << "qsTranslate('foo', 'bar', 123)" << "Error: qsTranslate(): third argument (disambiguation) must be a string";

    QTest::newRow("qsTrId()")  << "qsTrId()" << "Error: qsTrId() requires at least one argument";
    QTest::newRow("qsTrId(123)")  << "qsTrId(123)" << "TypeError: qsTrId(): first argument (id) must be a string";
    QTest::newRow("qsTrId('foo', 'bar')")  << "qsTrId('foo', 'bar')" << "TypeError: qsTrId(): second argument (n) must be a number";
}

void tst_QJSEngine::translateWithInvalidArgs()
{
    QFETCH(QString, expression);
    QFETCH(QString, expectedError);
    QJSEngine engine;
    engine.installExtensions(QJSEngine::TranslationExtension);
    QJSValue result = engine.evaluate(expression);
    QVERIFY(result.isError());
    QCOMPARE(result.toString(), expectedError);
}

void tst_QJSEngine::translationContext_data()
{
    QTest::addColumn<QString>("path");
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("expectedTranslation");

    QTest::newRow("translatable.js")  << "translatable.js" << "One" << "En";
    QTest::newRow("/translatable.js")  << "/translatable.js" << "One" << "En";
    QTest::newRow("/foo/translatable.js")  << "/foo/translatable.js" << "One" << "En";
    QTest::newRow("/foo/bar/translatable.js")  << "/foo/bar/translatable.js" << "One" << "En";
    QTest::newRow("./translatable.js")  << "./translatable.js" << "One" << "En";
    QTest::newRow("../translatable.js")  << "../translatable.js" << "One" << "En";
    QTest::newRow("foo/translatable.js")  << "foo/translatable.js" << "One" << "En";
    QTest::newRow("file:///home/qt/translatable.js")  << "file:///home/qt/translatable.js" << "One" << "En";
    QTest::newRow(":/resources/translatable.js")  << ":/resources/translatable.js" << "One" << "En";
    QTest::newRow("/translatable.1.0.js")  << "/translatable.1.0.js" << "One" << "En";
    QTest::newRow("/translatable.txt")  << "/translatable.txt" << "One" << "En";
    QTest::newRow("translatable")  << "translatable" << "One" << "En";
    QTest::newRow("foo/translatable")  << "foo/translatable" << "One" << "En";

    QTest::newRow("translatable.js/")  << "translatable.js/" << "One" << "One";
    QTest::newRow("nosuchscript.js")  << "" << "One" << "One";
    QTest::newRow("(empty)")  << "" << "One" << "One";
}

void tst_QJSEngine::translationContext()
{
    TranslationScope tranScope(":/translations/translatable_la");

    QJSEngine engine;
    engine.installExtensions(QJSEngine::TranslationExtension);

    QFETCH(QString, path);
    QFETCH(QString, text);
    QFETCH(QString, expectedTranslation);
    QJSValue ret = engine.evaluate(QString::fromLatin1("qsTr('%0')").arg(text), path);
    QVERIFY(ret.isString());
    QCOMPARE(ret.toString(), expectedTranslation);
}

void tst_QJSEngine::translateScriptIdBased()
{
    QJSEngine engine;

    TranslationScope tranScope(":/translations/idtranslatable_la");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QString fileName = QString::fromLatin1("idtranslatable.js");

    QHash<QString, QString> expectedTranslations;
    expectedTranslations["qtn_foo_bar"] = "First string";
    expectedTranslations["qtn_needle"] = "Second string";
    expectedTranslations["qtn_haystack"] = "Third string";
    expectedTranslations["qtn_bar_baz"] = "Fourth string";

    QHash<QString, QString>::const_iterator it;
    for (it = expectedTranslations.constBegin(); it != expectedTranslations.constEnd(); ++it) {
        for (int x = 0; x < 2; ++x) {
            QString fn;
            if (x)
                fn = fileName;
            // Top-level
            QCOMPARE(engine.evaluate(QString::fromLatin1("qsTrId('%0')")
                                     .arg(it.key()), fn).toString(),
                     it.value());
            QCOMPARE(engine.evaluate(QString::fromLatin1("QT_TRID_NOOP('%0')")
                                     .arg(it.key()), fn).toString(),
                     it.key());
            // From function
            QCOMPARE(engine.evaluate(QString::fromLatin1("(function() { return qsTrId('%0'); })()")
                                     .arg(it.key()), fn).toString(),
                     it.value());
            QCOMPARE(engine.evaluate(QString::fromLatin1("(function() { return QT_TRID_NOOP('%0'); })()")
                                     .arg(it.key()), fn).toString(),
                     it.key());
        }
    }

    // Plural form
    QCOMPARE(engine.evaluate("qsTrId('qtn_bar_baz', 10)").toString(),
             QString::fromLatin1("10 fooish bar(s) found"));
    QCOMPARE(engine.evaluate("qsTrId('qtn_foo_bar', 10)").toString(),
             QString::fromLatin1("qtn_foo_bar")); // Doesn't have plural
}

// How to add a new test row:
// - Find a nice list of Unicode characters to choose from
// - Write source string/context/comment in .js using Unicode escape sequences (\uABCD)
// - Update corresponding .ts file (e.g. lupdate foo.js -ts foo.ts -codecfortr UTF-8)
// - Enter translation in Linguist
// - Update corresponding .qm file (e.g. lrelease foo.ts)
// - Evaluate script that performs translation; make sure the correct result is returned
//   (e.g. by setting the resulting string as the text of a QLabel and visually verifying
//   that it looks the same as what you entered in Linguist :-) )
// - Generate the expectedTranslation column data using toUtf8().toHex()
void tst_QJSEngine::translateScriptUnicode_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("expectedTranslation");

    QString fileName = QString::fromLatin1("translatable-unicode.js");
    QTest::newRow("qsTr('H\\u2082O')@translatable-unicode.js")
            << QString::fromLatin1("qsTr('H\\u2082O')") << fileName << QString::fromUtf8("\xcd\xbb\xcd\xbc\xcd\xbd");
    QTest::newRow("qsTranslate('\\u010C\\u0101\\u011F\\u0115', 'CO\\u2082')@translatable-unicode.js")
            << QString::fromLatin1("qsTranslate('\\u010C\\u0101\\u011F\\u0115', 'CO\\u2082')") << fileName << QString::fromUtf8("\xd7\x91\xd7\x9a\xd7\xa2");
    QTest::newRow("qsTr('\\u0391\\u0392\\u0393')@translatable-unicode.js")
            << QString::fromLatin1("qsTr('\\u0391\\u0392\\u0393')") << fileName << QString::fromUtf8("\xd3\x9c\xd2\xb4\xd1\xbc");
    QTest::newRow("qsTranslate('\\u010C\\u0101\\u011F\\u0115', '\\u0414\\u0415\\u0416')@translatable-unicode.js")
            << QString::fromLatin1("qsTranslate('\\u010C\\u0101\\u011F\\u0115', '\\u0414\\u0415\\u0416')") << fileName << QString::fromUtf8("\xd8\xae\xd8\xb3\xd8\xb3");
    QTest::newRow("qsTr('H\\u2082O', 'not the same H\\u2082O')@translatable-unicode.js")
            << QString::fromLatin1("qsTr('H\\u2082O', 'not the same H\\u2082O')") << fileName << QString::fromUtf8("\xd4\xb6\xd5\x8a\xd5\x92");
    QTest::newRow("qsTr('H\\u2082O')")
            << QString::fromLatin1("qsTr('H\\u2082O')") << QString() << QString::fromUtf8("\x48\xe2\x82\x82\x4f");
    QTest::newRow("qsTranslate('\\u010C\\u0101\\u011F\\u0115', 'CO\\u2082')")
            << QString::fromLatin1("qsTranslate('\\u010C\\u0101\\u011F\\u0115', 'CO\\u2082')") << QString() << QString::fromUtf8("\xd7\x91\xd7\x9a\xd7\xa2");
}

void tst_QJSEngine::translateScriptUnicode()
{
    QFETCH(QString, expression);
    QFETCH(QString, fileName);
    QFETCH(QString, expectedTranslation);

    QJSEngine engine;

    TranslationScope tranScope(":/translations/translatable-unicode");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QCOMPARE(engine.evaluate(expression, fileName).toString(), expectedTranslation);
}

void tst_QJSEngine::translateScriptUnicodeIdBased_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<QString>("expectedTranslation");

    QTest::newRow("qsTrId('\\u01F8\\u01D2\\u0199\\u01D0\\u01E1'')")
            << QString::fromLatin1("qsTrId('\\u01F8\\u01D2\\u0199\\u01D0\\u01E1')") << QString::fromUtf8("\xc6\xa7\xc6\xb0\xc6\x88\xc8\xbc\xc8\x9d\xc8\xbf\xc8\x99");
    QTest::newRow("qsTrId('\\u0191\\u01CE\\u0211\\u0229\\u019C\\u018E\\u019A\\u01D0')")
            << QString::fromLatin1("qsTrId('\\u0191\\u01CE\\u0211\\u0229\\u019C\\u018E\\u019A\\u01D0')") << QString::fromUtf8("\xc7\xa0\xc8\xa1\xc8\x8b\xc8\x85\xc8\x95");
    QTest::newRow("qsTrId('\\u0181\\u01A1\\u0213\\u018F\\u018C', 10)")
            << QString::fromLatin1("qsTrId('\\u0181\\u01A1\\u0213\\u018F\\u018C', 10)") << QString::fromUtf8("\x31\x30\x20\xc6\x92\xc6\xa1\xc7\x92\x28\xc8\x99\x29");
    QTest::newRow("qsTrId('\\u0181\\u01A1\\u0213\\u018F\\u018C')")
            << QString::fromLatin1("qsTrId('\\u0181\\u01A1\\u0213\\u018F\\u018C')") << QString::fromUtf8("\xc6\x91\xc6\xb0\xc7\xb9");
    QTest::newRow("qsTrId('\\u01CD\\u0180\\u01A8\\u0190\\u019E\\u01AB')")
            << QString::fromLatin1("qsTrId('\\u01CD\\u0180\\u01A8\\u0190\\u019E\\u01AB')") << QString::fromUtf8("\xc7\x8d\xc6\x80\xc6\xa8\xc6\x90\xc6\x9e\xc6\xab");
}

void tst_QJSEngine::translateScriptUnicodeIdBased()
{
    QFETCH(QString, expression);
    QFETCH(QString, expectedTranslation);

    QJSEngine engine;

    TranslationScope tranScope(":/translations/idtranslatable-unicode");
    engine.installExtensions(QJSEngine::TranslationExtension);

    QCOMPARE(engine.evaluate(expression).toString(), expectedTranslation);
}

void tst_QJSEngine::translateFromBuiltinCallback()
{
    QJSEngine eng;
    eng.installExtensions(QJSEngine::TranslationExtension);

    // Callback has no translation context.
    eng.evaluate("function foo() { qsTr('foo'); }");

    // Stack at translation time will be:
    // qsTr, foo, forEach, global
    // qsTr() needs to walk to the outer-most (global) frame before it finds
    // a translation context, and this should not crash.
    eng.evaluate("[10,20].forEach(foo)", "script.js");
}

void tst_QJSEngine::translationFilePath_data()
{
    QTest::addColumn<QString>("filename");

    QTest::newRow("relative") << QStringLiteral("script.js");
    QTest::newRow("absolute unix") << QStringLiteral("/script.js");
    QTest::newRow("absolute /windows/") << QStringLiteral("c:/script.js");
#ifdef Q_OS_WIN
    QTest::newRow("absolute \\windows\\") << QStringLiteral("c:\\script.js");
#endif
    QTest::newRow("relative url") << QStringLiteral("file://script.js");
    QTest::newRow("absolute url unix") << QStringLiteral("file:///script.js");
    QTest::newRow("absolute url windows") << QStringLiteral("file://c:/script.js");
}

class DummyTranslator : public QTranslator
{
    Q_OBJECT

public:
    DummyTranslator(const char *sourceText)
        : srcTxt(sourceText)
    {}

    QString translate(const char *context, const char *sourceText, const char *disambiguation, int n) const override
    {
        Q_UNUSED(disambiguation);
        Q_UNUSED(n);

        if (srcTxt == sourceText)
            ctxt = QByteArray(context);

        return QString(sourceText);
    }

    bool isEmpty() const override
    {
        return false;
    }

    const char *sourceText() const
    { return srcTxt.constData(); }

    QByteArray context() const
    { return ctxt; }

private:
    QByteArray srcTxt;
    mutable QByteArray ctxt;
};

void tst_QJSEngine::translationFilePath()
{
    QFETCH(QString, filename);

    DummyTranslator translator("some text");
    QCoreApplication::installTranslator(&translator);
    QByteArray scriptContent = QByteArray("qsTr('%1')").replace("%1", translator.sourceText());

    QJSEngine engine;
    engine.installExtensions(QJSEngine::TranslationExtension);
    QJSValue result = engine.evaluate(scriptContent, filename);
    QCOMPARE(translator.context(), QByteArray("script"));

    QCoreApplication::removeTranslator(&translator);
}

void tst_QJSEngine::translationFileName()
{
    const auto filename = QStringLiteral("multiple.dots.1.0.qml");

    DummyTranslator translator("some text");
    QCoreApplication::installTranslator(&translator);
    QByteArray scriptContent = QByteArray("qsTr('%1')").replace("%1", translator.sourceText());

    QJSEngine engine;
    engine.installExtensions(QJSEngine::TranslationExtension);
    QJSValue result = engine.evaluate(scriptContent, filename);
    QCOMPARE(translator.context(), QByteArray("multiple.dots.1.0"));

    QCoreApplication::removeTranslator(&translator);
}

void tst_QJSEngine::installConsoleFunctions()
{
    QJSEngine engine;
    QJSValue global = engine.globalObject();
    QVERIFY(global.property("console").isUndefined());
    QVERIFY(global.property("print").isUndefined());

    engine.installExtensions(QJSEngine::ConsoleExtension);
    QVERIFY(global.property("console").isObject());
    QVERIFY(global.property("print").isCallable());
}

void tst_QJSEngine::logging()
{
    QLoggingCategory loggingCategory("js");
    QVERIFY(loggingCategory.isDebugEnabled());
    QVERIFY(loggingCategory.isWarningEnabled());
    QVERIFY(loggingCategory.isCriticalEnabled());

    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    QTest::ignoreMessage(QtDebugMsg, "console.debug");
    engine.evaluate("console.debug('console.debug')");
    QTest::ignoreMessage(QtDebugMsg, "console.log");
    engine.evaluate("console.log('console.log')");
    QTest::ignoreMessage(QtInfoMsg, "console.info");
    engine.evaluate("console.info('console.info')");
    QTest::ignoreMessage(QtWarningMsg, "console.warn");
    engine.evaluate("console.warn('console.warn')");
    QTest::ignoreMessage(QtCriticalMsg, "console.error");
    engine.evaluate("console.error('console.error')");

    QTest::ignoreMessage(QtDebugMsg, ": 1");
    engine.evaluate("console.count()");

    QTest::ignoreMessage(QtDebugMsg, ": 2");
    engine.evaluate("console.count()");
}

void tst_QJSEngine::tracing()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    QTest::ignoreMessage(QtDebugMsg, "%entry (:1)");
    engine.evaluate("console.trace()");

    QTest::ignoreMessage(QtDebugMsg, "a (:1)\nb (:1)\nc (:1)\n%entry (:1)");
    engine.evaluate("function a() { console.trace(); } function b() { a(); } function c() { b(); }");
    engine.evaluate("c()");

    QQmlTestMessageHandler messageHandler;
    messageHandler.setIncludeCategoriesEnabled(true);
    engine.evaluate("c()");
    QCOMPARE(messageHandler.messageString(),
             QLatin1String("js: a (:1)\nb (:1)\nc (:1)\n%entry (:1)"));
}

void tst_QJSEngine::asserts()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    QTest::ignoreMessage(QtCriticalMsg, "This will fail\n%entry (:1)");
    engine.evaluate("console.assert(0, 'This will fail')");

    QTest::ignoreMessage(QtCriticalMsg, "This will fail too\n%entry (:1)");
    engine.evaluate("console.assert(1 > 2, 'This will fail too')");
}

void tst_QJSEngine::exceptions()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    QTest::ignoreMessage(QtCriticalMsg, "Exception 1\n%entry (:1)");
    engine.evaluate("console.exception('Exception 1')");
}

void tst_QJSEngine::exceptionReporting()
{
    QJSEngine engine;
    QStringList stackTrace;
    QJSValue result = engine.evaluate(R"(
    function f() {throw 'an exception'}
    function g() {f()}
    g() )", QString("tesfile.js"), 1, &stackTrace);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QCOMPARE(stackTrace.size(), 3);
    QCOMPARE(stackTrace.at(0), "f:2:-1:file:tesfile.js");
    QCOMPARE(stackTrace.at(1), "g:3:-1:file:tesfile.js");
    QCOMPARE(stackTrace.at(2), "%entry:4:-1:file:tesfile.js");

    result = engine.evaluate("42", QString(), 1, &stackTrace);
    QVERIFY2(!result.isError(), qPrintable(result.toString()));
    QVERIFY(stackTrace.isEmpty());
}

void tst_QJSEngine::installGarbageCollectionFunctions()
{
    QJSEngine engine;
    QJSValue global = engine.globalObject();
    QVERIFY(global.property("gc").isUndefined());

    engine.installExtensions(QJSEngine::GarbageCollectionExtension);
    QVERIFY(global.property("gc").isCallable());
}

void tst_QJSEngine::installAllExtensions()
{
    QJSEngine engine;
    QJSValue global = engine.globalObject();
    // Pick out a few properties from each extension and check that they're there.
    QVERIFY(global.property("qsTranslate").isUndefined());
    QVERIFY(global.property("console").isUndefined());
    QVERIFY(global.property("print").isUndefined());
    QVERIFY(global.property("gc").isUndefined());

    engine.installExtensions(QJSEngine::AllExtensions);
    QVERIFY(global.property("qsTranslate").isCallable());
    QVERIFY(global.property("console").isObject());
    QVERIFY(global.property("print").isCallable());
    QVERIFY(global.property("gc").isCallable());
}

class ObjectWithPrivateMethods : public QObject
{
    Q_OBJECT
private slots:
    void myPrivateMethod() {}
};

void tst_QJSEngine::privateMethods()
{
    ObjectWithPrivateMethods object;
    QJSEngine engine;
    QJSValue jsWrapper = engine.newQObject(&object);
    QQmlEngine::setObjectOwnership(&object, QQmlEngine::CppOwnership);

    QSet<QString> privateMethods;
    {
        const QMetaObject *mo = object.metaObject();
        for (int i = 0; i < mo->methodCount(); ++i) {
            const QMetaMethod method = mo->method(i);
            if (method.access() == QMetaMethod::Private)
                privateMethods << QString::fromUtf8(method.name());
        }
    }

    QVERIFY(privateMethods.contains("myPrivateMethod"));
    privateMethods << QStringLiteral("deleteLater") << QStringLiteral("destroyed");

    QJSValueIterator it(jsWrapper);
    while (it.hasNext()) {
        it.next();
        QVERIFY(!privateMethods.contains(it.name()));
    }
}

void tst_QJSEngine::engineForObject()
{
    QObject object;
    {
        QJSEngine engine;
        QVERIFY(!qjsEngine(&object));
        QJSValue wrapper = engine.newQObject(&object);
        QQmlEngine::setObjectOwnership(&object, QQmlEngine::CppOwnership);
        QVERIFY(qjsEngine(&object));
    }
    QVERIFY(!qjsEngine(&object));
}

void tst_QJSEngine::intConversion_QTBUG43309()
{
    // This failed in the interpreter:
    QJSEngine engine;
    QString jsCode = "var n = 0.1; var m = (n*255) | 0; m";
    QJSValue result = engine.evaluate( jsCode );
    QVERIFY(result.isNumber());
    QCOMPARE(result.toNumber(), 25.0);
}

#ifdef QT_DEPRECATED
// QTBUG-44039 and QTBUG-43885:
void tst_QJSEngine::toFixed()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate(QStringLiteral("(12.5).toFixed()"));
    QVERIFY(result.isString());
    QCOMPARE(result.toString(), QStringLiteral("13"));
    result = engine.evaluate(QStringLiteral("(12.05).toFixed(1)"));
    QVERIFY(result.isString());
    QCOMPARE(result.toString(), QStringLiteral("12.1"));
}
#endif

void tst_QJSEngine::argumentEvaluationOrder()
{
    QJSEngine engine;
    QJSValue ok = engine.evaluate(
            "function record(arg1, arg2) {\n"
            "    parameters = [arg1, arg2]\n"
            "}\n"
            "function test() {\n"
            "    var i = 2;\n"
            "    record(i, i += 2);\n"
            "}\n"
            "test()\n"
            "parameters[0] == 2 && parameters[1] == 4");
    qDebug() << ok.toString();
    QVERIFY(ok.isBool());
    QVERIFY(ok.toBool());

}

class TestObject : public QObject
{
    Q_OBJECT
public:
    TestObject() {}

    bool called = false;

    Q_INVOKABLE void callMe(QQmlV4FunctionPtr) {
        called = true;
    }
};

void tst_QJSEngine::v4FunctionWithoutQML()
{
    TestObject obj;
    QJSEngine engine;
    QJSValue wrapper = engine.newQObject(&obj);
    QQmlEngine::setObjectOwnership(&obj, QQmlEngine::CppOwnership);
    QVERIFY(!obj.called);
    wrapper.property("callMe").call();
    QVERIFY(obj.called);
}

void tst_QJSEngine::withNoContext()
{
    // Don't crash (QTBUG-53794)
    QJSEngine engine;
    engine.evaluate("with (noContext) true");
}

void tst_QJSEngine::holeInPropertyData()
{
    QJSEngine engine;
    QJSValue ok = engine.evaluate(
                "var o = {};\n"
                "o.bar = 0xcccccccc;\n"
                "o.foo = 0x55555555;\n"
                "Object.defineProperty(o, 'bar', { get: function() { return 0xffffffff }});\n"
                "o.bar === 0xffffffff && o.foo === 0x55555555;");
    QVERIFY(ok.isBool());
    QVERIFY(ok.toBool());
}

void tst_QJSEngine::basicBlockMergeAfterLoopPeeling()
{
    QJSEngine engine;
    QJSValue ok = engine.evaluate(
    "function crashMe() {\n"
    "    var seen = false;\n"
    "    while (globalVar) {\n"
    "        if (seen)\n"
    "            return;\n"
    "        seen = true;\n"
    "    }\n"
    "}\n");
    QVERIFY(!ok.isCallable());

}

void tst_QJSEngine::modulusCrash()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate(
    "var a = -2147483648; var b = -1; var c = a % b; c;"
    );
    QVERIFY(result.isNumber() && result.toNumber() == 0.);
}

void tst_QJSEngine::malformedExpression()
{
    QJSEngine engine;
    engine.evaluate("5%55555&&5555555\n7-0");
}

void tst_QJSEngine::scriptScopes()
{
    QJSEngine engine;

    QJSValue def = engine.evaluate("'use strict'; function foo() { return 42 }");
    QVERIFY(!def.isError());
    QJSValue globalObject = engine.globalObject();
    QJSValue foo = globalObject.property("foo");
    QVERIFY(foo.isObject());
    QVERIFY(foo.isCallable());

    QJSValue use = engine.evaluate("'use strict'; foo()");
    QVERIFY(use.isNumber());
    QCOMPARE(use.toInt(), 42);
}

void tst_QJSEngine::binaryNumbers()
{
    QJSEngine engine;

    QJSValue result = engine.evaluate("0b1001");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == 9);

    result = engine.evaluate("0B1001");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == 9);

    result = engine.evaluate("0b2");
    QVERIFY(result.isError());
}

void tst_QJSEngine::octalNumbers()
{
    QJSEngine engine;

    QJSValue result = engine.evaluate("0o11");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == 9);

    result = engine.evaluate("0O11");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == 9);

    result = engine.evaluate("0o9");
    QVERIFY(result.isError());
}

void tst_QJSEngine::incrementAfterNewline()
{
    QJSEngine engine;

    QJSValue result = engine.evaluate("var x = 0; if (\n++x) x; else -x;");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == 1);

    result = engine.evaluate("var x = 0; if (\n--x) x; else -x;");
    QVERIFY(result.isNumber());
    QVERIFY(result.toNumber() == -1);
}

void tst_QJSEngine::deleteInsideForIn()
{
    QJSEngine engine;

    QJSValue iterationCount = engine.evaluate(
                              "var o = { a: 1, b: 2, c: 3, d: 4};\n"
                              "var count = 0;\n"
                              "for (var prop in o) { count++; delete o[prop]; }\n"
                              "count");
    QVERIFY(iterationCount.isNumber());
    QCOMPARE(iterationCount.toInt(), 4);
}

void tst_QJSEngine::functionToString_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("expectedString");

    QTest::newRow("named function") << QString::fromLatin1("function f() {}; f.toString()")
        << QString::fromLatin1("function f() { [native code] }");
    QTest::newRow("anonymous function") << QString::fromLatin1("(function() {}).toString()")
        << QString::fromLatin1("function() { [native code] }");
}

// Tests that function.toString() prints the function's name.
void tst_QJSEngine::functionToString()
{
    QFETCH(QString, source);
    QFETCH(QString, expectedString);

    QJSEngine engine;
    engine.installExtensions(QJSEngine::AllExtensions);
    QJSValue evaluationResult = engine.evaluate(source);
    QVERIFY(!evaluationResult.isError());
    QCOMPARE(evaluationResult.toString(), expectedString);
}

void tst_QJSEngine::stringReplace()
{
    QJSEngine engine;

    QJSValue val = engine.evaluate("'x'.replace('x', '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$1"));

    val = engine.evaluate("'x'.replace('x', '$10')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$10"));

    val = engine.evaluate("'x'.replace('x', '$01')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$01"));

    val = engine.evaluate("'x'.replace('x', '$0')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$0"));

    val = engine.evaluate("'x'.replace('x', '$00')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$00"));

    val = engine.evaluate("'x'.replace(/(x)/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("x"));

    val = engine.evaluate("'x'.replace(/(x)/, '$01')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("x"));

    val = engine.evaluate("'x'.replace(/(x)/, '$2')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$2"));

    val = engine.evaluate("'x'.replace(/(x)/, '$02')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$02"));

    val = engine.evaluate("'x'.replace(/(x)/, '$0')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$0"));

    val = engine.evaluate("'x'.replace(/(x)/, '$00')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("$00"));

    val = engine.evaluate("'x'.replace(/()()()()()()()()()(x)/, '$11')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("1"));

    val = engine.evaluate("'x'.replace(/()()()()()()()()()(x)/, '$10')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("x"));

    val = engine.evaluate("'123'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123"));

    val = engine.evaluate("'123.00'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123"));

    val = engine.evaluate("'123.0'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123"));

    val = engine.evaluate("'123.'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123"));

    val = engine.evaluate("'123.50'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123.5"));

    val = engine.evaluate("'123.05'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("123.05"));

    val = engine.evaluate("'0.050'.replace(/\\.0*$|(\\.\\d*[1-9])(0+)$/, '$1')");
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString("0.05"));
}

void tst_QJSEngine::protoChanges_QTBUG68369()
{
    QJSEngine engine;
    QJSValue ok = engine.evaluate(
    "var o = { x: true };"
    "var p1 = {};"
    "var p2 = {};"
    "o.__proto__ = p1;"
    "o.__proto__ = p2;"
    "o.__proto__ = p1;"
    "p1.y = true;"
    "o.y"
    );
    QVERIFY(ok.toBool() == true);
}

void tst_QJSEngine::multilineStrings()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate(
    "var x = `a\nb`; x;"
    );
    QVERIFY(result.isString());
    QVERIFY(result.toString() == QStringLiteral("a\nb"));

}

void tst_QJSEngine::throwError()
{
    QJSEngine engine;
    QJSValue wrappedThis = engine.newQObject(this);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine.globalObject().setProperty("testCase", wrappedThis);

    QJSValue result = engine.evaluate(
                "function test(){\n"
                "try {\n"
                "    return testCase.throwingCppMethod1();\n"
                "} catch (error) {\n"
                "    return error;\n"
                "}\n"
                "return \"not reached!\";\n"
                "}\n"
                "test();"
    );
    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::GenericError);
    QCOMPARE(result.property("lineNumber").toString(), "3");
    QCOMPARE(result.property("message").toString(), "blub");
    QVERIFY(!result.property("stack").isUndefined());
}

void tst_QJSEngine::throwErrorObject()
{
    QJSEngine engine;
    QJSValue wrappedThis = engine.newQObject(this);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine.globalObject().setProperty("testCase", wrappedThis);

    QJSValue result = engine.evaluate(
                "function test(){\n"
                "    try {\n"
                "        testCase.throwingCppMethod2();\n"
                "    } catch (error) {\n"
                "        if (error instanceof TypeError) {\n"
                "            return error;\n"
                "        }\n"
                "    }\n"
                "    return null;\n"
                "}\n"
                "test();"
    );
    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::TypeError);
    QCOMPARE(result.property("lineNumber").toString(), "3");
    QCOMPARE(result.property("message").toString(), "Wrong type");
    QVERIFY(!result.property("stack").isUndefined());
}

void tst_QJSEngine::returnError()
{
    QJSEngine engine;
    QJSValue wrappedThis = engine.newQObject(this);
    QQmlEngine::setObjectOwnership(this, QQmlEngine::CppOwnership);
    engine.globalObject().setProperty("testCase", wrappedThis);

    QJSValue result = engine.evaluate("testCase.throwingCppMethod3()");
    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::EvalError);
    QCOMPARE(result.property("lineNumber").toString(), "1");
    QCOMPARE(result.property("message").toString(), "Something is wrong");
    QVERIFY(!result.property("stack").isUndefined());
}

void tst_QJSEngine::catchError()
{
    QJSEngine engine;
    QVERIFY(!engine.hasError());
    engine.throwError(QJSValue::GenericError, "some error");
    QVERIFY(engine.hasError());
    const QJSValue error = engine.catchError();
    QVERIFY(error.isError());
    QCOMPARE(error.errorType(), QJSValue::GenericError);
    QCOMPARE(error.property("message").toString(), "some error");
    QVERIFY(!engine.hasError());
}

QJSValue tst_QJSEngine::throwingCppMethod1()
{
    qjsEngine(this)->throwError(QStringLiteral("blub"));
    return QJSValue(47);
}

void tst_QJSEngine::throwingCppMethod2()
{
    qjsEngine(this)->throwError(QJSValue::TypeError, "Wrong type");
}

QJSValue tst_QJSEngine::throwingCppMethod3()
{
    QJSEngine *engine = qjsEngine(this);
    engine->throwError(engine->newErrorObject(QJSValue::EvalError, "Something is wrong"));
    return QJSValue(31);
}

void tst_QJSEngine::mathMinMax()
{
    QJSEngine engine;

    QJSValue result = engine.evaluate("var a = .5; Math.min(1, 2, 3.5 + a, '5')");
    QCOMPARE(result.toNumber(), 1.0);
    QVERIFY(QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&result)).isInteger());

    result = engine.evaluate("var a = .5; Math.max('0', 1, 2, 3.5 + a)");
    QCOMPARE(result.toNumber(), 4.0);
    QVERIFY(QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&result)).isInteger());
}

void tst_QJSEngine::mathNegativeZero()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate("var a = 0; Object.is(-1*a, -0)");
    QVERIFY(result.isBool());
    QVERIFY(result.toBool());

    result = engine.evaluate("var a = 0; Object.is(1*a, 0)");
    QVERIFY(result.isBool());
    QVERIFY(result.toBool());
}

void tst_QJSEngine::importModule()
{
    // This is just a basic test for the API. Primary test coverage is via the ES test suite.
    QJSEngine engine;
    QJSValue ns = engine.importModule(QStringLiteral(":/testmodule.mjs"));
    QCOMPARE(ns.property("value").toInt(), 42);
    ns.property("sideEffect").call();

    // Make sure that importing a second time will return the same instance.
    QJSValue secondNamespace = engine.importModule(QStringLiteral(":/testmodule.mjs"));
    QCOMPARE(secondNamespace.property("value").toInt(), 43);
}

void tst_QJSEngine::importModuleRelative()
{
    const QString oldWorkingDirectory = QDir::currentPath();
    auto workingDirectoryGuard = qScopeGuard([oldWorkingDirectory]{
        QDir::setCurrent(oldWorkingDirectory);
    });

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir::setCurrent(tempDir.path());

    {
        QFile f(QStringLiteral("relativemodule.mjs"));
        QVERIFY(f.open(QIODevice::WriteOnly|QIODevice::Truncate));
        f.write(QByteArrayLiteral("var value = 100; export { value }; export function change() { value = value + 1 }"));
    }

    QJSEngine engine;

    {
        QJSValue module = engine.importModule(QStringLiteral("relativemodule.mjs"));
        QVERIFY2(!module.isError(), qPrintable(module.toString()));
        QCOMPARE(module.property("value").toInt(), 100);

        module.property("change").call();
    }

    {
        QJSValue sameModule = engine.importModule(tempDir.filePath(QStringLiteral("relativemodule.mjs")));
        QVERIFY2(!sameModule.isError(), qPrintable(sameModule.toString()));
        QCOMPARE(sameModule.property("value").toInt(), 101);
    }
}

void tst_QJSEngine::importModuleWithLexicallyScopedVars()
{
    QJSEngine engine;
    QJSValue ns = engine.importModule(QStringLiteral(":/modulewithlexicals.mjs"));
    QVERIFY2(!ns.isError(), qPrintable(ns.toString()));
    QCOMPARE(ns.property("main").call().toInt(), 10);
}

void tst_QJSEngine::importExportErrors()
{
    {
        QJSEngine engine;
        QJSValue result = engine.importModule(QStringLiteral(":/importerror1.mjs"));
        QVERIFY(result.isError());
        QCOMPARE(result.property("lineNumber").toInt(), 2);
    }
    {
        QJSEngine engine;
        QJSValue result = engine.importModule(QStringLiteral(":/exporterror1.mjs"));
        QVERIFY(result.isError());
        QCOMPARE(result.property("lineNumber").toInt(), 2);
    }
}

void tst_QJSEngine::registerModule()
{
    QJSEngine engine;
    QJSValue magic(63);
    QJSValue name("Qt6");
    QJSValue version("6.1.3");
    QJSValue obj = engine.newObject();
    bool ret = false;

    obj.setProperty("name", name);
    obj.setProperty("version", version);

    ret = engine.registerModule("magic", magic);
    QVERIFY2(ret, "Error registering magic");
    ret = engine.registerModule("qt_info", obj);
    QVERIFY2(ret, "Error registering qt_info");
    QJSValue result = engine.importModule(QStringLiteral(":/testregister.mjs"));
    QVERIFY2(!result.isError(), qPrintable(result.toString()));

    QJSValue nameVal = result.property("getName").call();
    QJSValue magicVal = result.property("getMagic").call();
    QCOMPARE(nameVal.toString(), QLatin1String("Qt6"));
    QCOMPARE(magicVal.toInt(), 63);

    // Verify that "name" doesn't change in JS even if the object is changed.
    QJSValue replacement("Bad");
    obj.setProperty("name", replacement);
    QJSValue newNameVal = result.property("getName").call();
    QCOMPARE(nameVal.toString(), "Qt6");
}

class TestRegisterObject : public QObject
{
    Q_OBJECT
public:
    TestRegisterObject() {}

    Q_INVOKABLE int add(int a, int b) {
        return a + b;
    }
};

void tst_QJSEngine::registerModuleQObject()
{
    QJSEngine engine;
    TestRegisterObject obj;
    QJSValue wrapper = engine.newQObject(&obj);
    auto args = QJSValueList() << 1 << 2;

    bool ret = engine.registerModule("math", wrapper);
    QVERIFY(ret);

    QJSValue result = engine.importModule(QStringLiteral(":/testregister2.mjs"));
    QVERIFY(!result.isError());

    QJSValue value = result.property("addAndDouble").call(args);
    QCOMPARE(value.toInt(), 6);
}

void tst_QJSEngine::registerModuleNamedError() {
    QJSEngine engine;
    QJSValue notanobject(666);

    bool ret = engine.registerModule("notanobject", notanobject);
    QVERIFY(ret);

    QJSValue result = engine.importModule(QStringLiteral(":/testregister3.mjs"));
    QCOMPARE(result.toString(), QString("ReferenceError: Unable to resolve import reference subval"));
}

void tst_QJSEngine::equality()
{
    QJSEngine engine;
    QJSValue ok = engine.evaluate("(0 < 0) ? 'ko' : 'ok'");
    QVERIFY(ok.isString());
    QCOMPARE(ok.toString(), QString("ok"));
}

void tst_QJSEngine::aggressiveGc()
{
    const QByteArray origAggressiveGc = qgetenv("QV4_MM_AGGRESSIVE_GC");
    qputenv("QV4_MM_AGGRESSIVE_GC", "true");
    {
        QJSEngine engine; // ctor crashes if core allocation methods don't properly scope things.
        QJSValue obj = engine.newObject();
        QVERIFY(obj.isObject());
    }
    qputenv("QV4_MM_AGGRESSIVE_GC", origAggressiveGc);
}

void tst_QJSEngine::noAccumulatorInTemplateLiteral()
{
    // Use aggressive GC to increase our chances of triggering the problem.
    const QByteArray origAggressiveGc = qgetenv("QV4_MM_AGGRESSIVE_GC");
    qputenv("QV4_MM_AGGRESSIVE_GC", "true");

    QJSEngine engine;
    const int maxCallDepth = QV4::ExecutionEngine::maxCallDepth();

    const auto guard = qScopeGuard([&]() {
        QV4::ExecutionEngine::setMaxCallDepth(maxCallDepth);
        qputenv("QV4_MM_AGGRESSIVE_GC", origAggressiveGc);
    });

    // Since it takes too long to get a real stack overflow with the function below,
    // let's switch to call depth tracking.
    QV4::ExecutionEngine::setMaxCallDepth(64);
    engine.handle()->callDepth = 0;

    // getTemplateLiteral should not save the accumulator as it's garbage and trashes
    // the next GC run. Instead, we want to see the stack overflow error.
    QJSValue value = engine.evaluate("function a(){\nS=o=>s\nFunction``\na()}a()");

    QVERIFY(value.isError());
    QCOMPARE(value.toString(), "RangeError: Maximum call stack size exceeded.");
}

void tst_QJSEngine::interrupt_data()
{
    QTest::addColumn<int>("jitThreshold");
    QTest::addColumn<QString>("code");

    const int big = (1 << 24);
    for (int i = 0; i <= big; i += big) {
        const char *mode = i ? "interpret" : "jit";
        QTest::addRow("for with content / %s", mode)   << i << "var a = 0; for (;;) { a += 2; }";
        QTest::addRow("for empty / %s", mode)          << i << "for (;;) {}";
        QTest::addRow("for continue / %s", mode)       << i << "for (;;) { continue; }";
        QTest::addRow("while with content / %s", mode) << i << "var a = 0; while (true) { a += 2; }";
        QTest::addRow("while empty / %s", mode)        << i << "while (true) {}";
        QTest::addRow("while continue / %s", mode)     << i << "while (true) { continue; }";
        QTest::addRow("do with content / %s", mode)    << i << "var a = 0; do { a += 2; } while (true);";
        QTest::addRow("do empty / %s", mode)           << i << "do {} while (true);";
        QTest::addRow("do continue / %s", mode)        << i << "do { continue; } while (true);";
        QTest::addRow("nested loops / %s", mode)       << i << "while (true) { for (;;) {} }";
        QTest::addRow("labeled continue / %s", mode)   << i << "a: while (true) { for (;;) { continue a; } }";
        QTest::addRow("labeled break / %s", mode)      << i << "while (true) { a: for (;;) { break a; } }";
        QTest::addRow("tail call / %s", mode)          << i << "'use strict';\nfunction x() { return x(); }; x();";
        QTest::addRow("huge array join / %s", mode)    << i << "Array(1E9)|1";
    }
}

class TemporaryJitThreshold
{
    Q_DISABLE_COPY_MOVE(TemporaryJitThreshold)
public:
    TemporaryJitThreshold(int threshold) {
        m_wasSet = qEnvironmentVariableIsSet(m_envVar);
        m_value = qgetenv(m_envVar);
        qputenv(m_envVar, QByteArray::number(threshold));
    }

    ~TemporaryJitThreshold()
    {
        if (m_wasSet)
            qputenv(m_envVar, m_value);
        else
            qunsetenv(m_envVar);
    }

private:
    const char *m_envVar = "QV4_JIT_CALL_THRESHOLD";
    bool m_wasSet = false;
    QByteArray m_value;
};

void tst_QJSEngine::interrupt()
{
#if QT_CONFIG(cxx11_future)
    QFETCH(int, jitThreshold);
    QFETCH(QString, code);

    TemporaryJitThreshold threshold(jitThreshold);
    Q_UNUSED(threshold);

    QJSEngine *engineInThread = nullptr;
    QScopedPointer<QThread> worker(QThread::create([&engineInThread, &code](){
        QJSEngine jsEngine;
        engineInThread = &jsEngine;
        QJSValue result = jsEngine.evaluate(code);
        QVERIFY(jsEngine.isInterrupted());
        QVERIFY(result.isError());
        QCOMPARE(result.toString(), QString::fromLatin1("Error: Interrupted"));
        engineInThread = nullptr;
    }));
    worker->start();

    QTRY_VERIFY(engineInThread);

    engineInThread->setInterrupted(true);

    QVERIFY(worker->wait());
    QVERIFY(!engineInThread);
#else
    QSKIP("This test requires C++11 futures");
#endif
}

void tst_QJSEngine::triggerBackwardJumpWithDestructuring()
{
    QJSEngine engine;
    auto value = engine.evaluate(
            "function makeArray(n) { return [...Array(n).keys()]; }\n"
            "for (let i=0;i<100;++i) {\n"
            "    let arr = makeArray(20)\n"
            "    arr.sort( (a, b) => b - a )\n"
            "}"
            );
    QVERIFY(!value.isError());
}

void tst_QJSEngine::arrayConcatOnSparseArray()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::GarbageCollectionExtension);
    const auto value = engine.evaluate(
            "(function() {\n"
            "   const v4 = [1,2,3];\n"
            "   const v7 = [4,5];\n"
            "   v7.length = 1337;\n"
            "   const v9 = v4.concat(v7);\n"
            "   gc();\n"
            "   return v9;\n"
            "})();");
    QCOMPARE(value.property("length").toInt(), 1340);
    for (int i = 0; i < 5; ++i)
        QCOMPARE(value.property(i).toInt(), i + 1);
    for (int i = 5; i < 1340; ++i)
        QVERIFY(value.property(i).isUndefined());
}

void tst_QJSEngine::concatAfterUnshift()
{
    QJSEngine engine;
    const auto value = engine.evaluate(uR"(
            (function() {
            let test = ['val2']
            test.unshift('val1')
            test = test.concat([])
            return test
            })()
    )"_s);
    QVERIFY2(!value.isError(), qPrintable(value.toString()));
    QVERIFY(value.isArray());
    QCOMPARE(value.property(0).toString(), u"val1"_s);
    QCOMPARE(value.property(1).toString(), u"val2"_s);
}

void tst_QJSEngine::sortSparseArray()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);
    const auto value = engine.evaluate(
            "(function() {\n"
            "   var sparse = [0];\n"
            "   sparse = Object.defineProperty(sparse, \"10\", "
            "           {get: ()=>{return 2}, set: ()=>{return 2}} );\n"
            "   return Array.prototype.sort.call(sparse, ()=>{});\n"
            "})();");

    QCOMPARE(value.property("length").toInt(), 11);
    QVERIFY(value.property(0).isNumber());
    QCOMPARE(value.property(0).toInt(), 0);
    QVERIFY(value.property(1).isNumber());
    QCOMPARE(value.property(1).toInt(), 2);
    QVERIFY(value.property(10).isUndefined());
}

void tst_QJSEngine::compileBrokenRegexp()
{
    QJSEngine engine;
    const auto value = engine.evaluate(
        "(function() {"
        "var ret = new RegExp(Array(4097).join("
        "       String.fromCharCode(58)) + Array(4097).join(String.fromCharCode(480)) "
        "       + Array(65537).join(String.fromCharCode(5307)));"
        "return RegExp.prototype.compile.call(ret, 'a','b');"
        "})();"
    );

    QVERIFY(value.isError());
    QCOMPARE(value.toString(), "SyntaxError: Invalid flags supplied to RegExp constructor");
}

void tst_QJSEngine::tostringRecursionCheck()
{
    QJSEngine engine;
    auto value = engine.evaluate(R"js(
    var a = {};
    var b = new Array(1337);
    function main() {
        var ret = a.toLocaleString;
        b[1] = ret;
        Array = {};
        Object.toString = b[1];
        var ret = String.prototype.lastIndexOf.call({}, b[1]);
        var ret = String.prototype.charAt.call(Function, Object);
    }
    main();
    )js");
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), QLatin1String("RangeError: Maximum call stack size exceeded."));
}

void tst_QJSEngine::arrayIncludesWithLargeArray()
{
    QJSEngine engine;
    auto value = engine.evaluate(R"js(
        let arr = new Array(10000000)
        arr.includes(42)
    )js");
    QVERIFY(value.isBool());
    QCOMPARE(value.toBool(), false);
}

void tst_QJSEngine::printCircularArray()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);
    QTest::ignoreMessage(QtMsgType::QtDebugMsg, "[[Circular]]");
    auto value = engine.evaluate(R"js(
    let v1 = []
    v1.push(v1)
    console.log(v1)
    )js");
}

void tst_QJSEngine::sortNonStringArray()
{
    QJSEngine engine;
    const auto value = engine.evaluate(
        "const v4 = [Symbol.iterator, 1];"
        "const v5 = v4.sort();"
    );
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), "TypeError: Cannot convert a symbol to a string.");
}

void tst_QJSEngine::iterateInvalidProxy()
{
    QJSEngine engine;
    const auto value = engine.evaluate(
        "const v1 = new Proxy(Reflect, Reflect);"
        "for (const v2 in v1) {}"
        "const v3 = { getOwnPropertyDescriptor: eval, getPrototypeOf: eval };"
        "const v4 = new Proxy(v3, v3);"
        "for (const v5 in v4) {}"
    );
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), "TypeError: Type error");
}

void tst_QJSEngine::applyOnHugeArray()
{
    QJSEngine engine;
    const auto value = engine.evaluate(
        "var a = new Array(10);"
        "a[536870912] = Function;"
        "Function.apply('aaaaaaaa', a);"
    );
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), "RangeError: Array too large for apply().");
}


void tst_QJSEngine::reflectApplyOnHugeArray()
{
    QQmlEngine engine;
    const QJSValue value = engine.evaluate(R"(
(function(){
const v1 = [];
const v3 = [];
v3.length = 3900000000;
Reflect.apply(v1.reverse,v1,v3);
})()
    )");
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), QLatin1String("RangeError: Invalid array length."));
}

void tst_QJSEngine::jsonStringifyHugeArray()
{
    QQmlEngine engine;
    const QJSValue value = engine.evaluate(R"(
(function(){
const v3 = [];
v3.length = 3900000000;
JSON.stringify([], v3);
})()
    )");
    QVERIFY(value.isError());
    QCOMPARE(value.toString(), QLatin1String("RangeError: Invalid array length."));
}

void tst_QJSEngine::typedArraySet()
{
    QJSEngine engine;
    const auto value = engine.evaluate(
        "(function() {"
        "   var length = 0xfffffe0;"
        "   var offset = 0xfffffff0;"
        "   var e1;"
        "   var e2;"
        "   try {"
        "       var source1 = new Int8Array(length);"
        "       var target1 = new Int8Array(length);"
        "       target1.set(source1, offset);"
        "   } catch (intError) {"
        "       e1 = intError;"
        "   }"
        "   try {"
        "       var source2 = new Array(length);"
        "       var target2 = new Int8Array(length);"
        "       target2.set(source2, offset);"
        "   } catch (arrayError) {"
        "       e2 = arrayError;"
        "   }"
        "   return [e1, e2];"
        "})();"
    );

    QVERIFY(value.isArray());
    for (int i = 0; i < 2; ++i) {
        const auto error = value.property(i);
        QVERIFY(error.isError());
        QCOMPARE(error.toString(), "RangeError: TypedArray.set: out of range");
    }
}

void tst_QJSEngine::dataViewCtor()
{
    QJSEngine engine;
    const auto error = engine.evaluate(R"(
    (function() { try {
        var buf = new ArrayBuffer(0x200);
        var vuln = new DataView(buf, 8, 0xfffffff8);
    } catch (e) {
        return e;
    }})()
    )");
    QVERIFY(error.isError());
    QCOMPARE(error.toString(), "RangeError: DataView: constructor arguments out of range");
}

void tst_QJSEngine::uiLanguage()
{
    {
        QJSEngine engine;

        QVERIFY(!engine.globalObject().hasProperty("Qt"));

        engine.installExtensions(QJSEngine::TranslationExtension);
        QVERIFY(engine.globalObject().hasProperty("Qt"));
        QVERIFY(engine.globalObject().property("Qt").hasProperty("uiLanguage"));

        engine.setUiLanguage("Blah");
        QCOMPARE(engine.globalObject().property("Qt").property("uiLanguage").toString(), "Blah");

        engine.evaluate("Qt.uiLanguage = \"another\"");
        QCOMPARE(engine.globalObject().property("Qt").property("uiLanguage").toString(), "another");
    }

    {
        QQmlEngine qmlEngine;
        QVERIFY(qmlEngine.globalObject().hasProperty("Qt"));
        QVERIFY(qmlEngine.globalObject().property("Qt").hasProperty("uiLanguage"));
        qmlEngine.setUiLanguage("Blah");
        QCOMPARE(qmlEngine.globalObject().property("Qt").property("uiLanguage").toString(), "Blah");
    }
}

void tst_QJSEngine::urlObject()
{
    QJSEngine engine;

    const QString href = QStringLiteral(
                "http://uuu:ppp@example.com:777/foo/bar?search=stuff&other=where#hhh");
    const QUrl url(href);

    QJSManagedValue v(engine.evaluate(QStringLiteral("new URL('%1')").arg(href)), &engine);
    QVERIFY(v.isObject());
    QJSManagedValue proto(v.prototype());

    auto check = [&](const QString &prop, const QString &expected) {
        QCOMPARE(v.property(prop).toString(), expected);
        QVERIFY(proto.property(prop).isUndefined());
        QVERIFY(engine.hasError());
        QCOMPARE(engine.catchError().toString(),
                 QStringLiteral("TypeError: Value of \"this\" must be of type URL"));
    };

    check(QStringLiteral("href"), url.toString());
    check(QStringLiteral("origin"), QStringLiteral("http://example.com:777"));
    check(QStringLiteral("protocol"), url.scheme() + QLatin1Char(':'));
    check(QStringLiteral("username"), url.userName());
    check(QStringLiteral("password"), url.password());
    check(QStringLiteral("host"), url.host() + u':' + QString::number(url.port()));
    check(QStringLiteral("hostname"), url.host());
    check(QStringLiteral("port"), QString::number(url.port()));
    check(QStringLiteral("pathname"), url.path());
    check(QStringLiteral("search"), QStringLiteral("?search=stuff&other=where"));
    check(QStringLiteral("hash"), u'#' + url.fragment());

    QJSManagedValue s(v.property("searchParams"), &engine);
    QVERIFY(s.isObject());

    const QStringList searchParamsMethods = {
        QStringLiteral("append"),
        QStringLiteral("delete"),
        QStringLiteral("get"),
        QStringLiteral("getAll"),
        QStringLiteral("has"),
        QStringLiteral("set"),
        QStringLiteral("sort"),
        QStringLiteral("entries"),
        QStringLiteral("forEach"),
        QStringLiteral("keys"),
        QStringLiteral("values"),
        QStringLiteral("toString")
    };

    for (const QString &method : searchParamsMethods) {
        QJSManagedValue get(s.property(method), &engine);

        // Shoudn't crash.
        // We get different error messages depending on parameters, though.
        QJSValue undef = get.call({});
        QVERIFY(undef.isUndefined());
        QVERIFY(engine.hasError());
        engine.catchError();
    }

    QVariant urlVariant(url);
    QV4::Scope scope(engine.handle());
    QV4::ScopedValue urlValue(scope, scope.engine->fromVariant(urlVariant));
    QVERIFY(urlValue->isObject());

    QUrl result1;
    QVERIFY(QV4::ExecutionEngine::metaTypeFromJS(urlValue, QMetaType::fromType<QUrl>(), &result1));
    QCOMPARE(result1, url);

    QV4::ScopedValue urlVariantValue(scope, scope.engine->newVariantObject(
                                         QMetaType::fromType<QUrl>(), &url));
    QVERIFY(urlVariantValue->isObject());
    QUrl result2;
    QVERIFY(QV4::ExecutionEngine::metaTypeFromJS(urlVariantValue, QMetaType::fromType<QUrl>(),
                                                 &result2));
    QCOMPARE(result2, url);
}

void tst_QJSEngine::thisInConstructor()
{
    QJSEngine engine;
    const QJSValue result = engine.evaluate(R"((function() {
        let a = undefined;
        class Bugtest {
            constructor() {
                (() => {
                    if (true) {
                        a = this;
                    }
                })();
            }
        };
        new Bugtest();
        return a;
    })())");
    QVERIFY(!result.isUndefined());
    QVERIFY(result.isObject());
}

void tst_QJSEngine::forOfAndGc()
{
    // We want to guard against the iterator of a for..of loop leaving the result unprotected from
    // garbage collection. It should be possible to construct a pure JS test case, but due to the
    // vaguaries of garbage collection it's hard to reliably trigger the crash. This test is the
    // best I could come up with.

    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData(R"(
        import QtQml

        QtObject {
            id: counter
            property int count: 0

            property DelegateModel model: DelegateModel {
                id: filesModel

                model: ListModel {
                    Component.onCompleted: {
                        for (let idx = 0; idx < 50; idx++)
                            append({"i" : idx})
                    }
                }

                groups: [
                    DelegateModelGroup {
                        name: "selected"
                    }
                ]

                function getSelected() {
                    for (let i = 0; i < items.count; ++i) {
                        var item = items.get(i)
                        for (let el of item.groups) {
                            if (el === "selected")
                                ++counter.count
                        }
                    }
                }

                property bool bSelect: true
                function selectAll() {
                    for (let i = 0; i < items.count; ++i) {
                        if (bSelect && !items.get(i).inSelected)
                            items.addGroups(i, 1, ["selected"])
                        else
                            items.removeGroups(i, 1, ["selected"])
                        getSelected()
                    }
                    bSelect = !bSelect
                }
            }

            property Timer timer: Timer {
                running: true
                interval: 1
                repeat: true
                onTriggered: filesModel.selectAll()
            }
        }
    )", QUrl());

    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QTRY_VERIFY(o->property("count").toInt() > 32768);
}

void tst_QJSEngine::jsExponentiate()
{
    const double numbers[] = {
        std::numeric_limits<int>::min(), -10, -1, 0, 1, 10, std::numeric_limits<int>::max(),
        -std::numeric_limits<double>::infinity(), -100.1, -1.2, -0.0, 0.0, 1.2, 100.1,
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()
    };

    QJSEngine engine;

    const QJSManagedValue exp(engine.evaluate("(function(a, b) { return a ** b })"), &engine);
    const QJSManagedValue pow(engine.evaluate("Math.pow"), &engine);
    QVERIFY(exp.isFunction());
    QVERIFY(pow.isFunction());

    for (double a : numbers) {
        for (double b : numbers)
            QCOMPARE(exp.call({a, b}).toNumber(), pow.call({a, b}).toNumber());
    }
}

void tst_QJSEngine::arrayBuffer()
{

    QJSEngine engine;
    auto test = [&engine](const QByteArray &ba) {
        QJSValue value = engine.toScriptValue(ba);
        engine.globalObject().setProperty("array", value);

        const auto result = engine.evaluate("(function(){ return array.byteLength; })()");

        QVERIFY(result.isNumber());
        QCOMPARE(result.toInt(), ba.size());
    };

    test({});
    test("Hello");
}

void tst_QJSEngine::staticInNestedClasses()
{
    QJSEngine engine;
    const QString program = uR"(
        class Tester {
            constructor() {
                new (class {})();
            }
            static get test() { return "a" }
        }
        Tester.test
    )"_s;

    QCOMPARE(engine.evaluate(program).toString(), u"a"_s);
}

void tst_QJSEngine::callElement()
{
    QJSEngine engine;
    const QString program = uR"(
        function myFunc(arg) { return arg === this; };
        let array = [myFunc, "string"];
        array[0](array.reverse()) ? "a" : "b";
    )"_s;
    QCOMPARE(engine.evaluate(program).toString(), u"a"_s);
}

void tst_QJSEngine::functionCtorGeneratedCUIsNotCollectedByGc()
{
    QJSEngine engine;
    auto v4 = engine.handle();
    QVERIFY(!v4->isGCOngoing);

    // run gc until roots are collected
    // we run the gc steps manually, so use "Forever" as the dealine to avoid interference
    v4->memoryManager->gcStateMachine->deadline = QDeadlineTimer(QDeadlineTimer::Forever);
    auto sm = v4->memoryManager->gcStateMachine.get();
    sm->reset();
    while (sm->state != QV4::GCState::InitMarkPersistentValues) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }

    const QString program = "new Function('a', 'b', 'let x = \"Hello\"; return a + b');";
    auto sumFunc = engine.evaluate(program);
    QVERIFY(sumFunc.isCallable());
    auto *function = QJSValuePrivate::asManagedType<QV4::JavaScriptFunctionObject>(&sumFunc);
    auto *cu = function->d()->function->executableCompilationUnit();
    QVERIFY(cu->runtimeStrings); // should exist for "Hello"
    QVERIFY(cu->runtimeStrings[0]->isMarked());
    while (sm->state != QV4::GCState::Invalid) {
        QV4::GCStateInfo& stateInfo = sm->stateInfoMap[int(sm->state)];
        sm->state = stateInfo.execute(sm, sm->stateData);
    }

    auto sum = sumFunc.call({QJSValue(12), QJSValue(13)});
    QCOMPARE(sum.toInt(), 25);
}

void tst_QJSEngine::tdzViolations_data()
{
    QTest::addColumn<QString>("type");
    QTest::addRow("let") << u"let"_s;
    QTest::addRow("const") << u"const"_s;
}

void tst_QJSEngine::tdzViolations()
{
    QFETCH(QString, type);
    type.resize(8, u' '); // pad with some spaces, so that the columns match.

    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    const QString program1 = uR"(
        (function() {
            a = 5;
            %1 a = 1;
            return a;
        })();
    )"_s.arg(type);

    QTest::ignoreMessage(
                QtWarningMsg,
                ":3:13 Variable \"a\" is used before its declaration at 4:22.");

    const QJSValue result1 = engine.evaluate(program1);
    QVERIFY(result1.isError());
    QCOMPARE(result1.toString(), u"ReferenceError: a is not defined"_s);

    const QString program2 = uR"(
        (function() {
            function stringify(x) { return x + "" }
            var c = "";
            for (var a = 0; a < 10; ++a) {
                if (a > 0) {
                    c += stringify(b);
                }
                %1 b = 10;
            }
            return c;
        })();
    )"_s.arg(type);

    QTest::ignoreMessage(
                QtWarningMsg,
                ":7:36 Variable \"b\" is used before its declaration at 9:26.");

    const QJSValue result2 = engine.evaluate(program2);
    QVERIFY(result2.isError());
    QCOMPARE(result2.toString(), u"ReferenceError: b is not defined"_s);

    const QString program3 = uR"(
        (function() {
            var a = 10;
            switch (a) {
            case 1:
                %1 b = 5;
            case 10:
                console.log(b);
            }
        })();
    )"_s.arg(type);

    const QJSValue result3 = engine.evaluate(program3);
    QVERIFY(result3.isError());
    QCOMPARE(result3.toString(), u"ReferenceError: b is not defined"_s);
}

class WithToString : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE int toString() const { return 29; }
};

struct UnknownToJS
{
    int thing = 13;
};

void tst_QJSEngine::coerceValue()
{
    const UnknownToJS u;
    QMetaType::registerConverter<UnknownToJS, int>([](const UnknownToJS &u) {
        return u.thing;
    });
    int v = 0;
    QVERIFY(QMetaType::convert(QMetaType::fromType<UnknownToJS>(), &u,
                               QMetaType::fromType<int>(), &v));
    QCOMPARE(v, 13);

    QMetaType::registerConverter<UnknownToJS, QTypeRevision>([](const UnknownToJS &u) {
        return QTypeRevision::fromMinorVersion(u.thing);
    });
    QTypeRevision w;
    QVERIFY(QMetaType::convert(QMetaType::fromType<UnknownToJS>(),
                               &u, QMetaType::fromType<QTypeRevision>(), &w));
    QCOMPARE(w, QTypeRevision::fromMinorVersion(13));


    QJSEngine engine;
    WithToString withToString;
    const int i = 7;
    const QString a = QStringLiteral("5.25");

    QCOMPARE((engine.coerceValue<int, int>(i)), i);
    QVERIFY((engine.coerceValue<int, QJSValue>(i)).strictlyEquals(QJSValue(i)));
    QVERIFY((engine.coerceValue<int, QJSManagedValue>(i)).strictlyEquals(
             QJSManagedValue(QJSPrimitiveValue(i), &engine)));
    QCOMPARE((engine.coerceValue<QVariant, int>(QVariant(i))), i);
    QCOMPARE((engine.coerceValue<int, QVariant>(i)), QVariant(i));
    QCOMPARE((engine.coerceValue<WithToString *, QString>(&withToString)), QStringLiteral("29"));
    QCOMPARE((engine.coerceValue<WithToString *, const WithToString *>(&withToString)), &withToString);
    QCOMPARE((engine.coerceValue<QString, double>(a)), 5.25);
    QCOMPARE((engine.coerceValue<double, QString>(5.25)), a);
    QCOMPARE((engine.coerceValue<UnknownToJS, int>(u)), v); // triggers valueOf on a VariantObject
    QCOMPARE((engine.coerceValue<UnknownToJS, QTypeRevision>(u)), w);
}

void tst_QJSEngine::coerceDateTime_data()
{
    QTest::addColumn<QDateTime>("dateTime");

    QTest::newRow("invalid") << QDateTime();
    QTest::newRow("now")     << QDateTime::currentDateTime();

    QTest::newRow("denormal-March")      << QDateTime(QDate(2019, 3, 1), QTime(0, 0, 0, 1));
    QTest::newRow("denormal-leap")       << QDateTime(QDate(2020, 2, 29), QTime(23, 59, 59, 999));
    QTest::newRow("denormal-time")       << QDateTime(QDate(2020, 2, 29), QTime(0, 0));
    QTest::newRow("October")             << QDateTime(QDate(2019, 10, 3), QTime(12, 0));
    QTest::newRow("nonstandard-format")  << QDateTime::fromString("1991-08-25 20:57:08 GMT+0000", "yyyy-MM-dd hh:mm:ss t");
    QTest::newRow("nonstandard-format2") << QDateTime::fromString("Sun, 25 Mar 2018 11:10:49 GMT", "ddd, d MMM yyyy hh:mm:ss t");

    const QDate date(2009, 5, 12);
    const QTime early(0, 0, 1);
    const QTime late(23, 59, 59);
    const int offset = (11 * 60 + 30) * 60;

    QTest::newRow("Local time early") << QDateTime(date, early);
    QTest::newRow("Local time late")  << QDateTime(date, late);
    QTest::newRow("UTC early")        << QDateTime(date, early, QTimeZone::UTC);
    QTest::newRow("UTC late")         << QDateTime(date, late, QTimeZone::UTC);
    QTest::newRow("+11:30 early")     << QDateTime(date, early, QTimeZone::fromSecondsAheadOfUtc(offset));
    QTest::newRow("+11:30 late")      << QDateTime(date, late, QTimeZone::fromSecondsAheadOfUtc(offset));
    QTest::newRow("-11:30 early")     << QDateTime(date, early, QTimeZone::fromSecondsAheadOfUtc(-offset));
    QTest::newRow("-11:30 late")      << QDateTime(date, late, QTimeZone::fromSecondsAheadOfUtc(-offset));

    QTest::newRow("dt0") << QDateTime(QDate(1900,  1,  2), QTime( 8, 14));
    QTest::newRow("dt1") << QDateTime(QDate(2000, 11, 22), QTime(10, 45));
}

void tst_QJSEngine::coerceDateTime()
{
    QFETCH(QDateTime, dateTime);
    const QDate date = dateTime.date();
    const QTime time = dateTime.time();


    QQmlEngine engine;

    {
        QQmlComponent c(&engine);
        c.setData(R"(
            import Test
            DateTimeHolder {
                string: dateTime
                date: dateTime
                time: dateTime
            }
        )", QUrl(u"fromDateTime.qml"_s));

        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());

        DateTimeHolder *holder = qobject_cast<DateTimeHolder *>(o.data());
        QVERIFY(holder);

        const QJSValue jsDateTime = engine.toScriptValue(dateTime);
        holder->m_dateTime = dateTime;
        emit holder->dateTimeChanged();

        QCOMPARE((engine.coerceValue<QDateTime, QString>(dateTime)), holder->m_string);
        QCOMPARE(qjsvalue_cast<QString>(jsDateTime), holder->m_string);
        QCOMPARE((engine.coerceValue<QDateTime, QDate>(dateTime)), holder->m_date);
        QCOMPARE(qjsvalue_cast<QDate>(jsDateTime), holder->m_date);
        QCOMPARE((engine.coerceValue<QDateTime, QTime>(dateTime)), holder->m_time);
        QCOMPARE(qjsvalue_cast<QTime>(jsDateTime), holder->m_time);
    }

    {
        QQmlComponent c(&engine);
        c.setData(R"(
            import Test
            DateTimeHolder {
                dateTime: date
                time: date
                string: date
            }
        )", QUrl(u"fromDate.qml"_s));

        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());

        DateTimeHolder *holder = qobject_cast<DateTimeHolder *>(o.data());
        QVERIFY(holder);

        const QJSValue jsDate = engine.toScriptValue(date);
        holder->m_date = date;
        emit holder->dateChanged();

        QCOMPARE((engine.coerceValue<QDate, QDateTime>(date)), holder->m_dateTime);
        QCOMPARE(qjsvalue_cast<QDateTime>(jsDate), holder->m_dateTime);
        QCOMPARE((engine.coerceValue<QDate, QString>(date)), holder->m_string);
        QCOMPARE(qjsvalue_cast<QString>(jsDate), holder->m_string);
        QCOMPARE((engine.coerceValue<QDate, QTime>(date)), holder->m_time);
        QCOMPARE(qjsvalue_cast<QTime>(jsDate), holder->m_time);
    }

    {
        QQmlComponent c(&engine);
        c.setData(R"(
            import Test
            DateTimeHolder {
                dateTime: time
                date: time
                string: time
            }
        )", QUrl(u"fromTime.qml"_s));

        QVERIFY2(c.isReady(), qPrintable(c.errorString()));
        QScopedPointer<QObject> o(c.create());
        QVERIFY(!o.isNull());

        DateTimeHolder *holder = qobject_cast<DateTimeHolder *>(o.data());
        QVERIFY(holder);

        const QJSValue jsTime = engine.toScriptValue(time);
        holder->m_time = time;
        emit holder->timeChanged();

        QCOMPARE((engine.coerceValue<QTime, QDateTime>(time)), holder->m_dateTime);
        QCOMPARE(qjsvalue_cast<QDateTime>(jsTime), holder->m_dateTime);
        QCOMPARE((engine.coerceValue<QTime, QString>(time)), holder->m_string);
        QCOMPARE(qjsvalue_cast<QString>(jsTime), holder->m_string);
        QCOMPARE((engine.coerceValue<QTime, QDate>(time)), holder->m_date);
        QCOMPARE(qjsvalue_cast<QDate>(jsTime), holder->m_date);
    }
}

void tst_QJSEngine::callWithSpreadOnElement()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    const QString program = uR"(
        let f = console.error;
        const data = [f, ["That is great!"]]
        data[0](...data[1]);
    )"_s;

    QTest::ignoreMessage(QtCriticalMsg, "That is great!");
    const QJSValue result = engine.evaluate(program);
    QVERIFY(!result.isError());
}

void tst_QJSEngine::spreadNoOverflow()
{
    QJSEngine engine;

    const QString program = QString::fromLatin1("var a = [] ;a.length =  555840;Math.max(...a)");
    const QJSValue result = engine.evaluate(program);
    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::RangeError);
}

void tst_QJSEngine::symbolToVariant()
{
    QJSEngine engine;
    const QJSValue val = engine.newSymbol("asymbol");
    QCOMPARE(val.toVariant(), QStringLiteral("Symbol(asymbol)"));

    const QVariant retained = val.toVariant(QJSValue::RetainJSObjects);
    QCOMPARE(retained.metaType(), QMetaType::fromType<QJSValue>());
    QVERIFY(retained.value<QJSValue>().strictlyEquals(val));

    QCOMPARE(val.toVariant(QJSValue::ConvertJSObjects), QStringLiteral("Symbol(asymbol)"));
}

class PACHelper : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE bool shExpMatch(const QString &, const QString &) { return false; }
    Q_INVOKABLE QString dnsResolve(const QString &) { return QString{}; }
};

class ProxyAutoConf {
public:
    void exposeQObjectMethodsAsGlobal(QJSEngine *engine, QObject *object)
    {
        QJSValue helper = engine->newQObject(object);
        QJSValue g = engine->globalObject();
        QJSValueIterator it(helper);
        while (it.hasNext()) {
            it.next();
            if (!it.value().isCallable())
                continue;
            g.setProperty(it.name(), it.value());
        }
    }

    bool parse(const QString & pacBytes)
    {
        jsFindProxyForURL = QJSValue();
        engine = std::make_unique<QJSEngine>();
        exposeQObjectMethodsAsGlobal(engine.get(), new PACHelper);
        engine->evaluate(pacBytes);
        jsFindProxyForURL = engine->globalObject().property(QStringLiteral("FindProxyForURL"));
        return true;
    }

    QString findProxyForUrl(const QString &url, const QString &host)
    {
        QJSValueList args;
        args << url << host;
        gc(*engine->handle(), GCFlags::DontSendPostedEvents);
        QJSValue callResult = jsFindProxyForURL.call(args);
        return callResult.toString().trimmed();
    }

private:
    std::unique_ptr<QJSEngine> engine;
    QJSValue jsFindProxyForURL;
};

QString const pacstring = R"js(
function FindProxyForURL(host) {
    list_split_all = Array(
        "oneoneoneoneo.oneo.oneo.oneoneo.one",
        "twotwotwotwotw.otwo.twot.wotwotw.otw",
        "threethreethr.eeth.reet.hreethr.eet",
        "fourfourfourfo.urfo.urfo.urfourf.our",
        "fivefivefivef.ivef.ivef.ivefive.fiv",
        "sixsixsixsixsi.xsix.sixs.ixsixsi.xsi",
        "sevensevenseve.nsev.ense.venseve.nse",
        "eight.eighteigh.tei",
        "*.nin.eninen.ine"
    )
    list_myip_direct =
        "10.254.0.0/255.255.0.0"
    for (i = 0; i < list_split_all.length; ++i)
        for (j = 0; j < list_myip_direct.length; ++j)
            shExpMatch(host, list_split_all)
        shExpMatch()
    dnsResolve()}
)js";

void tst_QJSEngine::garbageCollectedObjectMethodBase()
{
    ProxyAutoConf proxyConf;
    bool pac_read = false;

    const auto processUrl = [&](QString const &url, QString const &host)
    {
        if (!pac_read) {
            proxyConf.parse(pacstring);
            pac_read = true;
        }
        return proxyConf.findProxyForUrl(url, host);
    };

    const QString url = QStringLiteral("https://servername.domain.test");
    const QString host = QStringLiteral("servername.domain.test");

    for (size_t i = 0; i < 5; ++i) {
        auto future = std::async(processUrl, url, host);
        QCOMPARE(future.get(), QLatin1String("Error: Insufficient arguments"));
    }
}

void tst_QJSEngine::optionalChainWithElementLookup()
{
    QJSEngine engine;

    const QString program = R"js(
        (function(xxx) { return xxx?.title["en"] ?? "A" })
    )js";

    QJSManagedValue func = QJSManagedValue(engine.evaluate(program), &engine);
    QVERIFY(func.isFunction());

    QCOMPARE(func.call({QJSValue::NullValue}).toString(), "A");
    QCOMPARE(func.call({QJSValue::UndefinedValue}).toString(), "A");

    const QJSValue nice
            = engine.toScriptValue(QVariantMap { {"title", QVariantMap { {"en", "B"} } } });
    QCOMPARE(func.call({nice}).toString(), "B");

    const QJSValue naughty1
            = engine.toScriptValue(QVariantMap { {"title", QVariantMap { {"fr", "B"} } } });
    QCOMPARE(func.call({naughty1}).toString(), "A");

    const QJSValue naughty2
            = engine.toScriptValue(QVariantMap { {"foos", QVariantMap { {"en", "B"} } } });
    QVERIFY(func.call({naughty2}).isUndefined());
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), "TypeError: Cannot read property 'en' of undefined");
    QVERIFY(!engine.hasError());

    QVERIFY(func.call({ QJSValue(4) }).isUndefined());
    QVERIFY(engine.hasError());
    QCOMPARE(engine.catchError().toString(), "TypeError: Cannot read property 'en' of undefined");
    QVERIFY(!engine.hasError());
}

void tst_QJSEngine::deleteDefineCycle()
{
  QJSEngine engine;
  QStringList stackTrace;

  QJSValue result = engine.evaluate(QString::fromLatin1(R"(
  let global = ({})

  for (let j = 0; j < 1000; j++) {
    for (let i = 0; i < 2; i++) {
      const name = "test" + i
      delete global[name]
      Object.defineProperty(global, name, { get() { return 0 }, configurable: true })
    }
  }
  )"), {}, 1, &stackTrace);
  QVERIFY(stackTrace.isEmpty());
}

void tst_QJSEngine::deleteFromSparseArray()
{
    QJSEngine engine;

    // Should not crash
    const QJSValue result = engine.evaluate(QLatin1String(R"((function() {
        let o = [];
        o[10000] = 10;
        o[20000] = 20;
        for (let k in o)
            delete o[k];
        return o;
    })())"));

    QVERIFY(result.isArray());
    QCOMPARE(result.property("length").toNumber(), 20001);
    QVERIFY(result.property(10000).isUndefined());
    QVERIFY(result.property(20000).isUndefined());
}

void tst_QJSEngine::emptyStringLiteralEvaluatesToANonNullString() {
  QJSEngine engine;
  QJSValue result = engine.evaluate(R"(
    function f() {
        return "";
    }
    f();
  )");

  QVERIFY(result.isString());
  QVERIFY(!result.toString().isNull());
  QVERIFY(result.toString().isEmpty());
}

static unsigned stringListFetchCount = 0;
class StringListProvider : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList strings READ strings CONSTANT)

public:
    QStringList strings() const
    {
        ++stringListFetchCount;
        QStringList ret;
        for (int i = 0; i < 10; ++i)
            ret.append(QString::number(i));
        return ret;
    }
};

void tst_QJSEngine::consoleLogSequence()
{
    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    engine.globalObject().setProperty(
                QStringLiteral("object"), engine.newQObject(new StringListProvider));

    QTest::ignoreMessage(QtDebugMsg, "[0,1,2,3,4,5,6,7,8,9]");

    engine.evaluate(QStringLiteral("console.log(object.strings)"));
    QCOMPARE(stringListFetchCount, 1);
}


void tst_QJSEngine::generatorFunctionInTailCallPosition() {
  QJSEngine engine;
  QJSValue result = engine.evaluate(R"(
    "use strict";
    function* gen() {
        yield 0;
    }
    function caller() { return gen(); }
    caller();
  )");

  QVERIFY(!result.isError());
  QVERIFY(!result.isUndefined());
}

void tst_QJSEngine::generatorMethodInTailCallPosition() {
  QJSEngine engine;
  QJSValue result = engine.evaluate(R"(
    "use strict";
    class Class {
        *gen() {
            yield 0;
        }

        caller() { return this.gen(); }
    }
    var c = new Class();
    c.caller();
  )");

  QVERIFY(!result.isError());
  QVERIFY(!result.isUndefined());
}

void tst_QJSEngine::generatorStackOverflow_data() {
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("callDepth");

    auto makeCode = [](QString method) {
        return uR"(
            function* gen() { yield 1; yield 2; }
            function indirection(g) { return g.%1(); }

            var g = gen();
            g.next() // used to trigger a resume in return and throw
            indirection(g);
          )"_s.arg(method);
    };

    QTest::addRow("Stack Overflow on calling a generator function")
        << u"function* gen(){}; gen()"_s << 1;
    QTest::addRow("Stack Overflow on next") << makeCode(u"next"_s) << 2;
    QTest::addRow("Stack Overflow on return") << makeCode(u"return"_s) << 2;
    QTest::addRow("Stack Overflow on throw") << makeCode(u"throw"_s) << 2;
}

void tst_QJSEngine::generatorStackOverflow() {
    QFETCH(QString, code);
    QFETCH(int, callDepth);

    const auto guard = qScopeGuard([maxCallDepth = QV4::ExecutionEngine::maxCallDepth()]() {
        QV4::ExecutionEngine::setMaxCallDepth(maxCallDepth);
    });

    QJSEngine engine;

    QV4::ExecutionEngine::setMaxCallDepth(callDepth);
    engine.handle()->callDepth = 0;

    QJSValue result = engine.evaluate(code);

    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::RangeError);
    QCOMPARE(result.toString(), "RangeError: Maximum call stack size exceeded.");
}

void tst_QJSEngine::generatorInfiniteRecursion() {
    QJSEngine engine;
    QJSValue result = engine.evaluate(R"(
        function* gen() { yield* gen() }
        for (const nothing of gen()) {}
    )");

    QVERIFY(result.isError());
    QCOMPARE(result.errorType(), QJSValue::RangeError);
    QCOMPARE(result.toString(), "RangeError: Maximum call stack size exceeded.");
}

void tst_QJSEngine::setDeleteDuringForEach() {
  QJSEngine engine;
  QJSValue result = engine.evaluate(R"(
    let set = new Set([1,2,3]);
    let visited = []
    set.forEach((v) => {
        visited.push(v);
        set.delete(v)
    })
    visited
  )");

  QVERIFY(result.isArray());

  QJsonArray visited = engine.fromScriptValue<QJsonArray>(result);
  QCOMPARE(visited, QJsonArray({1, 2, 3}));
}

void tst_QJSEngine::mapDeleteDuringForEach() {
  QJSEngine engine;
  QJSValue result = engine.evaluate(R"(
    let map = new Map([[1, 1], [2, 2], [3, 3]]);
    let visited = []
    map.forEach((v, k) => {
        visited.push(v);
        map.delete(k)
    })
    visited
  )");

  QVERIFY(result.isArray());

  QJsonArray visited = engine.fromScriptValue<QJsonArray>(result);
  QCOMPARE(visited, QJsonArray({1, 2, 3}));
}

void tst_QJSEngine::multiMatchingRegularExpression()
{
    QJSEngine engine;
    const QJSValue result = engine.evaluate(R"(
        "33312345.897".replace(/\./g, ",").replace(/\B(?=(\d{3})+(?!\d))/g, ".")
    )");

    QVERIFY(result.isString());
    QCOMPARE(result.toString(), "33.312.345,897"_L1);

    const QJSValue result2 = engine.evaluate(R"(
        "4F159D7AD40255D94A5B7EB9AAACD7408C79245D".replace(/(....)/g, '$1 ')
    )");

    QVERIFY(result2.isString());
    QCOMPARE(result2.toString(), "4F15 9D7A D402 55D9 4A5B 7EB9 AAAC D740 8C79 245D "_L1);
}

#if QT_CONFIG(icu)
void tst_QJSEngine::toLocaleLowerCase_data()
{
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("toBeLocaleLowerCased");
    QTest::addColumn<QString>("locales");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QJSValue::ErrorType>("errorType");
    QTest::addColumn<QString>("expectedFailureComment");
    // As there are no "NoError", the default error is set to GenericError
    // We only check the error if the call is not valid
    QTest::addRow("In Turkic (tr), U+0307 preceded by Capital Letter I is dropped.")
            << u"abci"_s << u"\"aBcI\u0307\""_s << u"\"tr\""_s << true << QJSValue::GenericError
            << "";
    QTest::addRow("In Turkic (az), U+0307 preceded by Capital Letter I is dropped.")
            << u"abci"_s << u"\"aBcI\u0307\""_s << u"\"az\""_s << true << QJSValue::GenericError
            << "";
    QTest::addRow("U+0307 preceded by Capital Letter I is dropped.")
            << u"abci"_s << u"\"aBcI\u0307\""_s << u"[\"tr\", \"en\"]"_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("concatened string.") << u"abcde"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u""_s
                                        << true << QJSValue::GenericError << "";
    QTest::addRow("concatened string in filipino.")
            << u"abcde"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u"\"fil\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("concatened string in structurally valid language.")
            << u"abcde"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u"\"longlang\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("english locale keeps U+0307.")
            << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"en\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("english (en-GB) locale keeps U+0307.")
            << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"en-GB\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("klingon locale.") << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"i-klingon\""_s
                                     << true << QJSValue::GenericError << "";
    QTest::addRow("enochian locale.")
            << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"i-enochian\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("x-foobar locale.") << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"x-foobar\""_s
                                      << true << QJSValue::GenericError << "";
    QTest::addRow("zh-Hant-TW locale.")
            << u"abci\u0307"_s << u"\"aBcI\u0307\""_s << u"\"zh-Hant-TW\""_s << true
            << QJSValue::GenericError << "";
    // cases not handled
    QTest::addRow("String ArrayBuffer error.")
            << u"[object arraybuffer]"_s << u"(new String(new ArrayBuffer()))"_s << u"\"fil\""_s
            << false << QJSValue::GenericError << "Should return [object arraybuffer]";

    QTest::addRow("undefined in Intl error.")
            << u"abc"_s << u"abc"_s << u"Intl.GetDefaultLocale"_s << false << QJSValue::GenericError
            << "Intl.GetDefaultLocale is undefined, however, as Intl is not defined in the tests, "
               "we fail due to it";
}

void tst_QJSEngine::toLocaleLowerCase()
{
    QFETCH(QString, expected);
    QFETCH(QString, toBeLocaleLowerCased);
    QFETCH(QString, locales);
    QFETCH(bool, valid);
    QFETCH(QJSValue::ErrorType, errorType);
    QFETCH(QString, expectedFailureComment);

    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    const QString program =
            QString::fromUtf8("%1.toLocaleLowerCase(%2);").arg(toBeLocaleLowerCased, locales);

    const QJSValue result = engine.evaluate(program);

    if (!expectedFailureComment.isEmpty()) {
        QEXPECT_FAIL("", expectedFailureComment.toLocal8Bit().data(), Abort);
    }
    QVERIFY(expectedFailureComment.isEmpty());
    QCOMPARE(!result.isError(), valid);
    if (valid) {
        QVERIFY(result.isString());
        QCOMPARE(result.toString(), expected);
    }
    if (!valid) {
        QCOMPARE(result.errorType(), errorType);
    }
}

void tst_QJSEngine::toLocaleLowerStringWithQLocale()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine);
    comp.setData(R"(
        import QtQml 2.15
        QtObject {
            id: root
            property string loweredCase
            property string loweredCaseArray
            Component.onCompleted: () => {
                const myLocale = Qt.locale("tr_TR")
                root.loweredCase = "aBcI\u0307".toLocaleLowerCase(myLocale);
                root.loweredCaseArray = "aBcI\u0307".toLocaleLowerCase([myLocale]);
            }
        }
    )", QUrl("testdata"));
    QScopedPointer<QObject> root {comp.create()};
    const auto error = comp.errorString();
    if (!error.isEmpty())
        qDebug() << error;
    QVERIFY(root);
    QCOMPARE(root->property("loweredCase").toString(), QLatin1String("abci"));
    QCOMPARE(root->property("loweredCaseArray").toString(), QLatin1String("abci"));
}

void tst_QJSEngine::toLocaleUpperCase_data()
{
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("toBeLocaleUpperCased");
    QTest::addColumn<QString>("locales");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QJSValue::ErrorType>("errorType");
    QTest::addColumn<QString>("expectedFailureComment");
    // As there are no "NoError", the default error is set to GenericError
    // We only check the error if the call is not valid
    QTest::addRow("OneByte input with buffer size increase")
            << u"ABCSS"_s << u"\"abCß\""_s << u"\"tr\""_s << true << QJSValue::GenericError << "";
    QTest::addRow("concatened string.") << u"ABCDE"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u""_s
                                        << true << QJSValue::GenericError << "";
    QTest::addRow("concatened string in filipino.")
            << u"ABCDE"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u"\"fil\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("concatened string in structurally valid language.")
            << u"ABCDE"_s << u"(\"a\" + \"b\" + \"cde\")"_s << u"\"longlang\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("english locale keeps U+0307.")
            << u"ABCI\u0307"_s << u"\"aBcI\u0307\""_s << u"\"en\""_s << true
            << QJSValue::GenericError << "";
    QTest::addRow("english (en-GB) locale keeps U+0307.")
            << u"ABCI\u0307"_s << u"\"aBcI\u0307\""_s << u"\"en-GB\""_s << true
            << QJSValue::GenericError << "";

    // Greek uppercasing: not covered by intl402/String/*, yet. Tonos (U+0301) and
    // other diacritic marks are dropped.  See
    // http://bugs.icu-project.org/trac/ticket/5456#comment:19 for more examples.
    // See also http://bugs.icu-project.org/trac/ticket/12845 .
    QTest::addRow("Greek uppercasing.")
            << u"A"_s << u"\"α\u0301\""_s << u"\"el-GR\""_s << true << QJSValue::GenericError
            << "Greek uppercasing: not covered by intl402/String/*, yet. Tonos (U+0301) and other "
               "diacritic marks are dropped. See "
               "http://bugs.icu-project.org/trac/ticket/5456#comment:19 for more examples. See "
               "also http://bugs.icu-project.org/trac/ticket/12845";
    QTest::addRow("Greek uppercasing 2.")
            << u"A"_s << u"\"α\u0301\""_s << u"\"el-Grek\""_s << true << QJSValue::GenericError
            << "Greek uppercasing: not covered by intl402/String/*, yet. Tonos (U+0301) and other "
               "diacritic marks are dropped. See "
               "http://bugs.icu-project.org/trac/ticket/5456#comment:19 for more examples. See "
               "also http://bugs.icu-project.org/trac/ticket/12845";
    QTest::addRow("Greek uppercasing 3.")
            << u"A"_s << u"\"α\u0301\""_s << u"\"el-Grek-GR\""_s << true << QJSValue::GenericError
            << "Greek uppercasing: not covered by intl402/String/*, yet. Tonos (U+0301) and other "
               "diacritic marks are dropped. See "
               "http://bugs.icu-project.org/trac/ticket/5456#comment:19 for more examples. See "
               "also http://bugs.icu-project.org/trac/ticket/12845";
    QTest::addRow("Greek uppercasing 4.") << u"ΡΩΜΕΪΚΑ"_s << u"\"ρωμέικα\""_s << u"\"el\""_s << true
                                          << QJSValue::GenericError << "";
    QTest::addRow("In other locales, U+0301 is preserved.")
            << u"Α\u0301Ο\u0301Υ\u0301Ω\u0301"_s << u"\"α\u0301ο\u0301υ\u0301ω\u0301\""_s
            << u"\"en\""_s << true << QJSValue::GenericError << "";
    // cases not handled
    QTest::addRow("String ArrayBuffer error.")
            << u"[OBJECT ARRAYBUFFER]"_s << u"(new String(new ArrayBuffer()))"_s << u"\"fil\""_s
            << false << QJSValue::GenericError << "Should return [OBJECT ARRAYBUFFER]";

    QTest::addRow("undefined in Intl error.")
            << u"abc"_s << u"abc"_s << u"Intl.GetDefaultLocale"_s << false << QJSValue::GenericError
            << "Intl.GetDefaultLocale is undefined, however, as Intl is not defined in the tests, "
               "we fail due to it";
}

void tst_QJSEngine::toLocaleUpperCase()
{
    QFETCH(QString, expected);
    QFETCH(QString, toBeLocaleUpperCased);
    QFETCH(QString, locales);
    QFETCH(bool, valid);
    QFETCH(QJSValue::ErrorType, errorType);
    QFETCH(QString, expectedFailureComment);

    QJSEngine engine;
    engine.installExtensions(QJSEngine::ConsoleExtension);

    const QString program =
            QString::fromUtf8("%1.toLocaleUpperCase(%2);").arg(toBeLocaleUpperCased, locales);

    const QJSValue result = engine.evaluate(program);

    if (!expectedFailureComment.isEmpty()) {
        QEXPECT_FAIL("", expectedFailureComment.toLocal8Bit().data(), Abort);
    }
    QVERIFY(expectedFailureComment.isEmpty());
    QCOMPARE(!result.isError(), valid);
    if (valid) {
        QVERIFY(result.isString());
        QCOMPARE(result.toString(), expected);
    }
    if (!valid) {
        QCOMPARE(result.errorType(), errorType);
    }
}

void tst_QJSEngine::toLocaleUpperStringWithQLocale()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine);
    comp.setData(R"(
        import QtQml 2.15
        QtObject {
            id: root
            property string upperedCase
            property string upperedCaseArray
            Component.onCompleted: () => {
                const myLocale = Qt.locale("tr_TR")
                root.upperedCase = "abCß".toLocaleUpperCase(myLocale);
                root.upperedCaseArray = "abCß".toLocaleUpperCase([myLocale]);
            }
        }
    )", QUrl("testdata"));
    QScopedPointer<QObject> root {comp.create()};
    const auto error = comp.errorString();
    if (!error.isEmpty())
        qDebug() << error;
    QVERIFY(root);
    QCOMPARE(root->property("upperedCase").toString(), QLatin1String("ABCSS"));
    QCOMPARE(root->property("upperedCaseArray").toString(), QLatin1String("ABCSS"));
}
#endif

QTEST_MAIN(tst_QJSEngine)

#include "tst_qjsengine.moc"

