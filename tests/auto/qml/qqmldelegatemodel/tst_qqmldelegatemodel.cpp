// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qsortfilterproxymodel.h>
#include <QtCore/QConcatenateTablesProxyModel>
#include <QtCore/qtimer.h>
#include <QtGui/QStandardItemModel>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitemview_p_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtTest/QSignalSpy>

#include <forward_list>

using namespace QQuickVisualTestUtils;

class tst_QQmlDelegateModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlDelegateModel();

private slots:
    void resettingRolesRespected();
    void resetInQAIMConstructor();
    void reset();
    void valueWithoutCallingObjectFirst_data();
    void valueWithoutCallingObjectFirst();
    void qtbug_86017();
    void filterOnGroup_removeWhenCompleted();
    void contextAccessedByHandler();
    void redrawUponColumnChange();
    void nestedDelegates();
    void universalModelData();
    void typedModelData();
    void requiredModelData();
    void nestedRequired();
    void overriddenModelData();
    void deleteRace();
    void persistedItemsStayInCache();
    void unknownContainersAsModel();
    void doNotUnrefObjectUnderConstruction();
    void clearCacheDuringInsertion();
    void viewUpdatedOnDelegateChoiceAffectingRoleChange();
    void proxyModelWithDelayedSourceModelInListView();
    void delegateChooser();

    void delegateModelAccess_data();
    void delegateModelAccess();
};

class BaseAbstractItemModel : public QAbstractItemModel
{
public:
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return QModelIndex();

        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;

        return mValues.size();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;

        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        return mValues.at(index.row());
    }

protected:
    QVector<QString> mValues;
};

class AbstractItemModel : public BaseAbstractItemModel
{
    Q_OBJECT
public:
    AbstractItemModel()
    {
        for (int i = 0; i < 3; ++i)
            mValues.append(QString::fromLatin1("Item %1").arg(i));
    }
};

tst_QQmlDelegateModel::tst_QQmlDelegateModel()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<AbstractItemModel>("Test", 1, 0, "AbstractItemModel");
}

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return 1;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch (role) {
        case 0:
            return QString("foo: %1, %2").arg(index.column()).arg(index.row());
        case 1:
            return 42;
        default:
            break;
        }

        return QVariant();
    }

    Q_INVOKABLE void change() { beginResetModel(); toggle = !toggle; endResetModel(); }

    QHash<int, QByteArray> roleNames() const override
    {
        if (toggle)
            return { {0, "foo"} };
        else
            return { {1, "bar"} };
    }

    bool toggle = true;
};

void tst_QQmlDelegateModel::resettingRolesRespected()
{
    auto model = std::make_unique<TableModel>();
    QQmlApplicationEngine engine;
    engine.setInitialProperties({ {"model", QVariant::fromValue(model.get()) }} );
    engine.load(testFileUrl("resetModelData.qml"));
    QTRY_VERIFY(!engine.rootObjects().isEmpty());
    QObject *root = engine.rootObjects().constFirst();
    QVERIFY(!root->property("success").toBool());
    model->change();
    QTRY_VERIFY_WITH_TIMEOUT(root->property("success").toBool(), 100);
}

class ResetInConstructorModel : public BaseAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    ResetInConstructorModel()
    {
        beginResetModel();
        QTimer::singleShot(0, this, &ResetInConstructorModel::finishReset);
    }

private:
    void finishReset()
    {
        mValues.append("First");
        endResetModel();
    }
};

void tst_QQmlDelegateModel::resetInQAIMConstructor()
{
    qmlRegisterTypesAndRevisions<ResetInConstructorModel>("Test", 1);

    QQuickApplicationHelper helper(this, "resetInQAIMConstructor.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *listView = window->property("listView").value<QQuickListView *>();
    QVERIFY(listView);
    QTRY_VERIFY_WITH_TIMEOUT(listView->itemAtIndex(0), 100);
    QQuickItem *firstDelegateItem = listView->itemAtIndex(0);
    QVERIFY(firstDelegateItem);
    QCOMPARE(firstDelegateItem->property("display").toString(), "First");
}

class ResettableModel : public BaseAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    ResettableModel()
    {
        mValues.append("First");
    }

    void callBeginResetModel()
    {
        beginResetModel();
        mValues.clear();
    }

    void appendData()
    {
        mValues.append(QString::fromLatin1("Item %1").arg(mValues.size()));
    }

    void callEndResetModel()
    {
        endResetModel();
    }
};

