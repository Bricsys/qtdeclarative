// Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QProcess>
#include <QString>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlCompiler/private/qqmljslinter_p.h>
#include <QtQmlCompiler/private/qqmlsa_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtCore/qplugin.h>
#include <QtCore/qcomparehelpers.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qlibraryinfo.h>

Q_IMPORT_PLUGIN(LintPlugin)

using namespace Qt::StringLiterals;

class TestQmllint: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmllint();

    struct Message
    {
        QString text = QString();
        quint32 line = 0, column = 0;
        QtMsgType severity = QtWarningMsg;
    };

    struct Result
    {
        enum Flag {
            ExitsNormally = 0x1,
            NoMessages = 0x2,
            AutoFixable = 0x4,
            UseSettings = 0x8
        };

        Q_DECLARE_FLAGS(Flags, Flag)

        static Result clean() { return Result { {}, {}, {}, { NoMessages, ExitsNormally } }; }
        static Result cleanWithSettings()
        {
            return Result{ {}, {}, {}, { NoMessages, ExitsNormally, UseSettings } };
        }

        QList<Message> expectedMessages = {};
        QList<Message> badMessages = {};
        QList<Message> expectedReplacements = {};

        Flags flags = {};
    };

    struct Environment : public QList<std::pair<QString, QString>>
    {
        using QList<std::pair<QString, QString>>::QList;
    };

private Q_SLOTS:
    void initTestCase() override;

    void testUnqualified();
    void testUnqualified_data();

    void cleanQmlCode_data();
    void cleanQmlCode();

    void dirtyQmlCode_data();
    void dirtyQmlCode();

    void dirtyQmlSnippet_data();
    void dirtyQmlSnippet();

    void cleanQmlSnippet_data();
    void cleanQmlSnippet();

    void dirtyJsSnippet_data();
    void dirtyJsSnippet();

    void cleanJsSnippet_data();
    void cleanJsSnippet();

    void compilerWarnings_data();
    void compilerWarnings();

    void testUnknownCausesFail();

    void directoryPassedAsQmlTypesFile();
    void oldQmltypes();

    void qmltypes_data();
    void qmltypes();

    void autoqmltypes();
    void resources();

    void multiDirectory();

    void requiredProperty();

    void settingsFile();

    void additionalImplicitImport();

    void qrcUrlImport();

    void incorrectImportFromHost_data();
    void incorrectImportFromHost();

    void attachedPropertyReuse();

    void missingBuiltinsNoCrash();
    void absolutePath();

    void importMultipartUri();

    void lintModule_data();
    void lintModule();

    void testLineEndings();
    void valueTypesFromString();

    void ignoreSettingsNotCommandLineOptions();
    void backslashedQmldirPath();

    void environment_data();
    void environment();

    void maxWarnings();

#if QT_CONFIG(library)
    void hasTestPlugin();
    void testPlugin_data();
    void testPlugin();
    void testPluginHelpCommandLine();
    void testPluginCommandLine();
    void quickPlugin();
    void hasQdsPlugin();
    void qdsPlugin_data();
    void qdsPlugin();
#endif

#if QT_CONFIG(process)
    void importRelScript();
#endif

    void replayImportWarnings();
    void errorCategory();

private:
    enum DefaultImportOption { NoDefaultImports, UseDefaultImports };
    enum ContainOption { StringNotContained, StringContained };
    enum ReplacementOption {
        NoReplacementSearch,
        DoReplacementSearch,
    };

    enum LintType { LintFile, LintModule };

    static QStringList warningsShouldFailArgs() {
        static QStringList args {"-W", "0"};
        return args;
    }

    QString runQmllint(const QString &fileToLint, std::function<void(QProcess &)> handleResult,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true,
                       const Environment &env = {});
    QString runQmllint(const QString &fileToLint, bool shouldSucceed,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true,
                       const Environment &env = {});

    enum CallQmllintCheck {
        ShouldFail = 0,
        ShouldSucceed = 1,
        HasAutoFix = 2,
        HasAutoFixAndSucceeds = HasAutoFix | ShouldSucceed,
    };
    Q_DECLARE_FLAGS(CallQmllintChecks, CallQmllintCheck);

    static CallQmllintChecks fromResultFlags(Result::Flags flags)
    {
        CallQmllintChecks result;
        result.setFlag(ShouldSucceed, flags.testFlags(Result::ExitsNormally));
        result.setFlag(HasAutoFix, flags.testFlags(Result::AutoFixable));
        return result;
    }

    struct CallQmllintOptions
    {
        QStringList importPaths = {};
        QStringList qmldirFiles = {};
        QStringList resources = {};
        DefaultImportOption defaultImports = UseDefaultImports;
        QList<QQmlJS::LoggerCategory> *categories = nullptr;
        LintType type = LintFile;
        bool readSettings = false;
        QStringList enableCategories = {};
    };

    QJsonArray callQmllintImpl(const QString &fileToLint, const QString &fileCpntent,
                               const CallQmllintOptions &options,
                               CallQmllintChecks check = CallQmllintCheck::ShouldSucceed);
    QJsonArray callQmllint(const QString &fileToLint, const CallQmllintOptions &options,
                           CallQmllintChecks check = CallQmllintCheck::ShouldSucceed);
    QJsonArray callQmllintOnSnippet(const QString &snippet, const CallQmllintOptions &options,
                                    CallQmllintChecks checks);

    void testFixes(bool shouldSucceed, QStringList importPaths, QStringList qmldirFiles,
                   QStringList resources, DefaultImportOption defaultImports,
                   QList<QQmlJS::LoggerCategory> *categories, bool autoFixable, bool readSettings,
                   const QString &fixedPath);

    void searchWarnings(const QJsonArray &warnings, const QString &string,
                        QtMsgType type = QtWarningMsg, quint32 line = 0, quint32 column = 0,
                        ContainOption shouldContain = StringContained,
                        ReplacementOption searchReplacements = NoReplacementSearch);

    template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler,
             typename ReplacementFailureHandler>
    void checkResult(const QJsonArray &warnings, const Result &result,
                     ExpectedMessageFailureHandler onExpectedMessageFailures,
                     BadMessageFailureHandler onBadMessageFailures,
                     ReplacementFailureHandler onReplacementFailures);

    void checkResult(const QJsonArray &warnings, const Result &result)
    {
        checkResult(
                warnings, result, [] {}, [] {}, [] {});
    }

    void runTest(const QString &testFile, const Result &result, QStringList importDirs = {},
                 QStringList qmltypesFiles = {}, QStringList resources = {},
                 DefaultImportOption defaultImports = UseDefaultImports,
                 QList<QQmlJS::LoggerCategory> *categories = nullptr);

    QString m_qmllintPath;

    QStringList m_defaultImportPaths;
    QQmlJSLinter m_linter;
    QList<QQmlJS::LoggerCategory> m_categories = QQmlJSLogger::defaultCategories();
};

Q_DECLARE_METATYPE(TestQmllint::Result)

TestQmllint::TestQmllint()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_defaultImportPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath), dataDirectory() }),
      m_linter(m_defaultImportPaths)
{
    for (const QQmlJSLinter::Plugin &plugin : m_linter.plugins())
        m_categories.append(plugin.categories());
}

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");

#ifdef Q_OS_WIN
    m_qmllintPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmllintPath).exists()) {
        QString message = QStringLiteral("qmllint executable not found (looked for %0)").arg(m_qmllintPath);
        QFAIL(qPrintable(message));
    }
}

void TestQmllint::testUnqualified()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    runTest(filename, result);
}

void TestQmllint::testUnqualified_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    // id from nowhere (as with setContextProperty)
    QTest::newRow("IdFromOuterSpace")
            << QStringLiteral("IdFromOuterSpace.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 4, 8 },
                          Message { QStringLiteral("Unqualified access"), 7, 21 } } };
    // access property of root object
    QTest::newRow("FromRootDirect")
            << QStringLiteral("FromRoot.qml")
            << Result {
                   {
                           Message { QStringLiteral("Unqualified access"), 9, 16 }, // new property
                           Message { QStringLiteral("Unqualified access"), 13,
                                     33 } // builtin property
                   },
                   {},
                   { { Message { u"root."_s, 9, 16 } }, { Message { u"root."_s, 13, 33 } } }
               };
    // access injected name from signal
    QTest::newRow("SignalHandler")
            << QStringLiteral("SignalHandler.qml")
            << Result { {
                                Message { QStringLiteral("Unqualified access"), 5, 21 },
                                Message { QStringLiteral("Unqualified access"), 10, 21 },
                                Message { QStringLiteral("Unqualified access"), 8, 29 },
                                Message { QStringLiteral("Unqualified access"), 12, 34 },
                        },
                        {},
                        {
                                Message { QStringLiteral("function(mouse)"), 4, 22 },
                                Message { QStringLiteral("function(mouse)"), 9, 24 },
                                Message { QStringLiteral("(mouse) => "), 8, 16 },
                                Message { QStringLiteral("(mouse) => "), 12, 21 },
                        } };
    // access catch identifier outside catch block
    QTest::newRow("CatchStatement")
            << QStringLiteral("CatchStatement.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 6, 21 } } };
    QTest::newRow("NonSpuriousParent")
            << QStringLiteral("nonSpuriousParentWarning.qml")
            << Result { {
                                Message { QStringLiteral("Unqualified access"), 6, 25 },
                        },
                        {},
                        { { Message { u"<id>."_s, 6, 25 } } } };

    QTest::newRow("crashConnections")
            << QStringLiteral("crashConnections.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 4, 13 } } };

    QTest::newRow("delegateContextProperties")
            << QStringLiteral("delegateContextProperties.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 6, 14 },
                          Message { QStringLiteral("Unqualified access"), 7, 15 },
                          Message { QStringLiteral("model is implicitly injected into this "
                                                   "delegate. Add a required property instead.") },
                          Message {
                                  QStringLiteral("index is implicitly injected into this delegate. "
                                                 "Add a required property instead.") } } };
    QTest::newRow("storeSloppy")
            << QStringLiteral("UnqualifiedInStoreSloppy.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 9, 26} } };
    QTest::newRow("storeStrict")
            << QStringLiteral("UnqualifiedInStoreStrict.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 9, 52} } };
}

void TestQmllint::testUnknownCausesFail()
{
    runTest("unknownElement.qml",
            Result { { Message {
                    QStringLiteral(
                            "Unknown was not found. "
                            "Did you add all imports and dependencies?"),
                    4, 5,
                    QtWarningMsg
            } } });
    runTest("TypeWithUnknownPropertyType.qml",
            Result { { Message {
                    QStringLiteral(
                            "Something was not found. "
                            "Did you add all imports and dependencies?"),
                    4, 5,
                    QtWarningMsg
            } } });
}

void TestQmllint::directoryPassedAsQmlTypesFile()
{
    runTest("unknownElement.qml",
            Result { { Message { QStringLiteral("QML types file cannot be a directory: ")
                                 + dataDirectory() } } },
            {}, { dataDirectory() });
}

void TestQmllint::oldQmltypes()
{
    runTest("oldQmltypes.qml",
            Result { {
                             Message { QStringLiteral("typeinfo not declared in qmldir file") },
                             Message {
                                     QStringLiteral("Found deprecated dependency specifications") },
                             Message { QStringLiteral(
                                     "Meta object revision and export version differ.") },
                             Message { QStringLiteral(
                                     "Revision 0 corresponds to version 0.0; it should be 1.0.") },
                     },
                     {
                             Message { QStringLiteral("QQuickItem was not found. "
                                                      "Did you add all imports and dependencies?")
                     }
            } });

    runTest("oldUnusedQmlTypes.qml",
            Result { {
                             Message { QStringLiteral("typeinfo not declared in qmldir file") },
                             Message {
                                      QStringLiteral("Found deprecated dependency specifications") },
                             Message { QStringLiteral(
                                     "Meta object revision and export version differ.") },
                             Message { QStringLiteral(
                                     "Revision 0 corresponds to version 0.0; it should be 1.0.") },
                      },
                      {
                             Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg
                      }
            } });
}

void TestQmllint::qmltypes_data()
{
    QTest::addColumn<QString>("file");

    const QString importsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QDirIterator it(importsPath, { "*.qmltypes" },
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        QTest::addRow("%s", qPrintable(it.next().mid(importsPath.size()))) << it.filePath();
}

void TestQmllint::qmltypes()
{
    QFETCH(QString, file);
    callQmllint(file, {});
}

void TestQmllint::autoqmltypes()
{
    QProcess process;
    process.setWorkingDirectory(testFile("autoqmltypes"));
    process.start(m_qmllintPath, warningsShouldFailArgs() << QStringLiteral("test.qml") );

    process.waitForFinished();

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QVERIFY(process.exitCode() != 0);

    QVERIFY(process.readAllStandardError()
                .contains("is not a qmldir file. Assuming qmltypes"));
    QVERIFY(process.readAllStandardOutput().isEmpty());

    {
        QProcess bare;
        bare.setWorkingDirectory(testFile("autoqmltypes"));
        bare.start(m_qmllintPath, warningsShouldFailArgs() << QStringLiteral("--bare") << QStringLiteral("test.qml") );
        bare.waitForFinished();

        const QByteArray errors = bare.readAllStandardError();
        QVERIFY(!errors.contains("is not a qmldir file. Assuming qmltypes"));
        QVERIFY(errors.contains("Failed to import TestTest."));
        QVERIFY(bare.readAllStandardOutput().isEmpty());

        QCOMPARE(bare.exitStatus(), QProcess::NormalExit);
        QVERIFY(bare.exitCode() != 0);
    }
}

void TestQmllint::resources()
{
    {
        // We need to clear the import cache before we add a qrc file with different
        // contents for the same paths.
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });

        CallQmllintOptions options;
        options.resources.append(testFile("resource.qrc"));

        callQmllint(testFile("resource.qml"), options);
        callQmllint(testFile("badResource.qml"), options, CallQmllintCheck::ShouldFail);
    }

    callQmllint(testFile("resource.qml"), CallQmllintOptions{}, CallQmllintCheck::ShouldFail);
    callQmllint(testFile("badResource.qml"), CallQmllintOptions{});

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        CallQmllintOptions options;
        options.resources.append(testFile("T/a.qrc"));

        callQmllint(testFile("T/b.qml"), options);
    }

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        CallQmllintOptions options;
        options.resources.append(testFile("relPathQrc/resources.qrc"));

        callQmllint(testFile("relPathQrc/Foo/Thing.qml"), options);
    }
}

