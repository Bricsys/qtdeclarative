// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_modules.h"
#include "QtQmlLS/private/qqmllsutils_p.h"
#include "QtQmlLS/private/qqmlsemantictokens_p.h"
#include "QtQmlLS/private/documentsymbolutils_p.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

// Check if QTest already has a QTEST_CHECKED macro
#ifndef QTEST_CHECKED
#define QTEST_CHECKED(...) \
do { \
    __VA_ARGS__; \
    if (QTest::currentTestFailed()) \
        return; \
} while (false)
#endif

QT_USE_NAMESPACE
using namespace Qt::StringLiterals;
using namespace QLspSpecification;

static constexpr bool enable_debug_output = false;

tst_qmlls_modules::tst_qmlls_modules() : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    m_qmllsPath =
            QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlls");
#ifdef Q_OS_WIN
    m_qmllsPath += QLatin1String(".exe");
#endif
    // allow overriding of the executable, to be able to use a qmlEcho script (as described in
    // qmllanguageservertool.cpp)
    m_qmllsPath = qEnvironmentVariable("QMLLS", m_qmllsPath);
    // qputenv("QT_LOGGING_RULES",
    // "qt.languageserver.codemodel.debug=true;qt.languageserver.codemodel.warning=true"); // helps
    qputenv("QT_LOGGING_RULES", "*.debug=true;*.warning=true");
    // when using EditingRecorder
    m_server.setProgram(m_qmllsPath);
    // m_server.setArguments(QStringList() << u"-v"_s << u"-w"_s << u"7"_s);
}

void tst_qmlls_modules::init()
{
    QQmlDataTest::init();

    m_protocol = std::make_unique<QLanguageServerProtocol>(
            [this](const QByteArray &data) { m_server.write(data); });

    connect(&m_server, &QProcess::readyReadStandardOutput, this, [this]() {
        QByteArray data = m_server.readAllStandardOutput();
        m_protocol->receiveData(data);
    });

    if constexpr (enable_debug_output) {
        connect(&m_server, &QProcess::readyReadStandardError, this, [this]() {
            QProcess::ProcessChannel tmp = m_server.readChannel();
            m_server.setReadChannel(QProcess::StandardError);
            while (m_server.canReadLine())
                qDebug() << m_server.readLine();
            m_server.setReadChannel(tmp);
        });
    }

    m_server.start();

    InitializeParams clientInfo;
    clientInfo.rootUri = QUrl::fromLocalFile(dataDirectory() + "/default").toString().toUtf8();

    TextDocumentClientCapabilities tDoc;
    tDoc.typeDefinition = TypeDefinitionClientCapabilities{ false, false };

    PublishDiagnosticsClientCapabilities pDiag;
    tDoc.publishDiagnostics = pDiag;
    pDiag.versionSupport = true;
    clientInfo.capabilities.textDocument = tDoc;
    bool didInit = false;
    m_protocol->requestInitialize(clientInfo, [this, &didInit](const InitializeResult &serverInfo) {
        Q_UNUSED(serverInfo);
        m_protocol->notifyInitialized(InitializedParams());
        didInit = true;
    });
    QTRY_COMPARE_WITH_TIMEOUT(didInit, true, 10000);
}

void tst_qmlls_modules::cleanup()
{
    for (const QByteArray &uri : m_uriToClose) {
        DidCloseTextDocumentParams closeP;
        closeP.textDocument.uri = uri;
        m_protocol->notifyDidCloseTextDocument(closeP);
    }
    m_uriToClose.clear();

    // note: properly exit the language server
    m_protocol->requestShutdown(nullptr, []() {});
    m_protocol->notifyExit(nullptr);

    m_server.waitForFinished();
    QTRY_COMPARE(m_server.state(), QProcess::NotRunning);
    QCOMPARE(m_server.exitStatus(), QProcess::NormalExit);
}

/*!
\internal
Opens a file from a relative filePath, and returns the loaded uri.
Returns an empty option when the file could not be found.
*/
std::optional<QByteArray> tst_qmlls_modules::openFile(const QString &filePath)
{
    return openFileFromAbsolutePath(testFile(filePath));
}

/*!
\internal
Opens a file from an absolute filePath, and returns the loaded uri.
Returns an empty option when the file could not be found.
*/
std::optional<QByteArray> tst_qmlls_modules::openFileFromAbsolutePath(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    DidOpenTextDocumentParams oParams;
    TextDocumentItem textDocument;
    QByteArray uri = QUrl::fromLocalFile(filePath).toEncoded();
    textDocument.uri = uri;
    textDocument.text = file.readAll();
    oParams.textDocument = textDocument;
    m_protocol->notifyDidOpenTextDocument(oParams);
    m_uriToClose.append(uri);
    return uri;
}

/*!
\internal
Ignore qmllint warnings, when not needed for the test.
*/
void tst_qmlls_modules::ignoreDiagnostics()
{
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [](const QByteArray &, const PublishDiagnosticsParams &) {});
}

void tst_qmlls_modules::initTestCase()
{
    QQmlDataTest::initTestCase();
    if (!QFileInfo::exists(m_qmllsPath)) {
        QString message =
                QStringLiteral("qmlls executable not found (looked for %0)").arg(m_qmllsPath);
        QSKIP(qPrintable(message)); // until we add a feature for this we avoid failing here
    }
}

