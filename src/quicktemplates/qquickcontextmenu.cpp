// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickcontextmenu_p.h"

#include <QtCore/qpointer.h>
#include <QtCore/qloggingcategory.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuickTemplates2/private/qquickmenu_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcContextMenu, "qt.quick.controls.contextmenu")

/*!
    \qmltype ContextMenu
    \brief The ContextMenu attached type provides a way to open a context menu
        in a platform-appropriate manner.
    \inqmlmodule QtQuick.Controls
    \ingroup qtquickcontrols-menus
    \since 6.9

    ContextMenu can be attached to any \l {QQuickItem}{item} in order to show a context menu
    upon a platform-specific event, such as a right click or the context menu
    key.

    \omit
    TODO: code snippet
    \endomit
*/

class QQuickContextMenuPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickContextMenu)

    static QQuickContextMenuPrivate *get(QQuickContextMenu *attachedObject)
    {
        return attachedObject->d_func();
    }

    QPointer<QQuickMenu> menu;
};

QQuickContextMenu::QQuickContextMenu(QObject *parent)
    : QObject(*(new QQuickContextMenuPrivate), parent)
{
    Q_ASSERT(parent);
    if (parent->isQuickItemType()) {
        auto *itemPriv = QQuickItemPrivate::get(static_cast<QQuickItem *>(parent));
        Q_ASSERT(itemPriv);
        if (QObject *oldMenu = itemPriv->setContextMenu(this))
            qCWarning(lcContextMenu) << this << "replaced" << oldMenu << "on" << parent;
    } else {
        qmlWarning(parent) << "ContextMenu must be attached to an Item";
    }
}

QQuickContextMenu *QQuickContextMenu::qmlAttachedProperties(QObject *object)
{
    return new QQuickContextMenu(object);
}

/*!
    \qmlproperty Menu QtQuick.Controls::ContextMenu::menu

    This property holds the context menu that will be opened. It can be set to
    any \l Menu object.
*/
QQuickMenu *QQuickContextMenu::menu() const
{
    Q_D(const QQuickContextMenu);
    return d->menu;
}

void QQuickContextMenu::setMenu(QQuickMenu *menu)
{
    Q_D(QQuickContextMenu);
    if (!parent()->isQuickItemType())
        return;

    if (menu == d->menu)
        return;

    d->menu = menu;
    emit menuChanged();
}

bool QQuickContextMenu::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ContextMenu: {
        qCDebug(lcContextMenu) << this << "handling" << event << "on behalf of" << parent();

        auto *attacheeItem = qobject_cast<QQuickItem *>(parent());
        auto *menu = this->menu();
        if (!menu) {
            qCDebug(lcContextMenu) << this << "no menu instance";
            return QObject::event(event);
        }
        menu->setParentItem(attacheeItem);

        const auto *contextMenuEvent = static_cast<QContextMenuEvent *>(event);
        const QPoint posRelativeToParent(attacheeItem->mapFromScene(contextMenuEvent->pos()).toPoint());
        qCDebug(lcContextMenu) << this << "showing" << menu << "at" << posRelativeToParent;
        menu->popup(posRelativeToParent);
        event->accept();
        return true;
    }
    default:
        break;
    }
    return QObject::event(event);
}

QT_END_NAMESPACE

#include "moc_qquickcontextmenu_p.cpp"
