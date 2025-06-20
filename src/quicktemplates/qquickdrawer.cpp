// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdrawer_p.h"
#include "qquickdrawer_p_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickpopuppositioner_p_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlinfo.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Drawer
    \inherits Popup
//!     \nativetype QQuickDrawer
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-navigation
    \ingroup qtquickcontrols-popups
    \brief Side panel that can be opened and closed using a swipe gesture.

    Drawer provides a swipe-based side panel, similar to those often used in
    touch interfaces to provide a central location for navigation.

    \image qtquickcontrols-drawer.gif

    Drawer can be positioned at any of the four edges of the content item.
    The drawer above is positioned against the left edge of the window. The
    drawer is then opened by \e "dragging" it out from the left edge of the
    window.

    \code
    import QtQuick
    import QtQuick.Controls

    ApplicationWindow {
        id: window
        visible: true

        Drawer {
            id: drawer
            width: 0.66 * window.width
            height: window.height

            Label {
                text: "Content goes here!"
                anchors.centerIn: parent
            }
        }
    }
    \endcode

    Drawer is a special type of popup that resides at one of the window \l {edge}{edges}.
    By default, Drawer re-parents itself to the window \c overlay, and therefore operates
    on window coordinates. It is also possible to manually set the \l{Popup::}{parent} to
    something else to make the drawer operate in a specific coordinate space.

    Drawer can be configured to cover only part of its window edge. The following example
    illustrates how Drawer can be positioned to appear below a window header:

    \code
    import QtQuick
    import QtQuick.Controls

    ApplicationWindow {
        id: window
        visible: true

        header: ToolBar { }

        Drawer {
            y: header.height
            width: window.width * 0.6
            height: window.height - header.height
        }
    }
    \endcode

    The \l position property determines how much of the drawer is visible, as
    a value between \c 0.0 and \c 1.0. It is not possible to set the x-coordinate
    (or horizontal margins) of a drawer at the left or right window edge, or the
    y-coordinate (or vertical margins) of a drawer at the top or bottom window edge.

    In the image above, the application's contents are \e "pushed" across the
    screen. This is achieved by applying a translation to the contents:

    \code
    import QtQuick
    import QtQuick.Controls

    ApplicationWindow {
        id: window
        width: 200
        height: 228
        visible: true

        Drawer {
            id: drawer
            width: 0.66 * window.width
            height: window.height
        }

        Label {
            id: content

            text: "Aa"
            font.pixelSize: 96
            anchors.fill: parent
            verticalAlignment: Label.AlignVCenter
            horizontalAlignment: Label.AlignHCenter

            transform: Translate {
                x: drawer.position * content.width * 0.33
            }
        }
    }
    \endcode

    If you would like the application's contents to stay where they are when
    the drawer is opened, don't apply a translation.

    Drawer can be configured as a non-closable persistent side panel by
    making the Drawer \l {Popup::modal}{non-modal} and \l {interactive}
    {non-interactive}. See the \l {Qt Quick Controls 2 - Gallery}{Gallery}
    example for more details.

    \note On some platforms, certain edges may be reserved for system
    gestures and therefore cannot be used with Drawer. For example, the
    top and bottom edges may be reserved for system notifications and
    control centers on Android and iOS.

    \sa SwipeView, {Customizing Drawer}, {Navigation Controls}, {Popup Controls}
*/

class QQuickDrawerPositioner : public QQuickPopupPositioner
{
public:
    QQuickDrawerPositioner(QQuickDrawer *drawer) : QQuickPopupPositioner(drawer) { }

    void reposition() override;
};

qreal QQuickDrawerPrivate::offsetAt(const QPointF &point) const
{
    qreal offset = positionAt(point) - position;

    // don't jump when dragged open
    if (offset > 0 && position > 0 && !contains(point))
        offset = 0;

    return offset;
}

qreal QQuickDrawerPrivate::positionAt(const QPointF &point) const
{
    Q_Q(const QQuickDrawer);
    QQuickWindow *window = q->window();
    if (!window)
        return 0;

    auto size = QSizeF(q->width(), q->height());

    switch (effectiveEdge()) {
    case Qt::TopEdge:
        if (edge == Qt::LeftEdge || edge == Qt::RightEdge)
            size.transpose();
        return point.y() / size.height();
    case Qt::LeftEdge:
        if (edge == Qt::TopEdge || edge == Qt::BottomEdge)
            size.transpose();
        return point.x() / size.width();
    case Qt::RightEdge:
        if (edge == Qt::TopEdge || edge == Qt::BottomEdge)
            size.transpose();
        return (window->width() - point.x()) / size.width();
    case Qt::BottomEdge:
        if (edge == Qt::LeftEdge || edge == Qt::RightEdge)
            size.transpose();
        return (window->height() - point.y()) / size.height();
    default:
        return 0;
    }
}

