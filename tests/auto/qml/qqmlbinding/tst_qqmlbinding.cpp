// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "WithBindableProperties.h"

#include <private/qmlutils_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qqmlbind_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qqmlinstantiator_p.h>
#include <private/qqmlpropertytopropertybinding_p.h>
#include <private/qquickrectangle_p.h>

#include <QtTest/qtest.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlproperty.h>

class tst_qqmlbinding : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlbinding();

private slots:
    void binding();
    void whenAfterValue();
    void restoreBinding();
    void restoreBindingBindablePorperty();
    void restoreBindingValue();
    void restoreBindingVarValue();
    void restoreBindingJSValue();
    void restoreBindingWithLoop();
    void restoreBindingWithoutCrash();
    void restoreBindingWhenDestroyed();
    void deletedObject();
    void warningOnUnknownProperty();
    void warningOnReadOnlyProperty();
    void disabledOnUnknownProperty();
    void disabledOnReadonlyProperty();
    void delayed();
    void bindingOverwriting();
    void bindToQmlComponent();
    void bindingDoesNoWeirdConversion();
    void bindNaNToInt();
    void intOverflow();
    void generalizedGroupedProperties();
    void localSignalHandler();
    void whenEvaluatedEarlyEnough();
    void propertiesAttachedToBindingItself();
    void toggleEnableProperlyRemembersValues();
    void qQmlPropertyToPropertyBinding();
    void qQmlPropertyToPropertyBindingReverse();
    void delayedBindingDestruction();

private:
    QQmlEngine engine;
};

tst_qqmlbinding::tst_qqmlbinding()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
#ifdef QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES
    qputenv("QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES", "1");
#endif
}

void tst_qqmlbinding::binding()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-binding.qml"));
    QScopedPointer<QQuickRectangle> rect { qobject_cast<QQuickRectangle*>(c.create()) };
    QVERIFY(rect != nullptr);

    QQmlBind *binding3 = qobject_cast<QQmlBind*>(rect->findChild<QQmlBind*>("binding3"));
    QVERIFY(binding3 != nullptr);

    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));
    QCOMPARE(binding3->when(), false);

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));

    QCOMPARE(binding3->when(), true);

    QQmlBind *binding = qobject_cast<QQmlBind*>(rect->findChild<QQmlBind*>("binding1"));
    QVERIFY(binding != nullptr);
    QCOMPARE(binding->object(), qobject_cast<QObject*>(rect.get()));
    QCOMPARE(binding->property(), QLatin1String("text"));
    QCOMPARE(binding->value().toString(), QLatin1String("Hello"));
}

void tst_qqmlbinding::whenAfterValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("test-binding2.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};

    QVERIFY(rect != nullptr);
    QCOMPARE(rect->color(), QColor("yellow"));
    QCOMPARE(rect->property("text").toString(), QString("Hello"));

    rect->setProperty("changeColor", true);
    QCOMPARE(rect->color(), QColor("red"));
}

void tst_qqmlbinding::restoreBinding()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding.qml"));
    QScopedPointer<QQuickRectangle> rect { qobject_cast<QQuickRectangle*>(c.create()) };
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(100-25));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(100-13));

    //Binding takes effect
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //original binding restored
    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(100-49));
}

void tst_qqmlbinding::restoreBindingBindablePorperty()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding5.qml"));
    QScopedPointer<QQuickRectangle> rect { qobject_cast<QQuickRectangle*>(c.create()) };
    QVERIFY2(rect, qPrintable(c.errorString()));

    auto *myItem = rect->findChild<WithBindableProperties*>("myItem");
    QVERIFY(myItem != nullptr);

    myItem->setB(25);
    QCOMPARE(myItem->a(), qreal(100-25));

    myItem->setB(13);
    QCOMPARE(myItem->a(), qreal(100-13));

    //Binding takes effect
    myItem->setB(51);
    QCOMPARE(myItem->a(), qreal(51));

    myItem->setB(88);
    QCOMPARE(myItem->a(), qreal(88));

    //original binding restored
    myItem->setB(49);
    QCOMPARE(myItem->a(), qreal(100-49));
}

void tst_qqmlbinding::restoreBindingValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding2.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(o.data());
    QVERIFY(rect);

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->height(), 100);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->height(), 300); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->height(), 100); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 200);
    QCOMPARE(myItem->height(), 200); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the
    // change (i.e. 100)
    QCOMPARE(myItem->height(), 300);
}

