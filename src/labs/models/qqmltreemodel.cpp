// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltreemodel_p.h"
#include "qqmltreerow_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qjsonobject.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const QString ROWS_PROPERTY_NAME = u"rows"_s;

/*!
    \qmltype TreeModel
//!    \nativetype QQmlTreeModel
    \inqmlmodule Qt.labs.qmlmodels
    \brief Encapsulates a simple tree model.
    \since 6.10

    The TreeModel type stores JavaScript/JSON objects as data for a tree
    model that can be used with \l TreeView. It is intended to support
    very simple models without requiring the creation of a custom
    \l QAbstractItemModel subclass in C++.

    \snippet qml/treemodel/treemodel-filesystem-basic.qml file

    The model's initial data is set with either the \l rows property or by
    calling \l appendRow(). Each column in the model is specified by declaring
    a \l TableModelColumn instance, where the order of each instance determines
    its column index. Once the model's \l Component::completed() signal has been
    emitted, the columns and roles will have been established and are then
    fixed for the lifetime of the model.

    \section1 Supported Row Data Structures

    Each row represents a node in the tree. Each node has the same type of
    columns. The TreeModel is designed to work with JavaScript/JSON data so
    each row is a list of simple key-value pairs:

    \snippet qml/treemodel/treemodel-filesystem-basic.qml rows

    A node can have child nodes and these will be stored in an array
    associated with the "rows" key. "rows" is reserved for this purpose: only
    the list of child nodes should be associated with this key.

    The model is manipulated via \l {QModelIndex} {QModelIndices}. To access
    a specific row/node, the \l getRow() function can be used. It's also
    possible to access the model's JavaScript data directly via the \l rows
    property, but it is not possible to modify the model data this way.

    To add new rows, use \l appendRow(). To modify existing rows, use
    \l setRow(), \l removeRow() and \l clear().
*/

QQmlTreeModel::QQmlTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{

}

QQmlTreeModel::~QQmlTreeModel() = default;

int QQmlTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return static_cast<int>(mRows.size());

    const auto *row = static_cast<const QQmlTreeRow *>(parent.internalPointer());
    return static_cast<int>(row->rowCount());
}

/*!
    \qmlproperty int TreeModel::columnCount
    \readonly

    This read-only property holds the number of columns in the model.

    The number of columns is fixed for the lifetime of the model
    after the \l rows property is set or \l appendRow() is called for the first
    time.
*/

int QQmlTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return mColumns.size();
}

/*!
    \qmlmethod QModelIndex TreeModel::index(int row, int column, object parent)

    Returns a \l QModelIndex object referencing the given \a row and \a column of
    a given \a parent which can be passed to the data() function to get the data
    from that cell, or to setData() to edit the contents of that cell.

    \sa {QModelIndex and related Classes in QML}, data()
*/
QModelIndex QQmlTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()){
        if (static_cast<size_t>(row) >= mRows.size())
            return {};

        return createIndex(row, column, mRows.at(row).get());
    }

    const auto *treeRow = static_cast<const QQmlTreeRow *>(parent.internalPointer());
    if (treeRow->rowCount() <= static_cast<size_t>(row))
        return {};

    return createIndex(row, column, treeRow->getRow(row));
}

/*!
    \qmlmethod QModelIndex TreeModel::index(list<int> treeIndex, int column)

    Returns a \l QModelIndex object referencing the given \a treeIndex and \a column,
    which can be passed to the data() function to get the data from that cell,
    or to setData() to edit the contents of that cell.

    The first parameter \a treeIndex represents a path of row numbers tracing from
    the root to the desired row and is used for navigation inside the tree.
    This is best explained through an example.

    \table

    \row \li \inlineimage treemodel.svg
    \li \list

    \li The root of the tree is special, as it can be referenced by an invalid
    \l QModelIndex.

    \li Node A is the first child of the root and the corresponding \a treeIndex is \c [0].

    \li Node B is the first child of node A. Since the \a treeIndex of A is \c [0]
    the \a treeIndex of B will be \c [0,0].

    \li Node C is the second child of A and its \a treeIndex is \c [0,1].

    \li Node D is the third child of A and its \a treeIndex is \c [0,2].

    \li Node E is the second child of the root and its \a treeIndex is \c [1].

    \li Node F is the third child of the root and its \a treeIndex is \c [2].

    \endlist

    \endtable

    With this overload it is possible to obtain a \l QModelIndex to a node without
    having a \l QModelIndex to its parent node.

    If no node is found by the list specified, an invalid model index is returned.
    Please note that an invalid model index is referencing the root of the node.

    \sa {QModelIndex and related Classes in QML}, data()
*/
QModelIndex QQmlTreeModel::index(const std::vector<int> &treeIndex, int column)
{
    QModelIndex mIndex;
    QQmlTreeRow *row = getPointerToTreeRow(mIndex, treeIndex);

    if (row)
        return createIndex(treeIndex.back(), column, row);

    qmlWarning(this) << "TreeModel::index: could not find any node at the specified index";
    return {};
}