QQuickPopupPositioner *QQuickDrawerPrivate::getPositioner()
{
    Q_Q(QQuickDrawer);
    if (!positioner)
        positioner = new QQuickDrawerPositioner(q);
    return positioner;
}

void QQuickDrawerPositioner::reposition()
{
    if (m_positioning)
        return;

    QQuickDrawer *drawer = static_cast<QQuickDrawer*>(popup());

    // The overlay is assumed to fully cover the window's contents, although the overlay's geometry
    // might not always equal the window's geometry (for example, if the window's contents are rotated).
    QQuickOverlay *overlay = QQuickOverlay::overlay(drawer->window());
    if (!overlay)
        return;

    const qreal position = drawer->position();
    QQuickItem *popupItem = drawer->popupItem();
    switch (drawer->edge()) {
    case Qt::LeftEdge:
        popupItem->setX((position - 1.0) * popupItem->width());
        break;
    case Qt::RightEdge:
        popupItem->setX(overlay->width() - position * popupItem->width());
        break;
    case Qt::TopEdge:
        popupItem->setY((position - 1.0) * popupItem->height());
        break;
    case Qt::BottomEdge:
        popupItem->setY(overlay->height() - position * popupItem->height());
        break;
    }

    QQuickPopupPositioner::reposition();
}

void QQuickDrawerPrivate::showDimmer()
{
    // managed in setPosition()
}

void QQuickDrawerPrivate::hideDimmer()
{
    // managed in setPosition()
}

void QQuickDrawerPrivate::resizeDimmer()
{
    if (!dimmer || !window)
        return;

    const QQuickOverlay *overlay = QQuickOverlay::overlay(window);

    QRectF geometry(0, 0, overlay ? overlay->width() : 0, overlay ? overlay->height() : 0);

    if (edge == Qt::LeftEdge || edge == Qt::RightEdge) {
        geometry.setY(popupItem->y());
        geometry.setHeight(popupItem->height());
    } else {
        geometry.setX(popupItem->x());
        geometry.setWidth(popupItem->width());
    }

    dimmer->setPosition(geometry.topLeft());
    dimmer->setSize(geometry.size());
}

bool QQuickDrawerPrivate::isWithinDragMargin(const QPointF &pos) const
{
    Q_Q(const QQuickDrawer);
    switch (effectiveEdge()) {
    case Qt::LeftEdge:
        return pos.x() <= q->dragMargin();
    case Qt::RightEdge:
        return pos.x() >= q->window()->width() - q->dragMargin();
    case Qt::TopEdge:
        return pos.y() <= q->dragMargin();
    case Qt::BottomEdge:
        return pos.y() >= q->window()->height() - q->dragMargin();
    default:
        Q_UNREACHABLE();
        break;
    }
    return false;
}

bool QQuickDrawerPrivate::startDrag(QEvent *event)
{
    delayedEnterTransition = false;
    if (!window || !interactive || dragMargin < 0.0 || qFuzzyIsNull(dragMargin))
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event); isWithinDragMargin(mouseEvent->scenePosition())) {
            // watch future events and grab the mouse once it has moved
            // sufficiently fast or far (in grabMouse).
            delayedEnterTransition = true;
            mouseEvent->addPassiveGrabber(mouseEvent->point(0), popupItem);
            handleMouseEvent(window->contentItem(), mouseEvent);
            return false;
        }
        break;

#if QT_CONFIG(quicktemplates2_multitouch)
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        for (const QTouchEvent::TouchPoint &point : touchEvent->points()) {
            if (point.state() == QEventPoint::Pressed && isWithinDragMargin(point.scenePosition())) {
                delayedEnterTransition = true;
                touchEvent->addPassiveGrabber(point, popupItem);
                handleTouchEvent(window->contentItem(), touchEvent);
                return false;
            }
        }
        break;
    }
#endif

    default:
        break;
    }

    return false;
}

