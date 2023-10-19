// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QQUICKTEXTDOCUMENT_H
#define QQUICKTEXTDOCUMENT_H

#include <QtGui/QTextDocument>
#include <QtQuick/QQuickItem>

QT_BEGIN_NAMESPACE

class QQuickTextDocumentPrivate;
class Q_QUICK_EXPORT QQuickTextDocument : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_ADDED_IN_VERSION(2, 0)

public:
    QQuickTextDocument(QQuickItem *parent);
    QTextDocument *textDocument() const;

private:
    Q_DISABLE_COPY(QQuickTextDocument)
    Q_DECLARE_PRIVATE(QQuickTextDocument)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickTextDocument)

#endif