// Tests that everything works as expected when calling beginResetModel/endResetModel
// after the QAIM subclass constructor has run.
void tst_QQmlDelegateModel::reset()
{
    qmlRegisterTypesAndRevisions<ResettableModel>("Test", 1);

    QQuickApplicationHelper helper(this, "reset.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *listView = window->property("listView").value<QQuickListView *>();
    QVERIFY(listView);
    QQuickItem *firstDelegateItem = listView->itemAtIndex(0);
    QVERIFY(firstDelegateItem);
    QCOMPARE(firstDelegateItem->property("display").toString(), "First");

    const auto delegateModel = QQuickItemViewPrivate::get(listView)->model;
    QSignalSpy rootIndexChangedSpy(delegateModel, SIGNAL(rootIndexChanged()));
    QVERIFY(rootIndexChangedSpy.isValid());

    auto *model = listView->model().value<ResettableModel *>();
    model->callBeginResetModel();
    model->appendData();
    model->callEndResetModel();
    // This is verifies that handleModelReset isn't called
    // more than once during this process, since it unconditionally emits rootIndexChanged.
    QCOMPARE(rootIndexChangedSpy.count(), 1);

    QTRY_VERIFY_WITH_TIMEOUT(listView->itemAtIndex(0), 100);
    firstDelegateItem = listView->itemAtIndex(0);
    QVERIFY(firstDelegateItem);
    QCOMPARE(firstDelegateItem->property("display").toString(), "Item 0");
}

void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst_data()
{
    QTest::addColumn<QUrl>("qmlFileUrl");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("role");
    QTest::addColumn<QVariant>("expectedValue");

    QTest::addRow("integer") << testFileUrl("integerModel.qml")
        << 50 << QString::fromLatin1("modelData") << QVariant(50);
    QTest::addRow("ListModel") << testFileUrl("listModel.qml")
        << 1 << QString::fromLatin1("name") << QVariant(QLatin1String("Item 1"));
    QTest::addRow("QAbstractItemModel") << testFileUrl("abstractItemModel.qml")
        << 1 << QString::fromLatin1("display") << QVariant(QLatin1String("Item 1"));
}

// Tests that it's possible to call variantValue() without creating
// costly delegate items first via object().
void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst()
{
    QFETCH(const QUrl, qmlFileUrl);
    QFETCH(const int, index);
    QFETCH(const QString, role);
    QFETCH(const QVariant, expectedValue);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(qmlFileUrl);
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());
    QVERIFY(model);
    QCOMPARE(model->variantValue(index, role), expectedValue);
}

void tst_QQmlDelegateModel::qtbug_86017()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("qtbug_86017.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QTRY_VERIFY(component.isReady());
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());

    QVERIFY(model);
    QCOMPARE(model->count(), 2);
    QCOMPARE(model->filterGroup(), "selected");
}