static inline bool keepGrab(QQuickItem *item)
{
    return item->keepMouseGrab() || item->keepTouchGrab();
}

bool QQuickDrawerPrivate::grabMouse(QQuickItem *item, QMouseEvent *event)
{
    Q_Q(QQuickDrawer);
    handleMouseEvent(item, event);

    if (!window || !interactive || keepGrab(popupItem) || keepGrab(item))
        return false;

    const QPointF movePoint = event->scenePosition();

    // Flickable uses a hard-coded threshold of 15 for flicking, and
    // QStyleHints::startDragDistance for dragging. Drawer uses a bit
    // larger threshold to avoid being too eager to steal touch (QTBUG-50045)
    const int threshold = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5);
    bool overThreshold = false;
    Qt::Edge effEdge = effectiveEdge();
    if (position > 0 || dragMargin > 0) {
        const bool xOverThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(movePoint.x() - pressPoint.x(),
                                                                    Qt::XAxis, event, threshold);
        const bool yOverThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(movePoint.y() - pressPoint.y(),
                                                                    Qt::YAxis, event, threshold);
        if (effEdge == Qt::LeftEdge || effEdge == Qt::RightEdge)
            overThreshold = xOverThreshold && !yOverThreshold;
        else
            overThreshold = yOverThreshold && !xOverThreshold;
    }

    // Don't be too eager to steal presses outside the drawer (QTBUG-53929)
    if (overThreshold && qFuzzyCompare(position, qreal(1.0)) && !contains(movePoint)) {
        if (effEdge == Qt::LeftEdge || effEdge == Qt::RightEdge)
            overThreshold = qAbs(movePoint.x() - q->width()) < dragMargin;
        else
            overThreshold = qAbs(movePoint.y() - q->height()) < dragMargin;
    }

    if (overThreshold) {
        if (delayedEnterTransition) {
            prepareEnterTransition();
            reposition();
            delayedEnterTransition = false;
        }

        popupItem->grabMouse();
        popupItem->setKeepMouseGrab(true);
        offset = offsetAt(movePoint);
    }

    return overThreshold;
}

#if QT_CONFIG(quicktemplates2_multitouch)
bool QQuickDrawerPrivate::grabTouch(QQuickItem *item, QTouchEvent *event)
{
    Q_Q(QQuickDrawer);
    bool handled = handleTouchEvent(item, event);

    if (!window || !interactive || keepGrab(popupItem) || keepGrab(item) || !event->touchPointStates().testFlag(QEventPoint::Updated))
        return handled;

    bool overThreshold = false;
    for (const QTouchEvent::TouchPoint &point : event->points()) {
        if (!acceptTouch(point) || point.state() != QEventPoint::Updated)
            continue;

        const QPointF movePoint = point.scenePosition();

        // Flickable uses a hard-coded threshold of 15 for flicking, and
        // QStyleHints::startDragDistance for dragging. Drawer uses a bit
        // larger threshold to avoid being too eager to steal touch (QTBUG-50045)
        const int threshold = qMax(20, QGuiApplication::styleHints()->startDragDistance() + 5);
        const Qt::Edge effEdge = effectiveEdge();
        if (position > 0 || dragMargin > 0) {
            const bool xOverThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(movePoint.x() - pressPoint.x(),
                                                                    Qt::XAxis, point, threshold);
            const bool yOverThreshold = QQuickDeliveryAgentPrivate::dragOverThreshold(movePoint.y() - pressPoint.y(),
                                                                    Qt::YAxis, point, threshold);
            if (effEdge == Qt::LeftEdge || effEdge == Qt::RightEdge)
                overThreshold = xOverThreshold && !yOverThreshold;
            else
                overThreshold = yOverThreshold && !xOverThreshold;
        }

        // Don't be too eager to steal presses outside the drawer (QTBUG-53929)
        if (overThreshold && qFuzzyCompare(position, qreal(1.0)) && !contains(movePoint)) {
            if (effEdge == Qt::LeftEdge || effEdge == Qt::RightEdge)
                overThreshold = qAbs(movePoint.x() - q->width()) < dragMargin;
            else
                overThreshold = qAbs(movePoint.y() - q->height()) < dragMargin;
        }

        if (overThreshold) {
            if (delayedEnterTransition) {
                prepareEnterTransition();
                reposition();
                delayedEnterTransition = false;
            }
            event->setExclusiveGrabber(point, popupItem);
            popupItem->setKeepTouchGrab(true);
            offset = offsetAt(movePoint);
        }
    }

    return overThreshold;
}
#endif

