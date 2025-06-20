// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QFont>
#include <QFontDatabase>
#include <QString>
#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include <QtQuick/private/qquickfontmetrics_p.h>

#include <QFontMetricsF>

class tst_QuickFontMetrics : public QObject
{
    Q_OBJECT

public:
    tst_QuickFontMetrics();

private Q_SLOTS:
    void properties();
    void functions_data();
    void functions();
};

tst_QuickFontMetrics::tst_QuickFontMetrics()
{
}

void tst_QuickFontMetrics::properties()
{
    const QStringList families = QFontDatabase::families().mid(0, 10);
    QQuickFontMetrics metrics;

    for (const QString &family : families) {
        QFont font(family);
        QFontMetricsF expected(font);

        QSignalSpy spy(&metrics, SIGNAL(fontChanged(QFont)));
        metrics.setFont(font);
        QCOMPARE(spy.size(), 1);

        QCOMPARE(metrics.ascent(), expected.ascent());
        QCOMPARE(metrics.descent(), expected.descent());
        QCOMPARE(metrics.height(), expected.height());
        QCOMPARE(metrics.leading(), expected.leading());
        QCOMPARE(metrics.lineSpacing(), expected.lineSpacing());
        QCOMPARE(metrics.minimumLeftBearing(), expected.minLeftBearing());
        QCOMPARE(metrics.minimumRightBearing(), expected.minRightBearing());
        QCOMPARE(metrics.maximumCharacterWidth(), expected.maxWidth());
        QCOMPARE(metrics.xHeight(), expected.xHeight());
        QCOMPARE(metrics.averageCharacterWidth(), expected.averageCharWidth());
        QCOMPARE(metrics.underlinePosition(), expected.underlinePos());
        QCOMPARE(metrics.overlinePosition(), expected.overlinePos());
        QCOMPARE(metrics.strikeOutPosition(), expected.strikeOutPos());
        QCOMPARE(metrics.lineWidth(), expected.lineWidth());
        QCOMPARE(metrics.capitalHeight(), expected.capHeight());
    }
}

Q_DECLARE_METATYPE(Qt::TextElideMode)

void tst_QuickFontMetrics::functions_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<Qt::TextElideMode>("mode");
    QTest::addColumn<qreal>("width");
    QTest::addColumn<int>("flags");

    QStringList strings;
    strings << QString()
            << QString::fromLatin1("")
            << QString::fromLatin1("0")
            << QString::fromLatin1("@@@@@@@")
            << QString::fromLatin1("Hello");

    QVector<Qt::TextElideMode> elideModes;
    elideModes << Qt::ElideLeft << Qt::ElideMiddle << Qt::ElideRight << Qt::ElideNone;

    for (int stringIndex = 0; stringIndex < strings.size(); ++stringIndex) {
        const QString string = strings.at(stringIndex);

        for (int elideModeIndex = 0; elideModeIndex < elideModes.size(); ++elideModeIndex) {
            Qt::TextElideMode elideMode = static_cast<Qt::TextElideMode>(elideModes.at(elideModeIndex));

            for (qreal width = 0; width < 100; width += 20) {
                const QString tag = QString::fromLatin1("string=%1, mode=%2, width=%3").arg(string).arg(elideMode).arg(width);
                QTest::newRow(qPrintable(tag)) << QString() << elideMode << width << 0;
            }
        }
    }
}

void tst_QuickFontMetrics::functions()
{
    QFETCH(QString, text);
    QFETCH(Qt::TextElideMode, mode);
    QFETCH(qreal, width);
    QFETCH(int, flags);

    QQuickFontMetrics metrics;
    QFontMetricsF expected = QFontMetricsF(QFont());

    QCOMPARE(metrics.elidedText(text, mode, width, flags), expected.elidedText(text, mode, width, flags));
    QCOMPARE(metrics.advanceWidth(text), expected.horizontalAdvance(text));
    QCOMPARE(metrics.boundingRect(text), expected.boundingRect(text));
    QCOMPARE(metrics.tightBoundingRect(text), expected.tightBoundingRect(text));
}

QTEST_MAIN(tst_QuickFontMetrics)

#include "tst_quickfontmetrics.moc"
