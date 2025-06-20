// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljslinter_p.h"

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsliteralbindingcheck_p.h>
#include <QtQmlCompiler/private/qqmljstranslationfunctionmismatchcheck_p.h>

#include <QtCore/qjsonobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qpluginloader.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qscopedpointer.h>

#include <QtQmlCompiler/private/qqmlsa_p.h>
#include <QtQmlCompiler/private/qqmljsloggingutils_p.h>

#if QT_CONFIG(library)
#    include <QtCore/qdiriterator.h>
#    include <QtCore/qlibrary.h>
#endif

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>


QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class CodegenWarningInterface final : public QV4::Compiler::CodegenWarningInterface
{
public:
    CodegenWarningInterface(QQmlJSLogger *logger) : m_logger(logger) { }

    void reportVarUsedBeforeDeclaration(const QString &name, const QString &fileName,
                                        QQmlJS::SourceLocation declarationLocation,
                                        QQmlJS::SourceLocation accessLocation) override
    {
        Q_UNUSED(fileName)

        m_logger->log("Identifier '%1' is used here before its declaration."_L1.arg(name),
                      qmlVarUsedBeforeDeclaration, accessLocation);
        m_logger->log("Note: declaration of '%1' here"_L1.arg(name), qmlVarUsedBeforeDeclaration,
                      declarationLocation, true, true, {}, {}, accessLocation.startLine);
    }

private:
    QQmlJSLogger *m_logger;
};

QQmlJSLinter::QQmlJSLinter(const QStringList &importPaths, const QStringList &extraPluginPaths,
                           bool useAbsolutePath)
    : m_useAbsolutePath(useAbsolutePath),
      m_enablePlugins(true),
      m_importer(importPaths, nullptr, UseOptionalImports)
{
    m_plugins = loadPlugins(extraPluginPaths);
}

QQmlJSLinter::Plugin::Plugin(QQmlJSLinter::Plugin &&plugin) noexcept
    : m_name(std::move(plugin.m_name))
    , m_description(std::move(plugin.m_description))
    , m_version(std::move(plugin.m_version))
    , m_author(std::move(plugin.m_author))
    , m_categories(std::move(plugin.m_categories))
    , m_instance(std::move(plugin.m_instance))
    , m_loader(std::move(plugin.m_loader))
    , m_isBuiltin(std::move(plugin.m_isBuiltin))
    , m_isInternal(std::move(plugin.m_isInternal))
    , m_isValid(std::move(plugin.m_isValid))
{
    // Mark the old Plugin as invalid and make sure it doesn't delete the loader
    Q_ASSERT(!plugin.m_loader);
    plugin.m_instance = nullptr;
    plugin.m_isValid = false;
}

#if QT_CONFIG(library)
QQmlJSLinter::Plugin::Plugin(QString path)
{
    m_loader = std::make_unique<QPluginLoader>(path);
    if (!parseMetaData(m_loader->metaData(), path))
        return;

    QObject *object = m_loader->instance();
    if (!object)
        return;

    m_instance = qobject_cast<QQmlSA::LintPlugin *>(object);
    if (!m_instance)
        return;

    m_isValid = true;
}
#endif

QQmlJSLinter::Plugin::Plugin(const QStaticPlugin &staticPlugin)
{
    if (!parseMetaData(staticPlugin.metaData(), u"built-in"_s))
        return;

    m_instance = qobject_cast<QQmlSA::LintPlugin *>(staticPlugin.instance());
    if (!m_instance)
        return;

    m_isValid = true;
}

QQmlJSLinter::Plugin::~Plugin()
{
#if QT_CONFIG(library)
    if (m_loader != nullptr) {
        m_loader->unload();
        m_loader->deleteLater();
    }
#endif
}

