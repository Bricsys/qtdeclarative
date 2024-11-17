// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuickTest/QtQuickTest>

#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquicksafearea_p.h>

#if defined(Q_OS_MACOS)
#include <AppKit/NSView.h>
#endif

class tst_QQuickSafeArea : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickSafeArea()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
    }

private slots:
    void init() override;
    void cleanup();

    void properties();

#if defined(Q_OS_MACOS)
    void margins_data();
    void margins();
#endif

    void additionalMargins();
    void updateFlipFlop();

    void independentMargins_data();
    void independentMargins();

    void bindingLoop();
    void bindingLoopApplicationWindow();

    void safeAreaReuse();

private:
    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

void tst_QQuickSafeArea::init()
{
    QQmlDataTest::init();

    auto loadTestFile = [this](const QString &testName) -> bool {
        const auto testUrl = testFileUrl(testName + ".qml");
        if (QFileInfo::exists(QQmlFile::urlToLocalFileOrQrc(testUrl))) {
            m_engine.reset(new QQmlApplicationEngine(testUrl));
            return true;
        } else {
            return false;
        }
    };

    QString testFunction = QTest::currentTestFunction();
    if (auto *dataTag = QTest::currentDataTag()) {
        if (loadTestFile(testFunction % QChar('_') % dataTag)) {
            QVERIFY(m_engine->rootObjects().size() > 0);
            return;
        }
    }

    if (loadTestFile(testFunction))
        QVERIFY(m_engine->rootObjects().size() > 0);
}

void tst_QQuickSafeArea::cleanup()
{
    m_engine.reset(nullptr);
}

void tst_QQuickSafeArea::properties()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QCOMPARE(window->property("margins").metaType(), QMetaType::fromType<QMarginsF>());
    QCOMPARE(window->property("marginsTop").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsLeft").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsRight").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(window->property("marginsBottom").metaType(), QMetaType::fromType<qreal>());

    auto *item = window->findChild<QQuickItem*>("item");
    QVERIFY(item);
    QCOMPARE(item->property("margins").metaType(), QMetaType::fromType<QMarginsF>());
    QCOMPARE(item->property("marginsTop").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsLeft").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsRight").metaType(), QMetaType::fromType<qreal>());
    QCOMPARE(item->property("marginsBottom").metaType(), QMetaType::fromType<qreal>());
}

#if defined(Q_OS_MACOS)

static constexpr NSEdgeInsets additionalInsets = { 10, 20, 30, 40 };

void tst_QQuickSafeArea::margins_data()
{
    QTest::addColumn<QString>("itemName");
    QTest::addColumn<QMarginsF>("expectedMargins");

    QTest::newRow("fill") << "fillItem" << QMarginsF(
        additionalInsets.left, additionalInsets.top,
        additionalInsets.right, additionalInsets.bottom
    );

    QTest::newRow("top") << "topItem" << QMarginsF(0, additionalInsets.top, 0, 0);
    QTest::newRow("left") << "leftItem" << QMarginsF(additionalInsets.left, 0, 0, 0);
    QTest::newRow("right") << "rightItem" << QMarginsF(0, 0, additionalInsets.right, 0);
    QTest::newRow("bottom") << "bottomItem" << QMarginsF(0, 0, 0, additionalInsets.bottom);
    QTest::newRow("center") << "centerItem" << QMarginsF(0, 0, 0, 0);

    QTest::newRow("topChild") << "topChildItem" <<  QMarginsF(0, additionalInsets.top - 3, 0, 0);
    QTest::newRow("leftChild") << "leftChildItem" << QMarginsF(additionalInsets.left - 3, 0, 0, 0);
    QTest::newRow("rightChild") << "rightChildItem" << QMarginsF(0, 0, additionalInsets.right - 3, 0);
    QTest::newRow("bottomChild") << "bottomChildItem" << QMarginsF(0, 0, 0, additionalInsets.bottom - 3);
    QTest::newRow("centerChild") << "centerChildItem" << QMarginsF(0, 0, 0, 0);
}

void tst_QQuickSafeArea::margins()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QCOMPARE(window->property("margins").value<QMarginsF>(), QMarginsF());

    auto *fillItem = window->findChild<QQuickItem*>("fillItem");
    QVERIFY(fillItem);

    QCOMPARE(fillItem->property("margins").value<QMarginsF>(), QMarginsF());

    // Mock changes on the QWindow level by adjusting the NSView
    auto *view = reinterpret_cast<NSView*>(window->winId());
    view.additionalSafeAreaInsets = additionalInsets;

    QTRY_COMPARE(window->property("margins").value<QMarginsF>(),
        QMarginsF(additionalInsets.left, additionalInsets.top,
            additionalInsets.right, additionalInsets.bottom));

    QFETCH(QString, itemName);

    auto *item = window->findChild<QQuickItem*>(itemName);
    QVERIFY(item);

    QFETCH(QMarginsF, expectedMargins);
    QCOMPARE(item->property("margins").value<QMarginsF>(), expectedMargins);
}
#endif

