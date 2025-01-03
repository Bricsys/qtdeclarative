// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPELOADER_P_H
#define QQMLTYPELOADER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmldatablob_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltypeloaderthread_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qv4engine_p.h>

#include <QtQml/qtqmlglobal.h>
#include <QtQml/qqmlerror.h>

#include <QtCore/qcache.h>
#include <QtCore/qmutex.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlScriptBlob;
class QQmlQmldirData;
class QQmlTypeData;
class QQmlEngineExtensionInterface;
class QQmlExtensionInterface;
class QQmlProfiler;
class QQmlTypeLoaderThread;
class QQmlEngine;

class Q_QML_EXPORT QQmlTypeLoader
{
    Q_DECLARE_TR_FUNCTIONS(QQmlTypeLoader)
public:
    using ChecksumCache = QHash<quintptr, QByteArray>;
    enum Mode { PreferSynchronous, Asynchronous, Synchronous };

    class Q_QML_EXPORT Blob : public QQmlDataBlob
    {
    public:
        Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader);
        ~Blob() override;

        const QQmlImports *imports() const { return m_importCache.data(); }

        void setCachedUnitStatus(QQmlMetaType::CachedUnitLookupError status) { m_cachedUnitStatus = status; }

        struct PendingImport
        {
            QString uri;
            QString qualifier;

            QV4::CompiledData::Import::ImportType type
                = QV4::CompiledData::Import::ImportType::ImportLibrary;
            QV4::CompiledData::Location location;

            QQmlImports::ImportFlags flags;
            quint8 precedence = 0;
            int priority = 0;

            QTypeRevision version;

            PendingImport() = default;
            PendingImport(const QQmlRefPointer<Blob> &blob, const QV4::CompiledData::Import *import,
                          QQmlImports::ImportFlags flags);
        };
        using PendingImportPtr = std::shared_ptr<PendingImport>;

        void importQmldirScripts(const PendingImportPtr &import, const QQmlTypeLoaderQmldirContent &qmldir, const QUrl &qmldirUrl);
        bool handleLocalQmldirForImport(
                const PendingImportPtr &import, const QString &qmldirFilePath,
                const QString &qmldirUrl, QList<QQmlError> *errors);

    protected:
        bool addImport(const QV4::CompiledData::Import *import, QQmlImports::ImportFlags,
                       QList<QQmlError> *errors);
        bool addImport(const PendingImportPtr &import, QList<QQmlError> *errors);

        bool fetchQmldir(
                const QUrl &url, const PendingImportPtr &import, int priority,
                QList<QQmlError> *errors);
        bool updateQmldir(const QQmlRefPointer<QQmlQmldirData> &data, const PendingImportPtr &import, QList<QQmlError> *errors);

    private:
        bool addScriptImport(const PendingImportPtr &import);
        bool addFileImport(const PendingImportPtr &import, QList<QQmlError> *errors);
        bool addLibraryImport(const PendingImportPtr &import, QList<QQmlError> *errors);

        virtual bool qmldirDataAvailable(const QQmlRefPointer<QQmlQmldirData> &, QList<QQmlError> *);

        virtual void scriptImported(
                const QQmlRefPointer<QQmlScriptBlob> &, const QV4::CompiledData::Location &,
                const QString &, const QString &)
        {
            assertTypeLoaderThread();
        }

        void dependencyComplete(const QQmlDataBlob::Ptr &) override;

        bool loadImportDependencies(
                const PendingImportPtr &currentImport, const QString &qmldirUri,
                QQmlImports::ImportFlags flags, QList<QQmlError> *errors);

    protected:
        bool loadDependentImports(
                const QList<QQmlDirParser::Import> &imports, const QString &qualifier,
                QTypeRevision version, quint16 precedence, QQmlImports::ImportFlags flags,
                QList<QQmlError> *errors);
        virtual QString stringAt(int) const { return QString(); }

        bool isDebugging() const;
        bool readCacheFile() const;
        bool writeCacheFile() const;
        QQmlMetaType::CacheMode aotCacheMode() const;