void tst_qmlls_modules::checkCompletions(const QByteArray &uri, int lineNr, int character,
                                         ExpectedCompletions expected, QStringList notExpected)
{
    CompletionParams cParams;
    cParams.position.line = lineNr;
    cParams.position.character = character;
    cParams.textDocument.uri = uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol->requestCompletion(
            cParams,
            [clean, uri, expected, notExpected](auto res) {
                QScopeGuard cleanup(clean);
                const QList<CompletionItem> *cItems = std::get_if<QList<CompletionItem>>(&res);

                if (!cItems) {
                    return;
                }

                QSet<QString> labels;
                QDuplicateTracker<QByteArray> modulesTracker;
                QDuplicateTracker<QByteArray> keywordsTracker;
                QDuplicateTracker<QByteArray> classesTracker;
                QDuplicateTracker<QByteArray> fieldsTracker;
                QDuplicateTracker<QByteArray> propertiesTracker;

                for (const CompletionItem &c : *cItems) {
                    if (c.kind->toInt() == int(CompletionItemKind::Module)) {
                        QVERIFY2(!modulesTracker.hasSeen(c.label), "Duplicate module: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Keyword)) {
                        QVERIFY2(!keywordsTracker.hasSeen(c.label),
                                 "Duplicate keyword: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Class)) {
                        QVERIFY2(!classesTracker.hasSeen(c.label), "Duplicate class: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Field)) {
                        QVERIFY2(!fieldsTracker.hasSeen(c.label), "Duplicate field: " + c.label);
                    } else if (c.kind->toInt() == int(CompletionItemKind::Property)) {
                        QVERIFY2(!propertiesTracker.hasSeen(c.label),
                                 "Duplicate property: " + c.label);
                    }
                    labels << c.label;
                }

                for (const ExpectedCompletion &exp : expected) {
                    QVERIFY2(labels.contains(exp.first),
                             u"no %1 in %2"_s
                                     .arg(exp.first,
                                          QStringList(labels.begin(), labels.end()).join(u", "_s))
                                     .toUtf8());
                    if (labels.contains(exp.first)) {
                        for (const CompletionItem &c : *cItems) {
                            const auto kind = static_cast<CompletionItemKind>(c.kind->toInt());

                            bool foundEntry = false;
                            bool hasCorrectKind = false;
                            for (const ExpectedCompletion &e : expected) {
                                if (c.label == e.first) {
                                    foundEntry = true;
                                    hasCorrectKind |= kind == e.second;
                                }
                            }

                            // Ignore QVERIFY for those completions not in the expected list.
                            if (!foundEntry)
                                continue;

                            QVERIFY2(hasCorrectKind,
                                     qPrintable(
                                             QString::fromLatin1(
                                                     "Completion item '%1' has wrong kind '%2'")
                                                     .arg(c.label)
                                                     .arg(QMetaEnum::fromType<CompletionItemKind>()
                                                                  .valueToKey(int(kind)))));
                        }
                    }
                }
                for (const QString &nexp : notExpected) {
                    QVERIFY2(!labels.contains(nexp),
                             u"found unexpected completion  %1"_s.arg(nexp).toUtf8());
                }
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

void tst_qmlls_modules::function_documentations_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("lineNr");
    QTest::addColumn<int>("character");
    QTest::addColumn<ExpectedDocumentations>("expectedDocs");

    const QString filePath = u"completions/Yyy.qml"_s;

    QTest::newRow("longfunction")
            << filePath << 5 << 14
            << ExpectedDocumentations{
                   std::make_tuple(u"lala"_s, u"returns void"_s, u"lala()"_s),
                   std::make_tuple(u"longfunction"_s, u"returns string"_s,
                                   uR"(longfunction(a, b, c = "c", d = "d"))"_s),
                   std::make_tuple(u"documentedFunction"_s, u"returns string"_s,
                                   uR"(// documentedFunction: is documented
// returns 'Good'
documentedFunction(arg1, arg2 = "Qt"))"_s),
               };
}

void tst_qmlls_modules::function_documentations()
{
    QFETCH(QString, filePath);
    QFETCH(int, lineNr);
    QFETCH(int, character);
    QFETCH(ExpectedDocumentations, expectedDocs);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);

    CompletionParams cParams;
    cParams.position.line = lineNr;
    cParams.position.character = character;
    cParams.textDocument.uri = *uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol->requestCompletion(
            cParams,
            [clean, uri, expectedDocs](auto res) {
                const QList<CompletionItem> *cItems = std::get_if<QList<CompletionItem>>(&res);

                if (!cItems) {
                    return;
                }

                for (const ExpectedDocumentation &exp : expectedDocs) {
                    bool hasFoundExpected = false;
                    const auto expectedLabel = std::get<0>(exp);
                    for (const CompletionItem &c : *cItems) {
                        if (c.kind->toInt() != int(CompletionItemKind::Method)) {
                            // Only check functions.
                            continue;
                        }

                        if (c.label == expectedLabel) {
                            hasFoundExpected = true;
                        }
                    }

                    QVERIFY2(hasFoundExpected,
                             qPrintable(u"expected completion label '%1' wasn't found"_s.arg(
                                     expectedLabel)));
                }

                for (const CompletionItem &c : *cItems) {
                    if (c.kind->toInt() != int(CompletionItemKind::Function)) {
                        // Only check functions.
                        continue;
                    }

                    QVERIFY(c.documentation != std::nullopt);
                    // We currently don't support 'MarkupContent', change this when we do.
                    QVERIFY(c.documentation->index() == 0);
                    const QByteArray cDoc = std::get<0>(*c.documentation);

                    for (const ExpectedDocumentation &exp : expectedDocs) {
                        const auto &[label, details, docs] = exp;

                        if (c.label != label)
                            continue;

                        QVERIFY2(c.detail == details,
                                 qPrintable(u"Completion item '%1' has wrong details '%2'"_s
                                                    .arg(label).arg(*c.detail)));
                        QVERIFY2(cDoc == docs,
                                 qPrintable(u"Completion item '%1' has wrong documentation '%2'"_s
                                                    .arg(label).arg(cDoc)));
                    }
                }
                clean();
            },
            [clean](const ResponseError &err) {
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
                clean();
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 3000);
}

void tst_qmlls_modules::buildDir()
{
    ignoreDiagnostics();
    const QString filePath = u"completions/fromBuildDir.qml"_s;
    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QTEST_CHECKED(checkCompletions(
            *uri, 3, 0,
            ExpectedCompletions({
                    { u"Rectangle"_s, CompletionItemKind::Constructor },
            }),
            QStringList({ u"BuildDirType"_s, u"QtQuick"_s, u"width"_s, u"vector4d"_s })));
    Notifications::AddBuildDirsParams bDirs;
    UriToBuildDirs ub;
    ub.baseUri = *uri;
    ub.buildDirs.append(testFile("buildDir").toUtf8());
    bDirs.buildDirsToSet.append(ub);
    m_protocol->typedRpc()->sendNotification(QByteArray(Notifications::AddBuildDirsMethod), bDirs);

    DidChangeTextDocumentParams didChange;
    didChange.textDocument.uri = *uri;
    didChange.textDocument.version = 2;

    // change the file content to force qqml— to recreate a new DomItem
    // if it reuses the old DomItem then it will not know about the added build directory
    TextDocumentContentChangeEvent change;
    change.range = Range{ Position{ 4, 0 }, Position{ 4, 0 } };
    change.text = "\n";

    didChange.contentChanges.append(change);
    m_protocol->notifyDidChangeTextDocument(didChange);

    QTEST_CHECKED(checkCompletions(*uri, 3, 0,
                                   ExpectedCompletions({
                                           { u"BuildDirType"_s, CompletionItemKind::Constructor },
                                           { u"Rectangle"_s, CompletionItemKind::Constructor },
                                           { u"width"_s, CompletionItemKind::Property },
                                   }),
                                   QStringList({ u"QtQuick"_s, u"vector4d"_s })));
}

void tst_qmlls_modules::inMemoryEnumCompletion()
{
    ignoreDiagnostics();
    const QString filePath = u"completions/SimpleItem.qml"_s;
    const auto uri = openFile(filePath);
    QVERIFY(uri);

    // add in memory enum that does not exist on disk
    TextDocumentContentChangeEvent change;
    change.range = Range{ Position{ 3, 0 }, Position{ 3, 0 } };
    change.text = "enum HelloEnum { Hello, World, Kitty, Qt }\nproperty var myP: SimpleItem.H";

    DidChangeTextDocumentParams didChange;
    didChange.textDocument.uri = *uri;
    didChange.textDocument.version = 2;
    didChange.contentChanges.append(change);
    m_protocol->notifyDidChangeTextDocument(didChange);

    QTEST_CHECKED(checkCompletions(*uri, 4, 29,
                                   ExpectedCompletions({
                                           { u"Hello"_s, CompletionItemKind::EnumMember },
                                   }),
                                   QStringList({ u"QtQuick"_s, u"vector4d"_s })));
}

void tst_qmlls_modules::automaticSemicolonInsertionForCompletions_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("row");
    QTest::addColumn<int>("column");

    QTest::addRow("bindingAfterDot") << u"completions/bindingAfterDot.qml"_s << 11 << 32;
    QTest::addRow("defaultBindingAfterDot")
            << u"completions/defaultBindingAfterDot.qml"_s << 11 << 32;
}

void tst_qmlls_modules::automaticSemicolonInsertionForCompletions()
{
    ignoreDiagnostics();
    QFETCH(QString, filePath);
    QFETCH(int, row);
    QFETCH(int, column);
    row--;
    column--;
    const auto uri = openFile(filePath);
    QVERIFY(uri);

    QTEST_CHECKED(checkCompletions(
            *uri, row, column,
            ExpectedCompletions({
                    { u"good"_s, CompletionItemKind::Property },
            }),
            QStringList({ u"bad"_s, u"BuildDirType"_s, u"QtQuick"_s, u"width"_s, u"vector4d"_s })));
}

void tst_qmlls_modules::checkQuickSnippets()
{
    ignoreDiagnostics();
    const auto uri = openFile(u"completions/Yyy.qml"_s);
    QVERIFY(uri);

    // if at least one snippet is there, then the pluginloading works. To test the plugin itself,
    // add tests in tst_qmlls_utils instead.
    QTEST_CHECKED(checkCompletions(
            *uri, 4, 3,
            ExpectedCompletions({
                    { u"BorderImage snippet"_s, CompletionItemKind::Snippet },
            }),
            QStringList({})));
}

void tst_qmlls_modules::goToTypeDefinition_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("expectedFilePath");
    QTest::addColumn<int>("expectedStartLine");
    QTest::addColumn<int>("expectedStartCharacter");
    QTest::addColumn<int>("expectedEndLine");
    QTest::addColumn<int>("expectedEndCharacter");

    const QString yyyPath = u"completions/Yyy.qml"_s;
    const QString zzzPath = u"completions/Zzz.qml"_s;
    const QString someBasePath = u"completions/SomeBase.qml"_s;

    QTest::newRow("BaseOfYyy") << yyyPath << 3 << 1 << zzzPath << 2 << 0 << 9 << 1;
    QTest::newRow("BaseOfIC") << yyyPath << 29 << 19 << zzzPath << 2 << 0 << 9 << 1;

    QTest::newRow("PropertyType") << yyyPath << 30 << 14 << someBasePath << 2 << 0 << 4 << 1;

    QTest::newRow("TypeInIC") << yyyPath << 29 << 36 << someBasePath << 2 << 0 << 4 << 1;
    QTest::newRow("ICTypeDefinition") << yyyPath << 29 << 15 << yyyPath << 29 << 14 << 29 << 16;
}

void tst_qmlls_modules::goToTypeDefinition()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, expectedFilePath);
    QFETCH(int, expectedStartLine);
    QFETCH(int, expectedStartCharacter);
    QFETCH(int, expectedEndLine);
    QFETCH(int, expectedEndCharacter);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QVERIFY(uri->startsWith("file://"_ba));

    // note: do not call openFile(expectedFilePath), the definition should be found even if
    // the qmlls user did not open the qml file with the definition yet.
    const auto expectedUri = testFileUrl(expectedFilePath).toEncoded();

    // TODO
    TypeDefinitionParams params;
    params.position.line = line;
    params.position.character = character;
    params.textDocument.uri = *uri;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol->requestTypeDefinition(
            params,
            [&](auto res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QList<Location>>(&res);

                QVERIFY(result);

                QCOMPARE(result->size(), 1);

                Location l = result->front();
                QCOMPARE(l.uri, expectedUri);
                QCOMPARE(l.range.start.line, expectedStartLine);
                QCOMPARE(l.range.start.character, expectedStartCharacter);
                QCOMPARE(l.range.end.line, expectedEndLine);
                QCOMPARE(l.range.end.character, expectedEndCharacter);
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

void tst_qmlls_modules::goToDefinition_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");

    QTest::addColumn<QByteArray>("expectedUri");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedStartLine");
    QTest::addColumn<int>("expectedStartCharacter");
    QTest::addColumn<int>("expectedEndLine");
    QTest::addColumn<size_t>("expectedEndCharacter");

    const QByteArray JSDefinitionsQml =
            testFileUrl(u"findDefinition/jsDefinitions.qml"_s).toEncoded();
    const QString JSDefinitionsQmlPath = u"findDefinition/jsDefinitions.qml"_s;
    const QByteArray noResultExpected;

    QTest::addRow("JSIdentifierX") << JSDefinitionsQmlPath << 14 << 11 << JSDefinitionsQml << 13
                                   << 13 << 13 << 13 + strlen("x");
    QTest::addRow("propertyI") << JSDefinitionsQmlPath << 14 << 14 << JSDefinitionsQml << 9 << 18
                               << 9 << 18 + strlen("i");
    QTest::addRow("qualifiedPropertyI") << JSDefinitionsQmlPath << 15 << 21 << JSDefinitionsQml << 9
                                        << 18 << 9 << 18 + strlen("i");
    QTest::addRow("id") << JSDefinitionsQmlPath << 15 << 17 << JSDefinitionsQml << 7 << 9 << 7
                        << 9 + strlen("rootId");

    QTest::addRow("comment") << JSDefinitionsQmlPath << 10 << 21 << noResultExpected << -1 << -1
                             << -1 << size_t{};
}

void tst_qmlls_modules::goToDefinition()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QByteArray, expectedUri);
    QFETCH(int, expectedStartLine);
    QFETCH(int, expectedStartCharacter);
    QFETCH(int, expectedEndLine);
    QFETCH(size_t, expectedEndCharacter);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QVERIFY(uri->startsWith("file://"_ba));

    DefinitionParams params;
    params.position.line = line - 1;
    params.position.character = character - 1;
    params.textDocument.uri = *uri;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };

    m_protocol->requestDefinition(
            params,
            [&](auto res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QList<Location>>(&res);
                const QByteArray noResultExpected;

                QVERIFY(result);
                if (expectedUri == noResultExpected) {
                    QCOMPARE(result->size(), 0);
                } else {
                    QCOMPARE(result->size(), 1);

                    Location l = result->front();
                    QCOMPARE(l.uri, expectedUri);
                    QCOMPARE(l.range.start.line, expectedStartLine - 1);
                    QCOMPARE(l.range.start.character, expectedStartCharacter - 1);
                    QCOMPARE(l.range.end.line, expectedEndLine - 1);
                    QCOMPARE(l.range.end.character, (int)(expectedEndCharacter - 1));
                }
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 30000);
}

// startLine and startCharacter start at 1, not 0
static QLspSpecification::Range rangeFrom(const QString &code, quint32 startLine,
                                          quint32 startCharacter, quint32 length)
{
    QLspSpecification::Range range;

    // the LSP works with lines and characters starting at 0
    range.start.line = startLine - 1;
    range.start.character = startCharacter - 1;

    quint32 startOffset = QQmlLSUtils::textOffsetFrom(code, startLine - 1, startCharacter - 1);
    auto end = QQmlLSUtils::textRowAndColumnFrom(code, startOffset + length);
    range.end.line = end.line;
    range.end.character = end.character;

    return range;
}

// startLine and startCharacter start at 1, not 0
static QLspSpecification::Location locationFrom(const QByteArray fileName, const QString &code,
                                                quint32 startLine, quint32 startCharacter,
                                                quint32 length)
{
    QLspSpecification::Location location;
    location.uri = QQmlLSUtils::qmlUrlToLspUri(fileName);
    location.range = rangeFrom(code, startLine, startCharacter, length);
    return location;
}

void tst_qmlls_modules::findUsages_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QList<QLspSpecification::Location>>("expectedUsages");

    const QByteArray jsIdentifierUsagesUri =
            testFileUrl("findUsages/jsIdentifierUsages.qml").toEncoded();
    const QString jsIdentifierUsagesPath = u"findUsages/jsIdentifierUsages.qml"_s;

    QString jsIdentifierUsagesContent;
    {
        QFile file(testFile("findUsages/jsIdentifierUsages.qml").toUtf8());
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        jsIdentifierUsagesContent = QString::fromUtf8(file.readAll());
    }

    // line and character start at 1!
    const QList<QLspSpecification::Location> sumUsages = {
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 8, 13,
                     static_cast<quint32>(strlen("sum"))),
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 10, 13,
                     static_cast<quint32>(strlen("sum"))),
        locationFrom(jsIdentifierUsagesUri, jsIdentifierUsagesContent, 10, 19,
                     static_cast<quint32>(strlen("sum"))),
    };
    QVERIFY(sumUsages.front().uri.startsWith("file://"_ba));

    // line and character start at 1!
    QTest::addRow("sumUsagesFromUsage") << jsIdentifierUsagesPath << 10 << 14 << sumUsages;
    QTest::addRow("sumUsagesFromUsage2") << jsIdentifierUsagesPath << 10 << 20 << sumUsages;
    QTest::addRow("sumUsagesFromDefinition") << jsIdentifierUsagesPath << 8 << 14 << sumUsages;
}