void TestQmllint::multiDirectory()
{
    CallQmllintOptions options;
    options.resources.append(testFile("MultiDirectory/multi.qrc"));

    callQmllint(testFile("MultiDirectory/qml/Inner.qml"), options);
    callQmllint(testFile("MultiDirectory/qml/pages/Page.qml"), options);
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << Result { { Message { QStringLiteral("Expected token `:'"), 4, 8, QtCriticalMsg } } };
    QTest::newRow("Invalid_syntax_JS") << QStringLiteral("failure1.js")
                                       << Result { { Message { QStringLiteral("Expected token `;'"),
                                                               4, 12, QtCriticalMsg } } };
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 12, 36 } } };
    QTest::newRow("AutomatchedSignalHandler2")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << Result{ { Message{
                       QStringLiteral(
                               "Implicitly defining \"onClicked\" as signal handler in Connections "
                               "is deprecated. "
                               "Create a function instead: \"function onClicked() { ... }\"")} } };
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << Result { { Message {
                       QStringLiteral("Member \"foo\" not found on type \"QtObject\""), 6,
                       31 } } };
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << Result { { Message {
                       QStringLiteral("Member \"foo2\" not found on type \"Methods\""), 5,
                       25 } } };
    QTest::newRow("badAlias")
            << QStringLiteral("badAlias.qml")
            << Result { { Message { QStringLiteral("Cannot resolve alias \"wrong\""), 4, 5 } } };
    QTest::newRow("badAliasProperty1")
            << QStringLiteral("badAliasProperty.qml")
            << Result { { Message { QStringLiteral("Cannot resolve alias \"wrong\""), 5, 5 } } };
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << Result { { Message {
                       QStringLiteral("Invalid alias expression. Only IDs and field member "
                                      "expressions can be aliased"),
                       5, 26 } } };
    QTest::newRow("badAliasNotAnExpression")
            << QStringLiteral("badAliasNotAnExpression.qml")
            << Result { { Message {
                                  QStringLiteral("Invalid alias expression. Only IDs and field member "
                                                 "expressions can be aliased"),
                                  4, 30 } } };
    QTest::newRow("aliasCycle1") << QStringLiteral("aliasCycle.qml")
                                 << Result { { Message {
                                            QStringLiteral("Alias \"b\" is part of an alias cycle"),
                                            6, 5 } } };
    QTest::newRow("aliasCycle2") << QStringLiteral("aliasCycle.qml")
                                 << Result { { Message {
                                            QStringLiteral("Alias \"a\" is part of an alias cycle"),
                                            5, 5 } } };
    QTest::newRow("invalidAliasTarget1") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression – an initalizer is needed."),
                                            6, 18 } } };
    QTest::newRow("invalidAliasTarget2") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression. Only IDs and field member expressions can be aliased"),
                                            7, 30 } } };
    QTest::newRow("invalidAliasTarget3") << QStringLiteral("invalidAliasTarget.qml")
                                         << Result { { Message {
                                            QStringLiteral("Invalid alias expression. Only IDs and field member expressions can be aliased"),
                                            9, 34 } } };
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << Result { { Message { QStringLiteral("Member \"rrr\" not found on type \"Item\""),
                                    5, 34 } } };
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << Result { { Message {
                       QStringLiteral("Member \"progress\" not found on type \"QQuickItem\""), 7,
                       39 } } };
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << Result { { Message {
                       QStringLiteral("Member \"rrr\" not found on type \"QQuickItem\""), 5,
                       39 } } };
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << Result { { Message {
                       QStringLiteral("Type \"QPalette\" of property \"palette\" not found"), 5,
                       26 } } };
    QTest::newRow("incompleteQmltypes2")
            << QStringLiteral("incompleteQmltypes2.qml")
            << Result { { Message { QStringLiteral("Member \"weDontKnowIt\" "
                                                   "not found on type \"CustomPalette\""),
                                    5, 35 } } };
    QTest::newRow("incompleteQmltypes3")
            << QStringLiteral("incompleteQmltypes3.qml")
            << Result { { Message {
                       QStringLiteral("Type \"QPalette\" of property \"palette\" not found"), 5,
                       21 } } };
    QTest::newRow("inheritanceCycle")
            << QStringLiteral("Cycle1.qml")
            << Result { { Message {
                       QStringLiteral("Cycle1 is part of an inheritance cycle: Cycle2 -> Cycle3 "
                                      "-> Cycle1 -> Cycle2"),
                       2, 1 } } };
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << Result { { Message {
                       QStringLiteral("Item was not found. "
                                      "Did you add all imports and dependencies?"),
                       3, 1 } } };
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << Result { { Message {
                       QStringLiteral("Member \"unknownFunc\" not found on type \"Foo\""), 5,
                       21 } } };
    QTest::newRow("badEnumFromQtQml")
            << QStringLiteral("badEnumFromQtQml.qml")
            << Result { { Message { QStringLiteral("Member \"Linear123\" not "
                                                   "found on type \"QQmlEasingEnums\""),
                                    4, 30 } } };
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign binding of type QQuickItem to QQuickAnchorLine") } } };
    QTest::newRow("nanchors1") << QStringLiteral("nanchors1.qml")
                               << Result{ { Message{ QStringLiteral(
                                                  "unknown grouped property scope nanchors.") } },
                                          {},
                                          {} };
    QTest::newRow("nanchors2") << QStringLiteral("nanchors2.qml")
                               << Result{ { Message{ QStringLiteral(
                                                  "unknown grouped property scope nanchors.") } },
                                          {},
                                          {} };
    QTest::newRow("nanchors3") << QStringLiteral("nanchors3.qml")
                               << Result{ { Message{ QStringLiteral(
                                                  "unknown grouped property scope nanchors.") } },
                                          {},
                                          {} };
    QTest::newRow("badAliasObject")
            << QStringLiteral("badAliasObject.qml")
            << Result { { Message { QStringLiteral("Member \"wrongwrongwrong\" not "
                                                   "found on type \"QtObject\""),
                                    8, 40 } } };
    QTest::newRow("badScript") << QStringLiteral("badScript.qml")
                               << Result { { Message {
                                          QStringLiteral(
                                                  "Member \"stuff\" not found on type \"Empty\""),
                                          5, 21 } } };
    QTest::newRow("badScriptOnAttachedProperty")
            << QStringLiteral("badScript.attached.qml")
            << Result { { Message { QStringLiteral("Unqualified access"), 3, 26 } } };
    QTest::newRow("brokenNamespace")
            << QStringLiteral("brokenNamespace.qml")
            << Result { { Message { QStringLiteral("Type not found in namespace"), 4, 19 } } };
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << Result { { Message { QStringLiteral(
                       "Member \"foobar\" not found on type \"QQuickScreenAttached\"") } } };
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << Result{ {
                       Message{ "Identifier 'argq' is used here before its declaration"_L1, 5, 9 },
                       Message{ "Note: declaration of 'argq' here"_L1, 6, 13 },
               } };
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << Result { { Message { QStringLiteral(
                                "Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                                "The signal has a parameter of the same name in position 2.") } },
                        { Message { QStringLiteral("onSig2") } } };
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << Result { { Message {
                       QStringLiteral("Signal handler for \"onSig\" has more formal parameters "
                                      "than the signal it handles.") } } };
    QTest::newRow("OnAssignment") << QStringLiteral("onAssignment.qml")
                                  << Result { { Message { QStringLiteral(
                                             "Member \"loops\" not found on type \"bool\"") } } };
    QTest::newRow("BadAttached") << QStringLiteral("badAttached.qml")
                                 << Result { { Message { QStringLiteral(
                                            "unknown attached property scope WrongAttached.") } } };
    QTest::newRow("BadBinding") << QStringLiteral("badBinding.qml")
                                << Result{ { Message{ QStringLiteral(
                                           "Could not find property \"doesNotExist\".") } } };
    QTest::newRow("bad template literal (simple)")
            << QStringLiteral("badTemplateStringSimple.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("bad constant number to string")
            << QStringLiteral("numberToStringProperty.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign literal of type double to QString") } } };
    QTest::newRow("bad unary minus to string")
            << QStringLiteral("unaryMinusToStringProperty.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign literal of type double to QString") } } };
    QTest::newRow("bad tranlsation binding (qsTr)") << QStringLiteral("bad_qsTr.qml") << Result {};
    QTest::newRow("bad string binding (QT_TR_NOOP)")
            << QStringLiteral("bad_QT_TR_NOOP.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("BadScriptBindingOnGroup")
            << QStringLiteral("badScriptBinding.group.qml")
            << Result{ { Message{ QStringLiteral("Could not find property \"bogusProperty\"."), 3,
                                  10 } } };
    QTest::newRow("BadScriptBindingOnAttachedType")
            << QStringLiteral("badScriptBinding.attached.qml")
            << Result{ { Message{ QStringLiteral("Could not find property \"bogusProperty\"."), 5,
                                  12 } } };
    QTest::newRow("BadScriptBindingOnAttachedSignalHandler")
            << QStringLiteral("badScriptBinding.attachedSignalHandler.qml")
            << Result { { Message {
                       QStringLiteral("no matching signal found for handler \"onBogusSignal\""), 3,
                       10 } } };
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << Result { { Message { QStringLiteral(
                       "No type found for property \"bad\". This may be due to a missing "
                       "import statement or incomplete qmltypes files.") } } };
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << Result { { Message {
                       QStringLiteral("Property \"deprecated\" is deprecated (Reason: Test)") } } };
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << Result { { Message { QStringLiteral("Property \"deprecated\" is deprecated") } } };
    QTest::newRow("Deprecation (Property binding, with reason)")
            << QStringLiteral("deprecatedPropertyBindingReason.qml")
            << Result { { Message { QStringLiteral(
                       "Binding on deprecated property \"deprecatedReason\" (Reason: Test)") } } };
    QTest::newRow("Deprecation (Property binding, no reason)")
            << QStringLiteral("deprecatedPropertyBinding.qml")
            << Result { { Message {
                       QStringLiteral("Binding on deprecated property \"deprecated\"") } } };
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << Result { { Message { QStringLiteral(
                       "Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)") } } };
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << Result { { Message { QStringLiteral("Type \"TypeDeprecated\" is deprecated") } } };
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to non-existent default property") } } };
    QTest::newRow("MissingDefaultPropertyDefinedInTheSameType")
            << QStringLiteral("defaultPropertyWithinTheSameType.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to non-existent default property") } } };
    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot assign multiple objects to a default non-list property") } } };
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << Result { { Message { QStringLiteral(
                                "Cannot assign to default property of incompatible type") } },
                        { Message { QStringLiteral(
                                "Cannot assign to non-existent default property") } } };
    QTest::newRow("MultiDefaultPropertyWithWrongType")
            << QStringLiteral("multiDefaultPropertyWithWrongType.qml")
            << Result { { Message { QStringLiteral(
                                "Cannot assign to default property of incompatible type") } },
                        { Message { QStringLiteral(
                                "Cannot assign to non-existent default property") } } };
    QTest::newRow("DefaultPropertyLookupInUnknownType")
        << QStringLiteral("unknownParentDefaultPropertyCheck.qml")
        << Result { { Message {  QStringLiteral(
                "Alien was not found. Did you add all imports and dependencies?") } } };
    QTest::newRow("InvalidImport")
            << QStringLiteral("invalidImport.qml")
            << Result { { Message { QStringLiteral(
                       "Failed to import FooBar. Are your import paths set up properly?"), 2, 1 } } };
    QTest::newRow("Unused Import (simple)")
            << QStringLiteral("unused_simple.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        {},
                        {},
                        Result::ExitsNormally };
    QTest::newRow("Unused Import (prefix)")
            << QStringLiteral("unused_prefix.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        {},
                        {},
                        Result::ExitsNormally };
    QTest::newRow("TypePropertAccess") << QStringLiteral("typePropertyAccess.qml") << Result {};
    QTest::newRow("badAttachedProperty")
            << QStringLiteral("badAttachedProperty.qml")
            << Result { { Message {
                       QStringLiteral("Member \"progress\" not found on type \"TestTypeAttached\"")
               } } };
    QTest::newRow("badAttachedPropertyNested")
            << QStringLiteral("badAttachedPropertyNested.qml")
            << Result { { Message { QStringLiteral(
                                            "Member \"progress\" not found on type \"QObject\""),
                                    12, 41 } },
                        { Message { QString("Member \"progress\" not found on type \"QObject\""),
                                    6, 37 } } };
    QTest::newRow("badAttachedPropertyTypeString")
            << QStringLiteral("badAttachedPropertyTypeString.qml")
            << Result{ { Message{ QStringLiteral("Cannot assign literal of type string to int") } },
                       {},
                       {} };
    QTest::newRow("badAttachedPropertyTypeQtObject")
            << QStringLiteral("badAttachedPropertyTypeQtObject.qml")
            << Result{ { Message{
                                  QStringLiteral("Cannot assign object of type QtObject to int") } } };
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyAccess")
            << QStringLiteral("goodAttachedPropertyAccess.qml") << Result::clean();
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyNested")
            << QStringLiteral("goodAttachedPropertyNested.qml") << Result::clean();
    QTest::newRow("deprecatedFunction")
            << QStringLiteral("deprecatedFunction.qml")
            << Result { { Message { QStringLiteral(
                       "Method \"deprecated(foobar)\" is deprecated (Reason: No particular "
                       "reason.)") } } };
    QTest::newRow("deprecatedFunctionInherited")
            << QStringLiteral("deprecatedFunctionInherited.qml")
            << Result { { Message { QStringLiteral(
                       "Method \"deprecatedInherited(c, d)\" is deprecated (Reason: This "
                       "deprecation should be visible!)") } } };

    QTest::newRow("duplicated id")
            << QStringLiteral("duplicateId.qml")
            << Result { { Message {
                       QStringLiteral("Found a duplicated id. id root was first declared "), 0, 0,
                       QtCriticalMsg } } };

    QTest::newRow("string as id") << QStringLiteral("stringAsId.qml")
                                  << Result { { Message { QStringLiteral(
                                             "ids do not need quotation marks") } } };
    QTest::newRow("stringIdUsedInWarning")
            << QStringLiteral("stringIdUsedInWarning.qml")
            << Result { { Message {
                                QStringLiteral("i is a member of a parent element"),
                        } },
                        {},
                        { Message { QStringLiteral("stringy.") } } };
    QTest::newRow("id_in_value_type")
            << QStringLiteral("idInValueType.qml")
            << Result{ { Message{ "id declarations are only allowed in objects" } } };
    QTest::newRow("Invalid_id_expression")
            << QStringLiteral("invalidId1.qml")
            << Result { { Message { QStringLiteral("Failed to parse id") } } };
    QTest::newRow("Invalid_id_blockstatement")
            << QStringLiteral("invalidId2.qml")
            << Result { { Message { QStringLiteral("id must be followed by an identifier") } } };
    QTest::newRow("multilineString")
            << QStringLiteral("multilineString.qml")
            << Result { { Message { QStringLiteral("String contains unescaped line terminator "
                                                   "which is deprecated."),
                                    0, 0, QtInfoMsg } },
                        {},
                        { Message { "`Foo\nmultiline\\`\nstring`", 4, 32 },
                          Message { "`another\\`\npart\nof it`", 6, 11 },
                          Message { R"(`
quote: " \\" \\\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \' \\' \\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)",
                                    10, 28 },
                          Message {
                                  R"(`
quote: " \" \\" \\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \\' \\\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)",
                                  16, 27 } },
                        { Result::ExitsNormally, Result::AutoFixable } };
    QTest::addRow("multifix")
            << QStringLiteral("multifix.qml")
            << Result { {
                    Message { QStringLiteral("Unqualified access"), 7,  19, QtWarningMsg},
                    Message { QStringLiteral("Unqualified access"), 11, 19, QtWarningMsg},
                }, {}, {
                    Message { QStringLiteral("pragma ComponentBehavior: Bound\n"), 1, 1 }
                }, { Result::AutoFixable }};
    QTest::newRow("unresolvedType")
            << QStringLiteral("unresolvedType.qml")
            << Result { { Message { QStringLiteral(
                               "UnresolvedType was not found. "
                               "Did you add all imports and dependencies?")
                       } },
                        { Message { QStringLiteral("incompatible type") } } };
    QTest::newRow("invalidInterceptor")
            << QStringLiteral("invalidInterceptor.qml")
            << Result { { Message { QStringLiteral(
                       "On-binding for property \"angle\" has wrong type \"Item\"") } } };
    QTest::newRow("2Interceptors")
            << QStringLiteral("2interceptors.qml")
            << Result { { Message { QStringLiteral("Duplicate interceptor on property \"x\"") } } };
    QTest::newRow("ValueSource+2Interceptors")
            << QStringLiteral("valueSourceBetween2interceptors.qml")
            << Result { { Message { QStringLiteral("Duplicate interceptor on property \"x\"") } } };
    QTest::newRow("2ValueSources") << QStringLiteral("2valueSources.qml")
                                   << Result { { Message { QStringLiteral(
                                              "Duplicate value source on property \"x\"") } } };
    QTest::newRow("ValueSource+Value")
            << QStringLiteral("valueSource_Value.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot combine value source and binding on property \"obj\"") } } };
    QTest::newRow("ValueSource+ListValue")
            << QStringLiteral("valueSource_listValue.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot combine value source and binding on property \"objs\"") } } };
    QTest::newRow("NonExistentListProperty")
            << QStringLiteral("nonExistentListProperty.qml")
            << Result { { Message { QStringLiteral("Could not find property \"objs\".") } } };
    QTest::newRow("QtQuick.Window 2.0")
            << QStringLiteral("qtquickWindow20.qml")
            << Result { { Message { QStringLiteral(
                       "Member \"window\" not found on type \"QQuickWindow\"") } } };
    QTest::newRow("unresolvedAttachedType")
            << QStringLiteral("unresolvedAttachedType.qml")
            << Result { { Message { QStringLiteral(
                                "unknown attached property scope UnresolvedAttachedType.") } },
                        { Message { QStringLiteral("Could not find property \"property\".") } } };
    QTest::newRow("nestedInlineComponents")
            << QStringLiteral("nestedInlineComponents.qml")
            << Result { { Message {
                       QStringLiteral("Nested inline components are not supported") } } };
    QTest::newRow("inlineComponentNoComponent")
            << QStringLiteral("inlineComponentNoComponent.qml")
            << Result { { Message {
                          QStringLiteral("Inline component declaration must be followed by a typename"),
                         3, 2 } } };
    QTest::newRow("WithStatement") << QStringLiteral("WithStatement.qml")
                                   << Result { { Message { QStringLiteral(
                                              "with statements are strongly discouraged") } } };
    QTest::newRow("BadLiteralBinding")
            << QStringLiteral("badLiteralBinding.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign literal of type string to int") } } };
    QTest::newRow("BadLiteralBindingDate")
            << QStringLiteral("badLiteralBindingDate.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign binding of type QString to QDateTime") } } };
    QTest::newRow("BadModulePrefix")
            << QStringLiteral("badModulePrefix.qml")
            << Result { { Message {
                       QStringLiteral("Cannot access singleton as a property of an object") } } };
    QTest::newRow("BadModulePrefix2")
            << QStringLiteral("badModulePrefix2.qml")
            << Result { { Message { QStringLiteral(
                       "Cannot use non-QObject type QRectF to access prefixed import") } },
                        { Message { QStringLiteral(
                       "Type not found in namespace") },
                          Message { QStringLiteral(
                       "Member \"BirthdayParty\" not found on type \"QRectF\"") } },
               };
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty.qml")
            << Result{
                   { Message{ QStringLiteral("Cannot assign to read-only property activeFocus") } },
                   {},
                   {}
               };
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty2.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to read-only property activeFocus") } } };
    QTest::newRow("cachedDependency")
            << QStringLiteral("cachedDependency.qml")
            << Result { { Message { QStringLiteral("Unused import"), 1, 1, QtInfoMsg } },
                        { Message { QStringLiteral(
                                "Cannot assign binding of type QQuickItem to QObject") } },
                        {},
                        Result::ExitsNormally };
    QTest::newRow("cycle in import")
            << QStringLiteral("cycleHead.qml")
            << Result { { Message { QStringLiteral(
                       "MenuItem is part of an inheritance cycle: MenuItem -> MenuItem") } } };
    QTest::newRow("badGeneralizedGroup1")
            << QStringLiteral("badGeneralizedGroup1.qml")
            << Result{ { Message{ QStringLiteral("Could not find property \"aaaa\".") } } };
    QTest::newRow("badGeneralizedGroup2")
            << QStringLiteral("badGeneralizedGroup2.qml")
            << Result { { Message { QStringLiteral("unknown grouped property scope aself") } } };
    QTest::newRow("missingQmltypes")
            << QStringLiteral("missingQmltypes.qml")
            << Result { { Message { QStringLiteral("QML types file does not exist") } } };
    QTest::newRow("enumInvalid")
            << QStringLiteral("enumInvalid.qml")
            << Result { {
                Message { QStringLiteral("Member \"red\" not found on type \"QtObject\""), 5, 25 },
                Message { QStringLiteral("Member \"red\" not found on type \"QtObject\""), 6, 25 },
                Message {
                    QStringLiteral("You cannot access unscoped enum \"Unscoped\" from here."),
                    8, 32
                },
                Message {
                    QStringLiteral("You cannot access unscoped enum \"Unscoped\" from here."),
                    9, 38
                },
                Message {
                    QStringLiteral("Member \"S2\" not found on type \"EnumTesterScoped\""),
                    10, 38
                },
               }, {
                Message { QStringLiteral("Did you mean \"S2\"?"), 0, 0, QtInfoMsg }
               }, {
                Message { QStringLiteral("Did you mean \"U2\"?"), 10, 38, QtInfoMsg }
               } };
    QTest::newRow("inaccessibleId")
            << QStringLiteral("inaccessibleId.qml")
            << Result { { Message {
                       QStringLiteral("Member \"objectName\" not found on type \"int\"") } } };
    QTest::newRow("inaccessibleId2")
            << QStringLiteral("inaccessibleId2.qml")
            << Result { { Message {
                       QStringLiteral("Member \"objectName\" not found on type \"int\"") } } };
    QTest::newRow("unknownTypeCustomParser")
            << QStringLiteral("unknownTypeCustomParser.qml")
            << Result { { Message { QStringLiteral("TypeDoesNotExist was not found.") } } };
    QTest::newRow("nonNullStored")
            << QStringLiteral("nonNullStored.qml")
            << Result { { Message { QStringLiteral(
                                "Member \"objectName\" not found on type \"Foozle\"") } },
                        { Message { QStringLiteral("Unqualified access") } } };
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-bindable")
            << QStringLiteral("badCppPropertyChangeHandlers1.qml")
            << Result { { Message { QStringLiteral(
                       "Signal handler for \"onAChanged\" has more formal parameters than "
                       "the signal it handles") } } };
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-notify")
            << QStringLiteral("badCppPropertyChangeHandlers2.qml")
            << Result { { Message { QStringLiteral(
                       "Signal handler for \"onBChanged\" has more formal parameters than "
                       "the signal it handles") } } };
    QTest::newRow("cppPropertyChangeHandlers-no-property")
            << QStringLiteral("badCppPropertyChangeHandlers3.qml")
            << Result { { Message {
                       QStringLiteral("no matching signal found for handler \"onXChanged\"") } } };
    QTest::newRow("cppPropertyChangeHandlers-not-a-signal")
            << QStringLiteral("badCppPropertyChangeHandlers4.qml")
            << Result { { Message { QStringLiteral(
                       "no matching signal found for handler \"onWannabeSignal\"") } } };
    QTest::newRow("didYouMean(binding)")
            << QStringLiteral("didYouMeanBinding.qml")
            << Result{ { Message{ QStringLiteral("Could not find property \"witdh\".") } },
                       {},
                       { Message{ QStringLiteral("width") } } };
    QTest::newRow("didYouMean(unqualified)")
            << QStringLiteral("didYouMeanUnqualified.qml")
            << Result { { Message { QStringLiteral("Unqualified access") } },
                        {},
                        { Message { QStringLiteral("height") } } };
    QTest::newRow("didYouMean(unqualifiedCall)")
            << QStringLiteral("didYouMeanUnqualifiedCall.qml")
            << Result { { Message { QStringLiteral("Unqualified access") } },
                        {},
                        { Message { QStringLiteral("func") } } };
    QTest::newRow("didYouMean(property)")
            << QStringLiteral("didYouMeanProperty.qml")
            << Result { { Message { QStringLiteral(
                                  "Member \"hoight\" not found on type \"Rectangle\"") },
                          {},
                          { Message { QStringLiteral("height") } } } };
    QTest::newRow("didYouMean(propertyCall)")
            << QStringLiteral("didYouMeanPropertyCall.qml")
            << Result {
                   { Message { QStringLiteral("Member \"lgg\" not found on type \"Console\"") },
                     {},
                     { Message { QStringLiteral("log") } } }
               };
    QTest::newRow("didYouMean(component)")
            << QStringLiteral("didYouMeanComponent.qml")
            << Result { { Message { QStringLiteral(
                                  "Itym was not found. Did you add all imports and dependencies?")
                                  }, {},
                        { Message { QStringLiteral("Item") } } } };
    QTest::newRow("didYouMean(enum)")
            << QStringLiteral("didYouMeanEnum.qml")
            << Result { { Message { QStringLiteral(
                                  "Member \"Readx\" not found on type \"QQuickImage\"") },
                          {},
                          { Message { QStringLiteral("Ready") } } } };
    QTest::newRow("nullBinding") << QStringLiteral("nullBinding.qml")
                                 << Result{ { Message{ QStringLiteral(
                                            "Cannot assign literal of type null to double") } } };
    QTest::newRow("missingRequiredAlias")
            << QStringLiteral("missingRequiredAlias.qml")
            << Result{ { Message{ u"Component is missing required property foo from Item"_s } } };
    QTest::newRow("missingSingletonPragma")
            << QStringLiteral("missingSingletonPragma.qml")
            << Result { { Message { QStringLiteral(
                       "Type MissingPragma declared as singleton in qmldir but missing "
                       "pragma Singleton") } } };
    QTest::newRow("missingSingletonQmldir")
            << QStringLiteral("missingSingletonQmldir.qml")
            << Result { { Message { QStringLiteral(
                       "Type MissingQmldirSingleton not declared as singleton in qmldir but using "
                       "pragma Singleton") } } };
    QTest::newRow("jsVarDeclarationsWriteConst")
            << QStringLiteral("jsVarDeclarationsWriteConst.qml")
            << Result { { Message {
                       QStringLiteral("Cannot assign to read-only property constProp") } } };
    QTest::newRow("shadowedSignal")
            << QStringLiteral("shadowedSignal.qml")
            << Result { { Message {
                       QStringLiteral("Signal \"pressed\" is shadowed by a property.") } } };
    QTest::newRow("shadowedSignalWithId")
            << QStringLiteral("shadowedSignalWithId.qml")
            << Result { { Message {
                       QStringLiteral("Signal \"pressed\" is shadowed by a property") } } };
    QTest::newRow("shadowedSlot") << QStringLiteral("shadowedSlot.qml")
                                  << Result { { Message { QStringLiteral(
                                             "Slot \"move\" is shadowed by a property") } } };
    QTest::newRow("shadowedMethod") << QStringLiteral("shadowedMethod.qml")
                                    << Result { { Message { QStringLiteral(
                                               "Method \"foo\" is shadowed by a property.") } } };
    QTest::newRow("callVarProp")
            << QStringLiteral("callVarProp.qml")
            << Result { { Message { QStringLiteral(
                       "Property \"foo\" is a var property. It may or may not be a "
                       "method. Use a regular function instead.") } } };
    QTest::newRow("callJSValue")
            << QStringLiteral("callJSValueProp.qml")
            << Result { { Message { QStringLiteral(
                       "Property \"jsValue\" is a QJSValue property. It may or may not be "
                       "a method. Use a regular Q_INVOKABLE instead.") } } };
    QTest::newRow("assignNonExistingTypeToVarProp")
            << QStringLiteral("assignNonExistingTypeToVarProp.qml")
            << Result { { Message { QStringLiteral(
                       "NonExistingType was not found. Did you add all imports and dependencies?")
               } } };
    QTest::newRow("unboundComponents")
            << QStringLiteral("unboundComponents.qml")
            << Result { {
                         Message { QStringLiteral("Unqualified access"), 10, 25 },
                         Message { QStringLiteral("Unqualified access"), 14, 33 }
               } };
    QTest::newRow("badlyBoundComponents")
            << QStringLiteral("badlyBoundComponents.qml")
            << Result{ { Message{ QStringLiteral("Unqualified access"), 18, 36 } } };
    QTest::newRow("NotScopedEnumCpp")
            << QStringLiteral("NotScopedEnumCpp.qml")
            << Result{ { Message{
                       QStringLiteral("You cannot access unscoped enum \"TheEnum\" from here."), 5,
                       49 } } };

    QTest::newRow("unresolvedArrayBinding")
            << QStringLiteral("unresolvedArrayBinding.qml")
            << Result{ { Message{ QStringLiteral(u"Declaring an object which is not an Qml object"
                                                 " as a list member.") } },
                       {},
                       {} };
    QTest::newRow("duplicatedPropertyName")
            << QStringLiteral("duplicatedPropertyName.qml")
            << Result{ { Message{ QStringLiteral("Duplicated property name \"cat\"."), 5, 5 } } };
    QTest::newRow("duplicatedSignalName")
            << QStringLiteral("duplicatedPropertyName.qml")
            << Result{ { Message{ QStringLiteral("Duplicated signal name \"clicked\"."), 8, 5 } } };
    QTest::newRow("missingComponentBehaviorBound")
            << QStringLiteral("missingComponentBehaviorBound.qml")
            << Result {
                    {  Message{ QStringLiteral("Unqualified access"), 8, 31  } },
                    {},
                    {  Message{ QStringLiteral("Set \"pragma ComponentBehavior: Bound\" in "
                                               "order to use IDs from outer components "
                                               "in nested components."), 0, 0, QtInfoMsg } },
                    Result::AutoFixable
                };
    QTest::newRow("IsNotAnEntryOfEnum")
            << QStringLiteral("IsNotAnEntryOfEnum.qml")
            << Result{ {
                         Message {
                                  QStringLiteral("Member \"Mode\" not found on type \"Item\""), 12,
                                  29},
                          Message{
                                  QStringLiteral("\"Hour\" is not an entry of enum \"Mode\"."), 13,
                                  62}
                       },
                       {},
                       { Message{ QStringLiteral("Hours") } }
               };

    QTest::newRow("StoreNameMethod")
            << QStringLiteral("storeNameMethod.qml")
            << Result { { Message { QStringLiteral("Cannot assign to method foo") } } };

    QTest::newRow("CoerceToVoid")
            << QStringLiteral("coercetovoid.qml")
            << Result { { Message {
                    QStringLiteral("Function without return type annotation returns double")
               } } };

    QTest::newRow("annotatedDefaultParameter")
            << QStringLiteral("annotatedDefaultParameter.qml")
            << Result { { Message {
                       QStringLiteral("Type annotations on default parameters are not supported")
               } } };


    QTest::newRow("lowerCaseQualifiedImport")
            << QStringLiteral("lowerCaseQualifiedImport.qml")
            << Result{
                   {
                           Message{
                                   u"Import qualifier 'test' must start with a capital letter."_s },
                           Message{
                                   u"Namespace 'test' of 'test.Rectangle' must start with an upper case letter."_s },
                   },
                   {},
                   {}
               };
    QTest::newRow("lowerCaseQualifiedImport2")
            << QStringLiteral("lowerCaseQualifiedImport2.qml")
            << Result{
                   {
                           Message{
                                   u"Import qualifier 'test' must start with a capital letter."_s },
                           Message{
                                   u"Namespace 'test' of 'test.Item' must start with an upper case letter."_s },
                           Message{
                                   u"Namespace 'test' of 'test.Rectangle' must start with an upper case letter."_s },
                           Message{
                                   u"Namespace 'test' of 'test.color' must start with an upper case letter."_s },
                           Message{
                                   u"Namespace 'test' of 'test.Grid' must start with an upper case letter."_s },
                   },
                   {},
                   {}
               };
    QTest::newRow("notQmlRootMethods")
            << QStringLiteral("notQmlRootMethods.qml")
            << Result{ {
                       Message{ u"Member \"deleteLater\" not found on type \"QtObject\""_s },
                       Message{ u"Member \"destroyed\" not found on type \"QtObject\""_s },
               } };

    QTest::newRow("connectionsBinding")
            << QStringLiteral("autofix/ConnectionsHandler.qml")
            << Result{
                   { Message{
                             u"Implicitly defining \"onWidthChanged\" as signal handler in "
                             u"Connections is deprecated. "
                             u"Create a function instead: \"function onWidthChanged() { ... }\"."_s },
                     Message{
                             u"Implicitly defining \"onColorChanged\" as signal handler in "
                             u"Connections is deprecated. "
                             u"Create a function instead: \"function onColorChanged(collie) { ... }\"."_s } },
               };
    QTest::newRow("autoFixConnectionsBinding")
            << QStringLiteral("autofix/ConnectionsHandler.qml")
            << Result{
                   { Message{
                             u"Implicitly defining \"onWidthChanged\" as signal handler in "
                             u"Connections is deprecated. "
                             u"Create a function instead: \"function onWidthChanged() { ... }\"."_s },
                     Message{
                             u"Implicitly defining \"onColorChanged\" as signal handler in "
                             u"Connections is deprecated. "
                             u"Create a function instead: \"function onColorChanged(collie) { ... }\"."_s } },
                   {},
                   {
                           Message{ u"function onWidthChanged() { console.log(\"new width:\", width) }"_s },
                           Message{ u"function onColorChanged(col) { console.log(\"new color:\", col) }"_s },
                   },
                   };
    QTest::newRow("unresolvedTypeAnnotation")
            << QStringLiteral("unresolvedTypeAnnotations.qml")
            << Result{{
                       { uR"("A" was not found for the type of parameter "a" in method "f".)"_s, 4, 17 },
                       { uR"("B" was not found for the type of parameter "b" in method "f".)"_s, 4, 23 },
                       { uR"("R" was not found for the return type of method "g".)"_s, 5, 18 },
                       { uR"("C" was not found for the type of parameter "c" in method "h".)"_s, 6, 17 },
                       { uR"("R" was not found for the return type of method "h".)"_s, 6, 22 },
                       { uR"("D" was not found for the type of parameter "d" in method "i".)"_s, 7, 17 },
                       { uR"("G" was not found for the type of parameter "g" in method "i".)"_s, 7, 26 },
               }};

    {
        const QString warning = u"Do not mix translation functions"_s;
        QTest::addRow("BadTranslationMix") << testFile(u"translations/BadMix.qml"_s)
                                           << Result{ {
                                                      Message{ warning, 5, 49 },
                                                      Message{ warning, 6, 56 },
                                              } };
        QTest::addRow("BadTranslationMixWithMacros")
                << testFile(u"translations/BadMixWithMacros.qml"_s)
                << Result{ {
                           Message{ warning, 5, 29 },
                   } };
    }

    // We want to see the warning about the missing type only once.
    QTest::newRow("unresolvedType2")
            << QStringLiteral("unresolvedType2.qml")
            << Result { { Message { QStringLiteral(
                           "QQC2.Label was not found. Did you add all imports and dependencies?") } },
                       { Message { QStringLiteral(
                           "'QQC2.Label' is used but it is not resolved") },
                         Message { QStringLiteral(
                           "Type QQC2.Label is used but it is not resolved") } },
                       };

    // The warning should show up only once even though
    // we have to run the type propagator multiple times.
    QTest::newRow("multiplePasses")
            << testFile("multiplePasses.qml")
            << Result {{ Message { QStringLiteral("Unqualified access") }}};
    QTest::newRow("missingRequiredOnObjectDefinitionBinding")
            << QStringLiteral("missingRequiredPropertyOnObjectDefinitionBinding.qml")
            << Result{ { { uR"(Component is missing required property i from here)"_s, 4, 26 } } };

    QTest::newRow("PropertyAliasCycles") << QStringLiteral("settings/propertyAliasCycle/file.qml")
                                         << Result::cleanWithSettings();
    // make sure that warnings are triggered without settings:
    QTest::newRow("PropertyAliasCycles2")
            << QStringLiteral("settings/propertyAliasCycle/file.qml")
            << Result{ { { "\"cycle1\" is part of an alias cycle"_L1 },
                         { "\"cycle1\" is part of an alias cycle"_L1 } } };
    QTest::newRow("redundantOptionalChainingEnums")
            << QStringLiteral("RedundantOptionalChainingEnums.qml")
            << Result{ { { "Redundant optional chaining for enum lookup"_L1, 5, 54 },
                         { "Redundant optional chaining for enum lookup"_L1, 6, 26 } } };
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    QEXPECT_FAIL("attachedPropertyAccess", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("attachedPropertyNested", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("BadLiteralBindingDate",
                 "We're currently not able to verify any non-trivial QString conversion that "
                 "requires QQmlStringConverters",
                 Abort);
    QEXPECT_FAIL("bad tranlsation binding (qsTr)", "We currently do not check translation binding",
                 Abort);

    QEXPECT_FAIL("BadTranslationMixWithMacros",
                 "Translation macros are currently invisible to QQmlJSTypePropagator", Abort);

    CallQmllintOptions options;
    options.readSettings = result.flags.testFlag(Result::UseSettings);
    const QJsonArray warnings = callQmllint(filename, options, fromResultFlags(result.flags));

    checkResult(
            warnings, result,
            [] {
                QEXPECT_FAIL("BadLiteralBindingDate",
                             "We're currently not able to verify any non-trivial QString "
                             "conversion that "
                             "requires QQmlStringConverters",
                             Abort);
                QEXPECT_FAIL("BadTranslationMixWithMacros",
                             "Translation macros are currently invisible to QQmlJSTypePropagator",
                             Abort);
            },
            [] {
                QEXPECT_FAIL("badAttachedPropertyNested",
                             "We cannot discern between types and instances", Abort);
            },
            [] {
                QEXPECT_FAIL("autoFixConnectionsBinding",
                             "We can't autofix the code without the Dom.", Abort);
            });
}

static void addLocationOffsetTo(TestQmllint::Result *result, qsizetype lineOffset,
                                qsizetype columnOffset = 0)
{
    for (auto *messages :
         { &result->expectedMessages, &result->badMessages, &result->expectedReplacements }) {
        for (auto &message : *messages) {
            if (message.line != 0)
                message.line += lineOffset;
            if (message.column != 0)
                message.column += columnOffset;
        }
    }
}

void TestQmllint::dirtyQmlSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("testSnippet")
            << u"property int qwer: \"Hello\""_s
            << Result{ { { "Cannot assign literal of type string to int"_L1 } } } << defaultOptions;

    QTest::newRow("enum") << u"enum Hello { World, Kitty, World, dlrow }"_s
                          << Result{ { { "Enum key 'World' has already been declared"_L1, 1, 28 },
                                       { "Note: previous declaration of 'World' here"_L1, 1, 14 },
                                       { "Enum keys should start with an uppercase"_L1, 1, 35 } } }
                          << defaultOptions;

    QTest::newRow("color-name") << u"property color myColor: \"lbue\""_s
                                << Result{ { { "Invalid color \"lbue\""_L1, 1, 25 } },
                                           {},
                                           { { "Did you mean \"blue\"?", 1, 25 } } }
                                << defaultOptions;
    QTest::newRow("color-hex") << u"property color myColor: \"#12345\""_s
                               << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;
    QTest::newRow("color-hex2") << u"property color myColor: \"#123456789\""_s
                                << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;
    QTest::newRow("color-hex3") << u"property color myColor: \"##123456\""_s
                                << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;
    QTest::newRow("color-hex4") << u"property color myColor: \"#123456#\""_s
                                << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;
    QTest::newRow("color-hex5") << u"property color myColor: \"#HELLOL\""_s
                                << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;
    QTest::newRow("color-hex6") << u"property color myColor: \"#1234567\""_s
                                << Result{ { { "Invalid color"_L1, 1, 25 } } } << defaultOptions;

    QTest::newRow("upperCaseId")
            << u"id: Root"_s
            << Result{ { { "Id must start with a lower case letter or an '_'"_L1, 1, 5 } } }
            << defaultOptions;
}