void tst_QQmlDelegateModel::filterOnGroup_removeWhenCompleted()
{
    QQuickView view(testFileUrl("removeFromGroup.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    QVERIFY(root);
    QQmlDelegateModel *model = root->findChild<QQmlDelegateModel*>();
    QVERIFY(model);
    QVERIFY(QTest::qWaitFor([=]{ return model->count() == 2; }));
}

void tst_QQmlDelegateModel::contextAccessedByHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("contextAccessedByHandler.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->property("works").toBool());
}

void tst_QQmlDelegateModel::redrawUponColumnChange()
{
    QStandardItemModel m1;
    m1.appendRow({
            new QStandardItem("Banana"),
            new QStandardItem("Coconut"),
    });

    QQuickView view(testFileUrl("redrawUponColumnChange.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    root->setProperty("model", QVariant::fromValue<QObject *>(&m1));

    QObject *item = root->property("currentItem").value<QObject *>();
    QVERIFY(item);
    QCOMPARE(item->property("text").toString(), "Banana");

    QVERIFY(root);
    m1.removeColumn(0);

    QCOMPARE(item->property("text").toString(), "Coconut");
}

void tst_QQmlDelegateModel::nestedDelegates()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nestedDelegates.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQuickItem *item = qobject_cast<QQuickItem *>(o.data());
    QCOMPARE(item->childItems().size(), 2);
    for (QQuickItem *child : item->childItems()) {
        if (child->objectName() != QLatin1String("loader"))
            continue;

        QCOMPARE(child->childItems().size(), 1);
        QQuickItem *timeMarks = child->childItems().at(0);
        const QList<QQuickItem *> children = timeMarks->childItems();
        QCOMPARE(children.size(), 2);

        // One of them is the repeater, the other one is the rectangle
        QVERIFY(children.at(0)->objectName() == QLatin1String("zap")
                 || children.at(1)->objectName() == QLatin1String("zap"));
        QVERIFY(children.at(0)->objectName().isEmpty() || children.at(1)->objectName().isEmpty());

        return; // loader found
    }
    QFAIL("Loader not found");
}

void tst_QQmlDelegateModel::universalModelData()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("universalModelData.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    for (int i = 0; i < 6; ++i) {
        delegateModel->setProperty("n", i);
        QObject *delegate = delegateModel->object(0);
        QObject *modelItem = delegate->property("modelSelf").value<QObject *>();
        QVERIFY(modelItem != nullptr);
        switch (i) {
        case 0: {
            // list model with 1 role
            QCOMPARE(delegate->property("modelA"), QStringLiteral("a"));
            QVERIFY(!delegate->property("modelDataA").isValid());
            QCOMPARE(delegate->property("modelDataSelf"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelModelData"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelAnonymous"), QStringLiteral("a"));
            break;
        }
        case 1: {
            // list model with 2 roles
            QCOMPARE(delegate->property("modelA"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelDataA"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelDataSelf"), QVariant::fromValue(modelItem));
            QCOMPARE(delegate->property("modelModelData"), QVariant::fromValue(modelItem));
            QCOMPARE(delegate->property("modelAnonymous"), QVariant::fromValue(modelItem));
            break;
        }
        case 2: {
            // JS array of objects
            QCOMPARE(delegate->property("modelA"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelDataA"), QStringLiteral("a"));

            // Do the comparison in QVariantMap. The values get converted back and forth a
            // few times, making any JavaScript equality comparison impossible.
            // This is only due to test setup, though.
            const QVariantMap modelData = delegate->property("modelDataSelf").value<QVariantMap>();
            QVERIFY(!modelData.isEmpty());
            QCOMPARE(delegate->property("modelModelData").value<QVariantMap>(), modelData);
            QCOMPARE(delegate->property("modelAnonymous").value<QVariantMap>(), modelData);
            break;
        }
        case 3: {
            // string list
            QVERIFY(!delegate->property("modelA").isValid());
            QVERIFY(!delegate->property("modelDataA").isValid());
            QCOMPARE(delegate->property("modelDataSelf"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelModelData"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelAnonymous"), QStringLiteral("a"));
            break;
        }
        case 4: {
            // single object
            QCOMPARE(delegate->property("modelA"), QStringLiteral("a"));
            QCOMPARE(delegate->property("modelDataA"), QStringLiteral("a"));
            QObject *modelData = delegate->property("modelDataSelf").value<QObject *>();
            QVERIFY(modelData != nullptr);
            QCOMPARE(delegate->property("modelModelData"), QVariant::fromValue(modelData));
            QCOMPARE(delegate->property("modelAnonymous"), QVariant::fromValue(modelData));
            break;
        }
        case 5: {
            // a number
            QVERIFY(!delegate->property("modelA").isValid());
            QVERIFY(!delegate->property("modelDataA").isValid());
            const QVariant modelData = delegate->property("modelDataSelf");

            // This is int on 32bit systems because qsizetype fits into int there.
            // On 64bit systems it's double because qsizetype doesn't fit into int.
            if (sizeof(qsizetype) > sizeof(int))
                QCOMPARE(modelData.metaType(), QMetaType::fromType<double>());
            else
                QCOMPARE(modelData.metaType(), QMetaType::fromType<int>());

            QCOMPARE(modelData.value<int>(), 0);
            QCOMPARE(delegate->property("modelModelData"), modelData);
            QCOMPARE(delegate->property("modelAnonymous"), modelData);
            break;
        }
        default:
            QFAIL("wrong model number");
            break;
        }
    }

}

void tst_QQmlDelegateModel::typedModelData()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("typedModelData.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    for (int i = 0; i < 4; ++i) {
        if (i == 0) {
            for (int j = 0; j < 3; ++j) {
                QTest::ignoreMessage(
                    QtWarningMsg,
                    "Could not find any constructor for value type QQmlPointFValueType "
                    "to call with value QVariant(double, 11)");
            }

            QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + ":62:9: Unable to assign double to QPointF"));
            QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + ":61:9: Unable to assign double to QPointF"));
        }

        delegateModel->setProperty("n", i);
        QObject *delegate = delegateModel->object(0);
        QVERIFY(delegate);
        const QPointF modelItem = delegate->property("modelSelf").value<QPointF>();
        switch (i) {
        case 0: {
            // list model with 1 role.
            // Does not work, for the most part, because the model is singular
            QCOMPARE(delegate->property("modelX"), 11.0);
            QCOMPARE(delegate->property("modelDataX"), 0.0);
            QCOMPARE(delegate->property("modelSelf"), QPointF(11.0, 0.0));
            QCOMPARE(delegate->property("modelDataSelf"), QPointF());
            QCOMPARE(delegate->property("modelModelData"), QPointF());
            QCOMPARE(delegate->property("modelAnonymous"), QPointF());
            break;
        }
        case 1: {
            // list model with 2 roles
            QCOMPARE(delegate->property("modelX"), 13.0);
            QCOMPARE(delegate->property("modelDataX"), 13.0);
            QCOMPARE(delegate->property("modelSelf"), QVariant::fromValue(modelItem));
            QCOMPARE(delegate->property("modelDataSelf"), QVariant::fromValue(modelItem));
            QCOMPARE(delegate->property("modelModelData"), QVariant::fromValue(modelItem));
            QCOMPARE(delegate->property("modelAnonymous"), QVariant::fromValue(modelItem));
            break;
        }
        case 2: {
            // JS array of objects
            QCOMPARE(delegate->property("modelX"), 17.0);
            QCOMPARE(delegate->property("modelDataX"), 17.0);

            const QPointF modelData = delegate->property("modelDataSelf").value<QPointF>();
            QCOMPARE(modelData, QPointF(17, 18));
            QCOMPARE(delegate->property("modelSelf"), QVariant::fromValue(modelData));
            QCOMPARE(delegate->property("modelModelData").value<QPointF>(), modelData);
            QCOMPARE(delegate->property("modelAnonymous").value<QPointF>(), modelData);
            break;
        }
        case 3: {
            // single object
            QCOMPARE(delegate->property("modelX"), 21);
            QCOMPARE(delegate->property("modelDataX"), 21);
            const QPointF modelData = delegate->property("modelDataSelf").value<QPointF>();
            QCOMPARE(modelData, QPointF(21, 22));
            QCOMPARE(delegate->property("modelSelf"), QVariant::fromValue(modelData));
            QCOMPARE(delegate->property("modelModelData"), QVariant::fromValue(modelData));
            QCOMPARE(delegate->property("modelAnonymous"), QVariant::fromValue(modelData));
            break;
        }
        default:
            QFAIL("wrong model number");
            break;
        }
    }

}

