// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktextdocument.h"
#include "qquicktextdocument_p.h"

#include "qquicktextedit_p.h"
#include "qquicktextedit_p_p.h"
#include "qquicktext_p_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlcontext.h>
#include <QtQuick/private/qquickpixmap_p.h>

#include <QtCore/qmimedatabase.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \qmltype TextDocument
    \instantiates QQuickTextDocument
    \inqmlmodule QtQuick
    \brief A wrapper around TextEdit's backing QTextDocument.

    To load text into the document, set the \l source property. If the user then
    modifies the text and wants to save the same document, call \l save() to save
    it to the same source again (only if \l {QUrl::isLocalFile()}{it's a local file}).
    Or call \l saveAs() to save it to a different file.

    This class cannot be instantiated in QML, but is available from \l TextEdit::textDocument.

    \note All loading and saving is done synchronously for now.
    This may block the UI if the \l source is a slow network drive.
    This may be improved in future versions of Qt.

    \note This API is considered tech preview and may change in future versions of Qt.
*/

/*!
    \class QQuickTextDocument
    \since 5.1
    \brief The QQuickTextDocument class provides access to the QTextDocument of QQuickTextEdit.
    \inmodule QtQuick

    This class provides access to the QTextDocument of QQuickTextEdit elements.
    This is provided to allow usage of the \l{Rich Text Processing} functionalities of Qt,
    including document modifications. It can also be used to output content,
    for example with \l{QTextDocumentWriter}), or provide additional formatting,
    for example with \l{QSyntaxHighlighter}.
*/

/*!
   Constructs a QQuickTextDocument object with
   \a parent as the parent object.
*/
QQuickTextDocument::QQuickTextDocument(QQuickItem *parent)
    : QObject(*(new QQuickTextDocumentPrivate), parent)
{
    Q_D(QQuickTextDocument);
    Q_ASSERT(parent);
    d->editor = qobject_cast<QQuickTextEdit *>(parent);
    Q_ASSERT(d->editor);
    connect(textDocument(), &QTextDocument::modificationChanged,
            this, &QQuickTextDocument::modifiedChanged);
}

/*!
    \qmlproperty url QtQuick::TextDocument::source
    \since 6.7

    QQuickTextDocument can handle any text format supported by Qt, loaded from
    any URL scheme supported by Qt.

    The URL may be absolute, or relative to the URL of the component.

    The \c source property cannot be changed while the document's \l modified
    state is \c true. If the user has modified the document contents, you
    should prompt the user whether to \l save(), or else discard changes by
    setting \c {modified = false} before setting the \l source property to a
    different URL.

    \sa QTextDocumentWriter::supportedDocumentFormats()
*/
QUrl QQuickTextDocument::source() const
{
    Q_D(const QQuickTextDocument);
    return d->url;
}

void QQuickTextDocument::setSource(const QUrl &url)
{
    Q_D(QQuickTextDocument);

    if (url == d->url)
        return;

    if (isModified()) {
        qmlWarning(this) << "Existing document modified: you should save(),"
                            "or call TextEdit.clear() before setting a different source";
        return;
    }

    d->url = url;
    emit sourceChanged();
    d->load();
}

/*!
    \qmlproperty bool QtQuick::TextDocument::modified
    \since 6.7

    This property holds whether the document has been modified by the user
    since the last time it was loaded or saved. By default, this property is
    \c false.

    As with \l QTextDocument::modified, you can set the modified property:
    for example, set it to \c false to allow setting the \c source property
    to a different URL (thus discarding the user's changes).

    \sa QTextDocument::modified
*/
bool QQuickTextDocument::isModified() const
{
    const auto *doc = textDocument();
    return doc && doc->isModified();
}

void QQuickTextDocument::setModified(bool modified)
{
    if (auto *doc = textDocument())
        doc->setModified(modified);
}

void QQuickTextDocumentPrivate::load()
{
    Q_Q(QQuickTextDocument);
    const QQmlContext *context = qmlContext(editor);
    const QUrl &resolvedUrl = context ? context->resolvedUrl(url) : url;
    const QString fileName = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);
    if (QFile::exists(fileName)) {
        mimeType = QMimeDatabase().mimeTypeForFile(fileName);
        QFile file(fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QByteArray data = file.readAll();
            if (auto *doc = editor->document()) {
                doc->setBaseUrl(resolvedUrl.adjusted(QUrl::RemoveFilename));
                if (mimeType.inherits("text/markdown"_L1)) {
                    doc->setMarkdown(QString::fromUtf8(data));
                } else if (mimeType.inherits("text/html"_L1)) {
                    // If a user loads an HTML file, remember the encoding.
                    // If the user then calls save() later, the same encoding will be used.
                    encoding = QStringConverter::encodingForHtml(data);
                    if (encoding) {
                        QStringDecoder decoder(*encoding);
                        doc->setHtml(decoder(data));
                    } else {
                        // fall back to utf8
                        doc->setHtml(QString::fromUtf8(data));
                    }
                } else {
                    doc->setPlainText(QString::fromUtf8(data));
                }
                doc->setModified(false);
            }
        } else {
            emit q->error(QQuickTextDocument::tr("Cannot load: %1").arg(file.errorString()));
        }
    }
}

void QQuickTextDocumentPrivate::writeTo(const QUrl &fileUrl)
{
    Q_Q(QQuickTextDocument);
    auto *doc = editor->document();
    if (!doc)
        return;

    const QString filePath = fileUrl.toLocalFile();
    const bool sameUrl = fileUrl == url;
    const auto type = (sameUrl ? mimeType : QMimeDatabase().mimeTypeForUrl(fileUrl));
    const bool isHtml = type.inherits("text/html"_L1);
    QFile file(filePath);
    if (!file.open(QFile::WriteOnly | QFile::Truncate | (isHtml ? QFile::NotOpen : QFile::Text))) {
        emit q->error(QQuickTextDocument::tr("Cannot save: %1").arg(file.errorString()));
        return;
    }
    QByteArray raw;
    if (type.inherits("text/markdown"_L1)) {
        raw = doc->toMarkdown().toUtf8();
    } else if (isHtml) {
        if (sameUrl && encoding) {
            QStringEncoder enc(*encoding);
            raw = enc.encode(doc->toHtml());
        } else {
            // default to UTF-8 unless the user is saving the same file as previously loaded
            raw = doc->toHtml().toUtf8();
        }
    } else {
        raw = doc->toPlainText().toUtf8();
    }

    file.write(raw);
    file.close();
    doc->setModified(false);
}

QTextDocument *QQuickTextDocumentPrivate::document() const
{
    return editor->document();
}

void QQuickTextDocumentPrivate::setDocument(QTextDocument *doc)
{
    Q_Q(QQuickTextDocument);
    QTextDocument *oldDoc = editor->document();
    if (doc == oldDoc)
        return;

    if (oldDoc)
        oldDoc->disconnect(q);
    if (doc) {
        q->connect(doc, &QTextDocument::modificationChanged,
                   q, &QQuickTextDocument::modifiedChanged);
    }
    editor->setDocument(doc);
    emit q->textDocumentChanged();
}

/*!
   Returns a pointer to the QTextDocument object.
*/
QTextDocument *QQuickTextDocument::textDocument() const
{
    Q_D(const QQuickTextDocument);
    return d->document();
}

/*!
    \brief Sets the given \a document.
    \since 6.7

    The caller retains ownership of the document.
*/
void QQuickTextDocument::setTextDocument(QTextDocument *document)
{
    d_func()->setDocument(document);
}

/*!
    \fn void QQuickTextDocument::textDocumentChanged()
    \since 6.7

    This signal is emitted when the underlying QTextDocument is
    replaced with a different instance.

    \sa setTextDocument()
*/

/*!
    \qmlmethod void QtQuick::TextDocument::save()
    \brief Saves the contents to the same file and format specified by \l source.
    \since 6.7

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, saveAs()
*/
void QQuickTextDocument::save()
{
    Q_D(QQuickTextDocument);
    d->writeTo(d->url);
}

/*!
    \qmlmethod void QtQuick::TextDocument::saveAs(url url)
    \brief Saves the contents to the file and format specified by \a url.
    \since 6.7

    The file extension in \a url specifies the file format
    (as determined by QMimeDatabase::mimeTypeForUrl()).

    \note You can save only to a \l {QUrl::isLocalFile()}{file on a mounted filesystem}.

    \sa source, saveAs()
*/
void QQuickTextDocument::saveAs(const QUrl &url)
{
    Q_D(QQuickTextDocument);
    if (!url.isLocalFile()) {
        emit error(tr("Can only save to local files"));
        return;
    }
    d->writeTo(url);

    if (url == d->url)
        return;

    d->url = url;
    emit sourceChanged();
}

QQuickTextImageHandler::QQuickTextImageHandler(QObject *parent)
    : QObject(parent)
{
}

QSizeF QQuickTextImageHandler::intrinsicSize(
        QTextDocument *doc, int, const QTextFormat &format)
{
    if (format.isImageFormat()) {
        QTextImageFormat imageFormat = format.toImageFormat();
        const int width = qRound(imageFormat.width());
        const bool hasWidth = imageFormat.hasProperty(QTextFormat::ImageWidth) && width > 0;
        const int height = qRound(imageFormat.height());
        const bool hasHeight = imageFormat.hasProperty(QTextFormat::ImageHeight) && height > 0;
        QSizeF size(width, height);
        if (!hasWidth || !hasHeight) {
            QVariant res = doc->resource(QTextDocument::ImageResource, QUrl(imageFormat.name()));
            QImage image = res.value<QImage>();
            if (image.isNull()) {
                // autotests expect us to reserve a 16x16 space for a "broken image" icon,
                // even though we don't actually display one
                if (!hasWidth)
                    size.setWidth(16);
                if (!hasHeight)
                    size.setHeight(16);
                return size;
            }
            QSize imgSize = image.size();
            if (!hasWidth) {
                if (!hasHeight)
                    size.setWidth(imgSize.width());
                else
                    size.setWidth(qRound(height * (imgSize.width() / (qreal) imgSize.height())));
            }
            if (!hasHeight) {
                if (!hasWidth)
                    size.setHeight(imgSize.height());
                else
                    size.setHeight(qRound(width * (imgSize.height() / (qreal) imgSize.width())));
            }
        }
        return size;
    }
    return QSizeF();
}

/*!
    \qmlsignal QtQuick::TextDocument::error(string message)

    This signal is emitted when an error \a message (translated string) should
    be presented to the user, for example with a MessageDialog.
*/

QT_END_NAMESPACE

#include "moc_qquicktextdocument.cpp"
#include "moc_qquicktextdocument_p.cpp"