bool QQmlJSLinter::Plugin::parseMetaData(const QJsonObject &metaData, QString pluginName)
{
    const QString pluginIID = QStringLiteral(QmlLintPluginInterface_iid);

    if (metaData[u"IID"].toString() != pluginIID)
        return false;

    QJsonObject pluginMetaData = metaData[u"MetaData"].toObject();

    for (const QString &requiredKey :
         { u"name"_s, u"version"_s, u"author"_s, u"loggingCategories"_s }) {
        if (!pluginMetaData.contains(requiredKey)) {
            qWarning() << pluginName << "is missing the required " << requiredKey
                       << "metadata, skipping";
            return false;
        }
    }

    m_name = pluginMetaData[u"name"].toString();
    m_author = pluginMetaData[u"author"].toString();
    m_version = pluginMetaData[u"version"].toString();
    m_description = pluginMetaData[u"description"].toString(u"-/-"_s);
    m_isInternal = pluginMetaData[u"isInternal"].toBool(false);

    if (!pluginMetaData[u"loggingCategories"].isArray()) {
        qWarning() << pluginName << "has loggingCategories which are not an array, skipping";
        return false;
    }

    QJsonArray categories = pluginMetaData[u"loggingCategories"].toArray();

    for (const QJsonValue &value : std::as_const(categories)) {
        if (!value.isObject()) {
            qWarning() << pluginName << "has invalid loggingCategories entries, skipping";
            return false;
        }

        const QJsonObject object = value.toObject();

        for (const QString &requiredKey : { u"name"_s, u"description"_s }) {
            if (!object.contains(requiredKey)) {
                qWarning() << pluginName << " logging category is missing the required "
                           << requiredKey << "metadata, skipping";
                return false;
            }
        }

        const auto it = object.find("enabled"_L1);
        const bool ignored = (it != object.end() && !it->toBool());

        const QString prefix = (m_isInternal ? u""_s : u"Plugin."_s).append(m_name).append(u'.');
        const QString categoryId =
                prefix + object[u"name"].toString();
        const auto settingsNameIt = object.constFind(u"settingsName");
        const QString settingsName = (settingsNameIt == object.constEnd())
                ? categoryId
                : prefix + settingsNameIt->toString(categoryId);
        m_categories << QQmlJS::LoggerCategory{ categoryId, settingsName,
                                                object["description"_L1].toString(), QtWarningMsg,
                                                ignored };
        const auto itLevel = object.find("defaultLevel"_L1);
        if (itLevel == object.end())
            continue;

        const QString level = itLevel->toString();
        if (!QQmlJS::LoggingUtils::applyLevelToCategory(level, m_categories.last())) {
            qWarning() << "Invalid logging level" << level << "provided for"
                       << m_categories.last().id().name().toString()
                       << "(allowed are: disable, info, warning, error) found in plugin metadata.";
        }
    }

    return true;
}

std::vector<QQmlJSLinter::Plugin> QQmlJSLinter::loadPlugins(QStringList extraPluginPaths)
{
    std::vector<Plugin> plugins;

    QDuplicateTracker<QString> seenPlugins;

    const auto &staticPlugins = QPluginLoader::staticPlugins();
    for (const QStaticPlugin &staticPlugin : staticPlugins) {
        Plugin plugin(staticPlugin);
        if (!plugin.isValid())
            continue;

        if (seenPlugins.hasSeen(plugin.name().toLower())) {
            qWarning() << "Two plugins named" << plugin.name()
                       << "present, make sure no plugins are duplicated. The second plugin will "
                          "not be loaded.";
            continue;
        }

        plugins.push_back(std::move(plugin));
    }

#if QT_CONFIG(library)
    const QStringList paths = [&extraPluginPaths]() {
        QStringList result{ extraPluginPaths };
        const QStringList libraryPaths = QCoreApplication::libraryPaths();
        for (const auto &path : libraryPaths) {
            result.append(path + u"/qmllint"_s);
        }
        return result;
    }();
    for (const QString &pluginDir : paths) {
        QDirIterator it{ pluginDir, QDir::Files };

        while (it.hasNext()) {
            auto potentialPlugin = it.next();

            if (!QLibrary::isLibrary(potentialPlugin))
                continue;

            Plugin plugin(potentialPlugin);

            if (!plugin.isValid())
                continue;

            if (seenPlugins.hasSeen(plugin.name().toLower())) {
                qWarning() << "Two plugins named" << plugin.name()
                           << "present, make sure no plugins are duplicated. The second plugin "
                              "will not be loaded.";
                continue;
            }

            plugins.push_back(std::move(plugin));
        }
    }
#endif
    Q_UNUSED(extraPluginPaths)
    return plugins;
}

