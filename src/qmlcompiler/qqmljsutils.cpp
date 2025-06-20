// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsutils_p.h"
#include "qqmljstyperesolver_p.h"
#include "qqmljsscopesbyid_p.h"

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*! \internal

    Fully resolves alias \a property and returns the information about the
    origin, which is not an alias.
*/
template<typename ScopeForId>
static QQmlJSUtils::ResolvedAlias
resolveAlias(ScopeForId scopeForId, const QQmlJSMetaProperty &property,
             const QQmlJSScope::ConstPtr &owner, const QQmlJSUtils::AliasResolutionVisitor &visitor)
{
    Q_ASSERT(property.isAlias());
    Q_ASSERT(owner);

    QQmlJSUtils::ResolvedAlias result {};
    result.owner = owner;

    // TODO: one could optimize the generated alias code for aliases pointing to aliases
    //  e.g., if idA.myAlias -> idB.myAlias2 -> idC.myProp, then one could directly generate
    //  idA.myProp as pointing to idC.myProp. //  This gets complicated when idB.myAlias is in a different Component than where the
    //  idA.myAlias is defined: scopeForId currently only contains the ids of the current
    //  component and alias resolution on the ids of a different component fails then.
    if (QQmlJSMetaProperty nextProperty = property; nextProperty.isAlias()) {
        QQmlJSScope::ConstPtr resultOwner = result.owner;
        result = QQmlJSUtils::ResolvedAlias {};

        visitor.reset();

        auto aliasExprBits = nextProperty.aliasExpression().split(u'.');
        // do not crash on invalid aliasexprbits when accessing aliasExprBits[0]
        if (aliasExprBits.size() < 1)
            return {};

        // resolve id first:
        resultOwner = scopeForId(aliasExprBits[0], resultOwner);
        if (!resultOwner)
            return {};

        visitor.processResolvedId(resultOwner);

        aliasExprBits.removeFirst(); // Note: for simplicity, remove the <id>
        result.owner = resultOwner;
        result.kind = QQmlJSUtils::AliasTarget_Object;

        for (const QString &bit : std::as_const(aliasExprBits)) {
            nextProperty = resultOwner->property(bit);
            if (!nextProperty.isValid())
                return {};

            visitor.processResolvedProperty(nextProperty, resultOwner);

            result.property = nextProperty;
            result.owner = resultOwner;
            result.kind = QQmlJSUtils::AliasTarget_Property;

            resultOwner = nextProperty.type();
        }
    }

    return result;
}

QQmlJSUtils::ResolvedAlias QQmlJSUtils::resolveAlias(const QQmlJSTypeResolver *typeResolver,
                                                     const QQmlJSMetaProperty &property,
                                                     const QQmlJSScope::ConstPtr &owner,
                                                     const AliasResolutionVisitor &visitor)
{
    return ::resolveAlias(
            [&](const QString &id, const QQmlJSScope::ConstPtr &referrer) {
                return typeResolver->typeForId(referrer, id);
            },
            property, owner, visitor);
}

QQmlJSUtils::ResolvedAlias QQmlJSUtils::resolveAlias(const QQmlJSScopesById &idScopes,
                                                     const QQmlJSMetaProperty &property,
                                                     const QQmlJSScope::ConstPtr &owner,
                                                     const AliasResolutionVisitor &visitor)
{
    return ::resolveAlias(
            [&](const QString &id, const QQmlJSScope::ConstPtr &referrer) {
                return idScopes.scope(id, referrer);
            },
            property, owner, visitor);
}

