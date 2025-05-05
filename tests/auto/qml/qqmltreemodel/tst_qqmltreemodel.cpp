// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtCore/qregularexpression.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>
#include <QtQuick/private/qquicktreeview_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtLabsQmlModels/private/qqmltreemodel_p.h>

using namespace Qt::Literals::StringLiterals;

class tst_QQmlTreeModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlTreeModel() : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings) {}

private slots:
    void appendToRoot();
    void clear();
    void getRow();
};

void tst_QQmlTreeModel::appendToRoot()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());
    int rowsChangedSignalEmissions = 0;

    const QHash<int, QByteArray> roleNames = model->roleNames();
    QCOMPARE(roleNames.size(), 2);
    QVERIFY(roleNames.values().contains("display"));
    QVERIFY(roleNames.values().contains("decoration"));

    // first top level node
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(0, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("display")).toString(), u"Apple"_s);
    QCOMPARE(model->data(model->index(0, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("display")).toString(), u"Granny Smith"_s);
    QCOMPARE(model->data(model->index(0, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(0, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"red"_s);

    // second top level node
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(1, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(1, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(1, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(1, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // append node to top level
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendNodeToRoot"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 9);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(2, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(2, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(2, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(2, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(2, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);

    // append subtree to top level
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendSubTreeToRoot"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index(3, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("display")).toInt(), 4);
    QCOMPARE(model->data(model->index(3, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 2, QModelIndex()), roleNames.key("display")).toString(), u"Peach"_s);
    QCOMPARE(model->data(model->index(3, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 3, QModelIndex()), roleNames.key("display")).toString(), u"Princess Peach"_s);
    QCOMPARE(model->data(model->index(3, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);
    QCOMPARE(model->data(model->index(3, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.45);
    QCOMPARE(model->data(model->index(3, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"yellow"_s);

    // {3,0} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({3,0}, 0), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index({3,0}, 0), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 1), roleNames.key("display")).toInt(), 5);
    QCOMPARE(model->data(model->index({3,0}, 1), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 2), roleNames.key("display")).toString(), u"Strawberry"_s);
    QCOMPARE(model->data(model->index({3,0}, 2), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 3), roleNames.key("display")).toString(), u"Perry the Berry"_s);
    QCOMPARE(model->data(model->index({3,0}, 3), roleNames.key("decoration")).toString(), u"red"_s);
    QCOMPARE(model->data(model->index({3,0}, 4), roleNames.key("display")).toDouble(), 3.8);
    QCOMPARE(model->data(model->index({3,0}, 4), roleNames.key("decoration")).toString(), u"red"_s);

    // {3,1} is the tree index, 0 is the col index
    QCOMPARE(model->data(model->index({3,1}, 0), roleNames.key("display")).toBool(), false);
    QCOMPARE(model->data(model->index({3,1}, 0), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 1), roleNames.key("display")).toInt(), 6);
    QCOMPARE(model->data(model->index({3,1}, 1), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 2), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index({3,1}, 2), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 3), roleNames.key("display")).toString(), u"Bear Pear"_s);
    QCOMPARE(model->data(model->index({3,1}, 3), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index({3,1}, 4), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index({3,1}, 4), roleNames.key("decoration")).toString(), u"green"_s);

    // Try to append something invalid - an int
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* got int instead"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid1"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Try to append something invalid - fruitName is an []
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* expected the property named"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid2"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Call append with something invalid - the input is an array instead of a simple object.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* row manipulation functions do not support complex rows"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendToRootInvalid3"));
    // nothing gets appended
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 12);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), rowsChangedSignalEmissions);

    // Call append with a node that has an unexpected role;
    // the node should be added and the extra data ignored.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* append: could not find any node at the specified index"));
    QVERIFY(QMetaObject::invokeMethod(view.rootObject(), "appendNodeToRootWithExtraData"));
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 13);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), ++rowsChangedSignalEmissions);

    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("display")).toBool(), true);
    QCOMPARE(model->data(model->index(4, 0, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("display")).toInt(), 1);
    QCOMPARE(model->data(model->index(4, 1, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 2, QModelIndex()), roleNames.key("display")).toString(), u"Pear"_s);
    QCOMPARE(model->data(model->index(4, 2, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 3, QModelIndex()), roleNames.key("display")).toString(), u"Williams"_s);
    QCOMPARE(model->data(model->index(4, 3, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
    QCOMPARE(model->data(model->index(4, 4, QModelIndex()), roleNames.key("display")).toDouble(), 1.5);
    QCOMPARE(model->data(model->index(4, 4, QModelIndex()), roleNames.key("decoration")).toString(), u"green"_s);
}

void tst_QQmlTreeModel::clear()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QSignalSpy columnCountSpy(model, SIGNAL(columnCountChanged()));
    QVERIFY(columnCountSpy.isValid());

    QSignalSpy rowsChangedSpy(model, SIGNAL(rowsChanged()));
    QVERIFY(rowsChangedSpy.isValid());

    QQuickTreeView *treeView = view.rootObject()->property("treeView").value<QQuickTreeView*>();
    QVERIFY(treeView);
    QCOMPARE(treeView->columns(), 5);
    QCOMPARE(treeView->rows(), 2);  // treeView cannot call our treeSize

    model->clear();
    // the rows should be cleared but the columns should not change
    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 0);
    QCOMPARE(columnCountSpy.size(), 0);
    QCOMPARE(rowsChangedSpy.size(), 1);
    // Wait until updatePolish() gets called, which is where the size is recalculated.
    QTRY_COMPARE(treeView->rows(), 0);
    QCOMPARE(treeView->columns(), 5);
}

void tst_QQmlTreeModel::getRow()
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("common.qml")));

    auto *model = view.rootObject()->property("testModel").value<QQmlTreeModel*>();
    QVERIFY(model);

    QCOMPARE(model->columnCount(), 5);
    QCOMPARE(model->treeSize(), 8);

    QVariant treeRow;
    QVariantMap rowValues;

    // Call getRow with an invalid tree index (contains negative number).
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    treeRow = model->getRow(model->index({0,1,-1}, 0));
    QVERIFY(!treeRow.isValid());

    // Call getRow with another invalid tree index (points to nonexistent element).
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".* could not find any node at the specified index"));
    treeRow = model->getRow(model->index({1,2}, 0));
    QVERIFY(!treeRow.isValid());

    // Call getRow with a valid tree index - this is a leaf.
    treeRow = model->getRow(model->index({0,1,0}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 4);
    QCOMPARE(rowValues.value("checked"), true);
    QCOMPARE(rowValues.value("color"), "orange");
    QCOMPARE(rowValues.value("fruitName"), "Navel");
    QCOMPARE(rowValues.value("fruitPrice"), 2.5);
    QCOMPARE(rowValues.value("fruitType"), "Orange");

    // Call getRow with a valid tree index - another leaf.
    treeRow = model->getRow(model->index({1,1}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 6);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "green");
    QCOMPARE(rowValues.value("fruitName"), "Bear Pear");
    QCOMPARE(rowValues.value("fruitPrice"), 1.5);
    QCOMPARE(rowValues.value("fruitType"), "Pear");

    // Call getRow with a valid tree index - from top level
    std::vector<int> treeIndex = {0};
    treeRow = model->getRow(model->index(treeIndex, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 1);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "red");
    QCOMPARE(rowValues.value("fruitName"), "Granny Smith");
    QCOMPARE(rowValues.value("fruitPrice"), 1.5);
    QCOMPARE(rowValues.value("fruitType"), "Apple");

    // Call getRow with a valid tree index - "intermediate node"
    treeRow = model->getRow(model->index({0,1}, 0));
    QVERIFY(treeRow.isValid());
    rowValues = treeRow.toMap();

    QCOMPARE(rowValues.value("amount"), 1);
    QCOMPARE(rowValues.value("checked"), false);
    QCOMPARE(rowValues.value("color"), "yellow");
    QCOMPARE(rowValues.value("fruitName"), "Cavendish");
    QCOMPARE(rowValues.value("fruitPrice"), 3.5);
    QCOMPARE(rowValues.value("fruitType"), "Banana");
}

QTEST_MAIN(tst_QQmlTreeModel)

#include "tst_qqmltreemodel.moc"
