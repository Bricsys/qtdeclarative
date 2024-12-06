// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickheaderviewdelegate_p.h"

#include <QtQuickTemplates2/private/qquickheaderview_p.h>
#include <QtQuickTemplates2/private/qquickheaderview_p_p.h>
#include <QtQuickTemplates2/private/qquicktableviewdelegate_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickHeaderViewDelegatePrivate : public QQuickTableViewDelegatePrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickHeaderViewDelegate)

public:
    QPointer<QQuickHeaderViewBase> headerView;
    QVariant model;
};

QQuickHeaderViewDelegate::QQuickHeaderViewDelegate(QQuickItem *parent)
    : QQuickTableViewDelegate(*(new QQuickHeaderViewDelegatePrivate), parent)
{
}

QQuickHeaderViewBase *QQuickHeaderViewDelegate::headerView() const
{
    return d_func()->headerView;
}

Qt::Orientation QQuickHeaderViewDelegate::orientation() const
{
    return headerView()
            ? QQuickHeaderViewBasePrivate::get(headerView())->orientation()
            : Qt::Horizontal;
}

void QQuickHeaderViewDelegate::setHeaderView(QQuickHeaderViewBase *headerView)
{
    Q_D(QQuickHeaderViewDelegate);
    if (d->headerView == headerView)
        return;

    const Qt::Orientation oldOrientation = orientation();

    d->headerView = headerView;
    emit headerViewChanged();

    if (oldOrientation != orientation())
        emit orientationChanged();
}

QVariant QQuickHeaderViewDelegate::model() const
{
    return d_func()->model;
}

void QQuickHeaderViewDelegate::setModel(const QVariant &model)
{
    Q_D(QQuickHeaderViewDelegate);
    if (d->model == model)
        return;
    d->model = model;
    emit modelChanged();
}

QT_END_NAMESPACE

#include "moc_qquickheaderviewdelegate_p.cpp"