static bool locationsAreEqual(const QLspSpecification::Location &a,
                              const QLspSpecification::Location &b)
{
    return std::tie(a.uri, a.range.start.character, a.range.start.line, a.range.end.character,
                    a.range.end.line)
            == std::tie(b.uri, b.range.start.character, b.range.start.line, b.range.end.character,
                        b.range.end.line);
}

static bool locationListsAreEqual(const QList<QLspSpecification::Location> &a,
                                  const QList<QLspSpecification::Location> &b)
{
    return std::equal(a.cbegin(), a.cend(), b.cbegin(), b.cend(), locationsAreEqual);
}

static QString locationToString(const QLspSpecification::Location &l)
{
    QString s = u"%1: (%2, %3) - (%4, %5)"_s.arg(l.uri)
                        .arg(l.range.start.line)
                        .arg(l.range.start.character)
                        .arg(l.range.end.line)
                        .arg(l.range.end.character);
    return s;
}

void tst_qmlls_modules::findUsages()
{
    QFETCH(QString, filePath);
    // line and character start at 1!
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QList<QLspSpecification::Location>, expectedUsages);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QVERIFY(uri->startsWith("file://"_ba));

    ReferenceParams params;
    params.position.line = line - 1;
    params.position.character = character - 1;
    params.textDocument.uri = *uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };
    m_protocol->requestReference(
            params,
            [&](auto res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QList<Location>>(&res);

                QVERIFY(result);
                if constexpr (enable_debug_output) {
                    if (!locationListsAreEqual(*result, expectedUsages)) {
                        qDebug() << "Got following locations:";
                        for (auto &x : *result) {
                            qDebug() << locationToString(x);
                        }
                        qDebug() << "But expected:";
                        for (auto &x : expectedUsages) {
                            qDebug() << locationToString(x);
                        }
                    }
                } else {
                    // dont get warning on unused function when enable_debug_output is false
                    Q_UNUSED(locationToString);
                }

                QVERIFY(locationListsAreEqual(*result, expectedUsages));
            },
            [clean](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                QVERIFY2(false, "error computing the completion");
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 3000);
}

void tst_qmlls_modules::documentFormatting_data()
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("expectedFile");

    QDir directory(QT_QMLFORMATTEST_DATADIR);

    // Exclude some test files which require options support
    QStringList excludedFiles;
    excludedFiles << u"tests/auto/qml/qmlformat/data/checkIdsNewline.qml"_s;
    excludedFiles << u"tests/auto/qml/qmlformat/data/normalizedFunctionsSpacing.qml"_s;
    excludedFiles << u"tests/auto/qml/qmlformat/data/normalizedObjectsSpacing.qml"_s;
    excludedFiles << u"tests/auto/qml/qmlformat/data/sortingImports.qml"_s;

    // excluded because it crashes Dom construction
    // TODO: fix QQMLDomAstConstructor to not crash on these files, see QTBUG-116392
    excludedFiles << u"tests/auto/qml/qmlformat/data/ecmaScriptClassInQml.qml"_s;
    excludedFiles << u"tests/auto/qml/qmlformat/data/Example1.qml"_s;
    excludedFiles << u"tests/auto/qml/qmlformat/data/nestedFunctions.qml"_s;

    const auto shouldSkip = [&excludedFiles](const QString &fileName) {
        for (const QString &file : excludedFiles) {
            if (fileName.endsWith(file))
                return true;
        }
        return false;
    };

    // Filter to include files contain .formatted.
    const auto formattedFilesInfo =
            directory.entryInfoList(QStringList{ { "*.formatted.qml" } }, QDir::Files);
    for (const auto &formattedFileInfo : formattedFilesInfo) {
        const QFileInfo unformattedFileInfo(directory, formattedFileInfo.fileName().remove(".formatted"));
        const auto unformattedFilePath = unformattedFileInfo.canonicalFilePath();
        if (shouldSkip(unformattedFilePath))
            continue;

        QTest::newRow(qPrintable(unformattedFileInfo.fileName()))
                << unformattedFilePath << formattedFileInfo.canonicalFilePath();
    }

    // Extra tests
    QTest::newRow("leading-and-trailing-blanklines")
            << testFile("formatting/blanklines.qml")
            << testFile("formatting/blanklines.formatted.qml");

    QTest::newRow("read-qmlformat-ini")
            << directory.filePath(u"settings/Example1.qml"_s)
            << directory.filePath(u"settings/Example1.formatted_mac_cr.qml"_s);
}

void tst_qmlls_modules::documentFormatting()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, expectedFile);

    ignoreDiagnostics();

    const auto uri = openFileFromAbsolutePath(originalFile);
    QVERIFY(uri);

    DocumentFormattingParams params;
    params.textDocument.uri = *uri;

    const auto lineCount = [](const QString &filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Error while opening the file " << filePath;
            return -1;
        }
        int lineCount = 0;
        QString line;
        while (!file.atEnd()) {
            line = file.readLine();
            ++lineCount;
        }
        if (line.endsWith('\n'))
            ++lineCount;
        return lineCount;
    };

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };
    auto &&responseHandler = [&](auto response) {
        QScopeGuard cleanup(clean);
        if (std::holds_alternative<QList<TextEdit>>(response)) {
            const auto results = std::get<QList<TextEdit>>(response);
            QVERIFY(results.size() == 1);
            QFile file(expectedFile);
            if (!file.open(QIODevice::ReadOnly)) {
                qWarning() << "Error while opening the file " << expectedFile;
                return;
            }

            const auto &textEdit = results.first();
            QCOMPARE(textEdit.range.start.line, 0);
            QCOMPARE(textEdit.range.start.character, 0);
            QCOMPARE(textEdit.range.end.line, lineCount(originalFile));
            QCOMPARE(textEdit.range.end.character, 0);

            QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_enum.qml",
                         "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)",
                         Continue);
            QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_id.qml",
                         "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)",
                         Continue);
            QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_parameters.qml",
                         "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)",
                         Continue);
            QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_QtObject.qml",
                         "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)",
                         Continue);
            QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_signal.qml",
                         "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)",
                         Continue);

            QCOMPARE(textEdit.newText, file.readAll());
        }
    };

    auto &&errorHandler = [&clean](const ResponseError &err) {
        QScopeGuard cleanup(clean);
        ProtocolBase::defaultResponseErrorHandler(err);
        QVERIFY2(false, "error computing the completion");
    };
    m_protocol->requestDocumentFormatting(params, std::move(responseHandler),
                                          std::move(errorHandler));

    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 50000);
}

