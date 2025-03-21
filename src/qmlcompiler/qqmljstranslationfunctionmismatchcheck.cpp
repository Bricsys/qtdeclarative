// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljstranslationfunctionmismatchcheck_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void QQmlJSTranslationFunctionMismatchCheck::onCall(const QQmlSA::Element &element,
                                                    const QString &propertyName,
                                                    const QQmlSA::Element &readScope,
                                                    QQmlSA::SourceLocation location)
{
    Q_UNUSED(readScope);

    const QQmlSA::Element globalJSObject = resolveBuiltinType(u"GlobalObject");
    if (element != globalJSObject)
        return;

    constexpr std::array translationFunctions = {
        "qsTranslate"_L1,
        "QT_TRANSLATE_NOOP"_L1,
        "qsTr"_L1,
        "QT_TR_NOOP"_L1,
    };

    constexpr std::array idTranslationFunctions = {
        "qsTrId"_L1,
        "QT_TRID_NOOP"_L1,
    };

    const bool isTranslation =
            std::find(translationFunctions.cbegin(), translationFunctions.cend(), propertyName)
            != translationFunctions.cend();
    const bool isIdTranslation =
            std::find(idTranslationFunctions.cbegin(), idTranslationFunctions.cend(), propertyName)
            != idTranslationFunctions.cend();

    if (!isTranslation && !isIdTranslation)
        return;

    const TranslationType current = isTranslation ? Normal : IdBased;

    if (m_lastTranslationFunction == None) {
        m_lastTranslationFunction = current;
        return;
    }

    if (m_lastTranslationFunction != current) {
        emitWarning("Do not mix translation functions", qmlTranslationFunctionMismatch, location);
    }
}

QT_END_NAMESPACE
