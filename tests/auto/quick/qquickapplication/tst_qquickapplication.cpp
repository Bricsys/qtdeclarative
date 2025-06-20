// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>
#include <QtGui/qinputmethod.h>
#include <QtGui/qstylehints.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformintegration.h>
#include <private/qguiapplication_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QFont>

using namespace Qt::StringLiterals;

class tst_qquickapplication : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickapplication();

private slots:
    void active();
    void state();
    void layoutDirection();
    void font();
    void inputMethod();
    void styleHints();
    void cleanup();
    void displayName();
    void platformName();

private:
    QQmlEngine engine;
};

tst_qquickapplication::tst_qquickapplication()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickapplication::cleanup()
{
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ApplicationState)) {
        QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
        QCoreApplication::processEvents();
    }
}

void tst_qquickapplication::active()
{
    for (const QString &app : { u"Qt.application"_s, u"Application"_s }) {
        QQmlComponent component(&engine);
        component.setData(u"import QtQuick 2.0; "
                          "Item { "
                          "    property bool active: %1.active; "
                          "    property bool active2: false; "
                          "    Connections { "
                          "        target: %1; "
                          "        function onActiveChanged(active) { active2 = %1.active; }"
                          "    } "
                          "}"_s.arg(app)
                                  .toUtf8(),
                          QUrl::fromLocalFile(""));
        QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
        QVERIFY(item);
        QQuickWindow window;
        item->setParentItem(window.contentItem());

        // If the platform plugin has the ApplicationState capability, app activation originate from
        // it as a result of a system event. We therefore have to simulate these events here.
        if (QGuiApplicationPrivate::platformIntegration()->hasCapability(
                    QPlatformIntegration::ApplicationState)) {

            // Flush pending events, in case the platform have already queued real application state
            // events
            QWindowSystemInterface::flushWindowSystemEvents();

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
            QWindowSystemInterface::flushWindowSystemEvents();
            QVERIFY(item->property("active").toBool());
            QVERIFY(item->property("active2").toBool());

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
            QWindowSystemInterface::flushWindowSystemEvents();
            QVERIFY(!item->property("active").toBool());
            QVERIFY(!item->property("active2").toBool());
        } else {
            // Otherwise, app activation is triggered by window activation.
            window.show();
            if (QGuiApplication::platformName().toLower() != "xcb")
                window.requestActivate();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
            QVERIFY(item->property("active").toBool());
            QVERIFY(item->property("active2").toBool());

            // not active again
            QWindowSystemInterface::handleFocusWindowChanged(nullptr);
            QTRY_COMPARE_NE(QGuiApplication::focusWindow(), &window);
            QVERIFY(!item->property("active").toBool());
            QVERIFY(!item->property("active2").toBool());
        }
    }
}

void tst_qquickapplication::state()
{
    for (const QString &app : { u"Qt.application"_s, u"Application"_s }) {
        QQmlComponent component(&engine);
        component.setData(u"import QtQuick 2.0; "
                          "Item { "
                          "    property int state: %1.state; "
                          "    property int state2: Qt.ApplicationInactive; "
                          "    Connections { "
                          "        target: %1; "
                          "        function onStateChanged(state) { state2 = %1.state; }"
                          "    } "
                          "    Component.onCompleted: state2 = %1.state; "
                          "}"_s.arg(app)
                                  .toUtf8(),
                          QUrl::fromLocalFile(""));
        QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
        QVERIFY(item);
        QQuickWindow window;
        item->setParentItem(window.contentItem());

        // If the platform plugin has the ApplicationState capability, state changes originate from
        // it as a result of a system event. We therefore have to simulate these events here.
        if (QGuiApplicationPrivate::platformIntegration()->hasCapability(
                    QPlatformIntegration::ApplicationState)) {

            // Flush pending events, in case the platform have already queued real application state
            // events
            QWindowSystemInterface::flushWindowSystemEvents();

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationActive);
            QWindowSystemInterface::flushWindowSystemEvents();
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()), Qt::ApplicationActive);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()), Qt::ApplicationActive);

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationInactive);
            QWindowSystemInterface::flushWindowSystemEvents();
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()),
                     Qt::ApplicationInactive);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()),
                     Qt::ApplicationInactive);

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationSuspended);
            QWindowSystemInterface::flushWindowSystemEvents();
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()),
                     Qt::ApplicationSuspended);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()),
                     Qt::ApplicationSuspended);

            QWindowSystemInterface::handleApplicationStateChanged(Qt::ApplicationHidden);
            QWindowSystemInterface::flushWindowSystemEvents();
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()), Qt::ApplicationHidden);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()), Qt::ApplicationHidden);

        } else {
            // Otherwise, the application can only be in two states, Active and Inactive. These are
            // triggered by window activation.
            window.show();
            if (QGuiApplication::platformName().toLower() != QLatin1String("xcb"))
                window.requestActivate();
            QVERIFY(QTest::qWaitForWindowActive(&window));
            QTRY_COMPARE(QGuiApplication::focusWindow(), &window);
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()), Qt::ApplicationActive);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()), Qt::ApplicationActive);

            // not active again
            QWindowSystemInterface::handleFocusWindowChanged(nullptr);
            QTRY_COMPARE_NE(QGuiApplication::focusWindow(), &window);
            QCOMPARE(Qt::ApplicationState(item->property("state").toInt()),
                     Qt::ApplicationInactive);
            QCOMPARE(Qt::ApplicationState(item->property("state2").toInt()),
                     Qt::ApplicationInactive);
        }
    }
}