void tst_qmlls_modules::renameUsages_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("newName");
    QTest::addColumn<QLspSpecification::WorkspaceEdit>("expectedEdit");
    QTest::addColumn<QLspSpecification::ResponseError>("expectedError");

    QLspSpecification::ResponseError noError;

    const QString jsIdentifierUsagesPath = u"findUsages/jsIdentifierUsages.qml"_s;
    const QByteArray jsIdentifierUsagesUri =
            testFileUrl("findUsages/jsIdentifierUsages.qml").toEncoded();

    QString jsIdentifierUsagesContent;
    {
        QFile file(testFile("findUsages/jsIdentifierUsages.qml").toUtf8());
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        jsIdentifierUsagesContent = QString::fromUtf8(file.readAll());
    }
    QString renamingContent;
    {
        QFile file(testFile("renameUsages/main.qml").toUtf8());
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        renamingContent = QString::fromUtf8(file.readAll());
    }

    // TODO: create workspace edit for the tests
    QLspSpecification::WorkspaceEdit sumRenames{
        std::nullopt, // TODO
        QList<QLspSpecification::WorkspaceEdit::DocumentChange>{
                TextDocumentEdit{
                        OptionalVersionedTextDocumentIdentifier{ { jsIdentifierUsagesUri } },
                        {
                                TextEdit{
                                        rangeFrom(jsIdentifierUsagesContent, 8, 13,
                                                  static_cast<quint32>(strlen("sum"))),
                                        "specialSum" },
                                TextEdit{
                                        rangeFrom(jsIdentifierUsagesContent, 10, 13,
                                                  static_cast<quint32>(strlen("sum"))),
                                        "specialSum" },
                                TextEdit{
                                        rangeFrom(jsIdentifierUsagesContent, 10, 19,
                                                  static_cast<quint32>(strlen("sum"))),
                                        "specialSum" },
                        } },
        }
    };

    // line and character start at 1!
    QTest::addRow("sumRenameFromUsage")
            << jsIdentifierUsagesPath << 10 << 14 << u"specialSum"_s << sumRenames << noError;
    QTest::addRow("sumRenameFromUsage2")
            << jsIdentifierUsagesPath << 10 << 20 << u"specialSum"_s << sumRenames << noError;
    QTest::addRow("sumRenameFromDefinition")
            << jsIdentifierUsagesPath << 8 << 14 << u"specialSum"_s << sumRenames << noError;
    QTest::addRow("invalidSumRenameFromDefinition")
            << jsIdentifierUsagesPath << 8 << 14 << u"function"_s << sumRenames
            << QLspSpecification::ResponseError{
                   0,
                   "Invalid EcmaScript identifier!",
                   std::nullopt,
               };

    const QString renameUsagesPath = u"renameUsages/main.qml"_s;
    const QByteArray renameUsagesUri = testFileUrl("renameUsages/main.qml").toEncoded();
    const QByteArray renameMeUri = testFileUrl("renameUsages/RenameMe.qml").toEncoded();
    const QByteArray renameMe2Uri = testFileUrl("renameUsages/RenameMe2.ui.qml").toEncoded();

    const QByteArray newFileUri = testFileUrl("renameUsages/HelloWorld.qml").toEncoded();
    const QByteArray newFileUri2 = testFileUrl("renameUsages/HelloWorld.ui.qml").toEncoded();

    {

        const QLspSpecification::WorkspaceEdit qmlComponentRename{
            std::nullopt,
            QList<QLspSpecification::WorkspaceEdit::DocumentChange>{
                    TextDocumentEdit{
                            OptionalVersionedTextDocumentIdentifier{ { renameUsagesUri } },
                            {
                                    TextEdit{ rangeFrom(renamingContent, 4, 5,
                                                        static_cast<quint32>(strlen("RenameMe"))),
                                              "HelloWorld" },
                            } },
                    RenameFile{ "rename", renameMeUri, newFileUri } }
        };

        QTest::addRow("renameQmlComponent")
                << renameUsagesPath << 4 << 8 << u"HelloWorld"_s << qmlComponentRename << noError;
    }

    {
        QLspSpecification::WorkspaceEdit qmlComponentRename{
            std::nullopt,
            QList<QLspSpecification::WorkspaceEdit::DocumentChange>{
                    TextDocumentEdit{
                            OptionalVersionedTextDocumentIdentifier{ { renameUsagesUri } },
                            {
                                    TextEdit{ rangeFrom(renamingContent, 5, 5,
                                                        static_cast<quint32>(strlen("RenameMe2"))),
                                              "HelloWorld" },
                            } },
                    RenameFile{ "rename", renameMe2Uri, newFileUri2 } }
        };

        QTest::addRow("renameUiQmlComponent")
                << renameUsagesPath << 5 << 8 << u"HelloWorld"_s << qmlComponentRename << noError;
    }
}

void tst_qmlls_modules::compareQTextDocumentEdit(const TextDocumentEdit &a,
                                                 const TextDocumentEdit &b)
{

    QCOMPARE(a.textDocument.uri, b.textDocument.uri);
    QVERIFY(a.textDocument.uri.startsWith("file://"));
    QCOMPARE(a.textDocument.version, b.textDocument.version);
    QCOMPARE(a.edits.size(), b.edits.size());

    for (qsizetype j = 0; j < a.edits.size(); ++j) {
        std::visit(
                [](auto &&textEdit, auto &&expectedTextEdit) {
                    using U = std::decay_t<decltype(textEdit)>;
                    using V = std::decay_t<decltype(expectedTextEdit)>;

                    if constexpr (std::conjunction_v<std::is_same<U, V>,
                                                     std::is_same<U, TextEdit>>) {
                        QCOMPARE(textEdit.range.start.line, expectedTextEdit.range.start.line);
                        QCOMPARE(textEdit.range.start.character,
                                 expectedTextEdit.range.start.character);
                        QCOMPARE(textEdit.range.end.line, expectedTextEdit.range.end.line);
                        QCOMPARE(textEdit.range.end.character,
                                 expectedTextEdit.range.end.character);
                        QCOMPARE(textEdit.newText, expectedTextEdit.newText);
                    } else {
                        QFAIL("Comparison not implemented");
                    }
                },
                a.edits[j], b.edits[j]);
    }
}

void tst_qmlls_modules::renameUsages()
{
    QFETCH(QString, filePath);
    // line and character start at 1!
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, newName);
    QFETCH(QLspSpecification::WorkspaceEdit, expectedEdit);
    QFETCH(QLspSpecification::ResponseError, expectedError);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QVERIFY(uri->startsWith("file://"_ba));

    RenameParams params;
    params.position.line = line - 1;
    params.position.character = character - 1;
    params.textDocument.uri = *uri;
    params.newName = newName.toUtf8();

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    auto clean = [didFinish]() { *didFinish = true; };
    m_protocol->requestRename(
            params,
            [&](auto &&res) {
                QScopeGuard cleanup(clean);
                auto *result = std::get_if<QLspSpecification::WorkspaceEdit>(&res);

                QVERIFY(result);
                QCOMPARE(result->changes.has_value(), expectedEdit.changes.has_value());
                QCOMPARE(result->changeAnnotations.has_value(),
                         expectedEdit.changeAnnotations.has_value());
                QCOMPARE(result->documentChanges.has_value(),
                         expectedEdit.documentChanges.has_value());

                auto &documentChanges = *result->documentChanges;
                auto &expectedDocumentChanges = *expectedEdit.documentChanges;

                if (!expectedError.message.isEmpty())
                    QVERIFY2(false, "No expected error was thrown.");

                QCOMPARE(documentChanges.size(), expectedDocumentChanges.size());

                for (qsizetype i = 0; i < expectedDocumentChanges.size(); ++i) {
                    QCOMPARE(documentChanges[i].index(), expectedDocumentChanges[i].index());
                    if (std::holds_alternative<TextDocumentEdit>(documentChanges[i])) {
                        compareQTextDocumentEdit(
                                std::get<TextDocumentEdit>(documentChanges[i]),
                                std::get<TextDocumentEdit>(expectedDocumentChanges[i]));
                    } else if (std::holds_alternative<RenameFile>(documentChanges[i])) {
                        const auto &actual = std::get<RenameFile>(documentChanges[i]);
                        const auto &expected = std::get<RenameFile>(expectedDocumentChanges[i]);

                        QCOMPARE(actual.kind, expected.kind);
                        QCOMPARE(expected.kind, "rename");
                        QCOMPARE(actual.oldUri, expected.oldUri);
                        QCOMPARE(actual.newUri, expected.newUri);
                        QCOMPARE(actual.options.has_value(), expected.options.has_value());
                        if (expected.options.has_value()) {
                            QCOMPARE(actual.options->overwrite, expected.options->overwrite);
                            QCOMPARE(actual.options->ignoreIfExists,
                                     expected.options->ignoreIfExists);
                        }
                        QCOMPARE(actual.annotationId, expected.annotationId);

                    } else {
                        QFAIL("TODO: implement me!");
                    }
                }
            },
            [clean, &expectedError](const ResponseError &err) {
                QScopeGuard cleanup(clean);
                ProtocolBase::defaultResponseErrorHandler(err);
                if (expectedError.message.isEmpty())
                    QVERIFY2(false, "unexpected error computing the completion");
                QCOMPARE(err.code, expectedError.code);
                QCOMPARE(err.message, expectedError.message);
            });
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 3000);
}

