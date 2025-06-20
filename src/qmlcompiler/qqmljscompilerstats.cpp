// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljscompilerstats_p.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

std::unique_ptr<AotStats> QQmlJSAotCompilerStats::s_instance = std::make_unique<AotStats>();
QString QQmlJSAotCompilerStats::s_moduleId;
bool QQmlJSAotCompilerStats::s_recordAotStats = false;

bool QQmlJS::AotStatsEntry::operator<(const AotStatsEntry &other) const
{
    if (line == other.line)
        return column < other.column;
    return line < other.line;
}

void AotStats::insert(const AotStats &other)
{
    for (const auto &[moduleUri, moduleStats] : other.m_entries.asKeyValueRange()) {
        m_entries[moduleUri].insert(moduleStats);
    }
}

std::optional<QList<QString>> AotStats::readAllLines(const QString &path)
{
    QFile aotstatsListFile(path);
    if (!aotstatsListFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug().noquote() << u"Could not open \"%1\" for reading"_s.arg(aotstatsListFile.fileName());
        return std::nullopt;
    }

    QStringList aotstatsFiles;
    QTextStream stream(&aotstatsListFile);
    while (!stream.atEnd())
        aotstatsFiles.append(stream.readLine());

    return aotstatsFiles;
}

std::optional<AotStats> AotStats::parseAotstatsFile(const QString &aotstatsPath)
{
    QFile file(aotstatsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug().noquote() << u"Could not open \"%1\""_s.arg(aotstatsPath);
        return std::nullopt;
    }

    return AotStats::fromJsonDocument(QJsonDocument::fromJson(file.readAll()));
}

std::optional<AotStats> AotStats::aggregateAotstatsList(const QString &aotstatsListPath)
{
    const auto aotstatsFiles = readAllLines(aotstatsListPath);
    if (!aotstatsFiles.has_value())
        return std::nullopt;

    AotStats aggregated;
    if (aotstatsFiles->empty())
        return aggregated;

    for (const auto &aotstatsFile : aotstatsFiles.value()) {
        auto parsed = parseAotstatsFile(aotstatsFile);
        if (!parsed.has_value())
            return std::nullopt;
        aggregated.insert(parsed.value());
    }

    return aggregated;
}

static constexpr int S_AOTSTATS_FORMAT_REVISION = 1; // Added support for skipping

static constexpr QLatin1StringView S_CODEGEN_RESULT{ "codegenResult" };
static constexpr QLatin1StringView S_COLUMN{ "column" };
static constexpr QLatin1StringView S_DURATION_MICROSECONDS{ "durationMicroseconds" };
static constexpr QLatin1StringView S_ENTRIES{ "entries" };
static constexpr QLatin1StringView S_FILE_PATH{ "filePath" };
static constexpr QLatin1StringView S_FORMAT_REVISION{ "formatRevision" };
static constexpr QLatin1StringView S_FUNCTION_NAME{ "functionName" };
static constexpr QLatin1StringView S_LINE{ "line" };
static constexpr QLatin1StringView S_MESSAGE{ "message" };
static constexpr QLatin1StringView S_MODULES{ "modules" };
static constexpr QLatin1StringView S_MODULE_FILES{ "moduleFiles" };
static constexpr QLatin1StringView S_MODULE_ID{ "moduleId" };

