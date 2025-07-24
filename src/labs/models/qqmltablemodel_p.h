// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTABLEMODEL_P_H
#define QQMLTABLEMODEL_P_H

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

#include "qqmlabstractcolumnmodel_p.h"

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QAbstractTableModel>
#include <QtQml/qqml.h>
#include <QtQmlModels/private/qtqmlmodelsglobal_p.h>
#include <QtQml/QJSValue>
#include <QtQml/QQmlListProperty>

QT_REQUIRE_CONFIG(qml_table_model);

QT_BEGIN_NAMESPACE

class Q_LABSQMLMODELS_EXPORT QQmlTableModel : public QQmlAbstractColumnModel
{
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged FINAL)
    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged FINAL)
    QML_NAMED_ELEMENT(TableModel)
    QML_ADDED_IN_VERSION(1, 0)

public:
    Q_DISABLE_COPY_MOVE(QQmlTableModel)

    explicit QQmlTableModel(QObject *parent = nullptr);
    ~QQmlTableModel() override;

    QVariant rows() const;
    void setRows(const QVariant &rows);

    Q_INVOKABLE void appendRow(const QVariant &row);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariant getRow(int rowIndex);
    Q_INVOKABLE void insertRow(int rowIndex, const QVariant &row);
    Q_INVOKABLE void moveRow(int fromRowIndex, int toRowIndex, int rows = 1);
    Q_INVOKABLE void removeRow(int rowIndex, int rows = 1);
    Q_INVOKABLE void setRow(int rowIndex, const QVariant &row);

    //AbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, const QString &role) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE bool setData(const QModelIndex &index, const QString &role, const QVariant &value);
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole) override;
    QModelIndex parent(const QModelIndex &index) const override;

Q_SIGNALS:
    void rowCountChanged();
    void rowsChanged();

protected:
    QVariant firstRow() const override;
    void setInitialRows() override;

private:

    enum NewRowOperationFlag {
        OtherOperation, // insert(), set(), etc.
        SetRowsOperation,
        AppendOperation
    };

    void setRowsPrivate(const QVariantList &rowsAsVariantList);

    bool validateRowType(QLatin1StringView functionName, const QVariant &row) const;
    bool validateNewRow(QLatin1StringView functionName, const QVariant &row,
        int rowIndex, NewRowOperationFlag operation = OtherOperation) const;
    bool validateRowIndex(QLatin1StringView functionName, const char *argumentName, int rowIndex) const;

    void doInsert(int rowIndex, const QVariant &row);

    QVariantList mRows;
    int mRowCount = 0;
};

QT_END_NAMESPACE

#endif // QQMLTABLEMODEL_P_H