void tst_qqmlbinding::restoreBindingVarValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding3.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(o.data());
    QVERIFY(rect);

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->property("foo"), 13);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->property("foo"), 42); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->property("foo"), 13); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 31337);
    QCOMPARE(myItem->property("foo"), 31337); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the
    // change (i.e. 100)
    QCOMPARE(myItem->property("foo"), 42);
}

void tst_qqmlbinding::restoreBindingJSValue()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBinding4.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(o.data());
    QVERIFY(rect);

    auto myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    QCOMPARE(myItem->property("fooCheck"), false);
    myItem->setProperty("when", QVariant(false));
    QCOMPARE(myItem->property("fooCheck"), true); // make sure the original value was restored

    myItem->setProperty("when", QVariant(true));
    QCOMPARE(myItem->property("fooCheck"), false); // make sure the value specified in Binding is set
    rect->setProperty("boundValue", 31337);
    QCOMPARE(myItem->property("fooCheck"), false); // make sure the changed binding value is set
    myItem->setProperty("when", QVariant(false));
    // make sure that the original value is back, not e.g. the value from before the change
    QCOMPARE(myItem->property("fooCheck"), true);

}

void tst_qqmlbinding::restoreBindingWithLoop()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBindingWithLoop.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(25 + 100));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(13 + 100));

    //Binding takes effect
    rect->setProperty("activateBinding", true);
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //original binding restored
    QString warning = c.url().toString() + QLatin1String(R"(:\d+:\d+: QML Rectangle: Binding loop detected for property "x")");
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warning));
    rect->setProperty("activateBinding", false);
    QCOMPARE(myItem->x(), qreal(88 + 100)); //if loop handling changes this could be 90 + 100

    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(49 + 100));
}

void tst_qqmlbinding::restoreBindingWithoutCrash()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("restoreBindingWithoutCrash.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QQuickRectangle *myItem = qobject_cast<QQuickRectangle*>(rect->findChild<QQuickRectangle*>("myItem"));
    QVERIFY(myItem != nullptr);

    myItem->setY(25);
    QCOMPARE(myItem->x(), qreal(100-25));

    myItem->setY(13);
    QCOMPARE(myItem->x(), qreal(100-13));

    //Binding takes effect
    myItem->setY(51);
    QCOMPARE(myItem->x(), qreal(51));

    myItem->setY(88);
    QCOMPARE(myItem->x(), qreal(88));

    //state sets a new binding
    rect->setState("state1");
    //this binding temporarily takes effect. We may want to change this behavior in the future
    QCOMPARE(myItem->x(), qreal(112));

    //Binding still controls this value
    myItem->setY(104);
    QCOMPARE(myItem->x(), qreal(104));

    //original binding restored
    myItem->setY(49);
    QCOMPARE(myItem->x(), qreal(100-49));
}

void tst_qqmlbinding::restoreBindingWhenDestroyed()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindingRestoredWhenDestroyed.qml"));
    QScopedPointer<QObject> root {c.create()};
    QVERIFY2(root, qUtf8Printable(c.errorString()));
    QCOMPARE(root->property("text").toString(), u"original");
    QCOMPARE(root->property("i").toInt(), 42);

    root->setProperty("toggle", true);
    // QTRY_COMPARE as loader is async
    QTRY_COMPARE(root->property("text").toString(), u"changed");
    QCOMPARE(root->property("i").toInt(), 100);

    root->setProperty("toggle", false);
    QTRY_COMPARE(root->property("text").toString(), u"original");
    QCOMPARE(root->property("i").toInt(), 100);
}

//QTBUG-20692
void tst_qqmlbinding::deletedObject()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("deletedObject.qml"));
    QScopedPointer<QQuickRectangle> rect {qobject_cast<QQuickRectangle*>(c.create())};
    QVERIFY(rect != nullptr);

    QGuiApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    //don't crash
    rect->setProperty("activateBinding", true);
}

void tst_qqmlbinding::warningOnUnknownProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("unknownProperty.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().size(), 1);

    const QString expectedMessage = c.url().toString() + QLatin1String(":6:5: QML Binding: Property 'unknown' does not exist on Item.");
    QCOMPARE(messageHandler.messages().first(), expectedMessage);
}