QModelIndex QQmlTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    const auto *thisRow = static_cast<const QQmlTreeRow *>(index.internalPointer());
    const QQmlTreeRow *parentRow = thisRow->parent();

    if (!parentRow) // parent is root
        return {};

    const QQmlTreeRow *grandparentRow = parentRow->parent();

    if (!grandparentRow) {// grandparent is root, parent is in mRows
        for (size_t i = 0; i < mRows.size(); i++) {
            if (mRows[i].get() == parentRow)
                return createIndex(static_cast<int>(i), 0, parentRow);
        }
        Q_UNREACHABLE_RETURN(QModelIndex());
    }

    for (size_t i = 0; i < grandparentRow->rowCount(); i++) {
        if (grandparentRow->getRow(static_cast<int>(i)) == parentRow)
            return createIndex(static_cast<int>(i), 0, parentRow);
    }
    Q_UNREACHABLE_RETURN(QModelIndex());
}

/*!
    \qmlmethod variant TreeModel::data(QModelIndex index, string role)

    Returns the data from the QQmlTreeModel at the given \a index belonging to the
    given \a role.

    \sa index(), setData()
*/

QVariant QQmlTreeModel::data(const QModelIndex &index, const QString &role) const
{
    const int iRole = mRoleNames.key(role.toUtf8(), -1);
    if (iRole >= 0)
        return data(index, iRole);
    return {};
}

QVariant QQmlTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        qmlWarning(this) << "data(): invalid QModelIndex";
        return {};
    }

    const int row = index.row();
    if (row < 0 || row >= rowCount(parent(index))) {
        qmlWarning(this) << "data(): invalid row specified in QModelIndex";
        return {};
    }

    const int column = index.column();
    if (column < 0 || column >= columnCount(parent(index))) {
        qmlWarning(this) << "data(): invalid column specified in QModelIndex";
        return {};
    }

    const ColumnMetadata columnMetadata = mColumnMetadata.at(column);
    const QString roleName = QString::fromUtf8(mRoleNames.value(role));
    if (!columnMetadata.roles.contains(roleName)) {
        qmlWarning(this) << "data(): no role named " << roleName
                         << " at column index " << column << ". The available roles for that column are: "
                         << columnMetadata.roles.keys();
        return {};
    }

    const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
    if (roleData.columnRole == ColumnRole::stringRole) {
        // We know the data structure, so we can get the data for the user.
        const QString propertyName = columnMetadata.roles.value(roleName).name;
        const auto *thisRow = static_cast<const QQmlTreeRow *>(index.internalPointer());
        return thisRow->data(propertyName);
    }

    // We don't know the data structure, so the user has to modify their data themselves.
    // First, find the getter for this column and role.
    QJSValue getter = mColumns.at(column)->getterAtRole(roleName);

    // Then, call it and return what it returned.
    const auto args = QJSValueList() << qmlEngine(this)->toScriptValue(index);
    return getter.call(args).toVariant();
}

/*!
    \qmlproperty object TreeModel::rows

    This property holds the model data in the form of an array of rows.

    \sa getRow(), setRow(), appendRow(), clear(), columnCount
*/
QVariant QQmlTreeModel::rows() const
{
    QVariantList rowsAsVariant;
    for (const auto &row : mRows)
        rowsAsVariant.append(row->toVariant());

    return rowsAsVariant;
}

