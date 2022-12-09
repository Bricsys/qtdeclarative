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
#ifndef QV4ARRAYBUFFER_H
#define QV4ARRAYBUFFER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qarraydatapointer.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct SharedArrayBufferCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

struct ArrayBufferCtor : SharedArrayBufferCtor {
    void init(QV4::ExecutionContext *scope);
};

struct Q_QML_PRIVATE_EXPORT SharedArrayBuffer : Object {
    void init(size_t length);
    void init(const QByteArray& array);
    void destroy();
    struct { alignas(QArrayDataPointer<char>) unsigned char data[sizeof(QArrayDataPointer<char>)]; } d;
    const QArrayDataPointer<char> &data() const { return *reinterpret_cast<const QArrayDataPointer<char> *>(&d); }
    QArrayDataPointer<char> &data()  { return *reinterpret_cast<QArrayDataPointer<char> *>(&d); }
    bool isShared;

    uint byteLength() const { return data().size; }

    bool isDetachedBuffer() const { return data().isNull(); }
    bool isSharedArrayBuffer() const { return isShared; }
};

struct Q_QML_PRIVATE_EXPORT ArrayBuffer : SharedArrayBuffer {
    void init(size_t length) {
        SharedArrayBuffer::init(length);
        isShared = false;
    }
    void init(const QByteArray& array) {
        SharedArrayBuffer::init(array);
        isShared = false;
    }
    void detachArrayBuffer() {
        data().clear();
    }
};


}

struct SharedArrayBufferCtor : FunctionObject
{
    V4_OBJECT2(SharedArrayBufferCtor, FunctionObject)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct ArrayBufferCtor : SharedArrayBufferCtor
{
    V4_OBJECT2(ArrayBufferCtor, SharedArrayBufferCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);

    static ReturnedValue method_isView(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

struct Q_QML_PRIVATE_EXPORT SharedArrayBuffer : Object
{
    V4_OBJECT2(SharedArrayBuffer, Object)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(sharedArrayBufferPrototype)

    QByteArray asByteArray() const;
    uint byteLength() const { return d()->byteLength(); }
    char *data() { return d()->data()->data(); }
    const char *constData() { return d()->data()->data(); }

    bool isShared() { return d()->data()->isShared(); }
    bool isDetachedBuffer() const { return d()->data().isNull(); }
    bool isSharedArrayBuffer() const { return d()->isShared; }
};

struct Q_QML_PRIVATE_EXPORT ArrayBuffer : SharedArrayBuffer
{
    V4_OBJECT2(ArrayBuffer, SharedArrayBuffer)
    V4_NEEDS_DESTROY
    V4_PROTOTYPE(arrayBufferPrototype)

    QByteArray asByteArray() const;
    uint byteLength() const { return d()->byteLength(); }
    char *data() { if (d()->data().needsDetach()) detach(); return d()->data().data(); }
    // ### is that detach needed?
    const char *constData() const { return d()->data().data(); }

    bool isShared() { return d()->data()->isShared(); }
    void detach();
    void detachArrayBuffer() { d()->detachArrayBuffer(); }
};

struct SharedArrayBufferPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    static ReturnedValue slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc, bool shared);
};

struct ArrayBufferPrototype : SharedArrayBufferPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_get_byteLength(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_slice(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_toString(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
