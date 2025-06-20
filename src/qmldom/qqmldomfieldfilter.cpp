// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomfieldfilter_p.h"
#include "qqmldompath_p.h"
#include "qqmldomitem_p.h"
#include "QtCore/qglobal.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

/*!
\internal
\class QQmljs::Dom::FieldFilter

\brief Class that represent a filter on DomItem, when dumping or comparing

DomItem can be duped or compared, but often one is interested only in a subset
of them, FieldFilter is a simple way to select a subset of them.
It uses two basic elements: the type of the object (internalKind) and the
name of fields.

A basic filter can be represented by <op><typeName>:<fieldName> or <op><fieldName>
where op is either + or - (if the matching elements should be added or removed)
Both typeName and fieldName can be the empty string (meaning any value matches).

Basic filters are ordered from the most specific to the least specific as follow:
type+field > type > field > empty.
When combining several filters the most specific always wins, so
-code,+ScriptExpression:code is the same as +ScriptExpression:code,-code and means
that normally the field code is not outputted but for a ScriptExpression DomItem
it is.

It is possible to get the string representation of the current filter with
FieldFilter::describeFieldsFilter(), and change the current filter with
FieldFilter::addFilter(), but after it one should call FieldFilter::setFiltred()
to ensure that the internal cache used to speed up comparisons is correct.
*/

QString FieldFilter::describeFieldsFilter() const
{
    QString fieldFilterStr;
    {
        auto it = m_fieldFilterRemove.begin();
        while (it != m_fieldFilterRemove.end()) {
            if (!fieldFilterStr.isEmpty())
                fieldFilterStr.append(u",");
            fieldFilterStr.append(QLatin1String("-%1:%2").arg(it.key(), it.value()));
            ++it;
        }
    }
    {
        auto it = m_fieldFilterAdd.begin();
        while (it != m_fieldFilterAdd.end()) {
            if (!fieldFilterStr.isEmpty())
                fieldFilterStr.append(u",");
            fieldFilterStr.append(QLatin1String("+%1:%2").arg(it.key(), it.value()));
            ++it;
        }
    }
    return fieldFilterStr;
}

bool FieldFilter::operator()(const DomItem &obj, const Path &p, const DomItem &i) const
{
    if (p)
        return this->operator()(obj, p.component(0), i);
    else
        return this->operator()(obj, PathEls::Empty(), i);
}

bool FieldFilter::operator()(const DomItem &base, const PathEls::PathComponent &c, const DomItem &obj) const
{
    DomType baseK = base.internalKind();
    if (c.kind() == Path::Kind::Field) {
        DomType objK = obj.internalKind();
        if (!m_filtredTypes.contains(baseK) && !m_filtredTypes.contains(objK)
            && !m_filtredFields.contains(qHash(c.stringView())))
            return m_filtredDefault;
        QString typeStr = domTypeToString(baseK);
        QList<QString> tVals = m_fieldFilterRemove.values(typeStr);
        QString name = c.name();
        if (tVals.contains(name))
            return false;
        if (tVals.contains(QString())
            || m_fieldFilterRemove.values(domTypeToString(objK)).contains(QString())
            || m_fieldFilterRemove.values(QString()).contains(name)) {
            return m_fieldFilterAdd.values(typeStr).contains(name);
        }
    } else if (m_filtredTypes.contains(baseK)) {
        QString typeStr = domTypeToString(baseK);
        QList<QString> tVals = m_fieldFilterRemove.values(typeStr);
        return !tVals.contains(QString());
    }
    return true;
}

bool FieldFilter::addFilter(const QString &fFields)
{
    // parses a base filter of the form <op><typeName>:<fieldName> or <op><fieldName>
    // as described in this class documentation
    QRegularExpression fieldRe(QRegularExpression::anchoredPattern(QStringLiteral(
            uR"((?<op>[-+])?(?:(?<type>[a-zA-Z0-9_]*):)?(?<field>[a-zA-Z0-9_]*))")));
    for (const QString &fField : fFields.split(QLatin1Char(','))) {
        QRegularExpressionMatch m = fieldRe.matchView(fField);
        if (m.hasMatch()) {
            if (m.capturedView(u"op") == u"+") {
                m_fieldFilterRemove.remove(m.captured(u"type"), m.captured(u"field"));
                m_fieldFilterAdd.insert(m.captured(u"type"), m.captured(u"field"));
            } else {
                m_fieldFilterRemove.insert(m.captured(u"type"), m.captured(u"field"));
                m_fieldFilterAdd.remove(m.captured(u"type"), m.captured(u"field"));
            }
        } else {
            qCWarning(domLog) << "could not extract filter from" << fField;
            return false;
        }
    }
    return true;
}

