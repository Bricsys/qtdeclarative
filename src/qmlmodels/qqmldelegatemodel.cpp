// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldelegatemodel_p_p.h"

#include <QtCore/private/qabstractitemmodel_p.h>

#include <QtQml/qqmlinfo.h>

#include <private/qqmlabstractdelegatecomponent_p.h>
#include <private/qquickpackage_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include <private/qqmladaptormodel_p.h>
#include <private/qqmlanybinding_p.h>
#include <private/qqmlchangeset_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlpropertytopropertybinding_p.h>
#include <private/qjsvalue_p.h>
#include <QtCore/private/qcoreapplication_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectiterator_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcItemViewDelegateRecycling, "qt.qml.delegatemodel.recycling")

class QQmlDelegateModelItem;

namespace QV4 {

namespace Heap {

struct DelegateModelGroupFunction : FunctionObject {
    void init(ExecutionEngine *engine, uint flag, QV4::ReturnedValue (*code)(QQmlDelegateModelItem *item, uint flag, const QV4::Value &arg));

    QV4::ReturnedValue (*code)(QQmlDelegateModelItem *item, uint flag, const QV4::Value &arg);
    uint flag;
};

struct QQmlDelegateModelGroupChange : Object {
    void init() { Object::init(); }

    QQmlChangeSet::ChangeData change;
};

struct QQmlDelegateModelGroupChangeArray : Object {
    void init(const QVector<QQmlChangeSet::Change> &changes);
    void destroy() {
        delete changes;
        Object::destroy();
    }

    QVector<QQmlChangeSet::Change> *changes;
};


}

struct DelegateModelGroupFunction : QV4::FunctionObject
{
    V4_OBJECT2(DelegateModelGroupFunction, FunctionObject)

    static Heap::DelegateModelGroupFunction *create(
            QV4::ExecutionEngine *engine, uint flag,
            QV4::ReturnedValue (*code)(QQmlDelegateModelItem *, uint, const QV4::Value &))
    {
        return engine->memoryManager->allocate<DelegateModelGroupFunction>(engine, flag, code);
    }

    static ReturnedValue virtualCall(const QV4::FunctionObject *that, const Value *thisObject, const Value *argv, int argc)
    {
        QV4::Scope scope(that->engine());
        QV4::Scoped<DelegateModelGroupFunction> f(scope, static_cast<const DelegateModelGroupFunction *>(that));
        QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject);
        if (!o)
            return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

        QV4::ScopedValue v(scope, argc ? argv[0] : Value::undefinedValue());
        return f->d()->code(o->d()->item, f->d()->flag, v);
    }
};

void Heap::DelegateModelGroupFunction::init(
        QV4::ExecutionEngine *engine, uint flag,
        QV4::ReturnedValue (*code)(QQmlDelegateModelItem *item, uint flag, const QV4::Value &arg))
{
    QV4::Heap::FunctionObject::init(engine, QStringLiteral("DelegateModelGroupFunction"));
    this->flag = flag;
    this->code = code;
}

}

DEFINE_OBJECT_VTABLE(QV4::DelegateModelGroupFunction);



class QQmlDelegateModelEngineData : public QV4::ExecutionEngine::Deletable
{
public:
    QQmlDelegateModelEngineData(QV4::ExecutionEngine *v4);
    ~QQmlDelegateModelEngineData();

    QV4::ReturnedValue array(QV4::ExecutionEngine *engine,
                             const QVector<QQmlChangeSet::Change> &changes);

    QV4::PersistentValue changeProto;
};

V4_DEFINE_EXTENSION(QQmlDelegateModelEngineData, qdmEngineData)


void QQmlDelegateModelPartsMetaObject::propertyCreated(int, QMetaPropertyBuilder &prop)
{
    prop.setWritable(false);
}

QVariant QQmlDelegateModelPartsMetaObject::initialValue(int id)
{
    QQmlDelegateModelParts *parts = static_cast<QQmlDelegateModelParts *>(object());
    QQmlPartsModel *m = new QQmlPartsModel(
            parts->model, QString::fromUtf8(name(id)), parts);
    parts->models.append(m);
    return QVariant::fromValue(static_cast<QObject *>(m));
}

QQmlDelegateModelParts::QQmlDelegateModelParts(QQmlDelegateModel *parent)
: QObject(parent), model(parent)
{
    new QQmlDelegateModelPartsMetaObject(this);
}

//---------------------------------------------------------------------------

/*!
    \qmltype DelegateModel
//!    \nativetype QQmlDelegateModel
    \inqmlmodule QtQml.Models
    \brief Encapsulates a model and delegate.

    The DelegateModel type encapsulates a model and the delegate that will
    be instantiated for items in the model.

    It is usually not necessary to create a DelegateModel.
    However, it can be useful for manipulating and accessing the \l modelIndex
    when a QAbstractItemModel subclass is used as the
    model. Also, DelegateModel is used together with \l Package to
    provide delegates to multiple views, and with DelegateModelGroup to sort and filter
    delegate items.

    DelegateModel only supports one-dimensional models -- assigning a table model to
    DelegateModel and that to TableView will thus only show one column.

    The example below illustrates using a DelegateModel with a ListView.

    \snippet delegatemodel/delegatemodel.qml 0
*/

QQmlDelegateModelPrivate::QQmlDelegateModelPrivate(QQmlContext *ctxt)
    : m_delegateChooser(nullptr)
    , m_context(ctxt)
    , m_parts(nullptr)
    , m_filterGroup(QStringLiteral("items"))
    , m_count(0)
    , m_groupCount(Compositor::MinimumGroupCount)
    , m_compositorGroup(Compositor::Cache)
    , m_complete(false)
    , m_delegateValidated(false)
    , m_reset(false)
    , m_transaction(false)
    , m_incubatorCleanupScheduled(false)
    , m_waitingToFetchMore(false)
    , m_cacheItems(nullptr)
    , m_items(nullptr)
    , m_persistedItems(nullptr)
{
}

QQmlDelegateModelPrivate::~QQmlDelegateModelPrivate()
{
    qDeleteAll(m_finishedIncubating);

    // Free up all items in the pool
    drainReusableItemsPool(0);
}

int QQmlDelegateModelPrivate::adaptorModelCount() const
{
    // QQmlDelegateModel currently only support list models.
    // So even if a model is a table model, only the first
    // column will be used.
    return m_adaptorModel.rowCount();
}

void QQmlDelegateModelPrivate::requestMoreIfNecessary()
{
    Q_Q(QQmlDelegateModel);
    if (!m_waitingToFetchMore && m_adaptorModel.canFetchMore()) {
        m_waitingToFetchMore = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
    }
}

void QQmlDelegateModelPrivate::init()
{
    Q_Q(QQmlDelegateModel);
    m_compositor.setRemoveGroups(Compositor::GroupMask & ~Compositor::PersistedFlag);

    m_items = new QQmlDelegateModelGroup(QStringLiteral("items"), q, Compositor::Default, q);
    m_items->setDefaultInclude(true);
    m_persistedItems = new QQmlDelegateModelGroup(QStringLiteral("persistedItems"), q, Compositor::Persisted, q);
    QQmlDelegateModelGroupPrivate::get(m_items)->emitters.insert(this);
}

QQmlDelegateModel::QQmlDelegateModel()
    : QQmlDelegateModel(nullptr, nullptr)
{
}

QQmlDelegateModel::QQmlDelegateModel(QQmlContext *ctxt, QObject *parent)
: QQmlInstanceModel(*(new QQmlDelegateModelPrivate(ctxt)), parent)
{
    Q_D(QQmlDelegateModel);
    d->init();
}

QQmlDelegateModel::~QQmlDelegateModel()
{
    Q_D(QQmlDelegateModel);
    d->disconnectFromAbstractItemModel();
    d->m_adaptorModel.setObject(nullptr);

    for (QQmlDelegateModelItem *cacheItem : std::as_const(d->m_cache)) {
        if (cacheItem->object) {
            delete cacheItem->object;

            cacheItem->object = nullptr;
            cacheItem->contextData.reset();
            cacheItem->scriptRef -= 1;
        } else if (cacheItem->incubationTask) {
            // Both the incubationTask and the object may hold a scriptRef,
            // but if both are present, only one scriptRef is held in total.
            cacheItem->scriptRef -= 1;
        }

        cacheItem->groups &= ~Compositor::UnresolvedFlag;
        cacheItem->objectRef = 0;

        if (cacheItem->incubationTask) {
            d->releaseIncubator(cacheItem->incubationTask);
            cacheItem->incubationTask->vdm = nullptr;
            cacheItem->incubationTask = nullptr;
        }

        if (!cacheItem->isReferenced())
            delete cacheItem;
    }
}


void QQmlDelegateModel::classBegin()
{
    Q_D(QQmlDelegateModel);
    if (!d->m_context)
        d->m_context = qmlContext(this);
}

void QQmlDelegateModel::componentComplete()
{
    Q_D(QQmlDelegateModel);
    d->m_complete = true;

    int defaultGroups = 0;
    QStringList groupNames;
    groupNames.append(QStringLiteral("items"));
    groupNames.append(QStringLiteral("persistedItems"));
    if (QQmlDelegateModelGroupPrivate::get(d->m_items)->defaultInclude)
        defaultGroups |= Compositor::DefaultFlag;
    if (QQmlDelegateModelGroupPrivate::get(d->m_persistedItems)->defaultInclude)
        defaultGroups |= Compositor::PersistedFlag;
    for (int i = Compositor::MinimumGroupCount; i < d->m_groupCount; ++i) {
        QString name = d->m_groups[i]->name();
        if (name.isEmpty()) {
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else if (name.at(0).isUpper()) {
            qmlWarning(d->m_groups[i]) << QQmlDelegateModelGroup::tr("Group names must start with a lower case letter");
            d->m_groups[i] = d->m_groups[d->m_groupCount - 1];
            --d->m_groupCount;
            --i;
        } else {
            groupNames.append(name);

            QQmlDelegateModelGroupPrivate *group = QQmlDelegateModelGroupPrivate::get(d->m_groups[i]);
            group->setModel(this, Compositor::Group(i));
            if (group->defaultInclude)
                defaultGroups |= (1 << i);
        }
    }

    d->m_cacheMetaType = QQml::makeRefPointer<QQmlDelegateModelItemMetaType>(
                d->m_context->engine()->handle(), this, groupNames);

    d->m_compositor.setGroupCount(d->m_groupCount);
    d->m_compositor.setDefaultGroups(defaultGroups);
    d->updateFilterGroup();

    while (!d->m_pendingParts.isEmpty())
        static_cast<QQmlPartsModel *>(d->m_pendingParts.first())->updateFilterGroup();

    QVector<Compositor::Insert> inserts;
    d->m_count = d->adaptorModelCount();
    d->m_compositor.append(
            &d->m_adaptorModel,
            0,
            d->m_count,
            defaultGroups | Compositor::AppendFlag | Compositor::PrependFlag,
            &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();
    d->requestMoreIfNecessary();
}

/*!
    \qmlproperty model QtQml.Models::DelegateModel::model
    This property holds the model providing data for the DelegateModel.

    The model provides a set of data that is used to create the items
    for a view.  For large or dynamic datasets the model is usually
    provided by a C++ model object.  The C++ model object must be a \l
    {QAbstractItemModel} subclass or a simple list.

    Models can also be created directly in QML, for example using
    ListModel.

    \sa {qml-data-models}{Data Models}
    \keyword dm-model-property
*/
QVariant QQmlDelegateModel::model() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_adaptorModel.model();
}

void QQmlDelegateModelPrivate::connectToAbstractItemModel()
{
    Q_Q(QQmlDelegateModel);
    if (!m_adaptorModel.adaptsAim())
        return;

    auto aim = m_adaptorModel.aim();

    QObject::connect(aim, &QAbstractItemModel::rowsInserted, q, &QQmlDelegateModel::_q_rowsInserted);
    QObject::connect(aim, &QAbstractItemModel::rowsRemoved, q, &QQmlDelegateModel::_q_rowsRemoved);
    QObject::connect(aim, &QAbstractItemModel::rowsAboutToBeRemoved, q, &QQmlDelegateModel::_q_rowsAboutToBeRemoved);
    QObject::connect(aim, &QAbstractItemModel::columnsInserted, q, &QQmlDelegateModel::_q_columnsInserted);
    QObject::connect(aim, &QAbstractItemModel::columnsRemoved, q, &QQmlDelegateModel::_q_columnsRemoved);
    QObject::connect(aim, &QAbstractItemModel::columnsMoved, q, &QQmlDelegateModel::_q_columnsMoved);
    QObject::connect(aim, &QAbstractItemModel::dataChanged, q, &QQmlDelegateModel::_q_dataChanged);
    QObject::connect(aim, &QAbstractItemModel::rowsMoved, q, &QQmlDelegateModel::_q_rowsMoved);
    QObject::connect(aim, &QAbstractItemModel::modelAboutToBeReset, q, &QQmlDelegateModel::_q_modelAboutToBeReset);
    QObject::connect(aim, &QAbstractItemModel::layoutChanged, q, &QQmlDelegateModel::_q_layoutChanged);
}

void QQmlDelegateModelPrivate::disconnectFromAbstractItemModel()
{
    Q_Q(QQmlDelegateModel);
    if (!m_adaptorModel.adaptsAim())
        return;

    auto aim = m_adaptorModel.aim();

    QObject::disconnect(aim, &QAbstractItemModel::rowsInserted, q, &QQmlDelegateModel::_q_rowsInserted);
    QObject::disconnect(aim, &QAbstractItemModel::rowsAboutToBeRemoved, q, &QQmlDelegateModel::_q_rowsAboutToBeRemoved);
    QObject::disconnect(aim, &QAbstractItemModel::rowsRemoved, q, &QQmlDelegateModel::_q_rowsRemoved);
    QObject::disconnect(aim, &QAbstractItemModel::columnsInserted, q, &QQmlDelegateModel::_q_columnsInserted);
    QObject::disconnect(aim, &QAbstractItemModel::columnsRemoved, q, &QQmlDelegateModel::_q_columnsRemoved);
    QObject::disconnect(aim, &QAbstractItemModel::columnsMoved, q, &QQmlDelegateModel::_q_columnsMoved);
    QObject::disconnect(aim, &QAbstractItemModel::dataChanged, q, &QQmlDelegateModel::_q_dataChanged);
    QObject::disconnect(aim, &QAbstractItemModel::rowsMoved, q, &QQmlDelegateModel::_q_rowsMoved);
    QObject::disconnect(aim, &QAbstractItemModel::modelAboutToBeReset, q, &QQmlDelegateModel::_q_modelAboutToBeReset);
    QObject::disconnect(aim, &QAbstractItemModel::layoutChanged, q, &QQmlDelegateModel::_q_layoutChanged);
}

void QQmlDelegateModel::setModel(const QVariant &model)
{
    Q_D(QQmlDelegateModel);

    if (d->m_complete)
        _q_itemsRemoved(0, d->m_count);

    d->disconnectFromAbstractItemModel();
    d->m_adaptorModel.setModel(model);
    d->connectToAbstractItemModel();

    d->m_adaptorModel.replaceWatchedRoles(QList<QByteArray>(), d->m_watchedRoles);
    for (int i = 0; d->m_parts && i < d->m_parts->models.size(); ++i) {
        d->m_adaptorModel.replaceWatchedRoles(
                QList<QByteArray>(), d->m_parts->models.at(i)->watchedRoles());
    }

    if (d->m_complete) {
        _q_itemsInserted(0, d->adaptorModelCount());
        d->requestMoreIfNecessary();
    }

    // Since 837c2f18cd223707e7cedb213257b0158ea07146, we connect to modelAboutToBeReset
    // rather than modelReset so that we can handle role name changes. _q_modelAboutToBeReset
    // now connects modelReset to handleModelReset with a single shot connection instead.
    // However, it's possible for user code to begin the reset before connectToAbstractItemModel is called
    // (QTBUG-125053), in which case we connect to modelReset too late and handleModelReset is never called,
    // resulting in delegates not being created in certain cases.
    // So, we check at the earliest point we can if the model is in the process of being reset,
    // and if so, connect modelReset to handleModelReset.
    if (d->m_adaptorModel.adaptsAim()) {
        auto *aim = d->m_adaptorModel.aim();
        auto *aimPrivate = QAbstractItemModelPrivate::get(aim);
        if (aimPrivate->resetting)
            QObject::connect(aim, &QAbstractItemModel::modelReset, this, &QQmlDelegateModel::handleModelReset, Qt::SingleShotConnection);
    }
}

/*!
    \qmlproperty Component QtQml.Models::DelegateModel::delegate

    The delegate provides a template defining each item instantiated by a view.
    The index is exposed as an accessible \c index property.  Properties of the
    model are also available depending upon the type of \l {qml-data-models}{Data Model}.
*/
QQmlComponent *QQmlDelegateModel::delegate() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_delegate;
}

void QQmlDelegateModel::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQmlDelegateModel);
    if (d->m_transaction) {
        qmlWarning(this) << tr("The delegate of a DelegateModel cannot be changed within onUpdated.");
        return;
    }
    if (d->m_delegate == delegate)
        return;
    if (d->m_complete)
        _q_itemsRemoved(0, d->m_count);
    d->m_delegate.setObject(delegate, this);
    d->m_delegateValidated = false;
    if (d->m_delegateChooser)
        QObject::disconnect(d->m_delegateChooserChanged);

    d->m_delegateChooser = nullptr;
    if (delegate) {
        QQmlAbstractDelegateComponent *adc =
                qobject_cast<QQmlAbstractDelegateComponent *>(delegate);
        if (adc) {
            d->m_delegateChooser = adc;
            d->m_delegateChooserChanged = connect(adc, &QQmlAbstractDelegateComponent::delegateChanged, this,
                                               [d](){ d->delegateChanged(); });
        }
    }
    if (d->m_complete) {
        _q_itemsInserted(0, d->adaptorModelCount());
        d->requestMoreIfNecessary();
    }
    emit delegateChanged();
}

