// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickitemgenerator_p.h"
#include "utils_p.h"
#include "qquicknodeinfo_p.h"

#include <private/qsgcurveprocessor_p.h>
#include <private/qquickshape_p.h>
#include <private/qquadpath_p.h>
#include <private/qquickitem_p.h>
#include <private/qquickimagebase_p_p.h>
#include <private/qquickanimation_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktranslate_p.h>
#include <private/qquickimage_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

QQuickItemGenerator::QQuickItemGenerator(const QString fileName, QQuickVectorImageGenerator::GeneratorFlags flags, QQuickItem *parentItem)
    :QQuickGenerator(fileName, flags)
{
    Q_ASSERT(parentItem);
    m_items.push(parentItem);
    m_parentItem = parentItem;
}

QQuickItemGenerator::~QQuickItemGenerator()
{
}

void QQuickItemGenerator::generateNodeBase(const NodeInfo &info)
{
    auto xformProp = currentItem()->transform();

    if (info.transform.isAnimated()) {
        QList<QQuickTransform *> transforms;

        for (int i = info.transform.animationCount() - 1; i >= 0; --i) {
            const auto &animation = info.transform.animation(i);
            QQuickTransform *transform = nullptr;
            switch (animation.subtype) {
            case QTransform::TxTranslate:
                transform = new QQuickTranslate;
                break;
            case QTransform::TxScale:
                transform = new QQuickScale;
                break;
            case QTransform::TxRotate:
                transform = new QQuickRotation;
                static_cast<QQuickRotation *>(transform)->setOrigin(QVector3D(currentItem()->width() / 2.0f,
                                                                              currentItem()->height() / 2.0f,
                                                                              0.0f));
                break;
            case QTransform::TxShear:
                transform = new QQuickShear;
                break;
            default:
                qCWarning(lcQuickVectorImage) << "Unhandled transform type" << animation.subtype;
                break;
            };

            xformProp.append(&xformProp, transform);
            transforms.prepend(transform);
        }

        QQuickMatrix4x4 *mainTransform = nullptr;
        if (!info.isDefaultTransform) {
            const QMatrix4x4 m(info.transform.defaultValue().value<QTransform>());
            mainTransform = new QQuickMatrix4x4;
            mainTransform->setMatrix(m);
            xformProp.append(&xformProp, mainTransform);
        }

        generateAnimateTransform(transforms, info);
    } else if (!info.isDefaultTransform) {
        const QTransform transform = info.transform.defaultValue().value<QTransform>();

        auto sx = transform.m11();
        auto sy = transform.m22();
        auto x = transform.m31();
        auto y = transform.m32();

        if (transform.type() == QTransform::TxTranslate) {
            auto *translate = new QQuickTranslate;
            translate->setX(x);
            translate->setY(y);
            xformProp.append(&xformProp, translate);
        } else if (transform.type() == QTransform::TxScale && !x && !y) {
            auto scale = new QQuickScale;
            scale->setParent(currentItem());
            scale->setXScale(sx);
            scale->setYScale(sy);
            xformProp.append(&xformProp, scale);
        } else {
            const QMatrix4x4 m(transform);
            auto xform = new QQuickMatrix4x4;
            xform->setMatrix(m);
            xformProp.append(&xformProp, xform);
        }
    }

    if (!info.isDefaultOpacity)
        currentItem()->setOpacity(info.opacity.defaultValue().toReal());

    if (info.opacity.isAnimated())
        generatePropertyAnimation(info.opacity, currentItem(), QStringLiteral("opacity"));
}

bool QQuickItemGenerator::generateDefsNode(const NodeInfo &info)
{
    Q_UNUSED(info)

    return false;
}

void QQuickItemGenerator::generateImageNode(const ImageNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    auto *imageItem = new QQuickImage;
    auto *imagePriv = static_cast<QQuickImageBasePrivate*>(QQuickItemPrivate::get(imageItem));
    imagePriv->currentPix->setImage(info.image);

    imageItem->setX(info.rect.x());
    imageItem->setY(info.rect.y());
    imageItem->setWidth(info.rect.width());
    imageItem->setHeight(info.rect.height());

    addCurrentItem(imageItem, info);
    generateNodeBase(info);

    m_items.pop();
}

