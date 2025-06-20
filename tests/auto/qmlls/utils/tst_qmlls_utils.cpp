// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_utils.h"
#include <algorithm>
#include <optional>

#include <QtCore/private/qduplicatetracker_p.h>
#include <QtQmlLS/private/qdochtmlparser_p.h>

// some helper constants for the tests
const static int positionAfterOneIndent = 5;
const static QString noResultExpected;
// constants for resultIndex
const static int firstResult = 0;
const static int secondResult = 1;
// constants for expectedItemsCount
const static int outOfOne = 1;
const static int outOfTwo = 2;

// enable/disable additional debug output
constexpr static bool enable_debug_output = true;

static QString printSet(const QSet<QString> &s)
{
    const QString r = QStringList(s.begin(), s.end()).join(u", "_s);
    return r;
}

static QString readFileContent(const QString &testFileName) {
        QFile file(testFileName);
        if (file.open(QIODeviceBase::ReadOnly))
            return QString::fromUtf8(file.readAll());
        return QString{};
};

std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>
tst_qmlls_utils::createEnvironmentAndLoadFile(const QString &filePath, const QStringList &extraBuildDir)
{
    CacheKey cacheKey = QDir::cleanPath(filePath + u"/.."_s);
    if (auto entry = cache.find(cacheKey); entry != cache.end()) {
        QQmlJS::Dom::DomItem env{ *entry };
        return { env, env.field(QQmlJS::Dom::Fields::qmlFileWithPath).key(filePath) };
    };

    const QStringList qmltypeDirs =
            QStringList({ dataDirectory(), QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath),
                          dataDirectory() + u"/sophisticatedBuildFolder"_s,
                        }) += extraBuildDir;

    auto envPtr = QQmlJS::Dom::DomEnvironment::create(
            qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded,
            QQmlJS::Dom::Extended);

    QQmlJS::Dom::DomItem file;
    QQmlJS::Dom::DomItem env(envPtr);
    envPtr->loadFile(QQmlJS::Dom::FileToLoad::fromFileSystem(envPtr, filePath),
                     [&file](QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &,
                             const QQmlJS::Dom::DomItem &newIt) { file = newIt; });

    envPtr->loadPendingDependencies();
    envPtr->loadBuiltins();

    cache[cacheKey] = envPtr;
    return std::make_tuple(env, file);
}

void tst_qmlls_utils::textOffsetRowColumnConversions_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<qsizetype>("expectedOffset");
    QTest::addColumn<QChar>("expectedChar");
    // in case they differ from line and character, e.g. when accessing non-existing line or rows
    // set to -1 when same as before
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    QTest::newRow("oneline") << u"Hello World!"_s << 0 << 6 << 6ll << QChar('W') << -1 << -1;
    QTest::newRow("multi-line") << u"Hello World!\n How are you? \n Bye!\n"_s << 0 << 6 << 6ll
                                << QChar('W') << -1 << -1;
    QTest::newRow("multi-line2") << u"Hello World!\n How are you? \n Bye!\n"_s << 1 << 5 << 18ll
                                 << QChar('a') << -1 << -1;
    QTest::newRow("multi-line3") << u"Hello World!\n How are you? \n Bye!\n"_s << 2 << 1 << 29ll
                                 << QChar('B') << -1 << -1;

    QTest::newRow("newlines") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 0 << 0 << 0ll << QChar('A') << -1
                              << -1;
    QTest::newRow("newlines2") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 0 << 2ll << QChar('B') << -1
                               << -1;

    // try to access '\r'
    QTest::newRow("newlines3") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 1 << 3ll << QChar('\r') << -1
                               << -1;
    // try to access '\n', should return the last character of the line (which is '\n' in this case)
    QTest::newRow("newlines4") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 2 << 4ll << QChar('\n') << -1
                               << -1;
    // try to access after the end of the line, should return the last character of the line (which
    // is '\n' in this case)
    QTest::newRow("afterLineEnd") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 42 << 4ll << QChar('\n')
                                  << -1 << 2;

    // try to access an inexisting column, seems to return the last character of the last line.
    QTest::newRow("afterColumnEnd")
            << u"A\nB\r\nC\n\r\nD\r\n\rAX"_s << 42 << 0 << 15ll << QChar('X') << 6 << 1;

    // \n\r are two newlines
    QTest::newRow("\\n\\r")
            << u"A\n\rB"_s << 2 << 0 << 3ll << QChar('B') << -1 << -1;
    // \r\n is a newline
    QTest::newRow("\\r\\n")
            << u"A\r\nB"_s << 1 << 0 << 3ll << QChar('B') << -1 << -1;

    QTest::newRow("windowsNewlineAfterColumnEnd")
            << u"A\r\nB\r\n"_s << 0 << 42 << 2ll << QChar('\n') << 0 << 2;
    QTest::newRow("windowsNewlineAfterLineEnd")
            << u"A\r\nB\r\n"_s << 42 << 0 << 5ll << QChar('\n') << 1 << 2;

    QTest::newRow("windowsNewlineR")
            << u"A\r\nB\r\n"_s << 0 << 1 << 1ll << QChar('\r') << 0 << 1;

}

void tst_qmlls_utils::textOffsetRowColumnConversions()
{
    QFETCH(QString, code);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(qsizetype, expectedOffset);
    QFETCH(QChar, expectedChar);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    qsizetype offset = QQmlLSUtils::textOffsetFrom(code, line, character);

    QCOMPARE(offset, expectedOffset);
    if (offset < code.size())
        QCOMPARE(code[offset], expectedChar);

    auto [computedRow, computedColumn] = QQmlLSUtils::textRowAndColumnFrom(code, expectedOffset);
    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    QCOMPARE(computedRow, expectedLine);
    QCOMPARE(computedColumn, expectedCharacter);
}

void tst_qmlls_utils::findItemFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // in case there are multiple items to be found (e.g. for a location between two objects), the
    // item to be checked against
    QTest::addColumn<int>("resultIndex");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<QQmlJS::Dom::DomType>("expectedType");
    // set to -1 when unchanged from above line and character, starts at 1
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);

    QTest::addRow("findIntProperty") << file1Qml << 9 << 18 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::PropertyDefinition
                                     // start of the "property"-token of the "a" property
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("findIntProperty2") << file1Qml << 9 << 10 << firstResult << outOfOne
                                      << QQmlJS::Dom::DomType::PropertyDefinition
                                      // start of the "property"-token of the "a" property
                                      << -1 << positionAfterOneIndent;
    QTest::addRow("findIntBinding")
            << file1Qml << 30 << positionAfterOneIndent << firstResult << outOfOne
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression
            // start of the a identifier of the "a" binding
            << -1 << positionAfterOneIndent;
    QTest::addRow("findIntBinding2") << file1Qml << 30 << 8 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::ScriptLiteral
                                     << -1 << 8;

    QTest::addRow("colorBinding") << file1Qml << 39 << 13 << firstResult << outOfOne
                                  << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1
                                  << 2 * positionAfterOneIndent - 1;

    QTest::addRow("findVarProperty") << file1Qml << 12 << 12 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::PropertyDefinition
                                     // start of the "property"-token of the "d" property
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("findVarBinding") << file1Qml << 31 << 8 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::ScriptLiteral << -1 << 8;
    QTest::addRow("beforeEProperty")
            << file1Qml << 13 << positionAfterOneIndent << firstResult << outOfOne
            << QQmlJS::Dom::DomType::PropertyDefinition
            // start of the "property"-token of the "e" property
            << -1 << -1;
    QTest::addRow("onEProperty") << file1Qml << 13 << 24 << firstResult << outOfOne
                                 << QQmlJS::Dom::DomType::PropertyDefinition
                                 // start of the "property"-token of the "e" property
                                 << -1 << positionAfterOneIndent;
    QTest::addRow("afterEProperty") << file1Qml << 13 << 25 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::PropertyDefinition
                                    // start of the "property"-token of the "e" property
                                    << -1 << positionAfterOneIndent;

    QTest::addRow("property-in-ic") << file1Qml << 28 << 38 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::PropertyDefinition << -1 << 26;

    QTest::addRow("onCChild") << file1Qml << 16 << positionAfterOneIndent << firstResult << outOfOne
                              << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1
                              << positionAfterOneIndent;

    // check for off-by-one/overlapping items
    QTest::addRow("closingBraceOfC")
            << file1Qml << 16 << 19 << firstResult << outOfOne << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;
    QTest::addRow("beforeClosingBraceOfC")
            << file1Qml << 16 << 18 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 12;
    QTest::addRow("firstBetweenCandD")
            << file1Qml << 16 << 20 << secondResult << outOfTwo << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;
    QTest::addRow("secondBetweenCandD")
            << file1Qml << 16 << 20 << firstResult << outOfTwo
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << -1;

    QTest::addRow("afterD") << file1Qml << 16 << 21 << firstResult << outOfOne
                            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 20;

    // check what happens between items (it should not crash)

    QTest::addRow("onWhitespaceBeforeC")
            << file1Qml << 16 << 1 << firstResult << outOfOne << QQmlJS::Dom::DomType::Map << 9
            << positionAfterOneIndent;

    QTest::addRow("onWhitespaceAfterC")
            << file1Qml << 17 << 8 << firstResult << outOfOne << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;

    QTest::addRow("onWhitespaceBetweenCAndD") << file1Qml << 17 << 23 << firstResult << outOfOne
                                              << QQmlJS::Dom::DomType::Map << 16 << 8;
    QTest::addRow("onWhitespaceBetweenCAndD2") << file1Qml << 17 << 24 << firstResult << outOfOne
                                               << QQmlJS::Dom::DomType::Map << 16 << 8;

    QTest::addRow("ic") << file1Qml << 15 << 5 << firstResult << outOfOne
                        << QQmlJS::Dom::DomType::QmlComponent << -1 << 5;
    QTest::addRow("ic2") << file1Qml << 15 << 20 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 18;
    QTest::addRow("ic3") << file1Qml << 15 << 33 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 29;

    QTest::addRow("function") << file1Qml << 33 << 5 << firstResult << outOfOne
                              << QQmlJS::Dom::DomType::MethodInfo << -1 << positionAfterOneIndent;
    QTest::addRow("function-parameter")
            << file1Qml << 33 << 20 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 19;
    // The return type of a function has no own DomItem. Instead, the return type of a function
    // is saved into the MethodInfo.
    QTest::addRow("function-return")
            << file1Qml << 33 << 41 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 41;
    QTest::addRow("function2") << file1Qml << 36 << 17 << firstResult << outOfOne
                               << QQmlJS::Dom::DomType::MethodInfo << -1
                               << positionAfterOneIndent;

    // check rectangle property
    QTest::addRow("rectangle-property")
            << file1Qml << 44 << 31 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::ScriptIdentifierExpression << -1 << 29;

    // check comment
    QTest::addRow("pre-comment")
            << file1Qml << 62 << 15 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::Comment << -1 << 5;
    QTest::addRow("post-comment")
            << file1Qml << 64 << 15 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::Comment << -1 << 5;
}

void tst_qmlls_utils::findItemFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QQmlJS::Dom::DomType, expectedType);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QVERIFY(resultIndex < locations.size());
    QCOMPARE(locations.size(), expectedItemsCount);

    QQmlJS::Dom::DomItem itemToTest = locations[resultIndex].domItem;
    // ask for the type in the args
    if constexpr (enable_debug_output) {
        if (itemToTest.internalKind() != expectedType) {
            qDebug() << itemToTest.internalKindStr() << " has not the expected kind "
                     << expectedType << " for item " << itemToTest.toString();
        }
    }
    QCOMPARE(itemToTest.internalKind(), expectedType);

    QQmlJS::Dom::FileLocations::Tree locationToTest = locations[resultIndex].fileLocation;
    QCOMPARE(locationToTest->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(locationToTest->info().fullRegion.startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findTypeDefinitionFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // in case there are multiple items to be found (e.g. for a location between two objects), the
    // item to be checked against
    QTest::addColumn<int>("resultIndex");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<QString>("expectedFilePath");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);
    const QString TypeQml = testFile(u"Type.qml"_s);
    // pass this as file when no result is expected, e.g. for type definition of "var".

    QTest::addRow("onCProperty") << file1Qml << 11 << 16 << firstResult << outOfOne << file1Qml << 7
                                 << positionAfterOneIndent;

    QTest::addRow("onCProperty2") << file1Qml << 28 << 37 << firstResult << outOfOne << file1Qml
                                  << 7 << positionAfterOneIndent;

    QTest::addRow("onCProperty3") << file1Qml << 28 << 35 << firstResult << outOfOne << file1Qml
                                  << 7 << positionAfterOneIndent;

    QTest::addRow("onCBinding") << file1Qml << 46 << 8 << firstResult << outOfOne << file1Qml << 7
                                << positionAfterOneIndent;

    QTest::addRow("onDefaultBinding") << file1Qml << 16 << positionAfterOneIndent << firstResult
                                      << outOfOne << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onDefaultBindingId")
            << file1Qml << 16 << 28 << firstResult << outOfOne << file1Qml << 16 << 20;

    QTest::addRow("findIntProperty") << file1Qml << 9 << 18 << firstResult << outOfOne << file1Qml
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("colorBinding") << file1Qml << 39 << 8 << firstResult << outOfOne << file1Qml
                                  << -1 << positionAfterOneIndent;

    // check what happens between items (it should not crash)

    QTest::addRow("onWhitespaceBeforeC")
            << file1Qml << 16 << 1 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceAfterC")
            << file1Qml << 17 << 23 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceBetweenCAndD")
            << file1Qml << 17 << 24 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("ic") << file1Qml << 15 << 15 << firstResult << outOfOne << file1Qml << 15 << 15;
    QTest::addRow("icBase") << file1Qml << 15 << 20 << firstResult << outOfOne
                            << u"TODO: file location for C++ defined types?"_s << -1 << -1;
    QTest::addRow("ic3") << file1Qml << 15 << 33 << firstResult << outOfOne << file1Qml << -1 << 18;

    // TODO: type definition of function = type definition of return type?
    // if not, this might need fixing:
    // currently, asking the type definition of the "function" keyword returns
    // the type definitin of the return type (when available).
    QTest::addRow("function-keyword") << file1Qml << 33 << 5 << firstResult << outOfOne << file1Qml
                                      << 7 << positionAfterOneIndent;
    QTest::addRow("function-parameter-builtin")
            << file1Qml << 33 << 20 << firstResult << outOfOne << file1Qml << -1 << -1;
    QTest::addRow("function-parameter-item") << file1Qml << 33 << 36 << firstResult << outOfOne
                                             << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("function-return") << file1Qml << 33 << 41 << firstResult << outOfOne << file1Qml
                                     << 7 << positionAfterOneIndent;

    QTest::addRow("void-function")
            << file1Qml << 36 << 17 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("rectangle-property") << file1Qml << 44 << 31 << firstResult << outOfOne
                                        << "TODO: c++ type location" << -1 << -1;

    QTest::addRow("functionParameterICUsage")
            << file1Qml << 34 << 16 << firstResult << outOfOne << file1Qml << 7 << 15;

    QTest::addRow("ICBindingUsage")
            << file1Qml << 47 << 21 << firstResult << outOfOne << file1Qml << 7 << 15;
    QTest::addRow("ICBindingUsage2")
            << file1Qml << 49 << 11 << firstResult << outOfOne << file1Qml << 7 << 15;
    QTest::addRow("ICBindingUsage3")
            << file1Qml << 52 << 17 << firstResult << outOfOne << file1Qml << 7 << 15;
    QTest::addRow("ICBindingUsageFromLambda")
            << file1Qml << 58 << 23 << firstResult << outOfOne << file1Qml << 7 << 15;
}