/*!
    \qmlproperty QModelIndex QtQml.Models::DelegateModel::rootIndex

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  \c rootIndex allows the children of
    any node in a QAbstractItemModel to be provided by this model.

    This property only affects models of type QAbstractItemModel that
    are hierarchical (e.g, a tree model).

    For example, here is a simple interactive file system browser.
    When a directory name is clicked, the view's \c rootIndex is set to the
    QModelIndex node of the clicked directory, thus updating the view to show
    the new directory's contents.

    \c main.cpp:
    \snippet delegatemodel/delegatemodel_rootindex/main.cpp 0

    \c view.qml:
    \snippet delegatemodel/delegatemodel_rootindex/view.qml 0

    If the \l {dm-model-property}{model} is a QAbstractItemModel subclass,
    the delegate can also reference a \c hasModelChildren property (optionally
    qualified by a \e model. prefix) that indicates whether the delegate's
    model item has any child nodes.

    \sa modelIndex(), parentModelIndex()
*/
QVariant QQmlDelegateModel::rootIndex() const
{
    Q_D(const QQmlDelegateModel);
    return QVariant::fromValue(QModelIndex(d->m_adaptorModel.rootIndex));
}

void QQmlDelegateModel::setRootIndex(const QVariant &root)
{
    Q_D(QQmlDelegateModel);

    QModelIndex modelIndex = qvariant_cast<QModelIndex>(root);
    const bool changed = d->m_adaptorModel.rootIndex != modelIndex;
    if (changed || !d->m_adaptorModel.isValid()) {
        const int oldCount = d->m_count;
        d->m_adaptorModel.rootIndex = modelIndex;
        if (!d->m_adaptorModel.isValid() && d->m_adaptorModel.aim()) {
            // The previous root index was invalidated, so we need to reconnect the model.
            d->disconnectFromAbstractItemModel();
            d->m_adaptorModel.setModel(d->m_adaptorModel.list.list());
            d->connectToAbstractItemModel();
        }
        if (d->m_adaptorModel.canFetchMore())
            d->m_adaptorModel.fetchMore();
        if (d->m_complete) {
            const int newCount = d->adaptorModelCount();
            if (oldCount)
                _q_itemsRemoved(0, oldCount);
            if (newCount)
                _q_itemsInserted(0, newCount);
        }
        if (changed)
            emit rootIndexChanged();
    }
}

/*!
    \qmlproperty enumeration QtQml.Models::DelegateModel::delegateModelAccess

    \include delegatemodelaccess.qdocinc
*/
QQmlDelegateModel::DelegateModelAccess QQmlDelegateModel::delegateModelAccess() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_adaptorModel.delegateModelAccess;
}

void QQmlDelegateModel::setDelegateModelAccess(
        QQmlDelegateModel::DelegateModelAccess delegateModelAccess)
{
    Q_D(QQmlDelegateModel);
    if (d->m_adaptorModel.delegateModelAccess == delegateModelAccess)
        return;

    if (d->m_transaction) {
        qmlWarning(this) << tr("The delegateModelAccess of a DelegateModel "
                               "cannot be changed within onUpdated.");
        return;
    }

    if (d->m_complete) {
        _q_itemsRemoved(0, d->m_count);
        d->m_adaptorModel.delegateModelAccess = delegateModelAccess;
        _q_itemsInserted(0, d->adaptorModelCount());
        d->requestMoreIfNecessary();
    } else {
        d->m_adaptorModel.delegateModelAccess = delegateModelAccess;
    }

    emit delegateModelAccessChanged();
}

/*!
    \qmlmethod QModelIndex QtQml.Models::DelegateModel::modelIndex(int index)

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data. This function assists in using
    tree models in QML.

    Returns a QModelIndex for the specified \a index.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQmlDelegateModel::modelIndex(int idx) const
{
    Q_D(const QQmlDelegateModel);
    return d->m_adaptorModel.modelIndex(idx);
}

/*!
    \qmlmethod QModelIndex QtQml.Models::DelegateModel::parentModelIndex()

    QAbstractItemModel provides a hierarchical tree of data, whereas
    QML only operates on list data.  This function assists in using
    tree models in QML.

    Returns a QModelIndex for the parent of the current rootIndex.
    This value can be assigned to rootIndex.

    \sa rootIndex
*/
QVariant QQmlDelegateModel::parentModelIndex() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_adaptorModel.parentModelIndex();
}

/*!
    \qmlproperty int QtQml.Models::DelegateModel::count
*/

int QQmlDelegateModel::count() const
{
    Q_D(const QQmlDelegateModel);
    if (!d->m_delegate)
        return 0;
    return d->m_compositor.count(d->m_compositorGroup);
}

QQmlDelegateModel::ReleaseFlags QQmlDelegateModelPrivate::release(QObject *object, QQmlInstanceModel::ReusableFlag reusableFlag)
{
    if (!object)
        return QQmlDelegateModel::ReleaseFlags{};

    QQmlDelegateModelItem *cacheItem = QQmlDelegateModelItem::dataForObject(object);
    if (!cacheItem)
        return QQmlDelegateModel::ReleaseFlags{};

    if (!cacheItem->releaseObject())
        return QQmlDelegateModel::Referenced;

    if (reusableFlag == QQmlInstanceModel::Reusable && m_reusableItemsPool.insertItem(cacheItem)) {
        removeCacheItem(cacheItem);
        emit q_func()->itemPooled(cacheItem->modelIndex(), cacheItem->object);
        return QQmlInstanceModel::Pooled;
    }

    destroyCacheItem(cacheItem);
    return QQmlInstanceModel::Destroyed;
}

void QQmlDelegateModelPrivate::destroyCacheItem(QQmlDelegateModelItem *cacheItem)
{
    emitDestroyingItem(cacheItem->object);
    cacheItem->destroyObject();
    if (cacheItem->incubationTask) {
        releaseIncubator(cacheItem->incubationTask);
        cacheItem->incubationTask = nullptr;
    }
    cacheItem->dispose();
}

/*
  Returns ReleaseStatus flags.
*/
QQmlDelegateModel::ReleaseFlags QQmlDelegateModel::release(QObject *item, QQmlInstanceModel::ReusableFlag reusableFlag)
{
    Q_D(QQmlDelegateModel);
    QQmlInstanceModel::ReleaseFlags stat = d->release(item, reusableFlag);
    return stat;
}

// Cancel a requested async item
void QQmlDelegateModel::cancel(int index)
{
    Q_D(QQmlDelegateModel);
    if (index < 0 || index >= d->m_compositor.count(d->m_compositorGroup)) {
        qWarning() << "DelegateModel::cancel: index out range" << index << d->m_compositor.count(d->m_compositorGroup);
        return;
    }

    Compositor::iterator it = d->m_compositor.find(d->m_compositorGroup, index);
    QQmlDelegateModelItem *cacheItem = it->inCache() ? d->m_cache.at(it.cacheIndex()) : 0;
    if (cacheItem) {
        if (cacheItem->incubationTask && !cacheItem->isObjectReferenced()) {
            d->releaseIncubator(cacheItem->incubationTask);
            cacheItem->incubationTask = nullptr;

            if (cacheItem->object) {
                QObject *object = cacheItem->object;
                cacheItem->destroyObject();
                if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(object))
                    d->emitDestroyingPackage(package);
                else
                    d->emitDestroyingItem(object);
            }

            cacheItem->scriptRef -= 1;
        }
        if (!cacheItem->isReferenced()) {
            d->m_compositor.clearFlags(
                        Compositor::Cache, it.cacheIndex(), 1, Compositor::CacheFlag);
            d->m_cache.removeAt(it.cacheIndex());
            delete cacheItem;
            Q_ASSERT(d->m_cache.size() == d->m_compositor.count(Compositor::Cache));
        }
    }
}

void QQmlDelegateModelPrivate::group_append(
        QQmlListProperty<QQmlDelegateModelGroup> *property, QQmlDelegateModelGroup *group)
{
    QQmlDelegateModelPrivate *d = static_cast<QQmlDelegateModelPrivate *>(property->data);
    if (d->m_complete)
        return;
    if (d->m_groupCount == Compositor::MaximumGroupCount) {
        qmlWarning(d->q_func()) << QQmlDelegateModel::tr("The maximum number of supported DelegateModelGroups is 8");
        return;
    }
    d->m_groups[d->m_groupCount] = group;
    d->m_groupCount += 1;
}

qsizetype QQmlDelegateModelPrivate::group_count(
        QQmlListProperty<QQmlDelegateModelGroup> *property)
{
    QQmlDelegateModelPrivate *d = static_cast<QQmlDelegateModelPrivate *>(property->data);
    return d->m_groupCount - 1;
}

QQmlDelegateModelGroup *QQmlDelegateModelPrivate::group_at(
        QQmlListProperty<QQmlDelegateModelGroup> *property, qsizetype index)
{
    QQmlDelegateModelPrivate *d = static_cast<QQmlDelegateModelPrivate *>(property->data);
    return index >= 0 && index < d->m_groupCount - 1
            ? d->m_groups[index + 1]
            : nullptr;
}

/*!
    \qmlproperty list<DelegateModelGroup> QtQml.Models::DelegateModel::groups

    This property holds a delegate model's group definitions.

    Groups define a sub-set of the items in a delegate model and can be used to filter
    a model.

    For every group defined in a DelegateModel two attached pseudo-properties are added to each
    delegate item.  The first of the form DelegateModel.in\e{GroupName} holds whether the
    item belongs to the group and the second DelegateModel.\e{groupName}Index holds the
    index of the item in that group.

    The following example illustrates using groups to select items in a model.

    \snippet delegatemodel/delegatemodelgroup.qml 0
    \keyword dm-groups-property


    \warning In contrast to normal attached properties, those cannot be set in a declarative way.
    The following would result in an error:
    \badcode
    delegate: Rectangle {
        DelegateModel.inSelected: true
    }
    \endcode
*/

QQmlListProperty<QQmlDelegateModelGroup> QQmlDelegateModel::groups()
{
    Q_D(QQmlDelegateModel);
    return QQmlListProperty<QQmlDelegateModelGroup>(
            this,
            d,
            QQmlDelegateModelPrivate::group_append,
            QQmlDelegateModelPrivate::group_count,
            QQmlDelegateModelPrivate::group_at,
            nullptr, nullptr, nullptr);
}

/*!
    \qmlproperty DelegateModelGroup QtQml.Models::DelegateModel::items

    This property holds default group to which all new items are added.
*/

QQmlDelegateModelGroup *QQmlDelegateModel::items()
{
    Q_D(QQmlDelegateModel);
    return d->m_items;
}

/*!
    \qmlproperty DelegateModelGroup QtQml.Models::DelegateModel::persistedItems

    This property holds delegate model's persisted items group.

    Items in this group are not destroyed when released by a view, instead they are persisted
    until removed from the group.

    An item can be removed from the persistedItems group by setting the
    DelegateModel.inPersistedItems property to false.  If the item is not referenced by a view
    at that time it will be destroyed.  Adding an item to this group will not create a new
    instance.

    Items returned by the \l QtQml.Models::DelegateModelGroup::create() function are automatically added
    to this group.
*/

QQmlDelegateModelGroup *QQmlDelegateModel::persistedItems()
{
    Q_D(QQmlDelegateModel);
    return d->m_persistedItems;
}

/*!
    \qmlproperty string QtQml.Models::DelegateModel::filterOnGroup

    This property holds name of the group that is used to filter the delegate model.

    Only items that belong to this group are visible to a view.

    By default this is the \l items group.
*/

QString QQmlDelegateModel::filterGroup() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_filterGroup;
}

