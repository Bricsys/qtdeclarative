// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QMLSORTFILTERPROXYMODEL_H
#define QMLSORTFILTERPROXYMODEL_H

#include <QtQml/qqmlregistration.h>
#include <QSortFilterProxyModel>

class QmlSortFilterProxyModel {
    Q_GADGET
    QML_FOREIGN(QSortFilterProxyModel)
    QML_NAMED_ELEMENT(SortFilterProxyModel)
};

#endif // QMLSORTFILTERPROXYMODEL_H
