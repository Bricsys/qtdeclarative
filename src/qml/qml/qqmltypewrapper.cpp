// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmltypewrapper_p.h"

#include <private/qjsvalue_p.h>

#include <private/qqmlcontext_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlmetaobject_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmlvaluetypewrapper_p.h>

#include <private/qv4dateobject_p.h>
#include <private/qv4identifiertable_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4symbol_p.h>
#include <private/qv4urlobject_p.h>
#include <private/qv4variantobject_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(QQmlTypeWrapper);
DEFINE_OBJECT_VTABLE(QQmlTypeConstructor);
DEFINE_OBJECT_VTABLE(QQmlScopedEnumWrapper);


void Heap::QQmlTypeWrapper::init(TypeNameMode m, QObject *o, const QQmlTypePrivate *type)
{
    Q_ASSERT(type);
    FunctionObject::init();
    flags = quint8(m) | quint8(Type);
    object.init(o);
    QQmlType::refHandle(type);
    t.typePrivate = type;
}

void Heap::QQmlTypeWrapper::init(
        TypeNameMode m, QObject *o, QQmlTypeNameCache *type, const QQmlImportRef *import)
{
    Q_ASSERT(type);
    FunctionObject::init();
    flags = quint8(m) | quint8(Namespace);
    object.init(o);
    n.typeNamespace = type;
    n.typeNamespace->addref();
    n.importNamespace = import;
}

void Heap::QQmlTypeWrapper::destroy()
{
    switch (kind()) {
    case Type:
        Q_ASSERT(t.typePrivate);
        QQmlType::derefHandle(t.typePrivate);
        delete[] t.constructors;
        break;
    case Namespace:
        Q_ASSERT(n.typeNamespace);
        n.typeNamespace->release();
        break;
    }

    object.destroy();
    FunctionObject::destroy();
}

QQmlType Heap::QQmlTypeWrapper::type() const
{
    switch (kind()) {
    case Type:
        return QQmlType(t.typePrivate);
    case Namespace:
        return QQmlType();
    }

    Q_UNREACHABLE_RETURN(QQmlType());
}

QQmlTypeNameCache::Result Heap::QQmlTypeWrapper::queryNamespace(
        const QV4::String *name, QQmlEnginePrivate *enginePrivate) const
{
    Q_ASSERT(kind() == Namespace);
    Q_ASSERT(n.typeNamespace);
    Q_ASSERT(n.importNamespace);
    return n.typeNamespace->query(name, n.importNamespace, QQmlTypeLoader::get(enginePrivate));

}

template<typename Callback>
void warnWithLocation(const Heap::QQmlTypeWrapper *wrapper, Callback &&callback)
{
    auto log = qWarning().noquote().nospace();
    if (const CppStackFrame *frame = wrapper->internalClass->engine->currentStackFrame)
        log << frame->source() << ':' << frame->lineNumber() << ':';
    callback(log.space());
}

void Heap::QQmlTypeWrapper::warnIfUncreatable() const
{
    const QQmlType t = type();
    Q_ASSERT(t.isValid());

    if (t.isValueType())
        return;

    if (t.isSingleton()) {
        warnWithLocation(this, [&](QDebug &log) {
            log << "You are calling a Q_INVOKABLE constructor of" << t.typeName()
                << "which is a singleton in QML.";
        });
        return;
    }

    if (!t.isCreatable()) {
        warnWithLocation(this, [&](QDebug &log) {
            log << "You are calling a Q_INVOKABLE constructor of" << t.typeName()
                << "which is uncreatable in QML.";
        });
    }
}

bool QQmlTypeWrapper::isSingleton() const
{
    return d()->type().isSingleton();
}