void QQmlDelegateModel::setFilterGroup(const QString &group)
{
    Q_D(QQmlDelegateModel);

    if (d->m_transaction) {
        qmlWarning(this) << tr("The group of a DelegateModel cannot be changed within onChanged");
        return;
    }

    if (d->m_filterGroup != group) {
        d->m_filterGroup = group;
        d->updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQmlDelegateModel::resetFilterGroup()
{
    setFilterGroup(QStringLiteral("items"));
}

void QQmlDelegateModelPrivate::updateFilterGroup()
{
    Q_Q(QQmlDelegateModel);
    if (!m_cacheMetaType)
        return;

    QQmlListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    for (int i = 1; i < m_groupCount; ++i) {
        if (m_filterGroup == m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQmlDelegateModelGroupPrivate::get(m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QQmlChangeSet::Change> removes;
        QVector<QQmlChangeSet::Change> inserts;
        m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QQmlChangeSet changeSet;
        changeSet.move(removes, inserts);
        emit q->modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit q->countChanged();

        if (m_parts) {
            auto partsCopy = m_parts->models; // deliberate; this may alter m_parts
            for (QQmlPartsModel *model : std::as_const(partsCopy))
                model->updateFilterGroup(m_compositorGroup, changeSet);
        }
    }
}

/*!
    \qmlproperty object QtQml.Models::DelegateModel::parts

    The \a parts property selects a DelegateModel which creates
    delegates from the part named.  This is used in conjunction with
    the \l Package type.

    For example, the code below selects a model which creates
    delegates named \e list from a \l Package:

    \code
    DelegateModel {
        id: visualModel
        delegate: Package {
            Item { Package.name: "list" }
        }
        model: myModel
    }

    ListView {
        width: 200; height:200
        model: visualModel.parts.list
    }
    \endcode

    \sa Package
*/

QObject *QQmlDelegateModel::parts()
{
    Q_D(QQmlDelegateModel);
    if (!d->m_parts)
        d->m_parts = new QQmlDelegateModelParts(this);
    return d->m_parts;
}

const QAbstractItemModel *QQmlDelegateModel::abstractItemModel() const
{
    Q_D(const QQmlDelegateModel);
    return d->m_adaptorModel.adaptsAim() ? d->m_adaptorModel.aim() : nullptr;
}

void QQmlDelegateModelPrivate::emitCreatedPackage(QQDMIncubationTask *incubationTask, QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->createdPackage(incubationTask->index[i], package);
}

void QQmlDelegateModelPrivate::emitInitPackage(QQDMIncubationTask *incubationTask, QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->initPackage(incubationTask->index[i], package);
}

void QQmlDelegateModelPrivate::emitDestroyingPackage(QQuickPackage *package)
{
    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->destroyingPackage(package);
}

static bool isDoneIncubating(QQmlIncubator::Status status)
{
     return status == QQmlIncubator::Ready || status == QQmlIncubator::Error;
}

void QQDMIncubationTask::initializeRequiredProperties(
        QQmlDelegateModelItem *modelItemToIncubate, QObject *object,
        QQmlDelegateModel::DelegateModelAccess access)
{
    // QQmlObjectCreator produces a private internal context.
    // We can always attach the extra object there.
    QQmlData *d = QQmlData::get(object);
    if (auto contextData = d ? d->context : nullptr)
        contextData->setExtraObject(modelItemToIncubate);

    Q_ASSERT(modelItemToIncubate->delegate);
    const bool isBound = QQmlComponentPrivate::get(modelItemToIncubate->delegate)->isBound();

    auto incubatorPriv = QQmlIncubatorPrivate::get(this);
    if (incubatorPriv->hadTopLevelRequiredProperties()) {
        // If we have required properties, we clear the context object
        // so that the model role names are not polluting the context.
        // Unless the context is bound, in which case we have never set the context object.
        if (incubating && !isBound) {
            Q_ASSERT(incubating->contextData);
            incubating->contextData->setContextObject(nullptr);
        }
        if (proxyContext) {
            proxyContext->setContextObject(nullptr);
        }

        // Retrieve the metaObject before the potential return so that the accessors have a chance
        // to perform some finalization in case they produce a dynamic metaobject. Here we know for
        // sure that we are using required properties.
        const QMetaObject *qmlMetaObject = modelItemToIncubate->metaObject();

        if (incubatorPriv->requiredProperties()->empty())
            return;
        RequiredProperties *requiredProperties = incubatorPriv->requiredProperties();

        // if a required property was not in the model, it might still be a static property of the
        // QQmlDelegateModelItem or one of its derived classes this is the case for index, row,
        // column, model and more
        // the most derived subclasses of QQmlDelegateModelItem are QQmlDMAbstractItemModelData and
        // QQmlDMObjectData at depth 2, so 4 should be plenty
        QVarLengthArray<std::pair<const QMetaObject *, QObject *>, 4> mos;
        // we first check the dynamic meta object for properties originating from the model
        // contains abstractitemmodelproperties
        mos.push_back(std::make_pair(qmlMetaObject, modelItemToIncubate));
        auto delegateModelItemSubclassMO = qmlMetaObject->superClass();
        mos.push_back(std::make_pair(delegateModelItemSubclassMO, modelItemToIncubate));

        while (strcmp(delegateModelItemSubclassMO->className(),
                      modelItemToIncubate->staticMetaObject.className())) {
            delegateModelItemSubclassMO = delegateModelItemSubclassMO->superClass();
            mos.push_back(std::make_pair(delegateModelItemSubclassMO, modelItemToIncubate));
        }
        if (proxiedObject)
            mos.push_back(std::make_pair(proxiedObject->metaObject(), proxiedObject));

        QQmlEngine *engine = QQmlEnginePrivate::get(incubatorPriv->enginePriv);
        QV4::ExecutionEngine *v4 = engine->handle();
        QV4::Scope scope(v4);

        for (const auto &metaObjectAndObject : mos) {
            const QMetaObject *mo = metaObjectAndObject.first;
            QObject *itemOrProxy = metaObjectAndObject.second;
            QV4::Scoped<QV4::QmlContext> qmlContext(scope);

            for (int i = mo->propertyOffset(); i < mo->propertyCount() + mo->propertyOffset(); ++i) {
                auto prop = mo->property(i);
                if (!prop.name())
                    continue;
                const QString propName = QString::fromUtf8(prop.name());
                bool wasInRequired = false;
                QQmlProperty targetProp = QQmlComponentPrivate::removePropertyFromRequired(
                            object, propName, requiredProperties,
                            engine, &wasInRequired);
                if (wasInRequired) {
                    QQmlProperty sourceProp(itemOrProxy, propName);
                    QQmlAnyBinding forward = QQmlPropertyToPropertyBinding::create(
                            engine, sourceProp, targetProp);
                    if (access != QQmlDelegateModel::Qt5ReadWrite)
                        forward.setSticky();
                    forward.installOn(targetProp);
                    if (access == QQmlDelegateModel::ReadWrite && sourceProp.isWritable()) {
                        QQmlAnyBinding reverse = QQmlPropertyToPropertyBinding::create(
                                engine, targetProp, sourceProp);
                        reverse.setSticky();
                        reverse.installOn(sourceProp);
                    }
                }
            }
        }
    } else {
        // To retain compatibility, we cannot enable structured model data if the data is passed
        // via context properties.
        modelItemToIncubate->disableStructuredModelData();

        if (!isBound)
            modelItemToIncubate->contextData->setContextObject(modelItemToIncubate);
        if (proxiedObject)
            proxyContext->setContextObject(proxiedObject);

        // Retrieve the metaObject() once so that the accessors have a chance to perform some
        // finalization in case they produce a dynamic metaobject. For example, they might be
        // inclined to create a propertyCache now because there are no required properties and any
        // revisioned properties should be hidden after all. Here is the first time we know for
        // sure whether we are using context properties.
        modelItemToIncubate->metaObject();
    }
}

void QQDMIncubationTask::statusChanged(Status status)
{
    if (vdm) {
        vdm->incubatorStatusChanged(this, status);
    } else if (isDoneIncubating(status)) {
        Q_ASSERT(incubating);
        // The model was deleted from under our feet, cleanup ourselves
        delete incubating->object;
        incubating->object = nullptr;
        incubating->contextData.reset();
        incubating->scriptRef = 0;
        incubating->deleteLater();
    }
}

void QQmlDelegateModelPrivate::releaseIncubator(QQDMIncubationTask *incubationTask)
{
    Q_Q(QQmlDelegateModel);
    if (!incubationTask->isError())
        incubationTask->clear();
    m_finishedIncubating.append(incubationTask);
    if (!m_incubatorCleanupScheduled) {
        m_incubatorCleanupScheduled = true;
        QCoreApplication::postEvent(q, new QEvent(QEvent::User));
    }
}

void QQmlDelegateModelPrivate::reuseItem(QQmlDelegateModelItem *item, int newModelIndex, int newGroups)
{
    Q_ASSERT(item->object);

    // Update/reset which groups the item belongs to
    item->groups = newGroups;

    // Update context property index (including row and column) on the delegate
    // item, and inform the application about it. For a list, the row is the same
    // as the index, and the column is always 0. We set alwaysEmit to true, to
    // force all bindings to be reevaluated, even if the index didn't change.
    const bool alwaysEmit = true;
    item->setModelIndex(newModelIndex, newModelIndex, 0, alwaysEmit);

    // Notify the application that all 'dynamic'/role-based context data has
    // changed as well (their getter function will use the updated index).
    auto const itemAsList = QList<QQmlDelegateModelItem *>() << item;
    auto const updateAllRoles = QVector<int>();
    m_adaptorModel.notify(itemAsList, newModelIndex, 1, updateAllRoles);

    if (QQmlDelegateModelAttached *att = static_cast<QQmlDelegateModelAttached *>(
                qmlAttachedPropertiesObject<QQmlDelegateModel>(item->object, false))) {
        // Update currentIndex of the attached DelegateModel object
        // to the index the item has in the cache.
        att->resetCurrentIndex();
        // emitChanges will emit both group-, and index changes to the application
        att->emitChanges();
    }

    // Inform the view that the item is recycled. This will typically result
    // in the view updating its own attached delegate item properties.
    emit q_func()->itemReused(newModelIndex, item->object);
}

void QQmlDelegateModelPrivate::drainReusableItemsPool(int maxPoolTime)
{
    m_reusableItemsPool.drain(maxPoolTime, [this](QQmlDelegateModelItem *cacheItem){ destroyCacheItem(cacheItem); });
}

void QQmlDelegateModel::drainReusableItemsPool(int maxPoolTime)
{
    d_func()->drainReusableItemsPool(maxPoolTime);
}

int QQmlDelegateModel::poolSize()
{
    return d_func()->m_reusableItemsPool.size();
}

QQmlComponent *QQmlDelegateModelPrivate::resolveDelegate(int index)
{
    if (!m_delegateChooser)
        return m_delegate;

    QQmlComponent *delegate = nullptr;
    QQmlAbstractDelegateComponent *chooser = m_delegateChooser;

    do {
        delegate = chooser->delegate(&m_adaptorModel, index);
        chooser = qobject_cast<QQmlAbstractDelegateComponent *>(delegate);
    } while (chooser);

    return delegate;
}

void QQmlDelegateModelPrivate::addCacheItem(QQmlDelegateModelItem *item, Compositor::iterator it)
{
    m_cache.insert(it.cacheIndex(), item);
    m_compositor.setFlags(it, 1, Compositor::CacheFlag);
    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
}

void QQmlDelegateModelPrivate::removeCacheItem(QQmlDelegateModelItem *cacheItem)
{
    int cidx = m_cache.lastIndexOf(cacheItem);
    if (cidx >= 0) {
        m_compositor.clearFlags(Compositor::Cache, cidx, 1, Compositor::CacheFlag);
        m_cache.removeAt(cidx);
    }
    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
}

void QQmlDelegateModelPrivate::incubatorStatusChanged(QQDMIncubationTask *incubationTask, QQmlIncubator::Status status)
{
    if (!isDoneIncubating(status))
        return;

    const QList<QQmlError> incubationTaskErrors = incubationTask->errors();

    QQmlDelegateModelItem *cacheItem = incubationTask->incubating;
    cacheItem->incubationTask = nullptr;
    incubationTask->incubating = nullptr;
    releaseIncubator(incubationTask);

    if (status == QQmlIncubator::Ready) {
        cacheItem->referenceObject();
        if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(cacheItem->object))
            emitCreatedPackage(incubationTask, package);
        else
            emitCreatedItem(incubationTask, cacheItem->object);
        cacheItem->releaseObject();
    } else if (status == QQmlIncubator::Error) {
        qmlInfo(m_delegate, incubationTaskErrors + m_delegate->errors()) << "Cannot create delegate";
    }

    if (!cacheItem->isObjectReferenced()) {
        if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(cacheItem->object))
            emitDestroyingPackage(package);
        else
            emitDestroyingItem(cacheItem->object);
        delete cacheItem->object;
        cacheItem->object = nullptr;
        cacheItem->scriptRef -= 1;
        cacheItem->contextData.reset();

        if (!cacheItem->isReferenced()) {
            removeCacheItem(cacheItem);
            delete cacheItem;
        }
    }
}

void QQDMIncubationTask::setInitialState(QObject *o)
{
    vdm->setInitialState(this, o);
}

QQmlDelegateModelGroupEmitter::~QQmlDelegateModelGroupEmitter() = default;
void QQmlDelegateModelGroupEmitter::createdPackage(int, QQuickPackage *) {}
void QQmlDelegateModelGroupEmitter::initPackage(int, QQuickPackage *) {}
void QQmlDelegateModelGroupEmitter::destroyingPackage(QQuickPackage *) {}

void QQmlDelegateModelPrivate::setInitialState(QQDMIncubationTask *incubationTask, QObject *o)
{
    QQmlDelegateModelItem *cacheItem = incubationTask->incubating;
    incubationTask->initializeRequiredProperties(
            incubationTask->incubating, o, m_adaptorModel.delegateModelAccess);
    cacheItem->object = o;

    if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(cacheItem->object))
        emitInitPackage(incubationTask, package);
    else
        emitInitItem(incubationTask, cacheItem->object);
}

QObject *QQmlDelegateModelPrivate::object(Compositor::Group group, int index, QQmlIncubator::IncubationMode incubationMode)
{
    if (!m_delegate || index < 0 || index >= m_compositor.count(group)) {
        qWarning() << "DelegateModel::item: index out range" << index << m_compositor.count(group);
        return nullptr;
    } else if (!m_context || !m_context->isValid()) {
        return nullptr;
    }

    Compositor::iterator it = m_compositor.find(group, index);
    const auto flags = it->flags;
    const auto modelIndex = it.modelIndex();

    QQmlDelegateModelItem *cacheItem = it->inCache() ? m_cache.at(it.cacheIndex()) : 0;

    if (!cacheItem || !cacheItem->delegate) {
        QQmlComponent *delegate = resolveDelegate(modelIndex);
        if (!delegate)
            return nullptr;

        if (!cacheItem) {
            cacheItem = m_reusableItemsPool.takeItem(delegate, index);
            if (cacheItem) {
                // Move the pooled item back into the cache, update
                // all related properties, and return the object (which
                // has already been incubated, otherwise it wouldn't be in the pool).
                addCacheItem(cacheItem, it);
                reuseItem(cacheItem, index, flags);
                cacheItem->referenceObject();

                if (index == m_compositor.count(group) - 1)
                    requestMoreIfNecessary();
                return cacheItem->object;
            }

            // Since we could't find an available item in the pool, we create a new one
            cacheItem = m_adaptorModel.createItem(m_cacheMetaType, modelIndex);
            if (!cacheItem)
                return nullptr;

            cacheItem->groups = flags;
            addCacheItem(cacheItem, it);
        }

        cacheItem->delegate = delegate;
    }

    // Bump the reference counts temporarily so neither the content data or the delegate object
    // are deleted if incubatorStatusChanged() is called synchronously.
    cacheItem->scriptRef += 1;
    cacheItem->referenceObject();

    if (cacheItem->incubationTask) {
        bool sync = (incubationMode == QQmlIncubator::Synchronous || incubationMode == QQmlIncubator::AsynchronousIfNested);
        if (sync && cacheItem->incubationTask->incubationMode() == QQmlIncubator::Asynchronous) {
            // previously requested async - now needed immediately
            cacheItem->incubationTask->forceCompletion();
        }
    } else if (!cacheItem->object) {
        QQmlContext *creationContext = cacheItem->delegate->creationContext();

        cacheItem->scriptRef += 1;

        cacheItem->incubationTask = new QQDMIncubationTask(this, incubationMode);
        cacheItem->incubationTask->incubating = cacheItem;
        cacheItem->incubationTask->clear();

        for (int i = 1; i < m_groupCount; ++i)
            cacheItem->incubationTask->index[i] = it.index[i];

        const QQmlRefPointer<QQmlContextData> componentContext
                = QQmlContextData::get(creationContext  ? creationContext : m_context.data());
        QQmlComponentPrivate *cp = QQmlComponentPrivate::get(cacheItem->delegate);

        if (cp->isBound()) {
            cacheItem->contextData = componentContext;

            // Ignore return value of initProxy. We want to know the proxy when assigning required
            // properties, but we don't want it to pollute our context. The context is bound.
            if (m_adaptorModel.hasProxyObject())
                cacheItem->initProxy();

            cp->incubateObject(
                        cacheItem->incubationTask,
                        cacheItem->delegate,
                        m_context->engine(),
                        componentContext,
                        QQmlContextData::get(m_context));
        } else {
            QQmlRefPointer<QQmlContextData> ctxt
                    = QQmlContextData::createRefCounted(componentContext);
            ctxt->setContextObject(cacheItem);
            cacheItem->contextData = ctxt;

            // If the model is read-only we cannot just expose the object as context
            // We actually need a separate model object to moderate access.
            if (m_adaptorModel.hasProxyObject()) {
                if (m_adaptorModel.delegateModelAccess == QQmlDelegateModel::ReadOnly)
                    cacheItem->initProxy();
                else
                    ctxt = cacheItem->initProxy();
            }

            cp->incubateObject(
                        cacheItem->incubationTask,
                        cacheItem->delegate,
                        m_context->engine(),
                        ctxt,
                        QQmlContextData::get(m_context));
        }
    }

    if (index == m_compositor.count(group) - 1)
        requestMoreIfNecessary();

    // Remove the temporary reference count.
    cacheItem->scriptRef -= 1;
    if (cacheItem->object && (!cacheItem->incubationTask || isDoneIncubating(cacheItem->incubationTask->status())))
        return cacheItem->object;

    if (cacheItem->objectRef > 0)
        cacheItem->releaseObject();

    if (!cacheItem->isReferenced()) {
        removeCacheItem(cacheItem);
        delete cacheItem;
    }

    return nullptr;
}

/*
  If asynchronous is true or the component is being loaded asynchronously due
  to an ancestor being loaded asynchronously, object() may return 0.  In this
  case createdItem() will be emitted when the object is available.  The object
  at this stage does not have any references, so object() must be called again
  to ensure a reference is held.  Any call to object() which returns a valid object
  must be matched by a call to release() in order to destroy the object.
*/
QObject *QQmlDelegateModel::object(int index, QQmlIncubator::IncubationMode incubationMode)
{
    Q_D(QQmlDelegateModel);
    if (!d->m_delegate || index < 0 || index >= d->m_compositor.count(d->m_compositorGroup)) {
        qWarning() << "DelegateModel::item: index out range" << index << d->m_compositor.count(d->m_compositorGroup);
        return nullptr;
    }

    return d->object(d->m_compositorGroup, index, incubationMode);
}

QQmlIncubator::Status QQmlDelegateModel::incubationStatus(int index)
{
    Q_D(QQmlDelegateModel);
    if (d->m_compositor.count(d->m_compositorGroup) <= index)
        return QQmlIncubator::Null;
    Compositor::iterator it = d->m_compositor.find(d->m_compositorGroup, index);
    if (!it->inCache())
        return QQmlIncubator::Null;

    if (auto incubationTask = d->m_cache.at(it.cacheIndex())->incubationTask)
        return incubationTask->status();

    return QQmlIncubator::Ready;
}

QVariant QQmlDelegateModelPrivate::variantValue(QQmlListCompositor::Group group, int index, const QString &name)
{
    Compositor::iterator it = m_compositor.find(group, index);
    if (QQmlAdaptorModel *model = it.list<QQmlAdaptorModel>()) {
        QString role = name;
        int dot = name.indexOf(QLatin1Char('.'));
        if (dot > 0)
            role = name.left(dot);
        QVariant value = model->value(it.modelIndex(), role);
        while (dot > 0) {
            const int from = dot + 1;
            dot = name.indexOf(QLatin1Char('.'), from);
            QStringView propertyName = QStringView{name}.mid(from, dot - from);
            if (QObject *obj = qvariant_cast<QObject*>(value)) {
                value = obj->property(propertyName.toUtf8());
            } else if (const QMetaObject *metaObject = QQmlMetaType::metaObjectForValueType(value.metaType())) {
                // Q_GADGET
                const int propertyIndex = metaObject->indexOfProperty(propertyName.toUtf8());
                if (propertyIndex >= 0)
                    value = metaObject->property(propertyIndex).readOnGadget(value.constData());
            } else {
                return QVariant();
            }
        }
        return value;
    }
    return QVariant();
}

QVariant QQmlDelegateModel::variantValue(int index, const QString &role)
{
    Q_D(QQmlDelegateModel);
    return d->variantValue(d->m_compositorGroup, index, role);
}

int QQmlDelegateModel::indexOf(QObject *item, QObject *) const
{
    Q_D(const QQmlDelegateModel);
    if (QQmlDelegateModelItem *cacheItem = QQmlDelegateModelItem::dataForObject(item))
        return cacheItem->groupIndex(d->m_compositorGroup);
    return -1;
}

void QQmlDelegateModel::setWatchedRoles(const QList<QByteArray> &roles)
{
    Q_D(QQmlDelegateModel);
    d->m_adaptorModel.replaceWatchedRoles(d->m_watchedRoles, roles);
    d->m_watchedRoles = roles;
}

void QQmlDelegateModelPrivate::addGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Insert> inserts;
    m_compositor.setFlags(from, count, group, groupFlags, &inserts);
    itemsInserted(inserts);
    emitChanges();
}

