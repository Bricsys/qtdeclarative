// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtTest/qtest.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>

using namespace QQuickVisualTestUtils;

class tst_QQuickContextMenu : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickContextMenu();

private slots:
    void initTestCase() override;

    void customContextMenu_data();
    void customContextMenu();
    void sharedContextMenu();
    void eventOrder();
    void notAttachedToItem();
    void nullMenu();
    void createOnRequested_data();
    void createOnRequested();
};

tst_QQuickContextMenu::tst_QQuickContextMenu()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickContextMenu::initTestCase()
{
    QQmlDataTest::initTestCase();

    // Can't test native menus with QTest.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows);
}

void tst_QQuickContextMenu::customContextMenu_data()
{
    QTest::addColumn<QString>("qmlFileName");

    QTest::addRow("Rectangle") << "customContextMenuOnRectangle.qml";
    QTest::addRow("Label") << "customContextMenuOnLabel.qml";
    QTest::addRow("Control") << "customContextMenuOnControl.qml";
    QTest::addRow("NestedRectangle") << "customContextMenuOnNestedRectangle.qml";
    QTest::addRow("Pane") << "customContextMenuOnPane.qml";
}

void tst_QQuickContextMenu::customContextMenu()
{
    QFETCH(QString, qmlFileName);

    QQuickApplicationHelper helper(this, qmlFileName);
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomatoItem = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);

    const bool contextMenuTriggeredOnRelease = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::ContextMenuOnMouseRelease).toBool();

    const QPoint &tomatoCenter = mapCenterToWindow(tomatoItem);
    QQuickMenu *menu = window->findChild<QQuickMenu *>();
    QVERIFY(menu);
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    QTRY_COMPARE(menu->isOpened(), !contextMenuTriggeredOnRelease);

    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);

#ifdef Q_OS_WIN
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Menu fails to open on Windows (QTBUG-132436)");
#endif
    QTRY_COMPARE(menu->isOpened(), true);

    // Popups are positioned relative to their parent, and it should be opened at the center:
    // width (100) / 2 = 50
#ifdef Q_OS_ANDROID
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QEXPECT_FAIL("", "This test fails on Android 14 in CI, but passes locally with 15", Abort);
#endif
    QCOMPARE(menu->position(), QPoint(50, 50));
}

void tst_QQuickContextMenu::sharedContextMenu()
{
    QQuickApplicationHelper helper(this, "sharedContextMenuOnRectangle.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *tomato = window->findChild<QQuickItem *>("tomato");
    QVERIFY(tomato);

    auto *reallyRipeTomato = window->findChild<QQuickItem *>("really ripe tomato");
    QVERIFY(reallyRipeTomato);

    // Check that parentItem allows users to distinguish which item triggered a menu.
    const QPoint &tomatoCenter = mapCenterToWindow(tomato);
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    // There should only be one menu.
    auto menus = window->findChildren<QQuickMenu *>();
    QCOMPARE(menus.count(), 1);
    QPointer<QQuickMenu> menu = menus.first();
#ifdef Q_OS_WIN
    if (qgetenv("QTEST_ENVIRONMENT").split(' ').contains("ci"))
        QSKIP("Menu fails to open on Windows (QTBUG-132436)");
#endif
    QTRY_VERIFY(menu->isOpened());
    QCOMPARE(menu->parentItem(), tomato);
    QCOMPARE(menu->itemAt(0)->property("text").toString(), "Eat tomato");

    menu->close();
    QTRY_VERIFY(!menu->isVisible());

    const QPoint &reallyRipeTomatoCenter = mapCenterToWindow(reallyRipeTomato);
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, reallyRipeTomatoCenter);
    QVERIFY(menu);
    menus = window->findChildren<QQuickMenu *>();
    QCOMPARE(menus.count(), 1);
    QCOMPARE(menus.last(), menu);
    QTRY_VERIFY(menu->isOpened());
    QCOMPARE(menu->parentItem(), reallyRipeTomato);
    QCOMPARE(menu->itemAt(0)->property("text").toString(), "Eat really ripe tomato");
}

// We should only send the context menu event if another item higher in the stacking order
// didn't accept the mouse event.
void tst_QQuickContextMenu::eventOrder()
{
    QQuickApplicationHelper helper(this, "deliverToHandlersBeforeContextMenu.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    QVERIFY(window->property("handlerGotEvent").toBool());
    // There shouldn't be a menu since the attached type's context event handler
    // never got the event and hence the menu was never created.
    auto *menu = window->findChild<QQuickMenu *>();
    QEXPECT_FAIL("", "TODO: we need to fix deferred execution so that the menu "
        "is created lazily before this will pass", Continue);
    QVERIFY(!menu);
}

void tst_QQuickContextMenu::notAttachedToItem()
{
    // Should warn but shouldn't crash.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*ContextMenu must be attached to an Item"));
    QQuickApplicationHelper helper(this, "notAttachedToItem.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
}

void tst_QQuickContextMenu::nullMenu()
{
    QQuickApplicationHelper helper(this, "nullMenu.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    // Shouldn't crash.
    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mouseClick(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    auto *menu = window->findChild<QQuickMenu *>();
    QVERIFY(!menu);
}

void tst_QQuickContextMenu::createOnRequested_data()
{
    QTest::addColumn<bool>("programmaticShow");

    QTest::addRow("auto") << false;
    QTest::addRow("manual") << true;
}

void tst_QQuickContextMenu::createOnRequested()
{
    QFETCH(bool, programmaticShow);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("customContextMenuOnRequested.qml")));
    auto *tomatoItem = window.findChild<QQuickItem *>("tomato");
    QVERIFY(tomatoItem);
    const QPoint &tomatoCenter = mapCenterToWindow(tomatoItem);
    window.rootObject()->setProperty("showItToo", programmaticShow);

    // On press or release (depending on QPlatformTheme::ContextMenuOnMouseRelease),
    // ContextMenu.onRequested(pos) should create a standalone custom context menu.
    // If programmaticShow, it will call popup() too; if not, QQuickContextMenu
    // will show it.  Either way, it should still be open after the release.
    QTest::mouseClick(&window, Qt::RightButton, Qt::NoModifier, tomatoCenter);
    QQuickMenu *menu = window.findChild<QQuickMenu *>();
    QVERIFY(menu);
    QVERIFY(menu->isOpened());
    QCOMPARE(window.rootObject()->property("pressPos").toPoint(), tomatoCenter);
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickContextMenu)

#include "tst_qquickcontextmenu.moc"