std::optional<AotStats> AotStats::fromJsonDocument(const QJsonDocument &document)
{
    QJsonObject root = document.object();
    const QJsonValue revision = root[S_FORMAT_REVISION];
    if (revision.isUndefined() || revision.toInt() != S_AOTSTATS_FORMAT_REVISION) {
        qDebug() << "AotStats format revision missmatch. Please try again with a clean build.";
        return std::nullopt;
    }

    const QJsonArray modulesArray = root[S_MODULES].toArray();
    QQmlJS::AotStats result;
    for (const auto &modulesArrayEntry : std::as_const(modulesArray)) {
        const auto &moduleObject = modulesArrayEntry.toObject();
        QString moduleId = moduleObject[S_MODULE_ID].toString();
        const QJsonArray &filesArray = moduleObject[S_MODULE_FILES].toArray();

        QHash<QString, QList<AotStatsEntry>> files;
        for (const auto &filesArrayEntry : filesArray) {
            const QJsonObject &fileObject = filesArrayEntry.toObject();
            QString filepath = fileObject[S_FILE_PATH].toString();
            const QJsonArray &statsArray = fileObject[S_ENTRIES].toArray();

            QList<AotStatsEntry> stats;
            for (const auto &statsArrayEntry : statsArray) {
                const auto &statsObject = statsArrayEntry.toObject();
                QQmlJS::AotStatsEntry stat;
                auto micros = statsObject[S_DURATION_MICROSECONDS].toInteger();
                stat.codegenDuration = std::chrono::microseconds(micros);
                stat.functionName = statsObject[S_FUNCTION_NAME].toString();
                stat.message = statsObject[S_MESSAGE].toString();
                stat.line = statsObject[S_LINE].toInt();
                stat.column = statsObject[S_COLUMN].toInt();
                stat.codegenResult = QQmlJS::CodegenResult(statsObject[S_CODEGEN_RESULT].toInt());
                stats.append(std::move(stat));
            }

            std::sort(stats.begin(), stats.end());
            files[filepath] = std::move(stats);
        }

        result.m_entries[moduleId] = std::move(files);
    }

    return result;
}

QJsonDocument AotStats::toJsonDocument() const
{
    QJsonArray modulesArray;
    for (auto it1 = m_entries.begin(); it1 != m_entries.end(); ++it1) {
        const QString moduleId = it1.key();
        const QHash<QString, QList<AotStatsEntry>> &files = it1.value();

        QJsonArray filesArray;
        for (auto it2 = files.begin(); it2 != files.end(); ++it2) {
            const QString &filename = it2.key();
            const QList<AotStatsEntry> &stats = it2.value();

            QJsonArray statsArray;
            for (const auto &stat : stats) {
                QJsonObject statObject;
                auto micros = static_cast<qint64>(stat.codegenDuration.count());
                statObject.insert(S_DURATION_MICROSECONDS, micros);
                statObject.insert(S_FUNCTION_NAME, stat.functionName);
                statObject.insert(S_MESSAGE, stat.message);
                statObject.insert(S_LINE, stat.line);
                statObject.insert(S_COLUMN, stat.column);
                using CodegenResType = std::underlying_type_t<QQmlJS::CodegenResult>;
                statObject.insert(S_CODEGEN_RESULT,
                                  static_cast<CodegenResType>(stat.codegenResult));
                statsArray.append(statObject);
            }

            QJsonObject o;
            o.insert(S_FILE_PATH, filename);
            o.insert(S_ENTRIES, statsArray);
            filesArray.append(o);
        }

        QJsonObject o;
        o.insert(S_MODULE_ID, moduleId);
        o.insert(S_MODULE_FILES, filesArray);
        modulesArray.append(o);
    }

    QJsonObject root;
    root.insert(S_FORMAT_REVISION, S_AOTSTATS_FORMAT_REVISION);
    root.insert(S_MODULES, modulesArray);
    return QJsonDocument(root);
}

void AotStats::registerFile(const QString &moduleId, const QString &filepath)
{
    m_entries[moduleId][filepath] = {};
}

void AotStats::addEntry(
        const QString &moduleId, const QString &filepath, const AotStatsEntry &entry)
{
    m_entries[moduleId][filepath].append(entry);
}

bool AotStats::saveToDisk(const QString &filepath) const
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qDebug().noquote() << u"Could not open \"%1\""_s.arg(filepath);
        return false;
    }

    file.write(this->toJsonDocument().toJson(QJsonDocument::Indented));
    return true;
}

void QQmlJSAotCompilerStats::registerFile(const QString &filepath)
{
    QQmlJSAotCompilerStats::instance()->registerFile(s_moduleId, filepath);
}

void QQmlJSAotCompilerStats::addEntry(const QString &filepath, const QQmlJS::AotStatsEntry &entry)
{
    QQmlJSAotCompilerStats::instance()->addEntry(s_moduleId, filepath, entry);
}

} // namespace QQmlJS

QT_END_NAMESPACE