void QQmlDelegateModelPrivate::removeGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    m_compositor.clearFlags(from, count, group, groupFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

void QQmlDelegateModelPrivate::setGroups(
        Compositor::iterator from, int count, Compositor::Group group, int groupFlags)
{
    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;

    m_compositor.setFlags(from, count, group, groupFlags, &inserts);
    itemsInserted(inserts);
    const int removeFlags = ~groupFlags & Compositor::GroupMask;

    from = m_compositor.find(from.group, from.index[from.group]);
    m_compositor.clearFlags(from, count, group, removeFlags, &removes);
    itemsRemoved(removes);
    emitChanges();
}

bool QQmlDelegateModel::event(QEvent *e)
{
    Q_D(QQmlDelegateModel);
    if (e->type() == QEvent::UpdateRequest) {
        d->m_waitingToFetchMore = false;
        d->m_adaptorModel.fetchMore();
    } else if (e->type() == QEvent::User) {
        d->m_incubatorCleanupScheduled = false;
        qDeleteAll(d->m_finishedIncubating);
        d->m_finishedIncubating.clear();
    }
    return QQmlInstanceModel::event(e);
}

void QQmlDelegateModelPrivate::itemsChanged(const QVector<Compositor::Change> &changes)
{
    if (!m_delegate)
        return;

    QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> translatedChanges(m_groupCount);

    for (const Compositor::Change &change : changes) {
        for (int i = 1; i < m_groupCount; ++i) {
            if (change.inGroup(i)) {
                translatedChanges[i].append(QQmlChangeSet::Change(change.index[i], change.count));
            }
        }
    }

    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.change(translatedChanges.at(i));
}

void QQmlDelegateModel::_q_itemsChanged(int index, int count, const QVector<int> &roles)
{
    Q_D(QQmlDelegateModel);
    if (count <= 0 || !d->m_complete)
        return;

    if (d->m_adaptorModel.notify(d->m_cache, index, count, roles)) {
        QVector<Compositor::Change> changes;
        d->m_compositor.listItemsChanged(&d->m_adaptorModel, index, count, &changes);
        d->itemsChanged(changes);
        d->emitChanges();
    }
    const bool needToCheckDelegateChoiceInvalidation = d->m_delegateChooser && !roles.isEmpty();
    if (!needToCheckDelegateChoiceInvalidation)
        return;

    // here, we only really can handle AIM based models, because only there
    // we can do something sensible with roles
    if (!d->m_adaptorModel.adaptsAim())
        return;

    const auto aim = d->m_adaptorModel.aim();
    const auto choiceRole = d->m_delegateChooser->role().toUtf8();
    const auto &roleNames = aim->roleNames();
    auto it = std::find_if(roles.begin(), roles.end(), [&](int role) {
        return roleNames[role] == choiceRole;
    });
    if (it == roles.end())
        return;

    // Compare handleModelReset - we're doing a more localized version

    /* A role change affecting the DelegateChoice is equivalent to removing all
       affected items (including  invalidating their cache entries) and afterwards
       reinserting them.
    */
    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsRemoved(&d->m_adaptorModel, index, count, &removes);
    const QList<QQmlDelegateModelItem *> cache = d->m_cache;
    for (QQmlDelegateModelItem *item : cache)
        item->referenceObject();
    for (const auto& removed: removes) {
        if (!d->m_cache.isSharedWith(cache))
            break;
        QQmlDelegateModelItem *item = cache.value(removed.cacheIndex(), nullptr);
        if (!d->m_cache.contains(item))
            continue;
        if (item->modelIndex() != -1)
            item->setModelIndex(-1, -1, -1);
    }
    for (QQmlDelegateModelItem *item : cache)
        item->releaseObject();
    d->m_compositor.listItemsInserted(&d->m_adaptorModel, index, count, &inserts);
    d->itemsMoved(removes, inserts);
    d->emitChanges();
}

static void incrementIndexes(QQmlDelegateModelItem *cacheItem, int count, const int *deltas)
{
    if (QQDMIncubationTask *incubationTask = cacheItem->incubationTask) {
        for (int i = 1; i < count; ++i)
            incubationTask->index[i] += deltas[i];
    }
    if (QQmlDelegateModelAttached *attached = cacheItem->attached()) {
        for (int i = 1; i < qMin<int>(count, Compositor::MaximumGroupCount); ++i)
            attached->m_currentIndex[i] += deltas[i];
    }
}

void QQmlDelegateModelPrivate::itemsInserted(
        const QVector<Compositor::Insert> &inserts,
        QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> *translatedInserts,
        QHash<int, QList<QQmlDelegateModelItem *> > *movedItems)
{
    int cacheIndex = 0;

    int inserted[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        inserted[i] = 0;

    for (const Compositor::Insert &insert : inserts) {
        for (; cacheIndex < insert.cacheIndex(); ++cacheIndex)
            incrementIndexes(m_cache.at(cacheIndex), m_groupCount, inserted);

        for (int i = 1; i < m_groupCount; ++i) {
            if (insert.inGroup(i)) {
                (*translatedInserts)[i].append(
                        QQmlChangeSet::Change(insert.index[i], insert.count, insert.moveId));
                inserted[i] += insert.count;
            }
        }

        if (!insert.inCache())
            continue;

        if (movedItems && insert.isMove()) {
            QList<QQmlDelegateModelItem *> items = movedItems->take(insert.moveId);
            Q_ASSERT(items.size() == insert.count);
            m_cache = m_cache.mid(0, insert.cacheIndex())
                    + items + m_cache.mid(insert.cacheIndex());
        }
        if (insert.inGroup()) {
            for (int offset = 0; cacheIndex < insert.cacheIndex() + insert.count;
                 ++cacheIndex, ++offset) {
                QQmlDelegateModelItem *cacheItem = m_cache.at(cacheIndex);
                cacheItem->groups |= insert.flags & Compositor::GroupMask;

                if (QQDMIncubationTask *incubationTask = cacheItem->incubationTask) {
                    for (int i = 1; i < m_groupCount; ++i)
                        incubationTask->index[i] = cacheItem->groups & (1 << i)
                                ? insert.index[i] + offset
                                : insert.index[i];
                }
                if (QQmlDelegateModelAttached *attached = cacheItem->attached()) {
                    for (int i = 1; i < m_groupCount; ++i)
                        attached->m_currentIndex[i] = cacheItem->groups & (1 << i)
                                ? insert.index[i] + offset
                                : insert.index[i];
                }
            }
        } else {
            cacheIndex = insert.cacheIndex() + insert.count;
        }
    }
    for (const QList<QQmlDelegateModelItem *> cache = m_cache; cacheIndex < cache.size(); ++cacheIndex)
        incrementIndexes(cache.at(cacheIndex), m_groupCount, inserted);
}

void QQmlDelegateModelPrivate::itemsInserted(const QVector<Compositor::Insert> &inserts)
{
    QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts);
    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.insert(translatedInserts.at(i));
}

void QQmlDelegateModel::_q_itemsInserted(int index, int count)
{

    Q_D(QQmlDelegateModel);
    if (count <= 0 || !d->m_complete)
        return;

    d->m_count += count;

    const QList<QQmlDelegateModelItem *> cache = d->m_cache;
    for (int i = 0, c = cache.size();  i < c; ++i) {
        QQmlDelegateModelItem *item = cache.at(i);
        // layout change triggered by changing the modelIndex might have
        // already invalidated this item in d->m_cache and deleted it.
        if (!d->m_cache.isSharedWith(cache) && !d->m_cache.contains(item))
            continue;

        if (item->modelIndex() >= index) {
            const int newIndex = item->modelIndex() + count;
            const int row = newIndex;
            const int column = 0;
            item->setModelIndex(newIndex, row, column);
        }
    }

    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsInserted(&d->m_adaptorModel, index, count, &inserts);
    d->itemsInserted(inserts);
    d->emitChanges();
}

//### This method should be split in two. It will remove delegates, and it will re-render the list.
// When e.g. QQmlListModel::remove is called, the removal of the delegates should be done on
// QAbstractItemModel::rowsAboutToBeRemoved, and the re-rendering on
// QAbstractItemModel::rowsRemoved. Currently both are done on the latter signal. The problem is
// that the destruction of an item will emit a changed signal that ends up at the delegate, which
// in turn will try to load the data from the model (which should have already freed it), resulting
// in a use-after-free. See QTBUG-59256.
void QQmlDelegateModelPrivate::itemsRemoved(
        const QVector<Compositor::Remove> &removes,
        QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> *translatedRemoves,
        QHash<int, QList<QQmlDelegateModelItem *> > *movedItems)
{
    int cacheIndex = 0;
    int removedCache = 0;

    int removed[Compositor::MaximumGroupCount];
    for (int i = 1; i < m_groupCount; ++i)
        removed[i] = 0;

    for (const Compositor::Remove &remove : removes) {
        for (; cacheIndex < remove.cacheIndex() && cacheIndex < m_cache.size(); ++cacheIndex)
            incrementIndexes(m_cache.at(cacheIndex), m_groupCount, removed);

        for (int i = 1; i < m_groupCount; ++i) {
            if (remove.inGroup(i)) {
                (*translatedRemoves)[i].append(
                        QQmlChangeSet::Change(remove.index[i], remove.count, remove.moveId));
                removed[i] -= remove.count;
            }
        }

        if (!remove.inCache())
            continue;

        if (movedItems && remove.isMove()) {
            movedItems->insert(remove.moveId, m_cache.mid(remove.cacheIndex(), remove.count));
            QList<QQmlDelegateModelItem *>::const_iterator begin = m_cache.constBegin() + remove.cacheIndex();
            QList<QQmlDelegateModelItem *>::const_iterator end = begin + remove.count;
            m_cache.erase(begin, end);
        } else {
            for (; cacheIndex < remove.cacheIndex() + remove.count - removedCache; ++cacheIndex) {
                QQmlDelegateModelItem *cacheItem = m_cache.at(cacheIndex);
                if (remove.inGroup(Compositor::Persisted) && cacheItem->objectRef == 0 && cacheItem->object) {
                    QObject *object = cacheItem->object;
                    cacheItem->destroyObject();
                    if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(object))
                        emitDestroyingPackage(package);
                    else
                        emitDestroyingItem(object);
                    cacheItem->scriptRef -= 1;
                }
                if (!cacheItem->isReferenced() && !remove.inGroup(Compositor::Persisted)) {
                    m_compositor.clearFlags(Compositor::Cache, cacheIndex, 1, Compositor::CacheFlag);
                    m_cache.removeAt(cacheIndex);
                    delete cacheItem;
                    --cacheIndex;
                    ++removedCache;
                    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
                } else if (remove.groups() == cacheItem->groups) {
                    cacheItem->groups = 0;
                    if (QQDMIncubationTask *incubationTask = cacheItem->incubationTask) {
                        for (int i = 1; i < m_groupCount; ++i)
                            incubationTask->index[i] = -1;
                    }
                    if (QQmlDelegateModelAttached *attached = cacheItem->attached()) {
                        for (int i = 1; i < m_groupCount; ++i)
                            attached->m_currentIndex[i] = -1;
                    }
                } else {
                    if (QQDMIncubationTask *incubationTask = cacheItem->incubationTask) {
                        if (!cacheItem->isObjectReferenced()) {
                            releaseIncubator(cacheItem->incubationTask);
                            cacheItem->incubationTask = nullptr;
                            if (cacheItem->object) {
                                QObject *object = cacheItem->object;
                                cacheItem->destroyObject();
                                if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(object))
                                    emitDestroyingPackage(package);
                                else
                                    emitDestroyingItem(object);
                            }
                            cacheItem->scriptRef -= 1;
                        } else {
                            for (int i = 1; i < m_groupCount; ++i) {
                                if (remove.inGroup(i))
                                    incubationTask->index[i] = remove.index[i];
                            }
                        }
                    }
                    if (QQmlDelegateModelAttached *attached = cacheItem->attached()) {
                        for (int i = 1; i < m_groupCount; ++i) {
                            if (remove.inGroup(i))
                                attached->m_currentIndex[i] = remove.index[i];
                        }
                    }
                    cacheItem->groups &= ~remove.flags;
                }
            }
        }
    }

    for (const QList<QQmlDelegateModelItem *> cache = m_cache; cacheIndex < cache.size(); ++cacheIndex)
        incrementIndexes(cache.at(cacheIndex), m_groupCount, removed);
}

void QQmlDelegateModelPrivate::itemsRemoved(const QVector<Compositor::Remove> &removes)
{
    QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves);
    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i)
       QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.remove(translatedRemoves.at(i));
}

void QQmlDelegateModel::_q_itemsRemoved(int index, int count)
{
    Q_D(QQmlDelegateModel);
    if (count <= 0|| !d->m_complete)
        return;

    d->m_count -= count;
    Q_ASSERT(d->m_count >= 0);
    const QList<QQmlDelegateModelItem *> cache = d->m_cache;
    //Prevents items being deleted in remove loop
    for (QQmlDelegateModelItem *item : cache)
        item->referenceObject();

    for (int i = 0, c = cache.size();  i < c; ++i) {
        QQmlDelegateModelItem *item = cache.at(i);
        // layout change triggered by removal of a previous item might have
        // already invalidated this item in d->m_cache and deleted it
        if (!d->m_cache.isSharedWith(cache) && !d->m_cache.contains(item))
            continue;

        if (item->modelIndex() >= index + count) {
            const int newIndex = item->modelIndex() - count;
            const int row = newIndex;
            const int column = 0;
            item->setModelIndex(newIndex, row, column);
        } else if (item->modelIndex() >= index) {
            item->setModelIndex(-1, -1, -1);
        }
    }
    //Release items which are referenced before the loop
    for (QQmlDelegateModelItem *item : cache)
        item->releaseObject();

    QVector<Compositor::Remove> removes;
    d->m_compositor.listItemsRemoved(&d->m_adaptorModel, index, count, &removes);
    d->itemsRemoved(removes);

    d->emitChanges();
}

void QQmlDelegateModelPrivate::itemsMoved(
        const QVector<Compositor::Remove> &removes, const QVector<Compositor::Insert> &inserts)
{
    QHash<int, QList<QQmlDelegateModelItem *> > movedItems;

    QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> translatedRemoves(m_groupCount);
    itemsRemoved(removes, &translatedRemoves, &movedItems);

    QVarLengthArray<QVector<QQmlChangeSet::Change>, Compositor::MaximumGroupCount> translatedInserts(m_groupCount);
    itemsInserted(inserts, &translatedInserts, &movedItems);
    Q_ASSERT(m_cache.size() == m_compositor.count(Compositor::Cache));
    Q_ASSERT(movedItems.isEmpty());
    if (!m_delegate)
        return;

    for (int i = 1; i < m_groupCount; ++i) {
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.move(
                    translatedRemoves.at(i),
                    translatedInserts.at(i));
    }
}

void QQmlDelegateModel::_q_itemsMoved(int from, int to, int count)
{
    Q_D(QQmlDelegateModel);
    if (count <= 0 || !d->m_complete)
        return;

    const int minimum = qMin(from, to);
    const int maximum = qMax(from, to) + count;
    const int difference = from > to ? count : -count;

    const QList<QQmlDelegateModelItem *> cache = d->m_cache;
    for (int i = 0, c = cache.size();  i < c; ++i) {
        QQmlDelegateModelItem *item = cache.at(i);
        // layout change triggered by changing the modelIndex might have
        // already invalidated this item in d->m_cache and deleted it.
        if (!d->m_cache.isSharedWith(cache) && !d->m_cache.contains(item))
            continue;

        if (item->modelIndex() >= from && item->modelIndex() < from + count) {
            const int newIndex = item->modelIndex() - from + to;
            const int row = newIndex;
            const int column = 0;
            item->setModelIndex(newIndex, row, column);
        } else if (item->modelIndex() >= minimum && item->modelIndex() < maximum) {
            const int newIndex = item->modelIndex() + difference;
            const int row = newIndex;
            const int column = 0;
            item->setModelIndex(newIndex, row, column);
        }
    }

    QVector<Compositor::Remove> removes;
    QVector<Compositor::Insert> inserts;
    d->m_compositor.listItemsMoved(&d->m_adaptorModel, from, to, count, &removes, &inserts);
    d->itemsMoved(removes, inserts);
    d->emitChanges();
}

void QQmlDelegateModelPrivate::emitModelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_Q(QQmlDelegateModel);
    emit q->modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQmlDelegateModelPrivate::delegateChanged(bool add, bool remove)
{
    Q_Q(QQmlDelegateModel);
    if (!m_complete)
        return;

    if (m_transaction) {
        qmlWarning(q) << QQmlDelegateModel::tr("The delegates of a DelegateModel cannot be changed within onUpdated.");
        return;
    }

    if (remove) {
        for (int i = 1; i < m_groupCount; ++i) {
            QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.remove(
                    0, m_compositor.count(Compositor::Group(i)));
        }
    }
    if (add) {
        for (int i = 1; i < m_groupCount; ++i) {
            QQmlDelegateModelGroupPrivate::get(m_groups[i])->changeSet.insert(
                    0, m_compositor.count(Compositor::Group(i)));
        }
    }
    emitChanges();
}

void QQmlDelegateModelPrivate::emitChanges()
{
    if (m_transaction || !m_complete || !m_context || !m_context->isValid())
        return;

    m_transaction = true;
    QV4::ExecutionEngine *engine = m_context->engine()->handle();
    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->emitChanges(engine);
    m_transaction = false;

    const bool reset = m_reset;
    m_reset = false;
    for (int i = 1; i < m_groupCount; ++i)
        QQmlDelegateModelGroupPrivate::get(m_groups[i])->emitModelUpdated(reset);

    // emitChanges may alter m_cache and delete items
    QVarLengthArray<QPointer<QQmlDelegateModelAttached>> attachedObjects;
    attachedObjects.reserve(m_cache.length());
    for (const QQmlDelegateModelItem *cacheItem : std::as_const(m_cache))
        attachedObjects.append(cacheItem->attached());

    for (const QPointer<QQmlDelegateModelAttached> &attached : std::as_const(attachedObjects)) {
        if (attached && attached->m_cacheItem)
            attached->emitChanges();
    }
}

void QQmlDelegateModel::_q_modelAboutToBeReset()
{
    Q_D(QQmlDelegateModel);
    if (!d->m_adaptorModel.adaptsAim())
        return;
    auto aim = d->m_adaptorModel.aim();
    auto oldRoleNames = aim->roleNames();
    // this relies on the fact that modelAboutToBeReset must be followed
    // by a modelReset signal before any further modelAboutToBeReset can occur
    QObject::connect(aim, &QAbstractItemModel::modelReset, this, [this, d, oldRoleNames, aim](){
        if (!d->m_adaptorModel.adaptsAim() || d->m_adaptorModel.aim() != aim)
            return;
        if (oldRoleNames == aim->roleNames()) {
            // if the rolenames stayed the same (most common case), then we don't have
            // to throw away all the setup that we did
            handleModelReset();
        } else {
            // If they did change, we give up and just start from scratch via setMode
            setModel(QVariant::fromValue(model()));
            // but we still have to call handleModelReset, otherwise views will
            // not refresh
            handleModelReset();
        }
    }, Qt::SingleShotConnection);
}

void QQmlDelegateModel::handleModelReset()
{
    Q_D(QQmlDelegateModel);
    if (!d->m_delegate)
        return;

    int oldCount = d->m_count;
    d->m_adaptorModel.rootIndex = QModelIndex();

    if (d->m_complete) {
        d->m_count = d->adaptorModelCount();

        const QList<QQmlDelegateModelItem *> cache = d->m_cache;
        for (QQmlDelegateModelItem *item : cache)
            item->referenceObject();

        for (int i = 0, c = cache.size();  i < c; ++i) {
            QQmlDelegateModelItem *item = cache.at(i);
            // layout change triggered by changing the modelIndex might have
            // already invalidated this item in d->m_cache and deleted it.
            if (!d->m_cache.isSharedWith(cache) && !d->m_cache.contains(item))
                continue;

            if (item->modelIndex() != -1)
                item->setModelIndex(-1, -1, -1);
        }

        for (QQmlDelegateModelItem *item : cache)
            item->releaseObject();
        QVector<Compositor::Remove> removes;
        QVector<Compositor::Insert> inserts;
        if (oldCount)
            d->m_compositor.listItemsRemoved(&d->m_adaptorModel, 0, oldCount, &removes);
        if (d->m_count)
            d->m_compositor.listItemsInserted(&d->m_adaptorModel, 0, d->m_count, &inserts);
        d->itemsMoved(removes, inserts);
        d->m_reset = true;

        if (d->m_adaptorModel.canFetchMore())
            d->m_adaptorModel.fetchMore();

        d->emitChanges();
    }
    emit rootIndexChanged();
}

