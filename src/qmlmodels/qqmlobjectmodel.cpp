// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlobjectmodel_p.h"

#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlinfo.h>

#include <private/qqmlchangeset_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qobject_p.h>
#include <private/qv4qobjectwrapper_p.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qhash.h>
#include <QtCore/qlist.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

class QQmlObjectModelPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QQmlObjectModel)
public:
    class Item {
    public:
        Item(QObject *i = nullptr) : m_item(i) {}

        void addRef() { ++m_ref; }
        bool deref() { return --m_ref == 0; }
        int refCount() const { return m_ref; }

        QObject *item() const { return m_item.data(); }

    private:
        QPointer<QObject> m_item;
        int m_ref = 0;
    };

    QQmlObjectModelPrivate() : QObjectPrivate(), moveId(0) {}

    static QQmlObjectModelPrivate *get(QQmlObjectModel *q) { return q->d_func(); }

    static void children_append(QQmlListProperty<QObject> *prop, QObject *item) {
        qsizetype index = static_cast<QQmlObjectModelPrivate *>(prop->data)->children.size();
        static_cast<QQmlObjectModelPrivate *>(prop->data)->insert(index, item);
    }

    static qsizetype children_count(QQmlListProperty<QObject> *prop) {
        return static_cast<QQmlObjectModelPrivate *>(prop->data)->children.size();
    }

    static QObject *children_at(QQmlListProperty<QObject> *prop, qsizetype index) {
        return static_cast<QQmlObjectModelPrivate *>(prop->data)->children.at(index).item();
    }

    static void children_clear(QQmlListProperty<QObject> *prop) {
        static_cast<QQmlObjectModelPrivate *>(prop->data)->clear();
    }

    static void children_replace(QQmlListProperty<QObject> *prop, qsizetype index, QObject *item) {
        static_cast<QQmlObjectModelPrivate *>(prop->data)->replace(index, item);
    }

    static void children_removeLast(QQmlListProperty<QObject> *prop) {
        auto data = static_cast<QQmlObjectModelPrivate *>(prop->data);
        data->remove(data->children.size() - 1, 1);
    }

    void markNewChild(QQmlObjectModel *q, QObject *item)
    {
        if (QJSEngine *engine = qjsEngine(q)) {
            QV4::WriteBarrier::markCustom(engine->handle(), [&](QV4::MarkStack *markStack) {
                QV4::QObjectWrapper::markWrapper(item, markStack);
            });
        }
    }

    void insert(int index, QObject *item) {
        Q_Q(QQmlObjectModel);
        children.insert(index, Item(item));
        markNewChild(q, item);
        for (int i = index, end = children.size(); i < end; ++i)
            setIndex(i);
        QQmlChangeSet changeSet;
        changeSet.insert(index, 1);
        emit q->modelUpdated(changeSet, false);
        emit q->countChanged();
        emit q->childrenChanged();
    }

    void replace(int index, QObject *item) {
        Q_Q(QQmlObjectModel);
        clearIndex(index);
        children.replace(index, Item(item));
        markNewChild(q, item);
        setIndex(index);
        QQmlChangeSet changeSet;
        changeSet.change(index, 1);
        emit q->modelUpdated(changeSet, false);
        emit q->childrenChanged();
    }

    void move(int from, int to, int n) {
        Q_Q(QQmlObjectModel);
        if (from > to) {
            // Only move forwards - flip if backwards moving
            int tfrom = from;
            int tto = to;
            from = tto;
            to = tto+n;
            n = tfrom-tto;
        }

        QVarLengthArray<QQmlObjectModelPrivate::Item, 4> store;
        for (int i = 0; i < to - from; ++i)
            store.append(std::move(children[from + n + i]));
        for (int i = 0; i < n; ++i)
            store.append(std::move(children[from + i]));

        for (int i = 0, end = store.count(); i < end; ++i) {
            children[from + i] = std::move(store[i]);
            setIndex(from + i);
        }

        QQmlChangeSet changeSet;
        changeSet.move(from, to, n, ++moveId);
        emit q->modelUpdated(changeSet, false);
        emit q->childrenChanged();
    }

    void remove(int index, int n) {
        Q_Q(QQmlObjectModel);
        for (int i = index; i < index + n; ++i)
            clearIndex(i);
        children.erase(children.begin() + index, children.begin() + index + n);
        for (int i = index, end = children.size(); i < end; ++i)
            setIndex(i);
        QQmlChangeSet changeSet;
        changeSet.remove(index, n);
        emit q->modelUpdated(changeSet, false);
        emit q->countChanged();
        emit q->childrenChanged();
    }

    void clear() {
        Q_Q(QQmlObjectModel);
        const auto copy = children;
        for (const Item &child : copy)
            emit q->destroyingItem(child.item());
        remove(0, children.size());
    }

    int indexOf(QObject *item) const {
        for (int i = 0; i < children.size(); ++i)
            if (children.at(i).item() == item)
                return i;
        return -1;
    }

    void markChildren(QV4::MarkStack *markStack) const
    {
        for (const Item &child : children)
            QV4::QObjectWrapper::markWrapper(child.item(), markStack);
    }

    quint64 _q_createJSWrapper(QQmlV4ExecutionEnginePtr engine);