void QQmlJSLinter::parseComments(QQmlJSLogger *logger,
                                 const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QString>> disablesPerLine;
    QHash<int, QSet<QString>> enablesPerLine;
    QHash<int, QSet<QString>> oneLineDisablesPerLine;

    const QString code = logger->code();
    const QStringList lines = code.split(u'\n');
    const auto loggerCategories = logger->categories();

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ', Qt::SkipEmptyParts);
        if (words.size() < 2)
            continue;

        QSet<QString> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto categoryExists = std::any_of(
                    loggerCategories.cbegin(), loggerCategories.cend(),
                    [&](const QQmlJS::LoggerCategory &cat) { return cat.id().name() == category; });

            if (categoryExists)
                categories << category;
            else
                logger->log(u"qmllint directive on unknown category \"%1\""_s.arg(category),
                            qmlInvalidLintDirective, loc);
        }

        if (categories.isEmpty()) {
            const auto &loggerCategories = logger->categories();
            for (const auto &option : loggerCategories)
                categories << option.id().name().toString();
        }

        const QString command = words.at(1);
        if (command == u"disable"_s) {
            if (const qsizetype lineIndex = loc.startLine - 1; lineIndex < lines.size()) {
                const QString line = lines[lineIndex];
                const QString preComment = line.left(line.indexOf(comment) - 2);

                bool lineHasContent = false;
                for (qsizetype i = 0; i < preComment.size(); i++) {
                    if (!preComment[i].isSpace()) {
                        lineHasContent = true;
                        break;
                    }
                }

                if (lineHasContent)
                    oneLineDisablesPerLine[loc.startLine] |= categories;
                else
                    disablesPerLine[loc.startLine] |= categories;
            }
        } else if (command == u"enable"_s) {
            enablesPerLine[loc.startLine + 1] |= categories;
        } else {
            logger->log(u"Invalid qmllint directive \"%1\" provided"_s.arg(command),
                        qmlInvalidLintDirective, loc);
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QString> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.size(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

static void addJsonWarning(QJsonArray &warnings, const QQmlJS::DiagnosticMessage &message,
                           QAnyStringView id, const std::optional<QQmlJSFixSuggestion> &suggestion = {})
{
    QJsonObject jsonMessage;

    QString type;
    switch (message.type) {
    case QtDebugMsg:
        type = u"debug"_s;
        break;
    case QtWarningMsg:
        type = u"warning"_s;
        break;
    case QtCriticalMsg:
        type = u"critical"_s;
        break;
    case QtFatalMsg:
        type = u"fatal"_s;
        break;
    case QtInfoMsg:
        type = u"info"_s;
        break;
    default:
        type = u"unknown"_s;
        break;
    }

    jsonMessage[u"type"_s] = type;
    jsonMessage[u"id"_s] = id.toString();

    if (message.loc.isValid()) {
        jsonMessage[u"line"_s] = static_cast<int>(message.loc.startLine);
        jsonMessage[u"column"_s] = static_cast<int>(message.loc.startColumn);
        jsonMessage[u"charOffset"_s] = static_cast<int>(message.loc.offset);
        jsonMessage[u"length"_s] = static_cast<int>(message.loc.length);
    }

    jsonMessage[u"message"_s] = message.message;

    QJsonArray suggestions;
    const auto convertLocation = [](const QQmlJS::SourceLocation &source, QJsonObject *target) {
        target->insert("line"_L1, int(source.startLine));
        target->insert("column"_L1, int(source.startColumn));
        target->insert("charOffset"_L1, int(source.offset));
        target->insert("length"_L1, int(source.length));
    };
    if (suggestion.has_value()) {
        QJsonObject jsonFix {
            { "message"_L1, suggestion->fixDescription() },
            { "replacement"_L1, suggestion->replacement() },
            { "isHint"_L1, !suggestion->isAutoApplicable() },
        };
        convertLocation(suggestion->location(), &jsonFix);
        const QString filename = suggestion->filename();
        if (!filename.isEmpty())
            jsonFix.insert("fileName"_L1, filename);
        suggestions << jsonFix;

        const QString hint = suggestion->hint();
        if (!hint.isEmpty()) {
            // We need to keep compatibility with the JSON format.
            // Therefore the overly verbose encoding of the hint.
            QJsonObject jsonHint {
                { "message"_L1,  hint },
                { "replacement"_L1, QString() },
                { "isHint"_L1, true }
            };
            convertLocation(QQmlJS::SourceLocation(), &jsonHint);
            suggestions << jsonHint;
        }
    }
    jsonMessage[u"suggestions"] = suggestions;

    warnings << jsonMessage;

}

void QQmlJSLinter::processMessages(QJsonArray &warnings)
{
    m_logger->iterateAllMessages([&](const Message &message) {
        addJsonWarning(warnings, message, message.id, message.fixSuggestion);
    });
}

QQmlJSLinter::LintResult QQmlJSLinter::lintFile(const QString &filename,
                                                const QString *fileContents, const bool silent,
                                                QJsonArray *json, const QStringList &qmlImportPaths,
                                                const QStringList &qmldirFiles,
                                                const QStringList &resourceFiles,
                                                const QList<QQmlJS::LoggerCategory> &categories)
{
    // Make sure that we don't expose an old logger if we return before a new one is created.
    m_logger.reset();

    QJsonArray warnings;
    QJsonObject result;

    LintResult success = LintSuccess;

    QScopeGuard jsonOutput([&] {
        if (!json)
            return;

        result[u"filename"_s] = QFileInfo(filename).absoluteFilePath();
        result[u"warnings"] = warnings;
        result[u"success"] = success == LintSuccess;

        json->append(result);
    });

    QString code;

    if (fileContents == nullptr) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            if (json) {
                addJsonWarning(
                        warnings,
                        QQmlJS::DiagnosticMessage { QStringLiteral("Failed to open file %1: %2")
                                                            .arg(filename, file.errorString()),
                                                    QtCriticalMsg, QQmlJS::SourceLocation() },
                        qmlImport.name());
            } else if (!silent) {
                qWarning() << "Failed to open file" << filename << file.error();
            }
            success = FailedToOpen;
            return success;
        }

        code = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        code = *fileContents;
    }

    m_fileContents = code;

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    if (!(isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram())
                       : parser.parse())) {
        success = FailedToParse;
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            if (json) {
                addJsonWarning(warnings, m, qmlSyntax.name());
            } else if (!silent) {
                qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                                .arg(filename)
                                                .arg(m.loc.startLine)
                                                .arg(m.loc.startColumn)
                                                .arg(m.message);
            }
        }
        return success;
    }

    if (isJavaScript)
        return success;

    if (m_importer.importPaths() != qmlImportPaths)
        m_importer.setImportPaths(qmlImportPaths);

    std::optional<QQmlJSResourceFileMapper> mapper;
    if (!resourceFiles.isEmpty())
        mapper.emplace(resourceFiles);
    m_importer.setResourceFileMapper(mapper.has_value() ? &*mapper : nullptr);

    m_logger.reset(new QQmlJSLogger);
    m_logger->setFilePath(m_useAbsolutePath ? info.absoluteFilePath() : filename);
    m_logger->setCode(code);
    m_logger->setSilent(silent || json);

    QQmlJSScope::Ptr target = QQmlJSScope::create();
    QQmlJSImportVisitor v{ target, &m_importer, m_logger.get(),
                           QQmlJSImportVisitor::implicitImportDirectory(
                                   m_logger->filePath(), m_importer.resourceFileMapper()),
                           qmldirFiles };

    if (m_enablePlugins) {
        for (const Plugin &plugin : m_plugins) {
            for (const QQmlJS::LoggerCategory &category : plugin.categories())
                m_logger->registerCategory(category);
        }
    }

    for (auto it = categories.cbegin(); it != categories.cend(); ++it) {
        if (auto logger = *it; !QQmlJS::LoggerCategoryPrivate::get(&logger)->hasChanged())
            continue;

        m_logger->setCategoryIgnored(it->id(), it->isIgnored());
        m_logger->setCategoryLevel(it->id(), it->level());
    }

    parseComments(m_logger.get(), engine.comments());

    QQmlJSTypeResolver typeResolver(&m_importer);

    // Type resolving is using document parent mode here so that it produces fewer false
    // positives on the "parent" property of QQuickItem. It does produce a few false
    // negatives this way because items can be reparented. Furthermore, even if items
    // are not reparented, the document parent may indeed not be their visual parent.
    // See QTBUG-95530. Eventually, we'll need cleverer logic to deal with this.
    typeResolver.setParentMode(QQmlJSTypeResolver::UseDocumentParent);
    // We don't need to create tracked types and such as we are just linting the code
    // here and not actually compiling it. The duplicated scopes would cause issues
    // during linting.
    typeResolver.setCloneMode(QQmlJSTypeResolver::DoNotCloneTypes);

    typeResolver.init(&v, parser.rootNode());

    const QStringList resourcePaths = mapper
            ? mapper->resourcePaths(QQmlJSResourceFileMapper::localFileFilter(filename))
            : QStringList();
    const QString resolvedPath =
            (resourcePaths.size() == 1) ? u':' + resourcePaths.first() : filename;

    QQmlJSLinterCodegen codegen{ &m_importer, resolvedPath, qmldirFiles, m_logger.get() };
    codegen.setTypeResolver(std::move(typeResolver));

    using PassManagerPtr =
            std::unique_ptr<QQmlSA::PassManager,
                            decltype(&QQmlSA::PassManagerPrivate::deletePassManager)>;
    PassManagerPtr passMan(
            QQmlSA::PassManagerPrivate::createPassManager(&v, codegen.typeResolver()),
            &QQmlSA::PassManagerPrivate::deletePassManager);
    passMan->registerPropertyPass(std::make_unique<QQmlJSLiteralBindingCheck>(passMan.get()),
                                  QString(), QString(), QString());
    passMan->registerPropertyPass(
            std::make_unique<QQmlJSTranslationFunctionMismatchCheck>(passMan.get()), QString(),
            QString(), QString());

    QQmlSA::PropertyPassBuilder(passMan.get())
            .withOnCall([](QQmlSA::PropertyPass *self, const QQmlSA::Element &, const QString &,
                           const QQmlSA::Element &, QQmlSA::SourceLocation location) {
                self->emitWarning("Do not use 'eval'", qmlEval, location);
            })
            .registerOnBuiltin("GlobalObject", "eval");

    if (m_enablePlugins) {
        for (const Plugin &plugin : m_plugins) {
            if (!plugin.isValid() || !plugin.isEnabled())
                continue;

            QQmlSA::LintPlugin *instance = plugin.m_instance;
            Q_ASSERT(instance);
            instance->registerPasses(passMan.get(), QQmlJSScope::createQQmlSAElement(v.result()));
        }
    }
    passMan->analyze(QQmlJSScope::createQQmlSAElement(v.result()));

    if (m_logger->hasErrors()) {
        success = HasErrors;
        if (json)
            processMessages(warnings);
        return success;
    } else if (m_logger->hasWarnings())
        success = HasWarnings;

    if (passMan) {
        // passMan now has a pointer to the moved from type resolver
        // we fix this in setPassManager
        codegen.setPassManager(passMan.get());
    }
    QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                         const QQmlJSAotFunctionMap &, QString *) { return true; };

    QQmlJSCompileError error;

    QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_s);

    CodegenWarningInterface interface(m_logger.get());
    qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &interface, fileContents);

    QList<QQmlJS::DiagnosticMessage> globalWarnings = m_importer.takeGlobalWarnings();

    if (!globalWarnings.isEmpty()) {
        m_logger->log(QStringLiteral("Type warnings occurred while evaluating file:"), qmlImport,
                      QQmlJS::SourceLocation());
        m_logger->processMessages(globalWarnings, qmlImport);
    }

    if (m_logger->hasErrors())
        success = HasErrors;
    else if (m_logger->hasWarnings())
        success = HasWarnings;

    if (json)
        processMessages(warnings);
    return success;
}