const QMetaObject *QQmlTypeWrapper::metaObject() const
{
    const QQmlType type = d()->type();
    if (!type.isValid())
        return nullptr;

    if (type.isSingleton()) {
        auto metaObjectCandidate = type.metaObject();
        // if the candidate is the same as te baseMetaObject, we know that
        // we don't have an extended singleton; in that case the
        // actual instance might be subclass of type instead of type itself
        // so we need to query the actual object for it's meta-object
        if (metaObjectCandidate == type.baseMetaObject()) {
            QQmlEnginePrivate *qmlEngine = QQmlEnginePrivate::get(engine()->qmlEngine());
            auto object = qmlEngine->singletonInstance<QObject *>(type);
            if (object)
                return object->metaObject();
        }
        /* if we instead have an extended singleton, the dynamic proxy
           meta-object must alreday be set up correctly
          ### TODO: it isn't, as QQmlTypePrivate::init has no way to
                    query the object
        */
        return metaObjectCandidate;
    }

    return type.attachedPropertiesType(QQmlEnginePrivate::get(engine()->qmlEngine()));
}

QObject *QQmlTypeWrapper::object() const
{
    const QQmlType type = d()->type();
    if (!type.isValid())
        return nullptr;

    QQmlEnginePrivate *qmlEngine = QQmlEnginePrivate::get(engine()->qmlEngine());
    if (type.isSingleton())
        return qmlEngine->singletonInstance<QObject *>(type);

    return qmlAttachedPropertiesObject(
            d()->object,
            type.attachedPropertiesFunction(qmlEngine));
}

QObject* QQmlTypeWrapper::singletonObject() const
{
    if (!isSingleton())
        return nullptr;

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(engine()->qmlEngine());
    return e->singletonInstance<QObject*>(d()->type());
}

QVariant QQmlTypeWrapper::toVariant() const
{
    QQmlEnginePrivate *e = QQmlEnginePrivate::get(engine()->qmlEngine());
    const QQmlType type = d()->type();

    if (!isSingleton()) {
        return QVariant::fromValue(qmlAttachedPropertiesObject(
                d()->object, type.attachedPropertiesFunction(e)));
    }

    if (type.isQJSValueSingleton())
        return QVariant::fromValue<QJSValue>(e->singletonInstance<QJSValue>(type));

    return QVariant::fromValue<QObject*>(e->singletonInstance<QObject*>(type));
}

ReturnedValue QQmlTypeWrapper::method_hasInstance(
        const FunctionObject *, const Value *thisObject, const Value *argv, int argc)
{
    // we want to immediately call instanceOf rather than going through Function

    if (!argc)
        return Encode(false);
    if (const Object *o = thisObject->as<Object>())
        return o->instanceOf(argv[0]);
    return Encode(false);
}

ReturnedValue QQmlTypeWrapper::method_toString(
        const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const QQmlTypeWrapper *typeWrapper = thisObject->as<QQmlTypeWrapper>();
    if (!typeWrapper)
        RETURN_UNDEFINED();

    const QString name = typeWrapper->d()->type().qmlTypeName();
    return Encode(b->engine()->newString(name.isEmpty()
            ? QLatin1String("Unknown Type")
            : name));
}

void QQmlTypeWrapper::initProto(ExecutionEngine *v4)
{
    if (v4->typeWrapperPrototype()->d_unchecked())
        return;

    Scope scope(v4);
    ScopedObject o(scope, v4->newObject());

    o->defineDefaultProperty(v4->symbol_hasInstance(), method_hasInstance, 1, Attr_ReadOnly);
    o->defineDefaultProperty(v4->id_toString(), method_toString, 0);
    o->setPrototypeOf(v4->functionPrototype());

    v4->jsObjects[QV4::ExecutionEngine::TypeWrapperProto] = o->d();
}

// Returns a type wrapper for type t on o.  This allows access of enums, and attached properties.
ReturnedValue QQmlTypeWrapper::create(QV4::ExecutionEngine *engine, QObject *o, const QQmlType &t,
                                     Heap::QQmlTypeWrapper::TypeNameMode mode)
{
    Q_ASSERT(t.isValid());
    initProto(engine);

    QV4::MemoryManager *mm = engine->memoryManager;

    if (const QMetaObject *mo = t.metaObject(); !mo || mo->constructorCount() == 0)
        return mm->allocate<QQmlTypeWrapper>(mode, o, t.priv())->asReturnedValue();

    return mm->allocate<QQmlTypeConstructor>(mode, o, t.priv())->asReturnedValue();
}

