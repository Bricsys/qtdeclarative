// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSLOGGINGUTILS_P_H
#define QQMLJSLOGGINGUTILS_P_H

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

#include <private/qtqmlcompilerexports_p.h>

#include "qqmljsloggingutils.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

class LoggerCategoryPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJS::LoggerCategory);

public:
    LoggerWarningId id() const { return LoggerWarningId(m_name); }

    void setLevel(QtMsgType);
    void setIgnored(bool);

    QString name() const;
    QString settingsName() const;
    QString description() const;
    QtMsgType level() const;
    bool isIgnored() const;
    bool isDefault() const;
    bool hasChanged() const;

    static LoggerCategoryPrivate *get(LoggerCategory *);

    friend bool operator==(const LoggerCategoryPrivate &lhs, const LoggerCategoryPrivate &rhs)
    {
        return operatorEqualsImpl(lhs, rhs);
    }
    friend bool operator!=(const LoggerCategoryPrivate &lhs, const LoggerCategoryPrivate &rhs)
    {
        return !operatorEqualsImpl(lhs, rhs);
    }

    bool operator==(const LoggerWarningId warningId) const { return warningId.name() == m_name; }

private:
    static bool operatorEqualsImpl(const LoggerCategoryPrivate &, const LoggerCategoryPrivate &);

    QString m_name;
    QString m_settingsName;
    QString m_description;
    QtMsgType m_level = QtDebugMsg;
    bool m_ignored = false;
    bool m_isDefault = false; // Whether or not the category can be disabled
    bool m_changed = false;
};

} // namespace QQmlJS

QT_END_NAMESPACE

#endif // QQMLJSLOGGINGUTILS_P_H