void tst_QQmlDelegateModel::requiredModelData()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("requiredModelData.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    for (int i = 0; i < 4; ++i) {
        delegateModel->setProperty("n", i);
        QObject *delegate = delegateModel->object(0);
        QVERIFY(delegate);
        const QVariant a = delegate->property("a");
        QCOMPARE(a.metaType(), QMetaType::fromType<QString>());
        QCOMPARE(a.toString(), QLatin1String("a"));
    }
}

void tst_QQmlDelegateModel::nestedRequired()
{
    QQmlEngine engine;

    const QUrl url = testFileUrl("nestedRequired.qml");

    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qvariant_cast<QQmlDelegateModel *>(o->property("m"));
    QVERIFY(delegateModel);

    QObject *delegate1 = delegateModel->object(0);
    QVERIFY(delegate1);
    QCOMPARE(delegate1->objectName(), QLatin1String("one"));

    QObject *delegate2= delegateModel->object(1);
    QVERIFY(delegate2);
    QCOMPARE(delegate2->objectName(), QLatin1String("two"));

    QQmlDelegateModel *delegateModel2 = qvariant_cast<QQmlDelegateModel *>(o->property("n"));
    QVERIFY(delegateModel2);

    QObject *delegate3 = delegateModel2->object(0);
    QVERIFY(delegate3);
    QCOMPARE(delegate3->objectName(), QLatin1String("three"));

    QObject *delegate4 = delegateModel2->object(1);
    QVERIFY(delegate4);
    QCOMPARE(delegate4->objectName(), QLatin1String("four"));

    QQmlDelegateModel *delegateModel3 = qvariant_cast<QQmlDelegateModel *>(o->property("o"));
    QVERIFY(delegateModel3);

    QTest::ignoreMessage(
            QtInfoMsg,
            qPrintable(url.toString()
                       + QLatin1String(":50:9: QML Component: Cannot create delegate")));
    QTest::ignoreMessage(
            QtWarningMsg,
            qPrintable(url.toString()
                       + QLatin1String(":13:9: Required property control was not initialized")));
    QObject *delegate5 = delegateModel3->object(0);

    QEXPECT_FAIL("", "object should not be created with required property unset", Continue);
    QVERIFY(!delegate5);
}