struct EditingRecorder
{
    QList<DidChangeTextDocumentParams> actions;
    QHash<int, QString> diagnosticsPerFileVersions;

    /*!
    All the indexes passed here must start at 1!

    If you want to make sure that your own written changes make sense, use
    \code
    qputenv("QT_LOGGING_RULES",
    "qt.languageserver.codemodel.debug=true;qt.languageserver.codemodel.warning=true"); \endcode
    before starting qmlls. It will print the differences between the different versions, and helps
    when some indices are off.
    */
    void changeText(int startLine, int startCharacter, int endLine, int endCharacter,
                    QString newText)
    {
        // The LSP starts at 0
        QVERIFY(startLine > 0);
        QVERIFY(startCharacter > 0);
        QVERIFY(endLine > 0);
        QVERIFY(endCharacter > 0);

        --startLine;
        --startCharacter;
        --endLine;
        --endCharacter;

        DidChangeTextDocumentParams params;
        params.textDocument = VersionedTextDocumentIdentifier{ { lastFileUri }, ++version };
        params.contentChanges.append({
                Range{ Position{ startLine, startCharacter }, Position{ endLine, endCharacter } },
                std::nullopt, // deprecated range length
                newText.toUtf8(),
        });
        actions.append(params);
    }

    void setFile(const QString &filePath) { lastFilePath = filePath; }

    void setCurrentExpectedDiagnostic(const QString &diagnostic)
    {
        Q_ASSERT(diagnosticsPerFileVersions.find(version) == diagnosticsPerFileVersions.end());
        diagnosticsPerFileVersions[version] = diagnostic;
    }

    QString lastFilePath;
    QByteArray lastFileUri;
    int version = 0;
};

static constexpr int characterAfter(const char *line)
{
    return static_cast<int>(std::char_traits<char>::length(line) + 1);
}

static EditingRecorder propertyTypoScenario(const QByteArray &fileUri)
{
    EditingRecorder propertyTypo;
    propertyTypo.lastFileUri = fileUri;

    propertyTypo.changeText(5, 1, 5, 1, u"    property int t"_s);

    // replace property by propertyt and expect a complaint from the parser
    propertyTypo.changeText(5, characterAfter("    property"), 5, characterAfter("    property"),
                            u"t"_s);
    propertyTypo.setCurrentExpectedDiagnostic(u"Expected token"_s);

    // replace propertyt back to property and expect no complaint from the parser
    propertyTypo.changeText(5, characterAfter("    property"), 5, characterAfter("    propertyt"),
                            u""_s);

    // replace property by propertyt and expect a complaint from the parser
    propertyTypo.changeText(5, characterAfter("    property"), 5, characterAfter("    property"),
                            u"t"_s);
    propertyTypo.setCurrentExpectedDiagnostic(u"Expected token"_s);

    // now, simulate some slow typing and expect the previous warning to not disappear
    const QString data = u"Item {}\n"_s;
    for (int i = 0; i < data.size(); ++i) {
        propertyTypo.changeText(6, i + 1, 6, i + 1, data[i]);
        propertyTypo.setCurrentExpectedDiagnostic(u"Expected token"_s);
    }

    // replace propertyt back to property and expect no complaint from the parser
    propertyTypo.changeText(5, characterAfter("    property"), 5, characterAfter("    propertyt"),
                            u""_s);

    return propertyTypo;
}

static EditingRecorder inMemoryEnumScenario(const QByteArray &fileUri)
{
    EditingRecorder scenario;
    scenario.lastFileUri = fileUri;

    const QString enumWithTypo =
            u"enum Hello { World, Qt, Kitty }\nproperty var xxx: SimpleItem.Q"_s;
    // expect an error because of the missing "t"
    scenario.changeText(5, 1, 5, 1, enumWithTypo);
    scenario.setCurrentExpectedDiagnostic("Member \"Q\" not found");

    // fix the typo and expect no more error, despite the enum not existing on the file on disk
    scenario.changeText(6, 31, 6, 31, u"t"_s);

    return scenario;
}

void tst_qmlls_modules::linting_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<EditingRecorder>("recorder");

    QTest::addRow("property-typo")
            << u"linting/SimpleItem.qml"_s
            << propertyTypoScenario(testFileUrl(u"linting/SimpleItem.qml"_s).toEncoded());

    QTest::addRow("in-memory-enum")
            << u"linting/SimpleItem.qml"_s
            << inMemoryEnumScenario(testFileUrl(u"linting/SimpleItem.qml"_s).toEncoded());
}

void tst_qmlls_modules::linting()
{
    QFETCH(QString, filePath);
    QFETCH(EditingRecorder, recorder);
    bool diagnosticOk = false;
    const auto uri = openFile(filePath);
    QVERIFY(uri);
    recorder.lastFileUri = *uri;
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [&recorder, &diagnosticOk, &uri](const QByteArray &,
                                             const PublishDiagnosticsParams &p) {
                if (p.uri != *uri || !p.version)
                    return;
                auto expectedMessage = recorder.diagnosticsPerFileVersions.find(*p.version);
                if (expectedMessage == recorder.diagnosticsPerFileVersions.end()) {
                    if constexpr (enable_debug_output) {
                        if (p.diagnostics.size() > 0)
                            qDebug() << "Did not expect message" << p.diagnostics.front().message;
                    }

                    QVERIFY(p.diagnostics.size() == 0);
                    diagnosticOk = true;
                    return;
                }
                QVERIFY(p.diagnostics.size() > 0);
                if constexpr (enable_debug_output) {
                    if (!p.diagnostics.front().message.contains(expectedMessage->toUtf8())) {
                        qDebug() << "expected a message with" << *expectedMessage << "but got"
                                 << p.diagnostics.front().message;
                    }
                }
                QVERIFY(p.diagnostics.front().message.contains(expectedMessage->toUtf8()));
                diagnosticOk = true;
            });
    for (const auto &action : recorder.actions) {
        m_protocol->notifyDidChangeTextDocument(action);
        QTRY_VERIFY_WITH_TIMEOUT(diagnosticOk, 5000);
        diagnosticOk = false;
    }
}

void tst_qmlls_modules::warnings_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QStringList>("expectedWarnings");

    QTest::addRow("unqualifiedAccess") << u"warnings/withoutQmllintIni/unqualifiedAccess.qml"_s
                                       << QStringList{ u"Unqualified access [unqualified]"_s };

    QTest::addRow("disableUnqualifiedEnabledCompiler")
            << u"warnings/disableUnqualifiedEnableCompiler/unqualifiedAccess.qml"_s
            << QStringList { u"Could not compile binding for i: Cannot access value for name unqualifiedAccess [compiler]"_s };

    QTest::addRow("enableQmllsGenerationIniViaCMake")
            << u"warnings/InvalidImport.qml"_s
            << QStringList{
                   u"Warnings occurred while importing module \"foobar\": [import]"_s,
                   u"Failed to import foobar. Are your import paths set up properly? Did you build your project? If yes, did you set the \"QT_QML_GENERATE_QMLLS_INI\" CMake variable on your project to \"ON\"? [import]"_s,
                   u"Warnings occurred while importing module \"foobaz\": [import]"_s,
                   u"Failed to import foobaz. Are your import paths set up properly? Did you build your project? If yes, did you set the \"QT_QML_GENERATE_QMLLS_INI\" CMake variable on your project to \"ON\"? [import]"_s,
               };
}

void tst_qmlls_modules::warnings()
{
    QFETCH(QString, filePath);
    QFETCH(QStringList, expectedWarnings);

    bool diagnosticOk = false;
    const auto uri = openFile(filePath);
    QVERIFY(uri);
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [&expectedWarnings, &diagnosticOk, &uri](const QByteArray &,
                                                    const PublishDiagnosticsParams &p) -> void {
                if (p.uri != *uri || !p.version)
                    return;

                if (expectedWarnings.isEmpty()) {
                    for (const auto& x: p.diagnostics)
                        qDebug() << "Received unexpected message:" << x.message;
                    QCOMPARE(p.diagnostics.size(), 0);
                    diagnosticOk = true;
                    return;
                }

                QCOMPARE(p.diagnostics.size(), expectedWarnings.size());
                for (qsizetype i = 0; i < p.diagnostics.size(); ++i) {
                    QCOMPARE(p.diagnostics[i].message, expectedWarnings[i].toUtf8());
                }
                diagnosticOk = true;
            });

    QTRY_VERIFY_WITH_TIMEOUT(diagnosticOk, 3000);
}