// Returns a type wrapper for importNamespace (of t) on o.  This allows nested resolution of a type in a
// namespace.
ReturnedValue QQmlTypeWrapper::create(
        QV4::ExecutionEngine *engine, QObject *o, const QQmlRefPointer<QQmlTypeNameCache> &t,
        const QQmlImportRef *importNamespace, Heap::QQmlTypeWrapper::TypeNameMode mode)
{
    Q_ASSERT(t);
    Q_ASSERT(importNamespace);
    initProto(engine);

    Scope scope(engine);

    Scoped<QQmlTypeWrapper> w(scope, engine->memoryManager->allocate<QQmlTypeWrapper>(
                                             mode, o, t.data(), importNamespace));
    return w.asReturnedValue();
}

static int enumForSingleton(QQmlTypeLoader *typeLoader, String *name, const QQmlType &type, bool *ok)
{
    Q_ASSERT(ok != nullptr);
    const int value = type.enumValue(typeLoader, name, ok);
    return *ok ? value : -1;
}

ReturnedValue QQmlTypeWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    // Keep this code in sync with ::virtualResolveLookupGetter
    Q_ASSERT(m->as<QQmlTypeWrapper>());

    if (!id.isString())
        return Object::virtualGet(m, id, receiver, hasProperty);

    QV4::ExecutionEngine *v4 = m->engine();
    QV4::Scope scope(v4);
    ScopedString name(scope, id.asStringOrSymbol());

    Scoped<QQmlTypeWrapper> w(scope, static_cast<const QQmlTypeWrapper *>(m));

    if (hasProperty)
        *hasProperty = true;

    QQmlRefPointer<QQmlContextData> context = v4->callingQmlContext();

    QObject *object = w->d()->object;
    QQmlType type = w->d()->type();

    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(v4->qmlEngine());
    if (type.isValid()) {

        // singleton types are handled differently to other types.
        if (type.isSingleton()) {

            QJSValue scriptSingleton;
            if (type.isQObjectSingleton() || type.isCompositeSingleton()) {
                if (QObject *qobjectSingleton = enginePrivate->singletonInstance<QObject*>(type)) {
                    // check for enum value
                    const bool includeEnums
                            = w->d()->typeNameMode() == Heap::QQmlTypeWrapper::IncludeEnums;
                    if (includeEnums && name->startsWithUpper()) {
                        bool ok = false;
                        int value = enumForSingleton(&enginePrivate->typeLoader, name, type, &ok);
                        if (ok)
                            return QV4::Value::fromInt32(value).asReturnedValue();

                        value = type.scopedEnumIndex(&enginePrivate->typeLoader, name, &ok);
                        if (ok) {
                            Scoped<QQmlScopedEnumWrapper> enumWrapper(scope, v4->memoryManager->allocate<QQmlScopedEnumWrapper>());
                            enumWrapper->d()->typePrivate = type.priv();
                            QQmlType::refHandle(enumWrapper->d()->typePrivate);
                            enumWrapper->d()->scopeEnumIndex = value;
                            return enumWrapper.asReturnedValue();
                        }
                    }

                    // check for property.
                    bool ok;
                    const ReturnedValue result = QV4::QObjectWrapper::getQmlProperty(
                                v4, context, w->d(), qobjectSingleton, name,
                                QV4::QObjectWrapper::AttachMethods, &ok);
                    if (hasProperty)
                        *hasProperty = ok;

                    return result;
                }
            } else if (type.isQJSValueSingleton()) {
                QJSValue scriptSingleton = enginePrivate->singletonInstance<QJSValue>(type);
                if (!scriptSingleton.isUndefined()) {
                    // NOTE: if used in a binding, changes will not trigger re-evaluation since non-bindable.
                    QV4::ScopedObject o(scope, QJSValuePrivate::asReturnedValue(&scriptSingleton));
                    if (!!o)
                        return o->get(name);
                }
            }

            // Fall through to base implementation

        } else {

            if (name->startsWithUpper()) {
                bool ok = false;
                int value = type.enumValue(&enginePrivate->typeLoader, name, &ok);
                if (ok)
                    return QV4::Value::fromInt32(value).asReturnedValue();

                value = type.scopedEnumIndex(&enginePrivate->typeLoader, name, &ok);
                if (ok) {
                    Scoped<QQmlScopedEnumWrapper> enumWrapper(scope, v4->memoryManager->allocate<QQmlScopedEnumWrapper>());
                    enumWrapper->d()->typePrivate = type.priv();
                    QQmlType::refHandle(enumWrapper->d()->typePrivate);
                    enumWrapper->d()->scopeEnumIndex = value;
                    return enumWrapper.asReturnedValue();
                }

                // Fall through to base implementation

            } else if (w->d()->object) {
                QObject *ao = qmlAttachedPropertiesObject(
                        object,
                        type.attachedPropertiesFunction(QQmlEnginePrivate::get(v4->qmlEngine())));
                if (ao)
                    return QV4::QObjectWrapper::getQmlProperty(
                                v4, context, w->d(), ao, name, QV4::QObjectWrapper::AttachMethods,
                                hasProperty);

                // Fall through to base implementation
            }

            // Fall through to base implementation
        }

        // Fall through to base implementation

    } else if (w->d()->kind() == Heap::QQmlTypeWrapper::Namespace) {
        const QQmlTypeNameCache::Result r = w->d()->queryNamespace(name, enginePrivate);
        if (r.isValid()) {
            if (r.type.isValid()) {
                return create(scope.engine, object, r.type, w->d()->typeNameMode());
            } else if (r.scriptIndex != -1) {
                QV4::ScopedObject scripts(scope, context->importedScripts());
                return scripts->get(r.scriptIndex);
            } else if (r.importNamespace) {
                return create(scope.engine, object, context->imports(), r.importNamespace);
            }

            return QV4::Encode::undefined();

        }

        // Fall through to base implementation

    } else {
        Q_ASSERT(!"Unreachable");
    }

    bool ok = false;
    const ReturnedValue result = Object::virtualGet(m, id, receiver, &ok);
    if (hasProperty)
        *hasProperty = ok;

    return result;
}