static const qreal openCloseVelocityThreshold = 300;

// Overrides QQuickPopupPrivate::blockInput, which is called by
// QQuickPopupPrivate::handlePress/Move/Release, which we call in our own
// handlePress/Move/Release overrides.
// This implementation conflates two things: should the event going to the item get
// modally blocked by us? Or should we accept the event and become the grabber?
// Those are two fundamentally different questions for the drawer as a (usually)
// interactive control.
bool QQuickDrawerPrivate::blockInput(QQuickItem *item, const QPointF &point) const
{
    // We want all events, if mouse/touch is already grabbed.
    if (popupItem->keepMouseGrab() || popupItem->keepTouchGrab())
        return true;

    // Don't block input to drawer's children/content.
    if (popupItem->isAncestorOf(item))
        return false;

    // Don't block outside a drawer's background dimming
    if (dimmer && !dimmer->contains(dimmer->mapFromScene(point)))
        return false;

    // Accept all events within drag area.
    if (isWithinDragMargin(point))
        return true;

    // Accept all other events if drawer is modal.
    return modal;
}

bool QQuickDrawerPrivate::handlePress(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    offset = 0;
    velocityCalculator.startMeasuring(point, timestamp);

    return QQuickPopupPrivate::handlePress(item, point, timestamp)
        || (interactive && popupItem == item);
}

bool QQuickDrawerPrivate::handleMove(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickDrawer);
    if (!QQuickPopupPrivate::handleMove(item, point, timestamp))
        return false;

    // limit/reset the offset to the edge of the drawer when pushed from the outside
    if (qFuzzyCompare(position, qreal(1.0)) && !contains(point))
        offset = 0;

    bool isGrabbed = popupItem->keepMouseGrab() || popupItem->keepTouchGrab();
    if (isGrabbed)
        q->setPosition(positionAt(point) - offset);

    return isGrabbed;
}

bool QQuickDrawerPrivate::handleRelease(QQuickItem *item, const QPointF &point, ulong timestamp)
{
    auto cleanup = qScopeGuard([this] {
        popupItem->setKeepMouseGrab(false);
        popupItem->setKeepTouchGrab(false);
        pressPoint = QPointF();
        touchId = -1;
    });
    if (pressPoint.isNull())
        return false;
    if (!popupItem->keepMouseGrab() && !popupItem->keepTouchGrab()) {
        velocityCalculator.reset();
        return QQuickPopupPrivate::handleRelease(item, point, timestamp);
    }

    velocityCalculator.stopMeasuring(point, timestamp);
    Qt::Edge effEdge = effectiveEdge();
    qreal velocity = 0;
    if (effEdge == Qt::LeftEdge || effEdge == Qt::RightEdge)
        velocity = velocityCalculator.velocity().x();
    else
        velocity = velocityCalculator.velocity().y();

    // the velocity is calculated so that swipes from left to right
    // and top to bottom have positive velocity, and swipes from right
    // to left and bottom to top have negative velocity.
    //
    // - top/left edge: positive velocity opens, negative velocity closes
    // - bottom/right edge: negative velocity opens, positive velocity closes
    //
    // => invert the velocity for bottom and right edges, for the threshold comparison below
    if (effEdge == Qt::RightEdge || effEdge == Qt::BottomEdge)
        velocity = -velocity;

    if (position > 0.7 || velocity > openCloseVelocityThreshold) {
        transitionManager.transitionEnter();
    } else if (position < 0.3 || velocity < -openCloseVelocityThreshold) {
        transitionManager.transitionExit();
    } else {
        switch (effEdge) {
        case Qt::LeftEdge:
            if (point.x() - pressPoint.x() > 0)
                transitionManager.transitionEnter();
            else
                transitionManager.transitionExit();
            break;
        case Qt::RightEdge:
            if (point.x() - pressPoint.x() < 0)
                transitionManager.transitionEnter();
            else
                transitionManager.transitionExit();
            break;
        case Qt::TopEdge:
            if (point.y() - pressPoint.y() > 0)
                transitionManager.transitionEnter();
            else
                transitionManager.transitionExit();
            break;
        case Qt::BottomEdge:
            if (point.y() - pressPoint.y() < 0)
                transitionManager.transitionEnter();
            else
                transitionManager.transitionExit();
            break;
        }
    }

    // the cleanup() lambda will run before return
    return popupItem->keepMouseGrab() || popupItem->keepTouchGrab();
}

