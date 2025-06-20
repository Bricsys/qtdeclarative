// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:interprocess-communication

#include "qquickdrag_p.h"
#include "qquickdrag_p_p.h"

#include <private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qquickitem_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <private/qquickitemchangelistener_p.h>
#include <private/qquickpixmap_p.h>
#include <private/qv4scopedvalue_p.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qmimedata.h>
#include <QtCore/qstringconverter.h>
#include <QtQml/qqmlinfo.h>
#include <QtGui/qevent.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qimagewriter.h>

#include <qpa/qplatformdrag.h>
#include <QtGui/qdrag.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;


/*!
    \qmltype Drag
    \nativetype QQuickDrag
    \inqmlmodule QtQuick
    \ingroup qtquick-input
    \brief For specifying drag and drop events for moved Items.

    Using the Drag attached property, any Item can be made a source of drag and drop
    events within a scene.

    When a drag is \l active on an item, any change in that item's position will
    generate a drag event that will be sent to any DropArea that intersects
    with the new position of the item.  Other items which implement drag and
    drop event handlers can also receive these events.

    The following snippet shows how an item can be dragged with a MouseArea.
    However, dragging is not limited to mouse drags; anything that can move an item
    can generate drag events, including touch events, animations and bindings.

    \snippet qml/drag.qml 0

    A drag can be terminated either by canceling it with Drag.cancel() or setting
    Drag.active to false, or it can be terminated with a drop event by calling
    Drag.drop().  If the drop event is accepted, Drag.drop() will return the
    \l {supportedActions}{drop action} chosen by the recipient of the event,
    otherwise it will return Qt.IgnoreAction.

    \sa {Qt Quick Examples - Drag and Drop}
*/

void QQuickDragAttachedPrivate::itemGeometryChanged(QQuickItem *, QQuickGeometryChange change,
                                                    const QRectF &)
{
    if (!change.positionChange() || !active || itemMoved)
        return;
    updatePosition();
}

void QQuickDragAttachedPrivate::itemParentChanged(QQuickItem *, QQuickItem *)
{
    if (!active || dragRestarted)
        return;

    QQuickWindow *newWindow = attachedItem->window();

    if (window != newWindow)
        restartDrag();
    else if (window)
        updatePosition();
}

