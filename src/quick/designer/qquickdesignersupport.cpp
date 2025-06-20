// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupport_p.h"
#include <private/qquickitem_p.h>

#if QT_CONFIG(quick_shadereffect)
#include <QtQuick/private/qquickshadereffectsource_p.h>
#endif
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQml/private/qabstractanimationjob_p.h>
#include <private/qqmlengine_p.h>
#include <private/qquickview_p.h>
#include <QtQuick/private/qquickstategroup_p.h>
#include <QtGui/QImage>
#include <private/qqmlvme_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qqmldata_p.h>
#include <private/qsgadaptationlayer_p.h>

QT_BEGIN_NAMESPACE

QQuickDesignerSupport::QQuickDesignerSupport()
{
}

QQuickDesignerSupport::~QQuickDesignerSupport()
{
    typedef QHash<QQuickItem*, QSGLayer*>::iterator ItemTextureHashIt;

    for (ItemTextureHashIt iterator = m_itemTextureHash.begin(), end = m_itemTextureHash.end(); iterator != end; ++iterator) {
        QSGLayer *texture = iterator.value();
        QQuickItem *item = iterator.key();
        QQuickItemPrivate::get(item)->derefFromEffectItem(true);
        delete texture;
    }
}

void QQuickDesignerSupport::refFromEffectItem(QQuickItem *referencedItem, bool hide)
{
    if (referencedItem == nullptr)
        return;

    QQuickItemPrivate::get(referencedItem)->refFromEffectItem(hide);
    QQuickWindowPrivate::get(referencedItem->window())->updateDirtyNode(referencedItem);

    Q_ASSERT(QQuickItemPrivate::get(referencedItem)->rootNode());

    if (!m_itemTextureHash.contains(referencedItem)) {
        QSGRenderContext *rc = QQuickWindowPrivate::get(referencedItem->window())->context;
        QSGLayer *texture = rc->sceneGraphContext()->createLayer(rc);

        QSizeF itemSize = referencedItem->size();
        texture->setLive(true);
        texture->setItem(QQuickItemPrivate::get(referencedItem)->rootNode());
        texture->setRect(QRectF(QPointF(0, 0), itemSize));
        texture->setSize(itemSize.toSize());
        texture->setRecursive(true);
        texture->setFormat(QSGLayer::RGBA8);
        texture->setHasMipmaps(false);

        m_itemTextureHash.insert(referencedItem, texture);
    }
}

void QQuickDesignerSupport::derefFromEffectItem(QQuickItem *referencedItem, bool unhide)
{
    if (referencedItem == nullptr)
        return;

    delete m_itemTextureHash.take(referencedItem);
    QQuickItemPrivate::get(referencedItem)->derefFromEffectItem(unhide);
}

QImage QQuickDesignerSupport::renderImageForItem(QQuickItem *referencedItem, const QRectF &boundingRect, const QSize &imageSize)
{
    if (referencedItem == nullptr || referencedItem->parentItem() == nullptr) {
        qDebug() << __FILE__ << __LINE__ << "Warning: Item can be rendered.";
        return QImage();
    }

    QSGLayer *renderTexture = m_itemTextureHash.value(referencedItem);

    Q_ASSERT(renderTexture);
    if (renderTexture == nullptr)
         return QImage();
    renderTexture->setRect(boundingRect);
    renderTexture->setSize(imageSize);
    renderTexture->setItem(QQuickItemPrivate::get(referencedItem)->rootNode());
    renderTexture->markDirtyTexture();
    renderTexture->updateTexture();

    QImage renderImage = renderTexture->toImage();
    renderImage.flip();

    if (renderImage.size().isEmpty())
        qDebug() << __FILE__ << __LINE__ << "Warning: Image is empty.";

    return renderImage;
}

bool QQuickDesignerSupport::isDirty(QQuickItem *referencedItem, DirtyType dirtyType)
{
    if (referencedItem == nullptr)
        return false;

    return QQuickItemPrivate::get(referencedItem)->dirtyAttributes & dirtyType;
}

void QQuickDesignerSupport::addDirty(QQuickItem *referencedItem, QQuickDesignerSupport::DirtyType dirtyType)
{
    if (referencedItem == nullptr)
        return;

    QQuickItemPrivate::get(referencedItem)->dirtyAttributes |= dirtyType;
}

void QQuickDesignerSupport::resetDirty(QQuickItem *referencedItem)
{
    if (referencedItem == nullptr)
        return;

    QQuickItemPrivate::get(referencedItem)->dirtyAttributes = 0x0;
    QQuickItemPrivate::get(referencedItem)->removeFromDirtyList();
}

QTransform QQuickDesignerSupport::windowTransform(QQuickItem *referencedItem)
{
    if (referencedItem == nullptr)
        return QTransform();

    return QQuickItemPrivate::get(referencedItem)->itemToWindowTransform();
}

