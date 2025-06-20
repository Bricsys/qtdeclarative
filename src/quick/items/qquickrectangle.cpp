// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrectangle_p.h"
#include "qquickrectangle_p_p.h"

#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qsgcontext_p.h>
#include <private/qsgadaptationlayer_p.h>

#include <private/qqmlmetatype_p.h>

#include <QtGui/qpixmapcache.h>
#include <QtCore/qmath.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

// XXX todo - should we change rectangle to draw entirely within its width/height?
/*!
    \internal
    \class QQuickPen
    \brief For specifying a pen used for drawing rectangle borders on a QQuickView

    By default, the pen is invalid and nothing is drawn. You must either set a color (then the default
    width is 1) or a width (then the default color is black).

    A width of 1 indicates is a single-pixel line on the border of the item being painted.

    Example:
    \qml
    Rectangle {
        border.width: 2
        border.color: "red"
    }
    \endqml
*/

QQuickPen::QQuickPen(QObject *parent)
    : QObject(parent)
    , m_width(1)
    , m_color(Qt::black)
    , m_aligned(true)
    , m_valid(false)
{
}

qreal QQuickPen::width() const
{
    return m_width;
}

void QQuickPen::setWidth(qreal w)
{
    if (m_width == w && m_valid)
        return;

    m_width = w;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    static_cast<QQuickItem*>(parent())->update();
    emit widthChanged();
}

QColor QQuickPen::color() const
{
    return m_color;
}

void QQuickPen::setColor(const QColor &c)
{
    m_color = c;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    static_cast<QQuickItem*>(parent())->update();
    emit colorChanged();
}

bool QQuickPen::pixelAligned() const
{
    return m_aligned;
}

void QQuickPen::setPixelAligned(bool aligned)
{
    if (aligned == m_aligned)
        return;
    m_aligned = aligned;
    m_valid = m_color.alpha() && (qRound(m_width) >= 1 || (!m_aligned && m_width > 0));
    static_cast<QQuickItem*>(parent())->update();
    emit pixelAlignedChanged();
}

bool QQuickPen::isValid() const
{
    return m_valid;
}

/*!
    \qmltype GradientStop
    \nativetype QQuickGradientStop
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-utility
    \brief Defines the color at a position in a Gradient.

    \sa Gradient
*/

/*!
    \qmlproperty real QtQuick::GradientStop::position
    \qmlproperty color QtQuick::GradientStop::color

    The position and color properties describe the color used at a given
    position in a gradient, as represented by a gradient stop.

    The default position is 0.0; the default color is black.

    \sa Gradient
*/
QQuickGradientStop::QQuickGradientStop(QObject *parent)
    : QObject(parent)
{
}

qreal QQuickGradientStop::position() const
{
    return m_position;
}

void QQuickGradientStop::setPosition(qreal position)
{
    m_position = position; updateGradient();
}

QColor QQuickGradientStop::color() const
{
    return m_color;
}

void QQuickGradientStop::setColor(const QColor &color)
{
    m_color = color; updateGradient();
}

void QQuickGradientStop::updateGradient()
{
    if (QQuickGradient *grad = qobject_cast<QQuickGradient*>(parent()))
        grad->doUpdate();
}

/*!
    \qmltype Gradient
    \nativetype QQuickGradient
    \inqmlmodule QtQuick
    \ingroup qtquick-visual-utility
    \brief Defines a gradient fill.

    A gradient is defined by two or more colors, which will be blended seamlessly.

    The colors are specified as a set of GradientStop child items, each of
    which defines a position on the gradient from 0.0 to 1.0 and a color.
    The position of each GradientStop is defined by setting its
    \l{GradientStop::}{position} property; its color is defined using its
    \l{GradientStop::}{color} property.

    A gradient without any gradient stops is rendered as a solid white fill.

    Note that this item is not a visual representation of a gradient. To display a
    gradient, use a visual item (like \l Rectangle) which supports the use
    of gradients.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage qml-gradient.png
    \enddiv

    The following example declares a \l Rectangle item with a gradient starting
    with red, blending to yellow at one third of the height of the rectangle,
    and ending with green:

    \snippet qml/gradient.qml code

    \clearfloat
    \section1 Performance and Limitations

    Calculating gradients can be computationally expensive compared to the use
    of solid color fills or images. Consider using gradients for static items
    in a user interface.

    Since Qt 5.12, vertical and horizontal linear gradients can be applied to items.
    If you need to apply angled gradients, a combination of rotation and clipping
    can be applied to the relevant items. Alternatively, consider using
    QtQuick.Shapes::LinearGradient or QtGraphicalEffects::LinearGradient. These
    approaches can all introduce additional performance requirements for your application.

    The use of animations involving gradient stops may not give the desired
    result. An alternative way to animate gradients is to use pre-generated
    images or SVG drawings containing gradients.

    \sa GradientStop
*/

