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
    void getRow();
};

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