void QQuickItemGenerator::generatePath(const PathNodeInfo &info, const QRectF &overrideBoundingRect)
{
    if (!isNodeVisible(info))
        return;

    if (m_parentShapeItems.size()) {
        if (!info.isDefaultTransform)
            qCWarning(lcQuickVectorImage) << "Skipped transform for node" << info.nodeId << "type" << info.typeName << "(this is not supposed to happen)";
        optimizePaths(info, overrideBoundingRect);
    } else {
        auto *shapeItem = new QQuickShape;
        if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
            shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
        shapeItem->setContainsMode(QQuickShape::ContainsMode::FillContains); // TODO: configurable?
        addCurrentItem(shapeItem, info);
        m_parentShapeItems.push(shapeItem);

        generateNodeBase(info);

        optimizePaths(info, overrideBoundingRect);
        //qCDebug(lcQuickVectorGraphics) << *node->qpath();
        m_items.pop();
        m_parentShapeItems.pop();
    }
}

void QQuickItemGenerator::generatePropertyAnimation(const QQuickAnimatedProperty &property,
                                                    QObject *target,
                                                    const QString &propertyName)
{
    for (int i = 0; i < property.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation &animation = property.animation(i);

        QQuickSequentialAnimation *sequentialAnimation = new QQuickSequentialAnimation(target);
        QQmlListProperty<QQuickAbstractAnimation> anims = sequentialAnimation->animations();

        if (animation.startOffset > 0) {
            QQuickPauseAnimation *pauseAnimation = new QQuickPauseAnimation(sequentialAnimation);
            pauseAnimation->setDuration(animation.startOffset);
            anims.append(&anims, pauseAnimation);
        }

        QQuickSequentialAnimation *keyFrameAnimation = new QQuickSequentialAnimation(sequentialAnimation);
        keyFrameAnimation->setLoops(animation.repeatCount);
        anims.append(&anims, keyFrameAnimation);

        anims = keyFrameAnimation->animations();

        int previousTime = 0;
        for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
            const int time = it.key();
            const QVariant &value = it.value();

            QQuickPropertyAnimation *propertyAnimation;
            if (value.typeId() == QMetaType::QColor) {
                QQuickColorAnimation *colorAnimation = new QQuickColorAnimation(keyFrameAnimation);
                colorAnimation->setTo(value.value<QColor>());
                propertyAnimation = colorAnimation;
            } else {
                propertyAnimation = new QQuickPropertyAnimation(keyFrameAnimation);
                propertyAnimation->setTo(value.toReal());
            }

            propertyAnimation->setTargetObject(target);
            propertyAnimation->setProperty(propertyName);

            propertyAnimation->setDuration(time - previousTime);
            anims.append(&anims, propertyAnimation);

            previousTime = time;
        }

        if (!(animation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd)) {
            anims = sequentialAnimation->animations();

            QQuickPropertyAction *resetAction = new QQuickPropertyAction(sequentialAnimation);
            resetAction->setTargetObject(target);
            resetAction->setProperty(propertyName);
            resetAction->setValue(property.defaultValue());
            anims.append(&anims, resetAction);
        }

        sequentialAnimation->setRunning(true);
    }
}

