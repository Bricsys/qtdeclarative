// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTREEMODEL_P_H
#define QQMLTREEMODEL_P_H

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

#include "qqmltablemodelcolumn_p.h"

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qvariant.h>

#include <memory>
#include <vector>

QT_REQUIRE_CONFIG(qml_tree_model);

class tst_QQmlTreeModel;

QT_BEGIN_NAMESPACE

class QQmlTreeRow;



class Q_LABSQMLMODELS_EXPORT QQmlTreeModel : public QAbstractItemModel, public QQmlParserStatus
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TreeModel)
    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged FINAL)
    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QQmlTableModelColumn> columns READ columns CONSTANT FINAL)
    Q_INTERFACES(QQmlParserStatus)
    Q_CLASSINFO("DefaultProperty", "columns")
    QML_ADDED_IN_VERSION(6, 10)

public:
    Q_DISABLE_COPY_MOVE(QQmlTreeModel)

    explicit QQmlTreeModel(QObject *parent = nullptr);
    ~QQmlTreeModel() override;

    QVariant rows() const;
    void setRows(const QVariant &rows);

    Q_INVOKABLE void appendRow(QModelIndex parent, const QVariant &row);
    Q_INVOKABLE void appendRow(const QVariant &row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariant getRow(const QModelIndex &index) const;
    Q_INVOKABLE void removeRow(QModelIndex index);
    Q_INVOKABLE void setRow(QModelIndex index, const QVariant &rowData);

    Q_INVOKABLE QModelIndex index(const std::vector<int> &rowIndex, int column);

    QQmlListProperty<QQmlTableModelColumn> columns();

    static void columns_append(QQmlListProperty<QQmlTableModelColumn> *property, QQmlTableModelColumn *value);
    static qsizetype columns_count(QQmlListProperty<QQmlTableModelColumn> *property);
    static QQmlTableModelColumn *columns_at(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index);
    static void columns_clear(QQmlListProperty<QQmlTableModelColumn> *property);
    static void columns_replace(QQmlListProperty<QQmlTableModelColumn> *property, qsizetype index, QQmlTableModelColumn *value);
    static void columns_removeLast(QQmlListProperty<QQmlTableModelColumn> *property);

    //AbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, const QString &role) const;
    QVariant data(const QModelIndex &index, int role) const override;
    Q_INVOKABLE bool setData(const QModelIndex &index, const QString &role, const QVariant &value);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QModelIndex parent(const QModelIndex &index) const override;

signals:
    void columnCountChanged();
    void rowsChanged();

protected:
    void classBegin() override;
    void componentComplete() override;

private:
    QQmlTreeRow *getPointerToTreeRow(QModelIndex &index, const std::vector<int> &rowIndex) const;

    int treeSize() const;
    friend class ::tst_QQmlTreeModel;

    enum class ColumnRole : quint8
    {
        stringRole,
        functionRole
    };

    class ColumnRoleMetadata
    {
    public:
        ColumnRoleMetadata();
        ColumnRoleMetadata(ColumnRole role, QString name, int type, QString typeName);

        bool isValid() const;

        ColumnRole columnRole = ColumnRole::functionRole;
        QString name;
        int type = QMetaType::UnknownType;
        QString typeName;
    };

    struct ColumnMetadata
    {
        // Key = role name that will be made visible to the delegate
        // Value = metadata about that role, including actual name in the model data, type, etc.
        QHash<QString, ColumnRoleMetadata> roles;
    };


    void setRowsPrivate(const QVariantList &rowsAsVariantList);
    ColumnRoleMetadata fetchColumnRoleData(const QString &roleNameKey, QQmlTableModelColumn *tableModelColumn, int columnIndex) const;
    void fetchColumnMetaData();

    bool validateRowType(QLatin1StringView functionName, const QVariant &row);
    bool validateRow(QLatin1StringView functionName, const QVariant &row, bool setRowsOperation = false);

    QList<QQmlTableModelColumn *> mColumns;
    std::vector<std::unique_ptr<QQmlTreeRow>> mRows;

    bool mComponentCompleted = false;
    int mColumnCount = 0;
    // Each entry contains information about the properties of the column at that index.
    QVector<ColumnMetadata> mColumnMetadata;
    // key = property index (0 to number of properties across all columns)
    // value = role name
    QHash<int, QByteArray> mRoleNames;
    QVariantList mInitialRows;
};

QT_END_NAMESPACE

#endif // QQMLTREEMODEL_P_H