void tst_qmlls_modules::rangeFormatting_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QLspSpecification::Range>("selectedRange");
    QTest::addColumn<QLspSpecification::Range>("expectedRange");
    QTest::addColumn<QString>("expectedAfterFormat");

    const QString filePath = u"formatting/rangeFormatting.qml"_s;

    {
        QLspSpecification::Range selectedRange = { { 5, 0 }, { 9, 0 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 24, 0 } };
        QTest::addRow("selectRegion1") << filePath << selectedRange << expectedRange
                                       << u"formatting/rangeFormatting.formatted1.qml"_s;
    }

    {
        QLspSpecification::Range selectedRange = { { 10, 25 }, { 23, 0 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 24, 0 } };
        QTest::addRow("selectRegion2") << filePath << selectedRange << expectedRange
                                        << u"formatting/rangeFormatting.formatted2.qml"_s;
    }

    {
        QLspSpecification::Range selectedRange = { { 14, 36 }, { 14, 45 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 24, 0 } };
        QTest::addRow("selectSingleLine") << filePath << selectedRange << expectedRange
                                          << u"formatting/rangeFormatting.formatted3.qml"_s;
    }

    {
        QLspSpecification::Range selectedRange = { { 0, 0 }, { 24, 0 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 24, 0 } };
        QTest::addRow("selectEntireFile") << filePath << selectedRange << expectedRange
                                           << u"formatting/rangeFormatting.formatted4.qml"_s;
    }

    {
        QLspSpecification::Range selectedRange = { { 10, 3 }, { 20, 4 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 24, 0 } };
        QTest::addRow("selectUnbalanced") << filePath << selectedRange << expectedRange
                                          << u"formatting/rangeFormatting.formatted5.qml"_s;
    }

    {
        QLspSpecification::Range selectedRange = { { 0, 1 }, { 7, 40 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 7, 0 } };
        QTest::addRow("NoNewlineAtEnd")
                << u"formatting/WeirdNewlines.qml"_s << selectedRange << expectedRange
                << u"formatting/WeirdNewlines.formatted.qml"_s;
    }
    {
        QLspSpecification::Range selectedRange = { { 0, 1 }, { 3, 1 } };
        QLspSpecification::Range expectedRange = { { 0, 0 }, { 7, 0 } };
        QTest::addRow("WindowsNewlineAtEnd")
                << u"formatting/WeirdNewlines.qml"_s << selectedRange << expectedRange
#ifdef Q_OS_WIN
                << u"formatting/WeirdNewlines.formatted2.windows.qml"_s;
#else
                << u"formatting/WeirdNewlines.formatted2.qml"_s;
#endif
    }
}

void tst_qmlls_modules::rangeFormatting()
{
    QFETCH(QString, filePath);
    QFETCH(QLspSpecification::Range, selectedRange);
    QFETCH(QLspSpecification::Range, expectedRange);
    QFETCH(QString, expectedAfterFormat);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);

    QLspSpecification::DocumentRangeFormattingParams params;
    params.textDocument.uri = *uri;
    params.range = selectedRange;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto clean = [didFinish]() { *didFinish = true; };

    auto &&responseHandler = [&](auto res) {
        Q_UNUSED(res);
        QScopeGuard cleanup(clean);
        auto result = std::get_if<QList<TextEdit>>(&res);
        QVERIFY(result);

        QFile file(testFile(expectedAfterFormat));
        if (!file.open(QIODevice::ReadOnly))
            QFAIL("Error while opening the file ");

        const auto text = result->first();
        QCOMPARE(text.range.start.line, expectedRange.start.line);
        QCOMPARE(text.range.start.character, expectedRange.start.character);
        QCOMPARE(text.range.end.line, expectedRange.end.line);
        QCOMPARE(text.range.end.character, expectedRange.end.character);
        const QString fileContent = file.readAll();
        QCOMPARE(text.newText, fileContent);
    };

    auto &&errorHandler = [&clean](auto &error) {
        QScopeGuard cleanup(clean);
        ProtocolBase::defaultResponseErrorHandler(error);
        QVERIFY2(false, "error occurred while range formatting");
    };

    m_protocol->requestDocumentRangeFormatting(params, std::move(responseHandler),
                                               std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);
}

void tst_qmlls_modules::hover_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QLspSpecification::Position>("hoveredPosition");
    QTest::addColumn<QLspSpecification::MarkupContent>("expectedResult");

    const QString filePath = u"hover/test.qml"_s;
    {
        QLspSpecification::MarkupContent content{ MarkupKind::PlainText, "should fail" };
        QTest::addRow("hover") << filePath << QLspSpecification::Position{ 7, 24 } << content;
    }
}

void tst_qmlls_modules::hover()
{
    QFETCH(QString, filePath);
    QFETCH(QLspSpecification::Position, hoveredPosition);
    QFETCH(QLspSpecification::MarkupContent, expectedResult);

    ignoreDiagnostics();

    const auto uri = openFile(filePath);
    QVERIFY(uri);

    QLspSpecification::HoverParams params;
    params.textDocument.uri = *uri;
    params.position = hoveredPosition;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto clean = [didFinish]() { *didFinish = true; };

    auto &&responseHandler = [&](auto res) {
        QScopeGuard cleanup(clean);
        const auto *const result = std::get_if<QLspSpecification::Hover>(&res);
        QVERIFY(result);

        const auto *const markupContent =
                std::get_if<QLspSpecification::MarkupContent>(&result->contents);
        QVERIFY(markupContent);

        QEXPECT_FAIL("hover", "Should fail until we get the actual documentation for hovered items",
                     Continue);
        QCOMPARE(markupContent->value, expectedResult.value);
        QCOMPARE(markupContent->kind, expectedResult.kind);
    };

    auto &&errorHandler = [&clean](auto &error) {
        QScopeGuard cleanup(clean);
        ProtocolBase::defaultResponseErrorHandler(error);
        QVERIFY2(false, "error occurred on hovering");
    };

    m_protocol->requestHover(params, std::move(responseHandler), std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);
}

enum AddBuildDirOption : bool { AddBuildDir, DoNotAddBuildDir };

void tst_qmlls_modules::qmldirImports_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<AddBuildDirOption>("addBuildDirectory");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("expectedCompletion");

    QTest::addRow("fromBuildFolder")
            << u"completions/fromBuildDir.qml"_s << AddBuildDir << 3 << 1 << u"BuildDirType"_s;
    QTest::addRow("fromSourceFolder")
            << u"sourceDir/Main.qml"_s << DoNotAddBuildDir << 3 << 1 << u"Button"_s;
}

void tst_qmlls_modules::qmldirImports()
{
    QFETCH(QString, filePath);
    QFETCH(AddBuildDirOption, addBuildDirectory);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, expectedCompletion);

    const auto uri = openFile(filePath);
    QVERIFY(uri);

    if (addBuildDirectory == AddBuildDir) {
        Notifications::AddBuildDirsParams bDirs;
        UriToBuildDirs ub;
        ub.baseUri = *uri;
        ub.buildDirs.append(testFile("buildDir").toUtf8());
        bDirs.buildDirsToSet.append(ub);
        m_protocol->typedRpc()->sendNotification(QByteArray(Notifications::AddBuildDirsMethod), bDirs);
    }

    bool diagnosticOk = false;
    bool completionOk = false;
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [&diagnosticOk, &uri](const QByteArray &, const PublishDiagnosticsParams &p) {
                if (p.uri != *uri)
                    return;

                if constexpr (enable_debug_output) {
                    for (const auto &x : p.diagnostics) {
                        qDebug() << x.message;
                    }
                }
                QCOMPARE(p.diagnostics.size(), 0);
                diagnosticOk = true;
            });

    // Currently, the Dom is created twice in qmlls: once for the linting and once for all other
    // features. Therefore, also test that this second dom also uses the right resource files.
    CompletionParams cParams;
    cParams.position.line = line - 1; // LSP is 0 based
    cParams.position.character = character - 1; // LSP is 0 based
    cParams.textDocument.uri = *uri;

    m_protocol->requestCompletion(cParams, [&completionOk, &expectedCompletion](auto res) {
        const QList<CompletionItem> *cItems = std::get_if<QList<CompletionItem>>(&res);

        QSet<QString> labels;
        for (const CompletionItem &c : *cItems) {
            labels << c.label;
        }
        QVERIFY(labels.contains(expectedCompletion));
        completionOk = true;
    });

    QTRY_VERIFY_WITH_TIMEOUT(diagnosticOk && completionOk, 5000);
}

void tst_qmlls_modules::quickFixes_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<CodeActionParams>("codeActionParams");
    QTest::addColumn<int>("diagnosticIndex");
    QTest::addColumn<Range>("replacementRange");
    QTest::addColumn<QString>("replacementText");

    const QString filePath = u"quickfixes/INeedAQuickFix.qml"_s;

    QString fileContent;
    {
        QFile file(testFile(filePath));
        QVERIFY(file.open(QFile::Text | QFile::ReadOnly));
        fileContent = file.readAll();
    }

    CodeActionParams firstCodeAction;
    firstCodeAction.range = rangeFrom(fileContent, 10, 23, 1);
    firstCodeAction.textDocument.uri = testFileUrl(filePath).toEncoded();

    const Range firstRange = rangeFrom(fileContent, 10, 10, 0);
    const QString firstReplacement = u"(xxx) => "_s;

    QTest::addRow("injectedParameters")
            << filePath << firstCodeAction << 0 << firstRange << firstReplacement;

    CodeActionParams secondCodeAction;
    secondCodeAction.textDocument.uri = testFileUrl(filePath).toEncoded();
    secondCodeAction.range = rangeFrom(fileContent, 15, 20, 1);

    const Range secondRange = rangeFrom(fileContent, 15, 20, 0);
    const QString secondReplacement = u"hello."_s;

    QTest::addRow("parentProperty")
            << filePath << secondCodeAction << 1 << secondRange << secondReplacement;
}

std::tuple<int, int, int, int> rangeAsTuple(const Range &range)
{
    return std::make_tuple(range.start.line, range.start.character, range.end.line,
                           range.end.character);
}