void QQuickItemGenerator::outputShapePath(const PathNodeInfo &info, const QPainterPath *painterPath, const QQuadPath *quadPath, QQuickVectorImageGenerator::PathSelector pathSelector, const QRectF &boundingRect)
{
    Q_UNUSED(pathSelector)
    Q_ASSERT(painterPath || quadPath);

    const QColor color = info.strokeStyle.color.defaultValue().value<QColor>();
    const bool noPen = color == QColorConstants::Transparent && !info.strokeStyle.color.isAnimated();
    if (pathSelector == QQuickVectorImageGenerator::StrokePath && noPen)
        return;

    const QColor fillColor = info.fillColor.defaultValue().value<QColor>();
    const bool noFill = info.grad.type() == QGradient::NoGradient && fillColor == QColorConstants::Transparent && !info.fillColor.isAnimated();

    if (pathSelector == QQuickVectorImageGenerator::FillPath && noFill)
        return;

    QQuickShapePath::FillRule fillRule = QQuickShapePath::FillRule(painterPath ? painterPath->fillRule() : quadPath->fillRule());

    QQuickShapePath *shapePath = new QQuickShapePath;
    Q_ASSERT(shapePath);

    if (!info.nodeId.isEmpty())
        shapePath->setObjectName(QStringLiteral("svg_path:") + info.nodeId);

    if (noPen || !(pathSelector & QQuickVectorImageGenerator::StrokePath)) {
        shapePath->setStrokeColor(Qt::transparent);
    } else {
        shapePath->setStrokeColor(color);
        shapePath->setStrokeWidth(info.strokeStyle.width);
        shapePath->setCapStyle(QQuickShapePath::CapStyle(info.strokeStyle.lineCapStyle));
        shapePath->setJoinStyle(QQuickShapePath::JoinStyle(info.strokeStyle.lineJoinStyle));
        shapePath->setMiterLimit(info.strokeStyle.miterLimit);
        if (info.strokeStyle.dashArray.length() != 0) {
            shapePath->setStrokeStyle(QQuickShapePath::DashLine);
            shapePath->setDashPattern(info.strokeStyle.dashArray.toVector());
            shapePath->setDashOffset(info.strokeStyle.dashOffset);
        }
    }

    QTransform fillTransform = info.fillTransform;
    if (!(pathSelector & QQuickVectorImageGenerator::FillPath)) {
        shapePath->setFillColor(Qt::transparent);
    } else if (info.grad.type() != QGradient::NoGradient) {
        generateGradient(&info.grad, shapePath);
        if (info.grad.coordinateMode() == QGradient::ObjectMode) {
            QTransform objectToUserSpace;
            objectToUserSpace.translate(boundingRect.x(), boundingRect.y());
            objectToUserSpace.scale(boundingRect.width(), boundingRect.height());
            fillTransform *= objectToUserSpace;
        }
    } else {
        shapePath->setFillColor(fillColor);
    }

    generatePropertyAnimation(info.strokeStyle.color, shapePath, QStringLiteral("strokeColor"));
    generatePropertyAnimation(info.fillColor, shapePath, QStringLiteral("fillColor"));

    shapePath->setFillRule(fillRule);
    if (!fillTransform.isIdentity())
        shapePath->setFillTransform(fillTransform);

    QString svgPathString = painterPath ? QQuickVectorImageGenerator::Utils::toSvgString(*painterPath) : QQuickVectorImageGenerator::Utils::toSvgString(*quadPath);

    auto *pathSvg = new QQuickPathSvg;
    pathSvg->setPath(svgPathString);
    pathSvg->setParent(shapePath);

    auto pathElementProp = shapePath->pathElements();
    pathElementProp.append(&pathElementProp, pathSvg);

    shapePath->setParent(currentItem());
    auto shapeDataProp = m_parentShapeItems.top()->data();
    shapeDataProp.append(&shapeDataProp, shapePath);
}