void tst_qqmlbinding::warningOnReadOnlyProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("readonlyProperty.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().size(), 1);

    const QString expectedMessage = c.url().toString() + QLatin1String(":8:5: QML Binding: Property 'name' on Item is read-only.");
    QCOMPARE(messageHandler.messages().first(), expectedMessage);
}

void tst_qqmlbinding::disabledOnUnknownProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabledUnknown.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);

    QCOMPARE(messageHandler.messages().size(), 0);
}

void tst_qqmlbinding::disabledOnReadonlyProperty()
{
    QQmlTestMessageHandler messageHandler;

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("disabledReadonly.qml"));
    QScopedPointer<QQuickItem> item { qobject_cast<QQuickItem *>(c.create()) };
    QVERIFY(item);
    QCOMPARE(messageHandler.messages().size(), 0);
}

void tst_qqmlbinding::delayed()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("delayed.qml"));
    QScopedPointer<QQuickItem> item {qobject_cast<QQuickItem*>(c.create())};

    QVERIFY(item != nullptr);

    // objectName is not deferred
    QCOMPARE(item->objectName(), QStringLiteral("c: 10"));

    // constants are never delayed
    QCOMPARE(item->x(), 10.0);
    QCOMPARE(item->y(), 20.0);

    // update on creation
    QCOMPARE(item->property("changeCount").toInt(), 1);
    QCOMPARE(item->property("changeCount2").toInt(), 1);

    QMetaObject::invokeMethod(item.get(), "updateText");
    // doesn't update immediately
    QCOMPARE(item->property("changeCount").toInt(), 1);
    QCOMPARE(item->property("changeCount2").toInt(), 1);

    // only updates once (non-delayed would update twice)
    QTRY_COMPARE(item->property("changeCount").toInt(), 2);
    QTRY_COMPARE(item->property("changeCount2").toInt(), 2);

    item->setProperty("delayed", QVariant::fromValue<bool>(false));
    QCOMPARE(item->property("changeCount"), 2);
    QCOMPARE(item->property("changeCount2"), 2);

    QMetaObject::invokeMethod(item.get(), "resetText");
    QCOMPARE(item->property("changeCount"), 4);
    QCOMPARE(item->property("changeCount2"), 4);

    QMetaObject::invokeMethod(item.get(), "updateText");
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    item->setProperty("delayed", QVariant::fromValue<bool>(true));
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    QMetaObject::invokeMethod(item.get(), "resetText");
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    QMetaObject::invokeMethod(item.get(), "updateText");
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    item->setProperty("delayed", QVariant::fromValue<bool>(false));
    // Intermediate change is ignored
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    item->setProperty("delayed", QVariant::fromValue<bool>(true));
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    QMetaObject::invokeMethod(item.get(), "resetText");
    QCOMPARE(item->property("changeCount"), 6);
    QCOMPARE(item->property("changeCount2"), 6);

    // only updates once (non-delayed would update twice)
    QTRY_COMPARE(item->property("changeCount").toInt(), 7);
    QTRY_COMPARE(item->property("changeCount2").toInt(), 7);

    QMetaObject::invokeMethod(item.get(), "updateText");
    // doesn't update immediately
    QCOMPARE(item->property("changeCount").toInt(), 7);
    QCOMPARE(item->property("changeCount2").toInt(), 7);

    // only updates once (non-delayed would update twice)
    QTRY_COMPARE(item->property("changeCount").toInt(), 8);
    QTRY_COMPARE(item->property("changeCount2").toInt(), 8);
}

void tst_qqmlbinding::bindingOverwriting()
{
    QQmlTestMessageHandler messageHandler;
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindingOverwriting.qml"));
    QScopedPointer<QQuickItem> item {qobject_cast<QQuickItem*>(c.create())};
    QVERIFY(item);
    QCOMPARE(messageHandler.messages().size(), 2);

    QQmlComponent c2(&engine, testFileUrl("bindingOverwriting2.qml"));
    QScopedPointer<QObject> o(c2.create());
    QVERIFY(o);
    QTRY_COMPARE(o->property("i").toInt(), 123);
    QCOMPARE(messageHandler.messages().size(), 3);

    QLoggingCategory::setFilterRules(QString());
}

void tst_qqmlbinding::bindToQmlComponent()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("bindToQMLComponent.qml"));
    QScopedPointer<QObject> root {c.create()};
    QVERIFY(root);
}

