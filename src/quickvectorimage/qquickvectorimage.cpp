// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qurl.h>
#include "qquickvectorimage_p.h"
#include "qquickvectorimage_p_p.h"
#include <QtQuickVectorImageGenerator/private/qquickitemgenerator_p.h>
#include <QtQuickVectorImageGenerator/private/qquickvectorimageglobal_p.h>
#include <QtCore/qloggingcategory.h>

#include <private/qquicktranslate_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQuick.VectorImage
    \title Qt Quick Vector Image QML Types
    \ingroup qmlmodules
    \brief Provides QML types for displaying vector image files.
    \since 6.8

    To use the types in this module, import the module with the following line:

    \qml
    import QtQuick.VectorImage
    \endqml

    Qt Quick Vector Image provides support for displaying vector image files in a Qt Quick
    scene.

    It currently supports the \c SVG file format.

    Qt supports multiple options for displaying SVG files. For an overview and comparison of the
    different ones, see the documentation of the \l{svgtoqml} tool.

    \section1 QML Types
*/

void QQuickVectorImagePrivate::setSource(const QUrl &source)
{
    Q_Q(QQuickVectorImage);
    if (sourceFile == source)
        return;

    sourceFile = source;
    loadSvg();
    emit q->sourceChanged();
}

void QQuickVectorImagePrivate::loadSvg()
{
    Q_Q(QQuickVectorImage);

    QUrl resolvedUrl = qmlContext(q)->resolvedUrl(sourceFile);
    QString localFile = QQmlFile::urlToLocalFileOrQrc(resolvedUrl);

    if (localFile.isEmpty())
        return;

    QQuickVectorImagePrivate::Format fileFormat = formatFromFilePath(localFile);

    if (fileFormat != QQuickVectorImagePrivate::Format::Svg) {
        qCWarning(lcQuickVectorImage) << "Unsupported file format";
        return;
    }

    if (svgItem)
        svgItem->deleteLater();

    svgItem = new QQuickItem(q);
    svgItem->setParentItem(q);

    QQuickVectorImageGenerator::GeneratorFlags flags;
    if (preferredRendererType == QQuickVectorImage::CurveRenderer)
        flags.setFlag(QQuickVectorImageGenerator::CurveRenderer);
    QQuickItemGenerator generator(localFile, flags, svgItem, qmlContext(q));
    generator.generate();

    q->setImplicitWidth(svgItem->width());
    q->setImplicitHeight(svgItem->height());

    q->updateAnimationProperties();
    q->updateSvgItemScale();
    q->update();
}

QQuickVectorImagePrivate::Format QQuickVectorImagePrivate::formatFromFilePath(const QString &filePath)
{
    Q_UNUSED(filePath)

    QQuickVectorImagePrivate::Format res = QQuickVectorImagePrivate::Format::Unknown;

    if (filePath.endsWith(QLatin1String(".svg")) || filePath.endsWith(QLatin1String(".svgz"))
        || filePath.endsWith(QLatin1String(".svg.gz"))) {
        res = QQuickVectorImagePrivate::Format::Svg;
    }

    return res;
}

/*!
    \qmltype VectorImage
    \inqmlmodule QtQuick.VectorImage
    \inherits Item
    \brief Loads a vector image file and displays it in a Qt Quick scene.
    \since 6.8

    The VectorImage can be used to load a vector image file and display this as an item in a Qt
    Quick scene. It currently supports the \c SVG file format.

    \note This complements the approach of loading the vector image file through an \l Image
    element: \l Image creates a raster version of the image at the requested size. VectorImage
    builds a Qt Quick scene that represents the image. This means the resulting item can be scaled
    and rotated without losing quality, and it will typically consume less memory than the
    rasterized version.
*/
QQuickVectorImage::QQuickVectorImage(QQuickItem *parent)
    : QQuickItem(*(new QQuickVectorImagePrivate), parent)
{
    setFlag(QQuickItem::ItemHasContents, true);

    QObject::connect(this, &QQuickItem::widthChanged, this, &QQuickVectorImage::updateSvgItemScale);
    QObject::connect(this, &QQuickItem::heightChanged, this, &QQuickVectorImage::updateSvgItemScale);
    QObject::connect(this, &QQuickVectorImage::fillModeChanged, this, &QQuickVectorImage::updateSvgItemScale);
}