bool QQmlTypeWrapper::virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver)
{
    if (!id.isString())
        return Object::virtualPut(m, id, value, receiver);


    Q_ASSERT(m->as<QQmlTypeWrapper>());
    QQmlTypeWrapper *w = static_cast<QQmlTypeWrapper *>(m);
    QV4::Scope scope(w);
    if (scope.hasException())
        return false;

    ScopedString name(scope, id.asStringOrSymbol());
    QQmlRefPointer<QQmlContextData> context = scope.engine->callingQmlContext();

    QQmlType type = w->d()->type();
    if (type.isValid() && !type.isSingleton() && w->d()->object) {
        QObject *object = w->d()->object;
        QQmlEngine *e = scope.engine->qmlEngine();
        QObject *ao = qmlAttachedPropertiesObject(
                object, type.attachedPropertiesFunction(QQmlEnginePrivate::get(e)));
        if (ao)
            return QV4::QObjectWrapper::setQmlProperty(
                        scope.engine, context, ao, name, QV4::QObjectWrapper::NoFlag, value);
        return false;
    } else if (type.isSingleton()) {
        QQmlEnginePrivate *e = QQmlEnginePrivate::get(scope.engine->qmlEngine());
        if (type.isQObjectSingleton() || type.isCompositeSingleton()) {
            if (QObject *qobjectSingleton = e->singletonInstance<QObject*>(type))
                return QV4::QObjectWrapper::setQmlProperty(
                            scope.engine, context, qobjectSingleton, name,
                            QV4::QObjectWrapper::NoFlag, value);

        } else {
            QJSValue scriptSingleton = e->singletonInstance<QJSValue>(type);
            if (!scriptSingleton.isUndefined()) {
                QV4::ScopedObject apiprivate(scope, QJSValuePrivate::asReturnedValue(&scriptSingleton));
                if (!apiprivate) {
                    QString error = QLatin1String("Cannot assign to read-only property \"") + name->toQString() + QLatin1Char('\"');
                    scope.engine->throwError(error);
                    return false;
                } else {
                    return apiprivate->put(name, value);
                }
            }
        }
    }

    return false;
}

PropertyAttributes QQmlTypeWrapper::virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p)
{
    if (id.isString()) {
        Scope scope(m);
        ScopedString n(scope, id.asStringOrSymbol());
        // ### Implement more efficiently.
        bool hasProperty = false;
        static_cast<const Object *>(m)->get(n, &hasProperty);
        return hasProperty ? Attr_Data : Attr_Invalid;
    }

    return QV4::Object::virtualGetOwnProperty(m, id, p);
}