void tst_QQmlDelegateModel::overriddenModelData()
{
    QTest::failOnWarning(QRegularExpression(
            "Final member [^ ]+ is overridden in class [^\\.]+. The override won't be used."));

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("overriddenModelData.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    for (int i = 0; i < 3; ++i) {
        delegateModel->setProperty("n", i);
        QObject *delegate = delegateModel->object(0);
        QVERIFY(delegate);

        if (i == 1 || i == 2) {
            // You can actually not override if the model is a QObject or a JavaScript array.
            // Someone is certainly relying on this.
            // We need to find a migration mechanism to fix it.
            QCOMPARE(delegate->objectName(), QLatin1String(" 0 0  e 0"));
        } else {
            QCOMPARE(delegate->objectName(), QLatin1String("a b c d e f"));
        }
    }
}

void tst_QQmlDelegateModel::deleteRace()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("deleteRace.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QTRY_COMPARE(o->property("count").toInt(), 2);
    QTRY_COMPARE(o->property("count").toInt(), 0);
}

void tst_QQmlDelegateModel::persistedItemsStayInCache()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("persistedItemsCache.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    const QVariant properyListModel = object->property("testListModel");
    QQmlListModel *listModel = qvariant_cast<QQmlListModel *>(properyListModel);
    QVERIFY(listModel);
    QTRY_COMPARE(object->property("createCount").toInt(), 3);
    listModel->clear();
    QTRY_COMPARE(object->property("destroyCount").toInt(), 3);
}

Q_DECLARE_SEQUENTIAL_CONTAINER_METATYPE(std::forward_list)
void tst_QQmlDelegateModel::unknownContainersAsModel()
{
    QQmlEngine engine;

    QQmlComponent modelComponent(&engine);
    modelComponent.setData("import QtQml.Models\nDelegateModel {}\n", QUrl());
    QCOMPARE(modelComponent.status(), QQmlComponent::Ready);

    QScopedPointer<QObject> o(modelComponent.create());
    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel*>(o.data());
    QVERIFY(delegateModel);

    QQmlComponent delegateComponent(&engine);
    delegateComponent.setData("import QtQml\nQtObject { objectName: modelData }\n", QUrl());
    QCOMPARE(delegateComponent.status(), QQmlComponent::Ready);

    delegateModel->setDelegate(&delegateComponent);

    QList<QJsonObject> json;
    for (int i = 0; i < 10; ++i)
        json.append(QJsonObject({{"foo", i}}));

    // Recognized as list
    delegateModel->setModel(QVariant::fromValue(json));
    QObject *obj = delegateModel->object(0, QQmlIncubator::Synchronous);
    QVERIFY(obj);
    QCOMPARE(delegateModel->count(), 10);
    QCOMPARE(delegateModel->model().metaType(), QMetaType::fromType<QList<QJsonObject>>());

    QVERIFY(qMetaTypeId<std::forward_list<int>>() > 0);
    std::forward_list<int> mess;
    mess.push_front(4);
    mess.push_front(5);
    mess.push_front(6);

    // Converted into QVariantList
    delegateModel->setModel(QVariant::fromValue(mess));
    obj = delegateModel->object(0, QQmlIncubator::Synchronous);
    QVERIFY(obj);
    QCOMPARE(delegateModel->count(), 3);
    QCOMPARE(delegateModel->model().metaType(), QMetaType::fromType<QVariantList>());
}