void QQmlTreeModel::setRows(const QVariant &rows)
{
    if (rows.userType() != qMetaTypeId<QJSValue>()) {
        qWarning() << "setRows(): \"rows\" must be an array; actual type is " << rows.typeName();
        return;
    }

    const auto rowsAsJSValue = rows.value<QJSValue>();
    const QVariantList rowsAsVariantList = rowsAsJSValue.toVariant().toList();

    if (!mComponentCompleted) {
        mInitialRows = rowsAsVariantList;
        return;
    }

    setRowsPrivate(rowsAsVariantList);
}

// TODO: Turn this into a snippet that compiles in CI
/*!
    \qmlmethod TreeModel::appendRow(QModelIndex treeRowIndex, object treeRow)

    Appends a new treeRow to the treeRow specified by \a treeRowIndex, with the
    values (cells) in \a treeRow.

    \code
        treeModel.appendRow(treeRowIndex, {
                            checked: false,
                            size: "-"
                            type: folder,
                            name: "Orders",
                            lastModified: "2025-07-02",
                            rows: [
                                {
                                    checked: true,
                                    size: "38 KB",
                                    type: "file",
                                    name: "monitors.xlsx",
                                    lastModified: "2025-07-02",
                                },
                                {
                                    checked: true,
                                    size: "54 KB",
                                    type: "file",
                                    name: "notebooks.xlsx",
                                    lastModified: "2025-07-02",
                                }
                            ]
                        })
    \endcode

    If \a treeRowIndex is invalid, \a treeRow gets appended to the root node.

    \sa setRow(), removeRow()
*/

void QQmlTreeModel::appendRow(QModelIndex index, const QVariant &row)
{
    if (!validateRow("TreeModel::appendRow"_L1, row))
        return;

    const QVariant data = row.userType() == QMetaType::QVariantMap ? row : row.value<QJSValue>().toVariant();

    if (index.isValid()) {
        auto *parent = static_cast<QQmlTreeRow *>(index.internalPointer());
        auto *newChild = new QQmlTreeRow(data);

        beginInsertRows(index, static_cast<int>(parent->rowCount()), static_cast<int>(parent->rowCount()));
        parent->addChild(newChild);

        // Gather metadata the first time a row is added.
        if (mColumnMetadata.isEmpty())
            fetchColumnMetaData();

        endInsertRows();
    } else {
        qmlWarning(this) << "append: could not find any node at the specified index"
                         << " - the new row will be appended to root";

        beginInsertRows(QModelIndex(),
                static_cast<int>(mRows.size()),
                static_cast<int>(mRows.size()));

        mRows.push_back(std::make_unique<QQmlTreeRow>(data));

        // Gather metadata the first time a row is added.
        if (mColumnMetadata.isEmpty())
            fetchColumnMetaData();

        endInsertRows();
    }

    emit rowsChanged();
}

/*!
    \qmlmethod TreeModel::appendRow(object treeRow)

    Appends \a treeRow to the root node.

    \sa setRow(), removeRow()
*/

void QQmlTreeModel::appendRow(const QVariant &row)
{
    appendRow({}, row);
}

QQmlTreeRow *QQmlTreeModel::getPointerToTreeRow(QModelIndex &modIndex,
                                                const std::vector<int> &rowIndex) const
{
    for (int r : rowIndex) {
        modIndex = index(r, 0, modIndex);
        if (!modIndex.isValid())
            return nullptr;
    }

    return static_cast<QQmlTreeRow*>(modIndex.internalPointer());
}

/*!
    \qmlmethod object TreeModel::getRow(const QModelIndex &rowIndex)

    Returns the treeRow at \a rowIndex in the model.

    \note the returned object cannot be used to modify the contents of the
    model; use setTreeRow() instead.

    \sa setRow(), appendRow(), removeRow()
*/

QVariant QQmlTreeModel::getRow(const QModelIndex &rowIndex) const
{
    if (rowIndex.isValid())
        return static_cast<QQmlTreeRow*>(rowIndex.internalPointer())->toVariant();

    qmlWarning(this) << "getRow: could not find any node at the specified index";
    return {};
}