void QQmlDelegateModel::_q_rowsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQmlDelegateModel);
    if (parent == d->m_adaptorModel.rootIndex)
        _q_itemsInserted(begin, end - begin + 1);
}

void QQmlDelegateModel::_q_rowsAboutToBeRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQmlDelegateModel);
    if (!d->m_adaptorModel.rootIndex.isValid())
        return;
    const QModelIndex index = d->m_adaptorModel.rootIndex;
    if (index.parent() == parent && index.row() >= begin && index.row() <= end) {
        const int oldCount = d->m_count;
        d->m_count = 0;
        d->disconnectFromAbstractItemModel();
        d->m_adaptorModel.invalidateModel();

        if (d->m_complete && oldCount > 0) {
            QVector<Compositor::Remove> removes;
            d->m_compositor.listItemsRemoved(&d->m_adaptorModel, 0, oldCount, &removes);
            d->itemsRemoved(removes);
            d->emitChanges();
        }
    }
}

void QQmlDelegateModel::_q_rowsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQmlDelegateModel);
    if (parent == d->m_adaptorModel.rootIndex)
        _q_itemsRemoved(begin, end - begin + 1);
}

void QQmlDelegateModel::_q_rowsMoved(
        const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
        const QModelIndex &destinationParent, int destinationRow)
{
   Q_D(QQmlDelegateModel);
    const int count = sourceEnd - sourceStart + 1;
    if (destinationParent == d->m_adaptorModel.rootIndex && sourceParent == d->m_adaptorModel.rootIndex) {
        _q_itemsMoved(sourceStart, sourceStart > destinationRow ? destinationRow : destinationRow - count, count);
    } else if (sourceParent == d->m_adaptorModel.rootIndex) {
        _q_itemsRemoved(sourceStart, count);
    } else if (destinationParent == d->m_adaptorModel.rootIndex) {
        _q_itemsInserted(destinationRow, count);
    }
}

void QQmlDelegateModel::_q_columnsInserted(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQmlDelegateModel);
    Q_UNUSED(end);
    if (parent == d->m_adaptorModel.rootIndex && begin == 0) {
        // mark all items as changed
        _q_itemsChanged(0, d->m_count, QVector<int>());
    }
}

void QQmlDelegateModel::_q_columnsRemoved(const QModelIndex &parent, int begin, int end)
{
    Q_D(QQmlDelegateModel);
    Q_UNUSED(end);
    if (parent == d->m_adaptorModel.rootIndex && begin == 0) {
        // mark all items as changed
        _q_itemsChanged(0, d->m_count, QVector<int>());
    }
}

void QQmlDelegateModel::_q_columnsMoved(const QModelIndex &parent, int start, int end,
                                        const QModelIndex &destination, int column)
{
    Q_D(QQmlDelegateModel);
    Q_UNUSED(end);
    if ((parent == d->m_adaptorModel.rootIndex && start == 0)
        || (destination == d->m_adaptorModel.rootIndex && column == 0)) {
        // mark all items as changed
        _q_itemsChanged(0, d->m_count, QVector<int>());
    }
}

void QQmlDelegateModel::_q_dataChanged(const QModelIndex &begin, const QModelIndex &end, const QVector<int> &roles)
{
    Q_D(QQmlDelegateModel);
    if (begin.parent() == d->m_adaptorModel.rootIndex)
        _q_itemsChanged(begin.row(), end.row() - begin.row() + 1, roles);
}

bool QQmlDelegateModel::isDescendantOf(const QPersistentModelIndex& desc, const QList< QPersistentModelIndex >& parents) const
{
    for (int i = 0, c = parents.size(); i < c; ++i) {
        for (QPersistentModelIndex parent = desc; parent.isValid(); parent = parent.parent()) {
            if (parent == parents[i])
                return true;
        }
    }

    return false;
}

void QQmlDelegateModel::_q_layoutChanged(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_D(QQmlDelegateModel);
    if (!d->m_complete)
        return;

    if (hint == QAbstractItemModel::VerticalSortHint) {
        if (!parents.isEmpty() && d->m_adaptorModel.rootIndex.isValid() && !isDescendantOf(d->m_adaptorModel.rootIndex, parents)) {
            return;
        }

        // mark all items as changed
        _q_itemsChanged(0, d->m_count, QVector<int>());

    } else if (hint == QAbstractItemModel::HorizontalSortHint) {
        // Ignored
    } else {
        // We don't know what's going on, so reset the model
        handleModelReset();
    }
}

QQmlDelegateModelAttached *QQmlDelegateModel::qmlAttachedProperties(QObject *obj)
{
    if (QQmlDelegateModelItem *cacheItem = QQmlDelegateModelItem::dataForObject(obj))
        return new QQmlDelegateModelAttached(cacheItem, obj);
    return new QQmlDelegateModelAttached(obj);
}

QQmlDelegateModelPrivate::InsertionResult
QQmlDelegateModelPrivate::insert(Compositor::insert_iterator &before, const QV4::Value &object, int groups)
{
    if (!m_context || !m_context->isValid())
        return InsertionResult::Error;

    QQmlDelegateModelItem *cacheItem = m_adaptorModel.createItem(m_cacheMetaType, -1);
    if (!cacheItem)
        return InsertionResult::Error;
    if (!object.isObject())
        return InsertionResult::Error;

    QV4::ExecutionEngine *v4 = object.as<QV4::Object>()->engine();
    QV4::Scope scope(v4);
    QV4::ScopedObject o(scope, object);
    if (!o)
        return InsertionResult::Error;

    QV4::ObjectIterator it(scope, o, QV4::ObjectIterator::EnumerableOnly);
    QV4::ScopedValue propertyName(scope);
    QV4::ScopedValue v(scope);
    const auto oldCache = m_cache;
    while (1) {
        propertyName = it.nextPropertyNameAsString(v);
        if (propertyName->isNull())
            break;
        cacheItem->setValue(
                    propertyName->toQStringNoThrow(),
                    QV4::ExecutionEngine::toVariant(v, QMetaType {}));
    }
    const bool cacheModified = !m_cache.isSharedWith(oldCache);
    if (cacheModified)
        return InsertionResult::Retry;

    cacheItem->groups = groups | Compositor::UnresolvedFlag | Compositor::CacheFlag;

    // Must be before the new object is inserted into the cache or its indexes will be adjusted too.
    itemsInserted(QVector<Compositor::Insert>(1, Compositor::Insert(before, 1, cacheItem->groups & ~Compositor::CacheFlag)));

    m_cache.insert(before.cacheIndex(), cacheItem);
    m_compositor.insert(before, nullptr, 0, 1, cacheItem->groups);

    return InsertionResult::Success;
}

//============================================================================

QQmlDelegateModelItemMetaType::QQmlDelegateModelItemMetaType(
        QV4::ExecutionEngine *engine, QQmlDelegateModel *model, const QStringList &groupNames)
    : model(model)
    , groupCount(groupNames.size() + 1)
    , v4Engine(engine)
    , groupNames(groupNames)
{
}

QQmlDelegateModelItemMetaType::~QQmlDelegateModelItemMetaType() = default;

void QQmlDelegateModelItemMetaType::initializeAttachedMetaObject()
{
    QMetaObjectBuilder builder;
    builder.setFlags(DynamicMetaObject);
    builder.setClassName(QQmlDelegateModelAttached::staticMetaObject.className());
    builder.setSuperClass(&QQmlDelegateModelAttached::staticMetaObject);

    int notifierId = 0;
    for (int i = 0; i < groupNames.size(); ++i, ++notifierId) {
        QString propertyName = QLatin1String("in") + groupNames.at(i);
        propertyName.replace(2, 1, propertyName.at(2).toUpper());
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "bool", notifierId);
        propertyBuilder.setWritable(true);
    }
    for (int i = 0; i < groupNames.size(); ++i, ++notifierId) {
        const QString propertyName = groupNames.at(i) + QLatin1String("Index");
        builder.addSignal("__" + propertyName.toUtf8() + "Changed()");
        QMetaPropertyBuilder propertyBuilder = builder.addProperty(
                propertyName.toUtf8(), "int", notifierId);
        propertyBuilder.setWritable(true);
    }

    attachedMetaObject = QQml::makeRefPointer<QQmlDelegateModelAttachedMetaObject>(
            this, builder.toMetaObject());
}

void QQmlDelegateModelItemMetaType::initializePrototype()
{
    QV4::Scope scope(v4Engine);

    QV4::ScopedObject proto(scope, v4Engine->newObject());
    proto->defineAccessorProperty(QStringLiteral("model"), QQmlDelegateModelItem::get_model, nullptr);
    proto->defineAccessorProperty(QStringLiteral("groups"), QQmlDelegateModelItem::get_groups, QQmlDelegateModelItem::set_groups);
    QV4::ScopedString s(scope);
    QV4::ScopedProperty p(scope);

    s = v4Engine->newString(QStringLiteral("isUnresolved"));
    QV4::ScopedFunctionObject f(scope);
    QV4::ExecutionEngine *engine = scope.engine;
    p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, 30, QQmlDelegateModelItem::get_member)));
    p->setSetter(nullptr);
    proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);

    s = v4Engine->newString(QStringLiteral("inItems"));
    p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Default, QQmlDelegateModelItem::get_member)));
    p->setSetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Default, QQmlDelegateModelItem::set_member)));
    proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);

    s = v4Engine->newString(QStringLiteral("inPersistedItems"));
    p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Persisted, QQmlDelegateModelItem::get_member)));
    p->setSetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Persisted, QQmlDelegateModelItem::set_member)));
    proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);

    s = v4Engine->newString(QStringLiteral("itemsIndex"));
    p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Default, QQmlDelegateModelItem::get_index)));
    proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);

    s = v4Engine->newString(QStringLiteral("persistedItemsIndex"));
    p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, QQmlListCompositor::Persisted, QQmlDelegateModelItem::get_index)));
    p->setSetter(nullptr);
    proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);

    for (int i = 2; i < groupNames.size(); ++i) {
        QString propertyName = QLatin1String("in") + groupNames.at(i);
        propertyName.replace(2, 1, propertyName.at(2).toUpper());
        s = v4Engine->newString(propertyName);
        p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, i + 1, QQmlDelegateModelItem::get_member)));
        p->setSetter((f = QV4::DelegateModelGroupFunction::create(engine, i + 1, QQmlDelegateModelItem::set_member)));
        proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);
    }
    for (int i = 2; i < groupNames.size(); ++i) {
        const QString propertyName = groupNames.at(i) + QLatin1String("Index");
        s = v4Engine->newString(propertyName);
        p->setGetter((f = QV4::DelegateModelGroupFunction::create(engine, i + 1, QQmlDelegateModelItem::get_index)));
        p->setSetter(nullptr);
        proto->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotConfigurable|QV4::Attr_NotEnumerable);
    }
    modelItemProto.set(v4Engine, proto);
}

int QQmlDelegateModelItemMetaType::parseGroups(const QStringList &groups) const
{
    int groupFlags = 0;
    for (const QString &groupName : groups) {
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
    }
    return groupFlags;
}

int QQmlDelegateModelItemMetaType::parseGroups(const QV4::Value &groups) const
{
    int groupFlags = 0;
    QV4::Scope scope(v4Engine);

    QV4::ScopedString s(scope, groups);
    if (s) {
        const QString groupName = s->toQString();
        int index = groupNames.indexOf(groupName);
        if (index != -1)
            groupFlags |= 2 << index;
        return groupFlags;
    }

    QV4::ScopedArrayObject array(scope, groups);
    if (array) {
        QV4::ScopedValue v(scope);
        uint arrayLength = array->getLength();
        for (uint i = 0; i < arrayLength; ++i) {
            v = array->get(i);
            const QString groupName = v->toQString();
            int index = groupNames.indexOf(groupName);
            if (index != -1)
                groupFlags |= 2 << index;
        }
    }
    return groupFlags;
}

QV4::ReturnedValue QQmlDelegateModelItem::get_model(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return b->engine()->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));
    if (!o->d()->item->metaType->model)
        RETURN_UNDEFINED();

    return o->d()->item->get();
}

QV4::ReturnedValue QQmlDelegateModelItem::get_groups(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

    QStringList groups;
    for (int i = 1; i < o->d()->item->metaType->groupCount; ++i) {
        if (o->d()->item->groups & (1 << i))
            groups.append(o->d()->item->metaType->groupNames.at(i - 1));
    }

    return scope.engine->fromVariant(groups);
}

QV4::ReturnedValue QQmlDelegateModelItem::set_groups(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    QV4::Scoped<QQmlDelegateModelItemObject> o(scope, thisObject->as<QQmlDelegateModelItemObject>());
    if (!o)
        return scope.engine->throwTypeError(QStringLiteral("Not a valid DelegateModel object"));

    if (!argc)
        THROW_TYPE_ERROR();

    if (!o->d()->item->metaType->model)
        RETURN_UNDEFINED();
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(o->d()->item->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(argv[0]);
    const int cacheIndex = model->m_cache.indexOf(o->d()->item);
    Compositor::iterator it = model->m_compositor.find(Compositor::Cache, cacheIndex);
    model->setGroups(it, 1, Compositor::Cache, groupFlags);
    return QV4::Encode::undefined();
}

QV4::ReturnedValue QQmlDelegateModelItem::get_member(QQmlDelegateModelItem *thisItem, uint flag, const QV4::Value &)
{
    return QV4::Encode(bool(thisItem->groups & (1 << flag)));
}

QV4::ReturnedValue QQmlDelegateModelItem::set_member(QQmlDelegateModelItem *cacheItem, uint flag, const QV4::Value &arg)
{
    if (!cacheItem->metaType->model)
        return QV4::Encode::undefined();

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(cacheItem->metaType->model);

    bool member = arg.toBoolean();
    uint groupFlag = (1 << flag);
    if (member == ((cacheItem->groups & groupFlag) != 0))
        return QV4::Encode::undefined();

    const int cacheIndex = model->m_cache.indexOf(cacheItem);
    Compositor::iterator it = model->m_compositor.find(Compositor::Cache, cacheIndex);
    if (member)
        model->addGroups(it, 1, Compositor::Cache, groupFlag);
    else
        model->removeGroups(it, 1, Compositor::Cache, groupFlag);
    return QV4::Encode::undefined();
}

QV4::ReturnedValue QQmlDelegateModelItem::get_index(QQmlDelegateModelItem *thisItem, uint flag, const QV4::Value &)
{
    return QV4::Encode((int)thisItem->groupIndex(Compositor::Group(flag)));
}

void QQmlDelegateModelItem::childContextObjectDestroyed(QObject *childContextObject)
{
    if (!contextData)
        return;

    for (QQmlRefPointer<QQmlContextData> ctxt = contextData->childContexts(); ctxt;
         ctxt = ctxt->nextChild()) {
        ctxt->deepClearContextObject(childContextObject);
    }
}


//---------------------------------------------------------------------------

DEFINE_OBJECT_VTABLE(QQmlDelegateModelItemObject);

void QV4::Heap::QQmlDelegateModelItemObject::destroy()
{
    item->dispose();
    Object::destroy();
}


QQmlDelegateModelItem::QQmlDelegateModelItem(
        const QQmlRefPointer<QQmlDelegateModelItemMetaType> &metaType,
        QQmlAdaptorModel::Accessors *accessor,
        int modelIndex, int row, int column)
    : metaType(metaType)
    , index(modelIndex)
    , row(row)
    , column(column)
{
    if (accessor->propertyCache) {
        // The property cache in the accessor is common for all the model
        // items in the model it wraps. It describes available model roles,
        // together with revisioned properties like row, column and index, all
        // which should be available in the delegate. We assign this cache to the
        // model item so that the QML engine can use the revision information
        // when resolving the properties (rather than falling back to just
        // inspecting the QObject in the model item directly).
        QQmlData::get(this, true)->propertyCache = accessor->propertyCache;
    }
}

QQmlDelegateModelItem::~QQmlDelegateModelItem()
{
    Q_ASSERT(scriptRef == 0);
    Q_ASSERT(objectRef == 0);
    Q_ASSERT(!object);

    if (incubationTask) {
        if (metaType->model)
            QQmlDelegateModelPrivate::get(metaType->model)->releaseIncubator(incubationTask);
        else
            delete incubationTask;
    }
}

void QQmlDelegateModelItem::dispose()
{
    --scriptRef;
    if (isReferenced())
        return;

    if (metaType->model) {
        QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(metaType->model);
        model->removeCacheItem(this);
    }
    delete this;
}

void QQmlDelegateModelItem::setModelIndex(int idx, int newRow, int newColumn, bool alwaysEmit)
{
    const int prevIndex = index;
    const int prevRow = row;
    const int prevColumn = column;

    index = idx;
    row = newRow;
    column = newColumn;

    if (idx != prevIndex || alwaysEmit)
        emit modelIndexChanged();
    if (row != prevRow || alwaysEmit)
        emit rowChanged();
    if (column != prevColumn || alwaysEmit)
        emit columnChanged();
}

void QQmlDelegateModelItem::destroyObject()
{
    Q_ASSERT(object);
    Q_ASSERT(contextData);

    QQmlData *data = QQmlData::get(object);
    Q_ASSERT(data);
    if (data->ownContext) {
        data->ownContext->clearContext();
        data->ownContext->deepClearContextObject(object);
        data->ownContext.reset();
        data->context = nullptr;
    }
    /* QTBUG-87228: when destroying object at the application exit, the deferred
     * parent by setting it to QCoreApplication instance if it's nullptr, so
     * deletion won't work. Not to leak memory, make sure our object has a that
     * the parent claims the object at the end of the lifetime. When not at the
     * application exit, normal event loop will handle the deferred deletion
     * earlier.
     */
    if (Q_UNLIKELY(static_cast<QCoreApplicationPrivate *>(QCoreApplicationPrivate::get(QCoreApplication::instance()))->aboutToQuitEmitted)) {
        if (object->parent() == nullptr)
            object->setParent(QCoreApplication::instance());
    }
    object->deleteLater();

    if (QQmlDelegateModelAttached *attachedObject = attached())
        attachedObject->m_cacheItem = nullptr;

    contextData.reset();
    object = nullptr;
}

QQmlDelegateModelItem *QQmlDelegateModelItem::dataForObject(QObject *object)
{
    QQmlData *d = QQmlData::get(object);
    if (!d)
        return nullptr;

    QQmlRefPointer<QQmlContextData> context = d->context;
    if (!context || !context->isValid())
        return nullptr;

    if (QObject *extraObject = context->extraObject())
        return qobject_cast<QQmlDelegateModelItem *>(extraObject);

    for (context = context->parent(); context; context = context->parent()) {
        if (QObject *extraObject = context->extraObject())
            return qobject_cast<QQmlDelegateModelItem *>(extraObject);
        if (QQmlDelegateModelItem *cacheItem = qobject_cast<QQmlDelegateModelItem *>(
                context->contextObject())) {
            return cacheItem;
        }
    }
    return nullptr;
}

int QQmlDelegateModelItem::groupIndex(Compositor::Group group)
{
    if (QQmlDelegateModelPrivate * const model = metaType->model
            ? QQmlDelegateModelPrivate::get(metaType->model)
            : nullptr) {
        return model->m_compositor.find(Compositor::Cache, model->m_cache.indexOf(this)).index[group];
    }
    return -1;
}

//---------------------------------------------------------------------------

QQmlDelegateModelAttachedMetaObject::QQmlDelegateModelAttachedMetaObject(
        QQmlDelegateModelItemMetaType *metaType, QMetaObject *metaObject)
    : metaType(metaType)
    , metaObject(metaObject)
    , memberPropertyOffset(QQmlDelegateModelAttached::staticMetaObject.propertyCount())
    , indexPropertyOffset(QQmlDelegateModelAttached::staticMetaObject.propertyCount() + metaType->groupNames.size())
{
    // Don't reference count the meta-type here as that would create a circular reference.
    // Instead we rely the fact that the meta-type's reference count can't reach 0 without first
    // destroying all delegates with attached objects.
    *static_cast<QMetaObject *>(this) = *metaObject;
}

QQmlDelegateModelAttachedMetaObject::~QQmlDelegateModelAttachedMetaObject()
{
    ::free(metaObject);
}

void QQmlDelegateModelAttachedMetaObject::objectDestroyed(QObject *)
{
    release();
}

int QQmlDelegateModelAttachedMetaObject::metaCall(QObject *object, QMetaObject::Call call, int _id, void **arguments)
{
    QQmlDelegateModelAttached *attached = static_cast<QQmlDelegateModelAttached *>(object);
    if (call == QMetaObject::ReadProperty) {
        if (_id >= indexPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - indexPropertyOffset + 1);
            *static_cast<int *>(arguments[0]) = attached->m_currentIndex[group];
            return -1;
        } else if (_id >= memberPropertyOffset) {
            Compositor::Group group = Compositor::Group(_id - memberPropertyOffset + 1);
            *static_cast<bool *>(arguments[0]) = attached->m_cacheItem->groups & (1 << group);
            return -1;
        }
    } else if (call == QMetaObject::WriteProperty) {
        if (_id >= memberPropertyOffset) {
            if (!metaType->model)
                return -1;
            QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(metaType->model);
            Compositor::Group group = Compositor::Group(_id - memberPropertyOffset + 1);
            const int groupFlag = 1 << group;
            const bool member = attached->m_cacheItem->groups & groupFlag;
            if (member && !*static_cast<bool *>(arguments[0])) {
                Compositor::iterator it = model->m_compositor.find(
                        group, attached->m_currentIndex[group]);
                model->removeGroups(it, 1, group, groupFlag);
            } else if (!member && *static_cast<bool *>(arguments[0])) {
                for (int i = 1; i < metaType->groupCount; ++i) {
                    if (attached->m_cacheItem->groups & (1 << i)) {
                        Compositor::iterator it = model->m_compositor.find(
                                Compositor::Group(i), attached->m_currentIndex[i]);
                        model->addGroups(it, 1, Compositor::Group(i), groupFlag);
                        break;
                    }
                }
            }
            return -1;
        }
    }
    return attached->qt_metacall(call, _id, arguments);
}