bool QQmlTypeWrapper::virtualIsEqualTo(Managed *a, Managed *b)
{
    Q_ASSERT(a->as<QV4::QQmlTypeWrapper>());
    QV4::QQmlTypeWrapper *qmlTypeWrapperA = static_cast<QV4::QQmlTypeWrapper *>(a);
    if (QV4::QQmlTypeWrapper *qmlTypeWrapperB = b->as<QV4::QQmlTypeWrapper>())
        return qmlTypeWrapperA->toVariant() == qmlTypeWrapperB->toVariant();
    else if (QV4::QObjectWrapper *qobjectWrapper = b->as<QV4::QObjectWrapper>())
        return qmlTypeWrapperA->toVariant().value<QObject*>() == qobjectWrapper->object();

    return false;
}

static ReturnedValue instanceOfQObject(
        const QV4::QQmlTypeWrapper *typeWrapper, QObject *wrapperObject)
{
    QV4::ExecutionEngine *engine = typeWrapper->internalClass()->engine;
    // in case the wrapper outlived the QObject*
    if (!wrapperObject)
        return engine->throwTypeError();

    const QQmlType type = typeWrapper->d()->type();
    const QMetaType myTypeId = type.typeId();
    QQmlMetaObject myQmlType;
    if (!myTypeId.isValid()) {
        // we're a composite type; a composite type cannot be equal to a
        // non-composite object instance (Rectangle{} is never an instance of
        // CustomRectangle)
        QQmlData *theirDData = QQmlData::get(wrapperObject);
        Q_ASSERT(theirDData); // must exist, otherwise how do we have a QObjectWrapper for it?!
        if (!theirDData->compilationUnit)
            return Encode(false);

        QQmlEnginePrivate *qenginepriv = QQmlEnginePrivate::get(engine->qmlEngine());
        QQmlRefPointer<QQmlTypeData> td = qenginepriv->typeLoader.getType(typeWrapper->d()->type().sourceUrl());
        if (CompiledData::CompilationUnit *cu = td->compilationUnit())
            myQmlType = QQmlMetaType::metaObjectForType(cu->metaType());
        else
            return Encode(false); // It seems myQmlType has some errors, so we could not compile it.
    } else {
        myQmlType = QQmlMetaType::metaObjectForType(myTypeId);
        if (myQmlType.isNull())
            return Encode(false);
    }

    const QMetaObject *theirType = wrapperObject->metaObject();

    if (QQmlMetaObject::canConvert(theirType, myQmlType))
        return Encode(true);
    else if (type.isValueType())
        return Encode::undefined();
    else
        return Encode(false);
}

ReturnedValue QQmlTypeWrapper::virtualInstanceOf(const Object *typeObject, const Value &var)
{
    Q_ASSERT(typeObject->as<QV4::QQmlTypeWrapper>());
    const QV4::QQmlTypeWrapper *typeWrapper = static_cast<const QV4::QQmlTypeWrapper *>(typeObject);

    if (const QObjectWrapper *objectWrapper = var.as<QObjectWrapper>())
        return instanceOfQObject(typeWrapper, objectWrapper->object());

    if (const QQmlTypeWrapper *varTypeWrapper = var.as<QQmlTypeWrapper>()) {
        // Singleton or attachment
        if (QObject *varObject = varTypeWrapper->object())
            return instanceOfQObject(typeWrapper, varObject);
    }

    const QQmlType type = typeWrapper->d()->type();

    // If the target type is an object type  we want null.
    if (!type.isValueType())
        return Encode(false);

    const auto canCastValueType = [&]() -> bool {
        if (const QQmlValueTypeWrapper *valueWrapper = var.as<QQmlValueTypeWrapper>()) {
            return QQmlMetaObject::canConvert(
                    valueWrapper->metaObject(), type.metaObjectForValueType());
        }

        const QMetaType typeId = type.typeId();
        if (const VariantObject *variantObject = var.as<VariantObject>()) {
            if (variantObject->d()->data().metaType() == typeId)
                return true;
        }

        switch (typeId.id()) {
        case QMetaType::Void:
            return var.isUndefined();
        case QMetaType::QVariant:
            return true; // Everything is a var
        case QMetaType::Int:
            return var.isInteger();
        case QMetaType::Double:
            return var.isDouble(); // Integers are also doubles
        case QMetaType::QString:
            return var.isString();
        case QMetaType::Bool:
            return var.isBoolean();
        case QMetaType::QUrl:
            if (var.as<UrlObject>())
                return true;
            break;
        case QMetaType::QDate:
        case QMetaType::QTime:
        case QMetaType::QDateTime:
            if (var.as<DateObject>())
                return true;
            break;
        default:
            break;
        }

        return false;
    };

    // We want "foo as valuetype" to return undefined if it doesn't match.
    return canCastValueType() ? Encode(true) : Encode::undefined();
}