/*!
    \qmlproperty list<GradientStop> QtQuick::Gradient::stops
    \qmldefault

    This property holds the gradient stops describing the gradient.

    By default, this property contains an empty list.

    To set the gradient stops, define them as children of the Gradient.
*/
QQuickGradient::QQuickGradient(QObject *parent)
: QObject(parent)
{
}

QQuickGradient::~QQuickGradient()
{
}

QQmlListProperty<QQuickGradientStop> QQuickGradient::stops()
{
    return QQmlListProperty<QQuickGradientStop>(this, &m_stops);
}

/*!
    \qmlproperty enumeration QtQuick::Gradient::orientation
    \since 5.12

    Set this property to define the direction of the gradient.

    \value Gradient.Vertical    a vertical gradient
    \value Gradient.Horizontal  a horizontal gradient

    The default is Gradient.Vertical.
*/
void QQuickGradient::setOrientation(Orientation orientation)
{
    if (m_orientation == orientation)
        return;

    m_orientation = orientation;
    emit orientationChanged();
    emit updated();
}

QGradientStops QQuickGradient::gradientStops() const
{
    QGradientStops stops;
    for (int i = 0; i < m_stops.size(); ++i){
        int j = 0;
        while (j < stops.size() && stops.at(j).first < m_stops[i]->position())
            j++;
        stops.insert(j, QGradientStop(m_stops.at(i)->position(), m_stops.at(i)->color()));
    }
    return stops;
}

void QQuickGradient::doUpdate()
{
    emit updated();
}

int QQuickRectanglePrivate::doUpdateSlotIdx = -1;

void QQuickRectanglePrivate::maybeSetImplicitAntialiasing()
{
    bool implicitAA = (radius > 0);
    if (extraRectangle.isAllocated() && !implicitAA) {
        const auto &extra = extraRectangle.value();
        implicitAA = (extra.isTopLeftRadiusSet && extra.topLeftRadius > 0)
            || (extra.isTopRightRadiusSet && extra.topRightRadius > 0)
            || (extra.isBottomLeftRadiusSet && extra.bottomLeftRadius > 0)
            || (extra.isBottomRightRadiusSet && extra.bottomRightRadius > 0);
    }
    setImplicitAntialiasing(implicitAA);
}
/*!
    \qmltype Rectangle
    \nativetype QQuickRectangle
    \inqmlmodule QtQuick
    \inherits Item
    \ingroup qtquick-visual
    \brief Paints a filled rectangle with an optional border.

    Rectangle items are used to fill areas with solid color or gradients, and/or
    to provide a rectangular border.

    \section1 Appearance

    Each Rectangle item is painted using either a solid fill color, specified using
    the \l color property, or a gradient, defined using a Gradient type and set
    using the \l gradient property. If both a color and a gradient are specified,
    the gradient is used.

    You can add an optional border to a rectangle with its own color and thickness
    by setting the \l border.color and \l border.width properties.  Set the color
    to "transparent" to paint a border without a fill color.

    You can also create rounded rectangles using the \l radius property. Since this
    introduces curved edges to the corners of a rectangle, it may be appropriate to
    set the \l Item::antialiasing property to improve its appearance.  To set the
    radii individually for different corners, you can use the properties
    \l topLeftRadius, \l topRightRadius, \l bottomLeftRadius and
    \l bottomRightRadius.

    \section1 Example Usage

    \div {class="float-right"}
    \inlineimage declarative-rect.png
    \enddiv

    The following example shows the effects of some of the common properties on a
    Rectangle item, which in this case is used to create a square:

    \snippet qml/rectangle/rectangle.qml document

    \clearfloat
    \section1 Performance

    Using the \l Item::antialiasing property improves the appearance of a rounded rectangle at
    the cost of rendering performance. You should consider unsetting this property
    for rectangles in motion, and only set it when they are stationary.

    \sa Image
*/