// TODO: Turn this into a snippet that compiles in CI
/*!
    \qmlmethod TreeModel::setRow(QModelIndex rowIndex, object treeRow)

    Replaces the TreeRow at \a rowIndex in the model with \a treeRow.
    A row with child rows will be rejected.

    All columns/cells must be present in \c treeRow, and in the correct order.
    The child rows of the row remain unaffected.

    \code
        treeModel.setRow(rowIndex, {
                        checked: true,
                        size: -,
                        type: "folder",
                        name: "Subtitles",
                        lastModified: "2025-07-07",
                        iconCcolor: "blue",
                    })
    \endcode

    \sa appendRow()
*/
void QQmlTreeModel::setRow(QModelIndex rowIndex, const QVariant &rowData)
{
    if (!rowIndex.isValid()) {
        qmlWarning(this) << "TreeModel::setRow: invalid modelIndex";
        return;
    }

    const QVariantMap rowAsMap = rowData.toMap();
    if (rowAsMap.contains(ROWS_PROPERTY_NAME) && rowAsMap[ROWS_PROPERTY_NAME].userType() == QMetaType::Type::QVariantList) {
        qmlWarning(this) << "TreeModel::setRow: child rows are not allowed";
        return;
    }

    if (!validateRow("TreeModel::setRow"_L1, rowData))
        return;

    const QVariant rowAsVariant = rowData.userType() == QMetaType::QVariantMap ? rowData : rowData.value<QJSValue>().toVariant();
    auto *row = static_cast<QQmlTreeRow *>(rowIndex.internalPointer());
    row->setData(rowAsVariant);

    const QModelIndex topLeftModelIndex(createIndex(rowIndex.row(), 0, rowIndex.internalPointer()));
    const QModelIndex bottomRightModelIndex(createIndex(rowIndex.row(), mColumnCount-1, rowIndex.internalPointer()));

    emit dataChanged(topLeftModelIndex, bottomRightModelIndex);
    emit rowsChanged();
}

/*!
    \qmlmethod TreeModel::clear()

    Removes all TreeRows from the model.

    \sa removeRow()
*/

void QQmlTreeModel::clear()
{
    QQmlEngine *engine = qmlEngine(this);
    Q_ASSERT(engine);
    setRows(QVariant::fromValue(engine->newArray()));
}

/*!
    \qmlmethod TreeModel::removeRow(QModelIndex rowIndex)

    Removes the TreeRow referenced by \a rowIndex from the model.

    \code
        treeModel.removeTreeRow(rowIndex)
    \endcode

    \sa clear()
*/

void QQmlTreeModel::removeRow(QModelIndex rowIndex)
{
    if (rowIndex.isValid()) {
        QModelIndex mIndexParent = rowIndex.parent();

        beginRemoveRows(mIndexParent, rowIndex.row(), rowIndex.row());

        if (mIndexParent.isValid()) {
            auto *parent = static_cast<QQmlTreeRow *>(mIndexParent.internalPointer());
            parent->removeChildAt(rowIndex.row());
        } else {
            mRows.erase(std::next(mRows.begin(), rowIndex.row()));
        }

        endRemoveRows();
    } else {
        qmlWarning(this) << "TreeModel::removeRow could not find any node at the specified index";
        return;
    }

    emit rowsChanged();
}

/*!
    \qmlmethod bool TreeModel::setData(QModelIndex index, string role, variant value)

    Inserts or updates the data field named by \a role in the TreeRow at the
    given \a index with \a value. Returns true if sucessful, false if not.

    \sa data(), index()
*/

bool QQmlTreeModel::setData(const QModelIndex &index, const QString &role, const QVariant &value)
{
    const int intRole = mRoleNames.key(role.toUtf8(), -1);
    if (intRole >= 0)
        return setData(index, value, intRole);
    return false;
}