QQmlJSLinter::LintResult QQmlJSLinter::lintModule(
        const QString &module, const bool silent, QJsonArray *json,
        const QStringList &qmlImportPaths, const QStringList &resourceFiles)
{
    // Make sure that we don't expose an old logger if we return before a new one is created.
    m_logger.reset();

    // We can't lint properly if a module has already been pre-cached
    m_importer.clearCache();

    if (m_importer.importPaths() != qmlImportPaths)
        m_importer.setImportPaths(qmlImportPaths);

    QQmlJSResourceFileMapper mapper(resourceFiles);
    if (!resourceFiles.isEmpty())
        m_importer.setResourceFileMapper(&mapper);
    else
        m_importer.setResourceFileMapper(nullptr);

    QJsonArray warnings;
    QJsonObject result;

    bool success = true;

    QScopeGuard jsonOutput([&] {
        if (!json)
            return;

        result[u"module"_s] = module;

        result[u"warnings"] = warnings;
        result[u"success"] = success;

        json->append(result);
    });

    m_logger.reset(new QQmlJSLogger);
    m_logger->setFilePath(module);
    m_logger->setCode(u""_s);
    m_logger->setSilent(silent || json);

    const QQmlJSImporter::ImportedTypes types = m_importer.importModule(module);

    QList<QQmlJS::DiagnosticMessage> importWarnings =
            m_importer.takeGlobalWarnings() + types.warnings();

    if (!importWarnings.isEmpty()) {
        m_logger->log(QStringLiteral("Warnings occurred while importing module:"), qmlImport,
                      QQmlJS::SourceLocation());
        m_logger->processMessages(importWarnings, qmlImport);
    }

    QMap<QString, QSet<QString>> missingTypes;
    QMap<QString, QSet<QString>> partiallyResolvedTypes;

    const QString modulePrefix = u"$module$."_s;
    const QString internalPrefix = u"$internal$."_s;

    for (auto &&[typeName, importedScope] : types.types().asKeyValueRange()) {
        QString name = typeName;
        const QQmlJSScope::ConstPtr scope = importedScope.scope;

        if (name.startsWith(modulePrefix))
            continue;

        if (name.startsWith(internalPrefix)) {
            name = name.mid(internalPrefix.size());
        }

        if (scope.isNull()) {
            if (!missingTypes.contains(name))
                missingTypes[name] = {};
            continue;
        }

        if (!scope->isFullyResolved()) {
            if (!partiallyResolvedTypes.contains(name))
                partiallyResolvedTypes[name] = {};
        }
        const auto &ownProperties = scope->ownProperties();
        for (const auto &property : ownProperties) {
            if (property.typeName().isEmpty()) {
                // If the type name is empty, then it's an intentional vaguery i.e. for some
                // builtins
                continue;
            }
            if (property.type().isNull()) {
                missingTypes[property.typeName()]
                        << scope->internalName() + u'.' + property.propertyName();
                continue;
            }
            if (!property.type()->isFullyResolved()) {
                partiallyResolvedTypes[property.typeName()]
                        << scope->internalName() + u'.' + property.propertyName();
            }
        }
        if (scope->attachedType() && !scope->attachedType()->isFullyResolved()) {
            m_logger->log(u"Attached type of \"%1\" not fully resolved"_s.arg(name),
                          qmlUnresolvedType, scope->sourceLocation());
        }

        const auto &ownMethods = scope->ownMethods();
        for (const auto &method : ownMethods) {
            if (method.returnTypeName().isEmpty())
                continue;
            if (method.returnType().isNull()) {
                missingTypes[method.returnTypeName()] << u"return type of "_s
                                + scope->internalName() + u'.' + method.methodName() + u"()"_s;
            } else if (!method.returnType()->isFullyResolved()) {
                partiallyResolvedTypes[method.returnTypeName()] << u"return type of "_s
                                + scope->internalName() + u'.' + method.methodName() + u"()"_s;
            }

            const auto parameters = method.parameters();
            for (qsizetype i = 0; i < parameters.size(); i++) {
                auto &parameter = parameters[i];
                const QString typeName = parameter.typeName();
                const QSharedPointer<const QQmlJSScope> type = parameter.type();
                if (typeName.isEmpty())
                    continue;
                if (type.isNull()) {
                    missingTypes[typeName] << u"parameter %1 of "_s.arg(i + 1)
                                    + scope->internalName() + u'.' + method.methodName() + u"()"_s;
                    continue;
                }
                if (!type->isFullyResolved()) {
                    partiallyResolvedTypes[typeName] << u"parameter %1 of "_s.arg(i + 1)
                                    + scope->internalName() + u'.' + method.methodName() + u"()"_s;
                    continue;
                }
            }
        }
    }

    for (auto &&[name, uses] :  missingTypes.asKeyValueRange()) {
        QString message = u"Type \"%1\" not found"_s.arg(name);

        if (!uses.isEmpty()) {
            const QStringList usesList = QStringList(uses.begin(), uses.end());
            message += u". Used in %1"_s.arg(usesList.join(u", "_s));
        }

        m_logger->log(message, qmlUnresolvedType, QQmlJS::SourceLocation());
    }

    for (auto &&[name, uses] : partiallyResolvedTypes.asKeyValueRange()) {
        QString message = u"Type \"%1\" is not fully resolved"_s.arg(name);

        if (!uses.isEmpty()) {
            const QStringList usesList = QStringList(uses.begin(), uses.end());
            message += u". Used in %1"_s.arg(usesList.join(u", "_s));
        }

        m_logger->log(message, qmlUnresolvedType, QQmlJS::SourceLocation());
    }

    if (json)
        processMessages(warnings);

    success &= !m_logger->hasWarnings() && !m_logger->hasErrors();

    return success ? LintSuccess : HasWarnings;
}