void tst_qmlls_modules::quickFixes()
{
    QFETCH(QString, filePath);
    QFETCH(CodeActionParams, codeActionParams);
    // The index of the diagnostic that the quickFix belongs to.
    // diagnostics are sorted by their range (= text position in the current file).
    QFETCH(int, diagnosticIndex);
    QFETCH(Range, replacementRange);
    QFETCH(QString, replacementText);

    const auto uri = openFile(filePath);
    QVERIFY(uri);

    bool diagnosticOk = false;
    QList<Diagnostic> diagnostics;

    // run first the diagnostic that proposes a quickfix
    m_protocol->registerPublishDiagnosticsNotificationHandler(
            [&diagnosticOk, &uri, &diagnostics,
             &diagnosticIndex](const QByteArray &, const PublishDiagnosticsParams &p) {
                if (p.uri != *uri)
                    return;

                if constexpr (enable_debug_output) {
                    for (const auto &x : p.diagnostics) {
                        qDebug() << x.message;
                    }
                }
                QCOMPARE_GE(p.diagnostics.size(), diagnosticIndex);

                QList<Diagnostic> partially_sorted{ p.diagnostics };
                std::nth_element(partially_sorted.begin(),
                                 std::next(partially_sorted.begin(), diagnosticIndex),
                                 partially_sorted.end(),
                                 [](const Diagnostic &a, const Diagnostic &b) {
                                     return rangeAsTuple(a.range) < rangeAsTuple(b.range);
                                 });
                diagnostics.append(partially_sorted[diagnosticIndex]);

                diagnosticOk = true;
            });

    QTRY_VERIFY_WITH_TIMEOUT(diagnosticOk, 5000);

    codeActionParams.context.diagnostics = diagnostics;

    using InnerT = QList<std::variant<Command, CodeAction>>;
    using T = std::variant<InnerT, std::nullptr_t>;

    bool codeActionOk = false;

    // request a quickfix with the obtained diagnostic
    m_protocol->requestCodeAction(codeActionParams, [&](const T &result) {
        QVERIFY(std::holds_alternative<InnerT>(result));
        InnerT inner = std::get<InnerT>(result);
        QCOMPARE(inner.size(), 1);
        QVERIFY(std::holds_alternative<CodeAction>(inner.front()));
        CodeAction codeAction = std::get<CodeAction>(inner.front());

        QCOMPARE(codeAction.kind, "quickfix"); // everything else is ignored by QtC, VS Code, ...

        QVERIFY(codeAction.edit);
        QVERIFY(codeAction.edit->documentChanges);
        const auto &edits =  *codeAction.edit->documentChanges;
        QCOMPARE(edits.size(), 1);
        const auto& firstEdit = std::get<TextDocumentEdit>(edits.front());
        QCOMPARE(firstEdit.edits.size(), 1);
        QVERIFY(std::holds_alternative<TextEdit>(firstEdit.edits.front()));
        auto textEdit = std::get<TextEdit>(firstEdit.edits.front());

        // make sure that the quick fix does something
        QCOMPARE(textEdit.newText, replacementText);
        QCOMPARE(rangeAsTuple(textEdit.range), rangeAsTuple(replacementRange));

        codeActionOk = true;
    });

    QTRY_VERIFY_WITH_TIMEOUT(codeActionOk, 5000);
}

static QQmlJS::Dom::DomItem fileObject(const QString &filePath)
{
    QFile f(filePath);
    QQmlJS::Dom::DomItem file;
    if (!f.open(QIODevice::ReadOnly))
        return file;
    QString code = f.readAll();

    QStringList dirs = {QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath)};
    auto envPtr = QQmlJS::Dom::DomEnvironment::create(
            dirs,
            QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                    | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
            QQmlJS::Dom::Extended);
    envPtr->loadBuiltins();
    envPtr->loadFile(QQmlJS::Dom::FileToLoad::fromMemory(envPtr, filePath, code),
                        [&file](QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &, const QQmlJS::Dom::DomItem &newIt) {
                            file = newIt.fileObject();
                        });
    envPtr->loadPendingDependencies();
    return file;
};

void tst_qmlls_modules::semanticHighlightingFull_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addRow("bigfile") << u"highlighting/bigFile.qml"_s;
}

void tst_qmlls_modules::semanticHighlightingFull()
{
    QFETCH(QString, filePath);
    const auto item = fileObject(testFile(filePath));
    Highlights highlights;
    const auto expectedData = HighlightingUtils::collectTokens(item, std::nullopt);

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QLspSpecification::SemanticTokensParams params;
    params.textDocument.uri = *uri;
    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto cleanup = [didFinish]() { *didFinish = true; };

    auto &&responseHandler = [&](auto res) {
        QScopeGuard callAtExit(cleanup);
        const auto *const result = std::get_if<QLspSpecification::SemanticTokens>(&res);
        QVERIFY(result);
        QList<int> data = result->data;
        QCOMPARE(data.size(), expectedData.size());
        QCOMPARE(data, expectedData);
    };

    auto &&errorHandler = [&](auto &error) {
        QScopeGuard callAtExit(cleanup);
        ProtocolBase::defaultResponseErrorHandler(error);
        QVERIFY2(false, "error occurred on full semantic tokens");
    };

    m_protocol->requestSemanticTokens(params, std::move(responseHandler), std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);
}

void tst_qmlls_modules::semanticHighlightingRange_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<QLspSpecification::Range>("range");
    QTest::addRow("bigfile") << u"highlighting/bigFile.qml"_s
                             << QLspSpecification::Range{ { 6, 0 }, { 15, 0 } };
}

void tst_qmlls_modules::semanticHighlightingRange()
{
    QFETCH(QString, filePath);
    QFETCH(QLspSpecification::Range, range);

    const auto item = fileObject(testFile(filePath));
    Highlights highlights;
    const auto qmlFile = item.as<QQmlJS::Dom::QmlFile>();
    const auto code = qmlFile->code();
    const int startOffset = int(QQmlLSUtils::textOffsetFrom(code, range.start.line, range.end.character));
    const int endOffset = int(QQmlLSUtils::textOffsetFrom(code, range.end.line, range.end.character));
    const auto expectedData = HighlightingUtils::collectTokens(item, HighlightsRange{startOffset, endOffset});

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QLspSpecification::SemanticTokensRangeParams params;
    params.textDocument.uri = *uri;
    params.range = range;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto cleanup = [didFinish]() { *didFinish = true; };

    auto &&responseHandler = [&](auto res) {
        QScopeGuard callAtExit(cleanup);
        const auto *const result = std::get_if<QLspSpecification::SemanticTokens>(&res);
        QVERIFY(result);
        QList<int> data = result->data;
        QCOMPARE(data.size(), expectedData.size());
        QCOMPARE(data, expectedData);
    };

    auto &&errorHandler = [&](auto &error) {
        QScopeGuard callAtExit(cleanup);
        ProtocolBase::defaultResponseErrorHandler(error);
        QVERIFY2(false, "error occurred on full semantic tokens");
    };

    m_protocol->requestSemanticTokensRange(params, std::move(responseHandler),
                                           std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);
}

void tst_qmlls_modules::semanticHighlightingDelta_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addRow("basicDelta") << u"highlighting/basic.qml"_s;
}

void tst_qmlls_modules::semanticHighlightingDelta()
{
    QSKIP("This test should be skipped until QTBUG-124870 is fixed");
    QFETCH(QString, filePath);
    QFETCH(QString, deltaFilePath);

    const auto fileItem = fileObject(testFile(filePath));
    const auto deltaFileItem = fileObject(testFile(deltaFilePath));
    auto fullDocumentSemanticTokensData = HighlightingUtils::collectTokens(fileItem, std::nullopt);
    auto editedDocumentSemanticTokensData = HighlightingUtils::collectTokens(deltaFileItem, std::nullopt);
    const auto expectedEdits = HighlightingUtils::computeDiff(fullDocumentSemanticTokensData, editedDocumentSemanticTokensData);

    const auto uri = openFile(filePath);
    QVERIFY(uri);
    const auto deltaUri = openFile(deltaFilePath);
    QVERIFY(deltaUri);

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto cleanup = [didFinish]() { *didFinish = true; };

    QLspSpecification::SemanticTokensDeltaParams params;
    QLspSpecification::Responses::SemanticTokensDeltaResultType result;

    auto &&errorHandler = [&](auto &error) {
        QScopeGuard callAtExit(cleanup);
        ProtocolBase::defaultResponseErrorHandler(error);
        QVERIFY2(false, "error occurred on semantic tokens/delta");
    };

    QLspSpecification::SemanticTokensParams fullParams;
    fullParams.textDocument.uri = *uri;
    m_protocol->requestSemanticTokens(fullParams,
    [&](auto res) {
        QScopeGuard callAtExit(cleanup);
        if (auto r = std::get_if<QLspSpecification::SemanticTokens>(&res)) {
            params.previousResultId = r->resultId.value();
            fullDocumentSemanticTokensData = r->data;
        }
    }, errorHandler);
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);

    // Change the file
    DidChangeTextDocumentParams didChange;
    didChange.textDocument.uri = *uri;
    didChange.textDocument.version = 2;

    TextDocumentContentChangeEvent change;
    change.range = Range{ Position{ 8, 4 }, Position{ 8, 4 } };
    change.text = "const Patron = 42";

    didChange.contentChanges.append(change);
    m_protocol->notifyDidChangeTextDocument(didChange);

    *didFinish = false;
    params.textDocument.uri = *uri;
    m_protocol->requestSemanticTokensDelta(params,
    [&](auto res) {
        QScopeGuard callAtExit(cleanup);
        result = res;
    }, std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);

    if (const auto *const delta = std::get_if<QLspSpecification::SemanticTokensDelta>(&result)) {
        QVERIFY(delta);
        const auto data = delta->edits.front().data;
        const auto start = delta->edits.front().start;
        const auto deleteCount = delta->edits.front().deleteCount;
        QCOMPARE(start, expectedEdits.front().start);
        QCOMPARE(deleteCount, expectedEdits.front().deleteCount);
        QCOMPARE(data, expectedEdits.front().data);
    } else {
        const auto *const full = std::get_if<QLspSpecification::SemanticTokens>(&result);
        QVERIFY(full);
        QCOMPARE(full->data, expectedEdits.front().data);
    }
}