// QTBUG-78943
void tst_qqmlbinding::bindingDoesNoWeirdConversion()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("noUnexpectedStringConversion.qml"));
    QScopedPointer<QObject> o {c.create()};
    QVERIFY(o);
    QObject *colorRect = o->findChild<QObject*>("colorRect");
    QVERIFY(colorRect);
    QCOMPARE(qvariant_cast<QColor>(colorRect->property("color")), QColorConstants::Red);
    QObject *colorLabel = o->findChild<QObject*>("colorLabel");
    QCOMPARE(colorLabel->property("text").toString(), QLatin1String("red"));
    QVERIFY(colorLabel);
}

//QTBUG-72442
void tst_qqmlbinding::bindNaNToInt()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nanPropertyToInt.qml"));
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(c.create()));

    QVERIFY(item != nullptr);
    QCOMPARE(item->property("val").toInt(), 0);
}

void tst_qqmlbinding::intOverflow()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("intOverflow.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> obj(c.create());
    QVERIFY(!obj.isNull());
    QCOMPARE(obj->property("b"), 5);
    QCOMPARE(obj->property("a").toDouble(), 1.09951162778e+12);
}

void tst_qqmlbinding::generalizedGroupedProperties()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("generalizedGroupedProperty.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QStringLiteral(
                               "%1:8:29: QML Binding: Unknown name \"root.objectNameChanged\". "
                               "The binding is ignored.").arg(url.toString())));
    QScopedPointer<QObject> root(c.create());
    QVERIFY(!root.isNull());

    QCOMPARE(root->objectName(), QStringLiteral("barrrrr ..."));
    QCOMPARE(root->property("i").toInt(), 2);

    QQmlComponentAttached *rootAttached = qobject_cast<QQmlComponentAttached *>(
                qmlAttachedPropertiesObject<QQmlComponent>(root.data()));
    QVERIFY(rootAttached);
    QCOMPARE(rootAttached->objectName(), QStringLiteral("foo"));

    QQmlBind *child = qvariant_cast<QQmlBind *>(root->property("child"));
    QVERIFY(child);
    QCOMPARE(child->objectName(), QStringLiteral("barrrrr"));
    QQmlComponentAttached *childAttached = qobject_cast<QQmlComponentAttached *>(
                qmlAttachedPropertiesObject<QQmlComponent>(child));
    QVERIFY(childAttached);
    QCOMPARE(childAttached->objectName(), QString());
    QCOMPARE(child->when(), true);
    child->setWhen(false);

    QCOMPARE(root->objectName(), QStringLiteral("foo"));
    QCOMPARE(root->property("i").toInt(), 112);
    QCOMPARE(rootAttached->objectName(), QString());

    QQmlBind *meanChild = qvariant_cast<QQmlBind *>(root->property("meanChild"));
    QVERIFY(meanChild);
    QCOMPARE(meanChild->when(), false);

    // This one is immediate
    QCOMPARE(qvariant_cast<QString>(meanChild->QObject::property("extra")),
             QStringLiteral("foo extra"));

    meanChild->setWhen(true);
    QCOMPARE(qvariant_cast<QString>(meanChild->QObject::property("extra")),
             QStringLiteral("foo extra"));

    QCOMPARE(root->objectName(), QStringLiteral("foo"));
    QCOMPARE(root->property("i").toInt(), 3);
    QCOMPARE(child->objectName(), QStringLiteral("bar"));
    QCOMPARE(childAttached->objectName(), QStringLiteral("bar"));

    child->setWhen(true);
    QCOMPARE(child->objectName(), QStringLiteral("bar"));
    QCOMPARE(root->objectName(), QStringLiteral("bar ..."));
    QCOMPARE(rootAttached->objectName(), QStringLiteral("foo"));
    QCOMPARE(root->property("i").toInt(), 2);

    meanChild->setWhen(false);
    // root->property("i") is now unspecified. Too bad.
    // In fact we restore the binding from before meanChild was activated, but that's
    // not what the user would expect here. We currently don't see that the value has
    // been meddled with.
    // TODO: Fix this. It's not related to generalized grouped properties, though.
    QCOMPARE(root->objectName(), QStringLiteral("barrrrr ..."));
    QCOMPARE(rootAttached->objectName(), QStringLiteral("foo"));
    QCOMPARE(child->objectName(), QStringLiteral("barrrrr"));
    QCOMPARE(childAttached->objectName(), QString());

    child->setWhen(false);
    QCOMPARE(root->objectName(), QStringLiteral("foo"));
    // root->property("i").toInt() is still unspecified.
    QCOMPARE(rootAttached->objectName(), QString());
}

