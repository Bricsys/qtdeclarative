// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshortcutcontext_p_p.h"
#include "qquickoverlay_p_p.h"
#include "qquicktooltip_p.h"
#include <QtQmlModels/private/qtqmlmodels-config_p.h>
#if QT_CONFIG(qml_object_model)
#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"
#endif
#include "qquickpopup_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQuick/qquickrendercontrol.h>
#include <QtQuickTemplates2/private/qquickpopupwindow_p_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcContextMatcher, "qt.quick.controls.shortcutcontext.matcher")

static bool isBlockedByPopup(QQuickItem *item)
{
    if (!item || !item->window())
        return false;

    QQuickOverlay *overlay = QQuickOverlay::overlay(item->window());
    auto popups = QQuickOverlayPrivate::get(overlay)->stackingOrderPopups();

    for (QWindow *popupWindow : QGuiApplicationPrivate::popup_list) {
        if (QQuickPopupWindow *quickPopupWindow = qobject_cast<QQuickPopupWindow *>(popupWindow);
            quickPopupWindow && quickPopupWindow->popup())
            popups += quickPopupWindow->popup();
    }

    for (QQuickPopup *popup : std::as_const(popups)) {
        if (qobject_cast<QQuickToolTip *>(popup))
            continue; // ignore tooltips (QTBUG-60492)
        if (popup->isModal() || popup->closePolicy() & QQuickPopup::CloseOnEscape) {
            qCDebug(lcContextMatcher) << popup << "is modal or has a CloseOnEscape policy;"
                                      << "if one of the following is true," << item
                                      << "will be blocked by it:" << (item != popup->popupItem())
                                      << !popup->popupItem()->isAncestorOf(item);
            return item != popup->popupItem() && !popup->popupItem()->isAncestorOf(item);
        }
    }
    return false;
}

bool QQuickShortcutContext::matcher(QObject *obj, Qt::ShortcutContext context)
{
    if ((context != Qt::ApplicationShortcut) && (context != Qt::WindowShortcut))
        return false;

    QQuickItem *item = nullptr;

    // look for the window contains embedded shortcut
    while (obj && !obj->isWindowType()) {
        item = qobject_cast<QQuickItem *>(obj);
        if (item && item->window()) {
            obj = item->window();
            break;
        } else if (QQuickPopup *popup = qobject_cast<QQuickPopup *>(obj)) {
            obj = popup->window();
            item = popup->popupItem();

#if QT_CONFIG(qml_object_model)
            if (!obj) {
                // The popup has no associated window (yet). However, sub-menus,
                // unlike top-level menus, will not have an associated window
                // until their parent menu is opened. So, check if this is a sub-menu
                // so that actions within it can grab shortcuts.
                if (auto *menu = qobject_cast<QQuickMenu *>(popup)) {
                    auto parentMenu = QQuickMenuPrivate::get(menu)->parentMenu;
                    while (parentMenu) {
                        obj = parentMenu->window();
                        if (obj)
                            break;

                        parentMenu = QQuickMenuPrivate::get(parentMenu)->parentMenu;
                    }
                }
            }
#endif
            break;
        }
        obj = obj->parent();
    }

    if (context == Qt::ApplicationShortcut) {
        // the application shortcuts inside hidden/closed windows should not match
        return obj && qobject_cast<QWindow*>(obj)->isVisible();
    } else {
        Q_ASSERT(context == Qt::WindowShortcut);
        QQuickWindow *window = qobject_cast<QQuickWindow *>(obj);
        if (QWindow *renderWindow = QQuickRenderControl::renderWindowFor(window))
            obj = renderWindow;
        qCDebug(lcContextMatcher) << "obj" << obj << "item" << item << "focusWindow"
                                  << QGuiApplication::focusWindow()
                                  << "!isBlockedByPopup(item)" << !isBlockedByPopup(item);
        return obj && qobject_cast<QWindow*>(obj)->isActive() && !isBlockedByPopup(item);
    }
}

QT_END_NAMESPACE