static bool compareRanges(const Range &lhs, const Range &rhs)
{
    return lhs.start.line == rhs.start.line && lhs.start.character == rhs.start.character
            && lhs.end.line == rhs.end.line && lhs.end.character == rhs.end.character;
}

static void debugPrint(const Range &range)
{
    qDebug() << '{' << range.start.line << ',' << range.start.character << '}' << ',' << '{'
             << range.end.line << ',' << range.end.character << '}';
}

static bool compareDocumentSymbols(const DocumentSymbol &current, const DocumentSymbol &expected)
{
    // For details about comparison see comment in documentSymbolsExpectedResult

    if (current.name.isEmpty() || current.kind == SymbolKind{})
        return false;

    if (current.tags != expected.tags || current.deprecated != expected.deprecated) {
        return false;
    }

    using namespace QQmlJS::Dom;
    QmlObject qmlObj;
    DomItem qmlObjectItem = DomItem(DomEnvironment::create({})).copy(&qmlObj);
    const auto qmlObjectKind = DocumentSymbolUtils::symbolKindOf(qmlObjectItem);
    if (current.kind == qmlObjectKind) {
        if (current.detail != expected.detail) {
            qDebug() << "Current " << current.detail << " Expected " << expected.detail;
            return false;
        }
    } else {
        if (expected.detail.has_value() != current.detail.has_value()) {
            qDebug() << current.name;
            return false;
        }
    }
    if (!compareRanges(current.range, expected.range)) {
        qDebug() << "Current Range: ";
        debugPrint(current.range);
        qDebug() << " Expected ";
        debugPrint(expected.range);
    }
    if (!compareRanges(current.selectionRange, expected.selectionRange)) {
        qDebug() << "Current Selection Range: ";
        debugPrint(current.selectionRange);
        qDebug() << " Expected ";
        debugPrint(expected.selectionRange);
    }
    // Compare children
    if (current.children.has_value() != expected.children.has_value()) {
        return false;
    }
    if (!current.children.has_value()) {
        return true;
    }
    return std::equal(current.children->cbegin(), current.children->cend(),
                      expected.children->cbegin(), expected.children->cend(),
                      compareDocumentSymbols);
}

static bool compareDocumentSymbolsLists(const QList<DocumentSymbol> &current,
                                        const QList<DocumentSymbol> &expected)
{
    return std::equal(current.cbegin(), current.cend(), expected.cbegin(), expected.cend(),
                      compareDocumentSymbols);
}

static QList<DocumentSymbol> documentSymbolsExpectedResult()
{
    // What essentially needs to be compared / verified are:
    // - ranges
    // - there is no duplication of symbols for properties bound at definition
    // - tags, deprecated
    // - other detail-s, name, kind are not empty or default when they need to be present*
    //
    //* The last one is a bit of a short-cut**, relying that
    // "correctness" of the assigned data is covered by unit-tests (tst_document_symbol_utils).
    // However it still needs to be tested, that the relevant data is indeed being
    // assigned when it should.
    // ** I find this short-cut fair, because the feature is not so critical

    // TODO(QTBUG-128277)

    using namespace QQmlJS::Dom;
    DocumentSymbol propDefA;
    propDefA.range = { { 4, 4 }, { 4, 18 } };
    propDefA.selectionRange = { { 4, 17 }, { 4, 18 } };

    // Simple bindings---------------------------------------------------
    DocumentSymbol bindingA;
    bindingA.range = { { 5, 4 }, { 5, 19 } };
    bindingA.selectionRange = { { 5, 4 }, { 5, 5 } };
    bindingA.detail = QByteArray{};

    DocumentSymbol bindingB;
    bindingB.range = bindingB.selectionRange = { { 6, 4 }, { 6, 22 } };
    bindingB.detail = QByteArray{};

    DocumentSymbol bindingC;
    bindingC.range = bindingC.selectionRange = { { 7, 4 }, { 7, 47 } };
    bindingC.detail = QByteArray{};

    // list<Rectangle> binding-------------------------------------------
    DocumentSymbol bindingColorRed;
    bindingColorRed.range = { { 11, 20 }, { 11, 32 } };
    bindingColorRed.selectionRange = { { 11, 20 }, { 11, 25 } };
    bindingColorRed.detail = QByteArray{};

    DocumentSymbol redRect;
    redRect.range = redRect.selectionRange = { { 11, 8 }, { 11, 34 } };
    redRect.children = { bindingColorRed };

    DocumentSymbol bindingColorBlue;
    bindingColorBlue.range = { { 12, 20 }, { 12, 33 } };
    bindingColorBlue.selectionRange = { { 12, 20 }, { 12, 25 } };
    bindingColorBlue.detail = QByteArray{};

    DocumentSymbol blueRect;
    blueRect.range = blueRect.selectionRange = { { 12, 8 }, { 12, 34 } };
    blueRect.children = { bindingColorBlue };

    DocumentSymbol bindingD;
    bindingD.range = bindingD.selectionRange = { { 10, 29 }, { 13, 5 } };
    bindingD.children = { redRect, blueRect };

    // Binding : Item{}-------------------------------------------------
    DocumentSymbol itemE;
    itemE.range = itemE.selectionRange = { { 14, 20 }, { 14, 26 } };
    DocumentSymbol bindingE;
    bindingE.range = { { 14, 17 }, { 14, 26 } };
    bindingE.selectionRange = { { 14, 17 }, { 14, 18 } };
    bindingE.children = { itemE };

    // Methods and Signals----------------------------------------------
    DocumentSymbol f;
    f.range = { { 17, 4 }, { 17, 18 } };
    f.selectionRange = { { 17, 13 }, { 17, 14 } };
    f.detail = QByteArray{};

    DocumentSymbol signalG;
    signalG.range = { { 18, 4 }, { 18, 20 } };
    signalG.selectionRange = { { 18, 11 }, { 18, 12 } };
    signalG.detail = QByteArray{};

    DocumentSymbol h;
    h.range = { { 19, 4 }, { 19, 39 } };
    h.selectionRange = { { 19, 13 }, { 19, 14 } };
    h.detail = QByteArray{};

    // Child obj & hierarchy----------------------------------------------
    DocumentSymbol inlCmpRootObj;
    inlCmpRootObj.detail = QString("root").toUtf8();
    inlCmpRootObj.range = { { 23, 26 }, { 23, 33 } };
    inlCmpRootObj.selectionRange = { { 23, 26 }, { 23, 30 } };

    DocumentSymbol inlCmp;
    inlCmp.range = { { 23, 8 }, { 23, 33 } };
    inlCmp.selectionRange = { { 23, 18 }, { 23, 24 } };
    inlCmp.children = { inlCmpRootObj };

    // Button and Enum----------------------------------------------
    DocumentSymbol enumMember;
    enumMember.range = enumMember.selectionRange = { { 27, 31 }, { 27, 36 } };
    enumMember.detail = QByteArray{};
    DocumentSymbol enumDecl;
    enumDecl.range = { { 27, 12 }, { 27, 38 } };
    enumDecl.selectionRange = { { 27, 17 }, { 27, 28 } };
    enumDecl.children = { enumMember };

    DocumentSymbol buttonId;
    // TODO(QTBUG-128277) Why there is no IdentifierRegion for this???
    buttonId.range = buttonId.selectionRange = { { 25, 12 }, { 25, 22 } };
    buttonId.detail = QByteArray{};

    DocumentSymbol button;
    button.detail = QString("button").toUtf8();
    button.range = { { 24, 8 }, { 28, 9 } };
    button.selectionRange = { { 24, 8 }, { 24, 14 } };
    button.children = { buttonId, enumDecl };

    DocumentSymbol childObj;
    childObj.range = { { 22, 4 }, { 29, 5 } };
    childObj.selectionRange = { { 22, 4 }, { 22, 13 } };
    childObj.children = {
        button, inlCmp
    }; // during rearrangement cmp is pushed down only after button is added

    DocumentSymbol rootItem;
    rootItem.detail = QString("root").toUtf8();
    rootItem.range = { { 2, 0 }, { 30, 1 } };
    rootItem.selectionRange = { { 2, 0 }, { 2, 4 } };
    rootItem.children = { propDefA, bindingA, bindingB, bindingC, bindingD,
                          bindingE, f,        signalG,  h,        childObj };

    return { rootItem };
}

void tst_qmlls_modules::documentSymbols()
{
    QString filePath("documentSymbol/DocumentSymbolTest.qml");
    const auto uri = openFile(filePath);
    QVERIFY(uri);
    QLspSpecification::DocumentSymbolParams params;
    params.textDocument.uri = *uri;

    std::shared_ptr<bool> didFinish = std::make_shared<bool>(false);
    const auto cleanup = [didFinish]() { *didFinish = true; };

    auto &&responseHandler = [&](auto res) {
        QScopeGuard callAtExit(cleanup);
        const auto result = std::get_if<QList<DocumentSymbol>>(&res);
        QVERIFY(result);
        auto expectedResult = documentSymbolsExpectedResult();
        compareDocumentSymbolsLists(*result, expectedResult);
    };

    auto &&errorHandler = [&](auto &error) {
        QScopeGuard callAtExit(cleanup);
        ProtocolBase::defaultResponseErrorHandler(error);
    };

    m_protocol->requestDocumentSymbol(params, std::move(responseHandler), std::move(errorHandler));
    QTRY_VERIFY_WITH_TIMEOUT(*didFinish, 10000);
}

QTEST_MAIN(tst_qmlls_modules)