void tst_qqmlbinding::localSignalHandler()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("bindingWithHandler.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    o->setProperty("input", QStringLiteral("abc"));
    QCOMPARE(o->property("output").toString(), QStringLiteral("abc"));
}

void tst_qqmlbinding::whenEvaluatedEarlyEnough()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("whenEvaluatedEarlyEnough.qml"));
    QTest::failOnWarning(QRegularExpression(".*"));
    std::unique_ptr<QObject> root { c.create() };
    root->setProperty("toggle", false); // should not cause warnings
    // until "when" is actually true
    QTest::ignoreMessage(QtMsgType::QtWarningMsg,
                         QRegularExpression(".*QML Binding: Property 'i' does not exist on Item.*"));
    root->setProperty("forceEnable", true);
}

void tst_qqmlbinding::propertiesAttachedToBindingItself()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("propertiesAttachedToBindingItself.qml"));
    QTest::failOnWarning(QRegularExpression(".*"));
    std::unique_ptr<QObject> root { c.create() };
    QVERIFY2(root, qPrintable(c.errorString()));
    // 0 => everything broken; 1 => normal attached properties broken;
    // 2 => Component.onCompleted broken, 3 => everything works
    QTRY_COMPARE(root->property("check").toInt(), 3);
}

void tst_qqmlbinding::toggleEnableProperlyRemembersValues()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("toggleEnableProperlyRemembersValues.qml"));
    std::unique_ptr<QObject> root { c.create() };
    QVERIFY2(root, qPrintable(c.errorString()));
    for (int i = 0; i < 3; ++i) {
        {
            QJSManagedValue arr(root->property("arr"), &e);
            QJSManagedValue func(root->property("func"), &e);
            QCOMPARE(arr.property("length").toInt(), 2);
            QCOMPARE(func.call().toInt(), 1);
        }
        root->setProperty("enabled", true);
        {
            QJSManagedValue arr(root->property("arr"), &e);
            QJSManagedValue func(root->property("func"), &e);
            QCOMPARE(arr.property("length").toInt(), 3);
            QCOMPARE(func.call().toInt(), 2);
        }
        root->setProperty("enabled", false);
    }
}

class PropertyToPropertyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QRectF a READ a WRITE setA NOTIFY aChanged)
    Q_PROPERTY(QRectF b READ b BINDABLE bindableB WRITE default)

    Q_PROPERTY(qreal top READ top WRITE setTop NOTIFY topChanged)
    Q_PROPERTY(qreal bottom READ bottom BINDABLE bindableBottom WRITE default)
    Q_PROPERTY(qreal left READ left WRITE setLeft NOTIFY leftChanged)
    Q_PROPERTY(qreal right READ right BINDABLE bindableRight WRITE default)
public:

    QRectF a() const { return m_a; }
    void setA(const QRectF &a)
    {
        if (m_a == a)
            return;
        m_a = a;
        emit aChanged();
    }

    QRectF b() const { return m_b.value(); }
    QBindable<QRectF> bindableB() { return QBindable<QRectF>(&m_b); }

    qreal top() const { return m_top; }
    void setTop(qreal top)
    {
        if (qFuzzyCompare(m_top, top))
            return;
        m_top = top;
        emit topChanged();
    }

    qreal bottom() const { return m_bottom; }
    QBindable<qreal> bindableBottom() { return QBindable<qreal>(&m_bottom); }

    qreal left() const { return m_left; }
    void setLeft(qreal left)
    {
        if (qFuzzyCompare(m_left, left))
            return;
        m_left = left;
        emit leftChanged();
    }

    qreal right() const { return m_right; }
    QBindable<qreal> bindableRight() { return QBindable<qreal>(&m_right); }

signals:
    void aChanged();
    void topChanged();
    void leftChanged();

private:
    QRectF m_a;
    QProperty<QRectF> m_b;
    qreal m_top = 0;
    QProperty<qreal> m_bottom;
    qreal m_left = 0;
    QProperty<qreal> m_right;
};