void tst_qmlls_utils::findTypeDefinitionFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QString, expectedFilePath);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), expectedItemsCount);

    auto base = QQmlLSUtils::findTypeDefinitionOf(locations[resultIndex].domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QVERIFY(!base);
        return;
    }

    QEXPECT_FAIL("findIntProperty", "Builtins not supported yet", Abort);
    QEXPECT_FAIL("function-parameter-builtin", "Base types defined in C++ are not supported yet",
                 Abort);
    QEXPECT_FAIL("colorBinding", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("rectangle-property", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("icBase", "Base types defined in C++ are not supported yet", Abort);
    QVERIFY(base);

    auto fileObject =
            locations[resultIndex].domItem.goToFile(base->filename()).as<QQmlJS::Dom::QmlFile>();

    // print some debug message when failing, instead of using QVERIFY2
    // (printing the type every time takes a lot of time).
    if constexpr (enable_debug_output) {
        if (!fileObject)
            qDebug() << "Could not find the file" << base->filename() << "in the Dom.";
    }

    QVERIFY(fileObject);
    QCOMPARE(base->filename(), expectedFilePath);
    QCOMPARE(fileObject->canonicalFilePath(), expectedFilePath);

    QCOMPARE(base->sourceLocation().startLine, quint32(expectedLine));
    QCOMPARE(base->sourceLocation().startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findLocationOfItem_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);

    QTest::addRow("root-element") << file1Qml << 6 << 2 << -1 << 1;

    QTest::addRow("property-a") << file1Qml << 9 << 18 << -1 << positionAfterOneIndent;
    QTest::addRow("property-a2") << file1Qml << 9 << 10 << -1 << positionAfterOneIndent;
    QTest::addRow("nested-C") << file1Qml << 20 << 9 << -1 << -1;
    QTest::addRow("nested-C2") << file1Qml << 23 << 13 << -1 << -1;
    QTest::addRow("D") << file1Qml << 17 << 33 << -1 << 32;
    QTest::addRow("property-d-var-type") << file1Qml << 12 << 15 << -1 << 14;

    QTest::addRow("import") << file1Qml << 4 << 6 << -1 << 1;
}

void tst_qmlls_utils::findLocationOfItem()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);
    QCOMPARE(locations.size(), 1);

    // once the item is grabbed, make sure its line/character position can be obtained back
    auto t = QQmlJS::Dom::FileLocations::treeOf(locations.front().domItem);

    QCOMPARE(t->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(t->info().fullRegion.startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findBaseObject_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // to avoid mixing up the types (because they are all called Item or QQuickItem eitherway)
    // mark them with properties and detect right types by their marker property,
    // usually called (in<Filename>DotQml or in<Inline component Name>)
    QTest::addColumn<QSet<QString>>("expectedPropertyName");
    // because types inherit properties, make sure that derived type properties are not in the base
    // type, to correctly detect mixups between types and their base types
    QTest::addColumn<QSet<QString>>("unExpectedPropertyName");

    // (non) Expected Properties Names = ePN (nEPN)
    // marker properties for the root object in BaseType.qml
    QSet<QString> ePNBaseType;
    ePNBaseType << u"inBaseTypeDotQml"_s;
    QSet<QString> nEPNBaseType;
    nEPNBaseType << u"inTypeDotQml"_s;

    // marker properties for the root object in Type.qml
    QSet<QString> ePNType;
    ePNType << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s;
    QSet<QString> nEPNType;

    // marker properties for QQuickItem (e.g. the base of "Item")
    QSet<QString> ePNQQuickItem;
    QSet<QString> nEPNQQuickItem;
    nEPNQQuickItem << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s;

    // marker properties for MyInlineComponent
    QSet<QString> ePNMyInlineComponent;
    QSet<QString> nEPNMyInlineComponent;
    ePNMyInlineComponent << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s << u"inMyInlineComponent"_s;

    // marker properties for MyNestedInlineComponent
    const QSet<QString> ePNMyNestedInlineComponent{ u"inMyNestedInlineComponent"_s };
    const QSet<QString> nEPNMyNestedInlineComponent{ u"inBaseTypeDotQml"_s, u"inTypeDotQml"_s,
                                                     u"inMyInlineComponent"_s };

    // marker properties for MyBaseInlineComponent
    const QSet<QString> ePNMyBaseInlineComponent{ u"inBaseTypeDotQml"_s };
    const QSet<QString> nEPNMyBaseInlineComponent{ u"inTypeDotQml"_s, u"inMyInlineComponent"_s };

    const int rootElementDefLine = 6;
    QTest::addRow("root-element") << testFile(u"Type.qml"_s) << rootElementDefLine << 5
                                  << ePNQQuickItem << nEPNQQuickItem;
    QTest::addRow("root-element-from-id") << testFile(u"Type.qml"_s) << rootElementDefLine + 1 << 12
                                          << ePNBaseType << nEPNBaseType;

    const int myInlineComponentDefLine = 10;
    // on the component name: go to BaseType
    QTest::addRow("ic-name") << testFile(u"Type.qml"_s) << myInlineComponentDefLine << 26
                             << ePNBaseType << nEPNBaseType;
    // on the "BaseType" type: go to QQuickitem (base type of BaseType).
    QTest::addRow("ic-basetypename") << testFile(u"Type.qml"_s) << myInlineComponentDefLine << 37
                                     << ePNQQuickItem << nEPNQQuickItem;
    QTest::addRow("ic-from-id") << testFile(u"Type.qml"_s) << myInlineComponentDefLine + 1 << 19
                                << ePNBaseType << nEPNBaseType;

    const int inlineTypeDefLine = 15;
    QTest::addRow("inline") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 23 << ePNQQuickItem
                            << nEPNQQuickItem;
    QTest::addRow("inline2") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 38 << ePNQQuickItem
                             << nEPNQQuickItem;
    QTest::addRow("inline3") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 15 << ePNQQuickItem
                             << nEPNQQuickItem;
    QTest::addRow("inline-from-id") << testFile(u"Type.qml"_s) << inlineTypeDefLine + 1 << 24
                                    << ePNBaseType << nEPNBaseType;

    const int inlineIcDefLine = 23;
    QTest::addRow("inline-ic") << testFile(u"Type.qml"_s) << inlineIcDefLine << 38
                               << ePNMyBaseInlineComponent << nEPNMyBaseInlineComponent;
    QTest::addRow("inline-ic-from-id") << testFile(u"Type.qml"_s) << inlineIcDefLine + 1 << 28
                                       << ePNMyBaseInlineComponent << nEPNMyBaseInlineComponent;

    const int inlineNestedIcDefLine = 27;
    QTest::addRow("inline-ic2") << testFile(u"Type.qml"_s) << inlineNestedIcDefLine << 46
                                << ePNMyNestedInlineComponent << nEPNMyNestedInlineComponent;
    QTest::addRow("inline-ic2-from-id")
            << testFile(u"Type.qml"_s) << inlineNestedIcDefLine + 1 << 23
            << ePNMyNestedInlineComponent << nEPNMyNestedInlineComponent;
}

void tst_qmlls_utils::findBaseObject()
{
    const QByteArray failOnInlineComponentsMessage =
            "The Dom cannot resolve inline components from the basetype yet.";

    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QSet<QString>, expectedPropertyName);
    QFETCH(QSet<QString>, unExpectedPropertyName);

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);
    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    auto typeLocation = QQmlLSUtils::findTypeDefinitionOf(locations.front().domItem);
    QEXPECT_FAIL("inline-ic", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2", failOnInlineComponentsMessage, Abort);
    QVERIFY(typeLocation);
    QQmlJS::Dom::DomItem type = QQmlLSUtils::sourceLocationToDomItem(
            locations.front().domItem.goToFile(typeLocation->filename()),
            typeLocation->sourceLocation());
    auto base = QQmlLSUtils::baseObject(type);

    if constexpr (enable_debug_output) {
        if (!base)
            qDebug() << u"Could not find the base of type "_s << type << u" from item:\n"_s
                     << locations.front().domItem.toString();
    }

    QEXPECT_FAIL("inline-ic-from-id", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2-from-id", failOnInlineComponentsMessage, Abort);
    QVERIFY(base);

    const QSet<QString> propertyDefs = base.field(QQmlJS::Dom::Fields::propertyDefs).keys();
    expectedPropertyName.subtract(propertyDefs);
    QVERIFY2(expectedPropertyName.empty(),
             u"Incorrect baseType found: it is missing following marker properties: "_s
                     .append(printSet(expectedPropertyName))
                     .toLatin1());
    unExpectedPropertyName.intersect(propertyDefs);
    QVERIFY2(unExpectedPropertyName.empty(),
             u"Incorrect baseType found: it has an unexpected marker properties: "_s
                     .append(printSet(unExpectedPropertyName))
                     .toLatin1());
}

/*! \internal
    \brief Wrapper for findUsages data.
*/
struct UsageData
{
    QString testFileName;
    QQmlLSUtils::Usages expectedUsages;
};

void tst_qmlls_utils::findUsages_data()
{
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<UsageData>("data");

    const auto readFileContent = [](const QString &testFileName) {
        QFile file(testFileName);
        if (file.open(QIODeviceBase::ReadOnly))
            return QString::fromUtf8(file.readAll());
        return QString{};
    };

    const auto makeUsages = [](const QString &fileName, QList<QQmlLSUtils::Location> &locations) {
        UsageData data;
        std::sort(locations.begin(), locations.end());
        data.expectedUsages = { locations, {} };
        data.testFileName = fileName;
        return data;
    };

    {
        QList<QQmlLSUtils::Location> expectedUsages;
        const auto testFileName = testFile("findUsages/jsIdentifier/jsIdentifier.qml");
        const auto testFileContent = readFileContent(testFileName);
        {
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 13,
                                                          strlen("sum"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 13,
                                                          strlen("sum"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 19,
                                                          strlen("sum"));
            const auto sumUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findSumFromDeclaration") <<  8 << 13 << sumUsages;
            QTest::addRow("findSumFromUsage") << 10 << 20 << sumUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 17,
                                                          strlen("i"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 24,
                                                          strlen("i"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 32,
                                                          strlen("i"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 36,
                                                          strlen("i"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 25,
                                                          strlen("i"));
            const auto iUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findIFromDeclaration") << 9 << 17 << iUsages;
            QTest::addRow("findIFromUsage") << 9 << 24 << iUsages;
            QTest::addRow("findIFromUsage2") <<  10 << 25 << iUsages;
        }
    }
    {
        const auto testFileName = testFile("findUsages/property/property.qml");
        const auto otherFile = testFile("findUsages/property/PropertyFromAnotherFile.qml");
        const auto testFileContent = readFileContent(testFileName);
        const auto otherFileContent = readFileContent(otherFile);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 18,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 13,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 29,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 20, 9,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 21, 9,
                                                          strlen("helloPropertyChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 5,
                                                          strlen("onHelloPropertyChanged"));
            const auto helloPropertyUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findPropertyFromDeclaration") << 8 << 18 << helloPropertyUsages;
            QTest::addRow("findPropertyFromUsage") << 13 << 13 << helloPropertyUsages;
            QTest::addRow("findPropertyFromUsage2") << 13 << 29 << helloPropertyUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 36, 20,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 38, 25,
                                                          strlen("helloProperty"));
            const auto subItemHelloPropertyUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findPropertyFromDeclarationInSubItem") << 38 << 25 << subItemHelloPropertyUsages;
            QTest::addRow("findPropertyFromUsageInSubItem") << 36 << 20 << subItemHelloPropertyUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 22,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 29, 20,
                                                          strlen("helloProperty"));
            const auto ICHelloPropertyUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findPropertyFromDeclarationInIC") << 27 << 22 << ICHelloPropertyUsages;
            QTest::addRow("findPropertyFromUsageInIC") << 29 << 20 << ICHelloPropertyUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(otherFile, otherFileContent, 4, 18,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 42, 9,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 44, 20,
                                                          strlen("helloProperty"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 46, 9,
                                                          strlen("OnHelloPropertyChanged"));
            const auto helloPropertyUsages = makeUsages(testFileName, expectedUsages);

            QTest::addRow("findPropertyFromOtherFile") << 42 << 13 << helloPropertyUsages;
        }
    }
    {
        const auto testFileName = testFile("findUsages/propertyInNested/propertyInNested.qml");
        const auto testFileContent = readFileContent(testFileName);

        const auto componentFileName =
                testFile("findUsages/propertyInNested/NestedComponentInFile.qml");
        const auto componentFileContent = readFileContent(componentFileName);

        const auto componentFileName3 =
                testFile("findUsages/propertyInNested/NestedComponentInFile3.qml");
        const auto componentFileContent3 = readFileContent(componentFileName);

        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 7, 18,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 31,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 34, 37,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 35, 43,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 36, 49,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 42, 26,
                                                          strlen("p2"));
            const auto p2Usages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findPropertyFromDeclaration2") <<  7 << 18 << p2Usages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 29, 13,
                                                          strlen("myNested"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 32, 17,
                                                          strlen("myNested"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 17,
                                                          strlen("myNested"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 34, 17,
                                                          strlen("myNested"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 35, 17,
                                                          strlen("myNested"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 36, 17,
                                                          strlen("myNested"));
            const auto nestedUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findIdFromUsage") <<  36 << 20 << nestedUsages;
            QTest::addRow("findIdFromDefinition") << 29 << 17 << nestedUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 35,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 32, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 35, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 36, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 16, 9,
                                                          strlen("inner"));
            const auto nestedComponent3Usages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findPropertyFromUsageInFieldMemberExpression")
                    << 36 << 34 << nestedComponent3Usages;

            QTest::addRow("findFieldMemberExpressionUsageFromPropertyDefinition")
                    << 14 << 38 << nestedComponent3Usages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(componentFileName, componentFileContent,
                                                          4, 37, strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 50, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 52, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 53, 32,
                                                          strlen("inner"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 54, 32,
                                                          strlen("inner"));
            const auto nestedComponent3Usages = makeUsages(testFileName, expectedUsages);
            const auto nestedComponent3UsagesFromOtherFile =
                    makeUsages(componentFileName, expectedUsages);
            QTest::addRow("findPropertyFromUsageInFieldMemberExpressionFromOtherFile")
                    << 50 << 33 << nestedComponent3Usages;

            QTest::addRow("findFieldMemberExpressionUsageFromPropertyDefinitionFromOtherFile")
                    << 4 << 38 << nestedComponent3UsagesFromOtherFile;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 35, 38,
                                                          strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 20, 22,
                                                          strlen("p2"));
            const auto nestedComponent3P2Usages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findProperty2FromUsageInFieldMemberExpression")
                    << 35 << 39 << nestedComponent3P2Usages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(componentFileName3, componentFileContent3,
                                                          5, 18, strlen("p2"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 53, 44,
                                                          strlen("p2"));
            const auto nestedComponent3P2Usages = makeUsages(testFileName, expectedUsages);
            const auto nestedComponent3P2UsagesFromOtherFile = makeUsages(componentFileName3, expectedUsages);
            QTest::addRow("findProperty2FromUsageInFieldMemberExpressionInOtherFile")
                    << 53 << 44 << nestedComponent3P2Usages;
            QTest::addRow("findProperty2FromUsageInDefinitionInOtherFile")
                    << 5 << 19 << nestedComponent3P2UsagesFromOtherFile;
        }
    }
    {
        QList<QQmlLSUtils::Location> expectedUsages;
        const auto testFileName = testFile("findUsages/idUsages/idUsages.qml");
        const auto testFileContent = readFileContent(testFileName);
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 7, 9,
                                                      strlen("rootId"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 11, 17,
                                                      strlen("rootId"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 12, 20,
                                                      strlen("rootId"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 17, 9,
                                                      strlen("rootId"));
        const auto rootIdUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("findIdFromUsageInChild") << 12 << 20 << rootIdUsages;
    }
    {
        QList<QQmlLSUtils::Location> expectedUsages;
        const auto testFileName = testFile("findUsages/recursive/recursive.qml");
        const auto testFileContent = readFileContent(testFileName);
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 14,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 24,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 34,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 51,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 68,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 12, 20,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 15, 34,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 19, 27,
                                                      strlen("recursive"));
        const auto recursiveUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("findFunctionUsage") << 10 << 30 << recursiveUsages;
        QTest::addRow("findFunctionUsage2") << 12 << 24 << recursiveUsages;
        QTest::addRow("findQualifiedFunctionUsage") << 19 << 31 << recursiveUsages;
        QTest::addRow("findFunctionUsageFromDefinition") << 8 << 17 << recursiveUsages;
    }
    {
        QList<QQmlLSUtils::Location> expectedUsages;
        const auto testFileName = testFile("findUsages/recursive/recursive.qml");
        const auto testFileContent = readFileContent(testFileName);
        const auto otherFileName = testFile("findUsages/recursive/RecursiveInOtherFile.qml");
        const auto otherFileContent = readFileContent(otherFileName);

        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 61,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 4, 14,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 6, 24,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 6, 34,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 6, 51,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 6, 68,
                                                      strlen("recursive"));
        expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 8, 20,
                                                      strlen("recursive"));

        const auto recursiveUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("findFunctionUsageFromOtherFile") << 27 << 64 << recursiveUsages;
        const auto recursiveUsagesFromOtherFile = makeUsages(otherFileName, expectedUsages);
        QTest::addRow("findFunctionUsageFromSameFile") << 6 << 39 << recursiveUsagesFromOtherFile;
        QTest::addRow("findFunctionUsageFromDefinitionInOtherFile")
                << 4 << 14 << recursiveUsagesFromOtherFile;
    }
    {
        const auto testFileName = testFile("findUsages/signalsAndHandlers/signalsAndHandlers.qml");
        const auto testFileContent = readFileContent(testFileName);

        const auto otherFileName = testFile("findUsages/signalsAndHandlers/widthChangedInAnotherFile.qml");
        const auto otherFileContent = readFileContent(otherFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 12,
                                                          strlen("helloSignal"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 11, 9,
                                                          strlen("helloSignal"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 13,
                                                          strlen("helloSignal"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 17, 17,
                                                          strlen("helloSignal"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 21, 9,
                                                          strlen("helloSignal"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 39, 5,
                                                          strlen("onHelloSignal"));
            const auto helloSignalUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findQmlSignalUsageFromDefinition") << 8 << 17 << helloSignalUsages;
            QTest::addRow("findQmlSignalUsageFromUsage") << 13 << 17 << helloSignalUsages;
            QTest::addRow("findQmlSignalUsageFromHandler") << 39 << 11 << helloSignalUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 5, 5,
                                                          strlen("onWidthChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 13,
                                                          strlen("widthChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 17,
                                                          strlen("widthChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 28, 20,
                                                          strlen("widthChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 34, 20,
                                                          strlen("widthChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 13,
                                                          strlen("widthChanged"));
            const auto widthChangedUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findCppSignalUsageFromUsage") << 27 << 23 << widthChangedUsages;
            QTest::addRow("findCppSignalUsageFromQualifiedUsage") << 28 << 23 << widthChangedUsages;
            QTest::addRow("findCppSignalUsageFromQualifiedUsage2") << 34 << 24 << widthChangedUsages;
        }
    }
    {
        const auto testFileName = testFile("findUsages/binding/binding.qml");
        const auto testFileContent = readFileContent(testFileName);
        QList<QQmlLSUtils::Location> expectedUsages;
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 18,
                                                      strlen("helloPropertyBinding"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 5,
                                                      strlen("helloPropertyBinding"));
        const auto helloPropertyBindingUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("findBindingUsagesFromDefinition") << 9 << 21 << helloPropertyBindingUsages;
        QTest::addRow("findBindingUsagesFromBinding") << 10 << 19 << helloPropertyBindingUsages;
    }
    {
        const auto testFileName = testFile("findUsages/signalsAndHandlers/signalAndHandlers2.qml");
        const auto testFileContent = readFileContent(testFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 7, 14,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 20,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 29,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 15, 24,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 17,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 24, 24,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 25, 21,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 19,
                                                          strlen("myHelloHandler"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 42, 29,
                                                          strlen("myHelloHandler"));
            const auto myHelloHandlerUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findJSMethodFromUsageInBinding") << 8 << 27 << myHelloHandlerUsages;
            QTest::addRow("findJSMethodFromDefinition") << 7 << 22 << myHelloHandlerUsages;
            QTest::addRow("findJSMethodFromDefinition2") << 7 << 9 << myHelloHandlerUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 18,
                                                          strlen("checkHandlers"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 5,
                                                          strlen("onCheckHandlersChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 17, 9,
                                                          strlen("checkHandlersChanged"));
            const auto checkHandlersUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findQmlPropertyHandlerFromDefinition") << 13 << 18 << checkHandlersUsages;
            QTest::addRow("findQmlPropertyHandlerFromHandler") <<  14 << 5 << checkHandlersUsages;
            QTest::addRow("findQmlPropertyHandlerFromSignalCall") << 17 << 9 << checkHandlersUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 15, 5,
                                                          strlen("onChildrenChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 18, 9,
                                                          strlen("childrenChanged"));
            const auto checkCppHandlersUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findCppPropertyHandlerFromHandler") << 15 << 5 << checkCppHandlersUsages;
            QTest::addRow("findCppPropertyHandlerFromSignalCall") << 18 << 9 << checkCppHandlersUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 20, 18,
                                                          strlen("_"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 5,
                                                          strlen("on_Changed"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 9,
                                                          strlen("_Changed"));
            const auto checkHandlersUsages2 = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findQmlPropertyHandler2FromDefinition") << 20 << 18 << checkHandlersUsages2;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 21, 18,
                                                          strlen("______42"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 24, 5,
                                                          strlen("on______42Changed"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 28, 9,
                                                          strlen("______42Changed"));
            const auto checkHandlersUsages3 = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findQmlPropertyHandler3FromDefinition") << 21 << 18 << checkHandlersUsages3;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 22, 18,
                                                          strlen("_123a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 25, 5,
                                                          strlen("on_123AChanged"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 29, 9,
                                                          strlen("_123aChanged"));
            const auto checkHandlersUsages4 = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findQmlPropertyHandler4FromDefinition") << 22 << 18 << checkHandlersUsages4;
        }
    }
    {
        QList<QQmlLSUtils::Location> expectedUsages;
        const auto testFileName = testFile("findUsages/connections/connections.qml");
        const auto testFileContent = readFileContent(testFileName);
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 9,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 17, 23,
                                                      strlen("clicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 15,
                                                      strlen("clicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 16, 22,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 34, 15,
                                                      strlen("clicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 18, 23,
                                                      strlen("clicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 28, 9,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 35, 15,
                                                      strlen("clicked"));
        const auto signalInConnection = makeUsages(testFileName, expectedUsages);
        QTest::addRow("findSignalsInConnectionFromSignal") << 33 << 15 << signalInConnection;
        QTest::addRow("findSignalsInConnectionFromHandler") << 9 << 9 << signalInConnection;
        QTest::addRow("findSignalsInConnectionFromFunction") << 16 << 22 << signalInConnection;
    }
    {
        const auto testFileName =
                testFile("findUsages/parametersAndDeconstruction/parametersAndDeconstruction.qml");
        const auto testFileContent = readFileContent(testFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 30,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 16,
                                                          strlen("a"));
            const auto aParamUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findMethodParameterA") << 9 << 16 << aParamUsages;
            QTest::addRow("findMethodParameterAFromUsage") << 8 << 30 << aParamUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 50,
                                                          strlen("x"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 28,
                                                          strlen("x"));
            const auto xParamUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findMethodParameterXDeconstructed") << 8 << 50 << xParamUsages;
            QTest::addRow("findMethodParameterXDeconstructedFromUsage") << 9 << 28 << xParamUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 53,
                                                          strlen("y"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 32,
                                                          strlen("y"));
            const auto yParamUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findMethodParameterYDeconstructed") << 8 << 53 << yParamUsages;
            QTest::addRow("findMethodParameterYDeconstructedFromUsage") << 9 << 32 << yParamUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 59,
                                                          strlen("z"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 36,
                                                          strlen("z"));
            const auto zParamUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findMethodParameterZDeconstructed") << 8 << 59 << zParamUsages;
            QTest::addRow("findMethodParameterZDeconstructedFromUsage") << 9 << 36 << zParamUsages;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 14,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 17,
                                                          strlen("a"));
            const auto deconstructedAUsages = makeUsages(testFileName, expectedUsages);
            QTest::addRow("deconstructed") << 14 << 17 << deconstructedAUsages;
            QTest::addRow("deconstructedFromDefinition") << 13 << 14 << deconstructedAUsages;
        }
    }
    {
        const auto testFileName = testFile("findUsages/groupPropertyUsage/groupPropertyUsage.qml");
        const auto testFileContent = readFileContent(testFileName);
        const auto otherFileName = testFile("findUsages/groupPropertyUsage/fontFamilyUsage.qml");
        const auto otherFileContent = readFileContent(otherFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 5, 34,
                                                          strlen("family"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 17,
                                                          strlen("family"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 35,
                                                          strlen("family"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 10,
                                                          strlen("family"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 48,
                                                          strlen("family"));
            const auto groupPropertyUsages1 = makeUsages(testFileName, expectedUsages);
            QTest::addRow("groupPropertyUsages1") << 14 << 17 << groupPropertyUsages1;
            const auto groupPropertyUsages1FromOtherFile =
                    makeUsages(otherFileName, expectedUsages);
            QTest::addRow("groupPropertyUsages1FromOtherFile")
                    << 5 << 37 << groupPropertyUsages1FromOtherFile;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 5,
                                                          strlen("font"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 24, 5,
                                                          strlen("font"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 12, 13,
                                                          strlen("font"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 30,
                                                          strlen("font"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 32, 41,
                                                          strlen("font"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 33, 43,
                                                          strlen("font"));
            const auto groupPropertyUsages2 = makeUsages(testFileName, expectedUsages);
            QTest::addRow("groupPropertyUsages2") << 23 << 5 << groupPropertyUsages2;
        }
    }
    {
        const auto testFileName =
                testFile("findUsages/attachedPropertyUsage/attachedPropertyUsage.qml");
        const auto testFileContent = readFileContent(testFileName);
        QList<QQmlLSUtils::Location> expectedUsages;
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 5,
                                                      strlen("Keys"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 12, 25,
                                                      strlen("Keys"));
        const auto attachedPropertyUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("attachedPropertyUsages") << 12 << 25 << attachedPropertyUsages;
    }
    {
        const auto testFileName = testFile("findUsages/inlineComponents/inlineComponents.qml");
        const auto testFileContent = readFileContent(testFileName);
        QList<QQmlLSUtils::Location> expectedUsages;
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 22,
                                                      strlen("foo"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 44,
                                                      strlen("foo"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 27,
                                                      strlen("foo"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 20, 20,
                                                      strlen("foo"));
        const auto inlineUsages = makeUsages(testFileName, expectedUsages);
        QTest::addRow("inlineUsagesFromProperty") << 9 << 22 << inlineUsages;
        QTest::addRow("inlineUsagesFromUsageOfBaseProperty") << 14 << 27 << inlineUsages;
        QTest::addRow("inlineUsagesFromJsScope") << 20 << 20 << inlineUsages;
    }
    {
        const auto testFileName = testFile("findUsages/propertyChanges/propertyChanges.qml");
        const auto testFileContent = readFileContent(testFileName);
        QList<QQmlLSUtils::Location> expectedUsages;
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 9,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 16, 21,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 19, 25,
                                                      strlen("onClicked"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 25, 17,
                                                      strlen("onClicked"));
        const auto propertyChanges = makeUsages(testFileName, expectedUsages);
        QTest::addRow("propertyChanges1") << 16 << 21 << propertyChanges;
    }
    {
        const auto testFileName = testFile("findUsages/bindings/bindings.qml");
        const auto testFileContent = readFileContent(testFileName);
        QList<QQmlLSUtils::Location> expectedUsages;
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 11, 23,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 14, 27,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 21, 27,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 19,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 27, 41,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 34, 17,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 13, 20,
                                                      strlen("patronChanged"));
        expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 20, 23,
                                                      strlen("\"patronChanged\""));
        const auto bindings = makeUsages(testFileName, expectedUsages);
        QTest::addRow("propertyInBindingsFromDecl") << 11 << 23 << bindings;
        QTest::addRow("generalizedGroupPropertyBindings") << 27 << 19 << bindings;
    }
    {
        const auto testFileName = testFile("findUsages/enums/Enums.qml");
        const auto testFileContent = readFileContent(testFileName);
        const auto otherFileName = testFile("findUsages/enums/EnumsFromAnotherFile.qml");
        const auto otherFileContent = readFileContent(otherFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 9,
                                                          strlen("Patron"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 22, 35,
                                                          strlen("Patron"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 23, 34,
                                                          strlen("Patron"));
            const auto enums = makeUsages(testFileName, expectedUsages);
            QTest::addRow("enumValuesFromDeclaration") << 9 << 9 << enums;
            QTest::addRow("enumValuesFromUsage") << 22 << 35 << enums;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 10,
                                                          strlen("Cats"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 22, 30,
                                                          strlen("Cats"));
            const auto enums = makeUsages(testFileName, expectedUsages);
            QTest::addRow("enumNameFromDeclaration") << 8 << 10 << enums;
            QTest::addRow("enumNameFromUsage") << 22 << 30 << enums;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 26, 46,
                                                          strlen("FromAnotherUniverse"));
            expectedUsages << QQmlLSUtils::Location::from(otherFileName, otherFileContent, 4, 68,
                                                          strlen("FromAnotherUniverse"));
            const auto enums = makeUsages(testFileName, expectedUsages);
            QTest::addRow("enumNameFromDeclarationInOtherFile") << 26 << 50 << enums;
            const auto enumsFromOtherFile = makeUsages(otherFileName, expectedUsages);
            QTest::addRow("enumNameFromUsageFromOtherFile") << 4 << 81 << enumsFromOtherFile;
        }
    }
    {
        const auto testFileName = testFile("findUsages/inlineComponents/inlineComponents2.qml");
        const auto testFileContent = readFileContent(testFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 4, 15,
                                                          strlen("MyIC"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 5, 5,
                                                          strlen("MyIC"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 5, 12,
                                                          strlen("MyIC"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 5, 19,
                                                          strlen("MyIC"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 6, 19,
                                                          strlen("MyIC"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 6, 26,
                                                          strlen("MyIC"));
            const auto inlineComponents = makeUsages(testFileName, expectedUsages);
            QTest::addRow("findICUsagesFromDefinition") << 4 << 16 << inlineComponents;
            QTest::addRow("findICUsagesFromDefinition2") << 4 << 9 << inlineComponents;
            QTest::addRow("findICUsagesFromUsage") << 5 << 19 << inlineComponents;
            QTest::addRow("findICUsagesFromTypeUsage") << 6 << 19 << inlineComponents;
        }
    }
    {
        const auto testFileName = testFile("findUsages/lambdas/lambdas.qml");
        const auto testFileContent = readFileContent(testFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 6, 29,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 7, 17,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 46,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 9, 21,
                                                          strlen("a"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 10, 17,
                                                          strlen("a"));

            const auto usagesForTestFile = makeUsages(testFileName, expectedUsages);
            QTest::addRow("lambdaParameter") << 7 << 17 << usagesForTestFile;
            QTest::addRow("lambdaParameter2") << 6 << 29 << usagesForTestFile;
            QTest::addRow("lambdaParameterFromInnerArrow") << 8 << 46 << usagesForTestFile;
        }
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 28,
                                                          strlen("x"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 8, 38,
                                                          strlen("x"));

            const auto usagesForTestFile = makeUsages(testFileName, expectedUsages);
            QTest::addRow("arrowParameter") << 8 << 28 << usagesForTestFile;
            QTest::addRow("arrowParameter2") << 8 << 38 << usagesForTestFile;
        }
    }
    {
        const auto testFileName = testFile("findUsages/inlineComponents/inlineComponents.qml");
        const auto testFileContent = readFileContent(testFileName);
        const auto providerFileName =
                testFile("findUsages/inlineComponents/InlineComponentProvider.qml");
        const auto providerFileContent = readFileContent(providerFileName);
        {
            QList<QQmlLSUtils::Location> expectedUsages;
            expectedUsages << QQmlLSUtils::Location::from(providerFileName, providerFileContent, 4,
                                                          15, strlen("IC1"));
            expectedUsages << QQmlLSUtils::Location::from(providerFileName, providerFileContent, 5,
                                                          36, strlen("IC1"));
            expectedUsages << QQmlLSUtils::Location::from(providerFileName, providerFileContent, 7,
                                                          5, strlen("IC1"));
            expectedUsages << QQmlLSUtils::Location::from(providerFileName, providerFileContent, 17,
                                                          13, strlen("IC1"));

            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 25, 38,
                                                          strlen("IC1"));
            expectedUsages << QQmlLSUtils::Location::from(testFileName, testFileContent, 25, 84,
                                                          strlen("IC1"));

            {
                const auto usagesForTestFile = makeUsages(testFileName, expectedUsages);
                QTest::addRow("findICUsagesFromTypeAnnotationInOtherFiles")
                        << 25 << 39 << usagesForTestFile;
                QTest::addRow("findICUsagesFromInstantiationInOtherFiles")
                        << 25 << 84 << usagesForTestFile;
            }

            {
                const auto usagesInProviderFile = makeUsages(providerFileName, expectedUsages);

                QTest::addRow("findICUsagesFromDefinitionInOtherFiles")
                        << 4 << 16 << usagesInProviderFile;
                QTest::addRow("findICUsagesFromInstantiationInOtherFiles2")
                        << 17 << 14 << usagesInProviderFile;
            }
        }
    }
}

void tst_qmlls_utils::findUsages()
{
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(UsageData, data);

    {
        auto usagesInFilename = data.expectedUsages.usagesInFilename();
        QVERIFY(std::is_sorted(usagesInFilename.begin(), usagesInFilename.end()));
        auto usagesInFile = data.expectedUsages.usagesInFile();
        QVERIFY(std::is_sorted(usagesInFile.begin(), usagesInFile.end()));
    }

    auto [env, file] = createEnvironmentAndLoadFile(data.testFileName);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    auto usages = QQmlLSUtils::findUsagesOf(locations.front().domItem);

    if constexpr (enable_debug_output) {
        if (usages != data.expectedUsages) {
            qDebug() << "Got:\n";
            for (auto &x : usages.usagesInFile()) {
                qDebug() << x.filename() << "(" << x.sourceLocation().startLine << ", "
                         << x.sourceLocation().startColumn << "), " << x.sourceLocation().offset << "+"
                         << x.sourceLocation().length;
            }
            qDebug() << "with usages in filenames:" << usages.usagesInFilename();
            qDebug() << "But expected: \n";
            for (auto &x : data.expectedUsages.usagesInFile()) {
                qDebug() << x.filename() << "(" << x.sourceLocation().startLine << ", "
                         << x.sourceLocation().startColumn << "), " << x.sourceLocation().offset << "+"
                         << x.sourceLocation().length;
            }
            qDebug() << "with usages in filenames:" << data.expectedUsages.usagesInFilename();
        }
    }

    QCOMPARE(usages, data.expectedUsages);
}


void tst_qmlls_utils::renameUsages_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("newName");
    QTest::addColumn<QQmlLSUtils::RenameUsages>("expectedRenames");
    QTest::addColumn<QString>("expectedError");

    const QString testFileName = testFile(u"JSUsages.qml"_s);
    const QString testFileNameFromAnotherFile = testFile(u"JSUsagesFromAnotherFile.qml"_s);
    const QString testFileContent = readFileContent(testFileName);
    const QString testFileFromAnotherFileContent = readFileContent(testFileNameFromAnotherFile);

    const QString noError;
    const QQmlLSUtils::RenameUsages noRenames;

    QQmlLSUtils::RenameUsages methodFRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 72, 14, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 74, 24, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 74, 34, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 74, 51, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 74, 68, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 76, 20, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 79, 34, strlen("recursive"),
                                        u"newNameNewMe"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 84, 27, strlen("recursive"),
                                        u"newNameNewMe"_s),
        },
        {}
    };

    QQmlLSUtils::RenameUsages JSIdentifierSumRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 8, 13, strlen("sum"),
                                        u"sumsumsum123"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 10, 13, strlen("sum"),
                                        u"sumsumsum123"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 10, 19, strlen("sum"),
                                        u"sumsumsum123"_s),
        },
        {}
    };

    QQmlLSUtils::RenameUsages qmlSignalRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 88, 12,
                                        strlen("helloSignal"), u"finalSignal"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 91, 9, strlen("helloSignal"),
                                        u"finalSignal"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 93, 13,
                                        strlen("helloSignal"), u"finalSignal"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 97, 17,
                                        strlen("helloSignal"), u"finalSignal"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 101, 9,
                                        strlen("helloSignal"), u"finalSignal"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 119, 5,
                                        strlen("onHelloSignal"), u"onFinalSignal"_s),
        },
        {}
    };

    QQmlLSUtils::RenameUsages helloPropertyRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 17, 18,
                                        strlen("helloProperty"), u"freshPropertyName"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 24, 13,
                                        strlen("helloProperty"), u"freshPropertyName"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 24, 29,
                                        strlen("helloProperty"), u"freshPropertyName"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 65, 60,
                                        strlen("helloProperty"), u"freshPropertyName"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 151, 9,
                                        strlen("helloPropertyChanged"),
                                        u"freshPropertyNameChanged"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 153, 5,
                                        strlen("onHelloPropertyChanged"),
                                        u"onFreshPropertyNameChanged"_s),
                QQmlLSUtils::Edit::from(testFileNameFromAnotherFile, testFileFromAnotherFileContent,
                                        12, 16, strlen("helloProperty"), u"freshPropertyName"_s),
        },
        {}
    };

    QQmlLSUtils::RenameUsages nestedComponentRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 42, 15,
                                        strlen("NestedComponent"), u"SuperInlineComponent"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 61, 5,
                                        strlen("NestedComponent"), u"SuperInlineComponent"_s),
        },
        {}
    };

    QQmlLSUtils::RenameUsages myNestedIdRename{
        {
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 62, 13, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 65, 17, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 66, 17, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 67, 17, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 68, 17, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
                QQmlLSUtils::Edit::from(testFileName, testFileContent, 69, 17, strlen("myNested"),
                                        u"freshNewIdForMyNested"_s),
        },
        {}
    };

    const QString renameFileQml = testFile("renaming/main.qml");
    const QString renameFileQmlContent = readFileContent(renameFileQml);
    const QQmlLSUtils::RenameUsages renameComponent1{
        {
            QQmlLSUtils::Edit::from(renameFileQml, renameFileQmlContent, 4, 5,
                                    strlen("RenameMe"), u"FreshNewComponentName"_s),
        },
        {
            { testFile("renaming/RenameMe.qml"),
                        testFile(u"renaming/FreshNewComponentName.qml"_s) },
        }
    };
    const QQmlLSUtils::RenameUsages renameComponent2{
        {
            QQmlLSUtils::Edit::from(renameFileQml, renameFileQmlContent, 5, 5,
                                    strlen("RenameMe2"), u"AnotherOneThankYou"_s),
        },
        {
            { testFile("renaming/RenameMe2.ui.qml"),
                        testFile(u"renaming/AnotherOneThankYou.ui.qml"_s) },
        }
    };
    const QQmlLSUtils::RenameUsages renameComponentNamedByQmldir{
        {
            QQmlLSUtils::Edit::from(renameFileQml, renameFileQmlContent, 6, 5,
                                    strlen("HelloWorld"), u"AnotherOneThankYou"_s),
        },
        // make sure that the file itself does not get renamed
        {}
    };

    const QString parserError = u"Invalid EcmaScript identifier!"_s;

    QTest::addRow("renameMethod") << testFileName << 72 << 19 << u"newNameNewMe"_s << methodFRename
                                  << noError;
    QTest::addRow("renameJSIdentifier")
            << testFileName << 10 << 19 << u"sumsumsum123"_s << JSIdentifierSumRename << noError;
    QTest::addRow("renameQmlSignal")
            << testFileName << 93 << 19 << u"finalSignal"_s << qmlSignalRename << noError;
    QTest::addRow("renameQmlSignalHandler")
            << testFileName << 119 << 10 << u"onFinalSignal"_s << qmlSignalRename << noError;

    QTest::addRow("renameQmlProperty")
            << testFileName << 17 << 20 << u"freshPropertyName"_s << helloPropertyRename << noError;
    QTest::addRow("renameQmlPropertyChanged")
            << testFileName << 151 << 18 << u"freshPropertyNameChanged"_s << helloPropertyRename
            << noError;
    QTest::addRow("renameQmlPropertyChangedHandler")
            << testFileName << 153 << 22 << u"onFreshPropertyNameChanged"_s << helloPropertyRename
            << noError;

    QTest::addRow("renameQmlObjectId") << testFileName << 65 << 21 << u"freshNewIdForMyNested"_s
                                       << myNestedIdRename << noError;

    // rename forbidden stuff
    QTest::addRow("renameCPPDefinedItem") << testFileName << 144 << 13 << u"onHelloWorld"_s
                                          << noRenames << u"defined in non-QML files."_s;
    QTest::addRow("renameFunctionKeyword") << testFileName << 8 << 10 << u"HelloWorld"_s
                                           << noRenames << "Requested item cannot be renamed";
    QTest::addRow("invalidCharactersInIdentifier")
            << testFileName << 12 << 22 << u"\""_s << noRenames << parserError;
    QTest::addRow("invalidCharactersInIdentifier2")
            << testFileName << 12 << 22 << u"hello world"_s << noRenames << parserError;
    QTest::addRow("invalidCharactersInIdentifier3")
            << testFileName << 12 << 22 << u"sum.sum.sum"_s << noRenames << parserError;
    QTest::addRow("emptyIdentifier")
            << testFileName << 12 << 22 << QString() << noRenames << parserError;
    QTest::addRow("usingKeywordAsIdentifier")
            << testFileName << 12 << 22 << u"function"_s << noRenames << parserError;

    QTest::addRow("changedSignalHandlerMissingOnChanged")
            << testFileName << 134 << 9 << u"___"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerMissingChanged")
            << testFileName << 134 << 9 << u"on___"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerMissingOn")
            << testFileName << 134 << 9 << u"___Changed"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerTypoInChanged")
            << testFileName << 134 << 9 << u"on___Chänged"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;

    QTest::addRow("signalHandlerMissingOn")
            << testFileName << 119 << 10 << u"helloSuperSignal"_s << noRenames
            << u"Invalid name for a signal handler identifier"_s;
    QTest::addRow("signalHandlerMissingCapitalization")
            << testFileName << 119 << 10 << u"onhelloSuperSignal"_s << noRenames
            << u"Invalid name for a signal handler identifier"_s;

    QTest::addRow("JSIdentifierStartsWithNumber")
            << testFileName << 67 << 13 << u"123"_s << noRenames << parserError;

    QTest::addRow("renameQmlFile") << testFile(u"renaming/main.qml"_s) << 4 << 9
                                   << u"FreshNewComponentName"_s << renameComponent1 << noError;

    QTest::addRow("renameUiQmlFile") << testFile(u"renaming/main.qml"_s) << 5 << 9
                                     << u"AnotherOneThankYou"_s << renameComponent2 << noError;

    QTest::addRow("renameQmlFileRenamedByQmldir")
            << testFile(u"renaming/main.qml"_s) << 6 << 8 << u"AnotherOneThankYou"_s
            << renameComponentNamedByQmldir << noError;
}

void tst_qmlls_utils::renameUsages()
{
    // findAndRenameUsages() already tests if all usages will be renamed
    // now test that the new name is correctly passed
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, newName);
    QFETCH(QQmlLSUtils::RenameUsages, expectedRenames);
    QFETCH(QString, expectedError);

    {
        const auto renameInFile = expectedRenames.renameInFile();
        QVERIFY(std::is_sorted(renameInFile.constBegin(), renameInFile.constEnd()));
        const auto renameInFilename = expectedRenames.renameInFilename();
        QVERIFY(std::is_sorted(renameInFilename.begin(), renameInFilename.end()));
    }

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    if (auto errors = QQmlLSUtils::checkNameForRename(locations.front().domItem, newName)) {
        if constexpr (enable_debug_output) {
            if (expectedError.isEmpty())
                qDebug() << "Expected no error but got" << errors->message;
            if (!errors->message.contains(expectedError))
                qDebug() << "Cannot find" << expectedError << "in" << errors->message;
        }
        QVERIFY(!expectedError.isEmpty());
        QVERIFY(errors->message.contains(expectedError));
        return;
    }
    auto edits = QQmlLSUtils::renameUsagesOf(locations.front().domItem, newName);

    if constexpr (enable_debug_output) {
        if (edits != expectedRenames) {
            qDebug() << "Got:\n";
            for (auto &x : edits.renameInFile()) {
                qDebug() << x.replacement << x.location.filename() << "("
                         << x.location.sourceLocation().startLine << ", "
                         << x.location.sourceLocation().startColumn << "), "
                         << x.location.sourceLocation().offset << "+"
                         << x.location.sourceLocation().length;
            }
            qDebug() << "with renames in filenames:";
            for (auto &x : edits.renameInFilename()) {
                qDebug() << x.oldFilename << "->" << x.newFilename;
            }
            qDebug() << "But expected: \n";
            for (auto &x : expectedRenames.renameInFile()) {
                qDebug() << x.replacement << x.location.filename() << "("
                         << x.location.sourceLocation().startLine << ", "
                         << x.location.sourceLocation().startColumn << "), "
                         << x.location.sourceLocation().offset << "+"
                         << x.location.sourceLocation().length;
            }
            qDebug() << "with renames in filenames:";
            for (auto &x : expectedRenames.renameInFilename()) {
                qDebug() << x.oldFilename << "->" << x.newFilename;
            }
        }
    }
    QCOMPARE(edits, expectedRenames);
}

void tst_qmlls_utils::findDefinitionFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");

    QTest::addColumn<QString>("expectedFilePath");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");
    QTest::addColumn<size_t>("expectedLength");
    QTest::addColumn<QStringList>("extraBuildDirs");

    const QString JSDefinitionsQml = testFile(u"JSDefinitions.qml"_s);
    const QString BaseTypeQml = testFile(u"BaseType.qml"_s);
    const QStringList noExtraBuildDir;

    QTest::addRow("JSIdentifierX")
            << JSDefinitionsQml << 14 << 11 << JSDefinitionsQml << 13 << 13 << strlen("x") << noExtraBuildDir;
    QTest::addRow("JSIdentifierX2")
            << JSDefinitionsQml << 15 << 11 << JSDefinitionsQml << 13 << 13 << strlen("x") << noExtraBuildDir;
    QTest::addRow("propertyI") << JSDefinitionsQml << 14 << 14 << JSDefinitionsQml << 9 << 18
                               << strlen("i") << noExtraBuildDir;
    QTest::addRow("qualifiedPropertyI")
            << JSDefinitionsQml << 15 << 21 << JSDefinitionsQml << 9 << 18 << strlen("i") << noExtraBuildDir;
    QTest::addRow("inlineComponentProperty")
            << JSDefinitionsQml << 62 << 21 << JSDefinitionsQml << 54 << 22 << strlen("data") << noExtraBuildDir;

    QTest::addRow("parameterA") << JSDefinitionsQml << 10 << 16 << JSDefinitionsQml << 10 << 16
                                << strlen("a") << noExtraBuildDir;
    QTest::addRow("parameterAUsage")
            << JSDefinitionsQml << 10 << 39 << JSDefinitionsQml << -1 << 16 << strlen("a") << noExtraBuildDir;

    QTest::addRow("parameterB") << JSDefinitionsQml << 10 << 28 << JSDefinitionsQml << 10 << 28
                                << strlen("b") << noExtraBuildDir;
    QTest::addRow("parameterBUsage")
            << JSDefinitionsQml << 10 << 86 << JSDefinitionsQml << -1 << 28 << strlen("b") << noExtraBuildDir;

    QTest::addRow("comment") << JSDefinitionsQml << 10 << 21 << noResultExpected << -1 << -1
                             << size_t{} << noExtraBuildDir;

    QTest::addRow("scopedX") << JSDefinitionsQml << 22 << 18 << JSDefinitionsQml << 21 << 17
                             << strlen("scoped") << noExtraBuildDir;
    QTest::addRow("scopedX2") << JSDefinitionsQml << 25 << 22 << JSDefinitionsQml << 21 << 17
                              << strlen("scoped") << noExtraBuildDir;
    QTest::addRow("scopedX3") << JSDefinitionsQml << 28 << 14 << JSDefinitionsQml << 19 << 13
                              << strlen("scoped") << noExtraBuildDir;

    QTest::addRow("normalI") << JSDefinitionsQml << 22 << 23 << JSDefinitionsQml << 9 << 18
                             << strlen("i") << noExtraBuildDir;
    QTest::addRow("scopedI") << JSDefinitionsQml << 25 << 27 << JSDefinitionsQml << 24 << 32
                             << strlen("i") << noExtraBuildDir;

    QTest::addRow("shadowingProperty")
            << JSDefinitionsQml << 37 << 21 << JSDefinitionsQml << 34 << 22 << strlen("i") << noExtraBuildDir;
    QTest::addRow("shadowingQualifiedProperty")
            << JSDefinitionsQml << 37 << 35 << JSDefinitionsQml << 34 << 22 << strlen("i") << noExtraBuildDir;
    QTest::addRow("shadowedProperty")
            << JSDefinitionsQml << 37 << 49 << JSDefinitionsQml << 9 << 18 << strlen("i") << noExtraBuildDir;

    QTest::addRow("propertyInBinding")
            << JSDefinitionsQml << 64 << 37 << JSDefinitionsQml << 9 << 18 << strlen("i") << noExtraBuildDir;
    QTest::addRow("propertyInBinding2")
            << JSDefinitionsQml << 65 << 38 << JSDefinitionsQml << 9 << 18 << strlen("i") << noExtraBuildDir;
    QTest::addRow("propertyInBinding3")
            << JSDefinitionsQml << 66 << 51 << JSDefinitionsQml << 9 << 18 << strlen("i") << noExtraBuildDir;

    QTest::addRow("propertyFromDifferentFile")
            << JSDefinitionsQml << 72 << 20 << BaseTypeQml << 24 << 18 << strlen("helloProperty") << noExtraBuildDir;

    QTest::addRow("id") << JSDefinitionsQml << 15 << 17 << JSDefinitionsQml << 7 << 9
                        << strlen("rootId") << noExtraBuildDir;
    QTest::addRow("onId") << JSDefinitionsQml << 32 << 16 << JSDefinitionsQml << 32 << 13
                          << strlen("nested") << noExtraBuildDir;
    QTest::addRow("parentId") << JSDefinitionsQml << 37 << 44 << JSDefinitionsQml << 7 << 9
                              << strlen("rootId") << noExtraBuildDir;
    QTest::addRow("currentId") << JSDefinitionsQml << 37 << 30 << JSDefinitionsQml << 32 << 13
                               << strlen("nested") << noExtraBuildDir;
    QTest::addRow("inlineComponentId")
            << JSDefinitionsQml << 56 << 35 << JSDefinitionsQml << 52 << 13 << strlen("helloIC") << noExtraBuildDir;

    QTest::addRow("recursiveFunction")
            << JSDefinitionsQml << 39 << 28 << JSDefinitionsQml << 36 << 18 << strlen("f") << noExtraBuildDir;
    QTest::addRow("recursiveFunction2")
            << JSDefinitionsQml << 39 << 39 << JSDefinitionsQml << 36 << 18 << strlen("f") << noExtraBuildDir;
    QTest::addRow("functionFromFunction")
            << JSDefinitionsQml << 44 << 20 << JSDefinitionsQml << 36 << 18 << strlen("f") << noExtraBuildDir;
    QTest::addRow("qualifiedFunctionName")
            << JSDefinitionsQml << 48 << 23 << JSDefinitionsQml << 36 << 18 << strlen("f") << noExtraBuildDir;

    QTest::addRow("functionInParent")
            << JSDefinitionsQml << 44 << 37 << JSDefinitionsQml << 18 << 14 << strlen("ffff") << noExtraBuildDir;
    QTest::addRow("functionFromDifferentFile")
            << JSDefinitionsQml << 72 << 47 << BaseTypeQml << 25 << 14 << strlen("helloFunction") << noExtraBuildDir;
    QTest::addRow("componentFromFile")
            << JSDefinitionsQml << 68 << 28 << BaseTypeQml << 6 << 1 << strlen("Item") << noExtraBuildDir;
    QTest::addRow("inlineComponentFromDifferentFile")
            << JSDefinitionsQml << 75 << 27 << BaseTypeQml << 9 << 38 << strlen("Item") << noExtraBuildDir;

    QTest::addRow("lambdaParameter")
            << JSDefinitionsQml << 78 << 33 << JSDefinitionsQml << 77 << 34 << strlen("c") << noExtraBuildDir;
    QTest::addRow("callFromLambda")
            << JSDefinitionsQml << 79 << 21 << JSDefinitionsQml << 47 << 14 << strlen("abc") << noExtraBuildDir;
    QTest::addRow("callFromLambda2")
            << JSDefinitionsQml << 81 << 32 << JSDefinitionsQml << 76 << 14 << strlen("helloLambda") << noExtraBuildDir;
    QTest::addRow("lambdaRecursion")
            << JSDefinitionsQml << 80 << 29 << JSDefinitionsQml << 77 << 24 << strlen("function") << noExtraBuildDir;

    QTest::addRow("arrowParameter")
            << JSDefinitionsQml << 84 << 37 << JSDefinitionsQml << 83 << 32 << strlen("c") << noExtraBuildDir;
    QTest::addRow("callFromArrow")
            << JSDefinitionsQml << 85 << 21 << JSDefinitionsQml << 47 << 14 << strlen("abc") << noExtraBuildDir;
    QTest::addRow("callFromArrow2")
            << JSDefinitionsQml << 87 << 32 << JSDefinitionsQml << 76 << 14 << strlen("helloLambda") << noExtraBuildDir;
    QTest::addRow("arrowRecursion")
            << JSDefinitionsQml << 86 << 29 << JSDefinitionsQml << 77 << 24 << strlen("function") << noExtraBuildDir;

    {
        const QString definitionFile =
                testFile(u"findDefinition/MyApplicationWindowModule/MyApplicationWindow.qml"_s);
        const QString qmlComponents = testFile(u"findDefinition/QmlComponents.qml"_s);
        const QString qualifiedQmlComponents = testFile(u"findDefinition/QualifiedQmlComponents.qml"_s);
        QTest::addRow("component") << qmlComponents << 7 << 11 << definitionFile << 7 << 1
                                   << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("attachedType") << qmlComponents << 9 << 42 << definitionFile << 7 << 1
                                      << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("enumValue") << qmlComponents << 10 << 42 << definitionFile << 7 << 1
                                   << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("enumName") << qmlComponents << 11 << 42 << definitionFile << 7 << 1
                                  << strlen("ApplicationWindow") << noExtraBuildDir;

        QTest::addRow("qualifiedComponent") << qualifiedQmlComponents << 7 << 11 << definitionFile
                                            << 7 << 1 << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("qualifiedAttachedType")
                << qualifiedQmlComponents << 9 << 47 << definitionFile << 7 << 1
                << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("qualifiedEnumValue") << qualifiedQmlComponents << 10 << 42 << definitionFile
                                            << 7 << 1 << strlen("ApplicationWindow") << noExtraBuildDir;
        QTest::addRow("qualifiedEnumName") << qualifiedQmlComponents << 11 << 42 << definitionFile
                                           << 7 << 1 << strlen("ApplicationWindow") << noExtraBuildDir;

        QTest::addRow("qualifiedModuleName")
                << qualifiedQmlComponents << 9 << 42 << qualifiedQmlComponents << 5 << 52
                << strlen("MAWM") << noExtraBuildDir;
    }

    {
        const QString mainQml = testFile(u"findDefinition/mymodule-source/MyModule/Main.qml"_s);
        const QString myComponentQml = testFile(u"findDefinition/mymodule-source/MyModule/X/Y/Z/MyComponent.qml"_s);

        QTest::addRow("nestedFromOwnModule") << mainQml << 4 << 5 << myComponentQml << 3 << 1
                                             << strlen("Item") << QStringList { testFile(u"findDefinition/mymodule-build"_s) };
        QTest::addRow("nestedFromOwnModuleWithoutQmldirPrefer") << mainQml << 4 << 5 << myComponentQml << 3 << 1
                                                                << strlen("Item")
                                                                << QStringList { testFile(u"findDefinition/mymodule-build-without-qmldir-prefer"_s) };
    }

    {
        const QString mainQml = testFile(u"findDefinition/TestAppWithBuildFolder/TestApp/Main.qml"_s);
        const QString myComponentQml = testFile(u"findDefinition/TestAppWithBuildFolder/TestApp/somesubfolder/anothersubfolder/MyModule/MyItem.qml"_s);
        QTest::addRow("componentFromOtherModule")
                << mainQml << 5 << 8 << myComponentQml << 3 << 1 << strlen("Item")
                << QStringList{
                       testFile(u"findDefinition/TestAppWithBuildFolder/build"_s),
                       testFile(
                           u"findDefinition/TestAppWithBuildFolder/build/somesubfolder/anothersubfolder"_s)
                   };
    }
}

void tst_qmlls_utils::findDefinitionFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, expectedFilePath);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);
    QFETCH(size_t, expectedLength);
    QFETCH(QStringList, extraBuildDirs);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, extraBuildDirs);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), 1);

    auto definition = QQmlLSUtils::findDefinitionOf(locations.front().domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QVERIFY(!definition);
        return;
    }

    QVERIFY(definition);

    // don't work with absolute paths, and only compare the end of the file path
    QCOMPARE(QStringView(definition->filename()).last(expectedFilePath.size()), expectedFilePath);

    QCOMPARE(definition->sourceLocation().startLine, quint32(expectedLine));
    QCOMPARE(definition->sourceLocation().startColumn, quint32(expectedCharacter));
    QCOMPARE(definition->sourceLocation().length, quint32(expectedLength));
}

void tst_qmlls_utils::resolveExpressionType_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QQmlLSUtils::ResolveOptions>("resolveOption");
    QTest::addColumn<QString>("expectedFile");
    // startline of the owners definition
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<QQmlLSUtils::IdentifierType>("expectedType");
    QTest::addColumn<QString>("expectedName");

    using namespace QQmlLSUtils;

    const int noLine = -1;
    const QString noFile;
    const QString unnamed;

    {
        const QString JSDefinitionsQml = testFile(u"JSDefinitions.qml"_s);
        const int parentLine = 6;
        const int childLine = 31;

        QTest::addRow("id") << JSDefinitionsQml << 15 << 17 << ResolveOwnerType << JSDefinitionsQml
                            << parentLine << QmlObjectIdIdentifier << u"rootId"_s;
        QTest::addRow("childIddInChild")
                << JSDefinitionsQml << 37 << 30 << ResolveOwnerType << JSDefinitionsQml << childLine
                << QmlObjectIdIdentifier << u"nested"_s;
        QTest::addRow("parentIdInChild")
                << JSDefinitionsQml << 37 << 43 << ResolveOwnerType << JSDefinitionsQml
                << parentLine << QmlObjectIdIdentifier << u"rootId"_s;

        QTest::addRow("propertyI")
                << JSDefinitionsQml << 14 << 14 << ResolveOwnerType << JSDefinitionsQml
                << parentLine << PropertyIdentifier << u"i"_s;
        QTest::addRow("qualifiedPropertyI")
                << JSDefinitionsQml << 15 << 21 << ResolveOwnerType << JSDefinitionsQml
                << parentLine << PropertyIdentifier << u"i"_s;
        QTest::addRow("propertyIInChild")
                << JSDefinitionsQml << 37 << 21 << ResolveOwnerType << JSDefinitionsQml << childLine
                << PropertyIdentifier << u"i"_s;
        QTest::addRow("qualifiedChildPropertyIInChild")
                << JSDefinitionsQml << 37 << 35 << ResolveOwnerType << JSDefinitionsQml << childLine
                << PropertyIdentifier << u"i"_s;
        QTest::addRow("qualifiedParentPropertyIInChild")
                << JSDefinitionsQml << 37 << 49 << ResolveOwnerType << JSDefinitionsQml
                << parentLine << PropertyIdentifier << u"i"_s;

        QTest::addRow("childMethod") << JSDefinitionsQml << 48 << 23 << ResolveOwnerType
                                     << JSDefinitionsQml << childLine << MethodIdentifier << u"f"_s;
        QTest::addRow("childMethod2")
                << JSDefinitionsQml << 44 << 20 << ResolveOwnerType << JSDefinitionsQml << childLine
                << MethodIdentifier << u"f"_s;
        QTest::addRow("parentMethod")
                << JSDefinitionsQml << 14 << 9 << ResolveOwnerType << JSDefinitionsQml << parentLine
                << MethodIdentifier << u"f"_s;
    }

    {
        const QString bindingsOnDeferredQml =
                testFile(u"resolveExpressionType/BindingsOnDeferred.qml"_s);
        const QString qQuickControl = u"private/qquickcontrol_p.h"_s;
        const QString qQuickKeysAttachedType = u"private/qquickitem_p.h"_s;
        QTest::addRow("bindingOnId")
                << bindingsOnDeferredQml << 12 << 14 << ResolveOwnerType << bindingsOnDeferredQml
                << 8 << QmlObjectIdIdentifier << u"inner"_s;
        QTest::addRow("bindingOnQualifiedDeferredProperty")
                << bindingsOnDeferredQml << 12 << 24 << ResolveOwnerType << qQuickControl << noLine
                << PropertyIdentifier << u"background"_s;
        QTest::addRow("groupedPropertyBindingOnId")
                << bindingsOnDeferredQml << 14 << 14 << ResolveOwnerType << bindingsOnDeferredQml
                << 8 << QmlObjectIdIdentifier << u"inner"_s;
        QTest::addRow("someDeferredProperty")
                << bindingsOnDeferredQml << 15 << 22 << ResolveOwnerType << qQuickControl << noLine
                << PropertyIdentifier << u"background"_s;
    }

    {
        const QString JSUsagesQml = testFile(u"JSUsages.qml"_s);
        const int rootLine = 6;
        const int nestedComponent2Line = 46;
        const int nestedComponent3Line = 51;
        const int nestedComponent4Line = 57;
        QTest::addRow("propertyAccess:inner.inner")
                << JSUsagesQml << 68 << 34 << ResolveOwnerType << JSUsagesQml
                << nestedComponent2Line << PropertyIdentifier << u"inner"_s;
        QTest::addRow("propertyAccess:inner.inner2")
                << JSUsagesQml << 69 << 34 << ResolveOwnerType << JSUsagesQml
                << nestedComponent2Line << PropertyIdentifier << u"inner"_s;
        QTest::addRow("propertyAccess:inner.inner.inner")
                << JSUsagesQml << 69 << 40 << ResolveOwnerType << JSUsagesQml
                << nestedComponent3Line << PropertyIdentifier << u"inner"_s;
        QTest::addRow("propertyAccess:inner.inner.inner.p2")
                << JSUsagesQml << 69 << 44 << ResolveOwnerType << JSUsagesQml
                << nestedComponent4Line << PropertyIdentifier << u"p2"_s;

        QTest::addRow("propertyAccess:helloProperty")
                << JSUsagesQml << 65 << 68 << ResolveOwnerType << JSUsagesQml << rootLine
                << PropertyIdentifier << u"helloProperty"_s;
        QTest::addRow("propertyAccess:nestedHelloProperty")
                << JSUsagesQml << 65 << 46 << ResolveOwnerType << JSUsagesQml
                << nestedComponent4Line << PropertyIdentifier << u"helloProperty"_s;
    }

    {
        const QString derivedType = testFile(u"resolveExpressionType/DerivedType.qml"_s);
        const QString derived2Type = testFile(u"resolveExpressionType/Derived2.qml"_s);
        const QString baseType = testFile(u"resolveExpressionType/BaseType.qml"_s);
        const QString qQuickValueTypes = u"private/qquickvaluetypes_p.h"_s;
        const QString qQuickKeysAttachedType = u"private/qquickitem_p.h"_s;

        const int baseTypeLine = 6;
        const int derivedTypeLine = 6;
        const int keysLine = 29;

        QTest::addRow("ownerOfMethod") << derivedType << 9 << 13 << ResolveOwnerType << baseType
                                       << baseTypeLine << MethodIdentifier << u"helloMethod"_s;
        QTest::addRow("ownerOfMethod2") << derivedType << 15 << 33 << ResolveOwnerType << baseType
                                        << baseTypeLine << MethodIdentifier << u"helloMethod"_s;
        QTest::addRow("ownerOfQualifiedMethod")
                << derivedType << 22 << 46 << ResolveOwnerType << baseType << baseTypeLine
                << MethodIdentifier << u"helloMethod"_s;

        QTest::addRow("ownerOfProperty")
                << derivedType << 10 << 22 << ResolveOwnerType << baseType << baseTypeLine
                << PropertyIdentifier << u"helloProperty"_s;
        QTest::addRow("ownerOfProperty2")
                << derivedType << 16 << 37 << ResolveOwnerType << baseType << baseTypeLine
                << PropertyIdentifier << u"helloProperty"_s;
        QTest::addRow("ownerOfQualifiedProperty")
                << derivedType << 23 << 46 << ResolveOwnerType << baseType << baseTypeLine
                << PropertyIdentifier << u"helloProperty"_s;

        QTest::addRow("ownerOfOwnProperty")
                << derivedType << 16 << 23 << ResolveOwnerType << derivedType << derivedTypeLine
                << PropertyIdentifier << u"someProperty"_s;

        QTest::addRow("ownerOfSignal") << derivedType << 11 << 13 << ResolveOwnerType << baseType
                                       << baseTypeLine << SignalIdentifier << u"helloSignal"_s;
        QTest::addRow("ownerOfSignal2") << derivedType << 18 << 37 << ResolveOwnerType << baseType
                                        << baseTypeLine << SignalIdentifier << u"helloSignal"_s;
        QTest::addRow("ownerOfSignalHandler")
                << derivedType << 19 << 10 << ResolveOwnerType << baseType << baseTypeLine
                << SignalHandlerIdentifier << u"onHelloSignal"_s;
        QTest::addRow("ownerOfQualifiedSignal")
                << derivedType << 25 << 22 << ResolveOwnerType << baseType << baseTypeLine
                << SignalIdentifier << u"helloSignal"_s;

        QTest::addRow("ownerOfGroupedProperty")
                << derivedType << 28 << 7 << ResolveOwnerType << baseType << baseTypeLine
                << GroupedPropertyIdentifier << u"helloFont"_s;
        QTest::addRow("ownerOfGroupedProperty2")
                << derivedType << 28 << 17 << ResolveOwnerType << qQuickValueTypes << noLine
                << PropertyIdentifier << u"family"_s;

        QTest::addRow("ownerOfAttachedProperty")
                << derivedType << 29 << 6 << ResolveOwnerType << derivedType << keysLine
                << AttachedTypeIdentifier << u"Keys"_s;
        QTest::addRow("ownerOfAttachedProperty2")
                << derivedType << 29 << 14 << ResolveOwnerType << qQuickKeysAttachedType << noLine
                << SignalHandlerIdentifier << u"onBackPressed"_s;

        QTest::addRow("actualTypeOfAttachedProperty")
                << derivedType << 29 << 14 << ResolveActualTypeForFieldMemberExpression << noFile
                << noLine << SignalHandlerIdentifier << u"onBackPressed"_s;

        QTest::addRow("id2") << derivedType << 7 << 10 << ResolveOwnerType << derivedType << 6
                            << QmlObjectIdIdentifier << u"self"_s;
        QTest::addRow("propertyBinding")
                << derivedType << 31 << 13 << ResolveOwnerType << baseType << baseTypeLine
                << PropertyIdentifier << u"helloProperty"_s;

        QTest::addRow("qmlObject") << derivedType << 6 << 4 << ResolveOwnerType << derived2Type << 4
                                   << QmlComponentIdentifier << u"Derived2"_s;
    }
    {
        const QString file = testFile(u"resolveExpressionType/JSObjects.qml"_s);
        QTest::addRow("jsObject") << file << 11 << 17 << ResolveOwnerType << file << 4
                                  << JavaScriptIdentifier << u"object"_s;

        QTest::addRow("jsObjectMember") << file << 11 << 23 << ResolveOwnerType << noFile << noLine
                                        << JavaScriptIdentifier << u"a"_s;

        QTest::addRow("jsObjectForFieldMemberExpression")
                << file << 11 << 17 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"object"_s;

        QTest::addRow("jsObjectMemberForFieldMemberExpression")
                << file << 11 << 23 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"a"_s;
        QTest::addRow("jsNewMemberExpression")
                << file << 15 << 26 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"Date"_s;
    }
    {
        const QString file = testFile(u"resolveExpressionType/lambdas.qml"_s);
        QTest::addRow("lambda") << file << 6 << 24 << ResolveOwnerType << file << 6
                                << LambdaMethodIdentifier << unnamed;
        QTest::addRow("lambdaParameter") << file << 7 << 28 << ResolveOwnerType << file << 6
                                         << JavaScriptIdentifier << u"b"_s;
        QTest::addRow("arrow") << file << 8 << 36 << ResolveOwnerType << file << 8
                               << LambdaMethodIdentifier << unnamed;
        QTest::addRow("arrowParameter")
                << file << 8 << 42 << ResolveOwnerType << file << 8 << JavaScriptIdentifier << u"y"_s;
        QTest::addRow("letWithLambda")
                << file << 9 << 13 << ResolveOwnerType << file << 6 << JavaScriptIdentifier << u"moreLambdas"_s;
    }
    {
        const QString myHeader = u"private/myfile_p.h"_s;
        const QString file = testFile(u"resolveExpressionType/AttachedComponentMixup.qml"_s);
        QTest::addRow("ComponentAsComponent")
                << file << 6 << 9 << ResolveOwnerType << myHeader << noLine
                << QmlComponentIdentifier << u"TypeWithAttachedAndEnum"_s;
        QTest::addRow("ComponentAsAttachedType")
                << file << 8 << 56 << ResolveOwnerType << myHeader << noLine
                << AttachedTypeIdentifier << u"TypeWithAttachedAndEnum"_s;
        QTest::addRow("ComponentAsEnumName")
                << file << 9 << 33 << ResolveOwnerType << myHeader << noLine
                << QmlComponentIdentifier << u"TypeWithAttachedAndEnum"_s;
    }
    {
        const QString myHeader = u"private/myfile_p.h"_s;
        const QString file = testFile(u"resolveExpressionType/QualifiedAttachedComponentMixup.qml"_s);
        QTest::addRow("QualifiedComponentAsComponent")
                << file << 6 << 9 << ResolveOwnerType << myHeader << noLine
                << QmlComponentIdentifier << u"TypeWithAttachedAndEnum"_s;
        QTest::addRow("QualifiedComponentAsAttachedType")
                << file << 8 << 56 << ResolveOwnerType << myHeader << noLine
                << AttachedTypeIdentifier << u"TypeWithAttachedAndEnum"_s;
        QTest::addRow("QualifiedComponentAsEnumName")
                << file << 9 << 35 << ResolveOwnerType << myHeader << noLine
                << QmlComponentIdentifier << u"TypeWithAttachedAndEnum"_s;

        QTest::addRow("qualifiedModule") << file << 6 << 1 << ResolveOwnerType << noFile << noLine
                                         << QualifiedModuleIdentifier << u"RETM"_s;
        QTest::addRow("qualifiedModule2") << file << 8 << 41 << ResolveOwnerType << noFile
                                          << noLine << QualifiedModuleIdentifier << u"RETM"_s;
        QTest::addRow("qualifiedModule3") << file << 9 << 30 << ResolveOwnerType << noFile
                                          << noLine << QualifiedModuleIdentifier << u"RETM"_s;
        QTest::addRow("qualifiedModule4") << file << 4 << 42 << ResolveOwnerType << noFile
                                          << noLine << QualifiedModuleIdentifier << u"RETM"_s;
    }
    {
        const QString myHeader = u"private/myfile_p.h"_s;
        const QString file = testFile(u"resolveExpressionType/parameterTypeFromBinding.qml"_s);
        QTest::addRow("invalidPropertyChangedHandlerParameter")
                << file << 9 << 23 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"invalid"_s;
        QTest::addRow("invalidPropertyChangedHandlerParameter2")
                << file << 9 << 49 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"invalid"_s;
        QTest::addRow("signalHandlerParameter")
                << file << 12 << 30 << ResolveActualTypeForFieldMemberExpression << myHeader
                << noLine << JavaScriptIdentifier << u"parameter"_s;
        QTest::addRow("signalHandlerParameter2")
                << file << 12 << 63 << ResolveActualTypeForFieldMemberExpression << myHeader
                << noLine << JavaScriptIdentifier << u"parameter"_s;
        QTest::addRow("invalidSignalParameter")
                << file << 12 << 39 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"invalid"_s;
        QTest::addRow("invalidSignalParameter2")
                << file << 12 << 85 << ResolveActualTypeForFieldMemberExpression << noFile << noLine
                << JavaScriptIdentifier << u"invalid"_s;
        QTest::addRow("unrelatedToHandlerParameter") << file << 16 << 17 << ResolveOwnerType << file
                                                     << 15 << JavaScriptIdentifier << u"xxx"_s;
        QTest::addRow("unrelatedToHandlerParameter2")
                << file << 17 << 26 << ResolveOwnerType << file << 15 << JavaScriptIdentifier
                << u"xxx"_s;
        QTest::addRow("signalHandlerParameterNonArrow")
                << file << 21 << 37 << ResolveActualTypeForFieldMemberExpression << myHeader
                << noLine << JavaScriptIdentifier << u"parameter"_s;
        QTest::addRow("onColorChangedHandlerParameter")
                << file << 12 << 30 << ResolveActualTypeForFieldMemberExpression << myHeader
                << noLine << JavaScriptIdentifier << u"parameter"_s;
        QTest::addRow("onFakePropertyChangedSignal")
                << file << 26 << 18 << ResolveOwnerType << myHeader << noLine
                << SignalHandlerIdentifier << u"onMySomeTypeChanged"_s;
    }
}

void tst_qmlls_utils::resolveExpressionType()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QQmlLSUtils::ResolveOptions, resolveOption);
    QFETCH(QString, expectedFile);
    QFETCH(int, expectedLine);
    QFETCH(QQmlLSUtils::IdentifierType, expectedType);
    QFETCH(QString, expectedName);

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);


    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), 1);

    auto definition = QQmlLSUtils::resolveExpressionType(locations.front().domItem, resolveOption);
    QVERIFY(definition);
    QCOMPARE(definition->type, expectedType);
    if (expectedName.isEmpty())
        QVERIFY(!definition->name);
    else
        QCOMPARE(definition->name, expectedName);

    if (expectedFile.isEmpty()) {
        QVERIFY(!definition->semanticScope);
        return;
    }

    QVERIFY(definition->semanticScope);
    QCOMPARE(definition->semanticScope->filePath(), expectedFile);

    if (expectedLine != -1) {
        QQmlJS::SourceLocation location = definition->semanticScope->sourceLocation();
        QCOMPARE((int)location.startLine, expectedLine);
    }
}