bool QQmlTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_ASSERT(index.isValid());

    const int row = index.row();
    if (row < 0 || row >= rowCount(parent(index)))
        return false;

    const int column = index.column();
    if (column < 0 || column >= columnCount(parent(index)))
        return false;

    const QString roleName = QString::fromUtf8(mRoleNames.value(role));

    // Verify that the role exists for this column.
    const ColumnMetadata columnMetadata = mColumnMetadata.at(index.column());
    if (!columnMetadata.roles.contains(roleName)) {
        qmlWarning(this) << "setData(): no role named \"" << roleName
                         << "\" at column index " << column << ". The available roles for that column are: "
                         << columnMetadata.roles.keys();
        return false;
    }

    // Verify that the type of the value is what we expect.
    // If the value set is not of the expected type, we can try to convert it automatically.
    const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
    QVariant effectiveValue = value;
    if (value.userType() != roleData.type) {
        if (!value.canConvert(QMetaType(roleData.type))) {
            qmlWarning(this).nospace() << "setData(): the value " << value
                                       << " set at row " << row << " column " << column << " with role " << roleName
                                       << " cannot be converted to " << roleData.typeName;
            return false;
        }

        if (!effectiveValue.convert(QMetaType(roleData.type))) {
            qmlWarning(this).nospace() << "setData(): failed converting value " << value
                                       << " set at row " << row << " column " << column << " with role " << roleName
                                       << " to " << roleData.typeName;
            return false;
        }
    }

    if (roleData.columnRole == ColumnRole::stringRole) {
        // We know the data structure, so we can set it for the user.
        auto *row = static_cast<QQmlTreeRow *>(index.internalPointer());
        row->setField(roleData.name, value);
    } else {
        qmlWarning(this).nospace() << "setData(): manipulation of complex row "
                << "structures is not supported";
            return false;
    }

    QVector<int> rolesChanged;
    rolesChanged.append(role);
    emit dataChanged(index, index, rolesChanged);
    emit rowsChanged();

    return true;
}

QHash<int, QByteArray> QQmlTreeModel::roleNames() const
{
    return mRoleNames;
}

Qt::ItemFlags QQmlTreeModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QQmlListProperty<QQmlTableModelColumn> QQmlTreeModel::columns()
{
    return {this, nullptr,
            &QQmlTreeModel::columns_append,
            &QQmlTreeModel::columns_count,
            &QQmlTreeModel::columns_at,
            &QQmlTreeModel::columns_clear,
            &QQmlTreeModel::columns_replace,
            &QQmlTreeModel::columns_removeLast};
}

void QQmlTreeModel::columns_append(QQmlListProperty<QQmlTableModelColumn> *property,
                                    QQmlTableModelColumn *value)
{
    auto *model = static_cast<QQmlTreeModel *>(property->object);
    Q_ASSERT(value);
    Q_ASSERT(model);
    auto *column = qobject_cast<QQmlTableModelColumn *>(value);
    if (column)
        model->mColumns.append(column);
}

qsizetype QQmlTreeModel::columns_count(QQmlListProperty<QQmlTableModelColumn> *property)
{
    const QQmlTreeModel *model = static_cast<QQmlTreeModel*>(property->object);
    Q_ASSERT(model);
    return model->mColumns.size();
}

QQmlTableModelColumn *QQmlTreeModel::columns_at(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index)
{
    const QQmlTreeModel *model = static_cast<QQmlTreeModel*>(property->object);
    Q_ASSERT(model);
    return model->mColumns.at(index);
}

void QQmlTreeModel::columns_clear(QQmlListProperty<QQmlTableModelColumn> *property)
{
    auto *model = static_cast<QQmlTreeModel *>(property->object);
    Q_ASSERT(model);
    return model->mColumns.clear();
}

void QQmlTreeModel::columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index, QQmlTableModelColumn *value)
{
    auto *model = static_cast<QQmlTreeModel *>(property->object);
    Q_ASSERT(model);
    if (auto *column = qobject_cast<QQmlTableModelColumn *>(value))
        return model->mColumns.replace(index, column);
}

void QQmlTreeModel::columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property)
{
    auto *model = static_cast<QQmlTreeModel *>(property->object);
    Q_ASSERT(model);
    model->mColumns.removeLast();
}

QQmlTreeModel::ColumnRoleMetadata::ColumnRoleMetadata() = default;

QQmlTreeModel::ColumnRoleMetadata::ColumnRoleMetadata(
    ColumnRole role, QString name, int type, QString typeName) :
    columnRole(role),
    name(std::move(name)),
    type(type),
    typeName(std::move(typeName))
{
}

bool QQmlTreeModel::ColumnRoleMetadata::isValid() const
{
    return !name.isEmpty();
}