/*!
    \qmlproperty url QtQuick.VectorImage::VectorImage::source

    This property holds the URL of the vector image file to load.

    VectorImage currently only supports the \c SVG file format.
*/
QUrl QQuickVectorImage::source() const
{
    Q_D(const QQuickVectorImage);
    return d->sourceFile;
}

void QQuickVectorImage::setSource(const QUrl &source)
{
    Q_D(QQuickVectorImage);
    d->setSource(source);
}

void QQuickVectorImage::updateSvgItemScale()
{
    Q_D(QQuickVectorImage);

    if (d->svgItem == nullptr
        || qFuzzyIsNull(d->svgItem->width())
        || qFuzzyIsNull(d->svgItem->height())) {
        return;
    }

    auto xformProp = d->svgItem->transform();
    QQuickScale *scaleTransform = nullptr;
    if (xformProp.count(&xformProp) == 0) {
        scaleTransform = new QQuickScale;
        scaleTransform->setParent(d->svgItem);
        xformProp.append(&xformProp, scaleTransform);
    } else {
        scaleTransform = qobject_cast<QQuickScale *>(xformProp.at(&xformProp, 0));
    }

    if (scaleTransform != nullptr) {
        qreal xScale = width() / d->svgItem->width();
        qreal yScale = height() / d->svgItem->height();

        switch (d->fillMode) {
        case QQuickVectorImage::NoResize:
            xScale = yScale = 1.0;
            break;
        case QQuickVectorImage::PreserveAspectFit:
            xScale = yScale = qMin(xScale, yScale);
            break;
        case QQuickVectorImage::PreserveAspectCrop:
            xScale = yScale = qMax(xScale, yScale);
            break;
        case QQuickVectorImage::Stretch:
            // Already correct
            break;
        };

        scaleTransform->setXScale(xScale);
        scaleTransform->setYScale(yScale);
    }
}

void QQuickVectorImage::updateAnimationProperties()
{
    Q_D(QQuickVectorImage);
    if (Q_UNLIKELY(d->svgItem == nullptr || d->svgItem->childItems().isEmpty()))
        return;

    QQuickItem *childItem = d->svgItem->childItems().first();
    if (Q_UNLIKELY(d->animations != nullptr)) {
        QObject *animationsInfo = childItem->property("animations").value<QObject*>();
        if (Q_UNLIKELY(animationsInfo != nullptr)) {
            animationsInfo->setProperty("loops", d->animations->loops());
            animationsInfo->setProperty("paused", d->animations->paused());
        }
    }
}

QQuickVectorImageAnimations *QQuickVectorImage::animations()
{
    Q_D(QQuickVectorImage);
    if (d->animations == nullptr) {
        d->animations = new QQuickVectorImageAnimations;
        QQml_setParent_noEvent(d->animations, this);
        QObject::connect(d->animations, &QQuickVectorImageAnimations::loopsChanged, this, &QQuickVectorImage::updateAnimationProperties);
        QObject::connect(d->animations, &QQuickVectorImageAnimations::pausedChanged, this, &QQuickVectorImage::updateAnimationProperties);
    }

    return d->animations;
}

/*!
    \qmlproperty enumeration QtQuick.VectorImage::VectorImage::fillMode

    This property defines what happens if the width and height of the VectorImage differs from
    the implicit size of its contents.

    \value VectorImage.NoResize             The contents are still rendered at the size provided by
                                            the input.
    \value VectorImage.Stretch              The contents are scaled to match the width and height of
                                            the \c{VectorImage}. (This is the default.)
    \value VectorImage.PreserveAspectFit    The contents are scaled to fit inside the bounds of the
                                            \c VectorImage, while preserving aspect ratio. The
                                            actual bounding rect of the contents will sometimes be
                                            smaller than the \c VectorImage item.
    \value VectorImage.PreserveAspectCrop   The contents are scaled to fill the \c VectorImage item,
                                            while preserving the aspect ratio. The actual bounds of
                                            the contents will sometimes be larger than the
                                            \c VectorImage item.
*/