ReturnedValue QQmlTypeWrapper::virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup)
{
    // Keep this code in sync with ::virtualGet
    PropertyKey id = engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->runtimeStrings[lookup->nameIndex]);
    if (!id.isString())
        return Object::virtualResolveLookupGetter(object, engine, lookup);
    Scope scope(engine);

    const QQmlTypeWrapper *This = static_cast<const QQmlTypeWrapper *>(object);
    ScopedString name(scope, id.asStringOrSymbol());
    QQmlRefPointer<QQmlContextData> qmlContext = engine->callingQmlContext();

    Scoped<QQmlTypeWrapper> w(scope, static_cast<const QQmlTypeWrapper *>(This));
    QQmlType type = w->d()->type();

    if (type.isValid()) {

        if (type.isSingleton()) {
            QQmlEnginePrivate *e = QQmlEnginePrivate::get(engine->qmlEngine());
            if (type.isQObjectSingleton() || type.isCompositeSingleton()) {
                if (QObject *qobjectSingleton = e->singletonInstance<QObject*>(type)) {
                    const bool includeEnums
                            = w->d()->typeNameMode() == Heap::QQmlTypeWrapper::IncludeEnums;
                    if (!includeEnums || !name->startsWithUpper()) {
                        QQmlData *ddata = QQmlData::get(qobjectSingleton, false);
                        if (ddata && ddata->propertyCache) {
                            const QQmlPropertyData *property = ddata->propertyCache->property(name.getPointer(), qobjectSingleton, qmlContext);
                            if (property) {
                                ScopedValue val(scope, Value::fromReturnedValue(QV4::QObjectWrapper::wrap(engine, qobjectSingleton)));
                                if (qualifiesForMethodLookup(property)) {
                                    QV4::Heap::QObjectMethod *method = nullptr;
                                    setupQObjectMethodLookup(
                                                lookup, ddata, property, val->objectValue(), method);
                                    lookup->call = QV4::Lookup::Call::GetterSingletonMethod;
                                } else {
                                    setupQObjectLookup(
                                                lookup, ddata, property, val->objectValue(), This);
                                    lookup->call = QV4::Lookup::Call::GetterSingletonProperty;
                                }
                                return lookup->getter(engine, *object);
                            }
                            // Fall through to base implementation
                        }
                        // Fall through to base implementation
                    }
                    // Fall through to base implementation
                }
                // Fall through to base implementation
            }
            // Fall through to base implementation
        }

        if (name->startsWithUpper()) {
            bool ok = false;
            QQmlTypeLoader *typeLoader = &QQmlEnginePrivate::get(engine->qmlEngine())->typeLoader;
            int value = type.enumValue(typeLoader, name, &ok);
            if (ok) {
                lookup->qmlEnumValueLookup.ic.set(engine, This->internalClass());
                lookup->qmlEnumValueLookup.encodedEnumValue
                        = QV4::Value::fromInt32(value).asReturnedValue();
                lookup->call = QV4::Lookup::Call::GetterEnumValue;
                return lookup->getter(engine, *object);
            }

            value = type.scopedEnumIndex(typeLoader, name, &ok);
            if (ok) {
                Scoped<QQmlScopedEnumWrapper> enumWrapper(
                            scope, engine->memoryManager->allocate<QQmlScopedEnumWrapper>());
                enumWrapper->d()->typePrivate = type.priv();
                QQmlType::refHandle(enumWrapper->d()->typePrivate);
                enumWrapper->d()->scopeEnumIndex = value;

                lookup->qmlScopedEnumWrapperLookup.ic.set(engine, This->internalClass());
                lookup->qmlScopedEnumWrapperLookup.qmlScopedEnumWrapper.set(engine,
                                                                            static_cast<Heap::Object*>(enumWrapper->heapObject()));
                lookup->call = QV4::Lookup::Call::GetterScopedEnum;
                return enumWrapper.asReturnedValue();
            }
            // Fall through to base implementation
        } else if (w->d()->object) {
            QObject *ao = qmlAttachedPropertiesObject(
                    w->d()->object,
                    type.attachedPropertiesFunction(QQmlEnginePrivate::get(engine->qmlEngine())));
            if (ao) {
                // ### QTBUG-126877: Optimize this case
                lookup->call = QV4::Lookup::Call::GetterQObjectPropertyFallback;
                return lookup->getter(engine, *object);
            }
        }
        // Fall through to base implementation
    }
    /* ### QTBUG-126877: use QV4::Object::virtualResolveLookupGetter once we can be sure
       that we don't run into issues related to Function being our prototype  */
    lookup->call = QV4::Lookup::Call::GetterQObjectPropertyFallback;
    return lookup->getter(engine, *object);
}