std::optional<QQmlJSFixSuggestion> QQmlJSUtils::didYouMean(const QString &userInput,
                                                     QStringList candidates,
                                                     QQmlJS::SourceLocation location)
{
    QString shortestDistanceWord;
    int shortestDistance = userInput.size();

    // Most of the time the candidates are keys() from QHash, which means that
    // running this function in the seemingly same setup might yield different
    // best cadidate (e.g. imagine a typo 'thing' with candidates 'thingA' vs
    // 'thingB'). This is especially flaky in e.g. test environment where the
    // results may differ (even when the global hash seed is fixed!) when
    // running one test vs the whole test suite (recall platform-dependent
    // QSKIPs). There could be user-visible side effects as well, so just sort
    // the candidates to guarantee consistent results
    std::sort(candidates.begin(), candidates.end());

    for (const QString &candidate : candidates) {
        /*
         * Calculate the distance between the userInput and candidate using Damerau–Levenshtein
         * Roughly based on
         * https://en.wikipedia.org/wiki/Levenshtein_distance#Iterative_with_two_matrix_rows.
         */
        QVarLengthArray<int> v0(candidate.size() + 1);
        QVarLengthArray<int> v1(candidate.size() + 1);

        std::iota(v0.begin(), v0.end(), 0);

        for (qsizetype i = 0; i < userInput.size(); i++) {
            v1[0] = i + 1;
            for (qsizetype j = 0; j < candidate.size(); j++) {
                int deletionCost = v0[j + 1] + 1;
                int insertionCost = v1[j] + 1;
                int substitutionCost = userInput[i] == candidate[j] ? v0[j] : v0[j] + 1;
                v1[j + 1] = std::min({ deletionCost, insertionCost, substitutionCost });
            }
            std::swap(v0, v1);
        }

        int distance = v0[candidate.size()];
        if (distance < shortestDistance) {
            shortestDistanceWord = candidate;
            shortestDistance = distance;
        }
    }

    if (shortestDistance
            < std::min(std::max(userInput.size() / 2, qsizetype(3)), userInput.size())) {
        return QQmlJSFixSuggestion {
            u"Did you mean \"%1\"?"_s.arg(shortestDistanceWord),
            location,
            shortestDistanceWord
        };
    } else {
        return {};
    }
}

/*! \internal

    Returns a corresponding source directory path for \a buildDirectoryPath
    Returns empty string on error
*/
std::variant<QString, QQmlJS::DiagnosticMessage>
QQmlJSUtils::sourceDirectoryPath(const QQmlJSImporter *importer, const QString &buildDirectoryPath)
{
    const auto makeError = [](const QString &msg) {
        return QQmlJS::DiagnosticMessage { msg, QtWarningMsg, QQmlJS::SourceLocation() };
    };

    if (!importer->metaDataMapper())
        return makeError(u"QQmlJSImporter::metaDataMapper() is nullptr"_s);

    // for now, meta data contains just a single entry
    QQmlJSResourceFileMapper::Filter matchAll { QString(), QStringList(),
                                                QQmlJSResourceFileMapper::Directory
                                                        | QQmlJSResourceFileMapper::Recurse };
    QQmlJSResourceFileMapper::Entry entry = importer->metaDataMapper()->entry(matchAll);
    if (!entry.isValid())
        return makeError(u"Failed to find meta data entry in QQmlJSImporter::metaDataMapper()"_s);
    if (!buildDirectoryPath.startsWith(entry.filePath)) // assume source directory path already
        return makeError(u"The module output directory does not match the build directory path"_s);

    QString qrcPath = buildDirectoryPath;
    qrcPath.remove(0, entry.filePath.size());
    qrcPath.prepend(entry.resourcePath);
    qrcPath.remove(0, 1); // remove extra "/"

    const QStringList sourceDirPaths = importer->resourceFileMapper()->filePaths(
            QQmlJSResourceFileMapper::resourceFileFilter(qrcPath));
    if (sourceDirPaths.size() != 1) {
        const QString matchedPaths =
                sourceDirPaths.isEmpty() ? u"<none>"_s : sourceDirPaths.join(u", ");
        return makeError(
                QStringLiteral("QRC path %1 (deduced from %2) has unexpected number of mappings "
                               "(%3). File paths that matched:\n%4")
                        .arg(qrcPath, buildDirectoryPath, QString::number(sourceDirPaths.size()),
                             matchedPaths));
    }
    return sourceDirPaths[0];
}

/*! \internal

    Utility method that checks if one of the registers is var, and the other can be
    efficiently compared to it
*/
bool canStrictlyCompareWithVar(
        const QQmlJSTypeResolver *typeResolver, const QQmlJSScope::ConstPtr &lhsType,
        const QQmlJSScope::ConstPtr &rhsType)
{
    Q_ASSERT(typeResolver);

    const QQmlJSScope::ConstPtr varType = typeResolver->varType();
    const bool leftIsVar = (lhsType == varType);
    const bool righttIsVar = (rhsType == varType);
    return leftIsVar != righttIsVar;
}

/*! \internal

    Utility method that checks if one of the registers is qobject, and the other can be
    efficiently compared to it
*/
bool canCompareWithQObject(
        const QQmlJSTypeResolver *typeResolver, const QQmlJSScope::ConstPtr &lhsType,
        const QQmlJSScope::ConstPtr &rhsType)
{
    Q_ASSERT(typeResolver);
    return (lhsType->isReferenceType()
            && (rhsType->isReferenceType() || rhsType == typeResolver->nullType()))
            || (rhsType->isReferenceType()
                && (lhsType->isReferenceType() || lhsType == typeResolver->nullType()));
}