void TestQmllint::dirtyQmlSnippet()
{
    QFETCH(QString, code);
    QFETCH(Result, result);
    QFETCH(CallQmllintOptions, options);

    QString qmlCode = "import QtQuick\nItem {\n%1}"_L1.arg(code);

    addLocationOffsetTo(&result, 2);

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));

    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::cleanQmlSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("testSnippet") << u"property int qwer: 123"_s << defaultOptions;
    QTest::newRow("enum") << u"enum Hello { World, Kitty, DlroW }"_s << defaultOptions;

    QTest::newRow("color-name") << u"property color myColor: \"blue\""_s << defaultOptions;
    QTest::newRow("color-name2") << u"property color myColor\nmyColor: \"green\""_s
                                 << defaultOptions;
    QTest::newRow("color-hex") << u"property color myColor: \"#123456\""_s << defaultOptions;
    QTest::newRow("color-hex2") << u"property color myColor: \"#FFFFFFFF\""_s << defaultOptions;
    QTest::newRow("color-hex3") << u"property color myColor: \"#A0AAff1f\""_s << defaultOptions;

    QTest::newRow("lowerCaseId") << u"id: root"_s << defaultOptions;
    QTest::newRow("underScoreId") << u"id: _Root"_s << defaultOptions;
}

void TestQmllint::cleanQmlSnippet()
{
    QFETCH(QString, code);
    QFETCH(CallQmllintOptions, options);

    const QString qmlCode = "import QtQuick\nItem {%1}"_L1.arg(code);
    const Result result = Result::clean();

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));
    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::dirtyJsSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("doubleLet")
            << u"let x = 4; let x = 4;"_s
            << Result{ { { "Identifier 'x' has already been declared"_L1, 1, 16 },
                         { "Note: previous declaration of 'x' here"_L1, 1, 5 } } }
            << defaultOptions;
    QTest::newRow("doubleConst")
            << u"const x = 4; const x = 4;"_s
            << Result{ { { "Identifier 'x' has already been declared"_L1, 1, 20 },
                         { "Note: previous declaration of 'x' here"_L1, 1, 7 } } }
            << defaultOptions;

    QTest::newRow("assignmentInCondition")
            << u"let xxx = 3; if (xxx=3) return;"_s
            << Result{ { { "Assignment in condition: did you meant to use \"===\" or \"==\" "
                           "instead of \"=\"?"_L1,
                           1, 21 } } }
            << defaultOptions;
    QTest::newRow("eval") << u"let x = eval();"_s << Result{ { { "Do not use 'eval'"_L1, 1, 9 } } }
                          << defaultOptions;
    QTest::newRow("eval2") << u"let x = eval(\"1 + 1\");"_s
                           << Result{ { { "Do not use 'eval'"_L1, 1, 9 } } } << defaultOptions;
    QTest::newRow("indirectEval") << u"let x = (1, eval)();"_s
                                  << Result{ { { "Do not use 'eval'"_L1, 1, 13 } } }
                                  << defaultOptions;
    QTest::newRow("indirectEval2")
            << u"let x = (1, eval)(\"1 + 1\");"_s << Result{ { { "Do not use 'eval'"_L1, 1, 13 } } }
            << defaultOptions;
    QTest::newRow("redundantOptionalChainingNonVoidableBase")
            << u"/a/?.flags"_s
            << Result{ { { "Redundant optional chaining for lookup on non-voidable and "_L1
                           "non-nullable type QRegularExpression"_L1,
                           1, 6 } } }
            << defaultOptions;

    QTest::newRow("shadowArgument")
            << u"function f(a) { const a = 33; }"_s
            << Result{ { { "Identifier 'a' has already been declared"_L1, 1, 23 },
                         { "Note: previous declaration of 'a' here"_L1, 1, 12 } } }
            << defaultOptions;
    QTest::newRow("shadowFunction")
            << u"function f() {} const f = 33"_s
            << Result{ { { "Identifier 'f' has already been declared"_L1, 1, 23 },
                         { "Note: previous declaration of 'f' here"_L1, 1, 10 } } }
            << defaultOptions;
    QTest::newRow("shadowFunction2")
            << u"const f = 33; function f() {}"_s
            << Result{ { { "Identifier 'f' has already been declared"_L1, 1, 24 },
                         { "Note: previous declaration of 'f' here"_L1, 1, 7 } } }
            << defaultOptions;
    QTest::newRow("assignmentWarningLocation")
            << u"console.log(a = 1)"_s
            << Result{ { { "Unqualified access"_L1, 1, 13 } } }
            << defaultOptions;
}

