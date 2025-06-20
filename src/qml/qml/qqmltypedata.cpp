// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmlcomponentandaliasresolver_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlirbuilder_p.h>
#include <private/qqmlirloader_p.h>
#include <private/qqmlpropertycachecreator_p.h>
#include <private/qqmlpropertyvalidator_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmltypecompiler_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qcryptographichash.h>

#include <memory>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcCycle, "qt.qml.typeresolution.cycle", QtWarningMsg)

QQmlTypeData::TypeDataCallback::~TypeDataCallback()
{
}

QString QQmlTypeData::TypeReference::qualifiedName() const
{
    QString result;
    if (!prefix.isEmpty()) {
        result = prefix + QLatin1Char('.');
    }
    result.append(type.qmlTypeName());
    return result;
}

QQmlTypeData::QQmlTypeData(const QUrl &url, QQmlTypeLoader *manager)
    : QQmlTypeLoader::Blob(url, QmlFile, manager),
      m_typesResolved(false), m_implicitImportLoaded(false)
{

}

QQmlTypeData::~QQmlTypeData()
{
    m_scripts.clear();
    m_compositeSingletons.clear();
    m_resolvedTypes.clear();
}

QV4::CompiledData::CompilationUnit *QQmlTypeData::compilationUnit() const
{
    return m_compiledData.data();
}

void QQmlTypeData::registerCallback(TypeDataCallback *callback)
{
    assertEngineThread();
    Q_ASSERT(!m_callbacks.contains(callback));
    m_callbacks.append(callback);
}

void QQmlTypeData::unregisterCallback(TypeDataCallback *callback)
{
    assertEngineThread();
    Q_ASSERT(m_callbacks.contains(callback));
    m_callbacks.removeOne(callback);
    Q_ASSERT(!m_callbacks.contains(callback));
}

QQmlType QQmlTypeData::qmlType(const QString &inlineComponentName) const
{
    if (inlineComponentName.isEmpty())
        return m_qmlType;
    return m_inlineComponentData[inlineComponentName].qmlType;
}

bool QQmlTypeData::tryLoadFromDiskCache()
{
    assertTypeLoaderThread();

    if (!m_backupSourceCode.isCacheable())
        return false;

    if (auto unit = QQmlMetaType::obtainCompilationUnit(url()))
        return loadFromDiskCache(unit);

    if (!m_typeLoader->readCacheFile())
        return false;

    auto unit = QQml::makeRefPointer<QV4::CompiledData::CompilationUnit>();
    {
        QString error;
        if (!unit->loadFromDisk(url(), m_backupSourceCode.sourceTimeStamp(), &error)) {
            qCDebug(DBG_DISK_CACHE) << "Error loading" << urlString() << "from disk cache:" << error;
            return false;
        }
    }

    if (unit->unitData()->flags & QV4::CompiledData::Unit::PendingTypeCompilation) {
        restoreIR(unit);
        return true;
    }

    return loadFromDiskCache(unit);
}