QQuickVectorImage::FillMode QQuickVectorImage::fillMode() const
{
    Q_D(const QQuickVectorImage);
    return d->fillMode;
}

void QQuickVectorImage::setFillMode(FillMode newFillMode)
{
    Q_D(QQuickVectorImage);
    if (d->fillMode == newFillMode)
        return;
    d->fillMode = newFillMode;
    emit fillModeChanged();
}

/*!
    \qmlproperty enumeration QtQuick.VectorImage::VectorImage::preferredRendererType

    Requests a specific backend to use for rendering shapes in the \c VectorImage.

    \value VectorImage.GeometryRenderer Equivalent to Shape.GeometryRenderer. This backend flattens
    curves and triangulates the result. It will give aliased results unless multi-sampling is
    enabled, and curve flattening may be visible when the item is scaled.
    \value VectorImage.CurveRenderer Equivalent to Shape.CurveRenderer. With this backend, curves
    are rendered on the GPU and anti-aliasing is built in. Will typically give better visual
    results, but at some extra cost to performance.

    The default is \c{VectorImage.GeometryRenderer}.
*/

QQuickVectorImage::RendererType QQuickVectorImage::preferredRendererType() const
{
    Q_D(const QQuickVectorImage);
    return d->preferredRendererType;
}

void QQuickVectorImage::setPreferredRendererType(RendererType newPreferredRendererType)
{
    Q_D(QQuickVectorImage);
    if (d->preferredRendererType == newPreferredRendererType)
        return;
    d->preferredRendererType = newPreferredRendererType;
    d->loadSvg();
    emit preferredRendererTypeChanged();
}

/*!
    \qmlpropertygroup QtQuick.VectorImage::VectorImage::animations
    \qmlproperty bool QtQuick.VectorImage::VectorImage::animations.paused
    \qmlproperty int QtQuick.VectorImage::VectorImage::animations.loops
    \since 6.10

    These properties can be used to control animations in the image, if it contains any.

    The \c paused property can be set to true to temporarily pause all animations. When the
    property is reset to \c false, the animations will resume where they were. By default this
    property is \c false.

    The \c loops property defines the number of times the animations in the document will repeat.
    By default this property is 1. Any animations that is set to loop indefinitely in the source
    image will be unaffected by this property. To make all animations in the document repeat
    indefinitely, the \c loops property can be set to \c{Animation.Infinite}.
*/
int QQuickVectorImageAnimations::loops() const
{
    return m_loops;
}

void QQuickVectorImageAnimations::setLoops(int loops)
{
    if (m_loops == loops)
        return;
    m_loops = loops;
    emit loopsChanged();
}

bool QQuickVectorImageAnimations::paused() const
{
    return m_paused;
}

void QQuickVectorImageAnimations::setPaused(bool paused)
{
    if (m_paused == paused)
        return;
    m_paused = paused;
    emit pausedChanged();
}

void QQuickVectorImageAnimations::restart()
{
    QQuickVectorImage *parentVectorImage = qobject_cast<QQuickVectorImage *>(parent());
    if (Q_UNLIKELY(parentVectorImage == nullptr)) {
        qCWarning(lcQuickVectorImage) << Q_FUNC_INFO << "Parent is not a VectorImage";
        return;
    }

    QQuickVectorImagePrivate *d = QQuickVectorImagePrivate::get(parentVectorImage);

    if (Q_UNLIKELY(d->svgItem == nullptr || d->svgItem->childItems().isEmpty()))
        return;

    QQuickItem *childItem = d->svgItem->childItems().first();
    QObject *animationsInfo = childItem->property("animations").value<QObject*>();

    if (Q_UNLIKELY(animationsInfo == nullptr)) {
        qCWarning(lcQuickVectorImage) << Q_FUNC_INFO << "Item does not have animations property";
        return;
    }

    QMetaObject::invokeMethod(animationsInfo, "restart");
}

QT_END_NAMESPACE

#include <moc_qquickvectorimage_p.cpp>