void TestQmllint::dirtyJsSnippet()
{
    QFETCH(QString, code);
    QFETCH(Result, result);
    QFETCH(CallQmllintOptions, options);

    static constexpr QLatin1StringView templateString =
            "import QtQuick\nItem { function f() {\n%1}}"_L1;

    // templateString adds 2 newlines before snippet
    addLocationOffsetTo(&result, 2);

    const QString qmlCode = templateString.arg(code);
    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));

    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::cleanJsSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("testSnippet") << u"let x = 5"_s << defaultOptions;
    QTest::newRow("doubleVar") << u"var x = 5; var y = 5"_s << defaultOptions;
    QTest::newRow("doubleInDifferentScopes")
            << u"const a = 42; for (let a = 1; a < 10; ++a) {}"_s << defaultOptions;

    QTest::newRow("notAssignmentInCondition")
            << u"let x = 3; if (x==3) return;"_s << defaultOptions;

    QTest::newRow("shadowArgument")
            << u"function f(a) { if (1){ const a = 33; } }"_s << defaultOptions;
    QTest::newRow("shadowFunction") << u"function f() { function f() {} }"_s << defaultOptions;

    QTest::newRow("varUsedBeforeDeclarationWithIgnore")
            << u"// qmllint disable var-used-before-declaration\n"
               "f(x) ;\n"
               "// qmllint enable var-used-before-declaration\n"
               "let x = 3;"_s
            << defaultOptions;
}

void TestQmllint::cleanJsSnippet()
{
    QFETCH(QString, code);
    QFETCH(CallQmllintOptions, options);

    const QString qmlCode = "import QtQuick\nItem { function f() {\n%1}}"_L1.arg(code);
    const Result result = Result::clean();

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));
    checkResult(warnings, result, [] { }, [] { }, [] { });
}