void QQmlTreeModel::classBegin()
{

}

void QQmlTreeModel::componentComplete()
{
    mComponentCompleted = true;
    mColumnCount = mColumns.size();

    if (mColumnCount > 0)
        emit columnCountChanged();

    setRowsPrivate(mInitialRows);
}

QQmlTreeModel::ColumnRoleMetadata QQmlTreeModel::fetchColumnRoleData(const QString &roleNameKey,
                      QQmlTableModelColumn *tableModelColumn, int columnIndex) const
{
    const QQmlTreeRow *firstRow = mRows.front().get();
    ColumnRoleMetadata roleData;

    QJSValue columnRoleGetter = tableModelColumn->getterAtRole(roleNameKey);
    if (columnRoleGetter.isUndefined()) {
        // This role is not defined, which is fine; just skip it.
        return roleData;
    }

    if (columnRoleGetter.isString()) {
        // The role is set as a string, so we assume the row is a simple object.
        const QString rolePropertyName = columnRoleGetter.toString();
        const QVariant roleProperty = firstRow->data(rolePropertyName);

        roleData.columnRole = ColumnRole::stringRole;
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
        roleData.columnRole = ColumnRole::functionRole;
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

void QQmlTreeModel::fetchColumnMetaData()
{
    static const auto supportedRoleNames = QQmlTableModelColumn::supportedRoleNames();

    for (int columnIndex = 0; columnIndex < mColumns.size(); ++columnIndex) {
        QQmlTableModelColumn *column = mColumns.at(columnIndex);

        ColumnMetadata metaData;
        const auto builtInRoleKeys = supportedRoleNames.keys();
        for (const int builtInRoleKey : builtInRoleKeys) {
            const QString builtInRoleName = supportedRoleNames.value(builtInRoleKey);
            ColumnRoleMetadata roleData = fetchColumnRoleData(builtInRoleName, column, columnIndex);
            if (roleData.type == QMetaType::UnknownType) {
                // This built-in role was not specified in this column.
                continue;
            }

            // This column now supports this specific built-in role.
            metaData.roles.insert(builtInRoleName, roleData);
            // Add it if it doesn't already exist.
            mRoleNames[builtInRoleKey] = builtInRoleName.toLatin1();
        }

        mColumnMetadata.insert(columnIndex, metaData);
    }
}

bool QQmlTreeModel::validateRowType(QLatin1StringView functionName, const QVariant &row)
{
    if (!row.canConvert<QJSValue>()) {
        qmlWarning(this) << functionName << ": expected \"row\" argument to be a QJSValue,"
                         << " but got " << row.typeName() << " instead:\n" << row;
        return false;
    }

    const auto rowAsJSValue = row.value<QJSValue>();
    if (!rowAsJSValue.isObject() && !rowAsJSValue.isArray()) {
        qmlWarning(this) << functionName << ": expected \"row\" argument "
                         << "to be an object or array, but got:\n" << rowAsJSValue.toString();
        return false;
    }

    return true;
}

bool QQmlTreeModel::validateRow(QLatin1StringView functionName, const QVariant &row, bool setRowsOperation)
{
    if (mColumnMetadata.isEmpty()) {
        // There is no column metadata, so we have nothing to validate the row against.
        // Rows have to be added before we can gather metadata from them, so just this
        // once we'll return true to allow the rows to be added.
        return true;
    }

    const bool isVariantMap = (row.userType() == QMetaType::QVariantMap);

    // Don't require each row to be a QJSValue when setting all rows,
    // as they won't be; they'll be QVariantMap.
    if (!setRowsOperation && (!isVariantMap && !validateRowType(functionName, row)))
        return false;

    const QVariant rowAsVariant = setRowsOperation || isVariantMap ? row : row.value<QJSValue>().toVariant();
    if (rowAsVariant.userType() != QMetaType::QVariantMap) {
        qmlWarning(this) << functionName << ": row manipulation functions do not support complex rows";
        return false;
    }

    const QVariantMap rowAsMap = rowAsVariant.toMap();
    const int columnCount = rowAsMap.size();
    if (columnCount < mColumnCount) {
        qmlWarning(this) << functionName << ": expected " << mColumnCount
                         << " columns, but only got " << columnCount;
        return false;
    }

    // We can't validate complex structures, but we can make sure that
    // each simple string-based role in each column is correct.
    for (int columnIndex = 0; columnIndex < mColumns.size(); ++columnIndex) {
        QQmlTableModelColumn *column = mColumns.at(columnIndex);
        const QHash<QString, QJSValue> getters = column->getters();
        const auto roleNames = getters.keys();
        const ColumnMetadata columnMetadata = mColumnMetadata.at(columnIndex);
        for (const QString &roleName : roleNames) {
            const ColumnRoleMetadata roleData = columnMetadata.roles.value(roleName);
            if (roleData.columnRole == ColumnRole::functionRole)
                continue;

            if (!rowAsMap.contains(roleData.name)) {
                qmlWarning(this).quote() << functionName << ": expected a property named "
                                         << roleData.name << " in row at index ";
                return false;
            }

            const QVariant rolePropertyValue = rowAsMap.value(roleData.name);

            if (rolePropertyValue.userType() != roleData.type) {
                if (!rolePropertyValue.canConvert(QMetaType(roleData.type))) {
                    qmlWarning(this).quote() << functionName << ": expected the property named "
                                           << roleData.name << " to be of type " << roleData.typeName
                                             << ", but got " << QString::fromLatin1(rolePropertyValue.typeName())
                                             << " instead";
                    return false;
                }

                QVariant effectiveValue = rolePropertyValue;
                if (!effectiveValue.convert(QMetaType(roleData.type))) {
                    qmlWarning(this).nospace() << functionName << ": failed converting value "
                                               << rolePropertyValue << " set at column " << columnIndex << " with role "
                                               << QString::fromLatin1(rolePropertyValue.typeName()) << " to "
                                               << roleData.typeName;
                    return false;
                }
            }
        }
    }

    if (rowAsMap.contains(ROWS_PROPERTY_NAME) && rowAsMap[ROWS_PROPERTY_NAME].userType() == QMetaType::Type::QVariantList)
    {
        const QList<QVariant> variantList = rowAsMap[ROWS_PROPERTY_NAME].toList();
        for (const QVariant &rowAsVariant : variantList)
            if (!validateRow(functionName, rowAsVariant))
                return false;
    }

    return true;
}

void QQmlTreeModel::setRowsPrivate(const QVariantList &rowsAsVariantList)
{
    Q_ASSERT(mComponentCompleted);

    // By now, all TableModelColumns should have been set.
    if (mColumns.isEmpty()) {
        qmlWarning(this) << "No TableModelColumns were set; model will be empty";
        return;
    }

    const bool firstTimeValidRowsHaveBeenSet = mColumnMetadata.isEmpty();
    if (!firstTimeValidRowsHaveBeenSet) {
        // This is not the first time the rows have been set; validate each one.
        for (const auto &row:rowsAsVariantList) {
            // validateNewRow() expects a QVariant wrapping a QJSValue, so to
            // simplify the code, just create one here.
            const QVariant wrappedRow = QVariant::fromValue(row);
            bool setRowsOperation = true;
            if (!validateRow("TreeModel::setRows"_L1, wrappedRow, setRowsOperation))
                return;
        }
    }

    beginResetModel();

    // We don't clear the column or role data, because a TreeModel should not be reused in that way.
    // Once it has valid data, its columns and roles are fixed.
    mRows.clear();

    for (const auto &rowAsVariant : rowsAsVariantList)
        mRows.push_back(std::make_unique<QQmlTreeRow>(rowAsVariant));

    // Gather metadata the first time the rows are set.
    if (firstTimeValidRowsHaveBeenSet && !mInitialRows.isEmpty())
        fetchColumnMetaData();
    // If we call setrows on an empty model, mInitialRows will be empty, but mRows is not
    else if (firstTimeValidRowsHaveBeenSet && !mRows.empty())
        fetchColumnMetaData();

    endResetModel();
    emit rowsChanged();
}

int QQmlTreeModel::treeSize() const
{
    int treeSize = 0;

    for (const auto &treeRow : mRows)
        treeSize += treeRow->subTreeSize();

    return treeSize;
}

QT_END_NAMESPACE

#include "moc_qqmltreemodel_p.cpp"
