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

#ifndef QQMLLISTACCESSOR_H
#define QQMLLISTACCESSOR_H

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

#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

class QQmlEngine;
class Q_AUTOTEST_EXPORT QQmlListAccessor
{
public:
    QQmlListAccessor();
    ~QQmlListAccessor();

    QVariant list() const;
    void setList(const QVariant &);

    bool isValid() const;

    qsizetype count() const;
    QVariant at(qsizetype) const;

    enum Type { Invalid, StringList, UrlList, VariantList, ObjectList, ListProperty, Instance, Integer };
    Type type() const { return m_type; }

private:
    Type m_type;
    QVariant d;
};

QT_END_NAMESPACE

#endif // QQMLLISTACCESSOR_H