QTransform QQuickDesignerSupport::parentTransform(QQuickItem *referencedItem)
{
    if (referencedItem == nullptr)
        return QTransform();

    QTransform parentTransform;

    QQuickItemPrivate::get(referencedItem)->itemToParentTransform(&parentTransform);

    return parentTransform;
}

QString propertyNameForAnchorLine(const QQuickAnchors::Anchor &anchorLine)
{
    switch (anchorLine) {
        case QQuickAnchors::LeftAnchor: return QLatin1String("left");
        case QQuickAnchors::RightAnchor: return QLatin1String("right");
        case QQuickAnchors::TopAnchor: return QLatin1String("top");
        case QQuickAnchors::BottomAnchor: return QLatin1String("bottom");
        case QQuickAnchors::HCenterAnchor: return QLatin1String("horizontalCenter");
        case QQuickAnchors::VCenterAnchor: return QLatin1String("verticalCenter");
        case QQuickAnchors::BaselineAnchor: return QLatin1String("baseline");
        case QQuickAnchors::InvalidAnchor: // fallthrough:
        default: return QString();
    }
}

bool isValidAnchorName(const QString &name)
{
    static QStringList anchorNameList(QStringList() << QLatin1String("anchors.top")
                                                    << QLatin1String("anchors.left")
                                                    << QLatin1String("anchors.right")
                                                    << QLatin1String("anchors.bottom")
                                                    << QLatin1String("anchors.verticalCenter")
                                                    << QLatin1String("anchors.horizontalCenter")
                                                    << QLatin1String("anchors.fill")
                                                    << QLatin1String("anchors.centerIn")
                                                    << QLatin1String("anchors.baseline"));

    return anchorNameList.contains(name);
}

bool QQuickDesignerSupport::isAnchoredTo(QQuickItem *fromItem, QQuickItem *toItem)
{
    QQuickItemPrivate *fromItemPrivate = QQuickItemPrivate::get(fromItem);
    QQuickAnchors *anchors = fromItemPrivate->anchors();
    return anchors->fill() == toItem
            || anchors->centerIn() == toItem
            || anchors->bottom().item == toItem
            || anchors->top().item == toItem
            || anchors->left().item == toItem
            || anchors->right().item == toItem
            || anchors->verticalCenter().item == toItem
            || anchors->horizontalCenter().item == toItem
            || anchors->baseline().item == toItem;
}

bool QQuickDesignerSupport::areChildrenAnchoredTo(QQuickItem *fromItem, QQuickItem *toItem)
{
    const auto childItems = fromItem->childItems();
    for (QQuickItem *childItem : childItems) {
        if (childItem) {
            if (isAnchoredTo(childItem, toItem))
                return true;

            if (areChildrenAnchoredTo(childItem, toItem))
                return true;
        }
    }

    return false;
}

QQuickAnchors *anchors(QQuickItem *item)
{
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    return itemPrivate->anchors();
}

QQuickAnchors::Anchor anchorLineFlagForName(const QString &name)
{
    if (name == QLatin1String("anchors.top"))
        return QQuickAnchors::TopAnchor;

    if (name == QLatin1String("anchors.left"))
        return QQuickAnchors::LeftAnchor;

    if (name == QLatin1String("anchors.bottom"))
         return QQuickAnchors::BottomAnchor;

    if (name == QLatin1String("anchors.right"))
        return QQuickAnchors::RightAnchor;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return QQuickAnchors::HCenterAnchor;

    if (name == QLatin1String("anchors.verticalCenter"))
         return QQuickAnchors::VCenterAnchor;

    if (name == QLatin1String("anchors.baseline"))
         return QQuickAnchors::BaselineAnchor;


    Q_ASSERT_X(false, Q_FUNC_INFO, "wrong anchor name - this should never happen");
    return QQuickAnchors::LeftAnchor;
}

bool QQuickDesignerSupport::hasAnchor(QQuickItem *item, const QString &name)
{
    if (!isValidAnchorName(name))
        return false;

    if (name == QLatin1String("anchors.fill"))
        return anchors(item)->fill() != nullptr;

    if (name == QLatin1String("anchors.centerIn"))
        return anchors(item)->centerIn() != nullptr;

    if (name == QLatin1String("anchors.right"))
        return anchors(item)->right().item != nullptr;

    if (name == QLatin1String("anchors.top"))
        return anchors(item)->top().item != nullptr;

    if (name == QLatin1String("anchors.left"))
        return anchors(item)->left().item != nullptr;

    if (name == QLatin1String("anchors.bottom"))
        return anchors(item)->bottom().item != nullptr;

    if (name == QLatin1String("anchors.horizontalCenter"))
        return anchors(item)->horizontalCenter().item != nullptr;

    if (name == QLatin1String("anchors.verticalCenter"))
        return anchors(item)->verticalCenter().item != nullptr;

    if (name == QLatin1String("anchors.baseline"))
        return anchors(item)->baseline().item != nullptr;

    return anchors(item)->usedAnchors().testFlag(anchorLineFlagForName(name));
}