void tst_qquickapplication::layoutDirection()
{
    for (const QString &app : { u"Qt.application"_s, u"Application"_s }) {
        QQmlComponent component(&engine);
        component.setData(
                u"import QtQuick 2.0; Item { property bool layoutDirection: %1.layoutDirection }"_s
                        .arg(app)
                        .toUtf8(),
                QUrl::fromLocalFile(""));
        QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
        QVERIFY(item);
        QQuickView view;
        item->setParentItem(view.rootObject());

        // not mirrored
        QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::LeftToRight);

        // mirrored
        QGuiApplication::setLayoutDirection(Qt::RightToLeft);
        QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::RightToLeft);

        // not mirrored again
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);
        QCOMPARE(Qt::LayoutDirection(item->property("layoutDirection").toInt()), Qt::LeftToRight);
    }
}

void tst_qquickapplication::font()
{
    for (const QString &app : { u"Qt.application"_s, u"Application"_s }) {
        QQmlComponent component(&engine);
        component.setData(
                u"import QtQuick 2.0; Item { property font defaultFont: %1.font }"_s.arg(app)
                        .toUtf8(),
                QUrl::fromLocalFile(""));
        QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
        QVERIFY(item);
        QQuickView view;
        item->setParentItem(view.rootObject());

        QVariant defaultFontProperty = item->property("defaultFont");
        QVERIFY(defaultFontProperty.isValid());
        QCOMPARE(defaultFontProperty.typeId(), QMetaType::QFont);
        QCOMPARE(defaultFontProperty.value<QFont>(), qApp->font());
    }
}

void tst_qquickapplication::inputMethod()
{
    // technically not in QQuickApplication, but testing anyway here
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property variant inputMethod: Qt.inputMethod }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // check that the inputMethod property maches with application's input method
    QCOMPARE(qvariant_cast<QObject*>(item->property("inputMethod")), qApp->inputMethod());
}

void tst_qquickapplication::styleHints()
{
    // technically not in QQuickApplication, but testing anyway here
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { property variant styleHints: Application.styleHints }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    QCOMPARE(qvariant_cast<QObject*>(item->property("styleHints")), qApp->styleHints());
}

void tst_qquickapplication::displayName()
{
    QString name[3] = { QStringLiteral("APP NAME 0"),
                        QStringLiteral("APP NAME 1"),
                        QStringLiteral("APP NAME 2")
                      };

    QQmlComponent component(&engine, testFileUrl("tst_displayname.qml"));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    QCoreApplication::setApplicationName(name[0]);
    QCOMPARE(qvariant_cast<QString>(item->property("displayName")), name[0]);

    QGuiApplication::setApplicationName(name[1]);
    QCOMPARE(qvariant_cast<QString>(item->property("displayName")), name[1]);

    QMetaObject::invokeMethod(item, "updateDisplayName", Q_ARG(QVariant, QVariant(name[2])));
    QCOMPARE(QGuiApplication::applicationDisplayName(), name[2]);
}

void tst_qquickapplication::platformName()
{
    // Set up QML component
    QQmlComponent component(&engine, testFileUrl("tst_platformname.qml"));
    QQuickItem *item = qobject_cast<QQuickItem *>(component.create());
    QVERIFY(item);
    QQuickView view;
    item->setParentItem(view.rootObject());

    // Get native platform name
    QString guiApplicationPlatformName = QGuiApplication::platformName();
    QVERIFY(!guiApplicationPlatformName.isEmpty());

    // Get platform name from QML component and verify it's same
    QString qmlPlatformName = qvariant_cast<QString>(item->property("platformName"));
    QCOMPARE(qmlPlatformName, guiApplicationPlatformName);
}

QTEST_MAIN(tst_qquickapplication)

#include "tst_qquickapplication.moc"
