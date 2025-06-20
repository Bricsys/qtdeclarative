// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickrectangleextruder : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickrectangleextruder() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
};

void tst_qquickrectangleextruder::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickrectangleextruder::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        QVERIFY(d->x >= 0.f);
        QVERIFY(d->x <= 100.f);
        QVERIFY(d->y >= 0.f);
        QVERIFY(d->y <= 100.f);
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }

    QCOMPARE(system->groupData[1]->size(), 500);
    for (QQuickParticleData *d : std::as_const(system->groupData[1]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        if (!myFuzzyCompare(d->x, 0.f) && !myFuzzyCompare(d->x, 100.f)){
            QVERIFY(d->x >= 0.f);
            QVERIFY(d->x <= 100.f);
            QVERIFY(myFuzzyCompare(d->y, 0.f) || myFuzzyCompare(d->y, 100.f));
        } else {
            QVERIFY(d->y >= 0.f);
            QVERIFY(d->y <= 100.f);
        }
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

QTEST_MAIN(tst_qquickrectangleextruder);

#include "tst_qquickrectangleextruder.moc"