void TestQmllint::cleanQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("Simple_QML")                << QStringLiteral("Simple.qml");
    QTest::newRow("QML_importing_JS")          << QStringLiteral("importing_js.qml");
    QTest::newRow("JS_with_pragma_and_import") << QStringLiteral("QTBUG-45916.js");
    QTest::newRow("uiQml")                     << QStringLiteral("FormUser.qml");
    QTest::newRow("methodInScope")             << QStringLiteral("MethodInScope.qml");
    QTest::newRow("importWithPrefix")          << QStringLiteral("ImportWithPrefix.qml");
    QTest::newRow("catchIdentifier")           << QStringLiteral("catchIdentifierNoWarning.qml");
    QTest::newRow("qmldirAndQmltypes")         << QStringLiteral("qmldirAndQmltypes.qml");
    QTest::newRow("forLoop")                   << QStringLiteral("forLoop.qml");
    QTest::newRow("esmodule")                  << QStringLiteral("esmodule.mjs");
    QTest::newRow("methodsInJavascript")       << QStringLiteral("javascriptMethods.qml");
    QTest::newRow("goodAlias")                 << QStringLiteral("goodAlias.qml");
    QTest::newRow("goodParent")                << QStringLiteral("goodParent.qml");
    QTest::newRow("goodTypeAssertion")         << QStringLiteral("goodTypeAssertion.qml");
    QTest::newRow("AttachedProps")             << QStringLiteral("AttachedProps.qml");
    QTest::newRow("unknownBuiltinFont")        << QStringLiteral("ButtonLoader.qml");
    QTest::newRow("confusingImport")           << QStringLiteral("Dialog.qml");
    QTest::newRow("qualifiedAttached")         << QStringLiteral("Drawer.qml");
    QTest::newRow("EnumAccess1") << QStringLiteral("EnumAccess1.qml");
    QTest::newRow("EnumAccess2") << QStringLiteral("EnumAccess2.qml");
    QTest::newRow("ListProperty") << QStringLiteral("ListProperty.qml");
    QTest::newRow("AttachedType") << QStringLiteral("AttachedType.qml");
    QTest::newRow("qmldirImportAndDepend") << QStringLiteral("qmldirImportAndDepend/good.qml");
    QTest::newRow("ParentEnum") << QStringLiteral("parentEnum.qml");
    QTest::newRow("Signals") << QStringLiteral("Signal.qml");
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleGood.qml");
    QTest::newRow("enumFromQtQml") << QStringLiteral("enumFromQtQml.qml");
    QTest::newRow("anchors1") << QStringLiteral("anchors1.qml");
    QTest::newRow("anchors2") << QStringLiteral("anchors2.qml");
    QTest::newRow("defaultImport") << QStringLiteral("defaultImport.qml");
    QTest::newRow("goodAliasObject") << QStringLiteral("goodAliasObject.qml");
    QTest::newRow("jsmoduleimport") << QStringLiteral("jsmoduleimport.qml");
    QTest::newRow("overridescript") << QStringLiteral("overridescript.qml");
    QTest::newRow("multiExtension") << QStringLiteral("multiExtension.qml");
    QTest::newRow("segFault") << QStringLiteral("SegFault.qml");
    QTest::newRow("grouped scope failure") << QStringLiteral("groupedScope.qml");
    QTest::newRow("layouts depends quick") << QStringLiteral("layouts.qml");
    QTest::newRow("attached") << QStringLiteral("attached.qml");
    QTest::newRow("enumProperty") << QStringLiteral("enumProperty.qml");
    QTest::newRow("externalEnumProperty") << QStringLiteral("externalEnumProperty.qml");
    QTest::newRow("shapes") << QStringLiteral("shapes.qml");
    QTest::newRow("var") << QStringLiteral("var.qml");
    QTest::newRow("defaultProperty") << QStringLiteral("defaultProperty.qml");
    QTest::newRow("defaultPropertyList") << QStringLiteral("defaultPropertyList.qml");
    QTest::newRow("defaultPropertyComponent") << QStringLiteral("defaultPropertyComponent.qml");
    QTest::newRow("defaultPropertyComponent2") << QStringLiteral("defaultPropertyComponent.2.qml");
    QTest::newRow("defaultPropertyListModel") << QStringLiteral("defaultPropertyListModel.qml");
    QTest::newRow("defaultPropertyVar") << QStringLiteral("defaultPropertyVar.qml");
    QTest::newRow("multiDefaultProperty") << QStringLiteral("multiDefaultPropertyOk.qml");
    QTest::newRow("propertyDelegate") << QStringLiteral("propertyDelegate.qml");
    QTest::newRow("duplicateQmldirImport") << QStringLiteral("qmldirImport/duplicate.qml");
    QTest::newRow("Used imports") << QStringLiteral("used.qml");
    QTest::newRow("Unused imports (multi)") << QStringLiteral("unused_multi.qml");
    QTest::newRow("Unused static module") << QStringLiteral("unused_static.qml");
    QTest::newRow("compositeSingleton") << QStringLiteral("compositesingleton.qml");
    QTest::newRow("stringLength") << QStringLiteral("stringLength.qml");
    QTest::newRow("stringLength2") << QStringLiteral("stringLength2.qml");
    QTest::newRow("stringLength3") << QStringLiteral("stringLength3.qml");
    QTest::newRow("attachedPropertyAssignments")
            << QStringLiteral("attachedPropertyAssignments.qml");
    QTest::newRow("groupedPropertyAssignments") << QStringLiteral("groupedPropertyAssignments.qml");
    QTest::newRow("goodAttachedProperty") << QStringLiteral("goodAttachedProperty.qml");
    QTest::newRow("objectBindingOnVarProperty") << QStringLiteral("objectBoundToVar.qml");
    QTest::newRow("Unversioned change signal without arguments") << QStringLiteral("unversionChangedSignalSansArguments.qml");
    QTest::newRow("deprecatedFunctionOverride") << QStringLiteral("deprecatedFunctionOverride.qml");
    QTest::newRow("multilineStringEscaped") << QStringLiteral("multilineStringEscaped.qml");
    QTest::newRow("propertyOverride") << QStringLiteral("propertyOverride.qml");
    QTest::newRow("propertyBindingValue") << QStringLiteral("propertyBindingValue.qml");
    QTest::newRow("customParser") << QStringLiteral("customParser.qml");
    QTest::newRow("customParser.recursive") << QStringLiteral("customParser.recursive.qml");
    QTest::newRow("2Behavior") << QStringLiteral("2behavior.qml");
    QTest::newRow("interceptor") << QStringLiteral("interceptor.qml");
    QTest::newRow("valueSource") << QStringLiteral("valueSource.qml");
    QTest::newRow("interceptor+valueSource") << QStringLiteral("interceptor_valueSource.qml");
    QTest::newRow("groupedProperty (valueSource+interceptor)")
            << QStringLiteral("groupedProperty_valueSource_interceptor.qml");
    QTest::newRow("QtQuick.Window 2.1") << QStringLiteral("qtquickWindow21.qml");
    QTest::newRow("attachedTypeIndirect") << QStringLiteral("attachedTypeIndirect.qml");
    QTest::newRow("objectArray") << QStringLiteral("objectArray.qml");
    QTest::newRow("aliasToList") << QStringLiteral("aliasToList.qml");
    QTest::newRow("QVariant") << QStringLiteral("qvariant.qml");
    QTest::newRow("Accessible") << QStringLiteral("accessible.qml");
    QTest::newRow("qjsroot") << QStringLiteral("qjsroot.qml");
    QTest::newRow("qmlRootMethods") << QStringLiteral("qmlRootMethods.qml");
    QTest::newRow("InlineComponent") << QStringLiteral("inlineComponent.qml");
    QTest::newRow("InlineComponentWithComponents") << QStringLiteral("inlineComponentWithComponents.qml");
    QTest::newRow("InlineComponentsChained") << QStringLiteral("inlineComponentsChained.qml");
    QTest::newRow("ignoreWarnings") << QStringLiteral("ignoreWarnings.qml");
    QTest::newRow("BindingBeforeDeclaration") << QStringLiteral("bindingBeforeDeclaration.qml");
    QTest::newRow("CustomParserUnqualifiedAccess")
            << QStringLiteral("customParserUnqualifiedAccess.qml");
    QTest::newRow("ImportQMLModule") << QStringLiteral("importQMLModule.qml");
    QTest::newRow("ImportDirectoryQmldir") << QStringLiteral("Things/LintDirectly.qml");
    QTest::newRow("BindingsOnGroupAndAttachedProperties")
            << QStringLiteral("goodBindingsOnGroupAndAttached.qml");
    QTest::newRow("QQmlEasingEnums::Type") << QStringLiteral("animationEasing.qml");
    QTest::newRow("ValidLiterals") << QStringLiteral("validLiterals.qml");
    QTest::newRow("GoodModulePrefix") << QStringLiteral("goodModulePrefix.qml");
    QTest::newRow("required_property_in_Component") << QStringLiteral("requiredPropertyInComponent.qml");
    QTest::newRow("requiredPropertyInGroupedPropertyScope") << QStringLiteral("requiredPropertyInGroupedPropertyScope.qml");
    QTest::newRow("bytearray") << QStringLiteral("bytearray.qml");
    QTest::newRow("initReadonly") << QStringLiteral("initReadonly.qml");
    QTest::newRow("connectionNoParent") << QStringLiteral("connectionNoParent.qml"); // QTBUG-97600
    QTest::newRow("goodGeneralizedGroup") << QStringLiteral("goodGeneralizedGroup.qml");
    QTest::newRow("on binding in grouped property") << QStringLiteral("onBindingInGroupedProperty.qml");
    QTest::newRow("declared property of JS object") << QStringLiteral("bareQt.qml");
    QTest::newRow("ID overrides property") << QStringLiteral("accessibleId.qml");
    QTest::newRow("matchByName") << QStringLiteral("matchByName.qml");
    QTest::newRow("QObject.hasOwnProperty") << QStringLiteral("qobjectHasOwnProperty.qml");
    QTest::newRow("cppPropertyChangeHandlers")
            << QStringLiteral("goodCppPropertyChangeHandlers.qml");
    QTest::newRow("unexportedCppBase") << QStringLiteral("unexportedCppBase.qml");
    QTest::newRow("requiredWithRootLevelAlias") << QStringLiteral("RequiredWithRootLevelAlias.qml");
    QTest::newRow("jsVarDeclarations") << QStringLiteral("jsVarDeclarations.qml");
    QTest::newRow("qmodelIndex") << QStringLiteral("qmodelIndex.qml");
    QTest::newRow("boundComponents") << QStringLiteral("boundComponents.qml");
    QTest::newRow("prefixedAttachedProperty") << QStringLiteral("prefixedAttachedProperty.qml");
    QTest::newRow("callLater") << QStringLiteral("callLater.qml");
    QTest::newRow("listPropertyMethods") << QStringLiteral("listPropertyMethods.qml");
    QTest::newRow("v4SequenceMethods") << QStringLiteral("v4SequenceMethods.qml");
    QTest::newRow("stringToByteArray") << QStringLiteral("stringToByteArray.qml");
    QTest::newRow("jsLibrary") << QStringLiteral("jsLibrary.qml");
    QTest::newRow("nullBindingFunction") << QStringLiteral("nullBindingFunction.qml");
    QTest::newRow("BindingTypeMismatchFunction") << QStringLiteral("bindingTypeMismatchFunction.qml");
    QTest::newRow("BindingTypeMismatch") << QStringLiteral("bindingTypeMismatch.qml");
    QTest::newRow("template literal (substitution)") << QStringLiteral("templateStringSubstitution.qml");
    QTest::newRow("enumsOfScrollBar") << QStringLiteral("enumsOfScrollBar.qml");
    QTest::newRow("optionalChainingCall") << QStringLiteral("optionalChainingCall.qml");
    QTest::newRow("EnumAccessCpp") << QStringLiteral("EnumAccessCpp.qml");
    QTest::newRow("qtquickdialog") << QStringLiteral("qtquickdialog.qml");
    QTest::newRow("callBase") << QStringLiteral("callBase.qml");
    QTest::newRow("propertyWithOn") << QStringLiteral("switcher.qml");
    QTest::newRow("constructorProperty") << QStringLiteral("constructorProperty.qml");
    QTest::newRow("onlyMajorVersion") << QStringLiteral("onlyMajorVersion.qml");
    QTest::newRow("attachedImportUse") << QStringLiteral("attachedImportUse.qml");
    QTest::newRow("VariantMapGetPropertyLookup") << QStringLiteral("variantMapLookup.qml");
    QTest::newRow("StringToDateTime") << QStringLiteral("stringToDateTime.qml");
    QTest::newRow("ScriptInTemplate") << QStringLiteral("scriptInTemplate.qml");
    QTest::newRow("AddressableValue") << QStringLiteral("addressableValue.qml");
    QTest::newRow("WriteListProperty") << QStringLiteral("writeListProperty.qml");
    QTest::newRow("dontConfuseMemberPrintWithGlobalPrint") << QStringLiteral("findMemberPrint.qml");
    QTest::newRow("groupedAttachedLayout") << QStringLiteral("groupedAttachedLayout.qml");
    QTest::newRow("QQmlScriptString") << QStringLiteral("scriptstring.qml");
    QTest::newRow("QEventPoint") << QStringLiteral("qEventPoint.qml");
    QTest::newRow("locale") << QStringLiteral("locale.qml");
    QTest::newRow("constInvokable") << QStringLiteral("useConstInvokable.qml");
    QTest::newRow("dontCheckJSTypes") << QStringLiteral("dontCheckJSTypes.qml");
    QTest::newRow("jsonObjectIsRecognized") << QStringLiteral("jsonObjectIsRecognized.qml");
    QTest::newRow("jsonArrayIsRecognized") << QStringLiteral("jsonArrayIsRecognized.qml");
    QTest::newRow("itemviewattached") << QStringLiteral("itemViewAttached.qml");
    QTest::newRow("scopedAndUnscopedEnums") << QStringLiteral("enumValid.qml");
    QTest::newRow("dependsOnDuplicateType") << QStringLiteral("dependsOnDuplicateType.qml");
#ifdef HAS_QC_BASIC
    QTest::newRow("overlay") << QStringLiteral("overlayFromControls.qml");
#endif
    QTest::newRow("thisObject") << QStringLiteral("thisObject.qml");
    QTest::newRow("aliasGroup") << QStringLiteral("aliasGroup.qml");

    QTest::addRow("ValidTranslations") << u"translations/qsTranslateTranslation.qml"_s;
    QTest::addRow("ValidTranslations2") << u"translations/qsTrTranslation.qml"_s;
    QTest::addRow("ValidTranslations3") << u"translations/qsTrIdTranslation.qml"_s;
    QTest::addRow("ValidTranslations4") << u"translations/Good.qml"_s;
    QTest::addRow("deceptiveLayout") << u"deceptiveLayout.qml"_s;
    QTest::addRow("regExp") << u"regExp.qml"_s;
    QTest::newRow("aliasToRequiredProperty")
            << QStringLiteral("aliasToRequiredPropertyIsNotRequiredItself.qml");
    QTest::newRow("setRequiredTroughAlias") << QStringLiteral("setRequiredPropertyThroughAlias.qml");
    QTest::newRow("setRequiredTroughAliasOfAlias")
            << QStringLiteral("setRequiredPropertyThroughAliasOfAlias.qml");
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);

    QJsonArray warnings;

    runTest(filename, Result::clean());
}

void TestQmllint::compilerWarnings_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");
    QTest::addColumn<bool>("enableCompilerWarnings");

    QTest::newRow("listIndices") << QStringLiteral("listIndices.qml") << Result::clean() << true;
    QTest::newRow("lazyAndDirect")
            << QStringLiteral("LazyAndDirect/Lazy.qml") << Result::clean() << true;
    QTest::newRow("qQmlV4Function") << QStringLiteral("varargs.qml") << Result::clean() << true;
    QTest::newRow("multiGrouped") << QStringLiteral("multiGrouped.qml") << Result::clean() << true;

    QTest::newRow("shadowable")
            << QStringLiteral("shadowable.qml")
            << Result { { Message {QStringLiteral("with type NotSoSimple can be shadowed") } } }
            << true;
    QTest::newRow("tooFewParameters")
            << QStringLiteral("tooFewParams.qml")
            << Result { { Message { QStringLiteral("Could not compile binding for a: "
                                                   "No matching override found") } } } << true;
    QTest::newRow("javascriptVariableArgs")
            << QStringLiteral("javascriptVariableArgs.qml")
            << Result { { Message {
                       QStringLiteral("Could not compile binding for onCompleted: "
                                      "Function expects 0 arguments, but 2 were provided") } } }
            << true;
    QTest::newRow("unknownTypeInRegister")
            << QStringLiteral("unknownTypeInRegister.qml")
            << Result { { Message {
                       QStringLiteral("Could not determine signature of function foo: "
                                      "Functions without type annotations won't be compiled") } } }
            << true;
    QTest::newRow("pragmaStrict")
            << QStringLiteral("pragmaStrict.qml")
            << Result { { { QStringLiteral(
                       "Could not determine signature of function add: "
                       "Functions without type annotations won't be compiled") } } }
            << true;
    QTest::newRow("generalizedGroupHint")
            << QStringLiteral("generalizedGroupHint.qml")
            << Result { { { QStringLiteral(
                       "Could not determine signature of binding for myColor: "
                       "Could not find property \"myColor\". "
                       "You may want use ID-based grouped properties here.") } } }
            << true;
    QTest::newRow("invalidIdLookup")
            << QStringLiteral("invalidIdLookup.qml")
            << Result { { {
                    QStringLiteral("Could not compile binding for objectName: "
                                   "Cannot retrieve a non-object type by ID: stateMachine")
               } } }
            << true;
    QTest::newRow("returnTypeAnnotation-component")
            << QStringLiteral("returnTypeAnnotation_component.qml")
            << Result{ { { "Could not compile function comp: function without return type "
                           "annotation returns (component in" },
                         { "returnTypeAnnotation_component.qml)::c with type Comp. "
                           "This may prevent proper compilation to Cpp." } } }
            << true;
    QTest::newRow("returnTypeAnnotation-enum")
            << QStringLiteral("returnTypeAnnotation_enum.qml")
            << Result{ { { "Could not compile function enumeration: function without return type "
                           "annotation returns QQuickText::HAlignment::AlignRight. "
                           "This may prevent proper compilation to Cpp." } } }
            << true;
    QTest::newRow("returnTypeAnnotation-method")
            << QStringLiteral("returnTypeAnnotation_method.qml")
            << Result{ { { "Could not compile function method: function without return type "
                           "annotation returns (component in " }, // Don't check the build folder path
                         { "returnTypeAnnotation_method.qml)::f(...). This may "
                           "prevent proper compilation to Cpp." } } }
            << true;
    QTest::newRow("returnTypeAnnotation-property")
            << QStringLiteral("returnTypeAnnotation_property.qml")
            << Result{ { { "Could not compile function prop: function without return type "
                           "annotation returns (component in " }, // Don't check the build folder path
                         { "returnTypeAnnotation_property.qml)::i with type int. This may prevent "
                           "proper compilation to Cpp." } } }
            << true;
    QTest::newRow("returnTypeAnnotation-type")
            << QStringLiteral("returnTypeAnnotation_type.qml")
            << Result{ { { "Could not compile function type: function without return type "
                           "annotation returns double. This may prevent proper compilation to "
                           "Cpp." } } }
            << true;

    QTest::newRow("functionAssign1")
            << QStringLiteral("functionAssign1.qml") << Result::clean() << true;
    QTest::newRow("functionAssign2")
            << QStringLiteral("functionAssign2.qml") << Result::clean() << true;

    // We want to see the warning about the missing property only once.
    QTest::newRow("unresolvedType2")
            << QStringLiteral("unresolvedType2.qml")
            << Result { { Message { QStringLiteral(
                           "Could not determine signature of binding for text: "
                           "Could not find property \"text\".") } },
                        { Message { QStringLiteral(
                           "Cannot resolve property type  for binding on text.") }, },
                        } << true;
}