private:
    void setIndex(int child, int index)
    {
        if (auto *attached = static_cast<QQmlObjectModelAttached *>(
                    qmlAttachedPropertiesObject<QQmlObjectModel>(children.at(child).item()))) {
            attached->setIndex(index);
        }
    }

    void setIndex(int child) { setIndex(child, child); }
    void clearIndex(int child) { setIndex(child, -1); }


    uint moveId;
    QList<Item> children;
};

namespace QV4 {
namespace Heap {
struct QQmlObjectModelWrapper : public QObjectWrapper {
    static void markObjects(Base *that, MarkStack *markStack)
    {
        Q_ASSERT(QV4::Value::fromHeapObject(that).as<QV4::QObjectWrapper>());
        QObject *object = static_cast<QObjectWrapper *>(that)->object();
        if (!object)
            return;

        Q_ASSERT(qobject_cast<QQmlObjectModel *>(object));
        QQmlObjectModelPrivate::get(static_cast<QQmlObjectModel *>(object))
                ->markChildren(markStack);

        QObjectWrapper::markObjects(that, markStack);
    }
};
} // namespace Heap

struct QQmlObjectModelWrapper : public QObjectWrapper {
    V4_OBJECT2(QQmlObjectModelWrapper, QObjectWrapper)
};

} // namspace QV4

DEFINE_OBJECT_VTABLE(QV4::QQmlObjectModelWrapper);

quint64 QQmlObjectModelPrivate::_q_createJSWrapper(QQmlV4ExecutionEnginePtr engine)
{
    return (engine->memoryManager->allocate<QV4::QQmlObjectModelWrapper>(
                    q_func()))->asReturnedValue();
}



/*!
    \qmltype ObjectModel
    \nativetype QQmlObjectModel
    \inqmlmodule QtQml.Models
    \ingroup qtquick-models
    \brief Defines a set of items to be used as a model.

    An ObjectModel contains the visual items to be used in a view.
    When an ObjectModel is used in a view, the view does not require
    a delegate since the ObjectModel already contains the visual
    delegate (items).

    An item can determine its index within the
    model via the \l{ObjectModel::index}{index} attached property.

    The example below places three colored rectangles in a ListView.
    \code
    import QtQuick 2.0
    import QtQml.Models 2.1

    Rectangle {
        ObjectModel {
            id: itemModel
            Rectangle { height: 30; width: 80; color: "red" }
            Rectangle { height: 30; width: 80; color: "green" }
            Rectangle { height: 30; width: 80; color: "blue" }
        }

        ListView {
            anchors.fill: parent
            model: itemModel
        }
    }
    \endcode

    \image objectmodel.png

    \sa {Qt Quick Examples - Views}
*/

QQmlObjectModel::QQmlObjectModel(QObject *parent)
    : QQmlInstanceModel(*(new QQmlObjectModelPrivate), parent)
{
}

QQmlObjectModel::~QQmlObjectModel() = default;

/*!
    \qmlattachedproperty int QtQml.Models::ObjectModel::index
    This attached property holds the index of this delegate's item within the model.

    It is attached to each instance of the delegate.
*/

QQmlListProperty<QObject> QQmlObjectModel::children()
{
    Q_D(QQmlObjectModel);
    return QQmlListProperty<QObject>(this, d,
                                     QQmlObjectModelPrivate::children_append,
                                     QQmlObjectModelPrivate::children_count,
                                     QQmlObjectModelPrivate::children_at,
                                     QQmlObjectModelPrivate::children_clear,
                                     QQmlObjectModelPrivate::children_replace,
                                     QQmlObjectModelPrivate::children_removeLast);
}

/*!
    \qmlproperty int QtQml.Models::ObjectModel::count

    The number of items in the model.  This property is readonly.
*/
int QQmlObjectModel::count() const
{
    Q_D(const QQmlObjectModel);
    return d->children.size();
}

bool QQmlObjectModel::isValid() const
{
    return true;
}

QObject *QQmlObjectModel::object(int index, QQmlIncubator::IncubationMode)
{
    Q_D(QQmlObjectModel);
    QQmlObjectModelPrivate::Item &item = d->children[index];
    item.addRef();
    QObject *obj = item.item();
    if (item.refCount() == 1) {
        emit initItem(index, obj);
        emit createdItem(index, obj);
    }
    return obj;
}