void tst_QQmlDelegateModel::doNotUnrefObjectUnderConstruction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("modifyObjectUnderConstruction.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QTRY_COMPARE(object->property("testModel").toInt(), 0);
}

void tst_QQmlDelegateModel::clearCacheDuringInsertion()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("clearCacheDuringInsertion.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QTRY_COMPARE(object->property("testModel").toInt(), 0);
}

void tst_QQmlDelegateModel::viewUpdatedOnDelegateChoiceAffectingRoleChange()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("viewUpdatedOnDelegateChoiceAffectingRoleChange.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QQuickItem *listview = object->findChild<QQuickItem *>("listview");
    QVERIFY(listview);
    QTRY_VERIFY(listview->property("count").toInt() > 0);
    bool returnedValue = false;
    QMetaObject::invokeMethod(object.get(), "verify", Q_RETURN_ARG(bool, returnedValue));
    QVERIFY(returnedValue);
    returnedValue = false;

    object->setProperty("triggered", "true");
    QTRY_VERIFY(listview->property("count").toInt() > 0);
    QMetaObject::invokeMethod(object.get(), "verify", Q_RETURN_ARG(bool, returnedValue));
    QVERIFY(returnedValue);
}

class ProxySourceModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit ProxySourceModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        for (int i = 0; i < rows; ++i) {
            beginInsertRows(QModelIndex(), i, i);
            endInsertRows();
        }
    }

    ~ProxySourceModel() override = default;

    int rowCount(const QModelIndex &) const override
    {
        return rows;
    }

    QVariant data(const QModelIndex &, int ) const override
    {
        return "Hello";
    }

    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles = QAbstractListModel::roleNames();
        roles[Qt::UserRole + 1] = "Name";

        return roles;
    }

    static const int rows = 1;
};

class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QAbstractItemModel *sourceModel READ sourceModel WRITE setSourceModel)

public:
    explicit ProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

    ~ProxyModel() override = default;
};