void TestQmllint::compilerWarnings()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);
    QFETCH(bool, enableCompilerWarnings);

    QJsonArray warnings;

    auto categories = QQmlJSLogger::defaultCategories();

    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlCompiler;
    });
    Q_ASSERT(category != categories.end());

    if (enableCompilerWarnings) {
        category->setLevel(QtWarningMsg);
        category->setIgnored(false);
    }

    runTest(filename, result, {}, {}, {}, UseDefaultImports, &categories);
}

QString TestQmllint::runQmllint(const QString &fileToLint,
                                std::function<void(QProcess &)> handleResult,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath, const Environment &env)
{
    auto qmlImportDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QStringList args;

    QString absoluteFilePath =
            QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint);

    args << QFileInfo(absoluteFilePath).fileName();

    if (addImportDirs) {
        args << QStringLiteral("-I") << qmlImportDir
             << QStringLiteral("-I") << dataDirectory();
    }

    if (ignoreSettings)
        args << QStringLiteral("--ignore-settings");

    if (absolutePath)
        args << QStringLiteral("--absolute-path");

    args << extraArgs;
    args << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
        QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
        for (const auto &entry : env)
            processEnv.insert(entry.first, entry.second);

        process.setProcessEnvironment(processEnv);
        process.setWorkingDirectory(QFileInfo(absoluteFilePath).absolutePath());
        process.start(m_qmllintPath, args);
        handleResult(process);
        errors = process.readAllStandardError();

        QStringList lines = errors.split(u'\n', Qt::SkipEmptyParts);

        auto end = std::remove_if(lines.begin(), lines.end(), [](const QString &line) {
            return !line.startsWith("Warning: ") && !line.startsWith("Error: ");
        });

        std::sort(lines.begin(), end);
        auto it = std::unique(lines.begin(), end);
        if (it != end) {
            qDebug() << "The warnings and errors were generated more than once:";
            do {
                qDebug() << *it;
            } while (++it != end);
            QTest::qFail("Duplicate warnings and errors", __FILE__, __LINE__);
        }

        if (isSilent) {
            QTest::qVerify(errors.isEmpty(), "errors.isEmpty()", "Silent mode outputs messages",
                           __FILE__, __LINE__);
        }

        if (QTest::currentTestFailed()) {
            qDebug().noquote() << "Command:" << process.program() << args.join(u' ');
            qDebug() << "Exit status:" << process.exitStatus();
            qDebug() << "Exit code:" << process.exitCode();
            qDebug() << "stderr:" << errors;
            qDebug() << "stdout:" << process.readAllStandardOutput();
        }
    };
    verify(true);
    args.removeLast();
    verify(false);
    return errors;
}

QString TestQmllint::runQmllint(const QString &fileToLint, bool shouldSucceed,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath, const Environment &env)
{
    return runQmllint(
            fileToLint,
            [&](QProcess &process) {
                QVERIFY(process.waitForFinished());
                QCOMPARE(process.exitStatus(), QProcess::NormalExit);

                if (shouldSucceed)
                    QCOMPARE(process.exitCode(), 0);
                else
                    QVERIFY(process.exitCode() != 0);
            },
            extraArgs, ignoreSettings, addImportDirs, absolutePath, env);
}

QJsonArray TestQmllint::callQmllintImpl(const QString &fileToLint, const QString &content,
                                        const CallQmllintOptions &options, CallQmllintChecks checks)
{
    QJsonArray jsonOutput;
    QJsonArray result;

    const QFileInfo info = QFileInfo(fileToLint);
    const QString lintedFile = info.isAbsolute() ? fileToLint : testFile(fileToLint);

    QQmlJSLinter::LintResult lintResult;

    const QStringList resolvedImportPaths = options.defaultImports == UseDefaultImports
            ? m_defaultImportPaths + options.importPaths
            : options.importPaths;
    if (options.type == LintFile) {
        QList<QQmlJS::LoggerCategory> resolvedCategories =
                options.categories != nullptr ? *options.categories : m_categories;

        for (const QString &name : options.enableCategories) {
            for (QQmlJS::LoggerCategory &category : resolvedCategories) {
                if (category.name() != name)
                    continue;

                [&category]() {
                    QVERIFY2(category.isIgnored(), "Can't enable already enabled category!");
                }();
                category.setIgnored(false);
                break;
            }
        }

        if (options.readSettings) {
            QQmlToolingSettings settings(QLatin1String("qmllint"));
            if (settings.search(lintedFile))
                QQmlJS::LoggingUtils::updateLogLevels(resolvedCategories, settings, nullptr);
        }

        lintResult = m_linter.lintFile(lintedFile, content.isEmpty() ? nullptr : &content, true,
                                       &jsonOutput, resolvedImportPaths, options.qmldirFiles,
                                       options.resources, resolvedCategories);
    } else {
        lintResult = m_linter.lintModule(fileToLint, true, &jsonOutput, resolvedImportPaths,
                                         options.resources);
    }

    [&]() {
        const bool success = lintResult == QQmlJSLinter::LintSuccess;
        const QByteArray errorOutput = QJsonDocument(jsonOutput).toJson();
        QVERIFY2(success == (checks.testFlag(ShouldSucceed)), errorOutput);
        QVERIFY2(jsonOutput.size() == 1, errorOutput);
        result = jsonOutput.at(0)[u"warnings"_s].toArray();
    }();

    if (lintResult == QQmlJSLinter::LintSuccess || lintResult == QQmlJSLinter::HasWarnings) {
        testFixes(checks.testFlag(ShouldSucceed), options.importPaths, options.qmldirFiles,
                  options.resources, options.defaultImports, options.categories,
                  checks.testFlag(HasAutoFix), options.readSettings,
                  info.baseName() + u".fixed.qml"_s);
    }
    return result;
}

QJsonArray TestQmllint::callQmllint(const QString &fileToLint, const CallQmllintOptions &options,
                                    CallQmllintChecks checks)
{
    return callQmllintImpl(fileToLint, QString(), options, checks);
}

QJsonArray TestQmllint::callQmllintOnSnippet(const QString &snippet,
                                             const CallQmllintOptions &options,
                                             CallQmllintChecks checks)
{
    return callQmllintImpl(QString(), snippet, options, checks);
}

void TestQmllint::testFixes(bool shouldSucceed, QStringList importPaths, QStringList qmldirFiles,
                            QStringList resources, DefaultImportOption defaultImports,
                            QList<QQmlJS::LoggerCategory> *categories, bool autoFixable,
                            bool readSettings, const QString &fixedPath)
{
    QString fixedCode;
    QQmlJSLinter::FixResult fixResult = m_linter.applyFixes(&fixedCode, true);

    if (autoFixable) {
        QCOMPARE(fixResult, QQmlJSLinter::FixSuccess);
        // Check that the fixed version of the file actually passes qmllint now
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile file(dir.filePath("Fixed.qml"));
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        file.write(fixedCode.toUtf8());
        file.flush();
        file.close();

        CallQmllintOptions options;
        options.importPaths = importPaths;
        options.qmldirFiles = qmldirFiles;
        options.resources = resources;
        options.defaultImports = defaultImports;
        options.categories = categories;
        options.readSettings = readSettings;

        callQmllint(QFileInfo(file).absoluteFilePath(), options);

        if (QFileInfo(fixedPath).exists()) {
            QFile fixedFile(fixedPath);
            QVERIFY(fixedFile.open(QFile::ReadOnly));
            QString fixedFileContents = QString::fromUtf8(fixedFile.readAll());
#ifdef Q_OS_WIN
            fixedCode = fixedCode.replace(u"\r\n"_s, u"\n"_s);
            fixedFileContents = fixedFileContents.replace(u"\r\n"_s, u"\n"_s);
#endif

            QCOMPARE(fixedCode, fixedFileContents);
        }
    } else {
        if (shouldSucceed)
            QCOMPARE(fixResult, QQmlJSLinter::NothingToFix);
        else
            QVERIFY(fixResult == QQmlJSLinter::FixSuccess
                    || fixResult == QQmlJSLinter::NothingToFix);
    }
}

void TestQmllint::runTest(const QString &testFile, const Result &result, QStringList importDirs,
                          QStringList qmltypesFiles, QStringList resources,
                          DefaultImportOption defaultImports,
                          QList<QQmlJS::LoggerCategory> *categories)
{
    CallQmllintOptions options;
    options.importPaths = importDirs;
    options.qmldirFiles = qmltypesFiles;
    options.resources = resources;
    options.defaultImports = defaultImports;
    options.categories = categories;
    options.readSettings = result.flags.testFlag(Result::Flag::UseSettings);

    const QJsonArray warnings = callQmllint(testFile, options, fromResultFlags(result.flags));
    checkResult(warnings, result);
}

static QtMsgType typeStringToMsgType(const QString &type)
{
    if (type == u"debug")
        return QtDebugMsg;
    if (type == u"info")
        return QtInfoMsg;
    if (type == u"warning")
        return QtWarningMsg;
    if (type == u"critical")
        return QtCriticalMsg;
    if (type == u"fatal")
        return QtFatalMsg;

    Q_UNREACHABLE();
}

struct SimplifiedWarning
{
    QString message;
    quint32 line;
    quint32 column;
    QtMsgType type;

    SimplifiedWarning(QJsonValueConstRef warning)
        : message(warning[u"message"].toString()),
          line(warning[u"line"].toInt()),
          column(warning[u"column"].toInt()),
          type(typeStringToMsgType(warning[u"type"].toString()))
    {
    }

    QString toString() const { return u"%1:%2: %3"_s.arg(line).arg(column).arg(message); }

    std::tuple<QString, quint32, quint32, QtMsgType> asTuple() const
    {
        return std::make_tuple(message, line, column, type);
    }

    friend bool comparesEqual(const SimplifiedWarning& a, const SimplifiedWarning& b) noexcept
    {
        return a.asTuple() == b.asTuple();
    }
    friend Qt::strong_ordering compareThreeWay(const SimplifiedWarning& a, const SimplifiedWarning& b) noexcept {
        return QtOrderingPrivate::compareThreeWayMulti(a.asTuple(), b.asTuple());
    }
    Q_DECLARE_STRONGLY_ORDERED(SimplifiedWarning)
};

template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler,
         typename ReplacementFailureHandler>
void TestQmllint::checkResult(const QJsonArray &warnings, const Result &result,
                              ExpectedMessageFailureHandler onExpectedMessageFailures,
                              BadMessageFailureHandler onBadMessageFailures,
                              ReplacementFailureHandler onReplacementFailures)
{
    if (result.flags.testFlag(Result::Flag::NoMessages))
        QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));

    for (const Message &msg : result.expectedMessages) {
        // output.contains() expect fails:
        onExpectedMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column);
    }

    for (const Message &msg : result.badMessages) {
        // !output.contains() expect fails:
        onBadMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column, StringNotContained);
    }

    for (const Message &replacement : result.expectedReplacements) {
        onReplacementFailures();
        searchWarnings(warnings, replacement.text, replacement.severity, replacement.line,
                       replacement.column, StringContained, DoReplacementSearch);
    }

    // check for duplicates
    QList<SimplifiedWarning> sortedWarnings;
    std::transform(warnings.begin(), warnings.end(), std::back_inserter(sortedWarnings),
                   [](QJsonValueConstRef ref) { return SimplifiedWarning(ref); });
    std::sort(sortedWarnings.begin(), sortedWarnings.end());
    const auto firstDuplicate =
            std::adjacent_find(sortedWarnings.constBegin(), sortedWarnings.constEnd());
    for (auto it = firstDuplicate; it != sortedWarnings.constEnd();
         it = std::adjacent_find(it + 1, sortedWarnings.constEnd())) {
        qDebug() << "Found duplicate warning: " << it->toString();
    }
    QVERIFY2(firstDuplicate == sortedWarnings.constEnd(), "Found duplicate warnings!");
}

void TestQmllint::searchWarnings(const QJsonArray &warnings, const QString &substring,
                                 QtMsgType type, quint32 line, quint32 column,
                                 ContainOption shouldContain, ReplacementOption searchReplacements)
{
    bool contains = false;

    for (const QJsonValueConstRef warningJson : warnings) {
        SimplifiedWarning warning(warningJson);

        if (warning.message.contains(substring)) {
            if (warning.type != type) {
                continue;
            }
            if (line != 0 || column != 0) {
                if (warning.line != line || warning.column != column) {
                    continue;
                }
            }

            contains = true;
            break;
        }

        for (const QJsonValueConstRef fix : warningJson[u"suggestions"].toArray()) {
            const QString fixMessage = fix[u"message"].toString();
            if (fixMessage.contains(substring)) {
                contains = true;
                break;
            }

            if (searchReplacements == DoReplacementSearch) {
                QString replacement = fix[u"replacement"].toString();
#ifdef Q_OS_WIN
                // Replacements can contain native line endings
                // but we need them to be uniform in order for them to conform to our test data
                replacement = replacement.replace(u"\r\n"_s, u"\n"_s);
#endif

                if (replacement.contains(substring)) {
                    quint32 fixLine = fix[u"line"].toInt();
                    quint32 fixColumn = fix[u"column"].toInt();
                    if (line != 0 || column != 0) {
                        if (fixLine != line || fixColumn != column) {
                            continue;
                        }
                    }
                    contains = true;
                    break;
                }
            }
        }
    }

    const auto toDescription = [](const QJsonArray &warnings, const QString &substring,
                                  quint32 line, quint32 column, bool must = true) {
        QString msg = QStringLiteral("qmllint output:\n%1\nIt %2 contain '%3'")
                              .arg(QString::fromUtf8(
                                           QJsonDocument(warnings).toJson(QJsonDocument::Indented)),
                                   must ? u"must" : u"must NOT", substring);
        if (line != 0 || column != 0)
            msg += u" (%1:%2)"_s.arg(line).arg(column);

        return msg;
    };

    if (shouldContain == StringContained) {
        if (!contains)
            qWarning().noquote() << toDescription(warnings, substring, line, column);
        QVERIFY(contains);
    } else {
        if (contains)
            qWarning().noquote() << toDescription(warnings, substring, line, column, false);
        QVERIFY(!contains);
    }
}

void TestQmllint::requiredProperty()
{
    runTest("requiredProperty.qml", Result::clean());

    runTest("requiredMissingProperty.qml",
            Result { { Message { QStringLiteral(
                    "Property \"foo\" was marked as required but does not exist.") } } });

    runTest("requiredPropertyBindings.qml", Result::clean());
    runTest("requiredPropertyBindingsNow.qml",
            Result { { Message { QStringLiteral("Component is missing required property "
                                                "required_now_string from Base") },
                       Message { QStringLiteral("Component is missing required property "
                                                "required_defined_here_string from here") } } });
    runTest("requiredPropertyBindingsLater.qml",
            Result { { Message { QStringLiteral("Component is missing required property "
                                                "required_later_string from "
                                                "Base") },
                       Message { QStringLiteral("Property marked as required in Derived") },
                       Message { QStringLiteral("Component is missing required property "
                                                "required_even_later_string "
                                                "from Base (marked as required by here)") } } });
}