void tst_qqmlbinding::qQmlPropertyToPropertyBinding()
{
    using namespace Qt::StringLiterals;

    QScopedPointer<PropertyToPropertyObject> target(new PropertyToPropertyObject);
    QScopedPointer<PropertyToPropertyObject> source(new PropertyToPropertyObject);

    auto installPropertyBinding
            = [&](QObject *targetObject, const QString &targetPropertyName,
                  QObject *sourceObject, const QString &sourcePropertyName) {
        QQmlProperty targetProperty(targetObject, targetPropertyName);
        QQmlProperty sourceProperty(sourceObject, sourcePropertyName);
        QQmlAnyBinding binding
                = QQmlPropertyToPropertyBinding::create(&engine, sourceProperty, targetProperty);
        binding.installOn(targetProperty);
    };

    installPropertyBinding(target.data(), "left"_L1,   source.data(), "a.left"_L1);
    installPropertyBinding(target.data(), "top"_L1,    source.data(), "b.top"_L1);

    installPropertyBinding(target.data(), "right"_L1,  source.data(), "a.right"_L1);
    installPropertyBinding(target.data(), "bottom"_L1, source.data(), "b.bottom"_L1);

    QCOMPARE(target->top(), 0);
    QCOMPARE(target->bottom(), 0);
    QCOMPARE(target->left(), 0);
    QCOMPARE(target->right(), 0);

    source->setA(QRectF(11, 22, 33, 44));
    QCOMPARE(target->top(), 0);
    QCOMPARE(target->bottom(), 0);
    QCOMPARE(target->left(), 11);
    QCOMPARE(target->right(), 11 + 33);

    source->bindableB().setValue(QRectF(55, 66, 77, 88));
    QCOMPARE(target->top(), 66);
    QCOMPARE(target->bottom(), 66 + 88);
    QCOMPARE(target->left(), 11);
    QCOMPARE(target->right(), 11 + 33);
}

void tst_qqmlbinding::qQmlPropertyToPropertyBindingReverse()
{
    using namespace Qt::StringLiterals;

    QScopedPointer<PropertyToPropertyObject> target(new PropertyToPropertyObject);
    QScopedPointer<PropertyToPropertyObject> source(new PropertyToPropertyObject);

    auto installPropertyBinding
            = [&](QObject *targetObject, const QString &targetPropertyName,
                  QObject *sourceObject, const QString &sourcePropertyName) {
        QQmlProperty targetProperty(targetObject, targetPropertyName);
        QQmlProperty sourceProperty(sourceObject, sourcePropertyName);
        QQmlAnyBinding binding
              = QQmlPropertyToPropertyBinding::create(&engine, sourceProperty, targetProperty);
        binding.installOn(targetProperty);
    };

    // QQmlRectFValueType has no setters for left/right/top/bottom.
    // It has setters for x/y/width/height, though.

    installPropertyBinding(target.data(), "a.x"_L1,      source.data(), "left"_L1);
    installPropertyBinding(target.data(), "b.y"_L1,      source.data(), "top"_L1);

    installPropertyBinding(target.data(), "a.width"_L1,  source.data(), "right"_L1);
    installPropertyBinding(target.data(), "b.height"_L1, source.data(), "bottom"_L1);

    QCOMPARE(source->top(), 0);
    QCOMPARE(source->bottom(), 0);
    QCOMPARE(source->left(), 0);
    QCOMPARE(source->right(), 0);

    target->setA(QRectF(0, 22, 0, 44));
    source->setLeft(11);
    source->bindableRight().setValue(33);
    QCOMPARE(target->a(), QRectF(11, 22, 33, 44));

    target->bindableB().setValue(QRectF(55, 0, 77, 0));
    source->setTop(66);
    source->bindableBottom().setValue(88);
    QCOMPARE(target->b(), QRectF(55, 66, 77, 88));
}

void tst_qqmlbinding::delayedBindingDestruction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("delayedBindingDestruction.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);
    QVERIFY(object->objectName().isEmpty());
    QVERIFY(object->property("result").toString().isEmpty());

    const auto verifyDelegate = [&](const QString &expected) {
        QQmlInstantiator *instantiator
                = object->property("instantiator").value<QQmlInstantiator *>();
        QVERIFY(instantiator);
        QCOMPARE(instantiator->object()->objectName(), expected);
    };

    QMetaObject::invokeMethod(object.data(), "toggle");
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QCOMPARE(object->objectName(), QLatin1String("bar"));
    verifyDelegate(QLatin1String("bar"));

    QMetaObject::invokeMethod(object.data(), "toggle");
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QCOMPARE(object->objectName(), QLatin1String("foo"));
    verifyDelegate(QLatin1String("foo"));
}

QTEST_MAIN(tst_qqmlbinding)

#include "tst_qqmlbinding.moc"
