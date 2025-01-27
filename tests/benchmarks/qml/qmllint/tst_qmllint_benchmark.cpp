// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QtCore/qobject.h>
#include <QtCore/QLibraryInfo>
#include <QtQmlCompiler/private/qqmljslinter_p.h>

using namespace Qt::StringLiterals;

class tst_qmllint_benchmark : public QObject
{
    Q_OBJECT

    enum PluginSelection { NoPlugins, AllPlugins, OnlyQdsLintPlugin };
    void runOnFile(const QString &fileName, PluginSelection allowedPlugins);

private slots:

    void noPlugins_data();
    void noPlugins();

    void allPlugins_data();
    void allPlugins();

    void onlyQdsLintPlugin_data();
    void onlyQdsLintPlugin();

private:
    const QString m_baseDir = QLatin1String(SRCDIR) + QLatin1String("/data/");
    static constexpr std::array m_files = {
        "propertyStressTestInts.ui.qml"_L1,
        "propertyStressTestItems.ui.qml"_L1,
        "longQmlFile.ui.qml"_L1,
        "deeplyNested.ui.qml"_L1,
    };
};

void tst_qmllint_benchmark::runOnFile(const QString &fileName, PluginSelection allowedPlugins)
{
    QStringList imports = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    QQmlJSLinter linter(imports);

    QList<QQmlJS::LoggerCategory> categories = QQmlJSLogger::defaultCategories();
    switch (allowedPlugins) {
    case NoPlugins:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins())
            plugin.setEnabled(false);
        break;
    case AllPlugins:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins()) {
            for (const QQmlJS::LoggerCategory &category : plugin.categories()) {
                categories.append(category);
            }
        }
        break;
    case OnlyQdsLintPlugin:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins()) {
            if (plugin.name() != "QtDesignStudio") {
                plugin.setEnabled(false);
                continue;
            }
            for (const QQmlJS::LoggerCategory &category : plugin.categories())
                categories.append(category);
        }
        break;
    }

    const QString content = [&fileName, this]() {
        QFile file(m_baseDir + fileName);
        [&file]() { QVERIFY(file.open(QFile::ReadOnly | QFile::Text)); }();
        return file.readAll();
    }();

    QBENCHMARK {
        linter.lintFile(fileName, &content, true, nullptr, imports, {}, {}, categories);
    }
}

void tst_qmllint_benchmark::allPlugins_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::allPlugins()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, AllPlugins);
}

void tst_qmllint_benchmark::noPlugins_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::noPlugins()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, NoPlugins);
}

void tst_qmllint_benchmark::onlyQdsLintPlugin_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::onlyQdsLintPlugin()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, OnlyQdsLintPlugin);
}

QTEST_MAIN(tst_qmllint_benchmark)
#include "tst_qmllint_benchmark.moc"