QQmlDelegateModelAttached::QQmlDelegateModelAttached(QObject *parent)
    : m_cacheItem(nullptr)
    , m_previousGroups(0)
{
    QQml_setParent_noEvent(this, parent);
}

QQmlDelegateModelAttached::QQmlDelegateModelAttached(
        QQmlDelegateModelItem *cacheItem, QObject *parent)
    : m_cacheItem(cacheItem)
    , m_previousGroups(cacheItem->groups)
{
    QQml_setParent_noEvent(this, parent);
    resetCurrentIndex();
    // Let m_previousIndex be equal to m_currentIndex
    std::copy(std::begin(m_currentIndex), std::end(m_currentIndex), std::begin(m_previousIndex));

    if (!cacheItem->metaType->attachedMetaObject)
        cacheItem->metaType->initializeAttachedMetaObject();

    QObjectPrivate::get(this)->metaObject = cacheItem->metaType->attachedMetaObject.data();
    cacheItem->metaType->attachedMetaObject->addref();
}

QQmlDelegateModelAttached::~QQmlDelegateModelAttached() = default;

void QQmlDelegateModelAttached::resetCurrentIndex()
{
    if (QQDMIncubationTask *incubationTask = m_cacheItem->incubationTask) {
        for (int i = 1; i < qMin<int>(m_cacheItem->metaType->groupCount, Compositor::MaximumGroupCount); ++i)
            m_currentIndex[i] = incubationTask->index[i];
    } else {
        QQmlDelegateModelPrivate * const model = QQmlDelegateModelPrivate::get(m_cacheItem->metaType->model);
        Compositor::iterator it = model->m_compositor.find(
                Compositor::Cache, model->m_cache.indexOf(m_cacheItem));
        for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i)
            m_currentIndex[i] = it.index[i];
    }
}

void QQmlDelegateModelAttached::setInPersistedItems(bool inPersisted)
{
    setInGroup(QQmlListCompositor::Persisted, inPersisted);
}

bool QQmlDelegateModelAttached::inPersistedItems() const
{
    if (!m_cacheItem)
        return false;
    const uint groupFlag = (1 << QQmlListCompositor::Persisted);
    return m_cacheItem->groups & groupFlag;
}

int QQmlDelegateModelAttached::persistedItemsIndex() const
{
    if (!m_cacheItem)
        return -1;
    return m_cacheItem->groupIndex(QQmlListCompositor::Persisted);
}

void QQmlDelegateModelAttached::setInGroup(QQmlListCompositor::Group group, bool inGroup)
{
    if (!(m_cacheItem && m_cacheItem->metaType && m_cacheItem->metaType->model))
        return;
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_cacheItem->metaType->model);
    const uint groupFlag = (1 << group);
    if (inGroup == bool(m_cacheItem->groups & groupFlag))
        return;

    const int cacheIndex = model->m_cache.indexOf(m_cacheItem);
    Compositor::iterator it = model->m_compositor.find(Compositor::Cache, cacheIndex);
    if (inGroup)
        model->addGroups(it, 1, Compositor::Cache, groupFlag);
    else
        model->removeGroups(it, 1, Compositor::Cache, groupFlag);
    // signal emission happens in add-/removeGroups
}

void QQmlDelegateModelAttached::setInItems(bool inItems)
{
    setInGroup(QQmlListCompositor::Default, inItems);
}

bool QQmlDelegateModelAttached::inItems() const
{
    if (!m_cacheItem)
        return false;
    const uint groupFlag = (1 << QQmlListCompositor::Default);
    return m_cacheItem->groups & groupFlag;
}

int QQmlDelegateModelAttached::itemsIndex() const
{
    if (!m_cacheItem)
        return -1;
    return m_cacheItem->groupIndex(QQmlListCompositor::Default);
}

/*!
    \qmlattachedproperty model QtQml.Models::DelegateModel::model

    This attached property holds the data model this delegate instance belongs to.

    It is attached to each instance of the delegate.
*/

QQmlDelegateModel *QQmlDelegateModelAttached::model() const
{
    return m_cacheItem ? m_cacheItem->metaType->model : nullptr;
}

/*!
    \qmlattachedproperty stringlist QtQml.Models::DelegateModel::groups

    This attached property holds the name of DelegateModelGroups the item belongs to.

    It is attached to each instance of the delegate.
*/

QStringList QQmlDelegateModelAttached::groups() const
{
    QStringList groups;

    if (!m_cacheItem)
        return groups;
    for (int i = 1; i < m_cacheItem->metaType->groupCount; ++i) {
        if (m_cacheItem->groups & (1 << i))
            groups.append(m_cacheItem->metaType->groupNames.at(i - 1));
    }
    return groups;
}

void QQmlDelegateModelAttached::setGroups(const QStringList &groups)
{
    if (!m_cacheItem)
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_cacheItem->metaType->model);

    const int groupFlags = model->m_cacheMetaType->parseGroups(groups);
    const int cacheIndex = model->m_cache.indexOf(m_cacheItem);
    Compositor::iterator it = model->m_compositor.find(Compositor::Cache, cacheIndex);
    model->setGroups(it, 1, Compositor::Cache, groupFlags);
}

/*!
    \qmlattachedproperty bool QtQml.Models::DelegateModel::isUnresolved

    This attached property indicates whether the visual item is bound to a data model index.
    Returns true if the item is not bound to the model, and false if it is.

    An unresolved item can be bound to the data model using the DelegateModelGroup::resolve()
    function.

    It is attached to each instance of the delegate.
*/

bool QQmlDelegateModelAttached::isUnresolved() const
{
    if (!m_cacheItem)
        return false;

    return m_cacheItem->groups & Compositor::UnresolvedFlag;
}

/*!
    \qmlattachedproperty bool QtQml.Models::DelegateModel::inItems

    This attached property holds whether the item belongs to the default \l items
    DelegateModelGroup.

    Changing this property will add or remove the item from the items group.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQml.Models::DelegateModel::itemsIndex

    This attached property holds the index of the item in the default \l items DelegateModelGroup.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty bool QtQml.Models::DelegateModel::inPersistedItems

    This attached property holds whether the item belongs to the \l persistedItems
    DelegateModelGroup.

    Changing this property will add or remove the item from the items group.  Change with caution
    as removing an item from the persistedItems group will destroy the current instance if it is
    not referenced by a model.

    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedproperty int QtQml.Models::DelegateModel::persistedItemsIndex

    This attached property holds the index of the item in the \l persistedItems DelegateModelGroup.

    It is attached to each instance of the delegate.
*/

void QQmlDelegateModelAttached::emitChanges()
{
    const int groupChanges = m_previousGroups ^ m_cacheItem->groups;
    m_previousGroups = m_cacheItem->groups;

    int indexChanges = 0;
    const int groupCount = m_cacheItem->metaType->groupCount;
    for (int i = 1; i < groupCount; ++i) {
        if (m_previousIndex[i] != m_currentIndex[i]) {
            m_previousIndex[i] = m_currentIndex[i];
            indexChanges |= (1 << i);
        }
    }

    // Don't access m_cacheItem anymore once we've started sending signals.
    // We don't own it and someone might delete it.

    int notifierId = 0;
    const QMetaObject *meta = metaObject();
    for (int i = 1; i < groupCount; ++i, ++notifierId) {
        if (groupChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, nullptr);
    }
    for (int i = 1; i < groupCount; ++i, ++notifierId) {
        if (indexChanges & (1 << i))
            QMetaObject::activate(this, meta, notifierId, nullptr);
    }

    if (groupChanges)
        emit groupsChanged();
}

//============================================================================

void QQmlDelegateModelGroupPrivate::setModel(QQmlDelegateModel *m, Compositor::Group g)
{
    Q_ASSERT(!model);
    model = m;
    group = g;
}

bool QQmlDelegateModelGroupPrivate::isChangedConnected()
{
    Q_Q(QQmlDelegateModelGroup);
    IS_SIGNAL_CONNECTED(q, QQmlDelegateModelGroup, changed, (const QJSValue &,const QJSValue &));
}

void QQmlDelegateModelGroupPrivate::emitChanges(QV4::ExecutionEngine *v4)
{
    Q_Q(QQmlDelegateModelGroup);
    if (isChangedConnected() && !changeSet.isEmpty()) {
        emit q->changed(QJSValuePrivate::fromReturnedValue(
                            qdmEngineData(v4)->array(v4, changeSet.removes())),
                        QJSValuePrivate::fromReturnedValue(
                            qdmEngineData(v4)->array(v4, changeSet.inserts())));
    }
    if (changeSet.difference() != 0)
        emit q->countChanged();
}

void QQmlDelegateModelGroupPrivate::emitModelUpdated(bool reset)
{
    for (QQmlDelegateModelGroupEmitterList::iterator it = emitters.begin(); it != emitters.end(); ++it)
        it->emitModelUpdated(changeSet, reset);
    changeSet.clear();
}

typedef QQmlDelegateModelGroupEmitterList::iterator GroupEmitterListIt;

void QQmlDelegateModelGroupPrivate::createdPackage(int index, QQuickPackage *package)
{
    for (GroupEmitterListIt it = emitters.begin(), end = emitters.end(); it != end; ++it)
        it->createdPackage(index, package);
}

void QQmlDelegateModelGroupPrivate::initPackage(int index, QQuickPackage *package)
{
    for (GroupEmitterListIt it = emitters.begin(), end = emitters.end(); it != end; ++it)
        it->initPackage(index, package);
}

void QQmlDelegateModelGroupPrivate::destroyingPackage(QQuickPackage *package)
{
    for (GroupEmitterListIt it = emitters.begin(), end = emitters.end(); it != end; ++it)
        it->destroyingPackage(package);
}

/*!
    \qmltype DelegateModelGroup
    \nativetype QQmlDelegateModelGroup
    \inqmlmodule QtQml.Models
    \ingroup qtquick-models
    \brief Encapsulates a filtered set of visual data items.

    The DelegateModelGroup type provides a means to address the model data of a
    DelegateModel's delegate items, as well as sort and filter these delegate
    items.

    The initial set of instantiable delegate items in a DelegateModel is represented
    by its \l {QtQml.Models::DelegateModel::items}{items} group, which normally directly reflects
    the contents of the model assigned to DelegateModel::model.  This set can be changed to
    the contents of any other member of DelegateModel::groups by assigning the  \l name of that
    DelegateModelGroup to the DelegateModel::filterOnGroup property.

    The data of an item in a DelegateModelGroup can be accessed using the get() function, which returns
    information about group membership and indexes as well as model data.  In combination
    with the move() function this can be used to implement view sorting, with remove() to filter
    items out of a view, or with setGroups() and \l Package delegates to categorize items into
    different views. Different groups can only be sorted independently if they are disjunct. Moving
    an item in one group will also move it in all other groups it is a part of.

    Data from models can be supplemented by inserting data directly into a DelegateModelGroup
    with the insert() function.  This can be used to introduce mock items into a view, or
    placeholder items that are later \l {resolve()}{resolved} to real model data when it becomes
    available.

    Delegate items can also be instantiated directly from a DelegateModelGroup using the
    create() function, making it possible to use DelegateModel without an accompanying view
    type or to cherry-pick specific items that should be instantiated irregardless of whether
    they're currently within a view's visible area.

    \sa {QML Dynamic View Ordering Tutorial}
*/
QQmlDelegateModelGroup::QQmlDelegateModelGroup(QObject *parent)
    : QObject(*new QQmlDelegateModelGroupPrivate, parent)
{
}

QQmlDelegateModelGroup::QQmlDelegateModelGroup(
        const QString &name, QQmlDelegateModel *model, int index, QObject *parent)
    : QQmlDelegateModelGroup(parent)
{
    Q_D(QQmlDelegateModelGroup);
    d->name = name;
    d->setModel(model, Compositor::Group(index));
}

QQmlDelegateModelGroup::~QQmlDelegateModelGroup() = default;

/*!
    \qmlproperty string QtQml.Models::DelegateModelGroup::name

    This property holds the name of the group.

    Each group in a model must have a unique name starting with a lower case letter.
*/

QString QQmlDelegateModelGroup::name() const
{
    Q_D(const QQmlDelegateModelGroup);
    return d->name;
}

void QQmlDelegateModelGroup::setName(const QString &name)
{
    Q_D(QQmlDelegateModelGroup);
    if (d->model)
        return;
    if (d->name != name) {
        d->name = name;
        emit nameChanged();
    }
}

/*!
    \qmlproperty int QtQml.Models::DelegateModelGroup::count

    This property holds the number of items in the group.
*/

int QQmlDelegateModelGroup::count() const
{
    Q_D(const QQmlDelegateModelGroup);
    if (!d->model)
        return 0;
    return QQmlDelegateModelPrivate::get(d->model)->m_compositor.count(d->group);
}

/*!
    \qmlproperty bool QtQml.Models::DelegateModelGroup::includeByDefault

    This property holds whether new items are assigned to this group by default.
*/

bool QQmlDelegateModelGroup::defaultInclude() const
{
    Q_D(const QQmlDelegateModelGroup);
    return d->defaultInclude;
}

void QQmlDelegateModelGroup::setDefaultInclude(bool include)
{
    Q_D(QQmlDelegateModelGroup);
    if (d->defaultInclude != include) {
        d->defaultInclude = include;

        if (d->model) {
            if (include)
                QQmlDelegateModelPrivate::get(d->model)->m_compositor.setDefaultGroup(d->group);
            else
                QQmlDelegateModelPrivate::get(d->model)->m_compositor.clearDefaultGroup(d->group);
        }
        emit defaultIncludeChanged();
    }
}