void tst_qmlls_utils::isValidEcmaScriptIdentifier_data()
{
    QTest::addColumn<QString>("identifier");
    QTest::addColumn<bool>("isValid");

    QTest::addRow("f") << u"f"_s << true;
    QTest::addRow("f-unicode") << u"\\u0046"_s << true;
    QTest::addRow("starts-with-digit") << u"8helloWorld"_s << false;
    QTest::addRow("starts-with-unicode-digit") << u"\\u0038helloWorld"_s << false; // \u0038 == '8'
    QTest::addRow("keyword") << u"return"_s << false;
    QTest::addRow("not-keyword") << u"returny"_s << true;
}

void tst_qmlls_utils::isValidEcmaScriptIdentifier()
{
    QFETCH(QString, identifier);
    QFETCH(bool, isValid);

    QCOMPARE(QQmlLSUtils::isValidEcmaScriptIdentifier(identifier), isValid);
}

using namespace QLspSpecification;

enum InsertOption { None, InsertColon };

tst_qmlls_utils::ExpectedCompletions
tst_qmlls_utils::quickSnippets(const QStringView firstPrefix, const QStringView secondPrefix) const
{
    const ExpectedCompletions result{
        { u"%1BorderImage snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1BorderImage {\n"
                    u"\tid: ${1:name}\n"
                    u"\tsource: \"${2:file}\"\n"
                    u"\twidth: ${3:100}; height: ${4:100}\n"
                    u"\tborder.left: ${5: 5}; border.top: ${5}\n"
                    u"\tborder.right: ${5}; border.bottom: ${5}\n"
                    u"}"_s.arg(firstPrefix) },
        { u"%1ColorAnimation snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1ColorAnimation {\n"
                    u"\tfrom: \"${1:white}\"\n"
                    u"\tto: \"${2:black}\"\n"
                    u"\tduration: ${3:200}\n"
                    u"}"_s.arg(firstPrefix) },
        { u"%1Image snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1Image {\n"
                    u"\tid: ${1:name}\n"
                    u"\tsource: \"${2:file}\"\n"
                    u"}"_s.arg(firstPrefix) },
        { u"%1Item snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1Item {\n"
                    u"\tid: ${1:name}\n"
                    u"}"_s.arg(firstPrefix) },
        { u"%1NumberAnimation snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1NumberAnimation {\n"
                    u"\ttarget: ${1:object}\n"
                    u"\tproperty: \"${2:name}\"\n"
                    u"\tduration: ${3:200}\n"
                    u"\teasing.type: %2Easing.${4:InOutQuad}\n"
                    u"}"_s.arg(firstPrefix, secondPrefix)  },
        { u"%1NumberAnimation with targets snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1NumberAnimation {\n"
                    u"\ttargets: [${1:object}]\n"
                    u"\tproperties: \"${2:name}\"\n"
                    u"\tduration: ${3:200}\n"
                    u"}"_s.arg(firstPrefix)  },
        { u"%1PauseAnimation snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1PauseAnimation {\n"
                    u"\tduration: ${1:200}\n"
                    u"}"_s.arg(firstPrefix)  },
        { u"%1PropertyAction snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1PropertyAction {\n"
                    u"\ttarget: ${1:object}\n"
                    u"\tproperty: \"${2:name}\"\n"
                    "}"_s.arg(firstPrefix)  },
        { u"%1PropertyAction with targets snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1PropertyAction {\n"
                    u"\ttargets: [${1:object}]\n"
                    u"\tproperties: \"${2:name}\"\n"
                    u"}"_s.arg(firstPrefix)  },
        { u"%1PropertyChanges snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1PropertyChanges {\n"
                    u"\ttarget: ${1:object}\n"
                    u"}"_s.arg(firstPrefix)  },
        { u"%1State snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1State {\n"
                    u"\tname: ${1:name}\n"
                    u"\t%2PropertyChanges {\n"
                    u"\t\ttarget: ${2:object}\n"
                    u"\t}\n"
                    u"}"_s.arg(firstPrefix, secondPrefix)  },
        { u"%1Text snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1Text {\n"
                    u"\tid: ${1:name}\n"
                    u"\ttext: qsTr(\"${2:text}\")\n"
                    u"}"_s.arg(firstPrefix)  },
        { u"%1Transition snippet"_s.arg(firstPrefix), CompletionItemKind::Snippet,
                    u"%1Transition {\n"
                    u"\tfrom: \"${1:fromState}\"\n"
                    u"\tto: \"${2:toState}\"\n"
                    u"}"_s.arg(firstPrefix)  }
    };
    return result;
}