void QQuickDrawerPrivate::handleUngrab()
{
    QQuickPopupPrivate::handleUngrab();

    velocityCalculator.reset();
}

static QList<QQuickStateAction> prepareTransition(QQuickDrawer *drawer, QQuickTransition *transition, qreal to)
{
    QList<QQuickStateAction> actions;
    if (!transition || !QQuickPopupPrivate::get(drawer)->window || !transition->enabled())
        return actions;

    qmlExecuteDeferred(transition);

    QQmlProperty defaultTarget(drawer, QLatin1String("position"));
    QQmlListProperty<QQuickAbstractAnimation> animations = transition->animations();
    int count = animations.count(&animations);
    for (int i = 0; i < count; ++i) {
        QQuickAbstractAnimation *anim = animations.at(&animations, i);
        anim->setDefaultTarget(defaultTarget);
    }

    actions << QQuickStateAction(drawer, QLatin1String("position"), to);
    return actions;
}

bool QQuickDrawerPrivate::prepareEnterTransition()
{
    Q_Q(QQuickDrawer);
    enterActions = prepareTransition(q, enter, 1.0);
    return QQuickPopupPrivate::prepareEnterTransition();
}

bool QQuickDrawerPrivate::prepareExitTransition()
{
    Q_Q(QQuickDrawer);
    exitActions = prepareTransition(q, exit, 0.0);
    return QQuickPopupPrivate::prepareExitTransition();
}

QQuickPopup::PopupType QQuickDrawerPrivate::resolvedPopupType() const
{
    // For now, a drawer will always be shown in-scene
    return QQuickPopup::Item;
}

bool QQuickDrawerPrivate::setEdge(Qt::Edge e)
{
    Q_Q(QQuickDrawer);
    switch (e) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        allowVerticalMove = true;
        allowVerticalResize = true;
        allowHorizontalMove = false;
        allowHorizontalResize = false;
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        allowVerticalMove = false;
        allowVerticalResize = false;
        allowHorizontalMove = true;
        allowHorizontalResize = true;
        break;
    default:
        qmlWarning(q) << "invalid edge value - valid values are: "
            << "Qt.TopEdge, Qt.LeftEdge, Qt.RightEdge, Qt.BottomEdge";
        return false;
    }

    edge = e;
    return true;
}

QQuickDrawer::QQuickDrawer(QObject *parent)
    : QQuickPopup(*(new QQuickDrawerPrivate), parent)
{
    Q_D(QQuickDrawer);
    d->dragMargin = QGuiApplication::styleHints()->startDragDistance();
    d->setEdge(Qt::LeftEdge);

    setFocus(true);
    setModal(true);

    QQuickItemPrivate::get(d->popupItem)->isTabFence = isModal();
    connect(this, &QQuickPopup::modalChanged, this, [this] {
        QQuickItemPrivate::get(d_func()->popupItem)->isTabFence = isModal();
    });

    setFiltersChildMouseEvents(true);
    setClosePolicy(CloseOnEscape | CloseOnReleaseOutside);
}

/*!
    \qmlproperty enumeration QtQuick.Controls::Drawer::edge

    This property holds the edge of the window at which the drawer will
    open from. The acceptable values are:

    \value Qt.TopEdge     The top edge of the window.
    \value Qt.LeftEdge    The left edge of the window (default).
    \value Qt.RightEdge   The right edge of the window.
    \value Qt.BottomEdge  The bottom edge of the window.
*/
Qt::Edge QQuickDrawer::edge() const
{
    Q_D(const QQuickDrawer);
    return d->edge;
}

Qt::Edge QQuickDrawerPrivate::effectiveEdge() const
{
    auto realEdge = edge;
    qreal rotation = window->contentItem()->rotation();
    const bool clockwise = rotation > 0;
    while (qAbs(rotation) >= 90) {
        rotation -= clockwise ? 90 : -90;
        switch (realEdge) {
        case Qt::LeftEdge:
            realEdge = clockwise ? Qt::TopEdge : Qt::BottomEdge;
            break;
        case Qt::TopEdge:
            realEdge = clockwise ? Qt::RightEdge : Qt::LeftEdge;
            break;
        case Qt::RightEdge:
            realEdge = clockwise ? Qt::BottomEdge : Qt::TopEdge;
            break;
        case Qt::BottomEdge:
            realEdge = clockwise ? Qt::LeftEdge : Qt::RightEdge;
            break;
        }
    }
    return realEdge;
}