/*!
    \qmlmethod object QtQml.Models::DelegateModelGroup::get(int index)

    Returns a javascript object describing the item at \a index in the group.

    The returned object contains the same information that is available to a delegate from the
    DelegateModel attached as well as the model for that item.  It has the properties:

    \list
    \li \b model The model data of the item.  This is the same as the model context property in
    a delegate
    \li \b groups A list the of names of groups the item is a member of.  This property can be
    written to change the item's membership.
    \li \b inItems Whether the item belongs to the \l {QtQml.Models::DelegateModel::items}{items} group.
    Writing to this property will add or remove the item from the group.
    \li \b itemsIndex The index of the item within the \l {QtQml.Models::DelegateModel::items}{items} group.
    \li \b {in<GroupName>} Whether the item belongs to the dynamic group \e groupName.  Writing to
    this property will add or remove the item from the group.
    \li \b {<groupName>Index} The index of the item within the dynamic group \e groupName.
    \li \b isUnresolved Whether the item is bound to an index in the model assigned to
    DelegateModel::model.  Returns true if the item is not bound to the model, and false if it is.
    \endlist
*/

QJSValue QQmlDelegateModelGroup::get(int index)
{
    Q_D(QQmlDelegateModelGroup);
    if (!d->model)
        return QJSValue();

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);
    if (!model->m_context || !model->m_context->isValid()) {
        return QJSValue();
    } else if (index < 0 || index >= model->m_compositor.count(d->group)) {
        qmlWarning(this) << tr("get: index out of range");
        return QJSValue();
    }

    Compositor::iterator it = model->m_compositor.find(d->group, index);
    QQmlDelegateModelItem *cacheItem = it->inCache()
            ? model->m_cache.at(it.cacheIndex())
            : 0;

    if (!cacheItem) {
        cacheItem = model->m_adaptorModel.createItem(
                model->m_cacheMetaType, it.modelIndex());
        if (!cacheItem)
            return QJSValue();
        cacheItem->groups = it->flags;

        model->m_cache.insert(it.cacheIndex(), cacheItem);
        model->m_compositor.setFlags(it, 1, Compositor::CacheFlag);
    }

    if (model->m_cacheMetaType->modelItemProto.isUndefined())
        model->m_cacheMetaType->initializePrototype();
    QV4::ExecutionEngine *v4 = model->m_cacheMetaType->v4Engine;
    QV4::Scope scope(v4);
    ++cacheItem->scriptRef;
    QV4::ScopedObject o(scope, v4->memoryManager->allocate<QQmlDelegateModelItemObject>(cacheItem));
    QV4::ScopedObject p(scope, model->m_cacheMetaType->modelItemProto.value());
    o->setPrototypeOf(p);

    return QJSValuePrivate::fromReturnedValue(o->asReturnedValue());
}

bool QQmlDelegateModelGroupPrivate::parseIndex(const QV4::Value &value, int *index, Compositor::Group *group) const
{
    if (value.isNumber()) {
        *index = value.toInt32();
        return true;
    }

    if (!value.isObject())
        return false;

    QV4::ExecutionEngine *v4 = value.as<QV4::Object>()->engine();
    QV4::Scope scope(v4);
    QV4::Scoped<QQmlDelegateModelItemObject> object(scope, value);

    if (object) {
        QQmlDelegateModelItem * const cacheItem = object->d()->item;
        if (QQmlDelegateModelPrivate *model = cacheItem->metaType->model
                ? QQmlDelegateModelPrivate::get(cacheItem->metaType->model)
                : nullptr) {
            *index = model->m_cache.indexOf(cacheItem);
            *group = Compositor::Cache;
            return true;
        }
    }
    return false;
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::insert(int index, jsdict data, array groups = undefined)
    \qmlmethod QtQml.Models::DelegateModelGroup::insert(jsdict data, var groups = undefined)

    Creates a new entry at \a index in a DelegateModel with the values from \a data that
    correspond to roles in the model assigned to DelegateModel::model.

    If no index is supplied the data is appended to the model.

    The optional \a groups parameter identifies the groups the new entry should belong to,
    if unspecified this is equal to the group insert was called on.

    Data inserted into a DelegateModel can later be merged with an existing entry in
    DelegateModel::model using the \l resolve() function.  This can be used to create placeholder
    items that are later replaced by actual data.
*/

void QQmlDelegateModelGroup::insert(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);

    int index = model->m_compositor.count(d->group);
    Compositor::Group group = d->group;

    if (args->length() == 0)
        return;

    int  i = 0;
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[i]);
    if (d->parseIndex(v, &index, &group)) {
        if (index < 0 || index > model->m_compositor.count(group)) {
            qmlWarning(this) << tr("insert: index out of range");
            return;
        }
        if (++i == args->length())
            return;
        v = (*args)[i];
    }

    if (v->as<QV4::ArrayObject>())
        return;

    int groups = 1 << d->group;
    if (++i < args->length()) {
        QV4::ScopedValue val(scope, (*args)[i]);
        groups |= model->m_cacheMetaType->parseGroups(val);
    }

    if (v->as<QV4::Object>()) {
        auto insertionResult = QQmlDelegateModelPrivate::InsertionResult::Retry;
        do {
            Compositor::insert_iterator before = index < model->m_compositor.count(group)
                    ? model->m_compositor.findInsertPosition(group, index)
                    : model->m_compositor.end();
            insertionResult = model->insert(before, v, groups);
        } while (insertionResult == QQmlDelegateModelPrivate::InsertionResult::Retry);
        if (insertionResult == QQmlDelegateModelPrivate::InsertionResult::Success)
            model->emitChanges();
    }
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::create(int index)
    \qmlmethod QtQml.Models::DelegateModelGroup::create(int index, jsdict data, array groups = undefined)
    \qmlmethod QtQml.Models::DelegateModelGroup::create(jsdict data, array groups = undefined)

    Returns a reference to the instantiated item at \a index in the group.

    If a \a data object is provided it will be \l {insert}{inserted} at \a index and an item
    referencing this new entry will be returned.  The optional \a groups parameter identifies
    the groups the new entry should belong to, if unspecified this is equal to the group create()
    was called on.

    All items returned by create are added to the
    \l {QtQml.Models::DelegateModel::persistedItems}{persistedItems} group.  Items in this
    group remain instantiated when not referenced by any view.
*/

void QQmlDelegateModelGroup::create(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    if (!d->model)
        return;

    if (args->length() == 0)
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);

    int index = model->m_compositor.count(d->group);
    Compositor::Group group = d->group;

    int  i = 0;
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[i]);
    if (d->parseIndex(v, &index, &group))
        ++i;

    if (i < args->length() && index >= 0 && index <= model->m_compositor.count(group)) {
        v = (*args)[i];
        if (v->as<QV4::Object>()) {
            int groups = 1 << d->group;
            if (++i < args->length()) {
                QV4::ScopedValue val(scope, (*args)[i]);
                groups |= model->m_cacheMetaType->parseGroups(val);
            }

            auto insertionResult = QQmlDelegateModelPrivate::InsertionResult::Retry;
            do {
                Compositor::insert_iterator before = index < model->m_compositor.count(group)
                        ? model->m_compositor.findInsertPosition(group, index)
                        : model->m_compositor.end();

                index = before.index[d->group];
                group = d->group;

                insertionResult = model->insert(before, v, groups);
            } while (insertionResult == QQmlDelegateModelPrivate::InsertionResult::Retry);
            if (insertionResult == QQmlDelegateModelPrivate::InsertionResult::Error)
                return;
        }
    }
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlWarning(this) << tr("create: index out of range");
        return;
    }

    QObject *object = model->object(group, index, QQmlIncubator::AsynchronousIfNested);
    if (object) {
        QVector<Compositor::Insert> inserts;
        Compositor::iterator it = model->m_compositor.find(group, index);
        model->m_compositor.setFlags(it, 1, d->group, Compositor::PersistedFlag, &inserts);
        model->itemsInserted(inserts);
        model->m_cache.at(it.cacheIndex())->releaseObject();
    }

    args->setReturnValue(QV4::QObjectWrapper::wrap(args->v4engine(), object));
    model->emitChanges();
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::resolve(int from, int to)

    Binds an unresolved item at \a from to an item in DelegateModel::model at index \a to.

    Unresolved items are entries whose data has been \l {insert()}{inserted} into a DelegateModelGroup
    instead of being derived from a DelegateModel::model index.  Resolving an item will replace
    the item at the target index with the unresolved item. A resolved an item will reflect the data
    of the source model at its bound index and will move when that index moves like any other item.

    If a new item is replaced in the DelegateModelGroup onChanged() handler its insertion and
    replacement will be communicated to views as an atomic operation, creating the appearance
    that the model contents have not changed, or if the unresolved and model item are not adjacent
    that the previously unresolved item has simply moved.

*/
void QQmlDelegateModelGroup::resolve(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    if (!d->model)
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);

    if (args->length() < 2)
        return;

    int from = -1;
    int to = -1;
    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;

    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[0]);
    if (d->parseIndex(v, &from, &fromGroup)) {
        if (from < 0 || from >= model->m_compositor.count(fromGroup)) {
            qmlWarning(this) << tr("resolve: from index out of range");
            return;
        }
    } else {
        qmlWarning(this) << tr("resolve: from index invalid");
        return;
    }

    v = (*args)[1];
    if (d->parseIndex(v, &to, &toGroup)) {
        if (to < 0 || to >= model->m_compositor.count(toGroup)) {
            qmlWarning(this) << tr("resolve: to index out of range");
            return;
        }
    } else {
        qmlWarning(this) << tr("resolve: to index invalid");
        return;
    }

    Compositor::iterator fromIt = model->m_compositor.find(fromGroup, from);
    Compositor::iterator toIt = model->m_compositor.find(toGroup, to);

    if (!fromIt->isUnresolved()) {
        qmlWarning(this) << tr("resolve: from is not an unresolved item");
        return;
    }
    if (!toIt->list) {
        qmlWarning(this) << tr("resolve: to is not a model item");
        return;
    }

    const int unresolvedFlags = fromIt->flags;
    const int resolvedFlags = toIt->flags;
    const int resolvedIndex = toIt.modelIndex();
    void * const resolvedList = toIt->list;

    QQmlDelegateModelItem *cacheItem = model->m_cache.at(fromIt.cacheIndex());
    cacheItem->groups &= ~Compositor::UnresolvedFlag;

    if (toIt.cacheIndex() > fromIt.cacheIndex())
        toIt.decrementIndexes(1, unresolvedFlags);
    if (!toIt->inGroup(fromGroup) || toIt.index[fromGroup] > from)
        from += 1;

    model->itemsMoved(
            QVector<Compositor::Remove>(1, Compositor::Remove(fromIt, 1, unresolvedFlags, 0)),
            QVector<Compositor::Insert>(1, Compositor::Insert(toIt, 1, unresolvedFlags, 0)));
    model->itemsInserted(
            QVector<Compositor::Insert>(1, Compositor::Insert(toIt, 1, (resolvedFlags & ~unresolvedFlags) | Compositor::CacheFlag)));
    toIt.incrementIndexes(1, resolvedFlags | unresolvedFlags);
    model->itemsRemoved(QVector<Compositor::Remove>(1, Compositor::Remove(toIt, 1, resolvedFlags)));

    model->m_compositor.setFlags(toGroup, to, 1, unresolvedFlags & ~Compositor::UnresolvedFlag);
    model->m_compositor.clearFlags(fromGroup, from, 1, unresolvedFlags);

    if (resolvedFlags & Compositor::CacheFlag)
        model->m_compositor.insert(
                    Compositor::Cache, toIt.cacheIndex(), resolvedList,
                    resolvedIndex, 1, Compositor::CacheFlag);

    Q_ASSERT(model->m_cache.size() == model->m_compositor.count(Compositor::Cache));

    if (!cacheItem->isReferenced()) {
        Q_ASSERT(toIt.cacheIndex() == model->m_cache.indexOf(cacheItem));
        model->m_cache.removeAt(toIt.cacheIndex());
        model->m_compositor.clearFlags(
                    Compositor::Cache, toIt.cacheIndex(), 1, Compositor::CacheFlag);
        delete cacheItem;
        Q_ASSERT(model->m_cache.size() == model->m_compositor.count(Compositor::Cache));
    } else {
        cacheItem->resolveIndex(model->m_adaptorModel, resolvedIndex);
        if (QQmlDelegateModelAttached *attached = cacheItem->attached())
            attached->emitUnresolvedChanged();
    }

    model->emitChanges();
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::remove(int index, int count)

    Removes \a count items starting at \a index from the group.
*/

void QQmlDelegateModelGroup::remove(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    if (!d->model)
        return;
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;

    if (args->length() == 0)
        return;

    int i = 0;
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[0]);
    if (!d->parseIndex(v, &index, &group)) {
        qmlWarning(this) << tr("remove: invalid index");
        return;
    }

    if (++i < args->length()) {
        v = (*args)[i];
        if (v->isNumber())
            count = v->toInt32();
    }

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlWarning(this) << tr("remove: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlWarning(this) << tr("remove: invalid count");
        } else {
            model->removeGroups(it, count, d->group, 1 << d->group);
        }
    }
}

bool QQmlDelegateModelGroupPrivate::parseGroupArgs(
        QQmlV4FunctionPtr args, Compositor::Group *group, int *index, int *count, int *groups) const
{
    if (!model || !QQmlDelegateModelPrivate::get(model)->m_cacheMetaType)
        return false;

    if (args->length() < 2)
        return false;

    int i = 0;
    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[i]);
    if (!parseIndex(v, index, group))
        return false;

    v = (*args)[++i];
    if (v->isNumber()) {
        *count = v->toInt32();

        if (++i == args->length())
            return false;
        v = (*args)[i];
    }

    *groups = QQmlDelegateModelPrivate::get(model)->m_cacheMetaType->parseGroups(v);

    return true;
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::addGroups(int index, int count, stringlist groups)

    Adds \a count items starting at \a index to \a groups.
*/

void QQmlDelegateModelGroup::addGroups(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlWarning(this) << tr("addGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlWarning(this) << tr("addGroups: invalid count");
        } else {
            model->addGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::removeGroups(int index, int count, stringlist groups)

    Removes \a count items starting at \a index from \a groups.
*/

void QQmlDelegateModelGroup::removeGroups(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlWarning(this) << tr("removeGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlWarning(this) << tr("removeGroups: invalid count");
        } else {
            model->removeGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::setGroups(int index, int count, stringlist groups)

    Changes the group membership of \a count items starting at \a index. The items are removed from
    their existing groups and added to \a groups.
*/

void QQmlDelegateModelGroup::setGroups(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);
    Compositor::Group group = d->group;
    int index = -1;
    int count = 1;
    int groups = 0;

    if (!d->parseGroupArgs(args, &group, &index, &count, &groups))
        return;

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);
    if (index < 0 || index >= model->m_compositor.count(group)) {
        qmlWarning(this) << tr("setGroups: index out of range");
    } else if (count != 0) {
        Compositor::iterator it = model->m_compositor.find(group, index);
        if (count < 0 || count > model->m_compositor.count(d->group) - it.index[d->group]) {
            qmlWarning(this) << tr("setGroups: invalid count");
        } else {
            model->setGroups(it, count, d->group, groups);
        }
    }
}

/*!
    \qmlmethod QtQml.Models::DelegateModelGroup::move(var from, var to, int count)

    Moves \a count at \a from in a group \a to a new position.

    \note The DelegateModel acts as a proxy model: it holds the delegates in a
    different order than the \l{dm-model-property}{underlying model} has them.
    Any subsequent changes to the underlying model will not undo whatever
    reordering you have done via this function.
*/

void QQmlDelegateModelGroup::move(QQmlV4FunctionPtr args)
{
    Q_D(QQmlDelegateModelGroup);

    if (args->length() < 2)
        return;

    Compositor::Group fromGroup = d->group;
    Compositor::Group toGroup = d->group;
    int from = -1;
    int to = -1;
    int count = 1;

    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue v(scope, (*args)[0]);
    if (!d->parseIndex(v, &from, &fromGroup)) {
        qmlWarning(this) << tr("move: invalid from index");
        return;
    }

    v = (*args)[1];
    if (!d->parseIndex(v, &to, &toGroup)) {
        qmlWarning(this) << tr("move: invalid to index");
        return;
    }

    if (args->length() > 2) {
        v = (*args)[2];
        if (v->isNumber())
            count = v->toInt32();
    }

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(d->model);

    if (count < 0) {
        qmlWarning(this) << tr("move: invalid count");
    } else if (from < 0 || from + count > model->m_compositor.count(fromGroup)) {
        qmlWarning(this) << tr("move: from index out of range");
    } else if (!model->m_compositor.verifyMoveTo(fromGroup, from, toGroup, to, count, d->group)) {
        qmlWarning(this) << tr("move: to index out of range");
    } else if (count > 0) {
        QVector<Compositor::Remove> removes;
        QVector<Compositor::Insert> inserts;

        model->m_compositor.move(fromGroup, from, toGroup, to, count, d->group, &removes, &inserts);
        model->itemsMoved(removes, inserts);
        model->emitChanges();
    }

}

/*!
    \qmlsignal QtQml.Models::DelegateModelGroup::changed(array removed, array inserted)

    This signal is emitted when items have been removed from or inserted into the group.

    Each object in the \a removed and \a inserted arrays has two values; the \e index of the first
    item inserted or removed and a \e count of the number of consecutive items inserted or removed.

    Each index is adjusted for previous changes with all removed items preceding any inserted
    items.
*/

//============================================================================

QQmlPartsModel::QQmlPartsModel(QQmlDelegateModel *model, const QString &part, QObject *parent)
    : QQmlInstanceModel(*new QObjectPrivate, parent)
    , m_model(model)
    , m_part(part)
    , m_compositorGroup(Compositor::Cache)
    , m_inheritGroup(true)
{
    QQmlDelegateModelPrivate *d = QQmlDelegateModelPrivate::get(m_model);
    if (d->m_cacheMetaType) {
        QQmlDelegateModelGroupPrivate::get(d->m_groups[1])->emitters.insert(this);
        m_compositorGroup = Compositor::Default;
    } else {
        d->m_pendingParts.insert(this);
    }
}

QQmlPartsModel::~QQmlPartsModel()
{
}

QString QQmlPartsModel::filterGroup() const
{
    if (m_inheritGroup)
        return m_model->filterGroup();
    return m_filterGroup;
}

void QQmlPartsModel::setFilterGroup(const QString &group)
{
    if (QQmlDelegateModelPrivate::get(m_model)->m_transaction) {
        qmlWarning(this) << tr("The group of a DelegateModel cannot be changed within onChanged");
        return;
    }

    if (m_filterGroup != group || m_inheritGroup) {
        m_filterGroup = group;
        m_inheritGroup = false;
        updateFilterGroup();

        emit filterGroupChanged();
    }
}

void QQmlPartsModel::resetFilterGroup()
{
    if (!m_inheritGroup) {
        m_inheritGroup = true;
        updateFilterGroup();
        emit filterGroupChanged();
    }
}

void QQmlPartsModel::updateFilterGroup()
{
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
    if (!model->m_cacheMetaType)
        return;

    if (m_inheritGroup) {
        if (m_filterGroup == model->m_filterGroup)
            return;
        m_filterGroup = model->m_filterGroup;
    }

    QQmlListCompositor::Group previousGroup = m_compositorGroup;
    m_compositorGroup = Compositor::Default;
    QQmlDelegateModelGroupPrivate::get(model->m_groups[Compositor::Default])->emitters.insert(this);
    for (int i = 1; i < model->m_groupCount; ++i) {
        if (m_filterGroup == model->m_cacheMetaType->groupNames.at(i - 1)) {
            m_compositorGroup = Compositor::Group(i);
            break;
        }
    }

    QQmlDelegateModelGroupPrivate::get(model->m_groups[m_compositorGroup])->emitters.insert(this);
    if (m_compositorGroup != previousGroup) {
        QVector<QQmlChangeSet::Change> removes;
        QVector<QQmlChangeSet::Change> inserts;
        model->m_compositor.transition(previousGroup, m_compositorGroup, &removes, &inserts);

        QQmlChangeSet changeSet;
        changeSet.move(removes, inserts);
        if (!changeSet.isEmpty())
            emit modelUpdated(changeSet, false);

        if (changeSet.difference() != 0)
            emit countChanged();
    }
}

void QQmlPartsModel::updateFilterGroup(
        Compositor::Group group, const QQmlChangeSet &changeSet)
{
    if (!m_inheritGroup)
        return;

    m_compositorGroup = group;
    QQmlDelegateModelGroupPrivate::get(QQmlDelegateModelPrivate::get(m_model)->m_groups[m_compositorGroup])->emitters.insert(this);

    if (!changeSet.isEmpty())
        emit modelUpdated(changeSet, false);

    if (changeSet.difference() != 0)
        emit countChanged();

    emit filterGroupChanged();
}

int QQmlPartsModel::count() const
{
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
    return model->m_delegate
            ? model->m_compositor.count(m_compositorGroup)
            : 0;
}

bool QQmlPartsModel::isValid() const
{
    return m_model->isValid();
}

QObject *QQmlPartsModel::object(int index, QQmlIncubator::IncubationMode incubationMode)
{
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);

    if (!model->m_delegate || index < 0 || index >= model->m_compositor.count(m_compositorGroup)) {
        qWarning() << "DelegateModel::item: index out range" << index << model->m_compositor.count(m_compositorGroup);
        return nullptr;
    }

    QObject *object = model->object(m_compositorGroup, index, incubationMode);

    if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(object)) {
        QObject *part = package->part(m_part);
        if (!part)
            return nullptr;
        m_packaged.insert(part, package);
        return part;
    }

    model->release(object);
    if (!model->m_delegateValidated) {
        if (object)
            qmlWarning(model->m_delegate) << tr("Delegate component must be Package type.");
        model->m_delegateValidated = true;
    }

    return nullptr;
}