QQuickRectangle::QQuickRectangle(QQuickItem *parent)
: QQuickItem(*(new QQuickRectanglePrivate), parent)
{
    setFlag(ItemHasContents);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setAcceptTouchEvents(false);
#endif
}

void QQuickRectangle::doUpdate()
{
    update();
}

/*!
    \qmlproperty bool QtQuick::Rectangle::antialiasing

    Used to decide if the Rectangle should use antialiasing or not.
    \l {Antialiasing} provides information on the performance implications
    of this property.

    The default is true for Rectangles with a radius, and false otherwise.
*/

/*!
    \qmlpropertygroup QtQuick::Rectangle::border
    \qmlproperty int QtQuick::Rectangle::border.width
    \qmlproperty color QtQuick::Rectangle::border.color
    \qmlproperty bool QtQuick::Rectangle::border.pixelAligned

    The width and color used to draw the border of the rectangle.

    A width of 1 creates a thin line. For no line, use a width of 0 or a transparent color.

    \note The width of the rectangle's border does not affect the geometry of the
    rectangle itself or its position relative to other items if anchors are used.

    The border is rendered within the rectangle's boundaries.

    If \c pixelAligned is \c true (the default), the rendered border width is rounded to a whole
    number of pixels, after device pixel ratio scaling. Setting \c pixelAligned to \c false will
    allow fractional border widths, which may be desirable when \c antialiasing is enabled.
*/
QQuickPen *QQuickRectangle::border()
{
    Q_D(QQuickRectangle);
    if (!d->pen) {
        d->pen = new QQuickPen;
        QQml_setParent_noEvent(d->pen, this);
    }
    return d->pen;
}

/*!
    \qmlproperty var QtQuick::Rectangle::gradient

    The gradient to use to fill the rectangle.

    This property allows for the construction of simple vertical or horizontal gradients.
    Other gradients may be formed by adding rotation to the rectangle.

    \div {class="float-left"}
    \inlineimage declarative-rect_gradient.png
    \enddiv

    \snippet qml/rectangle/rectangle-gradient.qml rectangles
    \clearfloat

    The property also accepts gradient presets from QGradient::Preset. Note however
    that due to Rectangle only supporting simple vertical or horizontal gradients,
    any preset with an unsupported angle will revert to the closest representation.

    \snippet qml/rectangle/rectangle-gradient.qml presets
    \clearfloat

    If both a gradient and a color are specified, the gradient will be used.

    \sa Gradient, color
*/
QJSValue QQuickRectangle::gradient() const
{
    Q_D(const QQuickRectangle);
    return d->gradient;
}

void QQuickRectangle::setGradient(const QJSValue &gradient)
{
    Q_D(QQuickRectangle);
    if (d->gradient.equals(gradient))
        return;

    static int updatedSignalIdx = QMetaMethod::fromSignal(&QQuickGradient::updated).methodIndex();
    if (d->doUpdateSlotIdx < 0)
        d->doUpdateSlotIdx = QQuickRectangle::staticMetaObject.indexOfSlot("doUpdate()");

    if (auto oldGradient = qobject_cast<QQuickGradient*>(d->gradient.toQObject()))
        QMetaObject::disconnect(oldGradient, updatedSignalIdx, this, d->doUpdateSlotIdx);

    if (gradient.isQObject()) {
        if (auto newGradient = qobject_cast<QQuickGradient*>(gradient.toQObject())) {
            d->gradient = gradient;
            QMetaObject::connect(newGradient, updatedSignalIdx, this, d->doUpdateSlotIdx);
        } else {
            qmlWarning(this) << "Can't assign "
                << QQmlMetaType::prettyTypeName(gradient.toQObject()) << " to gradient property";
            d->gradient = QJSValue();
        }
    } else if (gradient.isNumber() || gradient.isString()) {
        static const QMetaEnum gradientPresetMetaEnum = QMetaEnum::fromType<QGradient::Preset>();
        Q_ASSERT(gradientPresetMetaEnum.isValid());

        QGradient result;

        // This code could simply use gradient.toVariant().convert<QGradient::Preset>(),
        // but QTBUG-76377 prevents us from doing error checks. So we need to
        // do them manually. Also, NumPresets cannot be used.

        if (gradient.isNumber()) {
            const auto preset = QGradient::Preset(gradient.toInt());
            if (preset != QGradient::NumPresets && gradientPresetMetaEnum.valueToKey(preset))
                result = QGradient(preset);
        } else if (gradient.isString()) {
            const auto presetName = gradient.toString();
            if (presetName != QLatin1String("NumPresets")) {
                bool ok;
                const auto presetInt = gradientPresetMetaEnum.keyToValue(qPrintable(presetName), &ok);
                if (ok)
                    result = QGradient(QGradient::Preset(presetInt));
            }
        }

        if (result.type() != QGradient::NoGradient) {
            d->gradient = gradient;
        } else {
            qmlWarning(this) << "No such gradient preset '" << gradient.toString() << "'";
            d->gradient = QJSValue();
        }
    } else if (gradient.isNull() || gradient.isUndefined()) {
        d->gradient = gradient;
    } else {
        qmlWarning(this) << "Unknown gradient type. Expected int, string, or Gradient";
        d->gradient = QJSValue();
    }

    update();
}