FieldFilter FieldFilter::noFilter()
{
    return FieldFilter{ {}, {} };
}

FieldFilter FieldFilter::defaultFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd { { QLatin1String("ScriptExpression"),
                                                   QLatin1String("code") } };
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), Fields::code.toString() },
        { QString(), Fields::postCode.toString() },
        { QString(), Fields::preCode.toString() },
        { QString(), Fields::importScope.toString() },
        { QString(), Fields::fileLocationsTree.toString() },
        { QString(), Fields::astComments.toString() },
        { QString(), Fields::comments.toString() },
        { QString(), Fields::exports.toString() },
        { QString(), Fields::propertyInfos.toString() },
        { QLatin1String("FileLocationsNode"), Fields::parent.toString() }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

QQmlJS::Dom::FieldFilter QQmlJS::Dom::FieldFilter::noLocationFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), QLatin1String("code") },
        { QString(), QLatin1String("propertyInfos") },
        { QString(), QLatin1String("fileLocationsTree") },
        { QString(), QLatin1String("location") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("ScriptExpression"), QLatin1String("preCode") },
        { QLatin1String("ScriptExpression"), QLatin1String("postCode") },
        { QLatin1String("FileLocationsNode"), QLatin1String("parent") },
        { QLatin1String("Reference"), QLatin1String("get") },
        { QLatin1String("QmlComponent"), QLatin1String("ids") },
        { QLatin1String("QmlObject"), QLatin1String("prototypes") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

FieldFilter FieldFilter::compareFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), QLatin1String("propertyInfos") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("FileLocationsInfo"), QLatin1String("regions") },
        { QLatin1String("FileLocationsNode"), QLatin1String("parent") },
        { QLatin1String("QmlComponent"), QLatin1String("ids") },
        { QLatin1String("QmlObject"), QLatin1String("prototypes") },
        { QLatin1String("Reference"), QLatin1String("get") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

FieldFilter FieldFilter::compareNoCommentsFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd {};
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), QLatin1String("propertyInfos") },
        { QLatin1String("FileLocationsInfo"), QLatin1String("regions") },
        { QLatin1String("Reference"), QLatin1String("get") },
        { QLatin1String("QmlComponent"), QLatin1String("ids") },
        { QLatin1String("QmlObject"), QLatin1String("prototypes") },
        { QLatin1String(), QLatin1String("code") },
        { QLatin1String("ScriptExpression"), QLatin1String("localOffset") },
        { QLatin1String("FileLocationsNode"), QLatin1String("parent") },
        { QString(), QLatin1String("fileLocationsTree") },
        { QString(), QLatin1String("preCode") },
        { QString(), QLatin1String("postCode") },
        { QString(), QLatin1String("comments") },
        { QString(), QLatin1String("astComments") },
        { QString(), QLatin1String("location") }
    };
    return FieldFilter { fieldFilterAdd, fieldFilterRemove };
}

void FieldFilter::setFiltred()
{
    QSet<QString> filtredFieldStrs;
    QSet<QString> filtredTypeStrs;
    static QHash<QString, DomType> fieldToId = []() {
        QHash<QString, DomType> res;
        auto reverseMap = domTypeToStringMap();
        auto it = reverseMap.cbegin();
        auto end = reverseMap.cend();
        while (it != end) {
            res[it.value()] = it.key();
            ++it;
        }
        return res;
    }();
    auto addFilteredOfMap = [&](const QMultiMap<QString, QString> &map) {
        auto it = map.cbegin();
        auto end = map.cend();
        while (it != end) {
            filtredTypeStrs.insert(it.key());
            ++it;
        }
        const auto &fieldKeys = map.values(QString());
        for (const auto &f : fieldKeys)
            filtredFieldStrs.insert(f);
    };
    addFilteredOfMap(m_fieldFilterAdd);
    addFilteredOfMap(m_fieldFilterRemove);
    m_filtredDefault = true;
    if (m_fieldFilterRemove.values(QString()).contains(QString()))
        m_filtredDefault = false;
    m_filtredFields.clear();
    for (const auto &s : filtredFieldStrs)
        if (!s.isEmpty())
            m_filtredFields.insert(qHash(QStringView(s)));
    m_filtredTypes.clear();
    for (const auto &s : filtredTypeStrs) {
        if (s.isEmpty())
            continue;
        if (fieldToId.contains(s)) {
            m_filtredTypes.insert(fieldToId.value(s));
        } else {
            qCWarning(domLog) << "Filter on unknown type " << s << " will be ignored";
        }
    }
}

} // end namespace Dom
} // end namespace QQmlJS

QT_END_NAMESPACE

#include "moc_qqmldomfieldfilter_p.cpp"
