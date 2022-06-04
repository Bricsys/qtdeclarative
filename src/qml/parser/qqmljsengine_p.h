/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QQMLJSENGINE_P_H
#define QQMLJSENGINE_P_H

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

#include "qqmljsglobal_p.h"
#include <private/qqmljssourcelocation_p.h>

#include <private/qqmljsmemorypool_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {

class Lexer;
class MemoryPool;

class QML_PARSER_EXPORT Directives {
public:
    virtual ~Directives() {}

    virtual void pragmaLibrary()
    {
    }

    virtual void importFile(const QString &jsfile, const QString &module, int line, int column)
    {
        Q_UNUSED(jsfile);
        Q_UNUSED(module);
        Q_UNUSED(line);
        Q_UNUSED(column);
    }

    virtual void importModule(const QString &uri, const QString &version, const QString &module, int line, int column)
    {
        Q_UNUSED(uri);
        Q_UNUSED(version);
        Q_UNUSED(module);
        Q_UNUSED(line);
        Q_UNUSED(column);
    }
};

class QML_PARSER_EXPORT Engine
{
    Lexer *_lexer;
    Directives *_directives;
    MemoryPool _pool;
    QList<SourceLocation> _comments;
    QStringList _extraCode;
    QString _code;

public:
    Engine();
    ~Engine();

    void setCode(const QString &code);
    const QString &code() const { return _code; }

    void addComment(int pos, int len, int line, int col);
    QList<SourceLocation> comments() const;

    Lexer *lexer() const;
    void setLexer(Lexer *lexer);

    Directives *directives() const;
    void setDirectives(Directives *directives);

    MemoryPool *pool();

    inline QStringView midRef(int position, int size) { return QStringView{_code}.mid(position, size); }

    QStringView newStringRef(const QString &s);
    QStringView newStringRef(const QChar *chars, int size);
};

double integerFromString(const char *buf, int size, int radix);

} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSENGINE_P_H