// Checks that the correct amount of delegates are created when using a proxy
// model whose source model is set after a delay.
void tst_QQmlDelegateModel::proxyModelWithDelayedSourceModelInListView()
{
    QTest::failOnWarning();

    qmlRegisterTypesAndRevisions<ProxySourceModel>("Test", 1);
    qmlRegisterTypesAndRevisions<ProxyModel>("Test", 1);

    QQuickApplicationHelper helper(this, "proxyModelWithDelayedSourceModelInListView.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *listView = window->property("listView").value<QQuickListView *>();
    QVERIFY(listView);
    const auto delegateModel = QQuickItemViewPrivate::get(listView)->model;
    QTRY_COMPARE(listView->count(), 1);
}

void tst_QQmlDelegateModel::delegateChooser()
{
    QQuickApplicationHelper helper(this, "delegatechooser.qml");
    QVERIFY2(helper.ready, helper.failureMessage());

    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickListView *lv = window->findChild<QQuickListView *>();
    QVERIFY(lv);

    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel *>(QQuickItemViewPrivate::get(lv)->model);
    QVERIFY(model);
    QQmlAdaptorModel *adaptorModel = &QQmlDelegateModelPrivate::get(model)->m_adaptorModel;
    QVERIFY(adaptorModel);

    const int lvCount = lv->count();
    QCOMPARE(lvCount, model->count());

    for (int i = 0; i < lvCount; ++i) {
        QQuickRectangle *rectangle = qobject_cast<QQuickRectangle *>(QQuickVisualTestUtils::findViewDelegateItem(lv, i));
        QVERIFY(rectangle);
        QCOMPARE(rectangle->color(), QColor::fromString(adaptorModel->value(adaptorModel->indexAt(i, 0), "modelData").toString()));
    }
}

namespace Model {
Q_NAMESPACE
QML_ELEMENT
enum Kind : qint8
{
    None = -1,
    Singular,
    List,
    Array,
    Object
};
Q_ENUM_NS(Kind)
}

namespace Delegate {
Q_NAMESPACE
QML_ELEMENT
enum Kind : qint8
{
    None = -1,
    Untyped,
    Typed
};
Q_ENUM_NS(Kind)
}

template<typename Enum>
const char *enumKey(Enum value) {
    const QMetaObject *mo = qt_getEnumMetaObject(value);
    const QMetaEnum metaEnum = mo->enumerator(mo->indexOfEnumerator(qt_getEnumName(value)));
    return metaEnum.valueToKey(value);
}

void tst_QQmlDelegateModel::delegateModelAccess_data()
{
    QTest::addColumn<QQmlDelegateModel::DelegateModelAccess>("access");
    QTest::addColumn<Model::Kind>("modelKind");
    QTest::addColumn<Delegate::Kind>("delegateKind");

    using Access = QQmlDelegateModel::DelegateModelAccess;
    for (auto access : { Access::Qt5ReadWrite, Access::ReadOnly, Access::ReadWrite }) {
        for (auto model : { Model::Singular, Model::List, Model::Array, Model::Object }) {
            for (auto delegate : { Delegate::Untyped, Delegate::Typed }) {
                QTest::addRow("%s-%s-%s", enumKey(access), enumKey(model), enumKey(delegate))
                        << access << model << delegate;
            }
        }
    }
}

void tst_QQmlDelegateModel::delegateModelAccess()
{
    static const bool initialized = []() {
        qmlRegisterNamespaceAndRevisions(&Model::staticMetaObject, "Test", 1);
        qmlRegisterNamespaceAndRevisions(&Delegate::staticMetaObject, "Test", 1);
        return true;
    }();
    QVERIFY(initialized);

    QFETCH(QQmlDelegateModel::DelegateModelAccess, access);
    QFETCH(Model::Kind, modelKind);
    QFETCH(Delegate::Kind, delegateKind);

    QQmlEngine engine;
    const QUrl url = testFileUrl("delegateModelAccess.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> object(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(object.data());
    QVERIFY(delegateModel);

    if (delegateKind == Delegate::Untyped && modelKind == Model::Array)
        QSKIP("Properties of objects in arrays are not exposed as context properties");

    if (access == QQmlDelegateModel::ReadOnly) {
        const QRegularExpression message(
                url.toString() + ":[0-9]+: TypeError: Cannot assign to read-only property \"x\"");

        QTest::ignoreMessage(QtWarningMsg, message);
        if (delegateKind == Delegate::Untyped)
            QTest::ignoreMessage(QtWarningMsg, message);
    }

    object->setProperty("delegateModelAccess", access);
    object->setProperty("modelIndex", modelKind);
    object->setProperty("delegateIndex", delegateKind);

    QObject *delegate = delegateModel->object(0);
    QVERIFY(delegate);

    const bool modelWritable = access != QQmlDelegateModel::ReadOnly;
    const bool immediateWritable = (delegateKind == Delegate::Untyped)
            ? access != QQmlDelegateModel::ReadOnly
            : access == QQmlDelegateModel::ReadWrite;

    double expected = 11;

    QCOMPARE(delegate->property("immediateX").toDouble(), expected);
    QCOMPARE(delegate->property("modelX").toDouble(), expected);

    if (modelWritable)
        expected = 3;

    QMetaObject::invokeMethod(delegate, "writeThroughModel");
    QCOMPARE(delegate->property("immediateX").toDouble(), expected);
    QCOMPARE(delegate->property("modelX").toDouble(), expected);

    if (immediateWritable)
        expected = 1;

    QMetaObject::invokeMethod(delegate, "writeImmediate");

    // Writes to required properties always succeed, but might not be propagated to the model
    QCOMPARE(delegate->property("immediateX").toDouble(),
             delegateKind == Delegate::Untyped ? expected : 1);

    QCOMPARE(delegate->property("modelX").toDouble(), expected);
}

QTEST_MAIN(tst_QQmlDelegateModel)

#include "tst_qqmldelegatemodel.moc"