bool QQmlTypeWrapper::virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value)
{
    return Object::virtualResolveLookupSetter(object, engine, lookup, value);
}

OwnPropertyKeyIterator *QQmlTypeWrapper::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    QV4::Scope scope(m->engine());
    QV4::Scoped<QQmlTypeWrapper> typeWrapper(scope, m);
    Q_ASSERT(typeWrapper);
    if (QObject *object = typeWrapper->object()) {
        QV4::Scoped<QV4::QObjectWrapper> objectWrapper(scope, QV4::QObjectWrapper::wrap(typeWrapper->engine(), object));
        return QV4::QObjectWrapper::virtualOwnPropertyKeys(objectWrapper, target);
    }

    return Object::virtualOwnPropertyKeys(m, target);
}

int QQmlTypeWrapper::virtualMetacall(Object *object, QMetaObject::Call call, int index, void **a)
{
    QQmlTypeWrapper *wrapper = object->as<QQmlTypeWrapper>();
    Q_ASSERT(wrapper);

    if (QObject *qObject = wrapper->object())
        return QMetaObject::metacall(qObject, call, index, a);

    return 0;
}

ReturnedValue QQmlTypeWrapper::lookupSingletonProperty(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [l, engine, &object]() {
        l->qobjectLookup.propertyCache->release();
        l->qobjectLookup.propertyCache = nullptr;
        l->call = QV4::Lookup::Call::GetterGeneric;
        return Lookup::getterGeneric(l, engine, object);
    };

    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());

    // The qmlTypeIc check is not strictly necessary.
    // If we have different ways to get to the same QObject type
    // we can use the same lookup to get its properties, no matter
    // how we've found the object. Most of the few times this check
    // fails, we will, of course have different object types. So
    // this check provides an early exit for the error case.
    //
    // So, if we ever need more bits in qobjectLookup, qmlTypeIc is the
    // member to be replaced.
    if (!o || o->internalClass != l->qobjectLookup.qmlTypeIc)
        return revertLookup();

    Heap::QQmlTypeWrapper *This = static_cast<Heap::QQmlTypeWrapper *>(o);

    QQmlType type = This->type();
    if (!type.isValid())
        return revertLookup();

    if (!type.isQObjectSingleton() && !type.isCompositeSingleton())
        return revertLookup();

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(engine->qmlEngine());
    QObject *qobjectSingleton = e->singletonInstance<QObject *>(type);
    Q_ASSERT(qobjectSingleton);

    Scope scope(engine);
    ScopedValue obj(scope, QV4::QObjectWrapper::wrap(engine, qobjectSingleton));
    const QObjectWrapper::Flags flags = l->forCall
            ? QObjectWrapper::AllowOverride
            : (QObjectWrapper::AttachMethods | QObjectWrapper::AllowOverride);
    return QObjectWrapper::lookupPropertyGetterImpl(l, engine, obj, flags, revertLookup);
}

