/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qqmlproxymetaobject_p.h"
#include "qqmlproperty_p.h"

QT_BEGIN_NAMESPACE

QQmlProxyMetaObject::QQmlProxyMetaObject(QObject *obj, QList<ProxyData> *mList)
: metaObjects(mList), proxies(nullptr), parent(nullptr), object(obj)
{
    *static_cast<QMetaObject *>(this) = *metaObjects->constFirst().metaObject;

    QObjectPrivate *op = QObjectPrivate::get(obj);
    if (op->metaObject)
        parent = static_cast<QAbstractDynamicMetaObject*>(op->metaObject);

    op->metaObject = this;
}

QQmlProxyMetaObject::~QQmlProxyMetaObject()
{
    if (parent)
        delete parent;
    parent = nullptr;

    if (proxies)
        delete [] proxies;
    proxies = nullptr;
}

QObject *QQmlProxyMetaObject::getProxy(int index)
{
    if (!proxies) {
        if (!proxies) {
            proxies = new QObject*[metaObjects->count()];
            ::memset(proxies, 0,
                     sizeof(QObject *) * metaObjects->count());
        }
    }

    if (!proxies[index]) {
        const ProxyData &data = metaObjects->at(index);
        if (!data.createFunc)
            return nullptr;

        QObject *proxy = data.createFunc(object);
        const QMetaObject *metaObject = proxy->metaObject();
        proxies[index] = proxy;

        int localOffset = data.metaObject->methodOffset();
        int methodOffset = metaObject->methodOffset();
        int methods = metaObject->methodCount() - methodOffset;

        // ### - Can this be done more optimally?
        for (int jj = 0; jj < methods; ++jj) {
            QMetaMethod method =
                metaObject->method(jj + methodOffset);
            if (method.methodType() == QMetaMethod::Signal)
                QQmlPropertyPrivate::connect(proxy, methodOffset + jj, object, localOffset + jj);
        }
    }

    return proxies[index];
}

int QQmlProxyMetaObject::metaCall(QObject *o, QMetaObject::Call c, int id, void **a)
{
    Q_ASSERT(object == o);

    if ((c == QMetaObject::ReadProperty ||
        c == QMetaObject::WriteProperty) &&
            id >= metaObjects->constLast().propertyOffset) {

        for (int ii = 0; ii < metaObjects->count(); ++ii) {
            const int globalPropertyOffset = metaObjects->at(ii).propertyOffset;
            if (id >= globalPropertyOffset) {
                QObject *proxy = getProxy(ii);
                const int localProxyOffset = proxy->metaObject()->propertyOffset();
                const int localProxyId = id - globalPropertyOffset + localProxyOffset;

                return proxy->qt_metacall(c, localProxyId, a);
            }
        }
    } else if (c == QMetaObject::InvokeMetaMethod &&
               id >= metaObjects->constLast().methodOffset) {
        QMetaMethod m = object->metaObject()->method(id);
        if (m.methodType() == QMetaMethod::Signal) {
            QMetaObject::activate(object, id, a);
            return -1;
        } else {
            for (int ii = 0; ii < metaObjects->count(); ++ii) {
                const int globalMethodOffset = metaObjects->at(ii).methodOffset;
                if (id >= globalMethodOffset) {
                    QObject *proxy = getProxy(ii);

                    const int localMethodOffset = proxy->metaObject()->methodOffset();
                    const int localMethodId = id - globalMethodOffset + localMethodOffset;

                    return proxy->qt_metacall(c, localMethodId, a);
                }
            }
        }
    }

    if (parent)
        return parent->metaCall(o, c, id, a);
    else
        return object->qt_metacall(c, id, a);
}

QT_END_NAMESPACE