void TestQmllint::settingsFile()
{
    QVERIFY(runQmllint("settings/unqualifiedSilent/unqualified.qml", true, warningsShouldFailArgs(), false)
                    .isEmpty());
    QVERIFY(runQmllint("settings/unusedImportWarning/unused.qml", false, warningsShouldFailArgs(), false)
                    .contains(QStringLiteral("Warning: %1:2:1: Unused import")
                                      .arg(testFile("settings/unusedImportWarning/unused.qml"))));
    QVERIFY(runQmllint("settings/bare/bare.qml", false, warningsShouldFailArgs(), false, false)
                    .contains(
                            u"Failed to import QtQuick. Are your import paths set up properly?"_s));
    QVERIFY(runQmllint("settings/qmltypes/qmltypes.qml", false, warningsShouldFailArgs(), false)
                    .contains(QStringLiteral("not a qmldir file. Assuming qmltypes.")));
    QVERIFY(runQmllint("settings/qmlimports/qmlimports.qml", true, warningsShouldFailArgs(), false).isEmpty());
}

void TestQmllint::additionalImplicitImport()
{
    // We're polluting the resource file system here, so let's clean up afterwards.
    const auto guard = qScopeGuard([this]() {m_linter.clearCache(); });
    runTest("additionalImplicitImport.qml", Result::clean(), {}, {},
            { testFile("implicitImportResource.qrc") });
}

void TestQmllint::qrcUrlImport()
{
    const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
    CallQmllintOptions options;
    options.resources.append(testFile("untitled/qrcUrlImport.qrc"));

    const QJsonArray warnings = callQmllint(testFile("untitled/main.qml"), options);
    checkResult(warnings, Result::clean());
}

void TestQmllint::incorrectImportFromHost_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    QTest::newRow("NonexistentFile")
            << QStringLiteral("importNonexistentFile.qml")
            << Result{ { Message{
                       QStringLiteral("File or directory you are trying to import does not exist"),
                       1, 1 } } };
#ifndef Q_OS_WIN
    // there is no /dev/null device on Win
    QTest::newRow("NullDevice")
            << QStringLiteral("importNullDevice.qml")
            << Result{ { Message{ QStringLiteral("is neither a file nor a directory. Are sure the "
                                                 "import path is correct?"),
                                  1, 1 } } };
#endif
}

void TestQmllint::incorrectImportFromHost()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    runTest(filename, result);
}

void TestQmllint::attachedPropertyReuse()
{
    auto categories = QQmlJSLogger::defaultCategories();
    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlAttachedPropertyReuse;
    });
    Q_ASSERT(category != categories.end());

    category->setLevel(QtWarningMsg);
    category->setIgnored(false);
    runTest("attachedPropNotReused.qml",
            Result { { Message { QStringLiteral("Using attached type QQuickKeyNavigationAttached "
                                                "already initialized in a parent "
                                                "scope") } } },
            {}, {}, {}, UseDefaultImports, &categories);

    runTest("attachedPropEnum.qml", Result::clean(), {}, {}, {}, UseDefaultImports, &categories);
    runTest("MyStyle/ToolBar.qml",
            Result{ { Message{
                            "Using attached type MyStyle already initialized in a parent scope"_L1,
                            10, 16 } },
                    {},
                    { Message{ "Reference it by id instead"_L1, 10, 16 } },
                    Result::AutoFixable },
            {}, {}, {}, UseDefaultImports, &categories);
    runTest("pluginQuick_multipleAttachedPropertyReuse.qml",
            Result{ { Message{ QStringLiteral(
                    "Using attached type Test already initialized in a parent scope") } } },
            {}, {}, {}, UseDefaultImports, &categories);
}

void TestQmllint::missingBuiltinsNoCrash()
{
    // We cannot use the normal linter here since the other tests might have cached the builtins
    // alread
    QQmlJSLinter linter(m_defaultImportPaths);

    QJsonArray jsonOutput;
    QJsonArray warnings;

    bool success = linter.lintFile(testFile("missingBuiltinsNoCrash.qml"), nullptr, true,
                                   &jsonOutput, {}, {}, {}, {})
            == QQmlJSLinter::LintSuccess;
    QVERIFY2(!success, QJsonDocument(jsonOutput).toJson());

    QVERIFY2(jsonOutput.size() == 1, QJsonDocument(jsonOutput).toJson());
    warnings = jsonOutput.at(0)[u"warnings"_s].toArray();

    checkResult(
            warnings,
            Result{ { Message{
                    u"Failed to import QtQuick. Are your import paths set up properly?"_s } } });
}

void TestQmllint::absolutePath()
{
    QString absPathOutput = runQmllint("memberNotFound.qml", false, warningsShouldFailArgs(), true, true, true);
    QString relPathOutput = runQmllint("memberNotFound.qml", false, warningsShouldFailArgs(), true, true, false);
    const QString absolutePath = QFileInfo(testFile("memberNotFound.qml")).absoluteFilePath();

    QVERIFY(absPathOutput.contains(absolutePath));
    QVERIFY(!relPathOutput.contains(absolutePath));
}

void TestQmllint::importMultipartUri()
{
    runTest("here.qml", Result::clean(), {}, { testFile("Elsewhere/qmldir") });
}

void TestQmllint::lintModule_data()
{
    QTest::addColumn<QString>("module");
    QTest::addColumn<QStringList>("importPaths");
    QTest::addColumn<QStringList>("resources");
    QTest::addColumn<Result>("result");

    QTest::addRow("Things")
            << u"Things"_s
            << QStringList()
            << QStringList()
            << Result {
                   { Message {
                             u"Type \"QPalette\" not found. Used in SomethingEntirelyStrange.palette"_s,
                     },
                     Message {
                             u"Type \"CustomPalette\" is not fully resolved. Used in SomethingEntirelyStrange.palette2"_s } }
               };
    QTest::addRow("missingQmltypes")
            << u"Fake5Compat.GraphicalEffects.private"_s
            << QStringList()
            << QStringList()
            << Result { { Message { u"QML types file does not exist"_s } } };

    QTest::addRow("moduleWithQrc")
            << u"moduleWithQrc"_s
            << QStringList({ testFile("hidden") })
            << QStringList({
                               testFile("hidden/qmake_moduleWithQrc.qrc"),
                               testFile("hidden/moduleWithQrc_raw_qml_0.qrc")
                           })
            << Result::clean();
}

void TestQmllint::lintModule()
{
    QFETCH(QString, module);
    QFETCH(QStringList, importPaths);
    QFETCH(QStringList, resources);
    QFETCH(Result, result);

    CallQmllintOptions options;
    options.importPaths = importPaths;
    options.resources = resources;
    options.type = LintModule;

    const QJsonArray warnings = callQmllint(module, options, fromResultFlags(result.flags));
    checkResult(warnings, result);
}

void TestQmllint::testLineEndings()
{
    {
        const auto textWithLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
            "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\n    property string deprecated\n\n    "
            "property string a: root.a // qmllint disable unqualifi77777777777777777777777777777777777777777777777777777"
            "777777777777777777777777777777777777ed\n    property string b: root.a // qmllint di000000000000000000000000"
            "000000000000000000inyyyyyyyyg c: root.a\n    property string d: root.a\n    // qmllint enable unqualified\n\n    "
            "//qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: {\n        console.log"
            "(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile( {}, &textWithLF, true, nullptr, {}, {}, {}, {});

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
    {
        const auto textWithCRLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
        "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r"
        "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\n    property string deprecated\n\n    property string a: root.a "
        "// qmllint disable unqualifi77777777777777777777777777777777777777777777777777777777777777777777777777777777777777777ed\n    "
        "property string b: root.a // qmllint di000000000000000000000000000000000000000000inyyyyyyyyg c: root.a\n    property string d: "
        "root.a\n    // qmllint enable unqualified\n\n    //qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: "
        "{\n        console.log(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile( {}, &textWithCRLF, true, nullptr, {}, {}, {}, {});

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
}

void TestQmllint::valueTypesFromString()
{
    runTest("valueTypesFromString.qml",
            Result{ {
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QPointF\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QSizeF\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QRectF\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QVector2D\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QVector3D\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QVector4D\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QQuaternion\""_s },
                            Message{
                                    u"Construction from string is deprecated. Use structured value type construction instead for type \"QMatrix4x4\""_s },
                    },
                    { /*bad messages */ },
                    {
                            Message{ u"({ width: 30, height: 50 })"_s },
                            Message{ u"({ x: 10, y: 20, width: 30, height: 50 })"_s },
                            Message{ u"({ x: 30, y: 50 })"_s },
                            Message{ u"({ x: 1, y: 2 })"_s },
                            Message{ u"({ x: 1, y: 2 })"_s },
                            Message{ u"({ x: 1, y: 2, z: 3 })"_s },
                            Message{ u"({ x: 1, y: 2, z: 3, w: 4 })"_s },
                            Message{ u"({ scalar: 1, x: 2, y: 3, z: 4 })"_s },
                            Message{
                                    u"({ m11: 1, m12: 2, m13: 3, m14: 4, m21: 5, m22: 6, m23: 7, m24: 8, m31: 9, m32: 10, m33: 11, m34: 12, m41: 13, m42: 14, m43: 15, m44: 16 })"_s },
                    } });
}

#if QT_CONFIG(library)
void TestQmllint::hasTestPlugin()
{
    bool pluginFound = false;
    for (const QQmlJSLinter::Plugin &plugin : m_linter.plugins()) {
        if (plugin.name() != "testPlugin")
            continue;

        pluginFound = true;
        QCOMPARE(plugin.author(), u"Qt"_s);
        QCOMPARE(plugin.description(), u"A test plugin for tst_qmllint"_s);
        QCOMPARE(plugin.version(), u"1.0"_s);

        for (auto &category : plugin.categories()) {
            if (category.name() == u"testPlugin.TestDefaultValue") {
                QCOMPARE(category.level(), QtCriticalMsg);
                QCOMPARE(category.isIgnored(), true);
            } else if (category.name() == u"testPlugin.TestDefaultValue2") {
                QCOMPARE(category.level(), QtInfoMsg);
                QCOMPARE(category.isIgnored(), false);
            } else if (category.name() == u"testPlugin.test") {
                QCOMPARE(category.level(), QtWarningMsg);
                QCOMPARE(category.isIgnored(), false);
            } else if (category.name() == u"testPlugin.TestDefaultValue3"){
                QCOMPARE(category.level(), QtWarningMsg);
                QCOMPARE(category.isIgnored(), false);
            } else if (category.name() == u"testPlugin.TestDefaultValue4"){
                QCOMPARE(category.level(), QtCriticalMsg);
                QCOMPARE(category.isIgnored(), false);
            } else {
                QFAIL("This category was not tested!");
            }
        }
    }
    QVERIFY(pluginFound);
}
void TestQmllint::testPlugin_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Result>("expectedErrors");

    QTest::addRow("elementpass_pluginTest")
            << testFile(u"testPluginData/elementpass_pluginTest.qml"_s)
            << Result{ { Message{ u"ElementTest OK"_s, 4, 5 } } };
    QTest::addRow("propertypass_pluginTest_read")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << Result{
                   {
                       // Property on any type
                       Message{ u"Saw read on Text property x in scope Text"_s, 8, 12 },
                       Message{ u"Saw read on Text property x in scope Item"_s, 21, 25 },
                       // JavaScript
                       Message{ u"Saw read on ObjectPrototype property log in scope Item"_s, 21, 17 },
                       Message{ u"Saw read on ObjectPrototype property log in scope Item"_s, 22, 14 },
                   },
               };
    QTest::addRow("propertypass_pluginTest_write")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << Result{
                   {
                       Message{ u"Saw write on Text property x with value int in scope Item"_s, 23, 9 },
                   },
               };
    QTest::addRow("propertypass_pluginTest_binding")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << Result{
                   {
                     // Specific binding for specific property
                     Message{ u"Saw binding on Text property text with value NULL (and type 3) in scope Text"_s, 6, 15 },
                     Message{ u"Saw binding on Text property x with value NULL (and type 2) in scope Text"_s, 7, 12 },
                     Message{ u"Saw binding on Item property x with value NULL (and type 2) in scope Item"_s, 11, 8 },
                     Message{ u"Saw binding on ListView property model with value ListModel (and type 8) in scope ListView"_s, 16, 16 },
                     Message{ u"Saw binding on ListView property height with value NULL (and type 2) in scope ListView"_s, 17, 17 }
                   },
               };

    QTest::addRow("sourceLocations")
            << testFile(u"testPluginData/sourceLocations_pluginTest.qml"_s)
            << Result{
                   {
                     // Specific binding for specific property
                     Message{ u"Saw binding on Item property x with value QString (and type 8) in scope Item"_s, 5, 12 },
                     Message{ u"Saw binding on Item property EnterKey.type with value Qt::EnterKeyType (and type 8) in scope QQuickEnterKeyAttached"_s, 8, 24 },
                     Message{ u"Saw binding on Item property x with value QJSPrimitiveValue (and type 8) in scope Item"_s, 11, 12 },
                     Message{ u"Saw binding on Item property x with value NULL (and type 2) in scope Item"_s, 14, 12 },
                     Message{ u"Saw binding on Item property onXChanged with value function (and type 8) in scope Item"_s, 18, 21 },
                     Message{ u"Saw read on ObjectPrototype property log in scope Item"_s, 21, 36 },
                     Message{ u"Saw binding on Item property onXChanged with value QVariant (and type 8) in scope Item"_s, 22, 21 },
                     Message{ u"Saw write on Item property x with value double in scope Item"_s, 30, 17 },
                     Message{ u"Saw write on Item property x with value int in scope Item"_s, 35, 31 },
                     Message{ u"Saw read on Item property x in scope Item"_s, 35, 46 },
                   },
               };
    QTest::addRow("propertypass_pluginTest_call")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << Result{
                   {
                        Message{ u"Saw call on ObjectPrototype property log in scope Item"_s, 21, 17 },
                        Message{ u"Saw call on ObjectPrototype property log in scope Item"_s, 22, 14 },
                        Message{ u"Saw call on ObjectPrototype property abs in scope Item"_s, 26, 22 },
                        Message{ u"Saw call on Item property abs in scope Item"_s, 32, 16 },
                        Message{ u"Saw call on  property now in scope Item"_s, 39, 22 }, // happening for Date.now()
                   },
               };
    QTest::addRow("propertypass_pluginTest_translations")
            << testFile(u"testPluginData/translations_pluginTest.qml"_s)
            << Result{
                   {
                        // translations
                        Message{ u"Saw call on ObjectPrototype property qsTr in scope Item"_s, 4, 34 },

                        // should actually be qsTranslate, but better qsTr than nothing!
                        // see also test "propertypass_pluginTest_qsTranslateEdgeCase" below
                        Message{ u"Saw call on ObjectPrototype property qsTr in scope Item"_s, 5, 35 },

                        Message{ u"Saw call on ObjectPrototype property qsTrId in scope Item"_s, 6, 36 },
                        Message{ u"Saw call on ObjectPrototype property qsTr in scope Item"_s, 7, 46 },
                        Message{ u"Saw call on ObjectPrototype property qsTranslate in scope Item"_s, 8, 47 },
                        Message{ u"Saw call on ObjectPrototype property qsTrId in scope Item"_s, 9, 48 },
                   },
               };

    QTest::addRow("controlsWithQuick_pluginTest")
            << testFile(u"testPluginData/controlsWithQuick_pluginTest.qml"_s)
            << Result{ { Message{ u"QtQuick.Controls, QtQuick and QtQuick.Window present"_s } } };
    QTest::addRow("controlsWithoutQuick_pluginTest")
            << testFile(u"testPluginData/controlsWithoutQuick_pluginTest.qml"_s)
            << Result{ { Message{ u"QtQuick.Controls and NO QtQuick present"_s } } };

    // Verify that none of the passes do anything when they're not supposed to
    QTest::addRow("nothing_pluginTest")
            << testFile(u"testPluginData/nothing_pluginTest.qml"_s) << Result::clean();
    QTest::addRow("settings_pluginTest")
            << testFile(u"settings/plugin/elementpass_pluginTest.qml"_s) << Result::cleanWithSettings();
    QTest::addRow("old_settings_pluginTest")
            << testFile(u"settings/pluginOld/elementpass_pluginTest.qml"_s) << Result::cleanWithSettings();
    QTest::addRow("nosettings_pluginTest")
            << testFile(u"settings/plugin/elementpass_pluginTest.qml"_s)
            << Result{ { Message{ u"ElementTest OK"_s } }, {}, {} };
}