ReturnedValue QQmlTypeWrapper::lookupSingletonMethod(Lookup *l, ExecutionEngine *engine, const Value &object)
{
    const auto revertLookup = [l, engine, &object]() {
        l->qobjectMethodLookup.propertyCache->release();
        l->qobjectMethodLookup.propertyCache = nullptr;
        l->call = QV4::Lookup::Call::GetterGeneric;
        return Lookup::getterGeneric(l, engine, object);
    };

    // We cannot safely cast here as we don't explicitly check the IC. Therefore as().
    const QQmlTypeWrapper *This = object.as<QQmlTypeWrapper>();
    if (!This)
        return revertLookup();

    QQmlType type = This->d()->type();
    if (!type.isValid())
        return revertLookup();

    if (!type.isQObjectSingleton() && !type.isCompositeSingleton())
        return revertLookup();

    QQmlEnginePrivate *e = QQmlEnginePrivate::get(engine->qmlEngine());
    QObject *qobjectSingleton = e->singletonInstance<QObject *>(type);
    Q_ASSERT(qobjectSingleton);

    Scope scope(engine);
    ScopedValue obj(scope, QV4::QObjectWrapper::wrap(engine, qobjectSingleton));
    return QObjectWrapper::lookupMethodGetterImpl(
                l, engine, obj, l->forCall ? QObjectWrapper::NoFlag : QObjectWrapper::AttachMethods,
                revertLookup);
}

ReturnedValue QQmlTypeWrapper::lookupEnumValue(Lookup *l, ExecutionEngine *engine, const Value &base)
{
    auto *o = static_cast<Heap::Object *>(base.heapObject());
    if (!o || o->internalClass != l->qmlEnumValueLookup.ic) {
        l->call = QV4::Lookup::Call::GetterGeneric;
        return Lookup::getterGeneric(l, engine, base);
    }

    return l->qmlEnumValueLookup.encodedEnumValue;
}

ReturnedValue QQmlTypeWrapper::lookupScopedEnum(Lookup *l, ExecutionEngine *engine, const Value &base)
{
    Scope scope(engine);
    Scoped<QQmlScopedEnumWrapper> enumWrapper(scope, static_cast<Heap::QQmlScopedEnumWrapper *>(
                l->qmlScopedEnumWrapperLookup.qmlScopedEnumWrapper.get()));

    auto *o = static_cast<Heap::Object *>(base.heapObject());
    if (!o || o->internalClass != l->qmlScopedEnumWrapperLookup.ic) {
        QQmlType::derefHandle(enumWrapper->d()->typePrivate);
        l->qmlScopedEnumWrapperLookup.qmlScopedEnumWrapper.clear();
        l->call = QV4::Lookup::Call::GetterGeneric;
        return Lookup::getterGeneric(l, engine, base);
    }

    return enumWrapper.asReturnedValue();
}

void Heap::QQmlScopedEnumWrapper::destroy()
{
    QQmlType::derefHandle(typePrivate);
    typePrivate = nullptr;
    Object::destroy();
}

QQmlType Heap::QQmlScopedEnumWrapper::type() const
{
    return QQmlType(typePrivate);
}

ReturnedValue QQmlScopedEnumWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    Q_ASSERT(m->as<QQmlScopedEnumWrapper>());
    if (!id.isString())
        return Object::virtualGet(m, id, receiver, hasProperty);

    const QQmlScopedEnumWrapper *resource = static_cast<const QQmlScopedEnumWrapper *>(m);
    QV4::ExecutionEngine *v4 = resource->engine();
    QV4::Scope scope(v4);
    ScopedString name(scope, id.asStringOrSymbol());

    QQmlType type = resource->d()->type();
    int index = resource->d()->scopeEnumIndex;

    bool ok = false;
    int value = type.scopedEnumValue(
            &QQmlEnginePrivate::get(v4->qmlEngine())->typeLoader, index, name, &ok);
    if (hasProperty)
        *hasProperty = ok;
    if (ok)
        return QV4::Value::fromInt32(value).asReturnedValue();

    return Encode::undefined();
}

QT_END_NAMESPACE
