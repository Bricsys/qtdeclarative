// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickiconimage_p.h"
#include "qquickiconimage_p_p.h"

#include <QtCore/qmath.h>
#include <QtQuick/private/qquickimagebase_p_p.h>

QT_BEGIN_NAMESPACE

QQuickIconImagePrivate::~QQuickIconImagePrivate()
{
    icon.entries.clear();
}

bool QQuickIconImagePrivate::updateDevicePixelRatio(qreal targetDevicePixelRatio)
{
    if (isThemeIcon) {
        devicePixelRatio = calculateDevicePixelRatio();
        return true;
    }

    return QQuickImagePrivate::updateDevicePixelRatio(targetDevicePixelRatio);
}

void QQuickIconImage::load()
{
    Q_D(QQuickIconImage);
    // Both geometryChange() and QQuickImageBase::sourceSizeChanged()
    // (which we connect to load() in the constructor) can be called as a result
    // of load() changing the various sizes, so we must check that we're not recursing.
    if (d->updatingIcon)
        return;

    d->updatingIcon = true;

    QSize size = d->sourcesize;
    // If no size is specified for theme icons, it will use the smallest available size.
    if (size.width() <= 0)
        size.setWidth(width());
    if (size.height() <= 0)
        size.setHeight(height());

    const qreal dpr = d->calculateDevicePixelRatio();

    if (const auto *entry = QIconLoaderEngine::entryForSize(d->icon, size * dpr, qCeil(dpr))) {
        // First, try to load an icon from the theme (index.theme).
        QQmlContext *context = qmlContext(this);
        const QUrl entryUrl = QUrl::fromLocalFile(entry->filename);
        d->url = context ? context->resolvedUrl(entryUrl) : entryUrl;
        d->isThemeIcon = true;
    } else if (d->source.isEmpty()) {
        // Then, try loading it through a platform icon engine.
        std::unique_ptr<QIconEngine> iconEngine(QIconLoader::instance()->iconEngine(d->icon.iconName));
        if (iconEngine && !iconEngine->isNull()) {
            // ### TODO that's the best we can do for now to select different pixmaps based on the
            // QuickItem's state. QQuickIconImage cannot know about the state of the control that
            // uses it without adding more properties that are then synced up with the control.
            const QIcon::Mode mode = isEnabled() ? QIcon::Normal : QIcon::Disabled;
            const QImage image = iconEngine->scaledPixmap(size, mode, QIcon::Off, dpr).toImage();
            d->devicePixelRatio = image.devicePixelRatio();
            d->setImage(image);
        }
    } else {
        // Either we failed to load a named icon or the source was set instead; use that.
        d->url = d->source;
        d->isThemeIcon = false;
    }
    if (!d->url.isEmpty())
        QQuickImage::load();

    d->updatingIcon = false;
}

void QQuickIconImagePrivate::updateFillMode()
{
    Q_Q(QQuickIconImage);
    // If we start with a sourceSize of 28x28 and then set sourceSize.width to 24, the fillMode
    // will change to PreserveAspectFit (because pixmapSize.width() > width()), which causes the
    // pixmap to be reloaded at its original size of 28x28, which causes the fillMode to change
    // to Pad (because pixmapSize.width() <= width()), and so on.
    if (updatingFillMode)
        return;

    updatingFillMode = true;

    const QSize pixmapSize = QSize(currentPix->width(), currentPix->height()) / calculateDevicePixelRatio();
    if (pixmapSize.width() > q->width() || pixmapSize.height() > q->height())
        q->setFillMode(QQuickImage::PreserveAspectFit);
    else
        q->setFillMode(QQuickImage::Pad);

    updatingFillMode = false;
}

qreal QQuickIconImagePrivate::calculateDevicePixelRatio() const
{
    Q_Q(const QQuickIconImage);
    return q->window() ? q->window()->effectiveDevicePixelRatio() : qApp->devicePixelRatio();
}

QQuickIconImage::QQuickIconImage(QQuickItem *parent)
    : QQuickImage(*(new QQuickIconImagePrivate), parent)
{
    setFillMode(Pad);
}

QString QQuickIconImage::name() const
{
    Q_D(const QQuickIconImage);
    return d->icon.iconName;
}

void QQuickIconImage::setName(const QString &name)
{
    Q_D(QQuickIconImage);
    if (d->icon.iconName == name)
        return;

    d->icon.entries.clear();
    d->icon = QIconLoader::instance()->loadIcon(name);
    if (d->icon.iconName.isEmpty())
        d->icon.iconName = name;
    if (isComponentComplete())
        load();
    emit nameChanged();
}

QColor QQuickIconImage::color() const
{
    Q_D(const QQuickIconImage);
    return d->color;
}

void QQuickIconImage::setColor(const QColor &color)
{
    Q_D(QQuickIconImage);
    if (d->color == color)
        return;

    d->color = color;
    if (isComponentComplete())
        load();
    emit colorChanged();
}

void QQuickIconImage::setSource(const QUrl &source)
{
    Q_D(QQuickIconImage);
    if (d->source == source)
        return;

    d->source = source;
    if (isComponentComplete())
        load();
    emit sourceChanged(source);
}

void QQuickIconImage::snapPositionTo(QPointF pos)
{
    // Ensure that we are placed on an integer position relative to the owning control. We assume
    // that there is exactly one intermediate parent item between us and the control (in practice,
    // the contentItem). The idea is to enable the app, by placing the controls at integer
    // positions, to avoid the image being smoothed over pixel boundaries in the basic, DPR=1 case.
    QPointF offset;
    if (parentItem())
        offset = parentItem()->position();
    QPointF offsetPos(std::round(offset.x() + pos.x()), std::round(offset.y() + pos.y()));
    setPosition(offsetPos - offset);
}

void QQuickIconImage::componentComplete()
{
    QQuickImage::componentComplete();
    load();
}

void QQuickIconImage::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickImage::geometryChange(newGeometry, oldGeometry);
    if (isComponentComplete() && newGeometry.size() != oldGeometry.size())
        load();
}

void QQuickIconImage::pixmapChange()
{
    Q_D(QQuickIconImage);
    QQuickImage::pixmapChange();
    d->updateFillMode();

    // Don't apply the color if we're recursing (updateFillMode() can cause us to recurse).
    if (!d->updatingFillMode && d->color.alpha() > 0) {
        QImage image = d->currentPix->image();
        if (!image.isNull()) {
            QPainter painter(&image);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(image.rect(), d->color);
            d->currentPix->setImage(image);
        }
    }
}

QT_END_NAMESPACE

#include "moc_qquickiconimage_p.cpp"