/*! \internal

    Utility method that checks if both sides are QUrl type. In future, that might be extended to
    support comparison with other types i.e QUrl vs string
*/
bool canCompareWithQUrl(
        const QQmlJSTypeResolver *typeResolver, const QQmlJSScope::ConstPtr &lhsType,
        const QQmlJSScope::ConstPtr &rhsType)
{
    Q_ASSERT(typeResolver);
    return lhsType == typeResolver->urlType() && rhsType == typeResolver->urlType();
}

QStringList QQmlJSUtils::resourceFilesFromBuildFolders(const QStringList &buildFolders)
{
    QStringList result;
    for (const QString &path : buildFolders) {
        QDirIterator it(path, QStringList{ u"*.qrc"_s }, QDir::Files | QDir::Hidden,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            result.append(it.next());
        }
    }
    return result;
}

enum FilterType {
    LocalFileFilter,
    ResourceFileFilter
};

/*!
\internal
Obtain a QML module qrc entry from its qmldir entry.

Contains a heuristic for QML modules without nested-qml-module-with-prefer-feature
that tries to find a parent directory that contains a qmldir entry in the qrc.
*/
static QQmlJSResourceFileMapper::Entry
qmlModuleEntryFromBuildPath(const QQmlJSResourceFileMapper *mapper,
                            const QString &pathInBuildFolder, FilterType type)
{
    const QString cleanPath = QDir::cleanPath(pathInBuildFolder);
    QStringView directoryPath = cleanPath;

    while (!directoryPath.isEmpty()) {
        const qsizetype lastSlashIndex = directoryPath.lastIndexOf(u'/');
        if (lastSlashIndex == -1)
            return {};

        directoryPath.truncate(lastSlashIndex);
        const QString qmldirPath = u"%1/qmldir"_s.arg(directoryPath);
        const QQmlJSResourceFileMapper::Filter qmldirFilter = type == LocalFileFilter
                ? QQmlJSResourceFileMapper::localFileFilter(qmldirPath)
                : QQmlJSResourceFileMapper::resourceFileFilter(qmldirPath);

        QQmlJSResourceFileMapper::Entry result = mapper->entry(qmldirFilter);
        if (result.isValid()) {
            result.resourcePath.chop(std::char_traits<char>::length("/qmldir"));
            result.filePath.chop(std::char_traits<char>::length("/qmldir"));
            return result;
        }
    }
    return {};
}

/*!
\internal
Obtains the source folder path from a build folder QML file path via the passed \c mapper.

This works on proper QML modules when using the nested-qml-module-with-prefer-feature
from 6.8 and uses a heuristic when the qmldir with the prefer entry is missing.
*/
QString QQmlJSUtils::qmlSourcePathFromBuildPath(const QQmlJSResourceFileMapper *mapper,
                                                const QString &pathInBuildFolder)
{
    if (!mapper)
        return pathInBuildFolder;

    const auto qmlModuleEntry =
            qmlModuleEntryFromBuildPath(mapper, pathInBuildFolder, LocalFileFilter);
    if (!qmlModuleEntry.isValid())
        return pathInBuildFolder;
    const QString qrcPath = qmlModuleEntry.resourcePath
            + QStringView(pathInBuildFolder).sliced(qmlModuleEntry.filePath.size());

    const auto entry = mapper->entry(QQmlJSResourceFileMapper::resourceFileFilter(qrcPath));
    return entry.isValid()? entry.filePath : pathInBuildFolder;
}

/*!
\internal
Obtains the source folder path from a build folder QML file path via the passed \c mapper, see also
\l QQmlJSUtils::qmlSourcePathFromBuildPath.
*/
QString QQmlJSUtils::qmlBuildPathFromSourcePath(const QQmlJSResourceFileMapper *mapper,
                                                const QString &pathInSourceFolder)
{
    if (!mapper)
        return pathInSourceFolder;

    const QString qrcPath =
            mapper->entry(QQmlJSResourceFileMapper::localFileFilter(pathInSourceFolder))
                    .resourcePath;

    if (qrcPath.isEmpty())
        return pathInSourceFolder;

    const auto moduleBuildEntry =
            qmlModuleEntryFromBuildPath(mapper, qrcPath, ResourceFileFilter);

    if (!moduleBuildEntry.isValid())
        return pathInSourceFolder;

    const auto qrcFolderPath = qrcPath.first(qrcPath.lastIndexOf(u'/')); // drop the filename

    return moduleBuildEntry.filePath + qrcFolderPath.sliced(moduleBuildEntry.resourcePath.size())
            + pathInSourceFolder.sliced(pathInSourceFolder.lastIndexOf(u'/'));
}

QT_END_NAMESPACE
