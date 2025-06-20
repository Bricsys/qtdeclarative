// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtTest/QTest>
#include "../../../auto/particles/shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>

class tst_affectors : public QObject
{
    Q_OBJECT
public:
    tst_affectors();

private slots:
    void test_basic();
    void test_basic_data();
    void test_filtered();
    void test_filtered_data();
};

tst_affectors::tst_affectors()
{
}

inline QUrl TEST_FILE(const QString &filename)
{
    return QUrl::fromLocalFile(QLatin1String(SRCDIR) + QLatin1String("/data/") + filename);
}

void tst_affectors::test_basic_data()
{
    QTest::addColumn<int> ("dt");
    QTest::newRow("16ms") << 16;
    QTest::newRow("32ms") << 32;
    QTest::newRow("100ms") << 100;
    QTest::newRow("500ms") << 500;
}

void tst_affectors::test_filtered_data()
{
    QTest::addColumn<int> ("dt");
    QTest::newRow("16ms") << 16;
    QTest::newRow("32ms") << 32;
    QTest::newRow("100ms") << 100;
    QTest::newRow("500ms") << 500;
}

void tst_affectors::test_basic()
{
    QFETCH(int, dt);
    QQuickView* view = createView(TEST_FILE("basic.qml"));
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    //Pretend we're running, but we manually advance the simulation
    system->m_running = true;
    system->m_animation = 0;
    system->reset();

    int curTime = 1;
    system->updateCurrentTime(curTime);//Fixed point and get init out of the way - including emission

    QBENCHMARK {
        curTime += dt;
        system->updateCurrentTime(curTime);
    }

    int stillAlive = 0;
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 1000, 10));//Small simulation variance is permissible.
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        if (d->stillAlive(system))
            stillAlive++;
        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    QVERIFY(extremelyFuzzyCompare(stillAlive, 1000, 10));//Small simulation variance is permissible.
    delete view;
}

void tst_affectors::test_filtered()
{
    QFETCH(int, dt);
    QQuickView* view = createView(TEST_FILE("filtered.qml"));
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    //Pretend we're running, but we manually advance the simulation
    system->m_running = true;
    system->m_animation = 0;
    system->reset();

    int curTime = 1;
    system->updateCurrentTime(curTime);//Fixed point and get init out of the way - including emission

    QBENCHMARK {
        curTime += dt;
        system->updateCurrentTime(curTime);
    }

    int stillAlive = 0;
    QVERIFY(extremelyFuzzyCompare(system->groupData[1]->size(), 1000, 10));//Small simulation variance is permissible.
    for (QQuickParticleData *d : std::as_const(system->groupData[1]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        if (d->stillAlive(system))
            stillAlive++;
        QCOMPARE(d->x, 160.f);
        QCOMPARE(d->y, 160.f);
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    QVERIFY(extremelyFuzzyCompare(stillAlive, 1000, 10));//Small simulation variance is permissible.
    delete view;
}

QTEST_MAIN(tst_affectors);

#include "tst_affectors.moc"