void QQuickDragAttachedPrivate::updatePosition()
{
    Q_Q(QQuickDragAttached);
    itemMoved = true;
    if (!eventQueued) {
        eventQueued = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQuickDragAttachedPrivate::restartDrag()
{
    Q_Q(QQuickDragAttached);
    dragRestarted = true;
    if (!eventQueued) {
        eventQueued = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQuickDragAttachedPrivate::deliverEnterEvent()
{
    dragRestarted = false;
    itemMoved = false;

    window = attachedItem->window();

    mimeData->m_source = source;
    if (!overrideActions)
        mimeData->m_supportedActions = supportedActions;
    mimeData->m_keys = keys;

    if (window) {
        QPoint scenePos = attachedItem->mapToScene(hotSpot).toPoint();
        QDragEnterEvent event(scenePos, mimeData->m_supportedActions, mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, proposedAction);
        deliverEvent(window, &event);
    }
}

void QQuickDragAttachedPrivate::deliverMoveEvent()
{
    Q_Q(QQuickDragAttached);

    itemMoved = false;
    if (window) {
        QPoint scenePos = attachedItem->mapToScene(hotSpot).toPoint();
        QDragMoveEvent event(scenePos, mimeData->m_supportedActions, mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, proposedAction);
        deliverEvent(window, &event);
        if (target != dragGrabber.target()) {
            target = dragGrabber.target();
            emit q->targetChanged();
        }
    }
}

void QQuickDragAttachedPrivate::deliverLeaveEvent()
{
    if (window) {
        QDragLeaveEvent event;
        deliverEvent(window, &event);
        window = nullptr;
    }
}

void QQuickDragAttachedPrivate::deliverEvent(QQuickWindow *window, QEvent *event)
{
    Q_ASSERT(!inEvent);
    inEvent = true;
    QQuickWindowPrivate::get(window)->deliveryAgentPrivate()->deliverDragEvent(&dragGrabber, event);
    inEvent = false;
}

bool QQuickDragAttached::event(QEvent *event)
{
    Q_D(QQuickDragAttached);

    if (event->type() == QEvent::User) {
        d->eventQueued = false;
        if (d->dragRestarted) {
            d->deliverLeaveEvent();
            if (!d->mimeData)
                d->mimeData = new QQuickDragMimeData;
            d->deliverEnterEvent();

            if (d->target != d->dragGrabber.target()) {
                d->target = d->dragGrabber.target();
                emit targetChanged();
            }
        } else if (d->itemMoved) {
            d->deliverMoveEvent();
        }
        return true;
    } else {
        return QObject::event(event);
    }
}

QQuickDragAttached::QQuickDragAttached(QObject *parent)
    : QObject(*new QQuickDragAttachedPrivate, parent)
{
    Q_D(QQuickDragAttached);
    d->attachedItem = qobject_cast<QQuickItem *>(parent);
    d->source = d->attachedItem;
}

QQuickDragAttached::~QQuickDragAttached()
{
    Q_D(QQuickDragAttached);
    delete d->mimeData;
}

/*!
    \qmlattachedproperty bool QtQuick::Drag::active

    This property holds whether a drag event sequence is currently active.

    Binding this property to the active property of \l MouseArea::drag will
    cause \l startDrag to be called when the user starts dragging.

    Setting this property to true will also send a QDragEnter event to the scene
    with the item's current position.  Setting it to false will send a
    QDragLeave event.

    While a drag is active any change in an item's position will send a QDragMove
    event with item's new position to the scene.
*/

bool QQuickDragAttached::isActive() const
{
    Q_D(const QQuickDragAttached);
    return d->active;
}

void QQuickDragAttached::setActive(bool active)
{
    Q_D(QQuickDragAttached);
    if (d->active != active) {
        if (d->inEvent)
            qmlWarning(this) << "active cannot be changed from within a drag event handler";
        else if (active) {
            if (d->dragType == QQuickDrag::Internal) {
                d->start(d->supportedActions);
            } else {
                d->active = true;
                emit activeChanged();
                if (d->dragType == QQuickDrag::Automatic) {
                    // There are different semantics than start() since startDrag()
                    // may be called after an internal drag is already started.
                    d->startDrag(d->supportedActions);
                }
            }
        }
        else
            cancel();
    }
}

/*!
    \qmlattachedproperty Object QtQuick::Drag::source

    This property holds an object that is identified to recipients of drag events as
    the source of the events.  By default this is the item that the Drag
    property is attached to.

    Changing the source while a drag is active will reset the sequence of drag events by
    sending a drag leave event followed by a drag enter event with the new source.
*/

QObject *QQuickDragAttached::source() const
{
    Q_D(const QQuickDragAttached);
    return d->source;
}

void QQuickDragAttached::setSource(QObject *item)
{
    Q_D(QQuickDragAttached);
    if (d->source != item) {
        d->source = item;
        if (d->active)
            d->restartDrag();
        emit sourceChanged();
    }
}

void QQuickDragAttached::resetSource()
{
    Q_D(QQuickDragAttached);
    if (d->source != d->attachedItem) {
        d->source = d->attachedItem;
        if (d->active)
            d->restartDrag();
        emit sourceChanged();
    }
}

/*!
    \qmlattachedproperty Object QtQuick::Drag::target

    While a drag is active this property holds the last object to accept an
    enter event from the dragged item, if the current drag position doesn't
    intersect any accepting targets it is null.

    When a drag is not active this property holds the object that accepted
    the drop event that ended the drag, if no object accepted the drop or
    the drag was canceled the target will then be null.
*/

QObject *QQuickDragAttached::target() const
{
    Q_D(const QQuickDragAttached);
    return d->target;
}

/*!
    \qmlattachedproperty point QtQuick::Drag::hotSpot

    This property holds the drag position relative to the top left of the item.

    By default this is (0, 0).

    Changes to hotSpot trigger a new drag move with the updated position.
*/

QPointF QQuickDragAttached::hotSpot() const
{
    Q_D(const QQuickDragAttached);
    return d->hotSpot;
}

void QQuickDragAttached::setHotSpot(const QPointF &hotSpot)
{
    Q_D(QQuickDragAttached);
    if (d->hotSpot != hotSpot) {
        d->hotSpot = hotSpot;

        if (d->active)
            d->updatePosition();

        emit hotSpotChanged();
    }
}

/*!
    \qmlattachedproperty url QtQuick::Drag::imageSource
    \since 5.8

    This property holds the URL of the image which will be used to represent
    the data during the drag and drop operation. Changing this property after
    the drag operation has started will have no effect.

    The example below uses an item's contents as a drag image:

    \snippet qml/externaldrag.qml 0

    \sa Item::grabToImage()
*/

QUrl QQuickDragAttached::imageSource() const
{
    Q_D(const QQuickDragAttached);
    return d->imageSource;
}

void QQuickDragAttached::setImageSource(const QUrl &url)
{
    Q_D(QQuickDragAttached);
    if (d->imageSource != url) {
        d->imageSource = url;

        if (url.isEmpty()) {
            d->pixmapLoader.clear();
        } else {
            d->loadPixmap();
        }

        Q_EMIT imageSourceChanged();
    }
}

/*!
    \qmlattachedproperty size QtQuick::Drag::imageSourceSize
    \since 6.8

    This property holds the size of the image that will be used to represent
    the data during the drag and drop operation. Changing this property after
    the drag operation has started will have no effect.

    This property sets the maximum number of pixels stored for the loaded
    image so that large images do not use more memory than necessary.
    See \l {QtQuick::Image::sourceSize}{Image.sourceSize} for more details.

    The example below shows an SVG image rendered at one size, and re-renders
    it at a different size for the drag image:

    \snippet qml/externalDragScaledImage.qml 0

    \sa imageSource, Item::grabToImage()
*/

QSize QQuickDragAttached::imageSourceSize() const
{
    Q_D(const QQuickDragAttached);
    int width = d->imageSourceSize.width();
    int height = d->imageSourceSize.height();
    // If width or height is invalid, check whether the size is valid from the loaded image.
    // If it ends up 0x0 though, leave it as an invalid QSize instead (-1 x -1).
    if (width == -1) {
        width = d->pixmapLoader.width();
        if (!width)
            width = -1;
    }
    if (height == -1) {
        height = d->pixmapLoader.height();
        if (!height)
            height = -1;
    }
    return QSize(width, height);
}

void QQuickDragAttached::setImageSourceSize(const QSize &size)
{
    Q_D(QQuickDragAttached);
    if (d->imageSourceSize != size) {
        d->imageSourceSize = size;

        if (!d->imageSource.isEmpty())
            d->loadPixmap();

        Q_EMIT imageSourceSizeChanged();
    }
}

/*!
    \qmlattachedproperty stringlist QtQuick::Drag::keys

    This property holds a list of keys that can be used by a DropArea to filter drag events.

    Changing the keys while a drag is active will reset the sequence of drag events by
    sending a drag leave event followed by a drag enter event with the new source.
*/

QStringList QQuickDragAttached::keys() const
{
    Q_D(const QQuickDragAttached);
    return d->keys;
}

void QQuickDragAttached::setKeys(const QStringList &keys)
{
    Q_D(QQuickDragAttached);
    if (d->keys != keys) {
        d->keys = keys;
        if (d->active)
            d->restartDrag();
        emit keysChanged();
    }
}

/*!
    \qmlattachedproperty var QtQuick::Drag::mimeData
    \since 5.2

    This property holds a map from mime type to data that is used during startDrag.
    The mime data needs to be of a type that matches the mime type (e.g. a string if
    the mime type is "text/plain", or an image if the mime type is "image/png"), or
    an \c ArrayBuffer with the data encoded according to the mime type.
*/

QVariantMap QQuickDragAttached::mimeData() const
{
    Q_D(const QQuickDragAttached);
    return d->externalMimeData;
}

void QQuickDragAttached::setMimeData(const QVariantMap &mimeData)
{
    Q_D(QQuickDragAttached);
    if (d->externalMimeData != mimeData) {
        d->externalMimeData = mimeData;
        emit mimeDataChanged();
    }
}

/*!
    \qmlattachedproperty flags QtQuick::Drag::supportedActions

    This property holds return values of Drag.drop() supported by the drag source.

    Changing the supportedActions while a drag is active will reset the sequence of drag
    events by sending a drag leave event followed by a drag enter event with the new source.
*/

Qt::DropActions QQuickDragAttached::supportedActions() const
{
    Q_D(const QQuickDragAttached);
    return d->supportedActions;
}

void QQuickDragAttached::setSupportedActions(Qt::DropActions actions)
{
    Q_D(QQuickDragAttached);
    if (d->supportedActions != actions) {
        d->supportedActions = actions;
        if (d->active)
            d->restartDrag();
        emit supportedActionsChanged();
    }
}

/*!
    \qmlattachedproperty enumeration QtQuick::Drag::proposedAction

    This property holds an action that is recommended by the drag source as a
    return value from Drag.drop().

    Changes to proposedAction will trigger a move event with the updated proposal.
*/

Qt::DropAction QQuickDragAttached::proposedAction() const
{
    Q_D(const QQuickDragAttached);
    return d->proposedAction;
}

void QQuickDragAttached::setProposedAction(Qt::DropAction action)
{
    Q_D(QQuickDragAttached);
    if (d->proposedAction != action) {
        d->proposedAction = action;
        // The proposed action shouldn't affect whether a drag is accepted
        // so leave/enter events are excessive, but the target should still
        // updated.
        if (d->active)
            d->updatePosition();
        emit proposedActionChanged();
    }
}

/*!
    \qmlattachedproperty enumeration QtQuick::Drag::dragType
    \since 5.2

    This property indicates whether to automatically start drags, do nothing, or
    to use backwards compatible internal drags. The default is to use backwards
    compatible internal drags.

    A drag can also be started manually using \l startDrag.

    \value Drag.None        do not start drags automatically
    \value Drag.Automatic   start drags automatically
    \value Drag.Internal    (default) start backwards compatible drags automatically

    When using \c Drag.Automatic you should also define \l mimeData and bind the
    \l active property to the active property of MouseArea : \l {MouseArea::drag.active}
*/

QQuickDrag::DragType QQuickDragAttached::dragType() const
{
    Q_D(const QQuickDragAttached);
    return d->dragType;
}

void QQuickDragAttached::setDragType(QQuickDrag::DragType dragType)
{
    Q_D(QQuickDragAttached);
    if (d->dragType != dragType) {
        d->dragType = dragType;
        emit dragTypeChanged();
    }
}

void QQuickDragAttachedPrivate::start(Qt::DropActions supportedActions)
{
    Q_Q(QQuickDragAttached);
    Q_ASSERT(!active);

    if (!mimeData)
        mimeData = new QQuickDragMimeData;
    if (!listening) {
        QQuickItemPrivate::get(attachedItem)->addItemChangeListener(
                this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Parent);
        listening = true;
    }

    mimeData->m_supportedActions = supportedActions;
    active = true;
    itemMoved = false;
    dragRestarted = false;

    deliverEnterEvent();

    if (target != dragGrabber.target()) {
        target = dragGrabber.target();
        emit q->targetChanged();
    }

    emit q->activeChanged();
}

/*!
    \qmlattachedmethod void QtQuick::Drag::start(flags supportedActions)

    Starts sending drag events. Used for starting old-style internal drags. \l startDrag is the
    new-style, preferred method of starting drags.

    The optional \a supportedActions argument can be used to override the \l supportedActions
    property for the started sequence.
*/

void QQuickDragAttached::start(QQmlV4FunctionPtr args)
{
    Q_D(QQuickDragAttached);
    if (d->inEvent) {
        qmlWarning(this) << "start() cannot be called from within a drag event handler";
        return;
    }

    if (d->active)
        cancel();

    d->overrideActions = false;
    Qt::DropActions supportedActions = d->supportedActions;
    // check arguments for supportedActions, maybe data?
    if (args->length() >= 1) {
        QV4::Scope scope(args->v4engine());
        QV4::ScopedValue v(scope, (*args)[0]);
        if (v->isInt32()) {
            supportedActions = Qt::DropActions(v->integerValue());
            d->overrideActions = true;
        }
    }

    d->start(supportedActions);
}

/*!
    \qmlattachedmethod enumeration QtQuick::Drag::drop()

    Ends a drag sequence by sending a drop event to the target item.

    Returns the action accepted by the target item.  If the target item or a parent doesn't accept
    the drop event then Qt.IgnoreAction will be returned.

    The returned drop action may be one of:

    \value Qt.CopyAction    Copy the data to the target
    \value Qt.MoveAction    Move the data from the source to the target
    \value Qt.LinkAction    Create a link from the source to the target.
    \value Qt.IgnoreAction  Ignore the action (do nothing with the data).
*/

int QQuickDragAttached::drop()
{
    Q_D(QQuickDragAttached);
    Qt::DropAction acceptedAction = Qt::IgnoreAction;

    if (d->inEvent) {
        qmlWarning(this) << "drop() cannot be called from within a drag event handler";
        return acceptedAction;
    }

    if (d->itemMoved)
        d->deliverMoveEvent();

    if (!d->active)
        return acceptedAction;
    d->active = false;

    QObject *target = nullptr;

    if (d->window) {
        QPoint scenePos = d->attachedItem->mapToScene(d->hotSpot).toPoint();

        QDropEvent event(
                scenePos, d->mimeData->m_supportedActions, d->mimeData, Qt::NoButton, Qt::NoModifier);
        QQuickDropEventEx::setProposedAction(&event, d->proposedAction);
        d->deliverEvent(d->window, &event);

        if (event.isAccepted()) {
            acceptedAction = event.dropAction();
            target = d->dragGrabber.target();
        }
    }

    if (d->target != target) {
        d->target = target;
        emit targetChanged();
    }

    emit activeChanged();
    return acceptedAction;
}

/*!
    \qmlattachedmethod void QtQuick::Drag::cancel()

    Ends a drag sequence.
*/

void QQuickDragAttached::cancel()
{
    Q_D(QQuickDragAttached);

    if (d->inEvent) {
        qmlWarning(this) << "cancel() cannot be called from within a drag event handler";
        return;
    }

    if (!d->active)
        return;
    d->active = false;
    d->deliverLeaveEvent();

    if (d->target) {
        d->target = nullptr;
        emit targetChanged();
    }

    emit activeChanged();
}

/*!
    \qmlattachedsignal QtQuick::Drag::dragStarted()

    This signal is emitted when a drag is started with the \l startDrag() method
    or when it is started automatically using the \l dragType property.
 */

/*!
    \qmlattachedsignal QtQuick::Drag::dragFinished(DropAction dropAction)

    This signal is emitted when a drag finishes and the drag was started with the
    \l startDrag() method or started automatically using the \l dragType property.

    \a dropAction holds the action accepted by the target item.

    \sa drop()
 */

QMimeData *QQuickDragAttachedPrivate::createMimeData() const
{
    Q_Q(const QQuickDragAttached);
    QMimeData *mimeData = new QMimeData();

    for (const auto [mimeType, value] : externalMimeData.asKeyValueRange()) {
        switch (value.typeId()) {
        case QMetaType::QByteArray:
            // byte array assumed to already be correctly encoded
            mimeData->setData(mimeType, value.toByteArray());
            break;
        case QMetaType::QString: {
            const QString text = value.toString();
            if (mimeType == u"text/plain"_s) {
                mimeData->setText(text);
            } else if (mimeType == u"text/html"_s) {
                mimeData->setHtml(text);
            } else if (mimeType == u"text/uri-list"_s) {
                QList<QUrl> urls;
                // parse and split according to RFC2483
                const auto lines = text.split(u"\r\n"_s, Qt::SkipEmptyParts);
                for (const auto &line : lines) {
                    const QUrl url(line);
                    if (url.isValid())
                        urls.push_back(url);
                    else
                        qmlWarning(q) << line << " is not a valid URI";

                }
                mimeData->setUrls(urls);
            } else if (mimeType.startsWith(u"text/"_s)) {
                if (qsizetype charsetIdx = mimeType.lastIndexOf(u";charset="_s); charsetIdx != -1) {
                    charsetIdx += sizeof(";charset=") - 1;
                    const QByteArray encoding = mimeType.mid(charsetIdx).toUtf8();
                    QStringEncoder encoder(encoding);
                    if (encoder.isValid())
                        mimeData->setData(mimeType, encoder.encode(text));
                    else
                        qmlWarning(q) << "Don't know how to encode text as " << mimeType;
                } else {
                    mimeData->setData(mimeType, text.toUtf8());
                }
            } else {
                mimeData->setData(mimeType, text.toUtf8());
            }
            break;
        }
        case QMetaType::QVariantList:
        case QMetaType::QStringList:
            if (mimeType == u"text/uri-list"_s) {
                const QVariantList values = value.toList();
                QList<QUrl> urls;
                urls.reserve(values.size());
                bool error = false;
                for (qsizetype index = 0; index < values.size(); ++index) {
                    const QUrl url = values.at(index).value<QUrl>();
                    if (url.isValid()) {
                        urls += url;
                    } else {
                        error = true;
                        qmlWarning(q) << "Value '" << values.at(index) << "' at index " << index
                                      << " is not a valid URI";
                    }
                }
                if (!error)
                    mimeData->setUrls(urls);
            }
            break;
        case QMetaType::QColor:
            if (mimeType == u"application/x-color"_s)
                mimeData->setColorData(value);
            break;
        case QMetaType::QImage:
            if (const QByteArray mimeTypeUtf8 = mimeType.toUtf8();
                QImageWriter::supportedMimeTypes().contains(mimeTypeUtf8)) {
                const auto imageFormats = QImageWriter::imageFormatsForMimeType(mimeTypeUtf8);
                if (imageFormats.isEmpty()) { // shouldn't happen, but we can fall back
                    mimeData->setImageData(value);
                    break;
                }
                const QImage image = value.value<QImage>();
                QByteArray bytes;
                {
                    QBuffer buffer(&bytes);
                    QImageWriter encoder(&buffer, imageFormats.first());
                    encoder.write(image);
                }
                mimeData->setData(mimeType, bytes);
                break;
            }
            Q_FALLTHROUGH();
        default:
            qmlWarning(q) << "Don't know how to encode variant of type " << value.metaType()
                          << " as mime type " << mimeType;
            // compatibility with pre-6.5 - probably a bad idea
            mimeData->setData(mimeType, value.toString().toUtf8());
            break;
        }
    }

    return mimeData;
}

void QQuickDragAttachedPrivate::loadPixmap()
{
    Q_Q(QQuickDragAttached);

    QUrl loadUrl = imageSource;
    const QQmlContext *context = qmlContext(q->parent());
    if (context)
        loadUrl = context->resolvedUrl(imageSource);
    pixmapLoader.load(context ? context->engine() : nullptr, loadUrl, QRect(), q->imageSourceSize());
}

Qt::DropAction QQuickDragAttachedPrivate::startDrag(Qt::DropActions supportedActions)
{
    Q_Q(QQuickDragAttached);

    QDrag *drag = new QDrag(source ? source : q);

    drag->setMimeData(createMimeData());
    if (pixmapLoader.isReady())
        drag->setPixmap(QPixmap::fromImage(pixmapLoader.image()));

    drag->setHotSpot(hotSpot.toPoint());
    emit q->dragStarted();

    Qt::DropAction dropAction = drag->exec(supportedActions);

    if (!QGuiApplicationPrivate::platformIntegration()->drag()->ownsDragObject())
        drag->deleteLater();

    deliverLeaveEvent();

    if (target) {
        target = nullptr;
        emit q->targetChanged();
    }

    emit q->dragFinished(dropAction);

    active = false;
    emit q->activeChanged();

    return dropAction;
}


/*!
    \qmlattachedmethod void QtQuick::Drag::startDrag(flags supportedActions)

    Starts sending drag events.

    The optional \a supportedActions argument can be used to override the \l supportedActions
    property for the started sequence.
*/

void QQuickDragAttached::startDrag(QQmlV4FunctionPtr args)
{
    Q_D(QQuickDragAttached);

    if (d->inEvent) {
        qmlWarning(this) << "startDrag() cannot be called from within a drag event handler";
        return;
    }

    if (!d->active) {
        qmlWarning(this) << "startDrag() drag must be active";
        return;
    }

    Qt::DropActions supportedActions = d->supportedActions;

    // check arguments for supportedActions
    if (args->length() >= 1) {
        QV4::Scope scope(args->v4engine());
        QV4::ScopedValue v(scope, (*args)[0]);
        if (v->isInt32()) {
            supportedActions = Qt::DropActions(v->integerValue());
        }
    }

    Qt::DropAction dropAction = d->startDrag(supportedActions);

    args->setReturnValue(QV4::Encode((int)dropAction));
}

QQuickDrag::QQuickDrag(QObject *parent)
: QObject(parent), _target(nullptr), _axis(XAndYAxis), _xmin(-FLT_MAX),
_xmax(FLT_MAX), _ymin(-FLT_MAX), _ymax(FLT_MAX), _active(false), _filterChildren(false),
  _smoothed(true), _threshold(QGuiApplication::styleHints()->startDragDistance())
{
}

QQuickDrag::~QQuickDrag()
{
}

QQuickItem *QQuickDrag::target() const
{
    return _target;
}

void QQuickDrag::setTarget(QQuickItem *t)
{
    if (_target == t)
        return;
    _target = t;
    emit targetChanged();
}

void QQuickDrag::resetTarget()
{
    if (_target == nullptr)
        return;
    _target = nullptr;
    emit targetChanged();
}

QQuickDrag::Axis QQuickDrag::axis() const
{
    return _axis;
}

void QQuickDrag::setAxis(QQuickDrag::Axis a)
{
    if (_axis == a)
        return;
    _axis = a;
    emit axisChanged();
}

qreal QQuickDrag::xmin() const
{
    return _xmin;
}

void QQuickDrag::setXmin(qreal m)
{
    if (_xmin == m)
        return;
    _xmin = m;
    emit minimumXChanged();
}

qreal QQuickDrag::xmax() const
{
    return _xmax;
}

void QQuickDrag::setXmax(qreal m)
{
    if (_xmax == m)
        return;
    _xmax = m;
    emit maximumXChanged();
}

qreal QQuickDrag::ymin() const
{
    return _ymin;
}

void QQuickDrag::setYmin(qreal m)
{
    if (_ymin == m)
        return;
    _ymin = m;
    emit minimumYChanged();
}

qreal QQuickDrag::ymax() const
{
    return _ymax;
}

void QQuickDrag::setYmax(qreal m)
{
    if (_ymax == m)
        return;
    _ymax = m;
    emit maximumYChanged();
}

bool QQuickDrag::smoothed() const
{
    return _smoothed;
}

void QQuickDrag::setSmoothed(bool smooth)
{
    if (_smoothed != smooth) {
        _smoothed = smooth;
        emit smoothedChanged();
    }
}

qreal QQuickDrag::threshold() const
{
    return _threshold;
}

void QQuickDrag::setThreshold(qreal value)
{
    if (_threshold != value) {
        _threshold = value;
        emit thresholdChanged();
    }
}

void QQuickDrag::resetThreshold()
{
    setThreshold(QGuiApplication::styleHints()->startDragDistance());
}

bool QQuickDrag::active() const
{
    return _active;
}

void QQuickDrag::setActive(bool drag)
{
    if (_active == drag)
        return;
    _active = drag;
    emit activeChanged();
}

bool QQuickDrag::filterChildren() const
{
    return _filterChildren;
}

void QQuickDrag::setFilterChildren(bool filter)
{
    if (_filterChildren == filter)
        return;
    _filterChildren = filter;
    emit filterChildrenChanged();
}

QQuickDragAttached *QQuickDrag::qmlAttachedProperties(QObject *obj)
{
    return new QQuickDragAttached(obj);
}

QT_END_NAMESPACE

#include "moc_qquickdrag_p.cpp"
