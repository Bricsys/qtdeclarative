// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLFORMAT_BASE
#define TST_QMLFORMAT_BASE

#include <QtCore/qstringlist.h>
#include <QtCore/qdir.h>
#include <QtCore/qobject.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace Qt::StringLiterals;

class TestQmlformatBase : public QQmlDataTest
{
    Q_OBJECT

public:
    using QQmlDataTest::QQmlDataTest;
    TestQmlformatBase(const char *qmlTestDataDir,
                      QQmlDataTest::FailOnWarningsPolicy failOnWarningsPolicy)
        : QQmlDataTest(qmlTestDataDir, failOnWarningsPolicy)
    {
        // These snippets are not expected to run on their own.
        m_excludedDirs << "doc/src/snippets/qml/visualdatamodel_rootindex";
        m_excludedDirs << "doc/src/snippets/qml/qtbinding";
        m_excludedDirs << "doc/src/snippets/qml/imports";
        m_excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
        m_excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
        m_excludedDirs << "doc/src/snippets/qtquick1/imports";
        m_excludedDirs << "tests/manual/v4";
        m_excludedDirs << "tests/manual/qmllsformatter";
        m_excludedDirs << "tests/auto/qml/ecmascripttests";
        m_excludedDirs << "tests/auto/qml/qmllint";

        // Add invalid files (i.e. files with syntax errors)
        m_invalidFiles << "tests/auto/quick/qquickloader/data/InvalidSourceComponent.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.3.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.5.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/property.4.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/empty.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/missingObject.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/insertedSemicolon.1.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nonexistantProperty.5.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidRoot.1.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.1.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.3.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidID.4.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/questionDotEOF.qml";
        m_invalidFiles << "tests/auto/qml/qquickfolderlistmodel/data/dummy.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.1.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.3.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.4.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.5.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.6.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.1.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
        m_invalidFiles << "tests/auto/qml/debugger/qqmlpreview/data/broken.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.3.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/requiredProperties.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_Or.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_And.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_Or.qml";
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/typeAnnotations.2.qml";
        m_invalidFiles << "tests/auto/qml/qqmlparser/data/disallowedtypeannotations/qmlnestedfunction.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/emptyFile.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/completions/missingRHS.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/completions/missingRHS.parserfail.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/completions/attachedPropertyMissingRHS.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/completions/groupedPropertyMissingRHS.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/completions/afterDots.qml";
        m_invalidFiles << "tests/auto/qmlls/modules/data/completions/bindingAfterDot.qml";
        m_invalidFiles << "tests/auto/qmlls/modules/data/completions/defaultBindingAfterDot.qml";
        m_invalidFiles << "tests/auto/qmlls/utils/data/qualifiedModule.qml";

        // Files that get changed:
        // rewrite of import "bla/bla/.." to import "bla"
        m_invalidFiles << "tests/auto/qml/qqmlcomponent/data/componentUrlCanonicalization.4.qml";
        // block -> object in internal update
        m_invalidFiles << "tests/auto/qml/qqmlpromise/data/promise-executor-throw-exception.qml";
        // removal of unsupported indexing of Object declaration
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/hangOnWarning.qml";
        // removal of duplicated id
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/component.3.qml";
        // Optional chains are not permitted on the left-hand-side in assignments
        m_invalidFiles << "tests/auto/qml/qqmllanguage/data/optionalChaining.LHS.qml";
        // object literal with = assignements
        m_invalidFiles << "tests/auto/quickcontrols/controls/data/tst_scrollbar.qml";

        // These files rely on exact formatting
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon1.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
        m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon2.qml";

        // These files are too big
        m_ignoreFiles << "tests/benchmarks/qml/qmldom/data/longQmlFile.qml";
        m_ignoreFiles << "tests/benchmarks/qml/qmldom/data/deeplyNested.qml";
    }

    QStringList findFiles(const QDir &d)
    {
        for (int ii = 0; ii < m_excludedDirs.size(); ++ii) {
            QString s = m_excludedDirs.at(ii);
            if (d.absolutePath().endsWith(s))
                return QStringList();
        }

        QStringList rv;

        const QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                              QDir::Files);
        for (const QString &file : files) {
            QString absoluteFilePath = d.absoluteFilePath(file);
            if (!isIgnoredFile(QFileInfo(absoluteFilePath)))
                rv << absoluteFilePath;
        }

        const QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                             QDir::NoSymLinks);
        for (const QString &dir : dirs) {
            QDir sub = d;
            sub.cd(dir);
            rv << findFiles(sub);
        }

        return rv;
    }

    bool isInvalidFile(const QFileInfo &fileName) const
    {
        for (const QString &invalidFile : m_invalidFiles) {
            if (fileName.absoluteFilePath().endsWith(invalidFile))
                return true;
        }
        return false;
    }

    bool isIgnoredFile(const QFileInfo &fileName) const
    {
        for (const QString &file : m_ignoreFiles) {
            if (fileName.absoluteFilePath().endsWith(file))
                return true;
        }
        return false;
    }

    QString readTestFile(const QString &path) const
    {
        QFile file(testFile(path));

        if (!file.open(QIODevice::ReadOnly))
            return "";

        return QString::fromUtf8(file.readAll());
    }

    QStringList m_excludedDirs;
    QStringList m_invalidFiles;
    QStringList m_ignoreFiles;
};


#endif // TST_QMLFORMAT_BASE