void QQuickItemGenerator::generateGradient(const QGradient *grad, QQuickShapePath *shapePath)
{
    if (!shapePath)
        return;

    auto setStops = [=](QQuickShapeGradient *quickGrad, const QGradientStops &stops) {
        auto stopsProp = quickGrad->stops();
        for (auto &stop : stops) {
            auto *stopObj = new QQuickGradientStop(quickGrad);
            stopObj->setPosition(stop.first);
            stopObj->setColor(stop.second);
            stopsProp.append(&stopsProp, stopObj);
        }
    };

    if (grad->type() == QGradient::LinearGradient) {
        auto *linGrad = static_cast<const QLinearGradient *>(grad);

        auto *quickGrad = new QQuickShapeLinearGradient(shapePath);
        quickGrad->setX1(linGrad->start().x());
        quickGrad->setY1(linGrad->start().y());
        quickGrad->setX2(linGrad->finalStop().x());
        quickGrad->setY2(linGrad->finalStop().y());
        setStops(quickGrad, linGrad->stops());

        shapePath->setFillGradient(quickGrad);
    } else if (grad->type() == QGradient::RadialGradient) {
        auto *radGrad = static_cast<const QRadialGradient*>(grad);
        auto *quickGrad = new QQuickShapeRadialGradient(shapePath);
        quickGrad->setCenterX(radGrad->center().x());
        quickGrad->setCenterY(radGrad->center().y());
        quickGrad->setCenterRadius(radGrad->radius());
        quickGrad->setFocalX(radGrad->focalPoint().x());
        quickGrad->setFocalY(radGrad->focalPoint().y());
        setStops(quickGrad, radGrad->stops());

        shapePath->setFillGradient(quickGrad);
    }
}

void QQuickItemGenerator::generateNode(const NodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    qCWarning(lcQuickVectorImage) << "SVG NODE NOT IMPLEMENTED: "
                                  << info.nodeId
                                  << " type: " << info.typeName;
}

void QQuickItemGenerator::generateTextNode(const TextNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    QQuickItem *alignItem = nullptr;
    QQuickText *textItem = nullptr;

    QQuickItem *containerItem = new QQuickItem(currentItem());
    addCurrentItem(containerItem, info);

    generateNodeBase(info);

    if (!info.isTextArea) {
        alignItem = new QQuickItem(currentItem());
        alignItem->setX(info.position.x());
        alignItem->setY(info.position.y());
    }

    textItem = new QQuickText(containerItem);
    addCurrentItem(textItem, info);

    if (info.isTextArea) {
        textItem->setX(info.position.x());
        textItem->setY(info.position.y());
        if (info.size.width() > 0)
            textItem->setWidth(info.size.width());
        if (info.size.height() > 0)
            textItem->setHeight(info.size.height());
        textItem->setWrapMode(QQuickText::Wrap);
        textItem->setClip(true);
    } else {
        auto *anchors = QQuickItemPrivate::get(textItem)->anchors();
        auto *alignPrivate = QQuickItemPrivate::get(alignItem);
        anchors->setBaseline(alignPrivate->top());

        switch (info.alignment) {
        case Qt::AlignHCenter:
            anchors->setHorizontalCenter(alignPrivate->left());
            break;
        case Qt::AlignRight:
            anchors->setRight(alignPrivate->left());
            break;
        default:
            qCDebug(lcQuickVectorImage) << "Unexpected text alignment" << info.alignment;
            Q_FALLTHROUGH();
        case Qt::AlignLeft:
            anchors->setLeft(alignPrivate->left());
            break;
        }
    }

    textItem->setColor(info.fillColor.defaultValue().value<QColor>());
    textItem->setTextFormat(info.needsRichText ? QQuickText::RichText : QQuickText::StyledText);
    textItem->setText(info.text);
    textItem->setFont(info.font);

    const QColor strokeColor = info.strokeColor.defaultValue().value<QColor>();
    if (strokeColor != QColorConstants::Transparent || info.strokeColor.isAnimated()) {
        textItem->setStyleColor(strokeColor);
        textItem->setStyle(QQuickText::Outline);
    }

    generatePropertyAnimation(info.fillColor, textItem, QStringLiteral("color"));
    generatePropertyAnimation(info.strokeColor, textItem, QStringLiteral("styleColor"));

    m_items.pop(); m_items.pop();
}

void QQuickItemGenerator::generateUseNode(const UseNodeInfo &info)
{
    if (!isNodeVisible(info))
        return;

    if (info.stage == StructureNodeStage::Start) {
        QQuickItem *item = new QQuickItem();
        item->setPosition(info.startPos);
        addCurrentItem(item, info);
        generateNodeBase(info);
    } else {
        m_items.pop();
    }

}