void QQuickRectangle::resetGradient()
{
    setGradient(QJSValue());
}

/*!
    \qmlproperty real QtQuick::Rectangle::radius
    This property holds the corner radius used to draw a rounded rectangle.

    If radius is non-zero, the rectangle will be painted as a rounded rectangle,
    otherwise it will be painted as a normal rectangle. Individual corner radii
    can be set as well (see below). These values will override \l radius. If
    they are unset (by setting them to \c undefined), \l radius will be used instead.

    \sa topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius
*/
qreal QQuickRectangle::radius() const
{
    Q_D(const QQuickRectangle);
    return d->radius;
}

void QQuickRectangle::setRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->radius == radius)
        return;

    d->radius = radius;
    d->maybeSetImplicitAntialiasing();

    update();
    emit radiusChanged();

    if (d->extraRectangle.isAllocated()) {
        if (!d->extraRectangle->isTopLeftRadiusSet)
            emit topLeftRadiusChanged();
        if (!d->extraRectangle->isTopRightRadiusSet)
            emit topRightRadiusChanged();
        if (!d->extraRectangle->isBottomLeftRadiusSet)
            emit bottomLeftRadiusChanged();
        if (!d->extraRectangle->isBottomRightRadiusSet)
            emit bottomRightRadiusChanged();
    } else {
        emit topLeftRadiusChanged();
        emit topRightRadiusChanged();
        emit bottomLeftRadiusChanged();
        emit bottomRightRadiusChanged();
    }
}

/*!
    \since 6.7
    \qmlproperty real QtQuick::Rectangle::topLeftRadius
    This property holds the radius used to draw the top left corner.

    If \l topLeftRadius is not set, \l radius will be used instead.
    If \l topLeftRadius is zero, the corner will be sharp.

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa radius, topRightRadius, bottomLeftRadius, bottomRightRadius
*/
qreal QQuickRectangle::topLeftRadius() const
{
    Q_D(const QQuickRectangle);
    if (d->extraRectangle.isAllocated() && d->extraRectangle->isTopLeftRadiusSet)
        return d->extraRectangle->topLeftRadius;
    return d->radius;
}

void QQuickRectangle::setTopLeftRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->extraRectangle.isAllocated()
        && d->extraRectangle->topLeftRadius == radius
        && d->extraRectangle->isTopLeftRadiusSet) {
        return;
    }

    d->extraRectangle.value().topLeftRadius = radius;
    d->extraRectangle.value().isTopLeftRadiusSet = true;
    d->maybeSetImplicitAntialiasing();

    update();
    emit topLeftRadiusChanged();
}

void QQuickRectangle::resetTopLeftRadius()
{
    Q_D(QQuickRectangle);
    if (!d->extraRectangle.isAllocated())
        return;
    if (!d->extraRectangle->isTopLeftRadiusSet)
        return;

    d->extraRectangle->isTopLeftRadiusSet = false;
    d->maybeSetImplicitAntialiasing();

    update();
    emit topLeftRadiusChanged();
}

/*!
    \since 6.7
    \qmlproperty real QtQuick::Rectangle::topRightRadius
    This property holds the radius used to draw the top right corner.

    If \l topRightRadius is not set, \l radius will be used instead.
    If \l topRightRadius is zero, the corner will be sharp.

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa radius, topLeftRadius, bottomLeftRadius, bottomRightRadius
*/
qreal QQuickRectangle::topRightRadius() const
{
    Q_D(const QQuickRectangle);
    if (d->extraRectangle.isAllocated() && d->extraRectangle->isTopRightRadiusSet)
        return d->extraRectangle->topRightRadius;
    return d->radius;
}