void TestQmllint::testPlugin()
{
    QFETCH(QString, fileName);
    QFETCH(Result, expectedErrors);

    runTest(fileName, expectedErrors);
}

void TestQmllint::testPluginHelpCommandLine()
{
    auto qmllintOutput = [this](const QString& filename, const QStringList& args) {
        QString output;
        QString errorOutput;
        runQmllint(
                testFile(filename),
                [&](QProcess &process) {
                    QVERIFY(process.waitForFinished());
                    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
                    QCOMPARE(process.exitCode(), 0);
                    output = process.readAllStandardOutput();
                    errorOutput = process.readAllStandardError();
                },
                args);
        return std::pair<QString, QString>{ output, errorOutput };
    };
    {
        // make sure plugin warnings are documented by --help
        const auto [helpText, error] = qmllintOutput(u"testPluginData/nothing_pluginTest.qml"_s,
                                                     QStringList{ u"--help"_s });
        QVERIFY(helpText.contains(u"--Quick.property-changes-parsed"_s));
    }
}

void TestQmllint::testPluginCommandLine()
{
    // make sure plugin warnings are accepted as options
    const QString warnings =
            runQmllint(testFile(u"testPluginData/nothing_pluginTest.qml"_s), true,
                       QStringList{ u"--Quick.property-changes-parsed"_s, u"disable"_s });
    // should not contain a warning about --Quick.property-changes-parsed being an unknown option
    // and no warnings
    QVERIFY(warnings.isEmpty());
}

// TODO: Eventually tests for (real) plugins need to be moved into a separate file
void TestQmllint::quickPlugin()
{
    const auto &plugins = m_linter.plugins();

    const bool pluginFound =
            std::find_if(plugins.cbegin(), plugins.cend(),
                         [](const auto &plugin) { return plugin.name() == "Quick"; })
            != plugins.cend();
    QVERIFY(pluginFound);

    runTest("pluginQuick_anchors.qml",
            Result{ { Message{
                              u"Cannot specify left, right, and horizontalCenter anchors at the same time."_s },
                      Message {
                              u"Cannot specify top, bottom, and verticalCenter anchors at the same time."_s },
                      Message{
                              u"Baseline anchor cannot be used in conjunction with top, bottom, or verticalCenter anchors."_s },
                      Message { u"Cannot assign literal of type null to QQuickAnchorLine"_s, 5,
                                35 },
                      Message { u"Cannot assign literal of type null to QQuickAnchorLine"_s, 6,
                                33 } } });
    runTest("pluginQuick_anchorsUndefined.qml", Result::clean());
    runTest("pluginQuick_layoutChildren.qml",
            Result {
                    { Message {
                              u"Detected anchors on an item that is managed by a layout. This is undefined behavior; use Layout.alignment instead."_s },
                      Message {
                              u"Detected x on an item that is managed by a layout. This is undefined behavior; use Layout.leftMargin or Layout.rightMargin instead."_s },
                      Message {
                              u"Detected y on an item that is managed by a layout. This is undefined behavior; use Layout.topMargin or Layout.bottomMargin instead."_s },
                      Message {
                              u"Detected height on an item that is managed by a layout. This is undefined behavior; use implictHeight or Layout.preferredHeight instead."_s },
                      Message {
                              u"Detected width on an item that is managed by a layout. This is undefined behavior; use implicitWidth or Layout.preferredWidth instead."_s },
                      Message {
                              u"Cannot specify anchors for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify x for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify y for items inside Grid. Grid will not function."_s },
                      Message {
                              u"Cannot specify anchors for items inside Flow. Flow will not function."_s },
                      Message {
                              u"Cannot specify x for items inside Flow. Flow will not function."_s },
                      Message {
                              u"Cannot specify y for items inside Flow. Flow will not function."_s } } });
    runTest("pluginQuick_attached.qml",
            Result {
                    { Message { u"ToolTip attached property must be attached to an object deriving from Item"_s },
                      Message { u"SplitView attached property must be attached to an object deriving from Item"_s },
                      Message { u"ScrollIndicator attached property must be attached to an object deriving from Flickable"_s },
                      Message { u"ScrollBar attached property must be attached to an object deriving from Flickable or ScrollView"_s },
                      Message { u"Accessible attached property must be attached to an object deriving from Item or Action"_s },
                      Message { u"EnterKey attached property must be attached to an object deriving from Item"_s },
                      Message {
                              u"LayoutMirroring attached property must be attached to an object deriving from Item or Window"_s },
                      Message { u"Layout attached property must be attached to an object deriving from Item"_s },
                      Message { u"StackView attached property must be attached to an object deriving from Item"_s },
                      Message { u"TextArea attached property must be attached to an object deriving from Flickable"_s },
                      Message { u"StackLayout attached property must be attached to an object deriving from Item"_s },
                      Message { u"SwipeDelegate attached property must be attached to an object deriving from Item"_s },
                      Message { u"SwipeView attached property must be attached to an object deriving from Item"_s } } });

    {
        const Result result{ {}, { Message{ u"Tumbler"_s }, }, };
        runTest("pluginQuick_tumblerGood.qml", result);
    }

    runTest("pluginQuick_swipeDelegate.qml",
            Result { {
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_s,
                             6, 43 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_s,
                             7, 43 },
                         Message { u"SwipeDelegate: Cannot set both behind and left/right properties"_s,
                                   9, 9 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_s,
                             13, 47 },
                         Message {
                             u"SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_s,
                             14, 42 },
                         Message { u"SwipeDelegate: Cannot set both behind and left/right properties"_s,
                                   16, 9 },
                     } });

    runTest("pluginQuick_varProp.qml",
            Result {
                    { Message {
                              u"Unexpected type for property \"contentItem\" expected QQuickPathView, QQuickListView got QQuickItem"_s },
                      Message {
                              u"Unexpected type for property \"columnWidthProvider\" expected function got null"_s },
                      Message {
                              u"Unexpected type for property \"textFromValue\" expected function got null"_s },
                      Message {
                              u"Unexpected type for property \"valueFromText\" expected function got int"_s },
                      Message {
                              u"Unexpected type for property \"rowHeightProvider\" expected function got int"_s } } });
    runTest("pluginQuick_varPropClean.qml", Result::clean());
    runTest("pluginQuick_attachedClean.qml", Result::clean());
    runTest("pluginQuick_attachedIgnore.qml", Result::clean());
    runTest("pluginQuick_noCrashOnUneresolved.qml", Result {}); // we don't care about the specific warnings

    runTest("pluginQuick_propertyChangesParsed.qml",
            Result { {
                Message {
                      u"Property \"myColor\" is custom-parsed in PropertyChanges. "
                       "You should phrase this binding as \"foo.myColor: Qt.rgba(0.5, ...\""_s,
                      12, 30
                },
                Message {
                      u"You should remove any bindings on the \"target\" property and avoid "
                       "custom-parsed bindings in PropertyChanges."_s,
                      11, 29
                },
                Message {
                      u"Unknown property \"notThere\" in PropertyChanges."_s,
                      13, 31
                }
            } });
    runTest("pluginQuick_propertyChangesInvalidTarget.qml", Result {}); // we don't care about the specific warnings
}

void TestQmllint::hasQdsPlugin()
{
    const auto &plugins = m_linter.plugins();

    const bool pluginFound =
            std::find_if(plugins.cbegin(), plugins.cend(),
                         [](const auto &plugin) { return plugin.name() == "QtDesignStudio"; })
            != plugins.cend();
    QVERIFY(pluginFound);
}

void TestQmllint::qdsPlugin_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Result>("expectedResult");

    QTest::addRow("WhiteListedFunctions")
            << u"qdsPlugin/WhiteListedFunctions.ui.qml"_s << Result::clean();
    QTest::addRow("WhiteListedFunctionsDate")
            << u"qdsPlugin/WhiteListedFunctionsDate.ui.qml"_s << Result::clean();
    {
        const QString warning =
                u"Arbitrary functions and function calls outside of a Connections object are not "
                u"supported in a UI file (.ui.qml)"_s;

        QTest::addRow("BlackListedFunctions") << u"qdsPlugin/BlackListedFunctions.ui.qml"_s
                                              << Result{ {
                                                         Message{ warning, 7, 9 },
                                                         Message{ warning, 8, 14 },
                                                         Message{ warning, 12, 38 },
                                                         Message{ warning, 13, 35 },
                                                 } };
    }

    QTest::addRow("UnsupportedBindings")
            << u"qdsPlugin/UnsupportedBindings.ui.qml"_s
            << Result{ {
                       Message{
                               u"Referencing the parent of the root item is not supported in a UI file (.ui.qml)"_s,
                               4, 25 },
                       Message{
                               "Imperative JavaScript assignments can break the visual tooling in Qt Design Studio."_L1,
                               7, 24 },
                       Message{
                               "Imperative JavaScript assignments can break the visual tooling in Qt Design Studio."_L1,
                               8, 24 },
               } };

    QTest::addRow("SupportedBindings")
            << u"qdsPlugin/SupportedBindings.ui.qml"_s
            << Result::clean();

    QTest::addRow("UnsupportedElements")
            << u"qdsPlugin/UnsupportedElements.ui.qml"_s
            << Result{
                   {
                       Message{ "This type (ApplicationWindow) is not supported in a UI file (.ui.qml)"_L1, 4, 1 },
                       Message{ "This type (ShaderEffect) is not supported in a UI file (.ui.qml)"_L1, 7, 27 },
                       Message{ "This type (Drawer) is not supported in a UI file (.ui.qml)"_L1, 6, 9 },
                       Message{ "This id (bool) might be ambiguous and is not supported in a UI file (.ui.qml)"_L1, 11, 13 },
                   },
                   {
                       Message{ "This type (Item) is not supported in a UI file (.ui.qml)"_L1 },

                   }
               };
    QTest::addRow("SupportedElements")
            << u"qdsPlugin/SupportedElements.ui.qml"_s
            << Result::clean();

    QTest::addRow("UnsupportedRootElement")
            << u"qdsPlugin/UnsupportedRootElement.ui.qml"_s
            << Result{ {
                       Message{
                               u"This type (QtObject) is not supported as a root element of a UI file (.ui.qml)."_s,
                               3, 1 },
               } };

    QTest::addRow("UnsupportedRootElement2")
            << u"qdsPlugin/UnsupportedRootElement2.ui.qml"_s
            << Result{ {
                       Message{
                               u"This type (ListModel) is not supported as a root element of a UI file (.ui.qml)."_s,
                               4, 1 },
               } };
}

void TestQmllint::qdsPlugin()
{
    QFETCH(QString, fileName);
    QFETCH(Result, expectedResult);

    runTest(fileName, expectedResult);
}

void TestQmllint::environment_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("shouldSucceed");
    QTest::addColumn<QStringList>("extraArgs");
    QTest::addColumn<Environment>("env");
    QTest::addColumn<QString>("expectedWarning");

    const QString fileThatNeedsImportPath = testFile(u"NeedImportPath.qml"_s);
    const QString importPath = testFile(u"ImportPath"_s);
    const QString invalidImportPath = testFile(u"ImportPathThatDoesNotExist"_s);
    const QString noWarningExpected;

    QTest::addRow("missing-import-dir")
            << fileThatNeedsImportPath << false << warningsShouldFailArgs()
            << Environment{ { u"QML_IMPORT_PATH"_s, importPath } } << noWarningExpected;

    QTest::addRow("import-dir-via-arg")
            << fileThatNeedsImportPath << true << QStringList{ u"-I"_s, importPath }
            << Environment{ { u"QML_IMPORT_PATH"_s, invalidImportPath } } << noWarningExpected;

    QTest::addRow("import-dir-via-env")
            << fileThatNeedsImportPath << true << QStringList{ u"-E"_s }
            << Environment{ { u"QML_IMPORT_PATH"_s, importPath } }
            << u"Using import directories passed from environment variable \"QML_IMPORT_PATH\": \"%1\"."_s
                       .arg(importPath);

    QTest::addRow("import-dir-via-env2")
            << fileThatNeedsImportPath << true << QStringList{ u"-E"_s }
            << Environment{ { u"QML2_IMPORT_PATH"_s, importPath } }
            << u"Using import directories passed from the deprecated environment variable \"QML2_IMPORT_PATH\": \"%1\"."_s
                       .arg(importPath);
}

void TestQmllint::environment()
{
    QFETCH(QString, file);
    QFETCH(bool, shouldSucceed);
    QFETCH(QStringList, extraArgs);
    QFETCH(Environment, env);
    QFETCH(QString, expectedWarning);

    const QString output = runQmllint(file, shouldSucceed, extraArgs, false, true, false, env);
    if (!expectedWarning.isEmpty()) {
        QVERIFY(output.contains(expectedWarning));
    }
}

void TestQmllint::maxWarnings()
{
    // warnings are not fatal by default
    runQmllint(testFile("badScript.qml"), true);
    // or when max-warnings is set to -1
    runQmllint(testFile("badScript.qml"), true, {"-W", "-1"});
    // 1 warning => should fail
    runQmllint(testFile("badScript.qml"), false, {"--max-warnings", "0"});
    // only 2 warning => should exit normally
    runQmllint(testFile("badScript.qml"), true, {"--max-warnings", "2"});
}

#endif

void TestQmllint::ignoreSettingsNotCommandLineOptions()
{
    const QString importPath = testFile(u"ImportPath"_s);
    // makes sure that ignore settings only ignores settings and not command line options like
    // "-I".
    const QString output = runQmllint(testFile(u"NeedImportPath.qml"_s), true,
                                      QStringList{ u"-I"_s, importPath }, true);
    // should not complain about not finding the module that is in importPath
    QCOMPARE(output, QString());
}

void TestQmllint::backslashedQmldirPath()
{
    const QString qmldirPath
            = testFile(u"ImportPath/ModuleInImportPath/qmldir"_s).replace('/', QDir::separator());
    const QString output = runQmllint(
            testFile(u"something.qml"_s), true, QStringList{ u"-i"_s, qmldirPath });
    QVERIFY(output.isEmpty());
}

#if QT_CONFIG(process)
void TestQmllint::importRelScript()
{
    QProcess proc;
    proc.start(m_qmllintPath, { QStringLiteral(TST_QMLLINT_IMPORT_REL_SCRIPT_ARGS) });
    QVERIFY(proc.waitForFinished());
    const QByteArray output = proc.readAllStandardOutput();
    QVERIFY2(output.isEmpty(), output.constData());
    const QByteArray errors = proc.readAllStandardError();
    QVERIFY2(errors.isEmpty(), errors.constData());
}
#endif

void TestQmllint::replayImportWarnings()
{
    QJsonArray warnings =
            callQmllint(testFile(u"duplicateTypeUserUser.qml"_s), CallQmllintOptions{});

    // No warning because the offending import is indirect.
    QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));

    // No cache clearing here. We want the warnings restored.
    warnings = callQmllint(testFile(u"DuplicateTypeUser.qml"_s), CallQmllintOptions{},
                           CallQmllintCheck::ShouldFail);

    // Warning because the offending import is now direct.
    searchWarnings(warnings, "Ambiguous type detected. T 1.0 is defined multiple times.");
}

void TestQmllint::errorCategory()
{
    {
        const QString output = runQmllint(testFile(u"HasUnqualified.qml"_s), false,
                                          QStringList{ u"--unqualified"_s, u"error"_s });
        QVERIFY(output.startsWith("Error: "));
    }
    {
        const QString output = runQmllint(testFile(u"HasUnqualified.qml"_s), true);
        QVERIFY(output.startsWith("Warning: "));
    }

}

QTEST_GUILESS_MAIN(TestQmllint)
#include "tst_qmllint.moc"