void QQuickItemGenerator::generatePathContainer(const StructureNodeInfo &info)
{
    auto *shapeItem = new QQuickShape;
    if (m_flags.testFlag(QQuickVectorImageGenerator::GeneratorFlag::CurveRenderer))
        shapeItem->setPreferredRendererType(QQuickShape::CurveRenderer);
    m_parentShapeItems.push(shapeItem);
    addCurrentItem(shapeItem, info);
}

void QQuickItemGenerator::generateAnimateTransform(const QList<QQuickTransform *> &transforms,
                                                   const NodeInfo &info)
{
    Q_ASSERT(info.transform.isAnimated());
    Q_ASSERT(info.transform.animationCount() == transforms.size());

    for (int i = 0; i < info.transform.animationCount(); ++i) {
        const QQuickAnimatedProperty::PropertyAnimation &animation = info.transform.animation(i);
        QQuickTransform *xform = transforms.at(i);

        // Skip past broken entries
        if (xform == nullptr)
            continue;

        QQuickSequentialAnimation *sequentialAnimation = new QQuickSequentialAnimation(currentItem());
        QQmlListProperty<QQuickAbstractAnimation> anims = sequentialAnimation->animations();

        if (animation.startOffset > 0) {
            QQuickPauseAnimation *pauseAnimation = new QQuickPauseAnimation(sequentialAnimation);
            pauseAnimation->setDuration(animation.startOffset);
            anims.append(&anims, pauseAnimation);
        }

        QQuickSequentialAnimation *keyFrameAnimation = new QQuickSequentialAnimation(sequentialAnimation);
        keyFrameAnimation->setLoops(animation.repeatCount);
        anims.append(&anims, keyFrameAnimation);

        anims = keyFrameAnimation->animations();

        QTransform::TransformationType transformType = QTransform::TransformationType(animation.subtype);
        int previousTime = 0;
        for (auto it = animation.frames.constBegin(); it != animation.frames.constEnd(); ++it) {
            QQuickParallelAnimation *parallelAnimation = new QQuickParallelAnimation(keyFrameAnimation);
            anims.append(&anims, parallelAnimation);

            QQmlListProperty<QQuickAbstractAnimation> propertyAnims = parallelAnimation->animations();

            const int time = it.key();
            const int frameTime = time - previousTime;
            const QVariantList &parameters = it.value().value<QVariantList>();
            if (parameters.isEmpty())
                continue;

            switch (transformType) {
            case QTransform::TxTranslate:
            {
                const QPointF translation = parameters.first().value<QPointF>();
                auto *transXAnimation = new QQuickPropertyAnimation(parallelAnimation);
                transXAnimation->setDuration(frameTime);
                transXAnimation->setTargetObject(xform);
                transXAnimation->setProperty(QStringLiteral("x"));
                transXAnimation->setTo(translation.x());
                propertyAnims.append(&propertyAnims, transXAnimation);

                auto *transYAnimation = new QQuickPropertyAnimation(parallelAnimation);
                transYAnimation->setDuration(frameTime);
                transYAnimation->setTargetObject(xform);
                transYAnimation->setProperty(QStringLiteral("y"));
                transYAnimation->setTo(translation.y());
                propertyAnims.append(&propertyAnims, transYAnimation);
                break;
            }
            case QTransform::TxScale:
            {
                const QPointF scale = parameters.first().value<QPointF>();
                auto *scaleXAnimation = new QQuickPropertyAnimation(parallelAnimation);
                scaleXAnimation->setDuration(frameTime);
                scaleXAnimation->setTargetObject(xform);
                scaleXAnimation->setProperty(QStringLiteral("xScale"));
                scaleXAnimation->setTo(scale.x());
                propertyAnims.append(&propertyAnims, scaleXAnimation);

                auto *scaleYAnimation = new QQuickPropertyAnimation(parallelAnimation);
                scaleYAnimation->setDuration(frameTime);
                scaleYAnimation->setTargetObject(xform);
                scaleYAnimation->setProperty(QStringLiteral("yScale"));
                scaleYAnimation->setTo(scale.y());
                propertyAnims.append(&propertyAnims, scaleYAnimation);
                break;
            }
            case QTransform::TxRotate:
            {
                Q_ASSERT(parameters.size() == 2);
                const QPointF center = parameters.at(0).value<QPointF>();
                const qreal angle = parameters.at(1).toReal();
                auto *rotationOriginAnimation = new QQuickPropertyAnimation(parallelAnimation);
                rotationOriginAnimation->setDuration(frameTime);
                rotationOriginAnimation->setTargetObject(xform);
                rotationOriginAnimation->setProperty(QStringLiteral("origin"));
                rotationOriginAnimation->setTo(QVector3D(center.x(),
                                                         center.y(),
                                                         0.0));
                propertyAnims.append(&propertyAnims, rotationOriginAnimation);

                auto *rotationAngleAnimation = new QQuickPropertyAnimation(parallelAnimation);
                rotationAngleAnimation->setDuration(frameTime);
                rotationAngleAnimation->setTargetObject(xform);
                rotationAngleAnimation->setProperty(QStringLiteral("angle"));
                rotationAngleAnimation->setTo(angle);
                propertyAnims.append(&propertyAnims, rotationAngleAnimation);
                break;
            }
            case QTransform::TxShear:
            {
                const QPointF skew = parameters.first().value<QPointF>();
                auto *xSkewAnimation = new QQuickPropertyAnimation(parallelAnimation);
                xSkewAnimation->setDuration(frameTime);
                xSkewAnimation->setTargetObject(xform);
                xSkewAnimation->setProperty(QStringLiteral("xAngle"));
                xSkewAnimation->setTo(skew.x());
                propertyAnims.append(&propertyAnims, xSkewAnimation);

                auto *ySkewAnimation = new QQuickPropertyAnimation(parallelAnimation);
                ySkewAnimation->setDuration(frameTime);
                ySkewAnimation->setTargetObject(xform);
                ySkewAnimation->setProperty(QStringLiteral("yAngle"));
                ySkewAnimation->setTo(skew.y());
                propertyAnims.append(&propertyAnims, ySkewAnimation);
                break;
            }
            default:
                Q_UNREACHABLE();
            };

            previousTime = time;
        }

        if (!(animation.flags & QQuickAnimatedProperty::PropertyAnimation::FreezeAtEnd)) {
            anims = sequentialAnimation->animations();

            switch (transformType) {
            case QTransform::TxTranslate:
            {
                QQuickPropertyAction *resetActionX = new QQuickPropertyAction(sequentialAnimation);
                resetActionX->setTargetObject(xform);
                resetActionX->setProperty(QStringLiteral("x"));
                resetActionX->setValue(QVariant::fromValue(0.0));
                anims.append(&anims, resetActionX);

                QQuickPropertyAction *resetActionY = new QQuickPropertyAction(sequentialAnimation);
                resetActionY->setTargetObject(xform);
                resetActionY->setProperty(QStringLiteral("y"));
                resetActionY->setValue(QVariant::fromValue(0.0));
                anims.append(&anims, resetActionY);
                break;
            }
            case QTransform::TxScale:
            {
                QQuickPropertyAction *scaleActionX = new QQuickPropertyAction(sequentialAnimation);
                scaleActionX->setTargetObject(xform);
                scaleActionX->setProperty(QStringLiteral("xScale"));
                scaleActionX->setValue(QVariant::fromValue(1.0));
                anims.append(&anims, scaleActionX);

                QQuickPropertyAction *scaleActionY = new QQuickPropertyAction(sequentialAnimation);
                scaleActionY->setTargetObject(xform);
                scaleActionY->setProperty(QStringLiteral("yScale"));
                scaleActionY->setValue(QVariant::fromValue(1.0));
                anims.append(&anims, scaleActionY);
                break;
            }
            case QTransform::TxRotate:
            {
                QQuickPropertyAction *resetActionOrigin = new QQuickPropertyAction(sequentialAnimation);
                resetActionOrigin->setTargetObject(xform);
                resetActionOrigin->setProperty(QStringLiteral("origin"));
                resetActionOrigin->setValue(QVariant::fromValue(QPointF{}));
                anims.append(&anims, resetActionOrigin);

                QQuickPropertyAction *resetActionAngle = new QQuickPropertyAction(sequentialAnimation);
                resetActionAngle->setTargetObject(xform);
                resetActionAngle->setProperty(QStringLiteral("angle"));
                resetActionAngle->setValue(QVariant::fromValue(0.0));
                anims.append(&anims, resetActionAngle);
                break;
            }
            case QTransform::TxShear:
            {
                QQuickPropertyAction *resetActionXAngle = new QQuickPropertyAction(sequentialAnimation);
                resetActionXAngle->setTargetObject(xform);
                resetActionXAngle->setProperty(QStringLiteral("xAngle"));
                resetActionXAngle->setValue(QVariant::fromValue(0.0));
                anims.append(&anims, resetActionXAngle);

                QQuickPropertyAction *resetActionYAngle = new QQuickPropertyAction(sequentialAnimation);
                resetActionYAngle->setTargetObject(xform);
                resetActionYAngle->setProperty(QStringLiteral("yAngle"));
                resetActionYAngle->setValue(QVariant::fromValue(0.0));
                anims.append(&anims, resetActionYAngle);
                break;
            }
            default:
                Q_UNREACHABLE();
            };
        }

        sequentialAnimation->setRunning(true);
    }
}