void QQuickRectangle::setTopRightRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->extraRectangle.isAllocated()
        && d->extraRectangle->topRightRadius == radius
        && d->extraRectangle->isTopRightRadiusSet) {
        return;
    }

    d->extraRectangle.value().topRightRadius = radius;
    d->extraRectangle.value().isTopRightRadiusSet = true;
    d->maybeSetImplicitAntialiasing();

    update();
    emit topRightRadiusChanged();
}

void QQuickRectangle::resetTopRightRadius()
{
    Q_D(QQuickRectangle);
    if (!d->extraRectangle.isAllocated())
        return;
    if (!d->extraRectangle.value().isTopRightRadiusSet)
        return;

    d->extraRectangle->isTopRightRadiusSet = false;
    d->maybeSetImplicitAntialiasing();

    update();
    emit topRightRadiusChanged();
}

/*!
    \since 6.7
    \qmlproperty real QtQuick::Rectangle::bottomLeftRadius
    This property holds the radius used to draw the bottom left corner.

    If \l bottomLeftRadius is not set, \l radius will be used instead.
    If \l bottomLeftRadius is zero, the corner will be sharp.

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa radius, topLeftRadius, topRightRadius, bottomRightRadius
*/
qreal QQuickRectangle::bottomLeftRadius() const
{
    Q_D(const QQuickRectangle);
    if (d->extraRectangle.isAllocated() && d->extraRectangle->isBottomLeftRadiusSet)
        return d->extraRectangle->bottomLeftRadius;
    return d->radius;
}

void QQuickRectangle::setBottomLeftRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->extraRectangle.isAllocated()
        && d->extraRectangle->bottomLeftRadius == radius
        && d->extraRectangle->isBottomLeftRadiusSet) {
        return;
    }

    d->extraRectangle.value().bottomLeftRadius = radius;
    d->extraRectangle.value().isBottomLeftRadiusSet = true;
    d->maybeSetImplicitAntialiasing();

    update();
    emit bottomLeftRadiusChanged();
}

void QQuickRectangle::resetBottomLeftRadius()
{
    Q_D(QQuickRectangle);
    if (!d->extraRectangle.isAllocated())
        return;
    if (!d->extraRectangle.value().isBottomLeftRadiusSet)
        return;

    d->extraRectangle->isBottomLeftRadiusSet = false;
    d->maybeSetImplicitAntialiasing();

    update();
    emit bottomLeftRadiusChanged();
}

/*!
    \since 6.7
    \qmlproperty real QtQuick::Rectangle::bottomRightRadius
    This property holds the radius used to draw the bottom right corner.

    If \l bottomRightRadius is not set, \l radius will be used instead.
    If \l bottomRightRadius is zero, the corner will be sharp.

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.

    \sa radius, topLeftRadius, topRightRadius, bottomLeftRadius
*/
qreal QQuickRectangle::bottomRightRadius() const
{
    Q_D(const QQuickRectangle);
    if (d->extraRectangle.isAllocated() && d->extraRectangle->isBottomRightRadiusSet)
        return d->extraRectangle->bottomRightRadius;
    return d->radius;
}

void QQuickRectangle::setBottomRightRadius(qreal radius)
{
    Q_D(QQuickRectangle);
    if (d->extraRectangle.isAllocated()
        && d->extraRectangle->bottomRightRadius == radius
        && d->extraRectangle->isBottomRightRadiusSet) {
        return;
    }

    d->extraRectangle.value().bottomRightRadius = radius;
    d->extraRectangle.value().isBottomRightRadiusSet = true;
    d->maybeSetImplicitAntialiasing();

    update();
    emit bottomRightRadiusChanged();
}

void QQuickRectangle::resetBottomRightRadius()
{
    Q_D(QQuickRectangle);
    if (!d->extraRectangle.isAllocated())
        return;
    if (!d->extraRectangle.value().isBottomRightRadiusSet)
        return;

    d->extraRectangle->isBottomRightRadiusSet = false;
    d->maybeSetImplicitAntialiasing();

    update();
    emit bottomRightRadiusChanged();
}