bool QQmlTypeData::loadFromDiskCache(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit)
{
    assertTypeLoaderThread();

    m_compiledData = unit;

    QVector<QV4::CompiledData::InlineComponent> ics;
    for (int i = 0, count = m_compiledData->objectCount(); i < count; ++i) {
        auto object = m_compiledData->objectAt(i);
        m_typeReferences.collectFromObject(object);
        const auto inlineComponentTable = object->inlineComponentTable();
        for (auto i = 0; i != object->nInlineComponents; ++i) {
            ics.push_back(inlineComponentTable[i]);
        }
    }

    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For remote URLs, we don't delay the loading of the implicit import
    // because the loading probably requires an asynchronous fetch of the
    // qmldir (so we can't load it just in time).
    if (!finalUrl().scheme().isEmpty()) {
        QUrl qmldirUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (!QQmlImports::isLocal(qmldirUrl)) {
            if (!loadImplicitImport())
                return false;

            // find the implicit import
            for (quint32 i = 0, count = m_compiledData->importCount(); i < count; ++i) {
                const QV4::CompiledData::Import *import = m_compiledData->importAt(i);
                if (m_compiledData->stringAt(import->uriIndex) == QLatin1String(".")
                    && import->qualifierIndex == 0
                    && !import->version.hasMajorVersion()
                    && !import->version.hasMinorVersion()) {
                    QList<QQmlError> errors;
                    auto pendingImport = std::make_shared<PendingImport>(
                                this, import, QQmlImports::ImportNoFlag);
                    pendingImport->precedence = QQmlImportInstance::Implicit;
                    if (!fetchQmldir(qmldirUrl, std::move(pendingImport), 1, &errors)) {
                        setError(errors);
                        return false;
                    }
                    break;
                }
            }
        }
    }

    for (int i = 0, count = m_compiledData->importCount(); i < count; ++i) {
        const QV4::CompiledData::Import *import = m_compiledData->importAt(i);
        QList<QQmlError> errors;
        if (!addImport(import, {}, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_importCache->baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(import->location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(import->location.column()));
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
            return false;
        }
    }

    for (auto&& ic: ics) {
        QString const nameString = m_compiledData->stringAt(ic.nameIndex);
        auto importUrl = finalUrl();
        importUrl.setFragment(nameString);
        auto import = new QQmlImportInstance();
        m_importCache->addInlineComponentImport(import, nameString, importUrl);
    }

    return true;
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::allocateNamedObjects(
        const QV4::CompiledData::Object *object) const
{
    Q_UNUSED(object);
}

template<>
bool QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::markAsComponent(int index) const
{
    return m_compiler->objectAt(index)->hasFlag(QV4::CompiledData::Object::IsComponent);
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::setObjectId(int index) const
{
    Q_UNUSED(index)
    // we cannot sanity-check the index here because bindings are sorted in a different order
    // in the CU vs the IR.
}

template<>
void QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::resolveGeneralizedGroupProperty(
        const CompiledObject &component, CompiledBinding *binding)
{
    // We cannot make it fail here. It might be a custom-parsed property
    for (int i = 0, count = component.namedObjectsInComponentCount(); i < count; ++i) {
        const int candidateIndex = component.namedObjectsInComponentTable()[i];
        if (m_compiler->objectAt(candidateIndex)->idNameIndex == binding->propertyNameIndex) {
            m_propertyCaches->set(binding->value.objectIndex, m_propertyCaches->at(candidateIndex));
            return;
        }
    }
}

template<>
typename QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::AliasResolutionResult
QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::resolveAliasesInObject(
        const CompiledObject &component, int objectIndex, QQmlError *error)
{
    const CompiledObject *obj = m_compiler->objectAt(objectIndex);
    for (auto alias = obj->aliasesBegin(), end = obj->aliasesEnd(); alias != end; ++alias) {
        if (!alias->hasFlag(QV4::CompiledData::Alias::Resolved)) {
            *error = qQmlCompileError(alias->referenceLocation,
                                      QQmlComponentAndAliasResolverBase::tr("Unresolved alias found"));
            return NoAliasResolved;
        }

        if (alias->isAliasToLocalAlias() || alias->encodedMetaPropertyIndex == -1)
            continue;

        const int targetObjectIndex
                = objectForId(m_compiler, component, alias->targetObjectId());
        const int coreIndex
                = QQmlPropertyIndex::fromEncoded(alias->encodedMetaPropertyIndex).coreIndex();

        QQmlPropertyCache::ConstPtr targetCache = m_propertyCaches->at(targetObjectIndex);
        Q_ASSERT(targetCache);

        if (!targetCache->property(coreIndex))
            return SomeAliasesResolved;
    }

    return AllAliasesResolved;
}

template<>
bool QQmlComponentAndAliasResolver<QV4::CompiledData::CompilationUnit>::wrapImplicitComponent(
        const QV4::CompiledData::Binding *binding)
{
    // This should have been done when creating the CU.
    Q_UNUSED(binding);
    return false;
}

QQmlError QQmlTypeData::createTypeAndPropertyCaches(
        const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
        const QV4::CompiledData::ResolvedTypeReferenceMap &resolvedTypeCache)
{
    assertTypeLoaderThread();

    Q_ASSERT(m_compiledData);
    m_compiledData->typeNameCache = typeNameCache;
    m_compiledData->resolvedTypes = resolvedTypeCache;
    m_compiledData->inlineComponentData = m_inlineComponentData;

    QQmlPendingGroupPropertyBindings pendingGroupPropertyBindings;

    {
        QQmlPropertyCacheCreator<QV4::CompiledData::CompilationUnit> propertyCacheCreator(
                &m_compiledData->propertyCaches, &pendingGroupPropertyBindings, m_typeLoader,
                m_compiledData.data(), m_importCache.data(), typeClassName());

        QQmlError error = propertyCacheCreator.verifyNoICCycle();
        if (error.isValid())
            return error;

        QQmlPropertyCacheCreatorBase::IncrementalResult result;
        do {
            result = propertyCacheCreator.buildMetaObjectsIncrementally();
            if (result.error.isValid()) {
                return result.error;
            } else {
                QQmlComponentAndAliasResolver resolver(
                        m_compiledData.data(), &m_compiledData->propertyCaches);
                if (const QQmlError error = resolver.resolve(result.processedRoot);
                        error.isValid()) {
                    return error;
                }
                pendingGroupPropertyBindings.resolveMissingPropertyCaches(
                        &m_compiledData->propertyCaches);
                pendingGroupPropertyBindings.clear(); // anything that can be processed is now processed
            }

        } while (result.canResume);
    }

    pendingGroupPropertyBindings.resolveMissingPropertyCaches(&m_compiledData->propertyCaches);
    return QQmlError();
}

// local helper function for inline components
namespace  {
using InlineComponentData = QV4::CompiledData::InlineComponentData;

template<typename ObjectContainer>
void setupICs(
        const ObjectContainer &container, QHash<QString, InlineComponentData> *icData,
        const QUrl &baseUrl,
        const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit) {
    Q_ASSERT(icData->empty());
    for (int i = 0; i != container->objectCount(); ++i) {
        auto root = container->objectAt(i);
        for (auto it = root->inlineComponentsBegin(); it != root->inlineComponentsEnd(); ++it) {
            // We cannot re-use a previously finalized inline component type here. We need our own.
            // We can and should re-use speculative type references, though.
            InlineComponentData icDatum(
                    QQmlMetaType::findInlineComponentType(
                            baseUrl, container->stringAt(it->nameIndex), compilationUnit),
                int(it->objectIndex), int(it->nameIndex), 0, 0, 0);

            icData->insert(container->stringAt(it->nameIndex), icDatum);
        }
    }
};
}

template<typename Container>
void QQmlTypeData::setCompileUnit(const Container &container)
{
    assertTypeLoaderThread();

    for (int i = 0; i != container->objectCount(); ++i) {
        auto const root = container->objectAt(i);
        for (auto it = root->inlineComponentsBegin(); it != root->inlineComponentsEnd(); ++it) {
            auto *typeRef = m_compiledData->resolvedType(it->nameIndex);

            // We don't want the type reference to keep a strong reference to the compilation unit
            // here. The compilation unit owns the type reference, and having a strong reference
            // would prevent the compilation unit from ever getting deleted. We can still be sure
            // that the compilation unit outlives the type reference, due to ownership.
            typeRef->setReferencesCompilationUnit(false);

            typeRef->setCompilationUnit(m_compiledData); // share compilation unit
        }
    }
}

void QQmlTypeData::done()
{
    assertTypeLoaderThread();

    auto cleanup = qScopeGuard([this]{
        m_backupSourceCode = SourceCodeData();
        m_document.reset();
        m_typeReferences.clear();
        if (isError()) {
            const auto encounteredErrors = errors();
            for (const QQmlError &e : encounteredErrors)
                qCDebug(DBG_DISK_CACHE) << e.toString();
            m_compiledData.reset();
        }
    });

    if (isError())
        return;

    // Check all script dependencies for errors
    for (int ii = 0; ii < m_scripts.size(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QQmlError> errors = script.script->errors();
            QQmlError error;
            error.setUrl(url());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(script.location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(script.location.column()));
            error.setDescription(QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->urlString()));
            errors.prepend(error);
            setError(errors);
            return;
        }
    }

    // Check all type dependencies for errors
    auto createError = [&](const TypeReference &type , const QString &message) {
        QList<QQmlError> errors = type.typeData ? type.typeData->errors() : QList<QQmlError>{};
        QQmlError error;
        error.setUrl(url());
        error.setLine(qmlConvertSourceCoordinate<quint32, int>(type.location.line()));
        error.setColumn(qmlConvertSourceCoordinate<quint32, int>(type.location.column()));
        error.setDescription(message);
        errors.prepend(error);
        setError(errors);
    };
    for (auto it = std::as_const(m_resolvedTypes).begin(), end = std::as_const(m_resolvedTypes).end(); it != end;
         ++it) {
        const TypeReference &type = *it;
        Q_ASSERT(!type.typeData || type.typeData->isCompleteOrError() || type.type.isInlineComponentType());

        if (type.type.isInlineComponentType()) {
            const QUrl url = type.type.sourceUrl();
            if (!QQmlMetaType::equalBaseUrls(url, finalUrl())
                    && !QQmlMetaType::obtainCompilationUnit(type.type.typeId())) {
                const QString &typeName = stringAt(it.key());
                int lastDot = typeName.lastIndexOf(u'.');
                createError(
                    type,
                    QQmlTypeLoader::tr("Type %1 has no inline component type called %2")
                        .arg(QStringView{typeName}.left(lastDot), type.type.elementName()));
                return;
            }
        }
        if (type.typeData && type.typeData->isError()) {
            const QString &typeName = stringAt(it.key());
            createError(type, QQmlTypeLoader::tr("Type %1 unavailable").arg(typeName));
            return;
        }
    }

    // Check all composite singleton type dependencies for errors
    for (int ii = 0; ii < m_compositeSingletons.size(); ++ii) {
        const TypeReference &type = m_compositeSingletons.at(ii);
        Q_ASSERT(!type.typeData || type.typeData->isCompleteOrError());
        if (type.typeData && type.typeData->isError()) {
            QString typeName = type.type.qmlTypeName();

            createError(type, QQmlTypeLoader::tr("Type %1 unavailable").arg(typeName));
            return;
        }
    }

    if (QQmlPropertyCacheCreatorBase::canCreateClassNameTypeByUrl(finalUrl())) {
        const bool isSingleton = m_document
            ? m_document.data()->isSingleton()
            : (m_compiledData->unitData()->flags & QV4::CompiledData::Unit::IsSingleton);
        m_qmlType = QQmlMetaType::findCompositeType(
                url(), m_compiledData, isSingleton
                        ? QQmlMetaType::Singleton
                        : QQmlMetaType::NonSingleton);
        m_typeClassName = QByteArray(m_qmlType.typeId().name()).chopped(1);
    }

    if (m_document)
        setupICs(m_document, &m_inlineComponentData, finalUrl(), m_compiledData);
    else
        setupICs(m_compiledData, &m_inlineComponentData, finalUrl(), m_compiledData);

    QV4::CompiledData::ResolvedTypeReferenceMap resolvedTypeCache;
    QQmlRefPointer<QQmlTypeNameCache> typeNameCache;

    // If we've pulled the CU from the memory cache, we don't need to do any verification.
    const bool verifyCaches = !m_compiledData
            || (m_compiledData->resolvedTypes.isEmpty() && !m_compiledData->typeNameCache);

    if (verifyCaches) {
        QQmlError error = buildTypeResolutionCaches(&typeNameCache, &resolvedTypeCache);
        if (error.isValid()) {
            setError(error);
            qDeleteAll(resolvedTypeCache);
            return;
        }
    }

    const auto dependencyHasher = [&resolvedTypeCache, this]() {
        return typeLoader()->hashDependencies(&resolvedTypeCache, m_compositeSingletons);
    };

    // verify if any dependencies changed if we're using a cache
    if (m_document.isNull() && verifyCaches) {
        const QQmlError error = createTypeAndPropertyCaches(typeNameCache, resolvedTypeCache);
        if (!error.isValid() && m_compiledData->verifyChecksum(dependencyHasher)) {
            setCompileUnit(m_compiledData);
        } else {

            if (error.isValid()) {
                qCDebug(DBG_DISK_CACHE)
                        << "Failed to create property caches for"
                        << m_compiledData->fileName()
                        << "because" << error.description();
            } else {
                qCDebug(DBG_DISK_CACHE)
                        << "Checksum mismatch for cached version of"
                        << m_compiledData->fileName();
            }

            if (!loadFromSource())
                return;

            // We want to keep our resolve types ...
            m_compiledData->resolvedTypes.clear();
            // ... but we don't want the property caches we've created for the broken CU.
            for (QV4::ResolvedTypeReference *ref: std::as_const(resolvedTypeCache)) {
                const auto compilationUnit = ref->compilationUnit();
                if (compilationUnit.isNull()) {
                    // Inline component references without CU belong to the surrounding CU.
                    // We have to clear them. Inline component references to other documents
                    // have a CU.
                    if (!ref->type().isInlineComponentType())
                        continue;
                } else if (compilationUnit != m_compiledData) {
                    continue;
                }
                ref->setTypePropertyCache(QQmlPropertyCache::ConstPtr());
                ref->setCompilationUnit(QQmlRefPointer<QV4::CompiledData::CompilationUnit>());
            }

            m_compiledData.reset();
        }
    }

    if (!m_document.isNull()) {
        Q_ASSERT(verifyCaches);
        // Compile component
        compile(typeNameCache, &resolvedTypeCache, dependencyHasher);
        if (isError())
            return;
        else
            setCompileUnit(m_document);
    }

    {
        m_compiledData->inlineComponentData = m_inlineComponentData;
        {
            // Sanity check property bindings
            QQmlPropertyValidator validator(typeLoader(), m_importCache.data(), m_compiledData);
            QVector<QQmlError> errors = validator.validate();
            if (!errors.isEmpty()) {
                setError(errors);
                return;
            }
        }

        m_compiledData->finalizeCompositeType(qmlType());
    }

    {
        QQmlType type = QQmlMetaType::qmlType(finalUrl());
        if (m_compiledData && m_compiledData->unitData()->flags & QV4::CompiledData::Unit::IsSingleton) {
            if (!type.isValid()) {
                QQmlError error;
                error.setDescription(QQmlTypeLoader::tr("No matching type found, pragma Singleton files cannot be used by QQmlComponent."));
                setError(error);
                return;
            } else if (!type.isCompositeSingleton()) {
                QQmlError error;
                error.setDescription(QQmlTypeLoader::tr("pragma Singleton used with a non composite singleton type %1").arg(type.qmlTypeName()));
                setError(error);
                return;
            }
        } else {
            // If the type is CompositeSingleton but there was no pragma Singleton in the
            // QML file, lets report an error.
            if (type.isValid() && type.isCompositeSingleton()) {
                QString typeName = type.qmlTypeName();
                setError(QQmlTypeLoader::tr("qmldir defines type as singleton, but no pragma Singleton found in type %1.").arg(typeName));
                return;
            }
        }
    }

    {
        // Collect imported scripts
        m_compiledData->dependentScripts.reserve(m_scripts.size());
        for (int scriptIndex = 0; scriptIndex < m_scripts.size(); ++scriptIndex) {
            const QQmlTypeData::ScriptReference &script = m_scripts.at(scriptIndex);

            QStringView qualifier(script.qualifier);
            QString enclosingNamespace;

            const int lastDotIndex = qualifier.lastIndexOf(QLatin1Char('.'));
            if (lastDotIndex != -1) {
                enclosingNamespace = qualifier.left(lastDotIndex).toString();
                qualifier = qualifier.mid(lastDotIndex+1);
            }

            m_compiledData->typeNameCache->add(
                    qualifier.toString(), scriptIndex, enclosingNamespace);
            QQmlRefPointer<QQmlScriptData> scriptData = script.script->scriptData();
            m_compiledData->dependentScripts << scriptData;
        }
    }
}

void QQmlTypeData::completed()
{
    assertEngineThread();
    // Notify callbacks
    while (!m_callbacks.isEmpty()) {
        TypeDataCallback *callback = m_callbacks.takeFirst();
        callback->typeDataReady(this);
    }
}

bool QQmlTypeData::loadImplicitImport()
{
    assertTypeLoaderThread();

    m_implicitImportLoaded = true; // Even if we hit an error, count as loaded (we'd just keep hitting the error)

    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For local urls, add an implicit import "." as most overridden lookup.
    // This will also trigger the loading of the qmldir and the import of any native
    // types from available plugins.
    QList<QQmlError> implicitImportErrors;
    QString localQmldir;
    m_importCache->addImplicitImport(typeLoader(), &localQmldir, &implicitImportErrors);

    // When loading with QQmlImports::ImportImplicit, the imports are _appended_ to the namespace
    // in the order they are loaded. Therefore, the addImplicitImport above gets the highest
    // precedence. This is in contrast to normal priority imports. Those are _prepended_ in the
    // order they are loaded.
    if (!localQmldir.isEmpty()) {
        const QQmlTypeLoaderQmldirContent qmldir = typeLoader()->qmldirContent(localQmldir);
        const QList<QQmlDirParser::Import> moduleImports
                = QQmlMetaType::moduleImports(qmldir.typeNamespace(), QTypeRevision())
                + qmldir.imports();
        loadDependentImports(moduleImports, QString(), QTypeRevision(),
                             QQmlImportInstance::Implicit + 1, QQmlImports::ImportNoFlag,
                             &implicitImportErrors);
    }

    if (!implicitImportErrors.isEmpty()) {
        setError(implicitImportErrors);
        return false;
    }

    return true;
}

void QQmlTypeData::dataReceived(const SourceCodeData &data)
{
    assertTypeLoaderThread();

    m_backupSourceCode = data;

    if (tryLoadFromDiskCache())
        return;

    if (isError())
        return;

    if (!m_backupSourceCode.exists() || m_backupSourceCode.isEmpty()) {
        if (m_cachedUnitStatus == QQmlMetaType::CachedUnitLookupError::VersionMismatch)
            setError(QQmlTypeLoader::tr("File was compiled ahead of time with an incompatible version of Qt and the original file cannot be found. Please recompile"));
        else if (!m_backupSourceCode.exists())
            setError(QQmlTypeLoader::tr("No such file or directory"));
        else
            setError(QQmlTypeLoader::tr("File is empty"));
        return;
    }

    if (!loadFromSource())
        return;

    continueLoadFromIR();
}

void QQmlTypeData::initializeFromCachedUnit(const QQmlPrivate::CachedQmlUnit *unit)
{
    assertTypeLoaderThread();

    if (auto cu = QQmlMetaType::obtainCompilationUnit(finalUrl())) {
        if (loadFromDiskCache(cu))
            return;
    }

    if (unit->qmlData->qmlUnit()->nObjects == 0) {
        setError(QQmlTypeLoader::tr("Cached QML Unit has no objects"));
        return;
    }

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    QQmlIRLoader loader(unit->qmlData, m_document.data());
    loader.load();
    m_document->javaScriptCompilationUnit
            = QQmlRefPointer<QV4::CompiledData::CompilationUnit>(
                new QV4::CompiledData::CompilationUnit(unit->qmlData, unit->aotCompiledFunctions),
                QQmlRefPointer<QV4::CompiledData::CompilationUnit>::Adopt);
    continueLoadFromIR();
}

bool QQmlTypeData::loadFromSource()
{
    assertTypeLoaderThread();

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    m_document->jsModule.sourceTimeStamp = m_backupSourceCode.sourceTimeStamp();
    QmlIR::IRBuilder compiler;

    QString sourceError;
    const QString source = m_backupSourceCode.readAll(&sourceError);
    if (!sourceError.isEmpty()) {
        setError(sourceError);
        return false;
    }

    if (!compiler.generateFromQml(source, finalUrlString(), m_document.data())) {
        QList<QQmlError> errors;
        errors.reserve(compiler.errors.size());
        for (const QQmlJS::DiagnosticMessage &msg : std::as_const(compiler.errors)) {
            QQmlError e;
            e.setUrl(url());
            e.setLine(qmlConvertSourceCoordinate<quint32, int>(msg.loc.startLine));
            e.setColumn(qmlConvertSourceCoordinate<quint32, int>(msg.loc.startColumn));
            e.setDescription(msg.message);
            errors << e;
        }
        setError(errors);
        return false;
    }
    return true;
}

void QQmlTypeData::restoreIR(const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &unit)
{
    assertTypeLoaderThread();

    m_document.reset(
            new QmlIR::Document(urlString(), finalUrlString(), m_typeLoader->isDebugging()));
    QQmlIRLoader loader(unit->unitData(), m_document.data());
    loader.load();
    m_document->javaScriptCompilationUnit = unit;
    continueLoadFromIR();
}

void QQmlTypeData::continueLoadFromIR()
{
    assertTypeLoaderThread();

    for (auto const& object: m_document->objects) {
        for (auto it = object->inlineComponentsBegin(); it != object->inlineComponentsEnd(); ++it) {
            QString const nameString = m_document->stringAt(it->nameIndex);
            auto importUrl = finalUrl();
            importUrl.setFragment(nameString);
            auto import = new QQmlImportInstance(); // Note: The cache takes ownership of the QQmlImportInstance
            m_importCache->addInlineComponentImport(import, nameString, importUrl);
        }
    }

    m_typeReferences.collectFromObjects(m_document->objects.constBegin(), m_document->objects.constEnd());
    m_importCache->setBaseUrl(finalUrl(), finalUrlString());

    // For remote URLs, we don't delay the loading of the implicit import
    // because the loading probably requires an asynchronous fetch of the
    // qmldir (so we can't load it just in time).
    if (!finalUrl().scheme().isEmpty()) {
        QUrl qmldirUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (!QQmlImports::isLocal(qmldirUrl)) {
            if (!loadImplicitImport())
                return;
            // This qmldir is for the implicit import
            auto implicitImport = std::make_shared<PendingImport>();
            implicitImport->uri = QLatin1String(".");
            implicitImport->version = QTypeRevision();
            QList<QQmlError> errors;

            if (!fetchQmldir(qmldirUrl, implicitImport, 1, &errors)) {
                setError(errors);
                return;
            }
        }
    }

    QList<QQmlError> errors;

    for (const QV4::CompiledData::Import *import : std::as_const(m_document->imports)) {
        if (!addImport(import, {}, &errors)) {
            Q_ASSERT(errors.size());

            // We're only interested in the chronoligically last error. The previous
            // errors might be from unsuccessfully trying to load a module from the
            // resource file system.
            QQmlError error = errors.first();
            error.setUrl(m_importCache->baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(import->location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(import->location.column()));
            setError(error);
            return;
        }
    }
}

void QQmlTypeData::allDependenciesDone()
{
    assertTypeLoaderThread();

    QQmlTypeLoader::Blob::allDependenciesDone();

    if (!m_typesResolved) {
        // Check that all imports were resolved
        QList<QQmlError> errors;
        auto it = m_unresolvedImports.constBegin(), end = m_unresolvedImports.constEnd();
        for ( ; it != end; ++it) {
            const PendingImportPtr &import = *it;
            if (import->priority != 0)
                continue;

            // If the import was potentially remote and all the network requests have failed,
            // we now know that there is no qmldir. We can register its types.
            if (registerPendingTypes(import))
                continue;

            // This import was not resolved
            QQmlError error;
            error.setDescription(QQmlTypeLoader::tr("module \"%1\" is not installed")
                                         .arg(import->uri));
            error.setUrl(m_importCache->baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(
                    import->location.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(
                    import->location.column()));
            errors.prepend(error);
        }
        if (errors.size()) {
            setError(errors);
            return;
        }

        resolveTypes();
        m_typesResolved = true;
    }
}

void QQmlTypeData::downloadProgressChanged(qreal p)
{
    assertEngineThread();

    for (int ii = 0; ii < m_callbacks.size(); ++ii) {
        TypeDataCallback *callback = m_callbacks.at(ii);
        callback->typeDataProgress(this, p);
    }
}

QString QQmlTypeData::stringAt(int index) const
{
    if (m_compiledData)
        return m_compiledData->stringAt(index);
    return m_document->jsGenerator.stringTable.stringForIndex(index);
}

void QQmlTypeData::compile(const QQmlRefPointer<QQmlTypeNameCache> &typeNameCache,
                           QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache,
                           const QV4::CompiledData::DependentTypesHasher &dependencyHasher)
{
    assertTypeLoaderThread();

    Q_ASSERT(m_compiledData.isNull());

    const bool typeRecompilation = m_document
            && m_document->javaScriptCompilationUnit
            && m_document->javaScriptCompilationUnit->unitData()
            && (m_document->javaScriptCompilationUnit->unitData()->flags
                & QV4::CompiledData::Unit::PendingTypeCompilation);

    QQmlTypeCompiler compiler(
            typeLoader(), this, m_document.data(), resolvedTypeCache, dependencyHasher);
    auto compilationUnit = compiler.compile();
    if (!compilationUnit) {
        qDeleteAll(*resolvedTypeCache);
        resolvedTypeCache->clear();
        setError(compiler.compilationErrors());
        return;
    }

    const bool trySaveToDisk = m_typeLoader->writeCacheFile() && !typeRecompilation;
    if (trySaveToDisk) {
        QString errorString;
        if (compilationUnit->saveToDisk(url(), &errorString)) {
            QString error;
            if (!compilationUnit->loadFromDisk(url(), m_backupSourceCode.sourceTimeStamp(), &error)) {
                // ignore error, keep using the in-memory compilation unit.
            }
        } else {
            qCDebug(DBG_DISK_CACHE) << "Error saving cached version of"
                                    << compilationUnit->fileName() << "to disk:" << errorString;
        }
    }

    m_compiledData = std::move(compilationUnit);
    m_compiledData->typeNameCache = typeNameCache;
    m_compiledData->resolvedTypes = *resolvedTypeCache;
    m_compiledData->propertyCaches = std::move(*compiler.propertyCaches());
    Q_ASSERT(m_compiledData->propertyCaches.count()
             == static_cast<int>(m_compiledData->objectCount()));
}

void QQmlTypeData::resolveTypes()
{
    assertTypeLoaderThread();

    // Load the implicit import since it may have additional scripts.
    if (!m_implicitImportLoaded && !loadImplicitImport())
        return;

    // Add any imported scripts to our resolved set
    const auto resolvedScripts = m_importCache->resolvedScripts();
    for (const QQmlImports::ScriptReference &script : resolvedScripts) {
        QQmlRefPointer<QQmlScriptBlob> blob
                = typeLoader()->getScript(script.location, script.fileName);
        addDependency(blob.data());

        ScriptReference ref;
        //ref.location = ...
        if (!script.qualifier.isEmpty())
        {
            ref.qualifier = script.qualifier + QLatin1Char('.') + script.nameSpace;
            // Add a reference to the enclosing namespace
            m_namespaces.insert(script.qualifier);
        } else {
            ref.qualifier = script.nameSpace;
        }

        ref.script = blob;
        m_scripts << ref;
    }

    // Lets handle resolved composite singleton types
    const auto resolvedCompositeSingletons = m_importCache->resolvedCompositeSingletons();
    for (const QQmlImports::CompositeSingletonReference &csRef : resolvedCompositeSingletons) {
        TypeReference ref;
        QString typeName;
        if (!csRef.prefix.isEmpty()) {
            typeName = csRef.prefix + QLatin1Char('.') + csRef.typeName;
            // Add a reference to the enclosing namespace
            m_namespaces.insert(csRef.prefix);
        } else {
            typeName = csRef.typeName;
        }

        QTypeRevision version = csRef.version;
        if (!resolveType(typeName, version, ref, -1, -1, true, QQmlType::CompositeSingletonType))
            return;

        if (ref.type.isCompositeSingleton()) {
            ref.typeData = typeLoader()->getType(ref.type.sourceUrl());
            if (ref.typeData->isWaiting() || m_waitingOnMe.contains(ref.typeData.data())) {
                qCDebug(lcCycle) << "Possible cyclic dependency detected between"
                                 << ref.typeData->urlString() << "and" << urlString();
                continue;
            }
            addDependency(ref.typeData.data());
            ref.prefix = csRef.prefix;

            m_compositeSingletons << ref;
        }
    }

    for (QV4::CompiledData::TypeReferenceMap::ConstIterator unresolvedRef = m_typeReferences.constBegin(), end = m_typeReferences.constEnd();
         unresolvedRef != end; ++unresolvedRef) {

        TypeReference ref; // resolved reference

        const bool reportErrors = unresolvedRef->errorWhenNotFound;

        QTypeRevision version;

        const QString name = stringAt(unresolvedRef.key());

        bool *selfReferenceDetection = unresolvedRef->needsCreation ? nullptr : &ref.selfReference;

        if (!resolveType(name, version, ref, unresolvedRef->location.line(),
                         unresolvedRef->location.column(), reportErrors,
                         QQmlType::AnyRegistrationType, selfReferenceDetection) && reportErrors)
            return;

        if (ref.type.isComposite() && !ref.selfReference) {
            ref.typeData = typeLoader()->getType(ref.type.sourceUrl());
            addDependency(ref.typeData.data());
        }
        if (ref.type.isInlineComponentType()) {
            QUrl containingTypeUrl = ref.type.sourceUrl();
            containingTypeUrl.setFragment(QString());
            if (!containingTypeUrl.isEmpty()) {
                auto typeData = typeLoader()->getType(containingTypeUrl);
                if (typeData.data() != this) {
                    ref.typeData = typeData;
                    addDependency(typeData.data());
                }
            }
        }

        ref.version = version;
        ref.location = unresolvedRef->location;
        ref.needsCreation = unresolvedRef->needsCreation;
        m_resolvedTypes.insert(unresolvedRef.key(), ref);
    }

    // ### this allows enums to work without explicit import or instantiation of the type
    if (!m_implicitImportLoaded)
        loadImplicitImport();
}

QQmlError QQmlTypeData::buildTypeResolutionCaches(
        QQmlRefPointer<QQmlTypeNameCache> *typeNameCache,
        QV4::CompiledData::ResolvedTypeReferenceMap *resolvedTypeCache) const
{
    assertTypeLoaderThread();

    typeNameCache->adopt(new QQmlTypeNameCache(m_importCache));

    for (const QString &ns: m_namespaces)
        (*typeNameCache)->add(ns);

    // Add any Composite Singletons that were used to the import cache
    for (const QQmlTypeData::TypeReference &singleton: m_compositeSingletons)
        (*typeNameCache)->add(singleton.type.qmlTypeName(), singleton.type.sourceUrl(), singleton.prefix);

    m_importCache->populateCache(typeNameCache->data());

    for (auto resolvedType = m_resolvedTypes.constBegin(), end = m_resolvedTypes.constEnd(); resolvedType != end; ++resolvedType) {
        auto ref = std::make_unique<QV4::ResolvedTypeReference>();
        QQmlType qmlType = resolvedType->type;
        if (resolvedType->typeData) {
            if (resolvedType->needsCreation && qmlType.isCompositeSingleton()) {
                return qQmlCompileError(resolvedType->location, tr("Composite Singleton Type %1 is not creatable.").arg(qmlType.qmlTypeName()));
            }
            ref->setCompilationUnit(resolvedType->typeData->compilationUnit());
            if (resolvedType->type.isInlineComponentType()) {
                // Inline component which is part of an already resolved type
                QString icName = qmlType.elementName();
                Q_ASSERT(!icName.isEmpty());

                const auto compilationUnit = resolvedType->typeData->compilationUnit();
                ref->setTypePropertyCache(compilationUnit->propertyCaches.at(
                    compilationUnit->inlineComponentId(icName)));
                ref->setType(std::move(qmlType));
                Q_ASSERT(ref->type().isInlineComponentType());
            }
        } else if (resolvedType->type.isInlineComponentType()) {
            ref->setType(qmlType);

            // Inline component
            // If it's defined in the same file we're currently compiling, we don't want to use it.
            // We're going to fill in the property caches later after all.
            if (qmlType.isValid()
                    && !QQmlMetaType::equalBaseUrls(finalUrl(), qmlType.sourceUrl())) {

                // this is required for inline components in singletons
                const QMetaType type = qmlType.typeId();
                if (auto unit = QQmlMetaType::obtainCompilationUnit(type)) {
                    ref->setCompilationUnit(std::move(unit));
                    ref->setTypePropertyCache(QQmlMetaType::propertyCacheForType(type));
                }
            }
        } else if (qmlType.isValid() && !resolvedType->selfReference) {
            ref->setType(qmlType);
            Q_ASSERT(ref->type().isValid());

            if (resolvedType->needsCreation && !qmlType.isCreatable()) {
                QString reason = qmlType.noCreationReason();
                if (reason.isEmpty())
                    reason = tr("Element is not creatable.");
                return qQmlCompileError(resolvedType->location, reason);
            }

            if (qmlType.containsRevisionedAttributes()) {
                // It can only have (revisioned) properties or methods if it has a metaobject
                Q_ASSERT(qmlType.metaObject());
                ref->setTypePropertyCache(
                    QQmlMetaType::propertyCache(qmlType, resolvedType->version));
            }
        }
        ref->setVersion(resolvedType->version);
        ref->doDynamicTypeCheck();
        resolvedTypeCache->insert(resolvedType.key(), ref.release());
    }
    QQmlError noError;
    return noError;
}

bool QQmlTypeData::resolveType(const QString &typeName, QTypeRevision &version,
                               TypeReference &ref, int lineNumber, int columnNumber,
                               bool reportErrors, QQmlType::RegistrationType registrationType,
                               bool *typeRecursionDetected)
{
    assertTypeLoaderThread();

    QQmlImportNamespace *typeNamespace = nullptr;
    QList<QQmlError> errors;

    bool typeFound = m_importCache->resolveType(
            typeLoader(), typeName, &ref.type, &version, &typeNamespace, &errors, registrationType,
            typeRecursionDetected);
    if (!typeNamespace && !typeFound && !m_implicitImportLoaded) {
        // Lazy loading of implicit import
        if (loadImplicitImport()) {
            // Try again to find the type
            errors.clear();
            typeFound = m_importCache->resolveType(
                    typeLoader(), typeName, &ref.type, &version, &typeNamespace, &errors,
                    registrationType, typeRecursionDetected);
        } else {
            return false; //loadImplicitImport() hit an error, and called setError already
        }
    }

    if ((!typeFound || typeNamespace) && reportErrors) {
        // Known to not be a type:
        //  - known to be a namespace (Namespace {})
        //  - type with unknown namespace (UnknownNamespace.SomeType {})
        QQmlError error;
        if (typeNamespace) {
            error.setDescription(QQmlTypeLoader::tr("Namespace %1 cannot be used as a type").arg(typeName));
        } else {
            if (errors.size()) {
                error = errors.takeFirst();
            } else {
                // this should not be possible!
                // Description should come from error provided by addImport() function.
                error.setDescription(QQmlTypeLoader::tr("Unreported error adding script import to import database"));
            }
            error.setUrl(m_importCache->baseUrl());
            error.setDescription(QQmlTypeLoader::tr("%1 %2").arg(typeName).arg(error.description()));
        }

        if (lineNumber != -1)
            error.setLine(lineNumber);
        if (columnNumber != -1)
            error.setColumn(columnNumber);

        errors.prepend(error);
        setError(errors);
        return false;
    }

    return true;
}

void QQmlTypeData::scriptImported(
        const QQmlRefPointer<QQmlScriptBlob> &blob, const QV4::CompiledData::Location &location,
        const QString &nameSpace, const QString &qualifier)
{
    assertTypeLoaderThread();

    ScriptReference ref;
    ref.script = blob;
    ref.location = location;
    ref.qualifier = qualifier.isEmpty() ? nameSpace : qualifier + QLatin1Char('.') + nameSpace;

    m_scripts << ref;
}

QT_END_NAMESPACE
