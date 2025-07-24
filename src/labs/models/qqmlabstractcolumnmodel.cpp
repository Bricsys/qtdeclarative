// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlabstractcolumnmodel_p.h"

#include <QtCore/qloggingcategory.h>

#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(lcColumnModel, "qt.qml.columnmodel")

QQmlAbstractColumnModel::QQmlAbstractColumnModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QQmlListProperty<QQmlTableModelColumn> QQmlAbstractColumnModel::columns()
{
    return {this, nullptr,
             &QQmlAbstractColumnModel::columns_append,
             &QQmlAbstractColumnModel::columns_count,
             &QQmlAbstractColumnModel::columns_at,
             &QQmlAbstractColumnModel::columns_clear,
             &QQmlAbstractColumnModel::columns_replace,
             &QQmlAbstractColumnModel::columns_removeLast};
}

void QQmlAbstractColumnModel::columns_append(QQmlListProperty<QQmlTableModelColumn> *property,
                                   QQmlTableModelColumn *value)
{
    auto *model = static_cast<QQmlAbstractColumnModel *>(property->object);
    Q_ASSERT(value);
    Q_ASSERT(model);
    auto *column = qobject_cast<QQmlTableModelColumn *>(value);
    if (column)
        model->mColumns.append(column);
}

qsizetype QQmlAbstractColumnModel::columns_count(QQmlListProperty<QQmlTableModelColumn> *property)
{
    auto *model = static_cast<QQmlAbstractColumnModel*>(property->object);
    Q_ASSERT(model);
    return model->mColumns.size();
}

QQmlTableModelColumn *QQmlAbstractColumnModel::columns_at(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index)
{
    auto *model = static_cast<QQmlAbstractColumnModel*>(property->object);
    Q_ASSERT(model);
    return model->mColumns.at(index);
}

void QQmlAbstractColumnModel::columns_clear(QQmlListProperty<QQmlTableModelColumn> *property)
{
    auto *model = static_cast<QQmlAbstractColumnModel *>(property->object);
    Q_ASSERT(model);
    return model->mColumns.clear();
}

void QQmlAbstractColumnModel::columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index, QQmlTableModelColumn *value)
{
    auto *model = static_cast<QQmlAbstractColumnModel *>(property->object);
    Q_ASSERT(model);
    if (auto *column = qobject_cast<QQmlTableModelColumn *>(value))
        return model->mColumns.replace(index, column);
}

void QQmlAbstractColumnModel::columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property)
{
    auto *model = static_cast<QQmlAbstractColumnModel *>(property->object);
    Q_ASSERT(model);
    model->mColumns.removeLast();
}

QHash<int, QByteArray> QQmlAbstractColumnModel::roleNames() const
{
    return mRoleNames;
}

Qt::ItemFlags QQmlAbstractColumnModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

void QQmlAbstractColumnModel::classBegin()
{
}

void QQmlAbstractColumnModel::componentComplete()
{
    mComponentCompleted = true;

    mColumnCount = mColumns.size();
    if (mColumnCount > 0)
        emit columnCountChanged();

    setInitialRows();
}


QQmlAbstractColumnModel::ColumnRoleMetadata::ColumnRoleMetadata()
    = default;

QQmlAbstractColumnModel::ColumnRoleMetadata::ColumnRoleMetadata(
    ColumnRole role, QString name, int type, QString typeName) :
    columnRole(role),
    name(std::move(name)),
    type(type),
    typeName(std::move(typeName))
{
}

bool QQmlAbstractColumnModel::ColumnRoleMetadata::isValid() const
{
    return !name.isEmpty();
}

QQmlAbstractColumnModel::ColumnRoleMetadata QQmlAbstractColumnModel::fetchColumnRoleData(const QString &roleNameKey,
                                                                       QQmlTableModelColumn *tableModelColumn, int columnIndex) const
{
    const QVariant row = firstRow();
    ColumnRoleMetadata roleData;

    QJSValue columnRoleGetter = tableModelColumn->getterAtRole(roleNameKey);
    if (columnRoleGetter.isUndefined()) {
        // This role is not defined, which is fine; just skip it.
        return roleData;
    }

    if (columnRoleGetter.isString()) {
        // The role is set as a string, so we assume the row is a simple object.
        if (row.userType() != QMetaType::QVariantMap) {
            qmlWarning(this).quote() << "expected row for role "
                                     << roleNameKey << " of TableModelColumn at index "
                                     << columnIndex << " to be a simple object, but it's "
                                     << row.typeName() << " instead: " << row;
            return roleData;
        }
        const QString rolePropertyName = columnRoleGetter.toString();
        const QVariant roleProperty = row.toMap().value(rolePropertyName);

        roleData.columnRole = ColumnRole::StringRole;
        roleData.name = rolePropertyName;
        roleData.type = roleProperty.userType();
        roleData.typeName = QString::fromLatin1(roleProperty.typeName());
    } else if (columnRoleGetter.isCallable()) {
        // The role is provided via a function, which means the row is complex and
        // the user needs to provide the data for it.
        const auto modelIndex = index(0, columnIndex);
        const auto args = QJSValueList() << qmlEngine(this)->toScriptValue(modelIndex);
        const QVariant cellData = columnRoleGetter.call(args).toVariant();

        // We don't know the property name since it's provided through the function.
        // roleData.name = ???
        roleData.columnRole = ColumnRole::FunctionRole;
        roleData.type = cellData.userType();
        roleData.typeName = QString::fromLatin1(cellData.typeName());
    } else {
        // Invalid role.
        qmlWarning(this) << "TableModelColumn role for column at index "
                         << columnIndex << " must be either a string or a function; actual type is: "
                         << columnRoleGetter.toString();
    }

    return roleData;
}

void QQmlAbstractColumnModel::fetchColumnMetadata()
{
    qCDebug(lcColumnModel) << "gathering metadata for" << mColumnCount << "columns from first row:";

    static const auto supportedRoleNames = QQmlTableModelColumn::supportedRoleNames();

    // Since we support different data structures at the row level, we require that there
    // is a TableModelColumn for each column.
    // Collect and cache metadata for each column. This makes data lookup faster.
    for (int columnIndex = 0; columnIndex < mColumns.size(); ++columnIndex) {
        QQmlTableModelColumn *column = mColumns.at(columnIndex);
        qCDebug(lcColumnModel).nospace() << "- column " << columnIndex << ":";

        ColumnMetadata metaData;
        const auto builtInRoleKeys = supportedRoleNames.keys();
        for (const int builtInRoleKey : builtInRoleKeys) {
            const QString builtInRoleName = supportedRoleNames.value(builtInRoleKey);
            ColumnRoleMetadata roleData = fetchColumnRoleData(builtInRoleName, column, columnIndex);
            if (roleData.type == QMetaType::UnknownType) {
                // This built-in role was not specified in this column.
                continue;
            }

            qCDebug(lcColumnModel).nospace() << "  - added metadata for built-in role "
                                            << builtInRoleName << " at column index " << columnIndex
                                            << ": name=" << roleData.name << " typeName=" << roleData.typeName
                                            << " type=" << roleData.type;

            // This column now supports this specific built-in role.
            metaData.roles.insert(builtInRoleName, roleData);
            // Add it if it doesn't already exist.
            mRoleNames[builtInRoleKey] = builtInRoleName.toLatin1();
        }
        mColumnMetadata.insert(columnIndex, metaData);
    }
}

QT_END_NAMESPACE

#include "moc_qqmlabstractcolumnmodel_p.cpp"