/*!
    \qmlproperty color QtQuick::Rectangle::color
    This property holds the color used to fill the rectangle.

    The default color is white.

    \div {class="float-right"}
    \inlineimage rect-color.png
    \enddiv

    The following example shows rectangles with colors specified
    using hexadecimal and named color notation:

    \snippet qml/rectangle/rectangle-colors.qml rectangles

    \clearfloat
    If both a gradient and a color are specified, the gradient will be used.

    \sa gradient
*/
QColor QQuickRectangle::color() const
{
    Q_D(const QQuickRectangle);
    return d->color;
}

void QQuickRectangle::setColor(const QColor &c)
{
    Q_D(QQuickRectangle);
    if (d->color == c)
        return;

    d->color = c;
    update();
    emit colorChanged();
}

QSGNode *QQuickRectangle::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data)
{
    Q_UNUSED(data);
    Q_D(QQuickRectangle);

    if (width() <= 0 || height() <= 0
            || (d->color.alpha() == 0 && (!d->pen || d->pen->width() == 0 || d->pen->color().alpha() == 0))) {
        delete oldNode;
        return nullptr;
    }

    QSGInternalRectangleNode *rectangle = static_cast<QSGInternalRectangleNode *>(oldNode);
    if (!rectangle) rectangle = d->sceneGraphContext()->createInternalRectangleNode();

    rectangle->setRect(QRectF(0, 0, width(), height()));
    rectangle->setColor(d->color);

    if (d->pen && d->pen->isValid()) {
        rectangle->setPenColor(d->pen->color());
        qreal penWidth = d->pen->width();
        if (d->pen->pixelAligned()) {
            qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
            penWidth = qRound(penWidth * dpr) / dpr; // Ensures integer width after dpr scaling
        }
        rectangle->setPenWidth(penWidth);
        rectangle->setAligned(false); // width rounding already done, so the Node should not do it
    } else {
        rectangle->setPenWidth(0);
    }

    rectangle->setRadius(d->radius);
    if (d->extraRectangle.isAllocated()) {
        const auto &extra = d->extraRectangle.value();
        if (extra.isTopLeftRadiusSet)
            rectangle->setTopLeftRadius(extra.topLeftRadius);
        else
            rectangle->resetTopLeftRadius();
        if (extra.isTopRightRadiusSet)
            rectangle->setTopRightRadius(extra.topRightRadius);
        else
            rectangle->resetTopRightRadius();
        if (extra.isBottomLeftRadiusSet)
            rectangle->setBottomLeftRadius(extra.bottomLeftRadius);
        else
            rectangle->resetBottomLeftRadius();
        if (extra.isBottomRightRadiusSet)
            rectangle->setBottomRightRadius(extra.bottomRightRadius);
        else
            rectangle->resetBottomRightRadius();
    } else {
        rectangle->resetTopLeftRadius();
        rectangle->resetTopRightRadius();
        rectangle->resetBottomLeftRadius();
        rectangle->resetBottomRightRadius();
    }
    rectangle->setAntialiasing(antialiasing());

    QGradientStops stops;
    bool vertical = true;
    if (d->gradient.isQObject()) {
        auto gradient = qobject_cast<QQuickGradient*>(d->gradient.toQObject());
        Q_ASSERT(gradient);
        stops = gradient->gradientStops();
        vertical = gradient->orientation() == QQuickGradient::Vertical;
    } else if (d->gradient.isNumber() || d->gradient.isString()) {
        QGradient preset(d->gradient.toVariant().value<QGradient::Preset>());
        if (preset.type() == QGradient::LinearGradient) {
            auto linearGradient = static_cast<QLinearGradient&>(preset);
            const QPointF start = linearGradient.start();
            const QPointF end = linearGradient.finalStop();
            vertical = qAbs(start.y() - end.y()) >= qAbs(start.x() - end.x());
            stops = linearGradient.stops();
            if ((vertical && start.y() > end.y()) || (!vertical && start.x() > end.x())) {
                // QSGInternalRectangleNode doesn't support stops in the wrong order,
                // so we need to manually reverse them here.
                QGradientStops reverseStops;
                for (auto it = stops.rbegin(); it != stops.rend(); ++it) {
                    auto stop = *it;
                    stop.first = 1 - stop.first;
                    reverseStops.append(stop);
                }
                stops = reverseStops;
            }
        }
    }
    rectangle->setGradientStops(stops);
    rectangle->setGradientVertical(vertical);

    rectangle->update();

    return rectangle;
}

QT_END_NAMESPACE

#include "moc_qquickrectangle_p.cpp"