void tst_QQuickSafeArea::additionalMargins()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QCOMPARE(window->property("margins").value<QMarginsF>(),
        QMarginsF(20, 10, 40, 30));

    auto *additionalItem = window->findChild<QQuickItem*>("additionalItem");
    QCOMPARE(additionalItem->property("margins").value<QMarginsF>(),
        QMarginsF(120, 110, 140, 130));

    auto *additionalChild = additionalItem->findChild<QQuickItem*>("additionalChild");
    QCOMPARE(additionalChild->property("margins").value<QMarginsF>(),
        QMarginsF(117, 107, 137, 127));

    auto *additionalSibling = window->findChild<QQuickItem*>("additionalSibling");
    QCOMPARE(additionalSibling->property("margins").value<QMarginsF>(),
        QMarginsF(20, 10, 40, 30));
}

void tst_QQuickSafeArea::independentMargins_data()
{
    QTest::addColumn<QString>("itemName");
    QTest::addColumn<QMarginsF>("expectedMargins");

    QTest::newRow("top") << "topMarginItem" << QMarginsF(0, 50, 0, 0);
    QTest::newRow("left") << "leftMarginItem" << QMarginsF(50, 0, 0, 0);
    QTest::newRow("right") << "rightMarginItem" << QMarginsF(0, 0, 50, 0);
    QTest::newRow("bottom") << "bottomMarginItem" << QMarginsF(0, 0, 0, 50);
}

void tst_QQuickSafeArea::independentMargins()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QFETCH(QString, itemName);

    auto *item = window->findChild<QQuickItem*>(itemName);
    QVERIFY(item);

    QFETCH(QMarginsF, expectedMargins);
    QCOMPARE(item->property("margins").value<QMarginsF>(), expectedMargins);
}

void tst_QQuickSafeArea::updateFlipFlop()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QSignalSpy widthChangedSpy(window, SIGNAL(itemWidthChanged()));
    QSignalSpy marginChangeSpy(window, SIGNAL(safeAreaRightMarginChanged()));
    window->resize(window->width() - 1, window->height());
    QTRY_COMPARE(widthChangedSpy.count(), 1);
    QCOMPARE(marginChangeSpy.count(), 0);
}

void tst_QQuickSafeArea::bindingLoop()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    m_engine->setOutputWarningsToStandardError(false);

    QList<QQmlError> warnings;
    QObject::connect(m_engine.get(), &QQmlEngine::warnings, [&](auto w) { warnings = w; });
    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(window));
    QVERIFY(windowSafeArea);
    windowSafeArea->setAdditionalMargins(QMarginsF(50, 0, 0, 0));
    QCOMPARE(warnings.size(), 1);
    QVERIFY(warnings.at(0).description().endsWith("Safe area binding loop detected"));
}

void tst_QQuickSafeArea::bindingLoopApplicationWindow()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    m_engine->setOutputWarningsToStandardError(false);

    QList<QQmlError> warnings;
    QObject::connect(m_engine.get(), &QQmlEngine::warnings, [&](auto w) { warnings = w; });
    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(qmlAttachedPropertiesObject<QQuickSafeArea>(window));
    QVERIFY(windowSafeArea);

    // Control in footer should stabilize and not cause warning
    windowSafeArea->setAdditionalMargins(QMarginsF(50, 0, 50, 0));
    QSignalSpy widthChangedSpy(window, SIGNAL(itemWidthChanged()));
    window->resize(window->width() - 10, window->height());
    QTRY_COMPARE(widthChangedSpy.count(), 1);
    QCOMPARE(warnings.size(), 0);
}

void tst_QQuickSafeArea::safeAreaReuse()
{
    auto *window = qobject_cast<QQuickWindow*>(m_engine->rootObjects().first());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *windowSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(window, false));
    QVERIFY(windowSafeArea);

    auto *windowContentItemSafeArea = qobject_cast<QQuickSafeArea*>(
        qmlAttachedPropertiesObject<QQuickSafeArea>(window->contentItem(), false));
    QVERIFY(windowContentItemSafeArea);

    QCOMPARE(windowSafeArea, windowContentItemSafeArea);
}

QTEST_MAIN(tst_QQuickSafeArea)

#include "tst_qquicksafearea.moc"