bool QQuickItemGenerator::generateStructureNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info))
        return false;

    const bool isPathContainer = !info.forceSeparatePaths && info.isPathContainer;
    if (info.stage == StructureNodeStage::Start) {
        if (isPathContainer) {
            generatePathContainer(info);
        } else {
            QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorImageGenerator::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
            addCurrentItem(item, info);
        }

        generateNodeBase(info);
    } else {
        if (isPathContainer)
            m_parentShapeItems.pop();
        m_items.pop();
    }

    return true;
}

bool QQuickItemGenerator::generateRootNode(const StructureNodeInfo &info)
{
    if (!isNodeVisible(info)) {
        QQuickItem *item = new QQuickItem();
        item->setParentItem(m_parentItem);

        if (info.size.width() > 0)
            m_parentItem->setImplicitWidth(info.size.width());

        if (info.size.height() > 0)
            m_parentItem->setImplicitHeight(info.size.height());

        item->setWidth(m_parentItem->implicitWidth());
        item->setHeight(m_parentItem->implicitHeight());

        return false;
    }

    if (info.stage == StructureNodeStage::Start) {
        QQuickItem *item = !info.viewBox.isEmpty() ? new QQuickVectorImageGenerator::Utils::ViewBoxItem(info.viewBox) : new QQuickItem;
        addCurrentItem(item, info);
        if (info.size.width() > 0)
            m_parentItem->setImplicitWidth(info.size.width());

        if (info.size.height() > 0)
            m_parentItem->setImplicitHeight(info.size.height());

        item->setWidth(m_parentItem->implicitWidth());
        item->setHeight(m_parentItem->implicitHeight());
        generateNodeBase(info);

        if (!info.forceSeparatePaths && info.isPathContainer)
            generatePathContainer(info);
    } else {
        if (m_parentShapeItems.size()) {
            m_parentShapeItems.pop();
            m_items.pop();
        }

        m_items.pop();
    }

    return true;
}

QQuickItem *QQuickItemGenerator::currentItem()
{
    return m_items.top();
}

void QQuickItemGenerator::addCurrentItem(QQuickItem *item, const NodeInfo &info)
{
    item->setParentItem(currentItem());
    m_items.push(item);
    QStringView name = !info.nodeId.isEmpty() ? info.nodeId : info.typeName;
    item->setObjectName(name);
}

QT_END_NAMESPACE