QQuickItem *QQuickDesignerSupport::anchorFillTargetItem(QQuickItem *item)
{
    return anchors(item)->fill();
}

QQuickItem *QQuickDesignerSupport::anchorCenterInTargetItem(QQuickItem *item)
{
    return anchors(item)->centerIn();
}



std::pair<QString, QObject*> QQuickDesignerSupport::anchorLineTarget(QQuickItem *item, const QString &name, QQmlContext *context)
{
    QObject *targetObject = nullptr;
    QString targetName;

    if (name == QLatin1String("anchors.fill")) {
        targetObject = anchors(item)->fill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        targetObject = anchors(item)->centerIn();
    } else {
        QQmlProperty metaProperty(item, name, context);
        if (!metaProperty.isValid())
            return std::pair<QString, QObject*>();

        QQuickAnchorLine anchorLine = metaProperty.read().value<QQuickAnchorLine>();
        if (anchorLine.anchorLine != QQuickAnchors::InvalidAnchor) {
            targetObject = anchorLine.item;
            targetName = propertyNameForAnchorLine(anchorLine.anchorLine);
        }

    }

    return std::pair<QString, QObject*>(targetName, targetObject);
}

void QQuickDesignerSupport::resetAnchor(QQuickItem *item, const QString &name)
{
    if (name == QLatin1String("anchors.fill")) {
        anchors(item)->resetFill();
    } else if (name == QLatin1String("anchors.centerIn")) {
        anchors(item)->resetCenterIn();
    } else if (name == QLatin1String("anchors.top")) {
        anchors(item)->resetTop();
    } else if (name == QLatin1String("anchors.left")) {
        anchors(item)->resetLeft();
    } else if (name == QLatin1String("anchors.right")) {
        anchors(item)->resetRight();
    } else if (name == QLatin1String("anchors.bottom")) {
        anchors(item)->resetBottom();
    } else if (name == QLatin1String("anchors.horizontalCenter")) {
        anchors(item)->resetHorizontalCenter();
    } else if (name == QLatin1String("anchors.verticalCenter")) {
        anchors(item)->resetVerticalCenter();
    } else if (name == QLatin1String("anchors.baseline")) {
        anchors(item)->resetBaseline();
    }
}

void QQuickDesignerSupport::emitComponentCompleteSignalForAttachedProperty(QObject *object)
{
    if (!object)
        return;

    QQmlData *data = QQmlData::get(object);
    if (data && data->context) {
        QQmlComponentAttached *componentAttached = data->context->componentAttacheds();
        while (componentAttached) {
            if (componentAttached->parent())
                if (componentAttached->parent() == object)
                    emit componentAttached->completed();

            componentAttached = componentAttached->next();
        }
    }
}

QList<QObject*> QQuickDesignerSupport::statesForItem(QQuickItem *item)
{
    QList<QObject*> objectList;
    const QList<QQuickState *> stateList = QQuickItemPrivate::get(item)->_states()->states();

    objectList.reserve(stateList.size());
    for (QQuickState* state : stateList)
        objectList.append(state);

    return objectList;
}

bool QQuickDesignerSupport::isComponentComplete(QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->componentComplete;
}

int QQuickDesignerSupport::borderWidth(QQuickItem *item)
{
    QQuickRectangle *rectangle = qobject_cast<QQuickRectangle*>(item);
    if (rectangle)
        return rectangle->border()->width();

    return 0;
}

void QQuickDesignerSupport::refreshExpressions(QQmlContext *context)
{
    QQmlContextData::get(context)->refreshExpressions();
}

void QQuickDesignerSupport::setRootItem(QQuickView *view, QQuickItem *item)
{
    QQuickViewPrivate::get(view)->setRootObject(item);
}

bool QQuickDesignerSupport::isValidWidth(QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->heightValid();
}

bool QQuickDesignerSupport::isValidHeight(QQuickItem *item)
{
    return QQuickItemPrivate::get(item)->widthValid();
}

void QQuickDesignerSupport::updateDirtyNode(QQuickItem *item)
{
    if (item->window())
        QQuickWindowPrivate::get(item->window())->updateDirtyNode(item);
}

void QQuickDesignerSupport::activateDesignerMode()
{
    QQmlEnginePrivate::activateDesignerMode();
}

void QQuickDesignerSupport::disableComponentComplete()
{
    QQmlVME::disableComponentComplete();
}

void QQuickDesignerSupport::enableComponentComplete()
{
    QQmlVME::enableComponentComplete();
}

void QQuickDesignerSupport::polishItems(QQuickWindow *window)
{
    QQuickWindowPrivate::get(window)->polishItems();
}

ComponentCompleteDisabler::ComponentCompleteDisabler()
{
    QQuickDesignerSupport::disableComponentComplete();
}

ComponentCompleteDisabler::~ComponentCompleteDisabler()
{
    QQuickDesignerSupport::enableComponentComplete();
}


QT_END_NAMESPACE
