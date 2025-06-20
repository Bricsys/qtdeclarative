// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qv4objectiterator_p.h"
#include "qv4object_p.h"
#include "qv4iterator_p.h"
#include "qv4propertykey_p.h"
#include <QtQml/private/qv4functionobject_p.h>

using namespace QV4;

void ForInIteratorPrototype::init(ExecutionEngine *)
{
    defineDefaultProperty(QStringLiteral("next"), method_next, 0);
}

PropertyKey ObjectIterator::next(Property *pd, PropertyAttributes *attrs)
{
    if (!object || !iterator)
        return PropertyKey::invalid();

    Scope scope(engine);
    ScopedPropertyKey key(scope);

    while (1) {
        key = iterator->next(object, pd, attrs);
        if (!key->isValid()) {
            object = nullptr;
            return key;
        }
        if ((!(flags & WithSymbols) && key->isSymbol()) ||
            ((flags & EnumerableOnly) && !attrs->isEnumerable()))
            continue;
        return key;
    }
}

ReturnedValue ObjectIterator::nextPropertyName(Value *value)
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Scope scope(engine);
    ScopedProperty p(scope);
    ScopedPropertyKey key(scope, next(p, &attrs));
    if (!key->isValid())
        return Encode::null();

    *value = object->getValue(p->value, attrs);
    if (key->isArrayIndex())
        return Encode(key->asArrayIndex());
    Q_ASSERT(key->isStringOrSymbol());
    return key->asStringOrSymbol()->asReturnedValue();
}

ReturnedValue ObjectIterator::nextPropertyNameAsString(Value *value)
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Scope scope(engine);
    ScopedProperty p(scope);
    ScopedPropertyKey key(scope, next(p, &attrs));
    if (!key->isValid())
        return Encode::null();

    *value = object->getValue(p->value, attrs);

    return key->toStringOrSymbol(engine)->asReturnedValue();
}

ReturnedValue ObjectIterator::nextPropertyNameAsString()
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Scope scope(engine);
    ScopedPropertyKey key(scope, next(nullptr, &attrs));
    if (!key->isValid())
        return Encode::null();

    return key->toStringOrSymbol(engine)->asReturnedValue();
}


DEFINE_OBJECT_VTABLE(ForInIteratorObject);

void Heap::ForInIteratorObject::markObjects(Heap::Base *that, MarkStack *markStack)
{
    ForInIteratorObject *o = static_cast<ForInIteratorObject *>(that);
    if (o->object)
        o->object->mark(markStack);
    if (o->current)
        o->current->mark(markStack);
    if (o->target)
        o->target->mark(markStack);
    o->workArea[0].mark(markStack);
    o->workArea[1].mark(markStack);
    Object::markObjects(that, markStack);
}

void Heap::ForInIteratorObject::destroy()
{
    delete iterator;
}

ReturnedValue ForInIteratorPrototype::method_next(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const ForInIteratorObject *forIn = static_cast<const ForInIteratorObject *>(thisObject);
    Q_ASSERT(forIn);
    Scope scope(b);

    ScopedPropertyKey key(scope, forIn->nextProperty());
    bool done = false;
    if (!key->isValid())
        done = true;
    ScopedStringOrSymbol s(scope, key->toStringOrSymbol(scope.engine));
    return IteratorPrototype::createIterResultObject(scope.engine, s, done);
}


PropertyKey ForInIteratorObject::nextProperty() const
{
    if (!d()->current)
        return PropertyKey::invalid();

    Scope scope(this);
    ScopedObject c(scope, d()->current);
    ScopedObject t(scope, d()->target);
    ScopedObject o(scope);
    ScopedProperty p(scope);
    ScopedPropertyKey key(scope);
    PropertyAttributes attrs;

    while (1) {
        while (1) {
            key = d()->iterator->next(t, p, &attrs);
            if (!key->isValid())
                break;
            if (!attrs.isEnumerable() || key->isSymbol())
                continue;
            // check the property is not already defined earlier in the proto chain
            if (d()->current != d()->object) {
                o = d()->object;
                bool shadowed = false;
                while (o && o->d() != c->heapObject()) {
                    if (o->getOwnProperty(key) != Attr_Invalid) {
                        shadowed = true;
                        break;
                    }
                    o = o->getPrototypeOf();
                }
                if (shadowed)
                    continue;
            }
            return key;
        }

        c = c->getPrototypeOf();
        d()->current.set(scope.engine, c ? c->d() : nullptr);
        if (!c)
            break;
        delete d()->iterator;
        d()->iterator = c->ownPropertyKeys(t.getRef());
        d()->target.set(scope.engine, t->d());
        if (!d()->iterator) {
            scope.engine->throwTypeError();
            return PropertyKey::invalid();
        }
    }
    return PropertyKey::invalid();
}