QQmlJSLinter::FixResult QQmlJSLinter::applyFixes(QString *fixedCode, bool silent)
{
    Q_ASSERT(fixedCode != nullptr);

    // This means that the necessary analysis for applying fixes hasn't run for some reason
    // (because it was JS file, a syntax error etc.). We can't procede without it and if an error
    // has occurred that has to be handled by the caller of lintFile(). Just say that there is
    // nothing to fix.
    if (m_logger == nullptr)
        return NothingToFix;

    QString code = m_fileContents;

    QList<QQmlJSFixSuggestion> fixesToApply;

    QFileInfo info(m_logger->filePath());
    const QString currentFileAbsolutePath = info.absoluteFilePath();

    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    if (isESModule || isJavaScript)
        return NothingToFix;

    m_logger->iterateAllMessages([&](const Message &msg) {
        if (!msg.fixSuggestion.has_value() || !msg.fixSuggestion->isAutoApplicable())
            return;

        // Ignore fix suggestions for other files
        const QString filename = msg.fixSuggestion->filename();
        if (!filename.isEmpty()
                && QFileInfo(filename).absoluteFilePath() != currentFileAbsolutePath) {
            return;
        }

        fixesToApply << msg.fixSuggestion.value();
    });

    if (fixesToApply.isEmpty())
        return NothingToFix;

    std::sort(fixesToApply.begin(), fixesToApply.end(),
              [](const QQmlJSFixSuggestion &a, const QQmlJSFixSuggestion &b) {
                  return a.location().offset < b.location().offset;
              });

    const auto dupes = std::unique(fixesToApply.begin(), fixesToApply.end());
    fixesToApply.erase(dupes, fixesToApply.end());

    for (auto it = fixesToApply.begin(); it + 1 != fixesToApply.end(); it++) {
        const QQmlJS::SourceLocation srcLocA = it->location();
        const QQmlJS::SourceLocation srcLocB = (it + 1)->location();
        if (srcLocA.offset + srcLocA.length > srcLocB.offset) {
            if (!silent)
                qWarning() << "Fixes for two warnings are overlapping, aborting. Please file a bug "
                              "report.";
            return FixError;
        }
    }

    int offsetChange = 0;

    for (const auto &fix : std::as_const(fixesToApply)) {
        const QQmlJS::SourceLocation fixLocation = fix.location();
        qsizetype cutLocation = fixLocation.offset + offsetChange;
        const QString before = code.left(cutLocation);
        const QString after = code.mid(cutLocation + fixLocation.length);

        const QString replacement = fix.replacement();
        code = before + replacement + after;
        offsetChange += replacement.size() - fixLocation.length;
    }

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    bool success = parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();

        if (!silent) {
            qDebug() << "File became unparseable after suggestions were applied. Please file a bug "
                        "report.";
        } else {
            return FixError;
        }

        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                            .arg(m_logger->filePath())
                                            .arg(m.loc.startLine)
                                            .arg(m.loc.startColumn)
                                            .arg(m.message);
        }
        return FixError;
    }

    *fixedCode = code;
    return FixSuccess;
}

QT_END_NAMESPACE