        QQmlRefPointer<QQmlImports> m_importCache;
        QVector<PendingImportPtr> m_unresolvedImports;
        QVector<QQmlRefPointer<QQmlQmldirData>> m_qmldirs;
        QQmlMetaType::CachedUnitLookupError m_cachedUnitStatus = QQmlMetaType::CachedUnitLookupError::NoError;
    };

    QQmlTypeLoader(QQmlEngine *);
    ~QQmlTypeLoader();

    template<
            typename Engine,
            typename EnginePrivate = QQmlEnginePrivate,
            typename = std::enable_if_t<std::is_same_v<Engine, QQmlEngine>>>
    static QQmlTypeLoader *get(Engine *engine)
    {
        return get(EnginePrivate::get(engine));
    }

    template<
            typename Engine,
            typename = std::enable_if_t<std::is_same_v<Engine, QQmlEnginePrivate>>>
    static QQmlTypeLoader *get(Engine *engine)
    {
        return &engine->typeLoader;
    }

    static void sanitizeUNCPath(QString *path)
    {
        // This handles the UNC path case as when the path is retrieved from the QUrl it
        // will convert the host name from upper case to lower case. So the absoluteFilePath
        // is changed at this point to make sure it will match later on in that case.
        if (path->startsWith(QStringLiteral("//"))) {
            // toLocalFile() since that faithfully restores all the things you can do to a
            // path but not a URL, in particular weird characters like '%'.
            *path = QUrl::fromLocalFile(*path).toLocalFile();
        }
    }

    ChecksumCache *checksumCache() { return &m_checksumCache; }
    const ChecksumCache *checksumCache() const { return &m_checksumCache; }

    static QUrl normalize(const QUrl &unNormalizedUrl);

    QQmlRefPointer<QQmlTypeData> getType(const QUrl &unNormalizedUrl, Mode mode = PreferSynchronous);
    QQmlRefPointer<QQmlTypeData> getType(const QByteArray &, const QUrl &url, Mode mode = PreferSynchronous);

    QQmlRefPointer<QV4::CompiledData::CompilationUnit> injectScript(
            const QUrl &relativeUrl, const QV4::CompiledData::Unit *unit);

    QQmlRefPointer<QQmlScriptBlob> getScript(const QUrl &unNormalizedUrl, const QUrl &relativeUrl);
    QQmlRefPointer<QQmlQmldirData> getQmldir(const QUrl &);

    QString absoluteFilePath(const QString &path);
    bool fileExists(const QString &path, const QString &file);
    bool directoryExists(const QString &path);

    const QQmlTypeLoaderQmldirContent qmldirContent(const QString &filePath);
    void setQmldirContent(const QString &filePath, const QString &content);

    void clearCache();
    void trimCache();

    bool isTypeLoaded(const QUrl &url) const;
    bool isScriptLoaded(const QUrl &url) const;

    void lock() { ensureThread()->lock(); }
    void unlock() { ensureThread()->unlock(); }

    void load(const QQmlDataBlob::Ptr &,  Mode = PreferSynchronous);
    void loadWithStaticData(const QQmlDataBlob::Ptr &blob, const QByteArray &, Mode = PreferSynchronous);
    void loadWithCachedUnit(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit, Mode mode = PreferSynchronous);
    void drop(const QQmlDataBlob::Ptr &blob);

    void initializeEngine(QQmlEngineExtensionInterface *, const char *);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void invalidate();

    void addUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    void removeUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor);
    QList<QQmlAbstractUrlInterceptor *> urlInterceptors() const;
    QUrl interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const;
    bool hasUrlInterceptors() const;

#if !QT_CONFIG(qml_debug)
    quintptr profiler() const { return 0; }
    void setProfiler(quintptr) {}
#else
    QQmlProfiler *profiler() const { return m_profiler.data(); }
    void setProfiler(QQmlProfiler *profiler);
#endif // QT_CONFIG(qml_debug)

    QStringList importPathList() const { return m_importPaths; }
    void setImportPathList(const QStringList &paths);
    void addImportPath(const QString& dir);

    QStringList pluginPathList() const { return m_pluginPaths; }
    void setPluginPathList(const QStringList &paths);
    void addPluginPath(const QString& path);

    void setPluginInitialized(const QString &plugin) { m_initializedPlugins.insert(plugin); }
    bool isPluginInitialized(const QString &plugin) const
    {
        return m_initializedPlugins.contains(plugin);
    }

    void setModulePluginProcessingDone(const QString &module)
    {
        m_modulesForWhichPluginsHaveBeenProcessed.insert(module);
    }
    bool isModulePluginProcessingDone(const QString &module)
    {
        return m_modulesForWhichPluginsHaveBeenProcessed.contains(module);
    }