QQmlInstanceModel::ReleaseFlags QQmlPartsModel::release(QObject *item, ReusableFlag)
{
    QQmlInstanceModel::ReleaseFlags flags;

    auto it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        QQuickPackage *package = *it;
        QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
        flags = model->release(package);
        m_packaged.erase(it);
        if (!m_packaged.contains(item))
            flags &= ~Referenced;
        if (flags & Destroyed)
            QQmlDelegateModelPrivate::get(m_model)->emitDestroyingPackage(package);
    }
    return flags;
}

QVariant QQmlPartsModel::variantValue(int index, const QString &role)
{
    return QQmlDelegateModelPrivate::get(m_model)->variantValue(m_compositorGroup, index, role);
}

void QQmlPartsModel::setWatchedRoles(const QList<QByteArray> &roles)
{
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
    model->m_adaptorModel.replaceWatchedRoles(m_watchedRoles, roles);
    m_watchedRoles = roles;
}

QQmlIncubator::Status QQmlPartsModel::incubationStatus(int index)
{
    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
    Compositor::iterator it = model->m_compositor.find(model->m_compositorGroup, index);
    if (!it->inCache())
        return QQmlIncubator::Null;

    if (auto incubationTask = model->m_cache.at(it.cacheIndex())->incubationTask)
        return incubationTask->status();

    return QQmlIncubator::Ready;
}

int QQmlPartsModel::indexOf(QObject *item, QObject *) const
{
    auto it = m_packaged.find(item);
    if (it != m_packaged.end()) {
        if (QQmlDelegateModelItem *cacheItem = QQmlDelegateModelItem::dataForObject(*it))
            return cacheItem->groupIndex(m_compositorGroup);
    }
    return -1;
}

void QQmlPartsModel::createdPackage(int index, QQuickPackage *package)
{
    emit createdItem(index, package->part(m_part));
}

void QQmlPartsModel::initPackage(int index, QQuickPackage *package)
{
    if (m_modelUpdatePending)
        m_pendingPackageInitializations << index;
    else
        emit initItem(index, package->part(m_part));
}

void QQmlPartsModel::destroyingPackage(QQuickPackage *package)
{
    QObject *item = package->part(m_part);
    Q_ASSERT(!m_packaged.contains(item));
    emit destroyingItem(item);
}

void QQmlPartsModel::emitModelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    m_modelUpdatePending = false;
    emit modelUpdated(changeSet, reset);
    if (changeSet.difference() != 0)
        emit countChanged();

    QQmlDelegateModelPrivate *model = QQmlDelegateModelPrivate::get(m_model);
    QVector<int> pendingPackageInitializations;
    qSwap(pendingPackageInitializations, m_pendingPackageInitializations);
    for (int index : pendingPackageInitializations) {
        if (!model->m_delegate || index < 0 || index >= model->m_compositor.count(m_compositorGroup))
            continue;
        QObject *object = model->object(m_compositorGroup, index, QQmlIncubator::Asynchronous);
        if (QQuickPackage *package = qmlobject_cast<QQuickPackage *>(object))
            emit initItem(index, package->part(m_part));
        model->release(object);
    }
}

bool QQmlReusableDelegateModelItemsPool::insertItem(QQmlDelegateModelItem *modelItem)
{
    // We can only hold INT_MAX items, due to the return type of size().
    if (Q_UNLIKELY(m_reusableItemsPool.size() == QQmlReusableDelegateModelItemsPool::MaxSize))
        return false;

    Q_ASSERT(m_reusableItemsPool.size() < QQmlReusableDelegateModelItemsPool::MaxSize);

    // Currently, the only way for a view to reuse items is to call release()
    // in the model class with the second argument explicitly set to
    // QQmlReuseableDelegateModelItemsPool::Reusable. If the released item is
    // no longer referenced, it will be added to the pool. Reusing of items can
    // be specified per item, in case certain items cannot be recycled. A
    // QQmlDelegateModelItem knows which delegate its object was created from.
    // So when we are about to create a new item, we first check if the pool
    // contains an item based on the same delegate from before. If so, we take
    // it out of the pool (instead of creating a new item), and update all its
    // context properties and attached properties.

    // When a view is recycling items, it should call drain() regularly. As
    // there is currently no logic to 'hibernate' items in the pool, they are
    // only meant to rest there for a short while, ideally only from the time
    // e.g a row is unloaded on one side of the view, and until a new row is
    // loaded on the opposite side. Between these times, the application will
    // see the item as fully functional and 'alive' (just not visible on
    // screen). Since this time is supposed to be short, we don't take any
    // action to notify the application about it, since we don't want to
    // trigger any bindings that can disturb performance.

    // A recommended time for calling drain() is each time a view has finished
    // loading e.g a new row or column. If there are more items in the pool
    // after that, it means that the view most likely doesn't need them anytime
    // soon. Those items should be destroyed to reduce resource consumption.

    // Depending on if a view is a list or a table, it can sometimes be
    // performant to keep items in the pool for a bit longer than one "row
    // out/row in" cycle. E.g for a table, if the number of visible rows in a
    // view is much larger than the number of visible columns. In that case, if
    // you flick out a row, and then flick in a column, you would throw away a
    // lot of items in the pool if completely draining it. The reason is that
    // unloading a row places more items in the pool than what ends up being
    // recycled when loading a new column. And then, when you next flick in a
    // new row, you would need to load all those drained items again from
    // scratch. For that reason, you can specify a maxPoolTime to the
    // drainReusableItemsPool() that allows you to keep items in the pool for a
    // bit longer, effectively keeping more items in circulation. A recommended
    // maxPoolTime would be equal to the number of dimensions in the view,
    // which means 1 for a list view and 2 for a table view. If you specify 0,
    // all items will be drained.

    Q_ASSERT(!modelItem->incubationTask);
    Q_ASSERT(!modelItem->isObjectReferenced());
    Q_ASSERT(modelItem->object);
    Q_ASSERT(modelItem->delegate);

    m_reusableItemsPool.push_back({modelItem, 0});

    qCDebug(lcItemViewDelegateRecycling)
            << "item:" << modelItem
            << "delegate:" << modelItem->delegate
            << "index:" << modelItem->modelIndex()
            << "row:" << modelItem->modelRow()
            << "column:" << modelItem->modelColumn()
            << "pool size:" << m_reusableItemsPool.size();

    return true;
}

QQmlDelegateModelItem *QQmlReusableDelegateModelItemsPool::takeItem(const QQmlComponent *delegate, int newIndexHint)
{
    // Find the oldest item in the pool that was made from the same delegate as
    // the given argument, remove it from the pool, and return it.
    for (auto it = m_reusableItemsPool.cbegin(); it != m_reusableItemsPool.cend(); ++it) {
        QQmlDelegateModelItem *modelItem = it->item;
        if (modelItem->delegate != delegate)
            continue;
        m_reusableItemsPool.erase(it);

        qCDebug(lcItemViewDelegateRecycling)
                << "item:" << modelItem
                << "delegate:" << delegate
                << "old index:" << modelItem->modelIndex()
                << "old row:" << modelItem->modelRow()
                << "old column:" << modelItem->modelColumn()
                << "new index:" << newIndexHint
                << "pool size:" << m_reusableItemsPool.size();

        return modelItem;
    }

    qCDebug(lcItemViewDelegateRecycling)
            << "no available item for delegate:" << delegate
            << "new index:" << newIndexHint
            << "pool size:" << m_reusableItemsPool.size();

    return nullptr;
}

void QQmlReusableDelegateModelItemsPool::drain(int maxPoolTime, std::function<void(QQmlDelegateModelItem *cacheItem)> releaseItem)
{
    // Rather than releasing all pooled items upon a call to this function, each
    // item has a poolTime. The poolTime specifies for how many loading cycles an item
    // has been resting in the pool. And for each invocation of this function, poolTime
    // will increase. If poolTime is equal to, or exceeds, maxPoolTime, it will be removed
    // from the pool and released. This way, the view can tweak a bit for how long
    // items should stay in "circulation", even if they are not recycled right away.
    qCDebug(lcItemViewDelegateRecycling) << "pool size before drain:" << m_reusableItemsPool.size();

    for (auto it = m_reusableItemsPool.begin(); it != m_reusableItemsPool.end();) {
        if (++(it->poolTime) <= maxPoolTime) {
            ++it;
        } else {
            releaseItem(it->item);
            it = m_reusableItemsPool.erase(it);
        }
    }

    qCDebug(lcItemViewDelegateRecycling) << "pool size after drain:" << m_reusableItemsPool.size();
}

//============================================================================

struct QQmlDelegateModelGroupChange : QV4::Object
{
    V4_OBJECT2(QQmlDelegateModelGroupChange, QV4::Object)

    static QV4::Heap::QQmlDelegateModelGroupChange *create(QV4::ExecutionEngine *e) {
        return e->memoryManager->allocate<QQmlDelegateModelGroupChange>();
    }

    static QV4::ReturnedValue method_get_index(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int) {
        QV4::Scope scope(b);
        QV4::Scoped<QQmlDelegateModelGroupChange> that(scope, thisObject->as<QQmlDelegateModelGroupChange>());
        if (!that)
            THROW_TYPE_ERROR();
        return QV4::Encode(that->d()->change.index);
    }
    static QV4::ReturnedValue method_get_count(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int) {
        QV4::Scope scope(b);
        QV4::Scoped<QQmlDelegateModelGroupChange> that(scope, thisObject->as<QQmlDelegateModelGroupChange>());
        if (!that)
            THROW_TYPE_ERROR();
        return QV4::Encode(that->d()->change.count);
    }
    static QV4::ReturnedValue method_get_moveId(const QV4::FunctionObject *b, const QV4::Value *thisObject, const QV4::Value *, int) {
        QV4::Scope scope(b);
        QV4::Scoped<QQmlDelegateModelGroupChange> that(scope, thisObject->as<QQmlDelegateModelGroupChange>());
        if (!that)
            THROW_TYPE_ERROR();
        if (that->d()->change.moveId < 0)
            RETURN_UNDEFINED();
        return QV4::Encode(that->d()->change.moveId);
    }
};

DEFINE_OBJECT_VTABLE(QQmlDelegateModelGroupChange);

struct QQmlDelegateModelGroupChangeArray : public QV4::Object
{
    V4_OBJECT2(QQmlDelegateModelGroupChangeArray, QV4::Object)
    V4_NEEDS_DESTROY
public:
    static QV4::Heap::QQmlDelegateModelGroupChangeArray *create(QV4::ExecutionEngine *engine, const QVector<QQmlChangeSet::Change> &changes)
    {
        return engine->memoryManager->allocate<QQmlDelegateModelGroupChangeArray>(changes);
    }

    quint32 count() const { return d()->changes->size(); }
    const QQmlChangeSet::Change &at(int index) const { return d()->changes->at(index); }

    static QV4::ReturnedValue virtualGet(const QV4::Managed *m, QV4::PropertyKey id, const QV4::Value *receiver, bool *hasProperty)
    {
        if (id.isArrayIndex()) {
            uint index = id.asArrayIndex();
            Q_ASSERT(m->as<QQmlDelegateModelGroupChangeArray>());
            QV4::ExecutionEngine *v4 = static_cast<const QQmlDelegateModelGroupChangeArray *>(m)->engine();
            QV4::Scope scope(v4);
            QV4::Scoped<QQmlDelegateModelGroupChangeArray> array(scope, static_cast<const QQmlDelegateModelGroupChangeArray *>(m));

            if (index >= array->count()) {
                if (hasProperty)
                    *hasProperty = false;
                return QV4::Value::undefinedValue().asReturnedValue();
            }

            const QQmlChangeSet::Change &change = array->at(index);

            QV4::ScopedObject changeProto(scope, qdmEngineData(v4)->changeProto.value());
            QV4::Scoped<QQmlDelegateModelGroupChange> object(scope, QQmlDelegateModelGroupChange::create(v4));
            object->setPrototypeOf(changeProto);
            object->d()->change = change;

            if (hasProperty)
                *hasProperty = true;
            return object.asReturnedValue();
        }

        Q_ASSERT(m->as<QQmlDelegateModelGroupChangeArray>());
        const QQmlDelegateModelGroupChangeArray *array = static_cast<const QQmlDelegateModelGroupChangeArray *>(m);

        if (id == array->engine()->id_length()->propertyKey()) {
            if (hasProperty)
                *hasProperty = true;
            return QV4::Encode(array->count());
        }

        return Object::virtualGet(m, id, receiver, hasProperty);
    }
};

void QV4::Heap::QQmlDelegateModelGroupChangeArray::init(const QVector<QQmlChangeSet::Change> &changes)
{
    Object::init();
    this->changes = new QVector<QQmlChangeSet::Change>(changes);
    QV4::Scope scope(internalClass->engine);
    QV4::ScopedObject o(scope, this);
    o->setArrayType(QV4::Heap::ArrayData::Custom);
}

DEFINE_OBJECT_VTABLE(QQmlDelegateModelGroupChangeArray);

QQmlDelegateModelEngineData::QQmlDelegateModelEngineData(QV4::ExecutionEngine *v4)
{
    QV4::Scope scope(v4);

    QV4::ScopedObject proto(scope, v4->newObject());
    proto->defineAccessorProperty(QStringLiteral("index"), QQmlDelegateModelGroupChange::method_get_index, nullptr);
    proto->defineAccessorProperty(QStringLiteral("count"), QQmlDelegateModelGroupChange::method_get_count, nullptr);
    proto->defineAccessorProperty(QStringLiteral("moveId"), QQmlDelegateModelGroupChange::method_get_moveId, nullptr);
    changeProto.set(v4, proto);
}

QQmlDelegateModelEngineData::~QQmlDelegateModelEngineData()
{
}

QV4::ReturnedValue QQmlDelegateModelEngineData::array(QV4::ExecutionEngine *v4,
                                                      const QVector<QQmlChangeSet::Change> &changes)
{
    QV4::Scope scope(v4);
    QV4::ScopedObject o(scope, QQmlDelegateModelGroupChangeArray::create(v4, changes));
    return o.asReturnedValue();
}

QT_END_NAMESPACE

#include "moc_qqmldelegatemodel_p_p.cpp"

#include "moc_qqmldelegatemodel_p.cpp"