tst_qmlls_utils::ExpectedCompletions
tst_qmlls_utils::quickBindingSnippets(const QStringView firstPrefix) const
{
    const ExpectedCompletions result{
        { { u"states binding with PropertyChanges in State"_s, CompletionItemKind::Snippet,
            u"states: [\n"
            u"\t%1State {\n"
            u"\t\tname: \"${1:name}\"\n"
            u"\t\t%1PropertyChanges {\n"
            u"\t\t\ttarget: ${2:object}\n"
            u"\t\t}\n"
            u"\t}\n"
            u"]"_s.arg(firstPrefix) },
          { u"transitions binding with Transition"_s, CompletionItemKind::Snippet,
            u"transitions: [\n"
            u"\t%1Transition {\n"
            u"\t\tfrom: \"${1:fromState}\"\n"
            u"\t\tto: \"${2:fromState}\"\n"
            u"\t}\n"
            u"]"_s.arg(firstPrefix) } }
    };
    return result;
}

void tst_qmlls_utils::completions_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<ExpectedCompletions>("expected");
    QTest::addColumn<QStringList>("notExpected");

    const QString file = testFile(u"Yyy.qml"_s);
    const QString emptyFile = testFile(u"emptyFile.qml"_s);
    const QString pragmaFile = testFile(u"pragmas.qml"_s);

    const QString singletonName = u"SystemInformation"_s;
    const QString attachedTypeName = u"Component"_s;
    const QString attachedTypeName2 = u"Keys"_s;
    const auto attachedTypes = ExpectedCompletions({
            { attachedTypeName, CompletionItemKind::Class },
            { attachedTypeName2, CompletionItemKind::Class },
    });

    const auto keywords = ExpectedCompletions({
            { u"function"_s, CompletionItemKind::Keyword },
            { u"required"_s, CompletionItemKind::Keyword },
            { u"enum"_s, CompletionItemKind::Keyword },
            { u"component"_s, CompletionItemKind::Keyword },
    });

    const auto mixedTypes = ExpectedCompletions({
            { u"Zzz"_s, CompletionItemKind::Class },
            { u"Item"_s, CompletionItemKind::Class },
            { u"int"_s, CompletionItemKind::Class },
            { u"date"_s, CompletionItemKind::Class },
    });
    const auto constructorTypes = ExpectedCompletions({
            { u"Rectangle"_s, CompletionItemKind::Constructor },
            { u"MyRectangle"_s, CompletionItemKind::Constructor },
            { u"Zzz"_s, CompletionItemKind::Constructor },
            { u"Item"_s, CompletionItemKind::Constructor },
            { u"QtObject"_s, CompletionItemKind::Constructor },
    });
    const auto rectangleTypes = ExpectedCompletions({
            { u"Rectangle"_s, CompletionItemKind::Constructor },
            { u"MyRectangle"_s, CompletionItemKind::Constructor },
    });

    QTest::newRow("objEmptyLine") << file << 9 << 1
                                  << ExpectedCompletions({
                                             { u"Rectangle"_s, CompletionItemKind::Constructor },
                                             { u"width"_s, CompletionItemKind::Property },
                                     })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s });

    const QString propertyCompletion = u"property type name: value;"_s;
    const QString functionCompletion = u"function name(args...): returnType { statements...}"_s;

    const ExpectedCompletions quickSnippetsWithQualifier = quickSnippets(u"QQ.", u"QQ.");
    const ExpectedCompletions quickSnippetsWithQualifierInside = quickSnippets(u"", u"QQ.");
    const ExpectedCompletions quickSnippetsWithoutQualifier = quickSnippets(u"", u"");
    const ExpectedCompletions quickSnippetsWithoutQualifierWithBindings =
            quickBindingSnippets(u"") += quickSnippetsWithoutQualifier;
    const ExpectedCompletions quickSnippetsWithQualifierWithBindings =
            quickBindingSnippets(u"QQ.") += quickSnippetsWithQualifier;
    const ExpectedCompletions quickSnippetsWithQualifierInsideWithBindings =
            quickBindingSnippets(u"QQ.") += quickSnippetsWithQualifierInside;

    QTest::newRow("objEmptyLineSnippets")
            << file << 9 << 1
            << (ExpectedCompletions({
                        { propertyCompletion, CompletionItemKind::Snippet,
                          u"property ${1:type} ${2:name}: ${0:value};"_s },
                        { u"readonly property type name: value;"_s, CompletionItemKind::Snippet,
                          u"readonly property ${1:type} ${2:name}: ${0:value};"_s },
                        { u"default property type name: value;"_s, CompletionItemKind::Snippet,
                          u"default property ${1:type} ${2:name}: ${0:value};"_s },
                        { u"default required property type name: value;"_s,
                          CompletionItemKind::Snippet,
                          u"default required property ${1:type} ${2:name}: ${0:value};"_s },
                        { u"required property type name: value;"_s, CompletionItemKind::Snippet,
                          u"required property ${1:type} ${2:name}: ${0:value};"_s },
                        { u"property type name;"_s, CompletionItemKind::Snippet,
                          u"property ${1:type} ${0:name};"_s },
                        { u"required property type name;"_s, CompletionItemKind::Snippet,
                          u"required property ${1:type} ${0:name};"_s },
                        { u"default property type name;"_s, CompletionItemKind::Snippet,
                          u"default property ${1:type} ${0:name};"_s },
                        { u"default required property type name;"_s, CompletionItemKind::Snippet,
                          u"default required property ${1:type} ${0:name};"_s },
                        { u"final property type name;"_s, CompletionItemKind::Snippet,
                          u"final property ${1:type} ${0:name};"_s },
                        { u"default final property type name;"_s, CompletionItemKind::Snippet,
                          u"default final property ${1:type} ${0:name};"_s },
                        { u"final required property type name;"_s, CompletionItemKind::Snippet,
                          u"final required property ${1:type} ${0:name};"_s },
                        { u"final readonly property type name;"_s, CompletionItemKind::Snippet,
                          u"final readonly property ${1:type} ${0:name};"_s },
                        { u"default final required property type name;"_s,
                          CompletionItemKind::Snippet,
                          u"default final required property ${1:type} ${0:name};"_s },
                        { u"signal name(arg1:type1, ...)"_s, CompletionItemKind::Snippet,
                          u"signal ${1:name}($0)"_s },
                        { u"signal name;"_s, CompletionItemKind::Snippet, u"signal ${0:name};"_s },
                        { u"required name;"_s, CompletionItemKind::Snippet,
                          u"required ${0:name};"_s },
                        { functionCompletion, CompletionItemKind::Snippet,
                          u"function ${1:name}($2): ${3:returnType} {\n\t$0\n}"_s },
                        { u"enum name { Values...}"_s, CompletionItemKind::Snippet,
                          u"enum ${1:name} {\n\t${0:values}\n}"_s },
                        { u"component Name: BaseType { ... }"_s, CompletionItemKind::Snippet,
                          u"component ${1:name}: ${2:baseType} {\n\t$0\n}"_s },
                }) += quickSnippetsWithoutQualifierWithBindings)
            // not allowed because required properties need an initializer
            << QStringList({ u"readonly property type name;"_s });

    QTest::newRow("quickSnippetsForQualifiedQuickImport")
            << testFile("qualifiedModule.qml") << 5 << 1
            << quickSnippetsWithQualifier
            // not allowed because required properties need an initializer
            << QStringList({ u"readonly property type name;"_s });

    QTest::newRow("quickSnippetsForQualifiedQuickImportBeforeDot")
            << testFile("qualifiedModule.qml") << 5 << 6
            << quickSnippetsWithQualifierWithBindings
               // not allowed because required properties need an initializer
            << QStringList({ u"readonly property type name;"_s });

    QTest::newRow("quickSnippetsForQualifiedQuickImportAfterDot")
            << testFile("qualifiedModule.qml") << 5 << 7
            << quickSnippetsWithQualifierInsideWithBindings
            // not allowed because required properties need an initializer
            << QStringList({ u"readonly property type name;"_s });

    QTest::newRow("quickSnippetsForQualifiedQuickImportBeforeDotInBinding")
            << testFile("qualifiedModule.qml") << 4 << 32 << quickSnippetsWithQualifier
            << QStringList({ u"states binding with PropertyChanges in State"_s,
                             u"transitions binding with Transition"_s });

    QTest::newRow("quickSnippetsForQualifiedQuickImportAfterDotInBinding")
            << testFile("qualifiedModule.qml") << 4 << 33 << quickSnippetsWithQualifierInside
            << QStringList({ u"states binding with PropertyChanges in State"_s,
                             u"transitions binding with Transition"_s });

    // forbid transitions and states because QtObject does not inherit from Item
    QTest::newRow("qtObjectEmptyLineSnippets")
            << file << 141 << 8
            << ExpectedCompletions{ { u"Item"_s, CompletionItemKind::Constructor } }
            << QStringList({ u"transitions binding with Transition"_s,
                             u"states binding with PropertyChanges in State"_s });

    QTest::newRow("handlers") << file << 5 << 1
                              << ExpectedCompletions{ {
                                         { u"onHandleMe"_s, CompletionItemKind::Method },
                                         { u"onDefaultPropertyChanged"_s,
                                           CompletionItemKind::Method },
                                 } }
                              << QStringList({ u"QtQuick"_s, u"vector4d"_s });

    QTest::newRow("attachedTypes")
            << file << 9 << 1 << attachedTypes << QStringList{ u"QtQuick"_s, u"vector4d"_s };

    QTest::newRow("attachedTypesInScript")
            << file << 6 << 12 << attachedTypes << QStringList{ u"QtQuick"_s, u"vector4d"_s };
    QTest::newRow("attachedTypesInLongScript")
            << file << 10 << 16 << attachedTypes << QStringList{ u"QtQuick"_s, u"vector4d"_s };

    QTest::newRow("completionFromRootId") << file << 10 << 21
                                          << ExpectedCompletions({
                                                     { u"width"_s, CompletionItemKind::Property },
                                                     { u"lala"_s, CompletionItemKind::Method },
                                                     { u"foo"_s, CompletionItemKind::Property },
                                             })
                                          << QStringList{ u"QtQuick"_s, u"vector4d"_s };

    QTest::newRow("attachedProperties") << file << 89 << 15
                                        << ExpectedCompletions({
                                                   { u"onCompleted"_s, CompletionItemKind::Method },
                                           })
                                        << QStringList{ u"QtQuick"_s,
                                                        u"vector4d"_s,
                                                        attachedTypeName,
                                                        u"Rectangle"_s,
                                                        u"property"_s,
                                                        u"foo"_s,
                                                        u"onActiveFocusOnTabChanged"_s };

    QTest::newRow("inBindingLabel") << file << 6 << 10
                                    << ExpectedCompletions({
                                               { u"Rectangle"_s, CompletionItemKind::Constructor },
                                               { u"width"_s, CompletionItemKind::Property },
                                       })
                                    << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"property"_s });

    QTest::newRow("afterBinding") << file << 6 << 11
                                  << (ExpectedCompletions({
                                              { u"height"_s, CompletionItemKind::Property },
                                              { u"width"_s, CompletionItemKind::Property },
                                              { u"Rectangle"_s, CompletionItemKind::Constructor },
                                              { singletonName, CompletionItemKind::Class },
                                      })
                                      + attachedTypes)
                                  << QStringList({ u"QtQuick"_s, u"property"_s, u"vector4d"_s });

    QTest::newRow("jsGlobals") << file << 6 << 11
                               << ExpectedCompletions{ {
                                          { u"console"_s, CompletionItemKind::Property },
                                          { u"Math"_s, CompletionItemKind::Property },
                                  } }
                               << QStringList({ u"QtQuick"_s, u"property"_s, u"vector4d"_s });

    QTest::newRow("jsGlobals2") << file << 100 << 32
                                << ExpectedCompletions{ {
                                           { u"abs"_s, CompletionItemKind::Method },
                                           { u"log"_s, CompletionItemKind::Method },
                                           { u"E"_s, CompletionItemKind::Property },
                                   } }
                                << QStringList({ u"QtQuick"_s, u"property"_s, u"vector4d"_s,
                                                 u"foo"_s, u"lala"_s });

    QTest::newRow("afterLongBinding")
            << file << 10 << 16
            << ExpectedCompletions({
                       { u"height"_s, CompletionItemKind::Property },
                       { u"width"_s, CompletionItemKind::Property },
                       { u"Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList({ u"QtQuick"_s, u"property"_s, u"vector4d"_s });

    QTest::newRow("afterId") << file << 5 << 8 << ExpectedCompletions({})
                             << QStringList({
                                        u"QtQuick"_s,
                                        u"property"_s,
                                        u"Rectangle"_s,
                                        u"width"_s,
                                        u"vector4d"_s,
                                        u"import"_s,
                                });

    QTest::newRow("emptyFile") << emptyFile << 1 << 1
                               << ExpectedCompletions({
                                          { u"import"_s, CompletionItemKind::Keyword },
                                          { u"pragma"_s, CompletionItemKind::Keyword },
                                  })
                               << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s });

    QTest::newRow("importImport") << file << 1 << 4
                                  << ExpectedCompletions({
                                             { u"import"_s, CompletionItemKind::Keyword },
                                     })
                                  << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s,
                                                   u"Rectangle"_s });

    {

        const QString importFile = testFile(u"completions/imports.qml"_s);
        QTest::newRow("importAfterDot") << importFile << 1 << 16
                                        << ExpectedCompletions({
                                                                   { u"Controls"_s, CompletionItemKind::Module },
                                                               })
                                        << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s,
                                                         u"Rectangle"_s, u"as"_s });

        QTest::newRow("importAfterDot2") << importFile << 1 << 25
                                         << ExpectedCompletions({
                                                                    { u"Basic"_s, CompletionItemKind::Module },
                                                                })
                                         << QStringList({ u"QtQuick"_s, u"vector4d"_s, u"width"_s,
                                                          u"Rectangle"_s, u"as"_s });

        QTest::newRow("importAfterDotMissing")
                << importFile << 2 << 25
                << ExpectedCompletions({
                                           { u"Basic"_s, CompletionItemKind::Module },
                                       })
                << QStringList(
                       { u"QtQuick"_s, u"vector4d"_s, u"width"_s, u"Rectangle"_s, u"as"_s });
    }

    QTest::newRow("importModuleStart")
            << file << 1 << 8
            << ExpectedCompletions({
                       { u"QtQuick"_s, CompletionItemKind::Module },
               })
            << QStringList({ u"vector4d"_s, u"width"_s, u"Rectangle"_s, u"import"_s });

    QTest::newRow("importVersionStart")
            << file << 1 << 16
            << ExpectedCompletions({
                       { u"2"_s, CompletionItemKind::Constant },
                       { u"as"_s, CompletionItemKind::Keyword },
               })
            << QStringList({ u"Rectangle"_s, u"import"_s, u"vector4d"_s, u"width"_s });

    // QTest::newRow("importVersionMinor")
    //         << uri << 1 << 18
    //         << ExpectedCompletions({
    //                    { u"15"_s, CompletionItemKind::Constant },
    //            })
    //         << QStringList({ u"as"_s, u"Rectangle"_s, u"import"_s, u"vector4d"_s, u"width"_s });

    QTest::newRow("expandBase1") << file << 10 << 24
                                 << ExpectedCompletions({
                                            { u"width"_s, CompletionItemKind::Property },
                                            { u"foo"_s, CompletionItemKind::Property },
                                    })
                                 << QStringList({ u"import"_s, u"Rectangle"_s });

    QTest::newRow("expandBase2") << file << 11 << 30
                                 << ExpectedCompletions({
                                            { u"width"_s, CompletionItemKind::Property },
                                            { u"color"_s, CompletionItemKind::Property },
                                    })
                                 << QStringList({ u"foo"_s, u"import"_s, u"Rectangle"_s });

    QTest::newRow("qualifiedTypeCompletionBeforeDot")
            << testFile(u"qualifiedModule.qml"_s) << 4 << 31
            << ExpectedCompletions({
                       { u"QQ.Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList({ u"foo"_s, u"import"_s, u"lala"_s, });

    QTest::newRow("qualifiedTypeCompletionAfterDot")
            << testFile(u"qualifiedModule.qml"_s) << 4 << 35
            << ExpectedCompletions({
                       { u"Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList({ u"foo"_s, u"import"_s, u"lala"_s, u"width"_s });

    QTest::newRow("qualifiedTypeCompletionBeforeDotInDefaultBinding")
            << testFile(u"qualifiedModule.qml"_s) << 5 << 5
            << ExpectedCompletions({
                       { u"QQ.Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList({ u"foo"_s, u"import"_s, u"lala"_s });

    QTest::newRow("qualifiedTypeCompletionAfterDotInDefaultBinding")
            << testFile(u"qualifiedModule.qml"_s) << 5 << 7
            << ExpectedCompletions({
                       { u"Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList({ u"foo"_s, u"import"_s, u"lala"_s, u"width"_s });

    QTest::newRow("parameterCompletion")
            << file << 36 << 24
            << ExpectedCompletions({
                       { u"helloWorld"_s, CompletionItemKind::Variable },
                       { u"helloMe"_s, CompletionItemKind::Variable },
               })
            << QStringList();

    QTest::newRow("inMethodName") << file << 15 << 14 << ExpectedCompletions({})
                                  << QStringList{ u"QtQuick"_s, u"vector4d"_s, u"foo"_s,
                                                  u"root"_s,    u"Item"_s,     singletonName };

    QTest::newRow("inMethodReturnType") << file << 17 << 54 << mixedTypes
                                        << QStringList{
                                               u"QtQuick"_s,
                                               u"foo"_s,
                                               u"root"_s,
                                           };

    QTest::newRow("letStatement") << file << 95 << 13 << ExpectedCompletions({})
                                  << QStringList{ u"QtQuick"_s, u"vector4d"_s, u"root"_s };

    QTest::newRow("inParameterCompletion") << file << 35 << 39 << ExpectedCompletions({})
                                           << QStringList{
                                                  u"helloWorld"_s,
                                                  u"helloMe"_s,
                                              };

    QTest::newRow("parameterTypeCompletion") << file << 35 << 55 << mixedTypes
                                             << QStringList{
                                                    u"helloWorld"_s,
                                                    u"helloMe"_s,
                                                };

    QTest::newRow("propertyTypeCompletion") << file << 16 << 14 << mixedTypes
                                            << QStringList{
                                                   u"helloWorld"_s,
                                                   u"helloMe"_s,
                                               };
    QTest::newRow("propertyTypeCompletion2") << file << 16 << 23 << mixedTypes
                                             << QStringList{
                                                    u"helloWorld"_s,
                                                    u"helloMe"_s,
                                                };
    QTest::newRow("propertyNameCompletion")
            << file << 16 << 24 << ExpectedCompletions({})
            << QStringList{
                   u"helloWorld"_s, u"helloMe"_s, u"Zzz"_s, u"Item"_s, u"int"_s, u"date"_s,
               };
    QTest::newRow("propertyNameCompletion2")
            << file << 16 << 25 << ExpectedCompletions({})
            << QStringList{
                   u"helloWorld"_s, u"helloMe"_s, u"Zzz"_s, u"Item"_s, u"int"_s, u"date"_s,
               };

    QTest::newRow("propertyDefinitionBinding")
            << file << 90 << 27
            << (ExpectedCompletions({
                        { u"lala"_s, CompletionItemKind::Method },
                        { u"createRectangle"_s, CompletionItemKind::Method },
                        { u"createItem"_s, CompletionItemKind::Method },
                        { u"createAnything"_s, CompletionItemKind::Method },
                }) += constructorTypes)
            << QStringList{
                   u"helloWorld"_s,
                   u"helloMe"_s,
                   u"int"_s,
                   u"date"_s,
               };

    QTest::newRow("ignoreNonRelatedTypesForPropertyDefinitionBinding")
            << file << 16 << 28
            << (ExpectedCompletions({
                        { u"createRectangle"_s, CompletionItemKind::Method },
                        { u"createItem"_s, CompletionItemKind::Method },
                        { u"createAnything"_s, CompletionItemKind::Method },
                }) += rectangleTypes)
            << QStringList{
                   u"Item"_s, u"Zzz"_s,  u"helloWorld"_s, u"helloMe"_s,
                   u"int"_s,  u"date"_s, u"Item"_s,       u"QtObject"_s,
               };

    QTest::newRow("inBoundObject")
            << file << 16 << 40
            << (ExpectedCompletions({
                        { u"objectName"_s, CompletionItemKind::Property },
                        { u"width"_s, CompletionItemKind::Property },
                        { propertyCompletion, CompletionItemKind::Snippet },
                        { functionCompletion, CompletionItemKind::Snippet },
                }) += constructorTypes)
            << QStringList{
                   u"helloWorld"_s, u"helloMe"_s, u"int"_s, u"date"_s, u"QtQuick"_s, u"vector4d"_s,
               };

    QTest::newRow("qualifiedIdentifierCompletion")
            << file << 37 << 36
            << ExpectedCompletions({
                       { u"helloProperty"_s, CompletionItemKind::Property },
                       { u"childAt"_s, CompletionItemKind::Method },
               })
            << QStringList{ u"helloVar"_s, u"someItem"_s, u"color"_s, u"helloWorld"_s,
                            u"propertyOfZZZ"_s };

    QTest::newRow("scriptExpressionCompletion")
            << file << 60 << 16
            << ExpectedCompletions({
                       // parameters
                       { u"jsParameterInChild"_s, CompletionItemKind::Variable },
                       // own properties
                       { u"jsIdentifierInChild"_s, CompletionItemKind::Variable },
                       { u"functionInChild"_s, CompletionItemKind::Method },
                       { u"propertyInChild"_s, CompletionItemKind::Property },
                       // inherited properties from QML
                       { u"functionInBase"_s, CompletionItemKind::Method },
                       { u"propertyInBase"_s, CompletionItemKind::Property },
                       // inherited properties (transitive) from C++
                       { u"objectName"_s, CompletionItemKind::Property },
                       { u"someItem"_s, CompletionItemKind::Value },
                       { u"true"_s, CompletionItemKind::Value },
                       { u"false"_s, CompletionItemKind::Value },
                       { u"null"_s, CompletionItemKind::Value },
               })
            << QStringList{
                   u"helloVar"_s,
                   u"color"_s,
                   u"helloWorld"_s,
                   u"propertyOfZZZ"_s,
                   u"propertyInDerived"_s,
                   u"functionInDerived"_s,
                   u"jsIdentifierInDerived"_s,
                   u"jsIdentifierInBase"_s,
                   u"lala"_s,
                   u"foo"_s,
                   u"jsParameterInBase"_s,
                   u"jsParameterInDerived"_s,
               };

    QTest::newRow("qualifiedScriptExpressionCompletion")
            << file << 60 << 34
            << ExpectedCompletions({
                       // own properties
                       { u"helloProperty"_s, CompletionItemKind::Property },
                       // inherited properties (transitive) from C++
                       { u"width"_s, CompletionItemKind::Property },
               })
            << QStringList{
                   u"helloVar"_s,
                   u"color"_s,
                   u"helloWorld"_s,
                   u"propertyOfZZZ"_s,
                   u"propertyInDerived"_s,
                   u"functionInDerived"_s,
                   u"jsIdentifierInDerived"_s,
                   u"jsIdentifierInBase"_s,
                   u"jsIdentifierInChild"_s,
                   u"lala"_s,
                   u"foo"_s,
                   u"jsParameterInBase"_s,
                   u"jsParameterInDerived"_s,
                   u"jsParameterInChild"_s,
                   u"functionInChild"_s,
               };

    QTest::newRow("pragma") << pragmaFile << 1 << 8
                            << ExpectedCompletions({
                                       { u"NativeMethodBehavior"_s, CompletionItemKind::Value },
                                       { u"ComponentBehavior"_s, CompletionItemKind::Value },
                                       { u"ListPropertyAssignBehavior"_s,
                                         CompletionItemKind::Value },
                                       { u"Singleton"_s, CompletionItemKind::Value },
                                       // note: only complete the Addressible/Inaddressible part of
                                       // ValueTypeBehavior!
                                       { u"ValueTypeBehavior"_s, CompletionItemKind::Value },
                               })
                            << QStringList{
                                   u"int"_s,
                                   u"Rectangle"_s,
                                   u"FunctionSignatureBehavior"_s,
                                   u"Strict"_s,
                               };

    QTest::newRow("pragmaValue") << pragmaFile << 2 << 30
                                 << ExpectedCompletions({
                                            { u"AcceptThisObject"_s, CompletionItemKind::Value },
                                            { u"RejectThisObject"_s, CompletionItemKind::Value },
                                    })
                                 << QStringList{
                                        u"int"_s,
                                        u"Rectangle"_s,
                                        u"FunctionSignatureBehavior"_s,
                                        u"Strict"_s,
                                        u"NativeMethodBehavior"_s,
                                        u"ComponentBehavior"_s,
                                        u"ListPropertyAssignBehavior"_s,
                                        u"Singleton"_s,
                                        u"ValueTypeBehavior"_s,
                                        u"Unbound"_s,
                                    };

    QTest::newRow("pragmaMultiValue")
            << pragmaFile << 3 << 43
            << ExpectedCompletions({
                       { u"ReplaceIfNotDefault"_s, CompletionItemKind::Value },
                       { u"Append"_s, CompletionItemKind::Value },
                       { u"Replace"_s, CompletionItemKind::Value },
               })
            << QStringList{
                   u"int"_s,
                   u"Rectangle"_s,
                   u"FunctionSignatureBehavior"_s,
                   u"Strict"_s,
                   u"NativeMethodBehavior"_s,
                   u"ComponentBehavior"_s,
                   u"ListPropertyAssignBehavior"_s,
                   u"Singleton"_s,
                   u"ValueTypeBehavior"_s,
                   u"Unbound"_s,
               };

    QTest::newRow("pragmaWithoutValue")
            << pragmaFile << 1 << 17
            << ExpectedCompletions({
                       { u"NativeMethodBehavior"_s, CompletionItemKind::Value },
                       { u"ComponentBehavior"_s, CompletionItemKind::Value },
                       { u"ListPropertyAssignBehavior"_s, CompletionItemKind::Value },
                       { u"Singleton"_s, CompletionItemKind::Value },
                       // note: only complete the Addressible/Inaddressible part of
                       // ValueTypeBehavior!
                       { u"ValueTypeBehavior"_s, CompletionItemKind::Value },
               })
            << QStringList{
                   u"int"_s,
                   u"Rectangle"_s,
                   u"FunctionSignatureBehavior"_s,
                   u"Strict"_s,
               };

    QTest::newRow("non-block-scoped-variable")
            << file << 69 << 21
            << ExpectedCompletions({
                       { u"helloVarVariable"_s, CompletionItemKind::Variable },
               })
            << QStringList{};
    QTest::newRow("block-scoped-variable")
            << file << 76 << 21 << ExpectedCompletions{ { u"test2"_s, CompletionItemKind::Method } }
            << QStringList{ u"helloLetVariable"_s, u"helloVarVariable"_s };

    QTest::newRow("singleton") << file << 78 << 33
                               << ExpectedCompletions({
                                          { singletonName, CompletionItemKind::Class },
                                  })
                               << QStringList{};

    QTest::newRow("singletonPropertyAndEnums")
            << file << 78 << 52
            << ExpectedCompletions({
                       { u"byteOrder"_s, CompletionItemKind::Property },
                       { u"Little"_s, CompletionItemKind::EnumMember },
                       { u"Endian"_s, CompletionItemKind::Enum },
               })
            << QStringList{
                   u"int"_s,
                   u"Rectangle"_s,
                   u"foo"_s,
               };

    QTest::newRow("enumsFromItem") << file << 86 << 33
                                   << ExpectedCompletions({
                                              { u"World"_s, CompletionItemKind::EnumMember },
                                              { u"ValueOne"_s, CompletionItemKind::EnumMember },
                                              { u"ValueTwo"_s, CompletionItemKind::EnumMember },
                                              { u"Hello"_s, CompletionItemKind::Enum },
                                              { u"MyEnum"_s, CompletionItemKind::Enum },
                                      })
                                   << QStringList{
                                          u"int"_s,
                                          u"Rectangle"_s,
                                      };

    QTest::newRow("enumsFromEnumName")
            << file << 87 << 40
            << ExpectedCompletions({
                       { u"World"_s, CompletionItemKind::EnumMember },
               })
            << QStringList{
                   u"int"_s,      u"Rectangle"_s, u"foo"_s,    u"ValueOne"_s,
                   u"ValueTwo"_s, u"Hello"_s,     u"MyEnum"_s,
               };

    QTest::newRow("requiredProperty")
            << file << 97 << 14
            << ExpectedCompletions({
                       { u"property"_s, CompletionItemKind::Keyword },
                       { u"default"_s, CompletionItemKind::Keyword },
               })
            << QStringList{
                   u"readonly"_s, u"required"_s, u"int"_s,   u"Rectangle"_s, u"foo"_s,
                   u"ValueOne"_s, u"ValueTwo"_s, u"Hello"_s, u"MyEnum"_s,
               };

    QTest::newRow("readonlyProperty")
            << file << 98 << 13
            << ExpectedCompletions({
                       { u"property"_s, CompletionItemKind::Keyword },
                       { u"default"_s, CompletionItemKind::Keyword },
               })
            << QStringList{
                   u"required"_s, u"readonly"_s, u"int"_s,   u"Rectangle"_s, u"foo"_s,
                   u"ValueOne"_s, u"ValueTwo"_s, u"Hello"_s, u"MyEnum"_s,
               };

    QTest::newRow("defaultProperty")
            << file << 99 << 12
            << ExpectedCompletions({
                       { u"property"_s, CompletionItemKind::Keyword },
                       { u"readonly"_s, CompletionItemKind::Keyword },
                       { u"required"_s, CompletionItemKind::Keyword },
               })
            << QStringList{
                   u"default"_s,  u"int"_s,      u"Rectangle"_s, u"foo"_s,
                   u"ValueOne"_s, u"ValueTwo"_s, u"Hello"_s,     u"MyEnum"_s,
               };

    QTest::newRow("defaultProperty2")
            << file << 99 << 20
            << ExpectedCompletions({
                       { u"property"_s, CompletionItemKind::Keyword },
                       { u"readonly"_s, CompletionItemKind::Keyword },
                       { u"required"_s, CompletionItemKind::Keyword },
               })
            << QStringList{
                   u"default"_s,  u"int"_s,      u"Rectangle"_s, u"foo"_s,
                   u"ValueOne"_s, u"ValueTwo"_s, u"Hello"_s,     u"MyEnum"_s,
               };

    QTest::newRow("defaultProperty3")
            << file << 99 << 21 << ExpectedCompletions{ { u"int"_s, CompletionItemKind::Class } }
            << QStringList{
                   u"property"_s,
                   u"readonly"_s,
                   u"required"_s,
               };

    const QString forStatementCompletion = u"for (initializer; condition; increment) { statements... }"_s;
    const QString ifStatementCompletion = u"if (condition) statement"_s;
    const QString letStatementCompletion = u"let variable = value;"_s;
    const QString constStatementCompletion = u"const variable = value;"_s;
    const QString varStatementCompletion = u"var variable = value;"_s;

    // for the for loop
    const QString letStatementCompletionWithoutSemicolon = letStatementCompletion.chopped(1);
    const QString constStatementCompletionWithoutSemicolon = constStatementCompletion.chopped(1);
    const QString varStatementCompletionWithoutSemicolon = varStatementCompletion.chopped(1);

    const QString caseStatementCompletion = u"case value: statements..."_s;
    const QString caseStatement2Completion = u"case value: { statements... }"_s;
    const QString defaultStatementCompletion = u"default: statements..."_s;
    const QString defaultStatement2Completion = u"default: { statements... }"_s;

    // warning: the completion strings in the test below were all tested by hand in VS Code to
    // make sure they are easy to use. Make sure to check the code snippets by hand before changing
    // them.
    QTest::newRow("jsStatements")
            << file << 104 << 1
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet,
                                      u"let ${1:variable} = $0;"_s },
                                    { u"const variable = value;"_s, CompletionItemKind::Snippet,
                                      u"const ${1:variable} = $0;"_s },
                                    { u"var variable = value;"_s, CompletionItemKind::Snippet,
                                      u"var ${1:variable} = $0;"_s },
                                    { u"{ statements... }"_s, CompletionItemKind::Snippet,
                                      u"{\n\t$0\n}"_s },
                                    { u"if (condition) { statements }"_s,
                                      CompletionItemKind::Snippet, u"if ($1) {\n\t$0\n}"_s },
                                    { u"do { statements } while (condition);"_s,
                                      CompletionItemKind::Snippet, u"do {\n\t$1\n} while ($0);"_s },
                                    { u"while (condition) { statements...}"_s,
                                      CompletionItemKind::Snippet, u"while ($1) {\n\t$0\n}"_s },
                                    { forStatementCompletion,
                                      CompletionItemKind::Snippet, u"for ($1;$2;$3) {\n\t$0\n}"_s },
                                    { u"try { statements... } catch(error) { statements... }"_s,
                                      CompletionItemKind::Snippet, u"try {\n\t$1\n} catch($2) {\n\t$0\n}"_s },
                                    { u"try { statements... } finally { statements... }"_s,
                                      CompletionItemKind::Snippet, u"try {\n\t$1\n} finally {\n\t$0\n}"_s },
                                    { u"try { statements... } catch(error) { statements... } finally { statements... }"_s,
                                      CompletionItemKind::Snippet, u"try {\n\t$1\n} catch($2) {\n\t$3\n} finally {\n\t$0\n}"_s },
                                    { u"for (property in object) { statements... }"_s,
                                      CompletionItemKind::Snippet, u"for ($1 in $2) {\n\t$0\n}"_s },
                                    { u"for (element of array) { statements... }"_s,
                                      CompletionItemKind::Snippet, u"for ($1 of $2) {\n\t$0\n}"_s },
                                    { u"continue"_s, CompletionItemKind::Keyword },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ caseStatementCompletion,
                            caseStatement2Completion,
                            defaultStatementCompletion,
                            defaultStatement2Completion,
               };

    QTest::newRow("forStatementLet")
            << file << 103 << 13
            << ExpectedCompletions{ { letStatementCompletionWithoutSemicolon,
                                      CompletionItemKind::Snippet, u"let ${1:variable} = $0"_s },
                                    { constStatementCompletionWithoutSemicolon,
                                      CompletionItemKind::Snippet, u"const ${1:variable} = $0"_s },
                                    { varStatementCompletionWithoutSemicolon,
                                      CompletionItemKind::Snippet, u"var ${1:variable} = $0"_s },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method } }
            << QStringList{ u"property"_s,
                            u"readonly"_s,
                            u"required"_s,
                            forStatementCompletion,
                            ifStatementCompletion,
                            letStatementCompletion,
                            constStatementCompletion,
                            varStatementCompletion };

    QTest::newRow("forStatementCondition")
            << file << 103 << 25
            << ExpectedCompletions{
                   { u"helloJSStatements"_s, CompletionItemKind::Method },
                   { u"i"_s, CompletionItemKind::Variable },
               }
            << QStringList{ u"property"_s,          u"readonly"_s,           u"required"_s,
                            forStatementCompletion, ifStatementCompletion,   varStatementCompletion,
                            letStatementCompletion, constStatementCompletion, }
           ;

    QTest::newRow("forStatementIncrement")
            << file << 103 << 30
            << ExpectedCompletions{
                   { u"helloJSStatements"_s, CompletionItemKind::Method },
                   { u"i"_s, CompletionItemKind::Variable },
                }
            << QStringList{ u"property"_s,          u"readonly"_s,           u"required"_s,
                            forStatementCompletion, ifStatementCompletion,   varStatementCompletion,
                            letStatementCompletion, constStatementCompletion, }
           ;

    QTest::newRow("forStatementIncrement2")
            << file << 103 << 33
            << ExpectedCompletions{ { u"helloJSStatements"_s, CompletionItemKind::Method } }
            << QStringList{
                   u"property"_s,          u"readonly"_s,
                   u"required"_s,          forStatementCompletion,
                   ifStatementCompletion,  varStatementCompletion,
                   letStatementCompletion, constStatementCompletion,
               };

    QTest::newRow("forStatementWithoutBlock")
            << file << 107 << 12
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method },
                                    { u"j"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("blockStatementBeforeBracket")
            << file << 103 << 36
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method },
                                    { u"i"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("blockStatementAfterBracket")
            << file << 103 << 37
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("ifStatementCondition")
            << file << 110 << 15
            << ExpectedCompletions{
                                    { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ propertyCompletion, letStatementCompletion, constStatementCompletion }
           ;

    QTest::newRow("ifStatementConsequence")
            << file << 111 << 12
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ propertyCompletion }
           ;

    QTest::newRow("ifStatementAlternative")
            << file << 113 << 12
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ propertyCompletion }
           ;

    QTest::newRow("binaryExpressionCompletionInsideStatement")
            << file << 113 << 21
            << ExpectedCompletions{  { u"hello"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, forStatementCompletion }
           ;

    QTest::newRow("elseIfStatement")
            << file << 121 << 18
            << ExpectedCompletions{  { u"hello"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, letStatementCompletion, ifStatementCompletion }
           ;
    QTest::newRow("returnStatement")
            << file << 125 << 16
            << ExpectedCompletions{ { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;
    QTest::newRow("returnStatement2")
            << testFile("completions/returnStatement.qml") << 8 << 15
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("whileCondition")
            << file << 128 << 16
            << ExpectedCompletions{ { u"hello"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("whileConsequence")
            << file << 128 << 22
            << ExpectedCompletions{ { u"hello"_s, CompletionItemKind::Variable },
                                    { letStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("doWhileCondition")
            << file << 131 << 30
            << ExpectedCompletions{ { u"hello"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("doWhileConsequence")
            << file << 131 << 12
            << ExpectedCompletions{ { u"hello"_s, CompletionItemKind::Variable },
                                    { letStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("forInStatementLet")
            << file << 134 << 13
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method } }
            << QStringList{
                   u"property"_s,          u"readonly"_s,         u"required"_s,
                   forStatementCompletion, ifStatementCompletion,
               };

    QTest::newRow("forOfStatementLet")
            << file << 135 << 13
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method } }
            << QStringList{
                   u"property"_s,          u"readonly"_s,         u"required"_s,
                   forStatementCompletion, ifStatementCompletion,
               };

    QTest::newRow("forInStatementTarget")
            << file << 134 << 25
            << ExpectedCompletions{
                   { u"helloJSStatements"_s, CompletionItemKind::Method },
                   { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ u"property"_s,          u"readonly"_s,           u"required"_s,
                            forStatementCompletion, ifStatementCompletion,   varStatementCompletion,
                            letStatementCompletion, constStatementCompletion, }
           ;

    QTest::newRow("forOfStatementTarget")
            << file << 135 << 24
            << ExpectedCompletions{
                   { u"helloJSStatements"_s, CompletionItemKind::Method },
                   { u"hello"_s, CompletionItemKind::Variable },
               }
            << QStringList{ u"property"_s,          u"readonly"_s,           u"required"_s,
                            forStatementCompletion, ifStatementCompletion,   varStatementCompletion,
                            letStatementCompletion, constStatementCompletion, }
           ;

    QTest::newRow("forInStatementConsequence")
            << file << 134 << 31
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method },
                                    { u"hello"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("forOfStatementConsequence")
            << file << 135 << 30
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { constStatementCompletion, CompletionItemKind::Snippet },
                                    { varStatementCompletion, CompletionItemKind::Snippet },
                                    { u"helloJSStatements"_s, CompletionItemKind::Method },
                                    { u"hello"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("binaryExpressionRHS") << file << 138 << 17
                                 << ExpectedCompletions{
                                        { u"log"_s, CompletionItemKind::Method },
                                        { u"error"_s, CompletionItemKind::Method },
                                    }
                                 << QStringList{ propertyCompletion, u"helloVarVariable"_s,
                                                 u"test1"_s,         u"width"_s,
                                                 u"height"_s,        u"layer"_s,
                                                 u"left"_s, forStatementCompletion }
                                ;
    QTest::newRow("binaryExpressionLHS") << file << 138 << 12
                                 << ExpectedCompletions{
                                        { u"qualifiedScriptIdentifiers"_s, CompletionItemKind::Method },
                                        { u"width"_s, CompletionItemKind::Property },
                                        { u"layer"_s, CompletionItemKind::Property },
                                    }
                                 << QStringList{ u"log"_s, u"error"_s, forStatementCompletion}
                                ;

    const QString missingRHSFile = testFile(u"completions/missingRHS.qml"_s);
    QTest::newRow("binaryExpressionMissingRHS") << missingRHSFile << 12 << 25
                                 << ExpectedCompletions{
                                        { u"good"_s, CompletionItemKind::Property },
                                    }
                                 << QStringList{ propertyCompletion, u"bad"_s }
                                ;
    QTest::newRow("binaryExpressionMissingRHSWithDefaultProperty") << missingRHSFile << 14 << 33
                                 << ExpectedCompletions{
                                        { u"good"_s, CompletionItemKind::Property },
                                    }
                                 << QStringList{ propertyCompletion, u"bad"_s, u"helloSubItem"_s }
                                ;

    QTest::newRow("binaryExpressionMissingRHSWithSemicolon")
            << testFile(u"completions/missingRHS.parserfail.qml"_s)
                                                             << 5 << 22
                                 << ExpectedCompletions{
                                        { u"good"_s, CompletionItemKind::Property },
                                    }
                                 << QStringList{ propertyCompletion, u"bad"_s, u"helloSubItem"_s }
                                ;

    QTest::newRow("binaryExpressionMissingRHSWithStatement") <<
            testFile(u"completions/missingRHS.parserfail.qml"_s)
                                                             << 6 << 22
                                 << ExpectedCompletions{
                                        { u"good"_s, CompletionItemKind::Property },
                                    }
                                 << QStringList{ propertyCompletion, u"bad"_s, u"helloSubItem"_s }
                                ;

    QTest::newRow("tryStatements")
            << testFile(u"completions/tryStatements.qml"_s) << 5 << 14
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{};

    QTest::newRow("tryStatementsCatchParameter")
            << testFile(u"completions/tryStatements.qml"_s) << 5 << 23 << ExpectedCompletions{}
            << QStringList{ letStatementCompletion, forStatementCompletion };

    QTest::newRow("tryStatementsCatchBlock")
            << testFile(u"completions/tryStatements.qml"_s) << 5 << 27
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{};

    QTest::newRow("tryStatementsFinallyBlock")
            << testFile(u"completions/tryStatements.qml"_s) << 5 << 39
            << ExpectedCompletions{ { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{};

    QTest::newRow("inSwitchExpression")
            << testFile(u"completions/switchStatements.qml"_s) << 10 << 16
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   letStatementCompletion,
                   propertyCompletion,
               };

    QTest::newRow("beforeCaseStatement")
            << testFile(u"completions/switchStatements.qml"_s) << 11 << 1
            << ExpectedCompletions{ { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { caseStatement2Completion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatement2Completion, CompletionItemKind::Snippet } }
            << QStringList{ letStatementCompletion, propertyCompletion, u"x"_s, u"myProperty"_s };
    QTest::newRow("inCaseExpression")
            << testFile(u"completions/switchStatements.qml"_s) << 12 << 14
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{ letStatementCompletion, propertyCompletion, caseStatementCompletion };
    QTest::newRow("inCaseStatementList")
            << testFile(u"completions/switchStatements.qml"_s) << 13 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"return"_s, CompletionItemKind::Keyword },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{ propertyCompletion };
    QTest::newRow("inDefaultStatementList")
            << testFile(u"completions/switchStatements.qml"_s) << 24 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"return"_s, CompletionItemKind::Keyword },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{ propertyCompletion };
    QTest::newRow("inMoreCasesStatementList")
            << testFile(u"completions/switchStatements.qml"_s) << 26 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"return"_s, CompletionItemKind::Keyword },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{ propertyCompletion };

    QTest::newRow("inCaseBeforeBlock")
            << testFile(u"completions/switchStatements.qml"_s) << 14 << 23
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };
    QTest::newRow("inCaseBeforeBlock2")
            << testFile(u"completions/switchStatements.qml"_s) << 14 << 24
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };

    QTest::newRow("inCaseNestedStatement")
            << testFile(u"completions/switchStatements.qml"_s) << 16 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{ propertyCompletion, caseStatementCompletion,
                            defaultStatementCompletion };

    QTest::newRow("inCaseAfterBlock")
            << testFile(u"completions/switchStatements.qml"_s) << 22 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };

    QTest::newRow("inCaseBeforeDefault")
            << testFile(u"completions/switchStatements.qml"_s) << 23 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };

    QTest::newRow("inCaseAfterDefault")
            << testFile(u"completions/switchStatements.qml"_s) << 25 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };

    QTest::newRow("beforeAnyCase")
            << testFile(u"completions/switchStatements.qml"_s) << 20 << 1
            << ExpectedCompletions{ { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion, letStatementCompletion, u"myProperty"_s, u"x"_s,
                            u"f"_s };

    QTest::newRow("beforeAnyDefault")
            << testFile(u"completions/switchStatements.qml"_s) << 32 << 1
            << ExpectedCompletions{ { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion, letStatementCompletion, u"myProperty"_s, u"x"_s,
                            u"f"_s };
    QTest::newRow("inDefaultAfterDefault")
            << testFile(u"completions/switchStatements.qml"_s) << 33 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { caseStatementCompletion, CompletionItemKind::Snippet },
                                    { defaultStatementCompletion, CompletionItemKind::Snippet },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { u"myProperty"_s, CompletionItemKind::Property } }
            << QStringList{
                   propertyCompletion,
               };

    // variableDeclaration.qml tests for let/const/var statements + destructuring

    QTest::newRow("letStatement") << testFile(u"completions/variableDeclaration.qml"_s) << 7 << 13
                                  << ExpectedCompletions{}
                                  << QStringList{ propertyCompletion, letStatementCompletion,
                                                  u"x"_s, u"data"_s };

    QTest::newRow("letStatement2")
            << testFile(u"completions/variableDeclaration.qml"_s) << 7 << 26
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("letStatementBehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 7 << 28
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("letStatementBehindEqual2")
            << testFile(u"completions/variableDeclaration.qml"_s) << 7 << 33
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("constStatement")
            << testFile(u"completions/variableDeclaration.qml"_s) << 8 << 19
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("constStatementBehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 8 << 32
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("varStatement") << testFile(u"completions/variableDeclaration.qml"_s) << 9 << 17
                                  << ExpectedCompletions{}
                                  << QStringList{ propertyCompletion, letStatementCompletion,
                                                  u"x"_s, u"data"_s };

    QTest::newRow("varStatementBehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 9 << 28
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstruction")
            << testFile(u"completions/variableDeclaration.qml"_s) << 13 << 20
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("objectDeconstructionAloneBehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 14 << 51
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstructionAloneBehindEqual2")
            << testFile(u"completions/variableDeclaration.qml"_s) << 14 << 58
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstruction2BehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 83
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstruction2BehindEqual2")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 90
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstruction3BehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 140
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstructionBehindComma")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 143
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("objectDeconstructionBetweenObjects")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 50
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("objectDeconstructionBetweenDeconstructions")
            << testFile(u"completions/variableDeclaration.qml"_s) << 15 << 97
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("arrayDeconstructionAlone")
            << testFile(u"completions/variableDeclaration.qml"_s) << 19 << 24
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("arrayDeconstructionAloneBehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 19 << 33
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("arrayDeconstruction2")
            << testFile(u"completions/variableDeclaration.qml"_s) << 21 << 71
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("arrayDeconstruction2BehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 21 << 83
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;
    QTest::newRow("arrayDeconstruction3")
            << testFile(u"completions/variableDeclaration.qml"_s) << 21 << 125
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("arrayDeconstruction3BehindEqual")
            << testFile(u"completions/variableDeclaration.qml"_s) << 21 << 139
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("arrayDeconstructionIn_Wildcard")
            << testFile(u"completions/variableDeclaration.qml"_s) << 25 << 64
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("arrayDeconstructionBehind+")
            << testFile(u"completions/variableDeclaration.qml"_s) << 25 << 132
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("objectDeconstructionForNeedle")
            << testFile(u"completions/variableDeclaration.qml"_s) << 29 << 111
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("arrayInObjectDeconstructionInObjectInitializer")
            << testFile(u"completions/variableDeclaration.qml"_s) << 33 << 44
            << ExpectedCompletions{ {u"x"_s, CompletionItemKind::Variable},
                                    {u"data"_s, CompletionItemKind::Method},
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion }
           ;

    QTest::newRow("arrayInObjectDeconstructionInObjectPropertyName")
            << testFile(u"completions/variableDeclaration.qml"_s) << 33 << 26
            << ExpectedCompletions{}
            << QStringList{ propertyCompletion, letStatementCompletion, u"x"_s, u"data"_s };

    QTest::newRow("throwStatement")
            << testFile(u"completions/throwStatement.qml"_s) << 8 << 15
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("throwStatement2")
            << testFile(u"completions/throwStatement.qml"_s) << 9 << 20
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("labelledStatement")
            << testFile(u"completions/labelledStatement.qml"_s) << 5 << 16
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet },
                                    }
            << QStringList{ propertyCompletion, };

    QTest::newRow("nestedLabel")
            << testFile(u"completions/labelledStatement.qml"_s) << 7 << 22
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet },
                                    }
            << QStringList{ propertyCompletion, };

    QTest::newRow("nestedLabel2")
            << testFile(u"completions/labelledStatement.qml"_s) << 8 << 26
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet },
                                    }
            << QStringList{ propertyCompletion, };

    QTest::newRow("multiLabel")
            << testFile(u"completions/labelledStatement.qml"_s) << 15 << 21
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet },
                                    }
            << QStringList{ propertyCompletion, };

    QTest::newRow("multiLabel2")
            << testFile(u"completions/labelledStatement.qml"_s) << 16 << 21
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"f"_s, CompletionItemKind::Method },
                                    { letStatementCompletion, CompletionItemKind::Snippet },
                                    { forStatementCompletion, CompletionItemKind::Snippet },
                                    }
            << QStringList{ propertyCompletion, };

    QTest::newRow("continueNested")
            << testFile(u"completions/continueAndBreakStatement.qml"_s) << 12 << 26
            << ExpectedCompletions{ { u"nestedLabel1"_s, CompletionItemKind::Value },
                                    { u"nestedLabel2"_s, CompletionItemKind::Value }, }
            << QStringList{ propertyCompletion, u"x"_s, u"f"_s, u"multiLabel1"_s };

    QTest::newRow("breakNested")
            << testFile(u"completions/continueAndBreakStatement.qml"_s) << 13 << 23
            << ExpectedCompletions{ { u"nestedLabel1"_s, CompletionItemKind::Value },
                                    { u"nestedLabel2"_s, CompletionItemKind::Value }, }
            << QStringList{ propertyCompletion, u"x"_s, u"f"_s, u"multiLabel1"_s };

    QTest::newRow("continueMulti")
            << testFile(u"completions/continueAndBreakStatement.qml"_s) << 20 << 22
            << ExpectedCompletions{ { u"multiLabel1"_s, CompletionItemKind::Value },
                                    { u"multiLabel2"_s, CompletionItemKind::Value }, }
            << QStringList{ propertyCompletion, u"x"_s, u"f"_s, u"nestedLabel1"_s };

    QTest::newRow("breakMulti")
            << testFile(u"completions/continueAndBreakStatement.qml"_s) << 21 << 19
            << ExpectedCompletions{ { u"multiLabel1"_s, CompletionItemKind::Value },
                                    { u"multiLabel2"_s, CompletionItemKind::Value }, }
            << QStringList{ propertyCompletion, u"x"_s, u"f"_s, u"nestedLabel1"_s };

    QTest::newRow("continueNoLabel") << testFile(u"completions/continueAndBreakStatement.qml"_s)
                                     << 25 << 22 << ExpectedCompletions{}
                                     << QStringList{ propertyCompletion, u"x"_s, u"f"_s,
                                                     u"nestedLabel1"_s, u"multiLabel1"_s };

    QTest::newRow("breakNoLabel") << testFile(u"completions/continueAndBreakStatement.qml"_s) << 26
                                  << 19 << ExpectedCompletions{}
                                  << QStringList{ propertyCompletion, u"x"_s, u"f"_s,
                                                  u"nestedLabel1"_s, u"multiLabel1"_s };

    QTest::newRow("insideMethodBody")
            << testFile(u"completions/functionBody.qml"_s) << 5 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ propertyCompletion };

    QTest::newRow("insideMethodBody2")
            << testFile(u"completions/functionBody.qml"_s) << 11 << 11
            << ExpectedCompletions{ { u"helloProperty"_s, CompletionItemKind::Property }, }
            << QStringList{ u"badProperty"_s, forStatementCompletion };

    QTest::newRow("insideArrow")
            << testFile(u"completions/functionBody.qml"_s) << 12 << 22
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable }, }
            << QStringList{ propertyCompletion, forStatementCompletion };

    QTest::newRow("insideArrow2")
            << testFile(u"completions/functionBody.qml"_s) << 12 << 26
            << ExpectedCompletions{ { u"helloProperty"_s, CompletionItemKind::Property }, }
            << QStringList{ u"badProperty"_s, forStatementCompletion };

    QTest::newRow("insideMethodBodyStart")
            << testFile(u"completions/functionBody.qml"_s) << 11 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ u"helloProperty"_s };

    QTest::newRow("insideMethodBodyEnd")
            << testFile(u"completions/functionBody.qml"_s) << 12 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { forStatementCompletion, CompletionItemKind::Snippet } }
            << QStringList{ u"helloProperty"_s };

    QTest::newRow("insideArrowBody")
            << testFile(u"completions/functionBody.qml"_s) << 14 << 24
            << ExpectedCompletions{ { u"xxx"_s, CompletionItemKind::Variable } }
            << QStringList{ u"helloProperty"_s };

    QTest::newRow("noBreakInMethodBody")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 8 << 8
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable } }
            << QStringList{ u"break"_s, u"continue"_s };

    QTest::newRow("breakAndContinueInForLoop")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 11 << 12
            << ExpectedCompletions{ { u"break"_s, CompletionItemKind::Keyword },
                                    { u"continue"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{};

    QTest::newRow("noBreakInSwitch")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 15 << 12
            << ExpectedCompletions{ { caseStatementCompletion, CompletionItemKind::Snippet }, }
            << QStringList{ u"continue"_s, u"break"_s };

    QTest::newRow("breakInSwitchCase")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 17 << 12
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ u"continue"_s };

    QTest::newRow("breakInSwitchDefault")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 19 << 12
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ u"continue"_s };

    QTest::newRow("breakInSwitchSecondCase")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 21 << 12
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ u"continue"_s };

    QTest::newRow("breakInLabel")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 25 << 12
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ u"continue"_s };

    QTest::newRow("forLoopInsideOfLabel")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 33 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"continue"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ };

    QTest::newRow("switchInsideForLoopInsideOfLabel")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 36 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"continue"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ };

    QTest::newRow("switchInsideOfLabel")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 45 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ u"continue"_s };

    QTest::newRow("forLoopInSwitchInsideOfLabel")
            << testFile(u"completions/suggestContinueAndBreak.qml"_s) << 47 << 1
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    { u"break"_s, CompletionItemKind::Keyword },
                                    { u"continue"_s, CompletionItemKind::Keyword },
                                    }
            << QStringList{ };

    QTest::newRow("commaExpression")
            << testFile(u"completions/commaExpression.qml"_s) << 5 << 18
            << ExpectedCompletions{ { u"a"_s, CompletionItemKind::Variable },
                                    { u"b"_s, CompletionItemKind::Variable },
                                    { u"c"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion };

    QTest::newRow("conditionalExpressionConsequence")
            << testFile(u"completions/conditionalExpression.qml"_s) << 5 << 17
            << ExpectedCompletions{ { u"a"_s, CompletionItemKind::Variable },
                                    { u"b"_s, CompletionItemKind::Variable },
                                    { u"c"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("conditionalExpressionAlternative")
            << testFile(u"completions/conditionalExpression.qml"_s) << 5 << 30
            << ExpectedCompletions{ { u"a"_s, CompletionItemKind::Variable },
                                    { u"b"_s, CompletionItemKind::Variable },
                                    { u"c"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("unaryMinus")
            << testFile(u"completions/unaryExpression.qml"_s) << 5 << 10
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("unaryPlus")
            << testFile(u"completions/unaryExpression.qml"_s) << 6 << 10
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("unaryTilde")
            << testFile(u"completions/unaryExpression.qml"_s) << 7 << 10
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("unaryNot")
            << testFile(u"completions/unaryExpression.qml"_s) << 8 << 10
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("typeof")
            << testFile(u"completions/unaryExpression.qml"_s) << 9 << 16
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("delete")
            << testFile(u"completions/unaryExpression.qml"_s) << 10 << 16
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("void")
            << testFile(u"completions/unaryExpression.qml"_s) << 11 << 14
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("postDecrement")
            << testFile(u"completions/unaryExpression.qml"_s) << 12 << 9
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("postIncrement")
            << testFile(u"completions/unaryExpression.qml"_s) << 13 << 9
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("preDecrement")
            << testFile(u"completions/unaryExpression.qml"_s) << 14 << 11
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("preIncrement")
            << testFile(u"completions/unaryExpression.qml"_s) << 15 << 11
            << ExpectedCompletions{ { u"x"_s, CompletionItemKind::Variable },
                                    }
            << QStringList{ propertyCompletion, letStatementCompletion };

    QTest::newRow("attachedPropertyAfterDot")
            << testFile("completions/attachedAndGroupedProperty.qml") << 8 << 15
            << ExpectedCompletions({
                       { u"onCompleted"_s, CompletionItemKind::Method },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s, u"x"_s };

    QTest::newRow("attachedPropertyAfterDot2")
            << testFile("completions/attachedAndGroupedProperty.qml") << 13 << 34
            << ExpectedCompletions({
                       { u"completed"_s, CompletionItemKind::Method },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s, u"progress"_s, u"status"_s, u"createObject"_s, u"x"_s };

    QTest::newRow("groupedPropertyAfterDot")
            << testFile("completions/attachedAndGroupedProperty.qml") << 10 << 14
            << ExpectedCompletions({
                       { u"family"_s, CompletionItemKind::Property },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s, u"onCompleted"_s };

    QTest::newRow("attachedPropertyAfterDotMissingRHS")
            << testFile("completions/attachedPropertyMissingRHS.qml") << 7 << 16
            << ExpectedCompletions({
                       { u"onCompleted"_s, CompletionItemKind::Method },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s };

    QTest::newRow("QTBUG-127586")
            << testFile("completions/attachedPropertyQTBUG127586.qml") << 4 << 31
            << ExpectedCompletions({
                       { u"completed"_s, CompletionItemKind::Method },
                       { u"destruction"_s, CompletionItemKind::Method },
                       { u"Asynchronous"_s, CompletionItemKind::EnumMember },
                       { u"loadFromModule"_s, CompletionItemKind::Method},
               })
            << QStringList{ u"ItemClipsChildrenToState"_s, u"Top"_s, u"parent"_s };

    QTest::newRow("groupedPropertyAfterDotMissingRHS")
            << testFile("completions/groupedPropertyMissingRHS.qml") << 7 << 11
            << ExpectedCompletions({
                       { u"family"_s, CompletionItemKind::Property },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s, u"onCompleted"_s };

    QTest::newRow("dotFollowedByDefaultBinding")
            << testFile("completions/afterDots.qml") << 11 << 31
            << ExpectedCompletions({
                       { u"good"_s, CompletionItemKind::Property },
               })
            << QStringList{ u"bad"_s,         u"QtQuick"_s,   u"vector4d"_s,
                            attachedTypeName, u"Rectangle"_s, u"onCompleted"_s };

    QTest::newRow("dotFollowedByBinding")
            << testFile("completions/afterDots.qml") << 13 << 32
            << ExpectedCompletions({
                       { u"good"_s, CompletionItemKind::Property },
               })
            << QStringList{ u"bad"_s,         u"QtQuick"_s,   u"vector4d"_s,
                            attachedTypeName, u"Rectangle"_s, u"onCompleted"_s };

    QTest::newRow("dotFollowedByForStatement")
            << testFile("completions/afterDots.qml") << 16 << 16
            << ExpectedCompletions({
                       { u"good"_s, CompletionItemKind::Property },
               })
            << QStringList{
                   u"bad"_s,       u"QtQuick"_s,     u"vector4d"_s,         attachedTypeName,
                   u"Rectangle"_s, u"onCompleted"_s, forStatementCompletion
               };

    QTest::newRow("qualifiedTypeCompletionWithoutQualifier")
            << testFile("completions/quickcontrols_and_quicktemplates/qualifiedTypesCompletion.qml")
            << 9 << 5
            << ExpectedCompletions({
                       { u"T.Button"_s, CompletionItemKind::Constructor },
                       { u"Button"_s, CompletionItemKind::Constructor },
                       { u"Rectangle"_s, CompletionItemKind::Constructor },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s, u"bad"_s, u"onCompleted"_s };

    QTest::newRow("qualifiedTypeCompletionWithoutQualifier2")
            << testFile("completions/quickcontrols_and_quicktemplates/qualifiedTypesCompletion.qml")
            << 10 << 19
            << ExpectedCompletions({
                       { u"T.Button"_s, CompletionItemKind::Class },
                       { u"Button"_s, CompletionItemKind::Class },
                       { u"Rectangle"_s, CompletionItemKind::Class },
               })
            << QStringList{ u"QtQuick"_s, u"bad"_s, u"onCompleted"_s };

    QTest::newRow("qualifiedTypeCompletionWithQualifier")
            << testFile("completions/quickcontrols_and_quicktemplates/qualifiedTypesCompletion.qml")
            << 9 << 7
            << ExpectedCompletions({
                       { u"Button"_s, CompletionItemKind::Constructor },
               })
            << QStringList{ u"QtQuick"_s, u"vector4d"_s,    attachedTypeName, u"Rectangle"_s,
                            u"bad"_s,     u"onCompleted"_s, u"T.Button"_s };

    QTest::newRow("qualifiedTypeCompletionWithQualifier2")
            << testFile("completions/quickcontrols_and_quicktemplates/qualifiedTypesCompletion.qml")
            << 10 << 21
            << ExpectedCompletions({
                       { u"Button"_s, CompletionItemKind::Class },
               })
            << QStringList{ u"QtQuick"_s, attachedTypeName, u"Rectangle"_s,
                            u"bad"_s,     u"onCompleted"_s, u"T.Button"_s };

    QTest::newRow("parenthesizedExpression")
            << testFile("completions/parenthesizedExpression.qml") << 8 << 10
            << ExpectedCompletions({
                       { u"x"_s, CompletionItemKind::Variable },
               })
            << QStringList{ u"QtQuick"_s, u"Rectangle"_s, forStatementCompletion };

    QTest::newRow("behindParenthesizedExpression")
            << testFile("completions/parenthesizedExpression.qml") << 8 << 16
            << ExpectedCompletions({})
            << QStringList{ u"QtQuick"_s, attachedTypeName, u"Rectangle"_s, forStatementCompletion,
                            u"x"_s };

    QTest::newRow("assumeBoundComponentsIdFromParent")
            << testFile("completions/boundComponents.qml") << 14 << 33
            << ExpectedCompletions{ { u"rootId"_s, CompletionItemKind::Value } }
            << QStringList{ u"inRoot"_s };

    QTest::newRow("assumeBoundComponentsPropertyFromParent")
            << testFile("completions/boundComponents.qml") << 14 << 40
            << ExpectedCompletions{ { u"inRoot"_s, CompletionItemKind::Property } }
            << QStringList{ u"root"_s };

    QTest::newRow("insideTemplateLiteralExpression")
            << testFile("completions/templateLiterals.qml") << 9 << 27
            << ExpectedCompletions{ { u"helloProperty"_s, CompletionItemKind::Property } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideTemplateLiteralString")
            << testFile("completions/templateLiterals.qml") << 9 << 40
            << ExpectedCompletions{ }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideTaggedLiteralExpression")
            << testFile("completions/templateLiterals.qml") << 10 << 27
            << ExpectedCompletions{ { u"helloProperty"_s, CompletionItemKind::Property } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideTaggedLiteralString")
            << testFile("completions/templateLiterals.qml") << 10 << 40
            << ExpectedCompletions{ }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideRegularExpression")
            << testFile("completions/regularExpressions.qml") << 5 << 20
            << ExpectedCompletions{ }
            << QStringList{};

    QTest::newRow("insideRegularExpressionFlag")
            << testFile("completions/regularExpressions.qml") << 5 << 25
            << ExpectedCompletions{ }
            << QStringList{};

    QTest::newRow("insideNewMemberExpression")
            << testFile("completions/newExpressions.qml") << 5 << 13
            << ExpectedCompletions{ { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideNewMemberExpressionArgument")
            << testFile("completions/newExpressions.qml") << 5 << 15
            << ExpectedCompletions{ { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideNewExpression")
            << testFile("completions/newExpressions.qml") << 6 << 13
            << ExpectedCompletions{ { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideSuperLiteral")
            << testFile("completions/superLiteral.qml") << 5 << 11 << ExpectedCompletions{}
            << QStringList{ forStatementCompletion, u"f"_s };

    QTest::newRow("insideSuperParameter")
            << testFile("completions/superLiteral.qml") << 5 << 15
            << ExpectedCompletions{ { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideSuperField")
            << testFile("completions/superLiteral.qml") << 6 << 15 << ExpectedCompletions{}
            << QStringList{ forStatementCompletion, u"f"_s };

    QTest::newRow("insideSuperParameter2")
            << testFile("completions/superLiteral.qml") << 6 << 17
            << ExpectedCompletions{ { u"f"_s, CompletionItemKind::Method } }
            << QStringList{ forStatementCompletion };

    QTest::newRow("insideThisExpression")
            << testFile("completions/thisExpression.qml") << 5 << 10
            << ExpectedCompletions{ }
            << QStringList{ forStatementCompletion, u"f"_s };

    QTest::newRow("insideThisField")
            << testFile("completions/thisExpression.qml") << 5 << 14
            << ExpectedCompletions{ }
            << QStringList{ forStatementCompletion, u"f"_s };

    QTest::newRow("unexistingSignalParameter")
            << testFile("completions/parameterTypeFromBinding.qml") << 8 << 46
            << ExpectedCompletions{} << QStringList{};

    QTest::newRow("signalParameter")
            << testFile("completions/parameterTypeFromBinding.qml") << 11 << 59
            << ExpectedCompletions{ { u"helloData"_s, CompletionItemKind::Property } }
            << QStringList{};
    QTest::newRow("signalParameter2")
            << testFile("completions/parameterTypeFromBinding.qml") << 11 << 38
            << ExpectedCompletions{ { u"console"_s, CompletionItemKind::Property } }
            << QStringList{};
    QTest::newRow("fakePropertyChangedSignal")
            << testFile("completions/parameterTypeFromBinding.qml") << 14 << 64
            << ExpectedCompletions{ { u"helloData"_s, CompletionItemKind::Property } }
            << QStringList{};

    QTest::newRow("enumMemberFromRoot")
            << testFile("completions/Enumerations.qml") << 12 << 52
            << ExpectedCompletions{ { u"World"_s, CompletionItemKind::EnumMember } }
            << QStringList{};
    QTest::newRow("enumMemberInRoot")
            << testFile("completions/Enumerations.qml") << 18 << 40
            << ExpectedCompletions{ { u"World"_s, CompletionItemKind::EnumMember } }
            << QStringList{};
    QTest::newRow("enumFromRoot")
            << testFile("completions/Enumerations.qml") << 12 << 46
            << ExpectedCompletions{ { u"World"_s, CompletionItemKind::EnumMember },
                                    { u"Hello"_s, CompletionItemKind::Enum } }
                                  << QStringList{};
    QTest::newRow("enumInRoot")
            << testFile("completions/Enumerations.qml") << 18 << 34
            << ExpectedCompletions{ { u"World"_s, CompletionItemKind::EnumMember },
                                    { u"Hello"_s, CompletionItemKind::Enum } }
            << QStringList{};

    QTest::newRow("viaResourceFile")
            << testFile("AdminDialogFromSource.qml") << 4 << 33
            << ExpectedCompletions{ { u"LabelTitle"_s, CompletionItemKind::Constructor } }
            << QStringList{};

    QTest::newRow("Qt.Point") << testFile("completions/QtPoint.qml") << 5 << 34
                              << ExpectedCompletions{ { u"y"_s, CompletionItemKind::Property } }
                              << QStringList{ u"f"_s, forStatementCompletion };

    QTest::newRow("insideComment1")
            << testFile("completions/Comments.qml") << 4 << 9 << ExpectedCompletions{}
            << QStringList{ u"f"_s, forStatementCompletion };
    QTest::newRow("insideComment2")
            << testFile("completions/Comments.qml") << 6 << 9 << ExpectedCompletions{}
            << QStringList{ u"x"_s, u"Item"_s, forStatementCompletion };
}

void tst_qmlls_utils::completions()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(ExpectedCompletions, expected);
    QFETCH(QStringList, notExpected);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QEXPECT_FAIL("binaryExpressionMissingRHSWithSemicolon",
                 "Current parser cannot recover from this error yet!", Abort);
    QEXPECT_FAIL("binaryExpressionMissingRHSWithStatement",
                 "Current parser cannot recover from this error yet!", Abort);
    QCOMPARE(locations.size(), 1);

    QString code;
    {
        QFile file(filePath);
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        code = QString::fromUtf8(file.readAll());
    }

    qsizetype pos = QQmlLSUtils::textOffsetFrom(code, line - 1, character - 1);
    CompletionContextStrings ctxt{ code, pos };
    QQmlLSCompletion completionEngine(m_pluginLoader);
    QList<CompletionItem> completions =
            completionEngine.completions(locations.front().domItem, ctxt);

    if (expected.isEmpty()) {
        if constexpr (enable_debug_output) {
            if (!completions.isEmpty()) {
                QStringList unexpected;
                for (const auto &current : completions) {
                    unexpected << current.label;
                }
                qDebug() << "Received unexpected completions:" << unexpected.join(u", ");
            }
        }
        QEXPECT_FAIL("singleton", "completion not implemented yet!", Abort);
        QVERIFY(completions.isEmpty());
        return;
    }

    QSet<QString> labels;
    QStringList sortedLabels;
    QDuplicateTracker<QByteArray> modulesTracker;
    QDuplicateTracker<QByteArray> keywordsTracker;
    QDuplicateTracker<QByteArray> classesTracker;
    QDuplicateTracker<QByteArray> fieldsTracker;
    QDuplicateTracker<QByteArray> propertiesTracker;
    QDuplicateTracker<QByteArray> snippetTracker;

    // avoid QEXPECT_FAIL tests to XPASS when completion order changes
    std::sort(completions.begin(), completions.end(),
              [](const CompletionItem&a, const CompletionItem&b) {return a.label < b.label;});

    for (const CompletionItem &c : completions) {
        // explicitly forbid marker structs created by QQmlJSImporter
        QVERIFY(!c.label.contains("$internal$."));
        QVERIFY(!c.label.contains("$module$."));
        QVERIFY(!c.label.contains("$anonymous$."));

        if (c.kind->toInt() == int(CompletionItemKind::Module)) {
            QVERIFY2(!modulesTracker.hasSeen(c.label), "Duplicate module: " + c.label);
        } else if (c.kind->toInt() == int(CompletionItemKind::Keyword)) {
            QVERIFY2(!keywordsTracker.hasSeen(c.label), "Duplicate keyword: " + c.label);
        } else if (c.kind->toInt() == int(CompletionItemKind::Class)) {
            QVERIFY2(!classesTracker.hasSeen(c.label), "Duplicate class: " + c.label);
        } else if (c.kind->toInt() == int(CompletionItemKind::Field)) {
            QVERIFY2(!fieldsTracker.hasSeen(c.label), "Duplicate field: " + c.label);
        } else if (c.kind->toInt() == int(CompletionItemKind::Snippet)) {
            QVERIFY2(!snippetTracker.hasSeen(c.label), "Duplicate field: " + c.label);
            if (c.insertText->contains('\n') || c.insertText->contains('\r')) {
                QCOMPARE(c.insertTextMode, InsertTextMode::AdjustIndentation);
            }
        } else if (c.kind->toInt() == int(CompletionItemKind::Property)) {
            QVERIFY2(!propertiesTracker.hasSeen(c.label), "Duplicate property: " + c.label);
            QCOMPARE(c.insertText, std::nullopt);
        }
        labels << c.label;
        sortedLabels << c.label;
    }
    const QString labelsForPrinting = sortedLabels.join(u", "_s);

    for (const ExpectedCompletion &exp : expected) {
        QEXPECT_FAIL("letStatementAfterEqual", "Completion not implemented yet!", Abort);

        QVERIFY2(labels.contains(exp.label),
                 u"no %1 in %2"_s.arg(exp.label, labelsForPrinting).toUtf8());
        if (labels.contains(exp.label)) {

            bool foundEntry = false;
            bool hasCorrectKind = false;
            CompletionItemKind foundKind;
            for (const CompletionItem &c : completions) {
                if (c.label == exp.label) {
                    foundKind = static_cast<CompletionItemKind>(c.kind->toInt());
                    foundEntry = true;
                    if (foundKind == exp.kind) {
                        hasCorrectKind = true;
                        if (!exp.snippet.isEmpty()) {
                            QCOMPARE(QString::fromUtf8(c.insertText.value_or(QByteArray())),
                                     exp.snippet);
                        }
                        break;
                    }
                }
            }

            // Ignore QVERIFY for those completions not in the expected list.
            if (!foundEntry)
                continue;

            QVERIFY2(hasCorrectKind,
                     qPrintable(QString::fromLatin1("Completion item '%1' has wrong kind '%2'")
                                        .arg(exp.label)
                                        .arg(QMetaEnum::fromType<CompletionItemKind>().valueToKey(
                                                int(foundKind)))));
        }
    }
    for (const QString &nexp : notExpected) {
        QEXPECT_FAIL("ignoreNonRelatedTypesForPropertyDefinitionBinding",
                     "Filtering by Type not implemented yet, for example to avoid proposing "
                     "binding Items to Rectangle properties.",
                     Abort);
        QVERIFY2(!labels.contains(nexp), u"found unexpected completion  %1"_s.arg(nexp).toUtf8());
    }
}

void tst_qmlls_utils::cmakeBuildCommand()
{
    const QString path = u"helloWorldPath"_s;
    const std::pair<QString, QStringList> expected{
        u"cmake"_s, { u"--build"_s, path, u"-t"_s, u"all_qmltyperegistrations"_s }
    };
    QCOMPARE(QQmlLSUtils::cmakeBuildCommand(path), expected);
}

QTEST_MAIN(tst_qmlls_utils)