QQmlInstanceModel::ReleaseFlags QQmlObjectModel::release(QObject *item, ReusableFlag)
{
    Q_D(QQmlObjectModel);
    int idx = d->indexOf(item);
    if (idx >= 0) {
        if (!d->children[idx].deref())
            return QQmlInstanceModel::Referenced;
    }
    return {};
}

QVariant QQmlObjectModel::variantValue(int index, const QString &role)
{
    Q_D(QQmlObjectModel);
    if (index < 0 || index >= d->children.size())
        return QString();
    if (QObject *item = d->children.at(index).item())
        return item->property(role.toUtf8().constData());
    return QString();
}

QQmlIncubator::Status QQmlObjectModel::incubationStatus(int)
{
    return QQmlIncubator::Ready;
}

int QQmlObjectModel::indexOf(QObject *item, QObject *) const
{
    Q_D(const QQmlObjectModel);
    return d->indexOf(item);
}

QQmlObjectModelAttached *QQmlObjectModel::qmlAttachedProperties(QObject *obj)
{
    return new QQmlObjectModelAttached(obj);
}

/*!
    \qmlmethod object QtQml.Models::ObjectModel::get(int index)
    \since 5.6

    Returns the item at \a index in the model. This allows the item
    to be accessed or modified from JavaScript:

    \code
    Component.onCompleted: {
        objectModel.append(objectComponent.createObject())
        console.log(objectModel.get(0).objectName);
        objectModel.get(0).objectName = "first";
    }
    \endcode

    The \a index must be an element in the list.

    \sa append()
*/
QObject *QQmlObjectModel::get(int index) const
{
    Q_D(const QQmlObjectModel);
    if (index < 0 || index >= d->children.size())
        return nullptr;
    return d->children.at(index).item();
}

/*!
    \qmlmethod QtQml.Models::ObjectModel::append(object item)
    \since 5.6

    Appends a new \a item to the end of the model.

    \code
        objectModel.append(objectComponent.createObject())
    \endcode

    \sa insert(), remove()
*/
void QQmlObjectModel::append(QObject *object)
{
    Q_D(QQmlObjectModel);
    d->insert(count(), object);
}

/*!
    \qmlmethod QtQml.Models::ObjectModel::insert(int index, object item)
    \since 5.6

    Inserts a new \a item to the model at position \a index.

    \code
        objectModel.insert(2, objectComponent.createObject())
    \endcode

    The \a index must be to an existing item in the list, or one past
    the end of the list (equivalent to append).

    \sa append(), remove()
*/
void QQmlObjectModel::insert(int index, QObject *object)
{
    Q_D(QQmlObjectModel);
    if (index < 0 || index > count()) {
        qmlWarning(this) << tr("insert: index %1 out of range").arg(index);
        return;
    }
    d->insert(index, object);
}

/*!
    \qmlmethod QtQml.Models::ObjectModel::move(int from, int to, int n = 1)
    \since 5.6

    Moves \a n items \a from one position \a to another.

    The from and to ranges must exist; for example, to move the first 3 items
    to the end of the model:

    \code
        objectModel.move(0, objectModel.count - 3, 3)
    \endcode

    \sa append()
*/
void QQmlObjectModel::move(int from, int to, int n)
{
    Q_D(QQmlObjectModel);
    if (n <= 0 || from == to)
        return;
    if (from < 0 || to < 0 || from + n > count() || to + n > count()) {
        qmlWarning(this) << tr("move: out of range");
        return;
    }
    d->move(from, to, n);
}

/*!
    \qmlmethod QtQml.Models::ObjectModel::remove(int index, int n = 1)
    \since 5.6

    Removes \a n items at \a index from the model.

    \sa clear()
*/
void QQmlObjectModel::remove(int index, int n)
{
    Q_D(QQmlObjectModel);
    if (index < 0 || n <= 0 || index + n > count()) {
        qmlWarning(this) << tr("remove: indices [%1 - %2] out of range [0 - %3]").arg(index).arg(index+n).arg(count());
        return;
    }
    d->remove(index, n);
}

/*!
    \qmlmethod QtQml.Models::ObjectModel::clear()
    \since 5.6

    Clears all items from the model.

    \sa append(), remove()
*/
void QQmlObjectModel::clear()
{
    Q_D(QQmlObjectModel);
    d->clear();
}

bool QQmlInstanceModel::setRequiredProperty(int index, const QString &name, const QVariant &value)
{
    Q_UNUSED(index);
    Q_UNUSED(name);
    Q_UNUSED(value);
    // The view should not call this function, unless
    // it's actually handled in a subclass.
    Q_UNREACHABLE_RETURN(false);
}

QT_END_NAMESPACE

#include "moc_qqmlobjectmodel_p.cpp"