private:
    friend class QQmlDataBlob;
    friend class QQmlTypeLoaderThread;
#if QT_CONFIG(qml_network)
    friend class QQmlTypeLoaderNetworkReplyProxy;
#endif // qml_network

    enum PathType { Local, Remote, LocalOrRemote };

    enum LocalQmldirResult {
        QmldirFound,
        QmldirNotFound,
        QmldirInterceptedToRemote,
        QmldirRejected
    };

    struct QmldirInfo {
        QTypeRevision version;
        QString qmldirFilePath;
        QString qmldirPathUrl;
        QmldirInfo *next;
    };

    void startThread();
    void shutdownThread();
    QQmlTypeLoaderThread *ensureThread()
    {
        if (!m_thread)
            startThread();
        return m_thread;
    }

    void loadThread(const QQmlDataBlob::Ptr &);
    void loadWithStaticDataThread(const QQmlDataBlob::Ptr &, const QByteArray &);
    void loadWithCachedUnitThread(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);
#if QT_CONFIG(qml_network)
    void networkReplyFinished(QNetworkReply *);
    void networkReplyProgress(QNetworkReply *, qint64, qint64);

    typedef QHash<QNetworkReply *, QQmlDataBlob::Ptr> NetworkReplies;
#endif

    void setData(const QQmlDataBlob::Ptr &, const QByteArray &);
    void setData(const QQmlDataBlob::Ptr &, const QString &fileName);
    void setData(const QQmlDataBlob::Ptr &, const QQmlDataBlob::SourceCodeData &);
    void setCachedUnit(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit);

    QStringList importPathList(PathType type) const;
    void clearQmldirInfo();

    LocalQmldirResult locateLocalQmldir(
            QQmlTypeLoader::Blob *blob, const QQmlTypeLoader::Blob::PendingImportPtr &import,
            QList<QQmlError> *errors);

    typedef QHash<QUrl, QQmlRefPointer<QQmlTypeData>> TypeCache;
    typedef QHash<QUrl, QQmlRefPointer<QQmlScriptBlob>> ScriptCache;
    typedef QHash<QUrl, QQmlRefPointer<QQmlQmldirData>> QmldirCache;
    typedef QCache<QString, QCache<QString, bool> > ImportDirCache;
    typedef QStringHash<QQmlTypeLoaderQmldirContent *> ImportQmlDirCache;

    // URL interceptors must be set before loading any types. Otherwise we get data races.
    QList<QQmlAbstractUrlInterceptor *> m_urlInterceptors;
    QQmlEngine *m_engine;
    QQmlTypeLoaderThread *m_thread = nullptr;

#if QT_CONFIG(qml_debug)
    QScopedPointer<QQmlProfiler> m_profiler;
#endif

#if QT_CONFIG(qml_network)
    NetworkReplies m_networkReplies;
#endif
    TypeCache m_typeCache;
    ScriptCache m_scriptCache;
    QmldirCache m_qmldirCache;
    ImportDirCache m_importDirCache;
    ImportQmlDirCache m_importQmlDirCache;
    ChecksumCache m_checksumCache;
    int m_typeCacheTrimThreshold;

    QV4::ExecutionEngine::DiskCacheOptions m_diskCacheOptions
            = QV4::ExecutionEngine::DiskCache::Enabled;
    bool m_isDebugging = false;

    // Maps from an import to a linked list of qmldir info.
    // Used in locateLocalQmldir()
    QStringHash<QmldirInfo *> m_qmldirInfo;

    QStringList m_importPaths;
    QStringList m_pluginPaths;

    // Modules for which plugins have been loaded and processed in the context of this type
    // loader's engine. Plugins can have engine-specific initialization callbacks. This is why
    // we have to keep track of this.
    QSet<QString> m_modulesForWhichPluginsHaveBeenProcessed;

    // Plugins that have been initialized in the context of this engine. In theory, the same
    // plugin can be used for multiple modules. Therefore, we need to keep track of this
    // separately from m_modulesForWhichPluginsHaveBeenProcessed.
    QSet<QString> m_initializedPlugins;

    template<typename Loader>
    void doLoad(const Loader &loader, const QQmlDataBlob::Ptr &blob, Mode mode);
    void updateTypeCacheTrimThreshold();

    friend struct PlainLoader;
    friend struct CachedLoader;
    friend struct StaticLoader;
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADER_P_H