void QQuickDrawer::setEdge(Qt::Edge edge)
{
    Q_D(QQuickDrawer);
    if (d->edge == edge)
        return;

    if (!d->setEdge(edge))
        return;

    if (isComponentComplete())
        d->reposition();
    emit edgeChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Drawer::position

    This property holds the position of the drawer relative to its final
    destination. That is, the position will be \c 0.0 when the drawer
    is fully closed, and \c 1.0 when fully open.
*/
qreal QQuickDrawer::position() const
{
    Q_D(const QQuickDrawer);
    return d->position;
}

void QQuickDrawer::setPosition(qreal position)
{
    Q_D(QQuickDrawer);
    position = std::clamp(position, qreal(0.0), qreal(1.0));
    if (qFuzzyCompare(d->position, position))
        return;

    d->position = position;
    if (isComponentComplete())
        d->reposition();
    if (d->dimmer)
        d->dimmer->setOpacity(position);
    emit positionChanged();
}

/*!
    \qmlproperty real QtQuick.Controls::Drawer::dragMargin

    This property holds the distance from the screen edge within which
    drag actions will open the drawer. Setting the value to \c 0 or less
    prevents opening the drawer by dragging.

    The default value is \c Application.styleHints.startDragDistance.

    \sa interactive
*/
qreal QQuickDrawer::dragMargin() const
{
    Q_D(const QQuickDrawer);
    return d->dragMargin;
}

void QQuickDrawer::setDragMargin(qreal margin)
{
    Q_D(QQuickDrawer);
    if (qFuzzyCompare(d->dragMargin, margin))
        return;

    d->dragMargin = margin;
    emit dragMarginChanged();
}

void QQuickDrawer::resetDragMargin()
{
    setDragMargin(QGuiApplication::styleHints()->startDragDistance());
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty bool QtQuick.Controls::Drawer::interactive

    This property holds whether the drawer is interactive. A non-interactive
    drawer does not react to swipes.

    The default value is \c true.

    \sa dragMargin
*/
bool QQuickDrawer::isInteractive() const
{
    Q_D(const QQuickDrawer);
    return d->interactive;
}

void QQuickDrawer::setInteractive(bool interactive)
{
    Q_D(QQuickDrawer);
    if (d->interactive == interactive)
        return;

    setFiltersChildMouseEvents(interactive);
    d->interactive = interactive;
    emit interactiveChanged();
}

bool QQuickDrawer::childMouseEventFilter(QQuickItem *child, QEvent *event)
{
    Q_D(QQuickDrawer);
    switch (event->type()) {
#if QT_CONFIG(quicktemplates2_multitouch)
    case QEvent::TouchUpdate:
        return d->grabTouch(child, static_cast<QTouchEvent *>(event));
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
        return d->handleTouchEvent(child, static_cast<QTouchEvent *>(event));
#endif
    case QEvent::MouseMove:
        return d->grabMouse(child, static_cast<QMouseEvent *>(event));
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        return d->handleMouseEvent(child, static_cast<QMouseEvent *>(event));
    default:
        break;
    }
    return false;
}

void QQuickDrawer::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickDrawer);
    d->grabMouse(d->popupItem, event);
}

bool QQuickDrawer::overlayEvent(QQuickItem *item, QEvent *event)
{
    Q_D(QQuickDrawer);
    switch (event->type()) {
#if QT_CONFIG(quicktemplates2_multitouch)
    case QEvent::TouchUpdate:
        return d->grabTouch(item, static_cast<QTouchEvent *>(event));
#endif
    case QEvent::MouseMove:
        return d->grabMouse(item, static_cast<QMouseEvent *>(event));
    default:
        break;
    }
    return QQuickPopup::overlayEvent(item, event);
}

#if QT_CONFIG(quicktemplates2_multitouch)
void QQuickDrawer::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickDrawer);
    d->grabTouch(d->popupItem, event);
}
#endif

void QQuickDrawer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickDrawer);
    QQuickPopup::geometryChange(newGeometry, oldGeometry);
    d->resizeDimmer();
}

QT_END_NAMESPACE

#include "moc_qquickdrawer_p.cpp"
