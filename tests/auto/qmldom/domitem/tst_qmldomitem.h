// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLDOMITEM_H
#define TST_QMLDOMITEM_H
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomastdumper_p.h>
#include <QtQmlDom/private/qqmldommock_p.h>
#include <QtQmlDom/private/qqmldomcompare_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>

#include <QtTest/QTest>
#include <QtCore/QCborValue>
#include <QtCore/QDebug>
#include <QtCore/QLibraryInfo>
#include <QtCore/QFileInfo>

#include <deque>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace Dom {

inline DomItem wrapInt(const DomItem &self, const PathEls::PathComponent &p, const int &i)
{
    return self.subDataItem(p, i);
}

class DomTestClass
{
public:
    std::shared_ptr<int> i;
};

class TestDomItem : public QObject
{
    Q_OBJECT
public:
    static ErrorGroups myErrors()
    {
        static ErrorGroups res { { NewErrorGroup("tests"), NewErrorGroup("domitem") } };
        return res;
    }

private slots:
    void initTestCase()
    {
        baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/domitem");
        qmltypeDirs = QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::QmlImportsPath) });
        universePtr =
                std::shared_ptr<DomUniverse>(new DomUniverse(QStringLiteral(u"dummyUniverse")));
        envPtr = std::shared_ptr<DomEnvironment>(
                new DomEnvironment(QStringList(), DomEnvironment::Option::SingleThreaded,
                                   DomCreationOption::Default, universePtr));
        env = DomItem(envPtr);
        testOwnerPtr = std::shared_ptr<MockOwner>(new MockOwner(
                Path::fromRoot(u"env").withField(u"testOwner"), 0,
                QMap<QString, MockObject> {
                        MockObject(
                                Path::fromField(u"obj1"),
                                QMap<QString, MockObject> {
                                        MockObject(
                                                Path::fromField(u"obj1").withField(u"obj1_2"),
                                                QMap<QString, MockObject> {},
                                                QMap<QString, QCborValue> {
                                                        { QStringLiteral(u"val1"), QCborValue(3) },
                                                        { QLatin1String("val2"), QCborValue(4) } })
                                                .asStringPair() },
                                QMap<QString, QCborValue> {
                                        { QStringLiteral(u"val1"), QCborValue(1) },
                                        { QLatin1String("val2"), QCborValue(2) } })
                                .asStringPair(),
                        MockObject(
                                Path::fromField(u"obj2"),
                                QMap<QString, MockObject> {
                                        MockObject(
                                                Path::fromField(u"obj2").withField(u"obj2_2"),
                                                QMap<QString, MockObject> {},
                                                QMap<QString, QCborValue> {
                                                        { QStringLiteral(u"val1"), QCborValue(5) },
                                                        { QLatin1String("val2"), QCborValue(6) },
                                                        { QLatin1String("valX"),
                                                          QCborValue(QStringLiteral(u"pippo")) } })
                                                .asStringPair() },
                                QMap<QString, QCborValue> {
                                        { QStringLiteral(u"val1"), QCborValue(7) },
                                        { QLatin1String("val2"), QCborValue(8) } })
                                .asStringPair() },
                QMap<QString, QCborValue> {
                        { QStringLiteral(u"val1"), QCborValue(9) },
                },
                QMap<QString, QMap<QString, MockObject>> {
                        { QStringLiteral(u"map"),
                          QMap<QString, MockObject> {
                                  MockObject(Path::fromField(u"map").withKey(u"a"),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(10) },
                                                     { QLatin1String("val2"), QCborValue(11) } })
                                          .asStringPair(),
                                  MockObject(Path::fromField(u"map").withKey(u"b"),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(12) },
                                                     { QLatin1String("val2"), QCborValue(13) } })
                                          .asStringPair() } } },
                QMap<QString, QMultiMap<QString, MockObject>> {
                        { QStringLiteral(u"mmap"),
                          QMultiMap<QString, MockObject> {
                                  { QStringLiteral(u"a"),
                                    MockObject(
                                            Path::fromField(u"mmap").withKey(u"a").withIndex(0),
                                            QMap<QString, MockObject> {},
                                            QMap<QString, QCborValue> {
                                                    { QStringLiteral(u"val1"), QCborValue(14) },
                                                    { QLatin1String("val2"), QCborValue(15) } }) },
                                  { QStringLiteral(u"a"),
                                    MockObject(Path::fromField(u"mmap").withKey(u"a").withIndex(1),
                                               QMap<QString, MockObject> {},
                                               QMap<QString, QCborValue> {
                                                       { QStringLiteral(u"val1"), QCborValue(16) },
                                                       { QLatin1String("val2"),
                                                         QCborValue(17) } }) } } } },
                QMap<QString, QList<MockObject>> {
                        { QStringLiteral(u"list"),
                          QList<MockObject> {
                                  MockObject(Path::fromField(u"list").withIndex(0),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(18) },
                                                     { QLatin1String("val2"), QCborValue(19) } }),
                                  MockObject(Path::fromField(u"list").withIndex(1),
                                             QMap<QString, MockObject> {},
                                             QMap<QString, QCborValue> {
                                                     { QStringLiteral(u"val1"), QCborValue(20) },
                                                     { QLatin1String("val2"),
                                                       QCborValue(21) } }) } } }));
        envPtr->setExtraOwningItem(QStringLiteral(u"testOwner"), testOwnerPtr);
        tOwner = env.field(u"testOwner");
    }

    void testList()
    {
        QList<int> l({ 1, 2, 3, 4 });
        QList<int> l2 = l;
        QList<int> l3({ 1 });
        QList<int> l4 = l3;
        QCOMPARE(&(l[1]), &(l[1]));
        QCOMPARE(&(l3[0]), &(l3[0]));
        // QCOMPARE(&(l3[0]), &(l4[0])); // shallow copy actually copies els (QVector behavior)...
        DomItem list1 = env.subListItem(List::fromQListRef<int>(Path::fromField(u"list"), l, &wrapInt));
        DomItem list2 = env.subListItem(List::fromQListRef<int>(Path::fromField(u"reverseList"), l,
                                                                &wrapInt, ListOptions::Reverse));
        QCOMPARE(list1.domKind(), DomKind::List);
        QCOMPARE(list1.indexes(), 4);
        QCOMPARE(list1[0].value().toInteger(), 1);
        QCOMPARE(list1[3].value().toInteger(), 4);
        QVERIFY(!list1[4]);
        QCOMPARE(list1[4].value().toInteger(-1), -1);
        QVERIFY(list1[0].value() != list2[0].value());
        QCOMPARE(list1[0].value(), list2[3].value());
        QCOMPARE(list1[3].value(), list2[0].value());
    }
    void testMap()
    {
        QMap<QString, int> map({ { QStringLiteral(u"a"), 1 }, { QStringLiteral(u"b"), 2 } });
        // QMap<QString, int> map2 = map;
        QMap<QString, int> map3({ { QStringLiteral(u"a"), 1 } });
        // QMap<QString, int> map4 = map3;
        auto it = map.find(QStringLiteral(u"a"));
        auto it2 = map.find(QStringLiteral(u"a"));
        auto it3 = map3.find(QStringLiteral(u"a"));
        auto it4 = map3.find(QStringLiteral(u"a"));
        // auto it5 = map4.find(QStringLiteral(u"a"));
        QVERIFY(it != map.end());
        QVERIFY(it2 != map.end());
        QCOMPARE(&(*it), &(*it2));
        QCOMPARE(&(*it), &(map[QStringLiteral(u"a")]));
        QCOMPARE(&(it.value()), &(it2.value()));
        // QCOMPARE(&(*it), &(map2[QStringLiteral(u"a")]));
        QCOMPARE(&(*it3), &(*it4));
        // QCOMPARE(&(*it3), &(*it5));
        DomItem map1 = env.subMapItem(Map::fromMapRef<int>(Path::fromField(u"map"), map, &wrapInt));
        QCOMPARE(map1.domKind(), DomKind::Map);
        QCOMPARE(map1[u"a"].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).value().toInteger(), 1);
        QCOMPARE(map1[u"b"].value().toInteger(), 2);
        QVERIFY(!map1[u"c"]);
    }
    void testMultiMap()
    {
        QMultiMap<QString, int> mmap({ { QStringLiteral(u"a"), 1 },
                                       { QStringLiteral(u"b"), 2 },
                                       { QStringLiteral(u"a"), 3 } });
        // QMultiMap<QString, int> mmap2 = mmap;
        QMultiMap<QString, int> mmap3({ { QStringLiteral(u"a"), 1 } });
        // QMultiMap<QString, int> mmap4 = mmap3;
        auto it = mmap.find(QStringLiteral(u"a"));
        auto it2 = mmap.find(QStringLiteral(u"a"));
        // auto it3 = mmap2.find(QStringLiteral(u"a"));
        auto it4 = mmap3.find(QStringLiteral(u"a"));
        auto it5 = mmap3.find(QStringLiteral(u"a"));
        // auto it6 = mmap4.find(QStringLiteral(u"a"));
        QVERIFY(it != mmap.end());
        QVERIFY(it2 != mmap.end());
        QCOMPARE(&(it.value()), &(it2.value()));
        QCOMPARE(&(*it), &(it2.value()));
        // QCOMPARE(&(*it), &(*it2)); // copy has different address (copies elements for int)
        // QCOMPARE(&(*it), &(*it3));
        QCOMPARE(&(*it4), &(*it5));
        // QCOMPARE(&(*it4), &(*it6));
        DomItem map1 = env.subMapItem(Map::fromMultiMapRef<int>(Path::fromField(u"mmap"), mmap));
        QCOMPARE(map1[u"b"].index(0).value().toInteger(), 2);
        QVERIFY(!map1[u"b"].index(2));
        QVERIFY(!map1[u"c"]);
        QCOMPARE(map1[u"a"][0].value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(0).value().toInteger(), 1);
        QCOMPARE(map1.key(QStringLiteral(u"a")).index(1).value().toInteger(), 3);
        {
            QMultiMap<QString, DomTestClass> m1;
            m1.insert(QStringLiteral(u"xx"), DomTestClass { std::shared_ptr<int>(new int(4)) });
            QCOMPARE(m1.begin().value().i.use_count(), 1);
            QMultiMap<QString, DomTestClass> m2 = m1;
            m1.clear();
            auto it = m2.cbegin();
            auto end = m2.cend();
            while (it != end) {
                QCOMPARE(it.value().i.use_count(), 1);
                m1.insert(it.key(), it.value());
                QCOMPARE(it.value().i.use_count(), 2);
                ++it;
            }
            m2.insert(QStringLiteral(u"xy"), DomTestClass { std::shared_ptr<int>(new int(8)) });
            QMultiMap<QString, DomTestClass> m3 = m2;
            m3.begin().value() = DomTestClass { std::shared_ptr<int>(new int(2)) };
            auto it2 = m2.begin();
            auto it3 = m3.begin();
            QCOMPARE(*it2.value().i, 4);
            QCOMPARE(*it3.value().i, 2);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 1);
            m3.insert(QStringLiteral(u"xz"), DomTestClass { std::shared_ptr<int>(new int(16)) });
            it2 = m2.begin();
            it3 = m3.begin();
            QCOMPARE(*it2.value().i, 4);
            QCOMPARE(*it3.value().i, 2);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 1);
            ++it2;
            ++it3;
            QCOMPARE(*it2.value().i, 8);
            QCOMPARE(*it3.value().i, 8);
            QCOMPARE(it2.value().i.use_count(), 2);
            QCOMPARE(it3.value().i.use_count(), 2);
            ++it2;
            ++it3;
            QVERIFY(it2 == m2.end());
            QVERIFY(it3 != m3.end());
            QCOMPARE(*it3.value().i, 16);
            QCOMPARE(it3.value().i.use_count(), 1);
            ++it3;
            QVERIFY(it3 == m3.end());
        }
    }
    void testReference()
    {
        Path p = Path::fromRoot(u"env");
        DomItem ref = env.subReferenceItem(PathEls::Field(u"ref"), p);
        QCOMPARE(ref.field(u"referredObjectPath").value().toString(), p.toString());
        QCOMPARE(ref.fields(),
                 QList<QString>({ QStringLiteral(u"referredObjectPath"), QStringLiteral(u"get") }));
        QCOMPARE(ref.field(u"get").internalKind(), DomType::DomEnvironment);
        // test stability (cache)
        QCOMPARE(ref.field(u"get").internalKind(), DomType::DomEnvironment);
    }
    void testRefCache()
    {
        Path refPath = env.canonicalPath().withField(u"dummyRef");
        RefCacheEntry e0 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e0.cached, RefCacheEntry::Cached::None);
        bool didAdd1 = RefCacheEntry::addForPath(
                env, refPath, RefCacheEntry { RefCacheEntry::Cached::First, {} });
        QVERIFY(didAdd1);
        RefCacheEntry e1 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e1.cached, RefCacheEntry::Cached::All);
        QCOMPARE(e1.canonicalPaths.isEmpty(), true);
        bool didAdd2 = RefCacheEntry::addForPath(
                env, refPath,
                RefCacheEntry { RefCacheEntry::Cached::First, { env.canonicalPath() } },
                AddOption::Overwrite);
        QVERIFY(didAdd2);
        RefCacheEntry e2 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e2.cached, RefCacheEntry::Cached::First);
        QCOMPARE(e2.canonicalPaths.size(), 1);
        QCOMPARE(e2.canonicalPaths.first().toString(), env.canonicalPath().toString());
        bool didAdd3 = RefCacheEntry::addForPath(
                env, refPath,
                RefCacheEntry { RefCacheEntry::Cached::All,
                                { env.canonicalPath(), tOwner.canonicalPath() } },
                AddOption::Overwrite);
        QVERIFY(didAdd3);
        RefCacheEntry e3 = RefCacheEntry::forPath(env, refPath);
        QCOMPARE(e3.cached, RefCacheEntry::Cached::All);
        QCOMPARE(e3.canonicalPaths.size(), 2);
        QCOMPARE(e3.canonicalPaths.first().toString(), env.canonicalPath().toString());
        QCOMPARE(e3.canonicalPaths.last().toString(), tOwner.canonicalPath().toString());
    }
    void testEnvUniverse()
    {
        QCOMPARE(env.internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.pathFromOwner(), Path());
        QCOMPARE(env.containingObject().internalKind(), DomType::Empty);
        QCOMPARE(env.container().internalKind(), DomType::Empty);
        QCOMPARE(env.canonicalPath(), Path::fromRoot(u"env"));
        QCOMPARE(env.path(u"$env").internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.top().internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.environment().internalKind(), DomType::DomEnvironment);
        QCOMPARE(env.owningItemPtr(), envPtr);
        QCOMPARE(env.topPtr(), envPtr);
        DomItem univ = env.universe();
        QCOMPARE(univ.internalKind(), DomType::DomUniverse);
        QCOMPARE(univ.owningItemPtr(), universePtr);
        DomItem univ2 = env.path(u".universe");
        QCOMPARE(univ2.internalKind(), DomType::DomUniverse);
        QCOMPARE(univ2.owningItemPtr(), universePtr);
        QCOMPARE(univ2.topPtr(), universePtr);
        DomItem univ3 = env.field(u"universe");
        QCOMPARE(univ3.internalKind(), DomType::DomUniverse);
    }

    void testTOwner()
    {
        QVERIFY(env.fields().contains(QLatin1String("testOwner")));
        QCOMPARE(tOwner.internalKind(), DomType::MockOwner);
        QCOMPARE(tOwner.pathFromOwner(), Path());
        DomItem map = tOwner.field(u"map");
        QCOMPARE(map[u"b"].field(u"val1").value().toInteger(), 12);
        QCOMPARE(map[u"b"].container(), map);
        QCOMPARE(map[u"b"].container()[u"b"].field(u"val1").value().toInteger(), 12);
        QCOMPARE(map[u"b"].containingObject(), tOwner);
        DomItem mmap = tOwner.field(u"mmap");
        QCOMPARE(mmap[u"a"].index(0).field(u"val1").value().toInteger(), 14);
        QCOMPARE(mmap[u"a"].container(), mmap);
        QCOMPARE(mmap[u"a"].container()[u"a"].index(0).field(u"val1").value().toInteger(), 14);
        QCOMPARE(mmap[u"a"].containingObject(), tOwner);
        QCOMPARE(mmap[u"a"].index(0).container(), mmap[u"a"]);
        QCOMPARE(mmap[u"a"].index(0).containingObject(), tOwner);
        DomItem list = tOwner.field(u"list");
        QCOMPARE(list[0].field(u"val1").value().toInteger(), 18);
        QCOMPARE(list.container(), tOwner);
        QCOMPARE(list[0].container(), list);
        QCOMPARE(list[0].containingObject(), tOwner);
        QCOMPARE(list[0].container()[0].field(u"val1").value().toInteger(), 18);
        QCOMPARE(tOwner.containingObject().internalKind(), DomType::DomEnvironment);
        QCOMPARE(tOwner.container().internalKind(), DomType::DomEnvironment);
        QCOMPARE(tOwner.fields(),
                 QStringList({ QStringLiteral(u"val1"), QStringLiteral(u"obj1"),
                               QStringLiteral(u"obj2"), QStringLiteral(u"map"),
                               QStringLiteral(u"mmap"), QStringLiteral(u"list") }));
        auto tOwner2 = env.path(u"$env.testOwner");
        QCOMPARE(tOwner2.internalKind(), DomType::MockOwner);
        auto tOwner3 = tOwner.path(u"$env.testOwner");
        QCOMPARE(tOwner3.internalKind(), DomType::MockOwner);
        QList<qint64> values;
        tOwner.visitTree(Path(), [&values](const Path &p, DomItem i, bool) {
            if (i.pathFromOwner() != p)
                myErrors()
                        .error(QStringLiteral(u"unexpected path %1 %2")
                                       .arg(i.pathFromOwner().toString(), p.toString()))
                        .handle(defaultErrorHandler);
            Q_ASSERT(i == i.path(i.canonicalPath()));
            if (DomItem v1 = i.path(u".val1"))
                values.append(v1.value().toInteger());
            return true;
        });
        QCOMPARE(values, QList<qint64>({ 9, 1, 3, 7, 5, 10, 12, 14, 16, 18, 20 }));
    }
    void testSubObj()
    {
        auto obj1 = tOwner.field(u"obj1");
        QCOMPARE(obj1.internalKind(), DomType::MockObject);
        auto obj1_1 = env.path(u".testOwner.obj1.obj1_2");
        QCOMPARE(obj1_1.internalKind(), DomType::MockObject);
        QCOMPARE(obj1_1.field(u"val1").value().toInteger(), 3);
    }
    void testEquality()
    {
        auto obj1 = tOwner.field(u"obj1");
        auto obj1_1 = env.path(u".testOwner.obj1.obj1_2");
        QCOMPARE(obj1_1.container(), obj1);
        QCOMPARE(obj1_1.environment(), env);
        QCOMPARE(obj1_1.owner(), tOwner);
    }
    void testLoadNoDep()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        auto univPtr = std::shared_ptr<QQmlJS::Dom::DomUniverse>(
                new QQmlJS::Dom::DomUniverse(QLatin1String("univ1")));
        auto envPtr = std::shared_ptr<QQmlJS::Dom::DomEnvironment>(new QQmlJS::Dom::DomEnvironment(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                DomCreationOption::Default, univPtr));
        QQmlJS::Dom::DomItem env(envPtr);
        QVERIFY(env);
        QString testFile1 = baseDir + QLatin1String("/test1.qml");
        DomItem tFile;
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile1),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, baseDir), {});
        envPtr->loadPendingDependencies();

        QVERIFY(tFile);
        tFile = tFile.field(Fields::currentItem);
        QVERIFY(tFile);
        DomItem comp1 = tFile.field(Fields::components).key(QString()).index(0);
        QVERIFY(comp1);
        DomItem obj1 = comp1.field(Fields::objects).index(0);
        QVERIFY(obj1);

        tFile.visitTree(Path(), [&tFile](const Path &p, DomItem i, bool) {
            if (!(i == i.path(i.canonicalPath()))) {
                DomItem i2 = i.path(i.canonicalPath());
                qDebug() << p << i.canonicalPath() << i.internalKindStr() << i2.internalKindStr()
                         << i.id() << i2.id() << i.pathFromOwner() << i2.pathFromOwner();
            }
            Q_ASSERT(i == i.path(i.canonicalPath()));
            Q_ASSERT(!i || i == tFile.path(i.canonicalPath()));
            Q_ASSERT(!i || i.containingObject());

            return true;
        });

        {
            DomItem width = obj1.field(Fields::bindings).key(QLatin1String("width")).index(0);
            QCOMPARE(width.field(Fields::value).qmlObject(), obj1);
            DomItem w = obj1.bindings().key(QLatin1String("width"));
            QVERIFY(w.indexes() > 0);
            QCOMPARE(w.indexes(), 1);
            QVERIFY(w.index(0).as<Binding>());
            QVERIFY(w.index(0).as<Binding>()->scriptExpressionValue());
            QCOMPARE(w.index(0).as<Binding>()->scriptExpressionValue()->code(), u"640");
            PropertyInfo mPInfo;
            mPInfo.bindings = { width };
            mPInfo.propertyDefs.append(width);
            DomItem wrappedPInfo = obj1.wrapField(Fields::propertyInfos, mPInfo);
            QVERIFY(wrappedPInfo);
            const SimpleObjectWrapBase *wrappedPInfoPtr =
                    static_cast<const SimpleObjectWrapBase *>(wrappedPInfo.base());
            QVERIFY(wrappedPInfoPtr);
            const PropertyInfo *p1 =
                    reinterpret_cast<const PropertyInfo *>(wrappedPInfoPtr->m_value.data());
            PropertyInfo p2 = wrappedPInfoPtr->m_value.value<PropertyInfo>();
            QCOMPARE(mPInfo.bindings.size(), 1);
            QCOMPARE(mPInfo.propertyDefs.size(), 1);
            QCOMPARE(mPInfo.bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(mPInfo.propertyDefs.first().toString(),
                     mPInfo.propertyDefs.first().toString());

            QCOMPARE(p2.bindings.size(), 1);
            QCOMPARE(p2.propertyDefs.size(), 1);
            QCOMPARE(p2.bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(p2.propertyDefs.first().toString(), mPInfo.propertyDefs.first().toString());
            QCOMPARE(p1->bindings.size(), 1);
            QCOMPARE(p1->propertyDefs.size(), 1);
            QCOMPARE(p1->bindings.first().toString(), mPInfo.bindings.first().toString());
            QCOMPARE(p1->propertyDefs.first().toString(), mPInfo.propertyDefs.first().toString());
        }
        QString bPath = QFileInfo(baseDir).canonicalPath();
        if (!bPath.isEmpty()) {
            Path p = tFile.canonicalPath();
            Q_ASSERT(env.path(p));
            env.ownerAs<DomEnvironment>()->removePath(bPath);
            Q_ASSERT(!env.path(p));
        }
    }

    void testLoadDep()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        auto univPtr = std::shared_ptr<QQmlJS::Dom::DomUniverse>(
                new QQmlJS::Dom::DomUniverse(QLatin1String("univ1")));
        auto envPtr = std::shared_ptr<QQmlJS::Dom::DomEnvironment>(new QQmlJS::Dom::DomEnvironment(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, {}, univPtr));
        QQmlJS::Dom::DomItem env(envPtr);
        QVERIFY(env);
        QString testFile1 = baseDir + QLatin1String("/test1.qml");
        DomItem tFile;
        envPtr->loadBuiltins();
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile1),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, baseDir), {});
        envPtr->loadPendingDependencies();

        QVERIFY(tFile);
        tFile = tFile.field(Fields::currentItem);
        QVERIFY(tFile);
        DomItem comp1 = tFile.field(Fields::components).key(QString()).index(0);
        QVERIFY(comp1);
        DomItem obj1 = comp1.field(Fields::objects).index(0);
        QVERIFY(obj1);

        {
            using namespace Qt::StringLiterals;

            QList<DomItem> rect =
                    obj1.lookup(u"Rectangle"_s, LookupType::Type, LookupOption::Normal);
            QList<DomItem> rect2 =
                    obj1.lookup(u"Rectangle"_s, LookupType::Symbol, LookupOption::Normal);
            QList<DomItem> rectAs =
                    obj1.lookup(u"QQ.Rectangle"_s, LookupType::Symbol, LookupOption::Normal);

            QVERIFY(rect.size() == 1);
            QVERIFY(rect2.size() == 1);
            QVERIFY(rectAs.size() == 1);
            QCOMPARE(rect.first().internalKind(), DomType::Export);
            QCOMPARE(rect.first(), rect2.first());
            QCOMPARE(rect.first(), rectAs.first());
            DomItem rect3 = rect.first().proceedToScope();
            QCOMPARE(rect3.internalKind(), DomType::QmlObject);
            QList<DomItem> rects;
            obj1.resolve(
                    Path::fromCurrent(PathCurrent::Lookup).withField(Fields::type).withKey(u"Rectangle"_s),
                    [&rects](Path, const DomItem &el) {
                        rects.append(el);
                        return true;
                    },
                    {});
            QVERIFY(rects.size() == 1);
            for (const DomItem &el : rects) {
                QCOMPARE(rect.first(), el);
            }
        }
        {
            QString fPath = tFile.canonicalFilePath();
            QString fPath2 = fPath.mid(0, fPath.lastIndexOf(u'/')) % u"/MySingleton.qml";
            Path p2 = Paths::qmlFileObjectPath(fPath2);
            DomItem f2 = env.path(p2);
            QVERIFY2(f2, "Directory dependencies did not load MySingleton.qml");
        }
        {
            QString fPath = tFile.canonicalFilePath();
            QString fPath2 = fPath.mid(0, fPath.lastIndexOf(u'/')) % u"/ImportMeImplicitly.ui.qml";
            Path p2 = Paths::qmlFileObjectPath(fPath2);
            DomItem f2 = env.path(p2);
            QVERIFY2(f2, "Directory dependencies did not load .ui.qml file!");
        }
    }

    void testImports()
    {
#ifdef Q_OS_ANDROID
        QSKIP("Test uncompatible with Android (QTBUG-100171)");
#endif
        using namespace Qt::StringLiterals;

        QString testFile1 = baseDir + QLatin1String("/TestImports.qml");
        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile1),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) {
                             tFile = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        QVERIFY(tFile);
        QList<QmlUri> importedModules;
        for (auto &import : tFile.field(Fields::imports).values()) {
            if (const Import *importPtr = import.as<Import>()) {
                if (!importPtr->implicit)
                    importedModules.append(importPtr->uri);
            }
        }
        QCOMPARE(importedModules.at(0).moduleUri(), u"QtQuick"_s);
        QCOMPARE(importedModules.at(0).directoryString(), u""_s);
        QCOMPARE(importedModules.at(1).directoryString(), u"../.."_s);
        QCOMPARE(importedModules.at(1).localPath(), u"../.."_s);
        QCOMPARE(importedModules.at(1).absoluteLocalPath(), QString());
        QCOMPARE(importedModules.at(1).absoluteLocalPath(u"/bla/bla"_s), u"/bla/bla/../..");
        QCOMPARE(importedModules.at(2).directoryString(), u"../dommerging"_s);
        QCOMPARE(importedModules.at(2).localPath(), u"../dommerging"_s);
        QCOMPARE(importedModules.at(2).absoluteLocalPath(), QString());
        QCOMPARE(importedModules.at(2).absoluteLocalPath(u"/bla/bla"_s),
                 u"/bla/bla/../dommerging");
        QCOMPARE(importedModules.at(3).directoryString(), u"C:/some/path"_s);
        QCOMPARE(importedModules.at(3).localPath(), u"C:/some/path"_s);
        QCOMPARE(importedModules.at(4).directoryString(), u"http://bla.com/"_s);
        QCOMPARE(importedModules.at(4).directoryUrl().toString(), u"http://bla.com/"_s);
        QCOMPARE(importedModules.at(5).absoluteLocalPath(), u"/absolute/path"_s);
        QVERIFY(QmlUri::fromDirectoryString("QtQuick") != importedModules.at(0));
        QCOMPARE(QmlUri::fromUriString("QtQuick"), importedModules.at(0));
    }
    void testDeepCopy()
    {
        QString testFile = baseDir + QLatin1String("/test1.qml");

        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile; // place where to store the loaded file
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
        envPtr->loadPendingDependencies();
        DomItem f = tFile.fileObject();
        QString dump1;
        f.dump([&dump1](QStringView v) { dump1.append(v); });
        MutableDomItem copy = f.makeCopy();
        QString dump2;
        copy.item().dump([&dump2](QStringView v) { dump2.append(v); });
        QString diff = lineDiff(dump1, dump2, 2);
        if (!diff.isEmpty())
            qDebug().nospace().noquote() << diff;
        QCOMPARE(dump1, dump2);
        QStringList diffs = domCompareStrList(f, copy, FieldFilter::compareFilter());
        if (!diffs.isEmpty())
            qDebug() << "testDeepCopy.diffs:" << diffs;
        QVERIFY(diffs.isEmpty());
        DomItem env(envPtr);
        DomItem univFile = env.universe().path(f.canonicalPath());
        MutableDomItem univFileCopy = univFile.makeCopy();
        QStringList univFileDiffs =
                domCompareStrList(univFile, univFileCopy, FieldFilter::compareFilter());
        if (!univFileDiffs.isEmpty())
            qDebug() << "testDeepCopy.univFileDiffs:" << univFileDiffs;
        QVERIFY(univFileDiffs.isEmpty());
        QString bPath = QFileInfo(baseDir).canonicalFilePath();
        if (!bPath.isEmpty()) {
            Path p = f.canonicalPath();
            Q_ASSERT(env.path(p));
            env.ownerAs<DomEnvironment>()->removePath(bPath);
            Q_ASSERT(!env.path(p));
        }
    }

    void testInMemory()
    {
        DomItem res = DomItem::fromCode("MyItem{}");
        QCOMPARE(res.qmlObject(GoTo::Strict), DomItem());
        DomItem obj = res.qmlObject(GoTo::MostLikely);
        QCOMPARE(obj.name(), u"MyItem");
    }

    static void checkAliases(const DomItem &qmlObj)
    {
        using namespace Qt::StringLiterals;

        if (const QmlObject *qmlObjPtr = qmlObj.as<QmlObject>()) {
            auto pDefs = qmlObjPtr->propertyDefs();
            auto i = pDefs.constBegin();
            while (i != pDefs.constEnd()) {
                if (i.value().isAlias()) {
                    QString propName = i.key();
                    DomItem value = qmlObj.bindings().key(propName).index(0).field(Fields::value);
                    LocallyResolvedAlias rAlias =
                            qmlObjPtr->resolveAlias(qmlObj, value.ownerAs<ScriptExpression>());
                    if (propName.startsWith(u"a")) {
                        QCOMPARE(rAlias.baseObject.internalKind(), DomType::QmlObject);
                        switch (propName.last(1).at(0).unicode()) {
                        case u'i':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedProperty);
                            QCOMPARE(rAlias.typeName, u"int"_s);
                            QVERIFY(rAlias.accessedPath.isEmpty());
                            QCOMPARE(rAlias.localPropertyDef.internalKind(),
                                     DomType::PropertyDefinition);
                            break;
                        case u'r':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedProperty);
                            QCOMPARE(rAlias.typeName, u"real"_s);
                            QVERIFY(rAlias.accessedPath.isEmpty());
                            QCOMPARE(rAlias.localPropertyDef.internalKind(),
                                     DomType::PropertyDefinition);
                            break;
                        case u'I':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"Item"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(!rAlias.localPropertyDef);
                            break;
                        case u'q':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"QtObject"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(!rAlias.localPropertyDef);
                            break;
                        case u'Q':
                            QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                            QCOMPARE(rAlias.typeName, u"QtObject"_s);
                            QCOMPARE(rAlias.accessedPath, QStringList { u"objectName"_s });
                            QVERIFY(rAlias.localPropertyDef);
                            break;
                        default:
                            Q_ASSERT(false);
                        }
                    } else if (propName.startsWith(u"loop")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::Loop);
                    } else if (propName.startsWith(u"tooDeep")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::TooDeep);
                    } else if (propName.startsWith(u"invalid")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::Invalid);
                    } else if (propName.startsWith(u"objRef")) {
                        QCOMPARE(rAlias.status, LocallyResolvedAlias::Status::ResolvedObject);
                    } else {
                        Q_ASSERT(false);
                    }
                }
                ++i;
            }
            for (const DomItem &obj : qmlObj.children().values()) {
                if (obj.as<QmlObject>())
                    checkAliases(obj);
            }
        }
    }

    void testAliasResolve_data()
    {
        QTest::addColumn<QString>("inFile");

        QTest::newRow("aliasProperties") << QStringLiteral(u"aliasProperties.qml");
        QTest::newRow("invalidAliasProperties") << QStringLiteral(u"invalidAliasProperties.qml");
    }
    void testAliasResolve()
    {
        using namespace Qt::StringLiterals;

        QFETCH(QString, inFile);
        QString testFile1 = baseDir + u"/"_s + inFile;
        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile1),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) {
                             tFile = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        DomItem rootObj = tFile.qmlObject(GoTo::MostLikely);
        checkAliases(rootObj);
    }

    void inlineComponents()
    {
        using namespace Qt::StringLiterals;

        QString testFile = baseDir + u"/inlineComponents.qml"_s;

        DomCreationOption options = DomCreationOption::Extended;
        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies, options);

        DomItem tFile;
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) {
                             tFile = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        auto rootQmlObject = tFile.rootQmlObject(GoTo::MostLikely);

        // check if the lookup can find the inline components correctly, to see if the
        // visitScopeChain also visit them.
        auto ic3 = rootQmlObject.lookup("IC3", LookupType::Type, LookupOption::Normal,
                                        [](const ErrorMessage &) {});

        QCOMPARE(ic3.size(), 1);
        QCOMPARE(ic3.front().name(), "inlineComponents.IC3");
        QCOMPARE(ic3.front().field(Fields::nameIdentifiers).internalKind(), DomType::ScriptType);
        QCOMPARE(ic3.front()
                         .field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .value()
                         .toString(),
                 u"IC3"_s);

        auto ic1 = rootQmlObject.lookup("IC1", LookupType::Type, LookupOption::Normal,
                                        [](const ErrorMessage &) {});

        QCOMPARE(ic1.size(), 1);
        QCOMPARE(ic1.front().name(), "inlineComponents.IC1");
    }

    void inlineObject()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/inlineObject.qml"_s;

        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        DomItem tFile;
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFile),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) {
                             tFile = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        auto rootQmlObject = tFile.rootQmlObject(GoTo::MostLikely);

        // check that the inline objects have their prototypes set.

        {
            auto prototypes = rootQmlObject.propertyInfos()
                                      .key(u"myItem"_s)
                                      .field(Fields::bindings)
                                      .index(0)
                                      .field(Fields::value)
                                      .field(Fields::prototypes);
            QVERIFY(prototypes.internalKind() != DomType::Empty);
            QCOMPARE(prototypes.indexes(), 1);
            QCOMPARE(prototypes.index(0)
                             .field(Fields::referredObjectPath)
                             .as<ConstantData>()
                             ->value()
                             .toString(),
                     u"@lookup.type[\"Item\"]"_s);
        }

        {
            auto prototypes2 = rootQmlObject.propertyInfos()
                                       .key(u"myItem2"_s)
                                       .field(Fields::bindings)
                                       .index(0)
                                       .field(Fields::value)
                                       .field(Fields::prototypes);
            QVERIFY(prototypes2.internalKind() != DomType::Empty);
            QCOMPARE(prototypes2.indexes(), 1);
            QCOMPARE(prototypes2.index(0)
                             .field(Fields::referredObjectPath)
                             .as<ConstantData>()
                             ->value()
                             .toString(),
                     u"@lookup.type[\"IC\"]"_s);
        }
    }

    void scopesInDom()
    {
        QString fileName = baseDir + u"/checkScopes.qml"_s;

        const QStringList importPaths = {
            QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
        };

        DomItem tFile;

        auto envPtr = DomEnvironment::create(
                importPaths,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                Extended);

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, fileName),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) {
                             tFile = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        auto root = tFile.rootQmlObject(GoTo::MostLikely);

        {
            auto rootQmlObject = root.as<QmlObject>();
            QVERIFY(rootQmlObject);
            auto rootScope = rootQmlObject->semanticScope();
            QVERIFY(rootScope);
            QVERIFY(rootScope->hasOwnProperty("myInt"));
            QVERIFY(rootScope->hasOwnProperty("myInt2"));
            QVERIFY(rootScope->hasOwnPropertyBindings("myInt"));
            QVERIFY(rootScope->hasOwnPropertyBindings("myInt2"));
        }
    }

    void propertyBindings()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/propertyBindings.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        // check the binding to a and b
        DomItem a = rootQmlObject.path(".bindings[\"a\"][0].value.scriptElement.value");
        QCOMPARE(a.value().toDouble(), 42);

        auto aLoc = FileLocations::treeOf(rootQmlObject.path(".bindings[\"a\"][0]"));
        QVERIFY(aLoc);
        QCOMPARE(aLoc->info().fullRegion.startLine, 7);
        QCOMPARE(aLoc->info().fullRegion.startColumn, 19);

        DomItem b = rootQmlObject.path(".bindings[\"b\"][0].value.scriptElement.identifier");
        QCOMPARE(b.value().toString(), "a");

        DomItem root = rootQmlObject.path(".idStr");
        QCOMPARE(root.value().toString(), "root");

        DomItem componentIds = rootQmlObject.component().path(".ids");
        QCOMPARE(componentIds.keys(), QSet<QString>({ "root" }));

        DomItem idScriptExpression =
                componentIds.key("root").index(0).field(Fields::value).field(Fields::scriptElement);
        QCOMPARE(idScriptExpression.internalKind(), DomType::ScriptIdentifierExpression);
        QCOMPARE(idScriptExpression.field(Fields::identifier).value().toString(), "root");
    }

    void variableDeclarations()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/variableDeclarations.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement");

        // check that variabledeclarationlists and statement lists are correctly collected
        {
            // let one = 1, two = 2, three = 3, four = 4, five = 5, six = 6
            std::vector<QString> data = { u"one"_s,  u"two"_s,  u"three"_s,
                                          u"four"_s, u"five"_s, u"six"_s };
            DomItem block =
                    rootQmlObject.path(".methods[\"count\"][0].body.scriptElement.statements");
            DomItem variableDeclaration = block.index(0).field(Fields::declarations);
            QCOMPARE((size_t)variableDeclaration.indexes(), data.size());
            for (size_t i = 0; i < data.size(); ++i) {
                DomItem variableDeclarationEntry = variableDeclaration.index(i);
                QCOMPARE(variableDeclarationEntry.field(Fields::identifier).value().toString(),
                         data[i]);
                QCOMPARE((size_t)variableDeclarationEntry.field(Fields::initializer)
                                 .value()
                                 .toInteger(),
                         i + 1);
            }
            //  let testMe = 123
            DomItem testDeclaration = block.index(1).field(Fields::declarations);
            QCOMPARE(testDeclaration.indexes(), 1);
            QCOMPARE(testDeclaration.index(0).field(Fields::identifier).value().toString(),
                     u"testMe"_s);
            QCOMPARE(testDeclaration.index(0).field(Fields::initializer).value().toInteger(),
                     123ll);
        }

        // if there is a failure in the following line, then thats because either the
        // variabledeclarationlist or the statement list was wrongly collected and the
        // domcreator gave up on creating the Dom (unexpected DomType: DomType::ScriptExpression
        // means that no Dom was constructed for f's body)

        // This block should have a semantic scope that defines sum and helloWorld
        auto blockSemanticScope = block.semanticScope();
        QVERIFY(blockSemanticScope);
        QVERIFY(blockSemanticScope);
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"sum"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"helloWorld"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"a"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"b"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"aa"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"bb"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"bool1"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"bool2"_s));
        QVERIFY(blockSemanticScope->ownJSIdentifier(u"nullVar"_s));
        DomItem statements = block.field(Fields::statements);
        QCOMPARE(statements.indexes(), 8);

        // avoid that toInteger calls return 0 on error, which sometimes is the correct response.
        const int invalidEnumValue = 9999;

        {
            // let sum = 0, helloWorld = "hello"
            DomItem variableDeclaration = statements.index(0).field(Fields::declarations);
            QCOMPARE(variableDeclaration.indexes(), 2);
            DomItem sumInitialization = variableDeclaration.index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(sumInitialization.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Let);
            QCOMPARE(sumInitialization.field(Fields::identifier).value().toString(), "sum");
            QCOMPARE(sumInitialization.field(Fields::initializer)
                             .field(Fields::value)
                             .value()
                             .toDouble(),
                     0);

            DomItem helloWorldInitialization = variableDeclaration.index(1);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(helloWorldInitialization.field(Fields::scopeType)
                             .value()
                             .toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Let);
            QCOMPARE(helloWorldInitialization.field(Fields::identifier).value().toString(),
                     "helloWorld");
            QCOMPARE(helloWorldInitialization.field(Fields::initializer)
                             .field(Fields::value)
                             .value()
                             .toString(),
                     "hello");
        }
        {
            // const a = 3
            DomItem a = statements.index(1).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(a.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Const);
            QCOMPARE(a.field(Fields::identifier).value().toString(), "a");
            QCOMPARE(a.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(a.field(Fields::initializer).field(Fields::value).value().toInteger(), 3);
        }
        {
            // const b = "patron"
            DomItem b = statements.index(2).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(b.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Const);
            QCOMPARE(b.field(Fields::identifier).value().toString(), "b");
            QCOMPARE(b.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(b.field(Fields::initializer).field(Fields::value).value().toString(),
                     "patron");
        }
        {
            // var aa = helloWorld
            DomItem aa = statements.index(3).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(aa.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Var);
            QCOMPARE(aa.field(Fields::identifier).value().toString(), "aa");
            QCOMPARE(aa.field(Fields::initializer).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(aa.field(Fields::initializer).field(Fields::identifier).value().toString(),
                     "helloWorld");
            // var bb = aa
            DomItem bb = statements.index(3).field(Fields::declarations).index(1);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(bb.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Var);
            QCOMPARE(bb.field(Fields::identifier).value().toString(), "bb");
            QCOMPARE(bb.field(Fields::initializer).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(bb.field(Fields::initializer).field(Fields::identifier).value().toString(),
                     "aa");
        }
        {
            // const bool1 = true
            DomItem bool1 = statements.index(4).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(bool1.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Const);
            QCOMPARE(bool1.field(Fields::identifier).value().toString(), "bool1");
            QCOMPARE(bool1.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);
            QVERIFY(bool1.field(Fields::initializer).field(Fields::value).value().isTrue());
        }
        {
            // let bool2 = false
            DomItem bool2 = statements.index(5).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(bool2.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Let);
            QCOMPARE(bool2.field(Fields::identifier).value().toString(), "bool2");
            QCOMPARE(bool2.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);
            QVERIFY(bool2.field(Fields::initializer).field(Fields::value).value().isFalse());
        }
        {
            // var nullVar = null
            DomItem nullVar = statements.index(6).field(Fields::declarations).index(0);
            QEXPECT_FAIL(0, "ScopeTypes not implmented yet, as not needed by qmlls for now!",
                         Continue);
            QCOMPARE(nullVar.field(Fields::scopeType).value().toInteger(invalidEnumValue),
                     ScriptElements::VariableDeclarationEntry::ScopeType::Var);
            QCOMPARE(nullVar.field(Fields::identifier).value().toString(), "nullVar");
            QCOMPARE(nullVar.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);
            QVERIFY(nullVar.field(Fields::initializer).field(Fields::value).value().isNull());
        }
    }

    void ifStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/ifStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"conditional\"][0].body.scriptElement");
        auto blockSemanticScope = block.semanticScope();
        QVERIFY(blockSemanticScope);

        DomItem statements = block.field(Fields::statements);
        QCOMPARE(statements.indexes(), 5);

        // let i = 5
        DomItem iDeclaration = statements.index(0);
        QCOMPARE(iDeclaration.internalKind(), DomType::ScriptVariableDeclaration);

        {
            // if (i)
            //     i = 42
            DomItem conditional = statements.index(1);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(condition.field(Fields::identifier).value().toString(), u"i"_s);

            DomItem consequence = conditional.field(Fields::consequence);
            auto nonBlockSemanticScope = consequence.semanticScope();
            QVERIFY(!nonBlockSemanticScope); // because there is no block

            QCOMPARE(consequence.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(consequence.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence.field(Fields::right).field(Fields::value).value().toDouble(), 42);

            QCOMPARE(conditional.field(Fields::alternative).internalKind(), DomType::Empty);
        }
        {
            // if (i == 55)
            //     i = 32
            // else
            //     i = i - 1
            DomItem conditional = statements.index(2);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 55);

            DomItem consequence = conditional.field(Fields::consequence);
            auto nonBlockSemanticScope = consequence.semanticScope();
            QVERIFY(!nonBlockSemanticScope);

            QCOMPARE(consequence.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(consequence.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence.field(Fields::right).field(Fields::value).value().toDouble(), 32);

            DomItem alternative = conditional.field(Fields::alternative);
            QCOMPARE(alternative.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(alternative.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(alternative.field(Fields::right).internalKind(),
                     DomType::ScriptBinaryExpression);
        }
        {
            // if (i == 42) {
            //     i = 111
            // }
            DomItem conditional = statements.index(3);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 42);

            DomItem consequence = conditional.field(Fields::consequence);
            auto blockSemanticScope = consequence.semanticScope();
            QVERIFY(blockSemanticScope);

            QCOMPARE(consequence.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(consequence.field(Fields::statements).indexes(), 1);
            DomItem consequence1 = consequence.field(Fields::statements).index(0);
            QCOMPARE(consequence1.field(Fields::left).field(Fields::identifier).value().toString(),
                     u"i"_s);
            QCOMPARE(consequence1.field(Fields::right).field(Fields::value).value().toDouble(),
                     111);

            QCOMPARE(conditional.field(Fields::alternative).internalKind(), DomType::Empty);
        }
        {
            // if (i == 746) {
            //     i = 123
            // } else {
            //     i = 456
            // }

            DomItem conditional = statements.index(4);
            DomItem condition = conditional.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 746);

            {
                DomItem consequence = conditional.field(Fields::consequence);
                QCOMPARE(consequence.internalKind(), DomType::ScriptBlockStatement);
                QCOMPARE(consequence.field(Fields::statements).indexes(), 1);
                DomItem consequence1 = consequence.field(Fields::statements).index(0);
                QCOMPARE(consequence1.field(Fields::left)
                                 .field(Fields::identifier)
                                 .value()
                                 .toString(),
                         u"i"_s);
                QCOMPARE(consequence1.field(Fields::right).field(Fields::value).value().toDouble(),
                         123);
            }

            {
                DomItem alternative = conditional.field(Fields::alternative);
                auto blockSemanticScope = alternative.semanticScope();
                QVERIFY(blockSemanticScope);

                QCOMPARE(alternative.internalKind(), DomType::ScriptBlockStatement);
                QCOMPARE(alternative.field(Fields::statements).indexes(), 1);
                DomItem alternative1 = alternative.field(Fields::statements).index(0);
                QCOMPARE(alternative1.field(Fields::left)
                                 .field(Fields::identifier)
                                 .value()
                                 .toString(),
                         u"i"_s);
                QCOMPARE(alternative1.field(Fields::right).field(Fields::value).value().toDouble(),
                         456);
            }
        }
    }

    void returnStatement()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/returnStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"returningFunction\"][0].body.scriptElement");
        QCOMPARE(block.internalKind(), DomType::ScriptBlockStatement);
        QCOMPARE(block.field(Fields::statements).indexes(), 1);
        DomItem conditional = block.field(Fields::statements).index(0);
        DomItem consequence = conditional.field(Fields::consequence);
        QCOMPARE(consequence.internalKind(), DomType::ScriptReturnStatement);
        {
            DomItem returnValue = consequence.field(Fields::expression);
            QCOMPARE(returnValue.internalKind(), DomType::ScriptLiteral);
            QCOMPARE(returnValue.field(Fields::value).value().toDouble(), 123);
        }
        DomItem alternative = conditional.field(Fields::alternative);
        QCOMPARE(alternative.internalKind(), DomType::ScriptReturnStatement);
        {
            DomItem returnValue = alternative.field(Fields::expression);
            QCOMPARE(returnValue.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(returnValue.field(Fields::left).field(Fields::value).value().toDouble(), 1);
            QCOMPARE(returnValue.field(Fields::right).field(Fields::value).value().toDouble(), 2);
        }
    }

    void forStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/forStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement");
        DomItem statements = block.field(Fields::statements);
        DomItem forLoop = statements.index(1);
        {
            // for ( >> let i = 0 << ; i < 100; i = i + 1) {
            DomItem declarationList =
                    forLoop.field(Fields::declarations).field(Fields::declarations);
            QCOMPARE(declarationList.internalKind(), DomType::List);

            QCOMPARE(declarationList.indexes(), 1);
            DomItem declaration = declarationList.index(0);

            QCOMPARE(declaration.internalKind(), DomType::ScriptVariableDeclarationEntry);
            QCOMPARE(declaration.field(Fields::initializer).internalKind(), DomType::ScriptLiteral);

            QCOMPARE(declaration.field(Fields::identifier).value().toString(), "i");
            QCOMPARE(declaration.field(Fields::initializer).field(Fields::value).value().toDouble(),
                     0);
        }
        {
            // for ( let i = 0; >> i < 100 <<; i = i + 1) {
            DomItem condition = forLoop.field(Fields::condition);
            QCOMPARE(condition.internalKind(), DomType::ScriptBinaryExpression);

            QCOMPARE(condition.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(condition.field(Fields::left).field(Fields::identifier).value().toString(),
                     "i");

            QCOMPARE(condition.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(condition.field(Fields::right).field(Fields::value).value().toDouble(), 100);
        }
        {
            // for ( let i = 0; i < 100; >> i = i + 1 << ) {
            DomItem expression = forLoop.field(Fields::expression);
            QCOMPARE(expression.internalKind(), DomType::ScriptBinaryExpression);
            DomItem left = expression.field(Fields::left);
            QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(left.field(Fields::identifier).value().toString(), "i");

            // for ( let i = 0; i < 100; i = >> i + 1 << ) {
            DomItem right = expression.field(Fields::right);
            QCOMPARE(right.internalKind(), DomType::ScriptBinaryExpression);
            DomItem left2 = right.field(Fields::left);
            QCOMPARE(left2.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(left2.field(Fields::identifier).value().toString(), "i");

            DomItem right2 = right.field(Fields::right);
            QCOMPARE(right2.internalKind(), DomType::ScriptLiteral);
            QCOMPARE(right2.field(Fields::value).value().toDouble(), 1);
        }
        {
            // test the body of the for-loop
            DomItem body = forLoop.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            auto blockSemanticScope = body.semanticScope();
            QVERIFY(blockSemanticScope);

            DomItem statementList = body.field(Fields::statements);
            QCOMPARE(statementList.indexes(), 2);
            {
                //  >> sum = sum + 1 <<
                DomItem binaryExpression = statementList.index(0);
                QCOMPARE(binaryExpression.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left = binaryExpression.field(Fields::left);
                QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left.field(Fields::identifier).value().toString(), "sum");

                //  sum = >> sum + 1 <<
                DomItem right = binaryExpression.field(Fields::right);
                QCOMPARE(right.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left2 = right.field(Fields::left);
                QCOMPARE(left2.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left2.field(Fields::identifier).value().toString(), "sum");

                DomItem right2 = right.field(Fields::right);
                QCOMPARE(right2.internalKind(), DomType::ScriptLiteral);
                QCOMPARE(right2.field(Fields::value).value().toDouble(), 1);
            }

            {
                //  >> for (;;) <<
                //         i = 42
                DomItem innerForLoop = statementList.index(1);
                QCOMPARE(innerForLoop.internalKind(), DomType::ScriptForStatement);
                QCOMPARE(innerForLoop.field(Fields::declarations).indexes(), 0);
                QVERIFY(!innerForLoop.field(Fields::initializer));
                QVERIFY(!innerForLoop.field(Fields::condition));
                QVERIFY(!innerForLoop.field(Fields::expression));
                QVERIFY(innerForLoop.field(Fields::body));

                //  for (;;)
                //     >> i = 42 <<
                DomItem expression = innerForLoop.field(Fields::body);
                QCOMPARE(expression.internalKind(), DomType::ScriptBinaryExpression);

                DomItem left = expression.field(Fields::left);
                QCOMPARE(left.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(left.field(Fields::identifier).value().toString(), "i");

                DomItem right = expression.field(Fields::right);
                QCOMPARE(right.internalKind(), DomType::ScriptLiteral);
                QCOMPARE(right.field(Fields::value).value().toDouble(), 42);
            }
        }
    }

    void nullStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/nullStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        DomItem block = rootQmlObject.methods()
                                .key(u"testForNull"_s)
                                .index(0)
                                .field(Fields::body)
                                .field(Fields::scriptElement);

        QVERIFY(block);
        // First for
        DomItem statements = block.field(Fields::statements);
        QCOMPARE(statements.internalKind(), DomType::List);
        QVERIFY(statements.index(0).field(Fields::body).field(Fields::statements).internalKind()
                != DomType::Empty);
        QCOMPARE(statements.index(0).field(Fields::body).field(Fields::statements).length(), 0);

        // Second for
        DomItem secondFor = statements.index(1).field(Fields::body);
        QVERIFY(secondFor.internalKind() == DomType::ScriptIdentifierExpression);
        QCOMPARE(secondFor.field(Fields::identifier).value().toString(), u"x"_s);

        // Empty block
        QVERIFY(statements.index(3).field(Fields::statements).internalKind() != DomType::Empty);
        QCOMPARE(statements.index(3).field(Fields::statements).length(), 0);
    }

    void deconstruction()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/callExpressions.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        DomItem method = rootQmlObject.field(Fields::methods).key(u"deconstruct"_s).index(0);
        DomItem statement = method.field(Fields::body)
                                    .field(Fields::scriptElement)
                                    .field(Fields::statements)
                                    .index(0);
        QCOMPARE(statement.internalKind(), DomType::ScriptVariableDeclaration);
        QCOMPARE(statement.field(Fields::declarations).indexes(), 3);

        {
            DomItem entry = statement.field(Fields::declarations).index(0);
            QVERIFY(!entry.field(Fields::identifier));
            DomItem deconstructedProperty =
                    entry.field(Fields::bindingElement).field(Fields::properties);

            QCOMPARE(deconstructedProperty.indexes(), 1);
            QCOMPARE(deconstructedProperty.index(0).field(Fields::name).value().toString(), u"a"_s);

            DomItem initializer = entry.field(Fields::initializer).field(Fields::properties);
            QCOMPARE(initializer.indexes(), 2);

            DomItem a32 = initializer.index(0);
            QCOMPARE(a32.field(Fields::initializer).value().toInteger(), 32);
            QCOMPARE(a32.field(Fields::name).value().toString(), "a");

            DomItem b42 = initializer.index(1);
            QCOMPARE(b42.field(Fields::initializer).value().toInteger(), 42);
            QCOMPARE(b42.field(Fields::name).value().toString(), "b");
        }
        {
            DomItem entry = statement.field(Fields::declarations).index(1);
            QVERIFY(!entry.field(Fields::identifier));
            DomItem deconstructedProperty =
                    entry.field(Fields::bindingElement).field(Fields::properties);

            QCOMPARE(deconstructedProperty.indexes(), 2);
            QCOMPARE(deconstructedProperty.index(0).field(Fields::name).value().toString(), u"b"_s);
            QCOMPARE(deconstructedProperty.index(1).field(Fields::name).value().toString(), u"c"_s);

            DomItem initializer = entry.field(Fields::initializer).field(Fields::properties);
            QCOMPARE(initializer.indexes(), 2);

            DomItem a32 = initializer.index(0);
            QCOMPARE(a32.field(Fields::initializer).value().toInteger(), 32);
            QCOMPARE(a32.field(Fields::name).value().toString(), "b");

            DomItem b42 = initializer.index(1);
            QCOMPARE(b42.field(Fields::initializer).value().toInteger(), 42);
            QCOMPARE(b42.field(Fields::name).value().toString(), "c");
        }
        {
            DomItem entry = statement.field(Fields::declarations).index(2);
            QVERIFY(!entry.field(Fields::identifier));
            DomItem deconstructedProperty =
                    entry.field(Fields::bindingElement).field(Fields::elements);

            QCOMPARE(deconstructedProperty.indexes(), 3);
            QCOMPARE(deconstructedProperty.index(0).field(Fields::identifier).value().toString(),
                     u"d"_s);
            QCOMPARE(deconstructedProperty.index(1).field(Fields::identifier).value().toString(),
                     u"e"_s);
            QCOMPARE(deconstructedProperty.index(2).field(Fields::identifier).value().toString(),
                     u"f"_s);

            DomItem initializer = entry.field(Fields::initializer).field(Fields::elements);
            QCOMPARE(initializer.indexes(), 3);

            for (int i = 0; i < initializer.indexes(); ++i) {
                QCOMPARE(initializer.index(i).field(Fields::initializer).value().toInteger(),
                         (i + 1) * 111);
            }
        }
        {
            DomItem statement = method.field(Fields::body)
                                        .field(Fields::scriptElement)
                                        .field(Fields::statements)
                                        .index(1);
            DomItem listWithElision = statement.field(Fields::declarations)
                                              .index(0)
                                              .field(Fields::initializer)
                                              .field(Fields::elements);
            const int listElements = 6;
            const int elisionCount = 4;
            QCOMPARE(listWithElision.indexes(), listElements);
            for (int i = 0; i < elisionCount; ++i) {
                QCOMPARE(listWithElision.index(i).internalKind(), DomType::ScriptElision);
            }
            QCOMPARE(listWithElision.index(elisionCount).internalKind(), DomType::ScriptArrayEntry);
            QCOMPARE(listWithElision.index(elisionCount + 1).internalKind(),
                     DomType::ScriptElision);
        }
    }

    void callExpressions()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/callExpressions.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        {
            DomItem p1 = rootQmlObject.path(".bindings[\"p\"][0].value.scriptElement");
            QCOMPARE(p1.internalKind(), DomType::ScriptCallExpression);
            QCOMPARE(p1.field(Fields::callee).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::callee).field(Fields::identifier).value().toString(), "f");
            QCOMPARE(p1.field(Fields::arguments).internalKind(), DomType::List);
            QCOMPARE(p1.field(Fields::arguments).indexes(), 0);
        }

        {
            DomItem p2 = rootQmlObject.path(".bindings[\"p2\"][0].value.scriptElement");
            QCOMPARE(p2.internalKind(), DomType::ScriptCallExpression);
            QCOMPARE(p2.field(Fields::callee).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p2.field(Fields::callee).field(Fields::identifier).value().toString(), "f");

            DomItem p2List = p2.field(Fields::arguments);
            QCOMPARE(p2List.indexes(), 20);
            for (int i = 0; i < p2List.indexes(); ++i) {
                QCOMPARE(p2List.index(i).internalKind(), DomType::ScriptLiteral);
                QCOMPARE(p2List.index(i).field(Fields::value).value().toInteger(), i + 1);
            }
        }

        {
            DomItem p3 = rootQmlObject.path(".bindings[\"p3\"][0].value.scriptElement");
            QCOMPARE(p3.internalKind(), DomType::ScriptCallExpression);
            QCOMPARE(p3.field(Fields::callee).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p3.field(Fields::callee).field(Fields::identifier).value().toString(), "evil");

            DomItem p3List = p3.field(Fields::arguments);
            QCOMPARE(p3List.indexes(), 3);

            DomItem firstArg = p3List.index(0);
            QCOMPARE(firstArg.internalKind(), DomType::ScriptObject);

            {
                DomItem helloWorld = firstArg.field(Fields::properties).index(0);
                QCOMPARE(helloWorld.internalKind(), DomType::ScriptProperty);
                QCOMPARE(helloWorld.field(Fields::initializer).value().toString(), "World");
                QCOMPARE(helloWorld.field(Fields::name).value().toString(), "hello");
            }

            {
                DomItem yyyy = firstArg.field(Fields::properties).index(1);
                QCOMPARE(yyyy.internalKind(), DomType::ScriptProperty);
                QCOMPARE(yyyy.field(Fields::initializer).value().toString(), "yyy");
                QCOMPARE(yyyy.field(Fields::name).value().toString(), "y");
            }

            DomItem secondArg = p3List.index(1);
            QCOMPARE(secondArg.internalKind(), DomType::ScriptArray);
            {
                for (int i = 0; i < 3; ++i) {
                    DomItem current = secondArg.field(Fields::elements).index(i);
                    QCOMPARE(current.internalKind(), DomType::ScriptArrayEntry);
                    QCOMPARE(current.field(Fields::initializer).value().toInteger(), i + 1);
                }
            }

            DomItem thirdArg = p3List.index(2);
            QCOMPARE(thirdArg.internalKind(), DomType::ScriptObject);
            {
                DomItem is = thirdArg.field(Fields::properties).index(0);
                QCOMPARE(is.internalKind(), DomType::ScriptProperty);
                QCOMPARE(is.field(Fields::name).value().toString(), "is");
                DomItem initializer = is.field(Fields::initializer).field(Fields::properties);

                QCOMPARE(initializer.index(0).field(Fields::name).value().toString(), "a");
                QCOMPARE(initializer.index(0).field(Fields::initializer).value().toInteger(), 111);

                QCOMPARE(initializer.index(1).field(Fields::name).value().toString(), "lot");
                QCOMPARE(initializer.index(1).field(Fields::initializer).value().toInteger(), 222);

                QCOMPARE(initializer.index(2).field(Fields::name).value().toString(), "of");
                QCOMPARE(initializer.index(2).field(Fields::initializer).value().toInteger(), 333);

                QCOMPARE(initializer.index(3).field(Fields::name).value().toString(), "fun");
                QCOMPARE(initializer.index(3).field(Fields::initializer).value().toInteger(), 444);

                QCOMPARE(initializer.index(4).field(Fields::name).value().toString(), "really");
                QCOMPARE(initializer.index(4)
                                 .field(Fields::initializer)
                                 .field(Fields::elements)
                                 .index(0)
                                 .field(Fields::initializer)
                                 .value()
                                 .toString(),
                         "!");
            }
        }

        {
            DomItem functionFs = rootQmlObject.field(Fields::methods).key(u"f");
            QCOMPARE(functionFs.indexes(), 1);
            DomItem functionF = functionFs.index(0);

            QVERIFY(!functionF.semanticScope().isNull());
            QVERIFY(functionF.semanticScope()->ownJSIdentifier("helloF").has_value());
            DomItem parameters = functionF.field(Fields::parameters);
            std::vector<QString> parameterNames = {
                u"q"_s, u"w"_s, u"e"_s, u"r"_s, u"t"_s, u"y"_s
            };
            QCOMPARE(parameterNames.size(), (size_t)parameters.indexes());

            for (size_t i = 0; i < parameterNames.size(); ++i) {
                DomItem currentParameter = parameters.index(i);
                QCOMPARE(currentParameter.field(Fields::name).value().toString(),
                         parameterNames[i]);
            }
        }

        {
            DomItem functionFWithDefaults =
                    rootQmlObject.field(Fields::methods).key(u"fWithDefault");
            QCOMPARE(functionFWithDefaults.indexes(), 1);
            DomItem functionFWithDefault = functionFWithDefaults.index(0);
            QVERIFY(!functionFWithDefault.semanticScope().isNull());
            QVERIFY(functionFWithDefault.semanticScope()->ownJSIdentifier(u"helloFWithDefault"_s));
            DomItem parameters = functionFWithDefault.field(Fields::parameters);

            const std::vector<QString> parameterNames = { u"q"_s, u"w"_s, u"e"_s,
                                                          u"r"_s, u"t"_s, u"y"_s };
            const QSet<QString> parameterWithoutDefault = { u"e"_s, u"t"_s };

            QCOMPARE(parameterNames.size(), (size_t)parameters.indexes());

            for (size_t i = 0; i < parameterNames.size(); ++i) {
                DomItem currentParameter = parameters.index(i);
                QCOMPARE(currentParameter.field(Fields::name).value().toString(),
                         parameterNames[i]);

                DomItem scriptElement =
                        currentParameter.field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(scriptElement.internalKind(), DomType::ScriptFormalParameter);
                QCOMPARE(scriptElement.field(Fields::identifier).value().toString(),
                         parameterNames[i]);
                if (!parameterWithoutDefault.contains(parameterNames[i])) {
                    QCOMPARE((size_t)scriptElement.field(Fields::initializer).value().toInteger(),
                             i + 1);
                }
            }
        }

        {
            // note: no need to check the inside of ScriptObject and ScriptArray as those are
            // already tested in deconstruction().
            DomItem method = rootQmlObject.field(Fields::methods).key(u"evil"_s).index(0);
            QVERIFY(!method.semanticScope().isNull());
            QVERIFY(method.semanticScope()->ownJSIdentifier("helloEvil").has_value());
            DomItem parameters = method.field(Fields::parameters);
            QCOMPARE(parameters.indexes(), 3);

            {
                DomItem parameter1 =
                        parameters.index(0).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(parameter1.internalKind(), DomType::ScriptFormalParameter);

                DomItem objectPattern = parameter1.field(Fields::bindingElement);
                QCOMPARE(objectPattern.internalKind(), DomType::ScriptObject);
                QCOMPARE(objectPattern.field(Fields::properties).indexes(), 2);
            }
            {
                DomItem parameter2 =
                        parameters.index(1).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(parameter2.internalKind(), DomType::ScriptFormalParameter);

                DomItem objectPattern = parameter2.field(Fields::bindingElement);
                QCOMPARE(objectPattern.internalKind(), DomType::ScriptArray);
                QCOMPARE(objectPattern.field(Fields::elements).indexes(), 3);
            }
            {
                DomItem parameter3 =
                        parameters.index(2).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(parameter3.internalKind(), DomType::ScriptFormalParameter);

                DomItem objectPattern = parameter3.field(Fields::bindingElement);
                QCOMPARE(objectPattern.internalKind(), DomType::ScriptObject);
                QCOMPARE(objectPattern.field(Fields::properties).indexes(), 3);

                DomItem parameter3DefaultValue = parameter3.field(Fields::initializer);
                QCOMPARE(parameter3DefaultValue.internalKind(), DomType::ScriptObject);

                DomItem objectPatternDefaultValue =
                        parameter3DefaultValue.field(Fields::properties);
                QCOMPARE(objectPatternDefaultValue.indexes(), 3);
            }
        }
        {
            DomItem method = rootQmlObject.field(Fields::methods).key(u"marmelade"_s).index(0);
            QVERIFY(!method.semanticScope().isNull());
            QVERIFY(method.semanticScope()->ownJSIdentifier(u"helloMarmelade"_s));
            DomItem parameters = method.field(Fields::parameters);
            QCOMPARE(parameters.indexes(), 1);
            {
                DomItem parameter1 =
                        parameters.index(0).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(parameter1.internalKind(), DomType::ScriptFormalParameter);
                QCOMPARE(parameter1.field(Fields::identifier).value().toString(), "onTheBread");
            }
        }
        {
            DomItem method = rootQmlObject.field(Fields::methods).key(u"marmelade2"_s).index(0);
            QVERIFY(!method.semanticScope().isNull());
            QVERIFY(method.semanticScope()->ownJSIdentifier(u"helloMarmelade2"_s));
            DomItem parameters = method.field(Fields::parameters);
            QCOMPARE(parameters.indexes(), 3);
            {
                DomItem parameter3 =
                        parameters.index(2).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(parameter3.internalKind(), DomType::ScriptFormalParameter);
                QCOMPARE(parameter3.field(Fields::identifier).value().toString(), "onTheBread");
            }
        }
        {
            DomItem method = rootQmlObject.field(Fields::methods).key(u"withTypes"_s).index(0);
            QVERIFY(!method.semanticScope().isNull());
            QCOMPARE(method.semanticScope()->scopeType(),
                     QQmlJSScope::ScopeType::JSFunctionScope);
            DomItem parameters = method.field(Fields::parameters);
            QCOMPARE(parameters.indexes(), 2);
            QCOMPARE(parameters.index(0)
                             .field(Fields::value)
                             .field(Fields::scriptElement)
                             .field(Fields::type)
                             .field(Fields::typeName)
                             .value()
                             .toString(),
                     "int");
            QCOMPARE(parameters.index(1)
                             .field(Fields::value)
                             .field(Fields::scriptElement)
                             .field(Fields::type)
                             .field(Fields::typeName)
                             .value()
                             .toString(),
                     "MyType");
        }
        {
            DomItem method = rootQmlObject.field(Fields::methods).key(u"empty"_s).index(0);
            QVERIFY(!method.semanticScope().isNull());
            QCOMPARE(method.semanticScope()->scopeType(),
                     QQmlJSScope::ScopeType::JSFunctionScope);
        }
        {
            DomItem method = rootQmlObject.field(Fields::methods).key(u"mySignal"_s).index(0);
            // signals contain the scope of the QML object owning them
            QVERIFY(!method.semanticScope().isNull());
            QCOMPARE(method.semanticScope()->scopeType(), QQmlJSScope::ScopeType::QMLScope);
        }
    }

    void fieldMemberExpression()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/fieldMemberExpression.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        {
            DomItem p1 = rootQmlObject.path(".bindings[\"p1\"][0].value.scriptElement");
            QCOMPARE(p1.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::identifier).value().toString(), "p");
        }

        {
            DomItem p1Qualified =
                    rootQmlObject.path(".bindings[\"p1Qualified\"][0].value.scriptElement");
            fieldMemberExpressionHelper(p1Qualified, QStringList{ u"p"_s });
            QCOMPARE(p1Qualified.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(p1Qualified.field(Fields::left).field(Fields::identifier).value().toString(),
                     "root");
        }
        {
            // property var p1Bracket: root["p"]
            DomItem p1 = rootQmlObject.path(".bindings[\"p1Bracket\"][0].value.scriptElement");
            QCOMPARE(p1.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(p1.field(Fields::operation).value().toInteger(),
                     ScriptElements::BinaryExpression::ArrayMemberAccess);

            QCOMPARE(p1.field(Fields::left).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::left).field(Fields::identifier).value().toString(), "root");

            QCOMPARE(p1.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(p1.field(Fields::right).field(Fields::value).value().toString(), "p");
        }
        {
            // property var p1Index: root[42]
            DomItem p1 = rootQmlObject.path(".bindings[\"p1Index\"][0].value.scriptElement");
            QCOMPARE(p1.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(p1.field(Fields::operation).value().toInteger(),
                     ScriptElements::BinaryExpression::ArrayMemberAccess);

            QCOMPARE(p1.field(Fields::left).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::left).field(Fields::identifier).value().toString(), "root");

            QCOMPARE(p1.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(p1.field(Fields::right).field(Fields::value).value().toInteger(), 42);
        }
        {
            // property var p1Key: root[p]
            DomItem p1 = rootQmlObject.path(".bindings[\"p1Key\"][0].value.scriptElement");
            QCOMPARE(p1.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(p1.field(Fields::operation).value().toInteger(),
                     ScriptElements::BinaryExpression::ArrayMemberAccess);

            QCOMPARE(p1.field(Fields::left).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::left).field(Fields::identifier).value().toString(), "root");

            QCOMPARE(p1.field(Fields::right).internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p1.field(Fields::right).field(Fields::identifier).value().toString(), "p");
        }
        {
            DomItem p1Qualified =
                    rootQmlObject.path(".bindings[\"p1Qualified\"][0].value.scriptElement");
            fieldMemberExpressionHelper(p1Qualified, QStringList{ u"p"_s });
            QCOMPARE(p1Qualified.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(p1Qualified.field(Fields::left).field(Fields::identifier).value().toString(),
                     "root");
        }

        {
            DomItem p3 = rootQmlObject.path(".bindings[\"p3\"][0].value.scriptElement");
            fieldMemberExpressionHelper(p3, QStringList{ u"property2"_s, u"p"_s });
            DomItem p3Left = p3.field(Fields::left).field(Fields::left);
            QCOMPARE(p3Left.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p3Left.field(Fields::identifier).value().toString(), "property1");
        }

        {
            DomItem p3Qualified =
                    rootQmlObject.path(".bindings[\"p3Qualified\"][0].value.scriptElement");
            fieldMemberExpressionHelper(p3Qualified,
                                        QStringList{ u"property1"_s, u"property2"_s, u"p"_s });
            DomItem p3Left =
                    p3Qualified.field(Fields::left).field(Fields::left).field(Fields::left);
            QCOMPARE(p3Left.internalKind(), DomType::ScriptIdentifierExpression);
            QCOMPARE(p3Left.field(Fields::identifier).value().toString(), "root");
        }
    }

    void switchStatement()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/switchStatement.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        QVERIFY(rootQmlObject);

        {
            DomItem statements = rootQmlObject.methods()
                                         .key(u"switchStatement")
                                         .index(0)
                                         .field(Fields::body)
                                         .field(Fields::scriptElement)
                                         .field(Fields::statements);
            QVERIFY(statements);
            QCOMPARE(statements.index(0).internalKind(), DomType::ScriptVariableDeclaration);
            QCOMPARE(statements.index(1).internalKind(), DomType::ScriptVariableDeclaration);

            DomItem firstSwitch = statements.index(2);
            QVERIFY(firstSwitch);
            QCOMPARE(firstSwitch.internalKind(), DomType::ScriptSwitchStatement);
            QCOMPARE(firstSwitch.field(Fields::caseBlock).internalKind(), DomType::ScriptCaseBlock);
            QCOMPARE(firstSwitch.field(Fields::expression).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(firstSwitch.field(Fields::expression)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"animals");

            DomItem caseClauses = firstSwitch.field(Fields::caseBlock).field(Fields::caseClauses);
            QVERIFY(caseClauses);
            QCOMPARE(caseClauses.index(0).internalKind(), DomType::ScriptCaseClause);
            QCOMPARE(caseClauses.index(0).field(Fields::expression).internalKind(),
                     DomType::ScriptLiteral);
            QCOMPARE(caseClauses.index(0).field(Fields::expression).value().toString(), u"cat");

            DomItem innerSwitch = caseClauses.index(0).field(Fields::statements).index(0);
            QVERIFY(innerSwitch);
            QCOMPARE(innerSwitch.internalKind(), DomType::ScriptSwitchStatement);
            QCOMPARE(innerSwitch.field(Fields::expression).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(innerSwitch.field(Fields::expression).value().toString(), u"no");

            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(0)
                             .internalKind(),
                     DomType::ScriptCaseClause);
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(0)
                             .field(Fields::expression)
                             .internalKind(),
                     DomType::ScriptLiteral);
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(0)
                             .field(Fields::expression)
                             .value()
                             .toInteger(),
                     0);
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(0)
                             .field(Fields::statements)
                             .index(0)
                             .field(Fields::expression)
                             .value()
                             .toString(),
                     u"patron");
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(1)
                             .internalKind(),
                     DomType::ScriptCaseClause);
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::caseClauses)
                             .index(1)
                             .field(Fields::statements)
                             .index(0)
                             .field(Fields::expression)
                             .value()
                             .toString(),
                     u"mafik");
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::defaultClause)
                             .internalKind(),
                     DomType::ScriptDefaultClause);
            QCOMPARE(innerSwitch.field(Fields::caseBlock)
                             .field(Fields::defaultClause)
                             .field(Fields::statements)
                             .index(0)
                             .field(Fields::expression)
                             .value()
                             .toString(),
                     u"none");

            // case "dog"
            DomItem caseDogBlock = firstSwitch.field(Fields::caseBlock)
                                           .field(Fields::caseClauses)
                                           .index(1)
                                           .field(Fields::statements)
                                           .index(0);
            QVERIFY(caseDogBlock);
            // Check if semantic scope is correctly created for the CaseClause
            auto blockSemanticScope = caseDogBlock.semanticScope();
            QVERIFY(blockSemanticScope);
            QVERIFY(blockSemanticScope->ownJSIdentifier(u"name"_s));
            QVERIFY(blockSemanticScope->ownJSIdentifier(u"another"_s));

            // Default clause
            DomItem defaultClause =
                    firstSwitch.field(Fields::caseBlock).field(Fields::defaultClause);
            QVERIFY(defaultClause);
            QCOMPARE(defaultClause.internalKind(), DomType::ScriptDefaultClause);
            QCOMPARE(defaultClause.field(Fields::statements).index(0).internalKind(),
                     DomType::ScriptReturnStatement);
            QCOMPARE(defaultClause.field(Fields::statements)
                             .index(0)
                             .field(Fields::expression)
                             .internalKind(),
                     DomType::ScriptLiteral);
            QCOMPARE(defaultClause.field(Fields::statements)
                             .index(0)
                             .field(Fields::expression)
                             .value()
                             .toString(),
                     u"monster");

            // case "moreCases!"
            const DomItem moreCases = firstSwitch.field(Fields::caseBlock)
                                            .field(Fields::moreCaseClauses)
                                            .index(0)
                                            .field(Fields::statements)
                                            .index(0);

            QCOMPARE(moreCases.internalKind(),
                     DomType::ScriptReturnStatement);
            QCOMPARE(moreCases.field(Fields::expression).value().toString(), u"moreCaseClauses?");
        }
    }

    void iterationStatements()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/iterationStatements.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        QVERIFY(rootQmlObject);
        DomItem methods = rootQmlObject.methods();
        QVERIFY(methods);

        {
            // while statement
            DomItem whileTest = methods.key("whileStatement")
                                        .index(0)
                                        .field(Fields::body)
                                        .field(Fields::scriptElement)
                                        .field(Fields::statements)
                                        .index(1);
            QVERIFY(whileTest);
            QCOMPARE(whileTest.internalKind(), DomType::ScriptWhileStatement);
            auto whileScope = whileTest.semanticScope();
            QVERIFY(whileScope);
            DomItem whileBody = whileTest.field(Fields::body);
            QVERIFY(whileBody);
            QCOMPARE(whileBody.internalKind(), DomType::ScriptBlockStatement);
            auto scope = whileBody.semanticScope();
            QVERIFY(scope);
            QCOMPARE(whileBody.field(Fields::statements).index(0).internalKind(),
                     DomType::ScriptBinaryExpression); // i = i - 1
            QCOMPARE(
                    whileBody.field(Fields::statements).index(0).field(Fields::left).internalKind(),
                    DomType::ScriptIdentifierExpression);
            QCOMPARE(whileBody.field(Fields::statements)
                             .index(0)
                             .field(Fields::left)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"i");
            DomItem whileExpression = whileTest.field(Fields::expression);
            QVERIFY(whileExpression);
            QCOMPARE(whileExpression.internalKind(), DomType::ScriptBinaryExpression); // i > 0
            QCOMPARE(whileExpression.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(whileExpression.field(Fields::left)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"i");
            QCOMPARE(whileExpression.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(whileExpression.field(Fields::right)
                             .field(Fields::identifier)
                             .value()
                             .toInteger(),
                     0);

            DomItem singleLineWhile = methods.key("whileStatement")
                                              .index(0)
                                              .field(Fields::body)
                                              .field(Fields::scriptElement)
                                              .field(Fields::statements)
                                              .index(2);
            QVERIFY(singleLineWhile);
            QCOMPARE(singleLineWhile.internalKind(), DomType::ScriptWhileStatement);
            DomItem singleLineWhileBody = singleLineWhile.field(Fields::body);
            QVERIFY(singleLineWhileBody);
            QCOMPARE(singleLineWhileBody.internalKind(), DomType::ScriptBinaryExpression);
            auto singleLineWhileScope = singleLineWhile.semanticScope();
            QVERIFY(singleLineWhileScope);
            QVERIFY(singleLineWhileScope->jsIdentifier("i")); // i is in the parent scope
        }

        {
            // do-while statement
            DomItem doWhile = methods.key("doWhile")
                                      .index(0)
                                      .field(Fields::body)
                                      .field(Fields::scriptElement)
                                      .field(Fields::statements)
                                      .index(1);
            QVERIFY(doWhile);
            QCOMPARE(doWhile.internalKind(), DomType::ScriptDoWhileStatement);

            auto doWhileScope = doWhile.semanticScope();
            QVERIFY(doWhileScope);
            DomItem doWhileBody = doWhile.field(Fields::body);
            QVERIFY(doWhileBody);
            auto doWhileBodyScope = doWhileBody.semanticScope();
            QVERIFY(doWhileBodyScope);
            QVERIFY(doWhileBodyScope->ownJSIdentifier("b")); // const b = a
            QCOMPARE(doWhileBody.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(doWhileBody.field(Fields::statements).index(0).internalKind(),
                     DomType::ScriptVariableDeclaration); // const b = a
            QCOMPARE(doWhileBody.field(Fields::statements).index(1).internalKind(),
                     DomType::ScriptBinaryExpression); // a = a - 1

            DomItem doWhileExpression = doWhile.field(Fields::expression);
            QVERIFY(doWhileExpression);
            QCOMPARE(doWhileExpression.internalKind(), DomType::ScriptBinaryExpression); // a > 0
            QCOMPARE(doWhileExpression.field(Fields::left).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(doWhileExpression.field(Fields::left)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"a");
            QCOMPARE(doWhileExpression.field(Fields::right).internalKind(), DomType::ScriptLiteral);
            QCOMPARE(doWhileExpression.field(Fields::right)
                             .field(Fields::identifier)
                             .value()
                             .toInteger(),
                     0);
            DomItem singleDoWhile = methods.key("doWhile")
                                            .index(0)
                                            .field(Fields::body)
                                            .field(Fields::scriptElement)
                                            .field(Fields::statements)
                                            .index(2);
            auto singleDoWhileScope = singleDoWhile.semanticScope();
            QVERIFY(singleDoWhileScope);
            QVERIFY(singleDoWhileScope->jsIdentifier("a")); // a = a - 1
        }

        {
            // for..of
            DomItem statements = methods.key("forOf")
                                         .index(0)
                                         .field(Fields::body)
                                         .field(Fields::scriptElement)
                                         .field(Fields::statements);
            QVERIFY(statements);
            QCOMPARE(statements.index(0).internalKind(), DomType::ScriptVariableDeclaration);
            DomItem outerForEach = statements.index(1);
            QVERIFY(outerForEach);
            QCOMPARE(outerForEach.internalKind(), DomType::ScriptForEachStatement);
            DomItem bindingElements =
                    outerForEach.field(Fields::bindingElement).field(Fields::bindingElement);
            QVERIFY(bindingElements);
            QCOMPARE(bindingElements.internalKind(), DomType::ScriptArray);
            QCOMPARE(bindingElements.field(Fields::elements).length(), 2);
            QCOMPARE(bindingElements.field(Fields::elements)
                             .index(1)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "b");
            QCOMPARE(outerForEach.field(Fields::expression).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(outerForEach.field(Fields::expression)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"iterable");
            DomItem forEachStatements = outerForEach.field(Fields::body).field(Fields::statements);
            QCOMPARE(forEachStatements.index(0).internalKind(), DomType::ScriptVariableDeclaration);
            QCOMPARE(forEachStatements.index(1).internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(forEachStatements.index(2).internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(forEachStatements.index(3).internalKind(), DomType::ScriptForEachStatement);
            DomItem firstForEach = forEachStatements.index(1);
            QVERIFY(firstForEach);
            DomItem bindingElement =
                    firstForEach.field(Fields::bindingElement).field(Fields::bindingElement);
            QCOMPARE(bindingElement.internalKind(), DomType::ScriptArray);

            QCOMPARE(bindingElement.field(Fields::elements).length(), 4);
            QCOMPARE(bindingElement.field(Fields::elements)
                             .index(0)
                             .field(Fields::identifier)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "a1");
            DomItem secondForEach = forEachStatements.index(2);
            QCOMPARE(secondForEach.field(Fields::bindingElement).internalKind(),
                     DomType::ScriptPattern);
            QCOMPARE(secondForEach.field(Fields::bindingElement)
                             .field(Fields::identifier)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "k");
            QCOMPARE(secondForEach.internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(secondForEach.field(Fields::expression).internalKind(), DomType::ScriptArray);
            QVERIFY(secondForEach.field(Fields::body));
            QCOMPARE(secondForEach.field(Fields::body).internalKind(),
                     DomType::ScriptBlockStatement);
            DomItem thirdForEach = forEachStatements.index(3);
            QCOMPARE(thirdForEach.field(Fields::bindingElement).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(thirdForEach.field(Fields::bindingElement).value().toString(), "t");

            QCOMPARE(thirdForEach.internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(thirdForEach.field(Fields::expression).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(thirdForEach.field(Fields::expression)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"a");
            QVERIFY(thirdForEach.field(Fields::body));
            QCOMPARE(thirdForEach.field(Fields::body).internalKind(),
                     DomType::ScriptBlockStatement);

            DomItem forthForEach = forEachStatements.index(3);
            QVERIFY(forthForEach);
            auto forthForEachScope = forthForEach.semanticScope();
            QVERIFY(forthForEachScope);
            QVERIFY(forthForEachScope->jsIdentifier("t")); // t lives in parent scope
        }

        {
            // for..in
            DomItem statements = methods.key("forIn")
                                         .index(0)
                                         .field(Fields::body)
                                         .field(Fields::scriptElement)
                                         .field(Fields::statements);
            QVERIFY(statements);
            QCOMPARE(statements.index(0).internalKind(), DomType::ScriptVariableDeclaration);
            DomItem outerForEach = statements.index(1);
            QVERIFY(outerForEach);
            auto outerForEachScope = outerForEach.semanticScope();
            QVERIFY(outerForEachScope);
            QVERIFY(outerForEachScope->jsIdentifier("a")); // var [a,b,c,d]
            QVERIFY(outerForEachScope->jsIdentifier("b"));
            QVERIFY(outerForEachScope->jsIdentifier("c"));
            QVERIFY(outerForEachScope->jsIdentifier("d"));
            QCOMPARE(outerForEach.internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(statements.index(1).internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(outerForEach.field(Fields::expression)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "enumerable");
            auto outerForEachBodyScope = outerForEach.field(Fields::body).semanticScope();
            QVERIFY(outerForEachBodyScope);
            QVERIFY(outerForEachBodyScope->ownJSIdentifier("t")); // let t
            DomItem forInStatements = outerForEach.field(Fields::body).field(Fields::statements);
            QCOMPARE(forInStatements.index(0).internalKind(), DomType::ScriptVariableDeclaration);
            QCOMPARE(forInStatements.index(1).internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(forInStatements.index(2).internalKind(), DomType::ScriptForEachStatement);
            DomItem firstForEach = forInStatements.index(1);
            QVERIFY(firstForEach);
            auto firstForEachScope = firstForEach.semanticScope();
            QVERIFY(firstForEachScope);
            QVERIFY(firstForEachScope->jsIdentifier("t"));
            QCOMPARE(firstForEach.field(Fields::bindingElement).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(firstForEach.field(Fields::bindingElement)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "t");
            QCOMPARE(firstForEach.internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(firstForEach.field(Fields::expression).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(firstForEach.field(Fields::expression)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "enumerable");
            QVERIFY(firstForEach.field(Fields::body));
            QCOMPARE(firstForEach.field(Fields::body).internalKind(),
                     DomType::ScriptBlockStatement);

            DomItem secondForEach = forInStatements.index(2);
            QVERIFY(secondForEach);
            auto secondForEachScope = secondForEach.semanticScope();
            QVERIFY(secondForEachScope);
            QVERIFY(secondForEachScope->ownJSIdentifier("a1")); // const [a1,,a2,...rest]
            QVERIFY(secondForEachScope->ownJSIdentifier("a2"));
            QVERIFY(secondForEachScope->ownJSIdentifier("rest"));

            DomItem bindingElement =
                    secondForEach.field(Fields::bindingElement).field(Fields::bindingElement);
            QCOMPARE(bindingElement.internalKind(), DomType::ScriptArray);
            QCOMPARE(bindingElement.field(Fields::elements)
                             .index(3)
                             .field(Fields::identifier)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     "rest");
            QCOMPARE(secondForEach.internalKind(), DomType::ScriptForEachStatement);
            QCOMPARE(secondForEach.field(Fields::expression).internalKind(),
                     DomType::ScriptBinaryExpression);
            QVERIFY(secondForEach.field(Fields::body));
            QCOMPARE(secondForEach.field(Fields::body).internalKind(),
                     DomType::ScriptBlockStatement);
            DomItem thirdForEach = forInStatements.index(3);
            QVERIFY(thirdForEach);
            auto thirdForEachScope = thirdForEach.semanticScope();
            QVERIFY(thirdForEachScope);
            QVERIFY(thirdForEachScope->ownJSIdentifier("t"));
        }
    }

    void doNotCrashEcmaScriptClass()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/ecmaScriptClass.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        QVERIFY(rootQmlObject);
    }

    void bindingAttachedOrGroupedProperties()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/attachedOrGroupedProperties.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        QVERIFY(rootQmlObject);

        DomItem dotNotation = rootQmlObject.path(u".children[0].bindings[\"grouped.font.family\"][0].bindingIdentifiers");
        QVERIFY(dotNotation);
        QCOMPARE(dotNotation.internalKind(), DomType::ScriptBinaryExpression);
        QCOMPARE(dotNotation.field(Fields::left).internalKind(), DomType::ScriptBinaryExpression);
        QCOMPARE(dotNotation.field(Fields::right).field(Fields::identifier).value().toString(), u"family");
        QCOMPARE(dotNotation.field(Fields::right).internalKind(), DomType::ScriptIdentifierExpression);
        QCOMPARE(dotNotation.field(Fields::left).field(Fields::left).internalKind(), DomType::ScriptIdentifierExpression);
        QCOMPARE(dotNotation.field(Fields::left).field(Fields::left).field(Fields::identifier).value().toString(), u"grouped");
        QCOMPARE(dotNotation.field(Fields::left).field(Fields::right).internalKind(), DomType::ScriptIdentifierExpression);
        QCOMPARE(dotNotation.field(Fields::left).field(Fields::right).field(Fields::identifier).value().toString(), u"font");
        auto dotNotationScope = dotNotation.semanticScope();
        QVERIFY(!dotNotationScope);

        DomItem groupNotationChild1 = rootQmlObject.path(u".children[1].children[0]");
        QVERIFY(groupNotationChild1);
        QCOMPARE(groupNotationChild1.internalKind(), DomType::QmlObject);
        QCOMPARE(groupNotationChild1.field(Fields::name).value().toString(), u"myText");
        auto myTextScope = groupNotationChild1.semanticScope();
        QVERIFY(myTextScope);
        QCOMPARE(myTextScope->scopeType(), QQmlJSScope::ScopeType::GroupedPropertyScope);
        QVERIFY(myTextScope->hasProperty("font"));

        DomItem groupNotationChild2 = groupNotationChild1.path(u".children[0]");
        QCOMPARE(groupNotationChild2.internalKind(), DomType::QmlObject);
        QCOMPARE(groupNotationChild2.field(Fields::name).value().toString(), u"font");

        auto fontScope = groupNotationChild2.semanticScope();
        QVERIFY(fontScope);
        QCOMPARE(fontScope->scopeType(), QQmlJSScope::ScopeType::GroupedPropertyScope);
        QVERIFY(fontScope->hasProperty("pixelSize"));

        DomItem pixelSize = groupNotationChild2.path(u".bindings[\"pixelSize\"][0].bindingIdentifiers");
        QCOMPARE(pixelSize.internalKind(), DomType::ScriptIdentifierExpression);
        QCOMPARE(pixelSize.field(Fields::identifier).value().toString(), u"pixelSize");

        DomItem attached = rootQmlObject.path(u".bindings[\"Keys.onPressed\"][0].bindingIdentifiers");
        QVERIFY(attached);
        QCOMPARE(attached.internalKind(), DomType::ScriptBinaryExpression);
        QCOMPARE(attached.field(Fields::left).field(Fields::identifier).value().toString(), u"Keys");
        QCOMPARE(attached.field(Fields::right).field(Fields::identifier).value().toString(), u"onPressed");
    }

    void enumDeclarations()
    {
        using namespace Qt::StringLiterals;
        QString testFile = baseDir + u"/enumDeclarations.qml"_s;
        DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        QVERIFY(fileObject);
        DomItem enums = fileObject.path(u".components[\"\"][0].enumerations");
        QVERIFY(enums);

        DomItem catsEnum = enums.key("Cats").index(0);
        QVERIFY(catsEnum);
        QCOMPARE(catsEnum.internalKind(), DomType::EnumDecl);
        QCOMPARE(catsEnum.name(), u"Cats");

        auto values = catsEnum.field(Fields::values);
        QCOMPARE(values.length(), 3);
        QCOMPARE(values.index(0).internalKind(), DomType::EnumItem);
        QCOMPARE(values.index(0).name(), u"Patron");
        QCOMPARE(values.index(0).field(Fields::value).value().toInteger(), 0);
        QCOMPARE(values.index(1).internalKind(), DomType::EnumItem);
        QCOMPARE(values.index(1).name(), u"Mafya");
        QCOMPARE(values.index(1).field(Fields::value).value().toInteger(), 1);
        QCOMPARE(values.index(2).internalKind(), DomType::EnumItem);
        QCOMPARE(values.index(2).name(), u"Kivrik");
        QCOMPARE(values.index(2).field(Fields::value).value().toInteger(), -1);
    }

    void tryStatements()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/tryStatements.qml"_s;
        const DomItem root = rootQmlObjectFromFile(testFile, qmltypeDirs);
        QVERIFY(root);
        const DomItem statements = root.path(u".methods[\"f\"][0].body.scriptElement.statements");
        QCOMPARE(statements.indexes(), 3);

        // test the try blocks
        for (int i = 0; i < 3; ++i) {
            const DomItem statement = statements.index(i).field(Fields::block);
            QVERIFY(statement);
            QCOMPARE(statement.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(statement.field(Fields::statements)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"insideTry"_s);
        }

        // test the catch blocks
        for (int i = 0; i < 3; ++i) {
            const DomItem statement = statements.index(i).field(Fields::catchBlock);
            if (i == 2) {
                QVERIFY(!statement); // no catch in last statement
                continue;
            }

            QVERIFY(statement);
            QCOMPARE(statement.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(statement.field(Fields::statements)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"insideCatch"_s);

            const DomItem expression = statements.index(i).field(Fields::catchParameter);
            QVERIFY(expression);
            QCOMPARE(expression.field(Fields::identifier)
                             .value()
                             .toString(),
                     u"catchExpression"_s);
        }

        // test the finally blocks
        for (int i = 0; i < 3; ++i) {
            const DomItem statement = statements.index(i).field(Fields::finallyBlock);
            if (i == 1) {
                QVERIFY(!statement); // no finally in last statement
                continue;
            }

            QVERIFY(statement);
            QCOMPARE(statement.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(statement.field(Fields::statements)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"insideFinally"_s);
        }
    }

    void plainJSDOM_data()
    {
        QTest::addColumn<QString>("filename");
        QTest::addColumn<QString>("content");

        QTest::newRow("simplestJSStatement")
                << "simplestJSStatement.js" << QString(u"let v=1;\n"_s);
        QTest::newRow("import")
                << "import.js"
                << QString(u".import \"main.js\" as Main\nconsole.log(Main.a);\n"_s);
        QTest::newRow("simplestJSmodule")
                << "simplestJSmodule.mjs" << QString(u"export function entry() {}\n"_s);
    }

    // Verifies that DOM can load .js and .mjs files and
    // parse / store the content inside the ScriptExpression
    void plainJSDOM()
    {
        using namespace Qt::StringLiterals;
        QFETCH(QString, filename);
        QFETCH(QString, content);

        QString testFile = baseDir + "/" + filename;
        auto dom = parse(testFile, qmltypeDirs);
        QVERIFY(dom);
        QCOMPARE(dom.internalKind(), DomType::JsFile);
        auto filePtr = dom.fileObject().ownerAs<JsFile>();
        QVERIFY(filePtr && filePtr->isValid());
        auto exprAsString = dom.field(Fields::expression)
                               .field(Fields::code)
                               .value()
                               .toString();
        QVERIFY(!exprAsString.isEmpty());
        exprAsString.replace("\r\n", "\n");
        QCOMPARE(exprAsString, content);
    }

private:
    struct DomItemWithLocation
    {
        DomItem item;
        FileLocations::Tree tree;
    };
private slots:

    void fileLocations_data()
    {
        QTest::addColumn<QString>("fileName");
        QDir dir(baseDir);
        for (const QString &file : dir.entryList(QDir::Files, QDir::Name)) {
            if (!file.endsWith(".qml"))
                continue;
            QTest::addRow("%s", file.toStdString().c_str()) << baseDir + QDir::separator() + file;
        }
    }

    /*!
       \internal
        Check if finalizeScriptExpression() was called with the correct FileLocations::Tree
        argument when this test fails.
     */
    void fileLocations()
    {
        QFETCH(QString, fileName);

        DomItem rootQmlObject = rootQmlObjectFromFile(fileName, qmltypeDirs);
        std::deque<DomItemWithLocation> queue;

        DomItem fileDomItem = rootQmlObject.containingFile();
        std::shared_ptr<QmlFile> file = fileDomItem.ownerAs<QmlFile>();
        QVERIFY(file);

        DomItemWithLocation root{ fileDomItem, file->fileLocationsTree() };
        queue.push_back(root);

        while (!queue.empty()) {
            DomItemWithLocation current = queue.front();
            queue.pop_front();

            auto subEls = current.tree->subItems();
            for (auto it = subEls.begin(); it != subEls.end(); ++it) {
                DomItem childItem = current.item.path(it.key());
                FileLocations::Tree childTree = it.value();
                if (!childItem) {
                    const auto itemFLoc = FileLocations::treeOf(current.item);
                    qDebug() << current.item.internalKindStr()
                             << "has incorrect FileLocations! Make sure that "
                                "finalizeScriptExpression is called with the right arguments. It "
                                "should print out some debugging information with the "
                                "qt.qmldom.astcreator.debug logging rule.";
                    qDebug() << "Current FileLocations has keys:"
                             << itemFLoc->subItems().keys()
                             << "but current Item of type" << current.item.internalKindStr()
                             << "has fields: " << current.item.fields()
                             << "and keys: " << current.item.keys()
                             << "and indexes: " << current.item.indexes();
                }
                QVERIFY(childItem.internalKind() != DomType::Empty);
                queue.push_back({ childItem, childTree });
            }
        }
    }

private slots:
    void finalizeScriptExpressions()
    {
        QString fileName = baseDir + u"/finalizeScriptExpressions.qml"_s;
        DomItem rootQmlObject = rootQmlObjectFromFile(fileName, qmltypeDirs);

        /*
           Check if the path obtained by the filelocations is the same as the DomItem path. Both
           need to be equal to find DomItem's from their location in the source code.
        */
        auto compareFileLocationsPathWithCanonicalPath = [](const DomItem &item) {
            Path canonical = item.canonicalPath();
            QVERIFY(canonical.length() > 0);
            if (canonical.length() > 0)
                QCOMPARE(canonical.toString(),
                         FileLocations::canonicalPathForTesting(FileLocations::treeOf(item)));
        };

        /*
        for all places, where scriptelements are attached to Qml elements in the Dom, check if:
            a) scriptelement is accessible from the DomItem (is it correclty attached?)
            b) scriptelement has the correct path (is its pathFromOwner the path where it was
        attached?)

        For bindings to objects, arrays and scripts, check that the bindingIdentifiers are correctly
        attached in the Dom.
        */

        {
            DomItem binding = rootQmlObject.field(Fields::bindings).key("binding");
            QCOMPARE(binding.indexes(), 1);

            QCOMPARE(binding.index(0).field(Fields::value).internalKind(),
                     DomType::ScriptExpression);
            QCOMPARE(binding.index(0)
                             .field(Fields::value)
                             .field(Fields::scriptElement)
                             .internalKind(),
                     DomType::ScriptLiteral);
            // Fields::value is in the path of the owner, and therefore should not be in
            // pathFromOwner!
            DomItem scriptElement =
                    binding.index(0).field(Fields::value).field(Fields::scriptElement);
            QCOMPARE(scriptElement.pathFromOwner(), Path().withField(Fields::scriptElement));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
        }

        {
            DomItem bindingInPropertyDefinition =
                    rootQmlObject.field(Fields::bindings).key("bindingInPropertyDefinition");
            QCOMPARE(bindingInPropertyDefinition.indexes(), 1);

            QCOMPARE(bindingInPropertyDefinition.index(0).field(Fields::value).internalKind(),
                     DomType::ScriptExpression);
            QCOMPARE(bindingInPropertyDefinition.index(0)
                             .field(Fields::value)
                             .field(Fields::scriptElement)
                             .internalKind(),
                     DomType::ScriptLiteral);
            // Fields::value is in the path of the owner, and therefore should not be in
            // pathFromOwner!
            DomItem scriptElement = bindingInPropertyDefinition.index(0)
                                            .field(Fields::value)
                                            .field(Fields::scriptElement);
            QCOMPARE(scriptElement.pathFromOwner(), Path().withField(Fields::scriptElement));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
        }
        // check the parameters + returnType of the method
        {
            DomItem return42 = rootQmlObject.field(Fields::methods).key("return42");
            QCOMPARE(return42.indexes(), 1);

            DomItem typeAnnotation =
                    return42.index(0).field(Fields::returnType).field(Fields::scriptElement);
            QCOMPARE(typeAnnotation.internalKind(), DomType::ScriptType);
            compareFileLocationsPathWithCanonicalPath(typeAnnotation);

            DomItem parameters = return42.index(0).field(Fields::parameters);
            QCOMPARE(parameters.indexes(), 3);
            for (int i = 0; i < 3; ++i) {
                QCOMPARE(parameters.index(i).field(Fields::defaultValue).internalKind(),
                         DomType::ScriptExpression);
                DomItem scriptElement =
                        parameters.index(i).field(Fields::value).field(Fields::scriptElement);
                QCOMPARE(scriptElement.internalKind(), DomType::ScriptFormalParameter);
                QCOMPARE(scriptElement.pathFromOwner(), Path().withField(Fields::scriptElement));
                compareFileLocationsPathWithCanonicalPath(scriptElement);
            }
        }
        // check the body of the methods
        QList<QString> methodNames = { "full", "return42" };
        for (QString &methodName : methodNames) {
            DomItem method = rootQmlObject.field(Fields::methods).key(methodName);
            DomItem body = method.index(0).field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptExpression);
            DomItem scriptElement = body.field(Fields::scriptElement);
            QCOMPARE(scriptElement.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(scriptElement.pathFromOwner(), Path().withField(Fields::scriptElement));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
        }

        {
            DomItem binding = rootQmlObject.field(Fields::bindings).key("arrayBinding");
            QCOMPARE(binding.indexes(), 1);
            QCOMPARE(binding.index(0).field(Fields::value).internalKind(),
                     DomType::ScriptExpression);
            QCOMPARE(binding.index(0)
                             .field(Fields::value)
                             .field(Fields::scriptElement)
                             .internalKind(),
                     DomType::ScriptArray);
            // Fields::value is in the path of the owner, and therefore should not be in
            // pathFromOwner!
            DomItem scriptElement =
                    binding.index(0).field(Fields::value).field(Fields::scriptElement);
            QCOMPARE(scriptElement.pathFromOwner(), Path().withField(Fields::scriptElement));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
            // also check that the left hand side of the binding is correctly attached to the Dom:
            scriptElement = binding.index(0).field(Fields::bindingIdentifiers);
            QCOMPARE(scriptElement.pathFromOwner(),
                     Path::fromString(u".components[\"\"][0].objects[0].bindings[\"arrayBinding\"]["
                                      u"0].bindingIdentifiers"));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
        }
        {
            DomItem binding = rootQmlObject.field(Fields::bindings).key("objectBinding");
            QCOMPARE(binding.indexes(), 1);
            QCOMPARE(binding.index(0).field(Fields::value).internalKind(), DomType::QmlObject);
            // check that the left hand side of the binding is correctly attached to the Dom:
            DomItem scriptElement = binding.index(0).field(Fields::bindingIdentifiers);
            QCOMPARE(scriptElement.pathFromOwner(),
                     Path::fromString(u".components[\"\"][0].objects[0].bindings[\"objectBinding\"]["
                                      u"0].bindingIdentifiers"));
            compareFileLocationsPathWithCanonicalPath(scriptElement);
        }
    }

    void goToFile()
    {
        using namespace Qt::StringLiterals;
        const QString filePathA = baseDir + u"/nullStatements.qml"_s;
        const QString filePathB = baseDir + u"/propertyBindings.qml"_s;
        const QString canonicalFilePathB = QFileInfo(filePathB).canonicalFilePath();
        QVERIFY(!canonicalFilePathB.isEmpty());

        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                Extended);

        DomItem fileA;
        DomItem fileB;

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, filePathA),
                         [&fileA](Path, const DomItem &, const DomItem &newIt) {
                             fileA = newIt.fileObject();
                         });

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, filePathB),
                         [&fileB](Path, const DomItem &, const DomItem &newIt) {
                             fileB = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        QCOMPARE(fileA.goToFile(canonicalFilePathB), fileB);
    }

    void goUp()
    {
        using namespace Qt::StringLiterals;
        const QString filePath = baseDir + u"/nullStatements.qml"_s;
        const QString canonicalFilePathB = QFileInfo(filePath).canonicalFilePath();
        QVERIFY(!canonicalFilePathB.isEmpty());

        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                Extended);

        DomItem fileA;
        DomItem fileB;

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, filePath),
                         [&fileA](Path, const DomItem &, const DomItem &newIt) {
                             fileA = newIt.fileObject();
                         });

        envPtr->loadPendingDependencies();

        QCOMPARE(fileA.top().goUp(1), DomItem());
        QCOMPARE(fileA.top().directParent(), DomItem());

        DomItem component = fileA.field(Fields::components).key(QString()).index(0);

        DomItem forStatement = component.field(Fields::objects)
                                       .index(0)
                                       .field(Fields::methods)
                                       .key(u"testForNull"_s)
                                       .index(0)
                                       .field(Fields::body)
                                       .field(Fields::scriptElement)
                                       .field(Fields::statements)
                                       .index(0)
                                       .field(Fields::body);

        DomItem forStatementBlock = forStatement.field(Fields::statements);

        QCOMPARE(forStatementBlock.directParent(), forStatement);
        QCOMPARE(forStatementBlock.goUp(1), forStatement);
        QCOMPARE(forStatementBlock.goUp(11), component);

        QCOMPARE(forStatement.component(GoTo::Strict), component);
    }

private:
    static DomItem parse(const QString &path, const QStringList &qmltypeDirs)
    {
        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                Extended);

        DomItem fileItem;

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, path),
                         [&fileItem](Path, const DomItem &, const DomItem &newIt) {
                             fileItem = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();
        return fileItem;
    }

    static DomItem rootQmlObjectFromFile(const QString &path, const QStringList &qmltypeDirs)
    {
        auto dom = parse(path, qmltypeDirs);
        return dom.rootQmlObject(GoTo::MostLikely);
    }

    void fieldMemberExpressionHelper(const DomItem &actual, const QStringList &expected)
    {
        Q_ASSERT(!expected.isEmpty());
        auto currentString = expected.rbegin();
        auto endString = expected.rend();
        DomItem current = actual;

        for (; currentString != endString; ++currentString, current = current.field(Fields::left)) {
            QCOMPARE(current.internalKind(), DomType::ScriptBinaryExpression);
            QCOMPARE(current.field(Fields::operation).value().toInteger(),
                     ScriptElements::BinaryExpression::FieldMemberAccess);
            QCOMPARE(current.field(Fields::right).internalKind(),
                     DomType::ScriptIdentifierExpression);
            QCOMPARE(current.field(Fields::right).field(Fields::identifier).value().toString(),
                     *currentString);
        }
    }

private slots:
    void mapsKeyedByFileLocationRegion()
    {
        using namespace Qt::StringLiterals;
        const QString filePath = baseDir + u"/fileLocationRegion.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(filePath, qmltypeDirs);
        QVERIFY(rootQmlObject);

        // test if preComments map works correctly with DomItem interface
        const DomItem binding = rootQmlObject.field(Fields::bindings).key(u"helloWorld"_s).index(0);
        const DomItem bindingRegionComments =
                binding.field(Fields::comments).field(Fields::regionComments);
        const DomItem preComments =
                bindingRegionComments.key(fileLocationRegionName(FileLocationRegion::IdentifierRegion))
                        .field(Fields::preComments);

        QCOMPARE(preComments.indexes(), 1);
        QString rawPreComment = preComments.index(0).field(Fields::rawComment).value().toString();
        QCOMPARE(preComments.index(0)
                         .field(Fields::rawComment)
                         .value()
                         .toString()
                         // replace weird newlines by \n
                         .replace("\r\n", "\n")
                         .replace("\r", "\n"),
                 u"    // before helloWorld binding\n    "_s);

        // test if postComments map works correctly with DomItem interface
        const DomItem postComments =
                bindingRegionComments
                        .key(fileLocationRegionName(FileLocationRegion::MainRegion))
                        .field(Fields::postComments);
        QCOMPARE(postComments.indexes(), 1);
        QCOMPARE(postComments.index(0)
                         .field(Fields::rawComment)
                         .value()
                         .toString()
                         // replace the windows newlines by \n
                         .replace("\r\n", "\n")
                         .replace("\r", "\n"),
                 u" // after helloWorld binding\n"_s);

        // test if FileLocation Tree map works correctly with DomItem interface
        const auto fileLocationsPtr = FileLocations::treeOf(binding);
        QVERIFY(fileLocationsPtr);
        const DomItem bindingFileLocationItem =
                rootQmlObject.top().wrap({},fileLocationsPtr).field(Fields::infoItem);
        const auto bindingFileLocationInfo = fileLocationsPtr->info();

        QCOMPARE(bindingFileLocationItem.field(Fields::fullRegion).value(),
                 bindingFileLocationItem.field(Fields::regions)
                         .key(fileLocationRegionName(FileLocationRegion::MainRegion))
                         .value());
        QCOMPARE(bindingFileLocationItem.field(Fields::fullRegion).value(),
                 sourceLocationToQCborValue(bindingFileLocationInfo.fullRegion));
        QCOMPARE(bindingFileLocationItem.field(Fields::regions)
                         .key(fileLocationRegionName(FileLocationRegion::MainRegion))
                         .value(),
                 sourceLocationToQCborValue(bindingFileLocationInfo.regions[MainRegion]));
        QCOMPARE(bindingFileLocationItem.field(Fields::regions)
                         .key(fileLocationRegionName(FileLocationRegion::ColonTokenRegion))
                         .value(),
                 sourceLocationToQCborValue(bindingFileLocationInfo.regions[ColonTokenRegion]));

        QCOMPARE(bindingFileLocationItem.field(Fields::fullRegion).value(),
                 sourceLocationToQCborValue(bindingFileLocationInfo.fullRegion));
    }

    // This test exists to document the usage of DOM API on fileLocations for debug purposes
    void fileLocationsDebugDump() {
        using namespace Qt::StringLiterals;
        const QString filePath = baseDir + u"/Base.qml"_s;
        auto qmlFile = parse(filePath, qmltypeDirs).fileObject(GoTo::MostLikely);
        QString fileDump;
        qmlFile.dump([&fileDump](QStringView v) { fileDump.append(v); });
        QVERIFY(fileDump.contains("fileLocationsTree"));
        QVERIFY(fileDump.contains("infoItem"));
        QVERIFY(fileDump.contains("fullRegion"));
        QVERIFY(fileDump.contains("regions"));
    }

    // add qml files here that should not crash the dom construction
    void crashes_data()
    {
        QTest::addColumn<QString>("filePath");

        QTest::addRow("inactiveVisitorMarkerCrash")
                << baseDir + u"/inactiveVisitorMarkerCrash.qml"_s;

        QTest::addRow("templateStrings")
                << baseDir + u"/crashes/templateStrings.qml"_s;

        QTest::addRow("lambda")
                << baseDir + u"/crashes/lambda.qml"_s;

        QTest::addRow("bracketsInBinding")
                << baseDir + u"/crashes/bracketsInBinding.qml"_s;
    }
    void crashes()
    {
        QFETCH(QString, filePath);

        const DomItem rootQmlObject = rootQmlObjectFromFile(filePath, qmltypeDirs);
        QVERIFY(rootQmlObject);
    }

    void continueStatement()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/continueStatement.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        const DomItem block =
                rootQmlObject.path(".methods[\"f\"][0].body.scriptElement.statements");

        const DomItem firstContinue = block.index(0);
        QCOMPARE(firstContinue.internalKind(), DomType::ScriptContinueStatement);
        QCOMPARE(firstContinue.field(Fields::label).value().toString("UNEXISTING"),
                 u"helloWorld"_s);

        const DomItem secondContinue = block.index(1);
        QCOMPARE(secondContinue.internalKind(), DomType::ScriptContinueStatement);
        QCOMPARE(secondContinue.field(Fields::label).internalKind(), DomType::Empty);
    }

    void breakStatement()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/breakStatement.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        const DomItem block =
                rootQmlObject.path(".methods[\"f\"][0].body.scriptElement.statements");

        const DomItem firstContinue = block.index(0);
        QCOMPARE(firstContinue.internalKind(), DomType::ScriptBreakStatement);
        QCOMPARE(firstContinue.field(Fields::label).value().toString("UNEXISTING"),
                 u"helloWorld"_s);

        const DomItem secondContinue = block.index(1);
        QCOMPARE(secondContinue.internalKind(), DomType::ScriptBreakStatement);
        QCOMPARE(secondContinue.field(Fields::label).internalKind(), DomType::Empty);
    }

    void emptyMethodBody()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/emptyMethodBody.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        const DomItem block = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement");

        QCOMPARE(block.internalKind(), DomType::ScriptBlockStatement);
        QCOMPARE(block.field(Fields::statements).indexes(), 0);
    }

    void commaExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/commaExpression.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        const DomItem commaExpression = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement.statements[0]");

        QCOMPARE(commaExpression.internalKind(), DomType::ScriptBinaryExpression);
        QCOMPARE(commaExpression.field(Fields::right)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"c"_s);
        QCOMPARE(commaExpression.field(Fields::left)
                         .field(Fields::right)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"b"_s);
        QCOMPARE(commaExpression.field(Fields::left)
                         .field(Fields::left)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"a"_s);
    }

    void conditionalExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/conditionalExpression.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);
        const DomItem commaExpression = rootQmlObject.path(".methods[\"f\"][0].body.scriptElement.statements[0]");

        QCOMPARE(commaExpression.internalKind(), DomType::ScriptConditionalExpression);
        QCOMPARE(commaExpression.field(Fields::condition)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"a"_s);
        QCOMPARE(commaExpression.field(Fields::consequence)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"b"_s);
        QCOMPARE(commaExpression.field(Fields::alternative)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"c"_s);
    }

    void unaryExpression_data()
    {
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<DomType>("type");

        const QString folder = baseDir + u"/unaryExpressions/"_s;

        QTest::addRow("minus") << folder + u"unaryMinus.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("plus") << folder + u"unaryPlus.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("tilde") << folder + u"tilde.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("not") << folder + u"not.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("typeof") << folder + u"typeof.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("delete") << folder + u"delete.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("void") << folder + u"void.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("increment") << folder + u"increment.qml"_s << DomType::ScriptUnaryExpression;
        QTest::addRow("decrement") << folder + u"decrement.qml"_s << DomType::ScriptUnaryExpression;

        // post stuff
        QTest::addRow("postIncrement")
                << folder + u"postIncrement.qml"_s << DomType::ScriptPostExpression;
        QTest::addRow("postDecrement")
                << folder + u"postDecrement.qml"_s << DomType::ScriptPostExpression;
    }

    void unaryExpression()
    {
        using namespace Qt::StringLiterals;
        QFETCH(QString, fileName);
        QFETCH(DomType, type);
        const DomItem rootQmlObject = rootQmlObjectFromFile(fileName, qmltypeDirs);
        const DomItem firstStatement =
                rootQmlObject.path(".methods[\"f\"][0].body.scriptElement.statements[0]");

        QCOMPARE(firstStatement.internalKind(), type);
        QCOMPARE(firstStatement.field(Fields::expression)
                         .field(Fields::identifier)
                         .value()
                         .toString(),
                 u"a"_s);
    }

    void objectBindings()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/objectBindings.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        const DomItem xBinding = rootQmlObject.path(".bindings[\"x\"][0].value");
        QCOMPARE(xBinding.field(Fields::name).value().toString(), u"root.QQ.Drag");
        QCOMPARE(xBinding.field(Fields::nameIdentifiers).internalKind(),
                 DomType::ScriptType);
        QCOMPARE(xBinding.field(Fields::nameIdentifiers).field(Fields::typeName).internalKind(),
                 DomType::ScriptBinaryExpression);
        QCOMPARE(xBinding.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::operation)
                         .value()
                         .toInteger(-1),
                 ScriptElements::BinaryExpression::FieldMemberAccess);

        QCOMPARE(xBinding.field(Fields::nameIdentifiers).field(Fields::typeName).field(Fields::left).internalKind(),
                 DomType::ScriptBinaryExpression);
        QCOMPARE(xBinding.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::left)
                         .field(Fields::right)
                         .value()
                         .toString(),
                 u"QQ");
        QCOMPARE(xBinding.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::left)
                         .field(Fields::left)
                         .value()
                         .toString(),
                 u"root");

        const DomItem item = rootQmlObject.path(".children[0]");
        QCOMPARE(item.field(Fields::nameIdentifiers).field(Fields::typeName).value().toString(),
                 u"Item");

        const DomItem qqItem = rootQmlObject.path(".children[1]");
        QCOMPARE(qqItem.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::operation)
                         .value()
                         .toInteger(-1),
                 ScriptElements::BinaryExpression::FieldMemberAccess);
        QCOMPARE(qqItem.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::right)
                         .value()
                         .toString(),
                 u"Item");
        QCOMPARE(qqItem.field(Fields::nameIdentifiers)
                         .field(Fields::typeName)
                         .field(Fields::left)
                         .value()
                         .toString(),
                 u"QQ");
    }

    void scriptExpression()
    {
        // verifying support of ECMA script modules by ScriptExpression
        const ScriptExpression esmExport("export function a(){}",
                                         ScriptExpression::ExpressionType::ESMCode);
        QVERIFY(esmExport.localErrors().empty());
    }

    void semanticAnalysis()
    {

        DomItem baseItem;
        DomItem derivedItem;

        auto envPtr = DomEnvironment::create(qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option{},
                                             Extended);

        envPtr->loadFile(
                FileToLoad::fromFileSystem(envPtr, baseDir + u"/Base.qml"_s),
                [&baseItem](Path, const DomItem &, const DomItem &newIt) {
                    baseItem = newIt.rootQmlObject(GoTo::MostLikely);
                });

        envPtr->loadFile(
                FileToLoad::fromFileSystem(envPtr, baseDir + u"/Derived.qml"_s),
                [&derivedItem](Path, const DomItem &, const DomItem &newIt) {
                    derivedItem = newIt.rootQmlObject(GoTo::MostLikely);
                });
        envPtr->loadPendingDependencies();

        const auto baseScope = baseItem.semanticScope();
        const auto derivedScope = derivedItem.semanticScope();

        QCOMPARE_NE(baseScope, QQmlJSScope::ConstPtr{});
        QCOMPARE(baseScope, derivedScope->baseType());
    }

    void propertyDefinitionScopes()
    {
        DomItem qmlObject;

        auto envPtr = DomEnvironment::create(qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option{},
                                             Extended);

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, baseDir + u"/propertyBindings.qml"_s),
                         [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                             qmlObject = newIt.rootQmlObject(GoTo::MostLikely);
                         });
        envPtr->loadPendingDependencies();

        {
            const auto a = qmlObject.field(Fields::propertyDefs).key(u"a").index(0);
            const auto scopeA = a.semanticScope();
            QCOMPARE_NE(scopeA, QQmlJSScope::ConstPtr{});
            QCOMPARE(scopeA->scopeType(), QQmlSA::ScopeType::QMLScope);
        }

        {
            const auto b = qmlObject.field(Fields::propertyDefs).key(u"b").index(0);
            const auto scopeB = b.semanticScope();
            QCOMPARE_NE(scopeB, QQmlJSScope::ConstPtr{});
            QCOMPARE(scopeB->scopeType(), QQmlSA::ScopeType::QMLScope);
        }
    }

    // simulate qmlls loading the same file twice like in QTBUG-123591
    void loadFileTwice()
    {
        DomItem qmlObject;
        DomItem qmlObject2;

        std::shared_ptr<DomEnvironment> envPtr = DomEnvironment::create(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, Extended);

        const QString fileName{ baseDir + u"/propertyBindings.qml"_s };
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
        const QString content = file.readAll();

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, baseDir + u"/propertyBindings.qml"_s),
                         [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                             qmlObject = newIt.rootQmlObject(GoTo::MostLikely);
                         });
        envPtr->loadPendingDependencies();

        // should not assert when loading the same file again
        auto envPtrChild = envPtr->makeCopy(DomItem(envPtr));
        envPtrChild->loadFile(
                FileToLoad::fromMemory(envPtr, baseDir + u"/propertyBindings.qml"_s, content),
                [&qmlObject2](Path, const DomItem &, const DomItem &newIt) {
                    qmlObject2 = newIt.rootQmlObject(GoTo::MostLikely);
                });
        envPtrChild->loadPendingDependencies();
    }

    void populateLazyFileBeforeCommitToBase()
    {
        DomItem qmlObject;

        std::shared_ptr<DomEnvironment> envPtr = DomEnvironment::create(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, Extended);

        const QString fileName{ QDir::cleanPath(baseDir + u"/propertyBindings.qml"_s) };

        {
            DomItem envChild = DomItem(envPtr).makeCopy(DomItem::CopyOption::EnvConnected).item();
            auto envPtrChild = envChild.ownerAs<DomEnvironment>();
            envPtrChild->loadFile(
                    FileToLoad::fromFileSystem(envPtrChild, fileName),
                    [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                        qmlObject = newIt.fileObject();
                    });
            envPtrChild->loadPendingDependencies();

            const DomItem childEnv = DomItem(envPtrChild->shared_from_this());
            // populate the lazy file by accessing it via the DomItem interface
            const DomItem mainComponent =
                    childEnv.field(Fields::qmlFileWithPath)
                            .key(fileName)
                            .field(Fields::currentItem)
                            .field(Fields::components)
                            .key(QString());
            QVERIFY(mainComponent);

            envPtrChild->commitToBase(DomItem(envPtrChild));
        } // destroy the temporary environment that the file was loaded into

        // also make sure that the main component also exists in the base environment after the
        // commitToBase call.
        const DomItem env = DomItem(envPtr->shared_from_this());
        const DomItem mainComponent = env.field(Fields::qmlFileWithPath)
                                              .key(fileName)
                                              .field(Fields::currentItem)
                                              .field(Fields::components)
                                              .key(QString());
        QVERIFY(mainComponent);
    }

    void populateLazyFileAfterCommitToBase()
    {
        DomItem qmlObject;

        std::shared_ptr<DomEnvironment> envPtr = DomEnvironment::create(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, Extended);

        const QString fileName{ QDir::cleanPath(baseDir + u"/propertyBindings.qml"_s) };

        {
            DomItem envChild = DomItem(envPtr).makeCopy(DomItem::CopyOption::EnvConnected).item();
            auto envPtrChild = envChild.ownerAs<DomEnvironment>();
            envPtrChild->loadFile(
                    FileToLoad::fromFileSystem(envPtrChild, fileName),
                    [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                        qmlObject = newIt.fileObject();
                    });
            envPtrChild->loadPendingDependencies();
            envPtrChild->commitToBase(DomItem(envPtrChild));
        } // destroy the temporary environment that the file was loaded into

        const DomItem env = DomItem(envPtr->shared_from_this());
        // populate the lazy file by accessing it via the DomItem interface
        const DomItem mainComponent = env.field(Fields::qmlFileWithPath)
                                              .key(fileName)
                                              .field(Fields::currentItem)
                                              .field(Fields::components)
                                              .key(QString());
        QVERIFY(mainComponent);
    }

    void qtbug_124799()
    {
        // reproduces the completion crash in QTBUG-124799 that was actually not completion related:
        // triggering the completion was triggering the population of a file, that led to a
        // heap-use-after-free. The steps to reproduce the crash are following:
        // 1. load a file in a temporary environment
        // 2. grab an unpopulated qqmljsscope from the type resolver of the loaded file
        // 3. destroy the temporary environment
        // 4. update the loaded file with new content, to make sure the QQmlJSImporter (used to
        // populate of qmlfiles) has no more strong references in the QmlFile.
        // 5. populate the unpopulated qqmljsscope: its factory should have kept track that its
        // environment is not the temporary one but the base one (because of the commitToBase()
        // call) and use the correct QQmlJSImporter (if its the one from the temporary environment
        // this will lead to the heap-use-after-free memory error you get when triggering
        // completions before this fix)

        DomItem qmlObject;

        std::shared_ptr<DomEnvironment> envPtr = DomEnvironment::create(
                qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, Extended);

        const QString fileName{ QDir::cleanPath(baseDir + u"/propertyBindings.qml"_s) };

        QQmlJSScope::ConstPtr populateAfterEnvironmentDestruction;

        {
            DomItem envChild = DomItem(envPtr).makeCopy(DomItem::CopyOption::EnvConnected).item();
            auto envPtrChild = envChild.ownerAs<DomEnvironment>();
            envPtrChild->loadFile(
                    FileToLoad::fromFileSystem(envPtrChild, fileName),
                    [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                        qmlObject = newIt.fileObject();
                    });
            envPtrChild->loadPendingDependencies();

            auto qmlFilePtr = qmlObject.ownerAs<QmlFile>();
            auto resolver = qmlFilePtr->typeResolver();
            // simulate completion by grabbing some type from the resolver
            populateAfterEnvironmentDestruction = resolver->importedTypes()[u"Derived"_s].scope;
            envPtrChild->commitToBase(DomItem(envPtrChild));
        }

        // update the file
        {
            DomItem envChild = DomItem(envPtr).makeCopy(DomItem::CopyOption::EnvConnected).item();
            auto envPtrChild = envChild.ownerAs<DomEnvironment>();

            // simulate user typing something
            QFile file(fileName);
            QVERIFY(file.open(QFile::ReadOnly));
            const QString content = file.readAll();
            const QString newContent = content + "\n // important comment here\n";
            envPtrChild->loadFile(FileToLoad::fromMemory(envPtrChild, fileName, newContent),
                                  [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                                      qmlObject = newIt.fileObject();
                                  });
            envPtrChild->loadPendingDependencies();
            envPtrChild->commitToBase(DomItem(envPtrChild));
        }

        // step 3: populate the lazy qqmljsscope, it should not crash
        QCOMPARE(populateAfterEnvironmentDestruction->filePath(),
                 QDir::cleanPath(baseDir + u"/Derived.qml"_s));
    }

    void visitTreeFilter()
    {
        DomItem qmlObject;

        auto envPtr = DomEnvironment::create(qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option{},
                                             Extended);

        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, baseDir + u"/visitTreeFilter.qml"_s),
                         [&qmlObject](Path, const DomItem &, const DomItem &newIt) {
                             qmlObject = newIt.rootQmlObject(GoTo::MostLikely);
                         });
        envPtr->loadPendingDependencies();

        FieldFilter filter({}, { { QString(), Fields::propertyDefs.toString() } });

        // check if propertyDefs is visited without the filter
        bool success = false;
        qmlObject.visitTree(
                Path(), emptyChildrenVisitor, VisitOption::Recurse | VisitOption::VisitSelf,
                [&success](const Path &p, const DomItem &, bool) {
                    const QString pathString = p.toString();
                    if (p && p.checkHeadName(Fields::propertyDefs)) {
                        success = true;
                    }
                    return true;
                },
                emptyChildrenVisitor);
        QVERIFY(success);

        // check that propertyDefs is not visited with the filter
        success = true;
        qmlObject.visitTree(
                Path(), emptyChildrenVisitor, VisitOption::Recurse | VisitOption::VisitSelf,
                [&success](const Path &p, const DomItem &, bool) {
                    if (p && p.checkHeadName(Fields::propertyDefs)) {
                        qWarning() << "Filter did not filter propertyDefs at path" << p;
                        success = false;
                    }
                    return true;
                },
                emptyChildrenVisitor, filter);
        QVERIFY(success);
    }

    void fileLocationRegions_data()
    {
        QTest::addColumn<QString>("filePath");
        QTest::addColumn<FileLocationRegion>("region");
        QTest::addColumn<QSet<QQmlJS::SourceLocation>>("expectedLocs");

        QTest::newRow("import") << baseDir + u"/fileLocationRegions/imports.qml"_s << ImportTokenRegion <<
        QSet {
            QQmlJS::SourceLocation{112, 6, 4, 1},
            QQmlJS::SourceLocation{127, 6, 5, 1},
        };
        QTest::newRow("importUri") << baseDir + u"/fileLocationRegions/imports.qml"_s << ImportUriRegion <<
        QSet {
            QQmlJS::SourceLocation{119, 7, 4, 8},
            QQmlJS::SourceLocation{152, 16, 6, 8},
            QQmlJS::SourceLocation{186, 9, 7, 8}
        };
        QTest::newRow("asToken") << baseDir + u"/fileLocationRegions/imports.qml"_s << AsTokenRegion <<
        QSet {
            QQmlJS::SourceLocation{169, 2, 6, 25}
        };
        QTest::newRow("version") << baseDir + u"/fileLocationRegions/imports.qml"_s << VersionRegion <<
        QSet {
            QQmlJS::SourceLocation{140, 4, 5, 14}
        };
        QTest::newRow("namespace") << baseDir + u"/fileLocationRegions/imports.qml"_s << IdNameRegion <<
        QSet {
            QQmlJS::SourceLocation{172, 6, 6, 28}
        };

        QTest::newRow("function") << baseDir + u"/fileLocationRegions/functions.qml"_s
                                  << FunctionKeywordRegion
                                  << QSet{ QQmlJS::SourceLocation{ 139, 8, 7, 5 },
                                           QQmlJS::SourceLocation{ 195, 8, 10, 9 } };

        QTest::newRow("signal") << baseDir + u"/fileLocationRegions/functions.qml"_s
                                << SignalKeywordRegion
                                << QSet{ QQmlJS::SourceLocation{ 234, 6, 13, 5 },
                                         QQmlJS::SourceLocation{ 254, 6, 14, 5 } };
        QTest::newRow("return-type-identifier")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << TypeIdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 154, 3, 7, 20 },
                         QQmlJS::SourceLocation{ 216, 3, 10, 30 } };
        QTest::newRow("function-parameter-type-identifier")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << TypeIdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 209, 3, 10, 23 } };
        QTest::newRow("signal-parameter-type-identifier")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << TypeIdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 243, 3, 13, 14 },
                         QQmlJS::SourceLocation{ 267, 3, 14, 18 } };
        QTest::newRow("signal-parameter-identifier")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << IdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 247, 1, 13, 18 },
                         QQmlJS::SourceLocation{ 264, 1, 14, 15 } };

        QTest::newRow("signal-lparen")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << LeftParenthesisRegion
                << QSet{ QQmlJS::SourceLocation{ 242, 1, 13, 13 },
                         QQmlJS::SourceLocation{ 263, 1, 14, 14 } };

        QTest::newRow("signal-rparen")
                << baseDir + u"/fileLocationRegions/functions.qml"_s << RightParenthesisRegion
                << QSet{ QQmlJS::SourceLocation{ 248, 1, 13, 19 },
                         QQmlJS::SourceLocation{ 270, 1, 14, 21 } };

        QTest::newRow("pragma-keyword")
                << baseDir + u"/fileLocationRegions/pragmas.qml"_s << PragmaKeywordRegion
                << QSet{ QQmlJS::SourceLocation{ 112, 6, 4, 1 },
                         QQmlJS::SourceLocation{ 129, 6, 5, 1 },
                         QQmlJS::SourceLocation{ 161, 6, 6, 1 },
                         QQmlJS::SourceLocation{ 204, 6, 7, 1 }};
        QTest::newRow("pragmaId")
                << baseDir + u"/fileLocationRegions/pragmas.qml"_s << IdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 119, 9, 4, 8 },
                         QQmlJS::SourceLocation{ 136, 17, 5, 8 },
                         QQmlJS::SourceLocation{ 168, 25, 6, 8 },
                         QQmlJS::SourceLocation{ 211, 17, 7, 8 }};
        QTest::newRow("pragmaValues")
                << baseDir + u"/fileLocationRegions/pragmas.qml"_s << PragmaValuesRegion
                << QSet{ QQmlJS::SourceLocation{ 155, 5, 5, 27 },
                         QQmlJS::SourceLocation{ 195, 8, 6, 35 },
                         QQmlJS::SourceLocation{ 230, 4, 7, 27 },
                         QQmlJS::SourceLocation{ 235, 11, 7, 32 }};

        QTest::newRow("enum-keyword")
                << baseDir + u"/fileLocationRegions/enums.qml"_s << EnumKeywordRegion
                << QSet{ QQmlJS::SourceLocation{ 139, 4, 7, 5 }};
        QTest::newRow("enum-id")
                << baseDir + u"/fileLocationRegions/enums.qml"_s << IdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 144, 3, 7, 10 }};
        QTest::newRow("enum-member")
                << baseDir + u"/fileLocationRegions/enums.qml"_s << IdentifierRegion
                << QSet{ QQmlJS::SourceLocation{ 158, 3, 8, 9 },
                         QQmlJS::SourceLocation{ 175, 3, 9, 9 },
                        QQmlJS::SourceLocation{ 188, 3, 10, 9 }};
        QTest::newRow("enum-value")
                << baseDir + u"/fileLocationRegions/enums.qml"_s << EnumValueRegion
                << QSet{ QQmlJS::SourceLocation{ 164, 1, 8, 15 },
                         QQmlJS::SourceLocation{ 194, 2, 10, 15 }};
    }

    void fileLocationRegions()
    {
        QFETCH(QString, filePath);
        QFETCH(FileLocationRegion, region);
        QFETCH(QSet<QQmlJS::SourceLocation>, expectedLocs);
        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        QFile f(filePath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QString code = f.readAll();
        DomItem file;
        envPtr->loadFile(FileToLoad::fromMemory(envPtr, filePath, code),
                         [&file](Path, const DomItem &, const DomItem &newIt) {
                             file = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        const auto tree = FileLocations::treeOf(file);
        QSet<QQmlJS::SourceLocation> locs;
        auto visitor = [&](const Path &currentPath, const FileLocations::Tree &fLocPtr){
            Q_UNUSED(currentPath);
            const auto regions = fLocPtr->info().regions;
            if (regions.contains(region)) {
               locs << regions.value(region);
            }
            return true;
        };
        FileLocations::visitTree(tree, visitor, Path());
        [&] {
            QVERIFY(locs.contains(expectedLocs));
        }();

        if (QTest::currentTestFailed()) {
            qDebug() << "Got:\n";
            for (auto &x : locs) {
                qDebug() << "Offset: " << x.offset
                         << ", Length:" << x.length
                         << ", Startline: " << x.startLine
                         << ", StartColumn: " << x.startColumn;
            }
            qDebug() << "But expected: \n";
            for (auto &x : expectedLocs) {
                qDebug() << "Offset: " << x.offset
                         << ", Length:" << x.length
                         << ", Startline: " << x.startLine
                         << ", StartColumn: " << x.startColumn;
            }
        }
    }

    void doNotCrashAtAstComments()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/astComments.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem astComments = fileObject.path(".astComments");

        // Visiting astComment element shouldn't fail
        QSet<QStringView> comments;
        astComments.visitTree(
            Path(),
            [&comments](const Path &, const DomItem &item, bool) {
                if (item.internalKind() == DomType::Comment) {
                    auto comment = item.as<Comment>();
                    comments << comment->rawComment();
                }
                return true;
            }
        );

        QVERIFY(comments.contains(u"/*Ast Comment*/ "_s));
    }

    void doNotCrashOnMissingLogger()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = QDir::cleanPath(baseDir + u"/astComments.qml"_s);
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        auto filePtr = fileObject.as<QmlFile>();
        QVERIFY(filePtr);
        auto typeResolver = filePtr->typeResolver();
        QVERIFY(typeResolver);
        auto logger = typeResolver->logger();
        // make sure that the logger is not use-after-free by checking its content
        QCOMPARE(logger->filePath(), testFile);
    }

    void commentLocations()
    {
        auto envPtr = DomEnvironment::create(
                QStringList(),
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                DomCreationOption::Extended);

        const auto filePath = baseDir + u"/fileLocationRegions/comments.qml"_s;
        QFile f(filePath);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QString code = f.readAll();
        DomItem file;
        envPtr->loadFile(FileToLoad::fromMemory(envPtr, filePath, code),
                         [&file](Path, const DomItem &, const DomItem &newIt) {
                             file = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();

        const auto expctedCommentLocations = QSet {
        QQmlJS::SourceLocation(0, 41, 1, 1),
        QQmlJS::SourceLocation(42,68, 2, 1),
        QQmlJS::SourceLocation(126,25, 6, 1),
        QQmlJS::SourceLocation(152,14, 10, 1),
        QQmlJS::SourceLocation(167,21, 11, 1)
        };

        QSet<SourceLocation> locs;
        file.fileObject(GoTo::MostLikely).visitTree(Path(), [&locs](Path, const DomItem &item, bool){
            if (item.internalKind() == DomType::Comment) {
                const auto comment = item.as<Comment>();
                if (comment) {
                    locs << comment->info().sourceLocation();
                }
            }
            return true;
        }, VisitOption::Default, emptyChildrenVisitor, emptyChildrenVisitor);

        QCOMPARE(locs, expctedCommentLocations);
    }

private:
    void checkFunctionKeyword(const DomItem &item) const
    {
        auto fileLocationRegions = FileLocations::treeOf(item)->info().regions;
        QVERIFY(fileLocationRegions.contains(FileLocationRegion::FunctionKeywordRegion));
        QCOMPARE(fileLocationRegions[FileLocationRegion::FunctionKeywordRegion].length,
                 std::char_traits<char>::length("function"));
    }

private slots:

    void lambdas()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem lambda = mainObject.field(Fields::methods)
                                           .key(u"method"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements)
                                           .index(1)
                                           .field(Fields::declarations)
                                           .index(0)
                                           .field(Fields::initializer);
            QVERIFY(lambda);
            QCOMPARE(lambda.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(lambda.field(Fields::name).value().toString(), u"myLambda"_s);
            QCOMPARE(lambda.field(Fields::parameters).indexes(), 2);
            QCOMPARE(lambda.field(Fields::parameters).index(0).field(Fields::identifier).value().toString(), u"a");
            QCOMPARE(lambda.field(Fields::parameters).index(1).field(Fields::identifier).value().toString(), u"b");

            auto scope = lambda.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"b"_s));

            checkFunctionKeyword(lambda);

            const DomItem body = lambda.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
        }
    }
    void arrow()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem arrow = mainObject.field(Fields::methods)
                                          .key(u"method"_s)
                                          .index(0)
                                          .field(Fields::body)
                                          .field(Fields::scriptElement)
                                          .field(Fields::statements)
                                          .index(2)
                                          .field(Fields::declarations)
                                          .index(0)
                                          .field(Fields::initializer);
            QVERIFY(arrow);
            QCOMPARE(arrow.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(arrow.field(Fields::name).value().toString(), u"myArrow"_s);
            QCOMPARE(arrow.field(Fields::parameters).indexes(), 2);
            QCOMPARE(arrow.field(Fields::parameters)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"v");
            QCOMPARE(arrow.field(Fields::parameters)
                             .index(1)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"w");

            auto scope = arrow.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"w"_s));

            const DomItem body = arrow.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            QCOMPARE(body.field(Fields::statements).indexes(), 1);
            QCOMPARE(body.field(Fields::statements).index(0).internalKind(),
                     DomType::ScriptReturnStatement);
        }
    }
    void lamdbaInBinding()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem lambda = mainObject.field(Fields::bindings)
                                           .key(u"onHelloSignal"_s)
                                           .index(0)
                                           .field(Fields::value)
                                           .field(Fields::scriptElement);
            QVERIFY(lambda);
            QCOMPARE(lambda.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(lambda.field(Fields::name).value().toString(), QString());
            QCOMPARE(lambda.field(Fields::parameters).indexes(), 3);
            QCOMPARE(lambda.field(Fields::parameters).index(0).field(Fields::identifier).value().toString(), u"x");
            QCOMPARE(lambda.field(Fields::parameters).index(2).field(Fields::identifier).value().toString(), u"z");
            auto scope = lambda.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"z"_s));

            checkFunctionKeyword(lambda);

            const DomItem body = lambda.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
        }
    }
    void nestedFunction()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem nested = mainObject.field(Fields::methods)
                                           .key(u"testNestedFunctions"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements)
                                           .index(0);
            QVERIFY(nested);
            QCOMPARE(nested.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(nested.field(Fields::name).value().toString(), u"nested"_s);
            QCOMPARE(nested.field(Fields::parameters).indexes(), 3);
            QCOMPARE(nested.field(Fields::parameters)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"tic");
            QCOMPARE(nested.field(Fields::parameters)
                             .index(2)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"toe");
            const DomItem body = nested.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            auto scope = nested.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"toe"_s));

            checkFunctionKeyword(nested);
        }
    }
    void generatorDeclaration()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem generator = mainObject.field(Fields::methods)
                                              .key(u"generators"_s)
                                              .index(0)
                                              .field(Fields::body)
                                              .field(Fields::scriptElement)
                                              .field(Fields::statements)
                                              .index(0);
            QVERIFY(generator);
            QCOMPARE(generator.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(generator.field(Fields::name).value().toString(), u"myGeneratorDeclaration"_s);
            QCOMPARE(generator.field(Fields::parameters).indexes(), 2);
            QCOMPARE(generator.field(Fields::parameters)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"a");
            QCOMPARE(generator.field(Fields::parameters)
                             .index(1)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"b");
            const DomItem body = generator.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            auto scope = generator.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"b"_s));

            const DomItem yieldExpression =
                    generator.field(Fields::body).field(Fields::statements).index(0);
            QCOMPARE(yieldExpression.internalKind(), DomType::ScriptYieldExpression);
            QCOMPARE(yieldExpression.field(Fields::expression).value().toInteger(), 5);
        }
    }
    void generatorExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem mainObject = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0);
        {
            const DomItem generator = mainObject.field(Fields::methods)
                                              .key(u"generators"_s)
                                              .index(0)
                                              .field(Fields::body)
                                              .field(Fields::scriptElement)
                                              .field(Fields::statements)
                                              .index(1)
                                              .field(Fields::declarations)
                                              .index(0)
                                              .field(Fields::initializer);
            QVERIFY(generator);
            QCOMPARE(generator.internalKind(), DomType::ScriptFunctionExpression);
            QCOMPARE(generator.field(Fields::name).value().toString(), u"myGenerator"_s);
            QCOMPARE(generator.field(Fields::parameters).indexes(), 3);
            QCOMPARE(generator.field(Fields::parameters)
                             .index(0)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"tic");
            QCOMPARE(generator.field(Fields::parameters)
                             .index(2)
                             .field(Fields::identifier)
                             .value()
                             .toString(),
                     u"toe");
            const DomItem body = generator.field(Fields::body);
            QCOMPARE(body.internalKind(), DomType::ScriptBlockStatement);
            auto scope = generator.semanticScope();
            QVERIFY(scope);
            QVERIFY(scope->jsIdentifier(u"toe"_s));
        }
    }
    void generatorDeclarationInQmlObject()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"generatorInQmlObject"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);
        {
            const DomItem nested = statements.index(0);
            QVERIFY(nested);
            QCOMPARE(nested.internalKind(), DomType::ScriptFunctionExpression);

            const DomItem nested2 = statements.index(1);
            QVERIFY(nested2);
            QCOMPARE(nested2.internalKind(), DomType::ScriptFunctionExpression);

            const DomItem yield = statements.index(2);
            QVERIFY(yield);
            QCOMPARE(yield.internalKind(), DomType::ScriptYieldExpression);

            const DomItem yieldStar = statements.index(3);
            QVERIFY(yieldStar);
            QCOMPARE(yieldStar.internalKind(), DomType::ScriptYieldExpression);

        }
    }
    void traditionalLambda()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem initializer = fileObject.field(Fields::components)
                                            .key(QString())
                                            .index(0)
                                            .field(Fields::objects)
                                            .index(0)
                                            .field(Fields::methods)
                                            .key(u"traditionalLambda"_s)
                                            .index(0)
                                            .field(Fields::body)
                                            .field(Fields::scriptElement)
                                            .field(Fields::statements)
                                            .index(0)
                                            .field(Fields::declarations)
                                            .index(0)
                                            .field(Fields::initializer);
        QVERIFY(initializer);
        QCOMPARE(initializer.internalKind(), DomType::ScriptParenthesizedExpression);
        const DomItem lambda = initializer.field(Fields::expression);
        QVERIFY(lambda);
        QCOMPARE(lambda.internalKind(), DomType::ScriptFunctionExpression);
    }
    void doNotCrashOnNestedFunction()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/lambdas.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem lambda = fileObject.field(Fields::components)
                .key(QString())
                .index(0)
                .field(Fields::objects)
                .index(0)
                .field(Fields::bindings)
                .key("Component.onCompleted")
                .index(0)
                .field(Fields::value)
                .field(Fields::scriptElement)
                .field(Fields::statements)
                .index(0);
        QVERIFY(lambda);
        QCOMPARE(lambda.internalKind(), DomType::ScriptFunctionExpression);
    }

    void regexpLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/regexpLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        const DomItem noFlag =
                statements.index(0).field(Fields::declarations).index(0).field(Fields::initializer);
        QVERIFY(noFlag);
        QCOMPARE(noFlag.internalKind(), DomType::ScriptRegExpLiteral);
        const DomItem pattern = noFlag.field(Fields::regExpPattern);
        QVERIFY(pattern);
        QCOMPARE(pattern.value().toString(), u"HelloWorld"_s);
        const DomItem flags = noFlag.field(Fields::regExpFlags);
        QVERIFY(flags);
        QCOMPARE(flags.value().toInteger(42), 0);
    }

    void regexpLiteralWithFlag()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/regexpLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);
        const DomItem withFlag =
                statements.index(1).field(Fields::declarations).index(0).field(Fields::initializer);
        QVERIFY(withFlag);
        QCOMPARE(withFlag.internalKind(), DomType::ScriptRegExpLiteral);
        const DomItem pattern = withFlag.field(Fields::regExpPattern);
        QVERIFY(pattern);
        QCOMPARE(pattern.value().toString(), u"H?ello.*[^s]+"_s);
        const DomItem flags = withFlag.field(Fields::regExpFlags);
        QVERIFY(flags);
        QCOMPARE(flags.value().toInteger(), 1);
    }

    void templateLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);
        {
            auto checkIdentifier = [](const DomItem &component, const QStringView value) {
                QVERIFY(component);
                QCOMPARE(component.internalKind(), DomType::ScriptTemplateExpressionPart);
                QCOMPARE(component[Fields::expression].internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(component[Fields::expression].value().toString(), value);
            };
            auto checkLiteral = [](const DomItem &component, const QStringView value) {
                QVERIFY(component);
                QCOMPARE(component.internalKind(), DomType::ScriptTemplateStringPart);
                QCOMPARE(component.value().toString(), value);
            };
            for (int i = 0; i < 4; ++i) {
                const DomItem initializer = statements.index(i)
                                                    .field(Fields::declarations)
                                                    .index(0)
                                                    .field(Fields::initializer);
                QVERIFY(initializer);
                QCOMPARE(initializer.internalKind(), DomType::ScriptTemplateLiteral);

                const DomItem components = initializer.field(Fields::components);
                QVERIFY(components);

                QCOMPARE(components.indexes(), i == 0 ? 5 : (i == 3 ? 3 : 4));

                int currentIndex = 0;
                if (i == 0 || i == 2)
                    checkLiteral(components.index(currentIndex++), u"a");

                checkIdentifier(components.index(currentIndex++), u"b");
                checkLiteral(components.index(currentIndex++), u"c");
                checkIdentifier(components.index(currentIndex++), u"d");

                if (i == 0 || i == 1)
                    checkLiteral(components.index(currentIndex++), u"e");
            }
        }
    }

    static QString fileContent(const QString& path)
    {
        QFile file(path);
        [&file] () { QVERIFY(file.open(QFile::ReadOnly)); }();
        return file.readAll();
    }

    static SourceLocation fromText(QStringView code, quint32 line, quint32 column, quint32 length)
    {
        auto offset = SourceLocation::offsetFrom(code, line, column);
        return SourceLocation {offset, length, line, column};
    }

    void templateLiteralExpression_data()
    {
        QTest::addColumn<int>("variableIndex");
        QTest::addColumn<SourceLocation>("expectedLeftBacktick");
        QTest::addColumn<SourceLocation>("expectedRightBacktick");

        const QString code = fileContent(baseDir + u"/templateLiterals.qml"_s);

        QTest::addRow("normality") << 0 << fromText(code, 7, 25, 1) << fromText(code, 7, 37, 1);
        QTest::addRow("noHeadNoTail")
                << 3 << fromText(code, 10, 28, 1) << fromText(code, 10, 38, 1);
        QTest::addRow("empty") << 4 << fromText(code, 11, 21, 1) << fromText(code, 11, 22, 1);
        QTest::addRow("manyLines2") << 8 << fromText(code, 17, 26, 1) << fromText(code, 19, 1, 1);
    }

    void templateLiteralExpression()
    {
        QFETCH(int, variableIndex);
        QFETCH(SourceLocation, expectedLeftBacktick);
        QFETCH(SourceLocation, expectedRightBacktick);

        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem component = fileObject.field(Fields::components)
                .key(QString())
                .index(0)
                .field(Fields::objects)
                .index(0)
                .field(Fields::methods)
                .key(u"f"_s)
                .index(0)
                .field(Fields::body)
                .field(Fields::scriptElement)
                .field(Fields::statements)
                .index(variableIndex)
                .field(Fields::declarations)
                .index(0)
                .field(Fields::initializer);

        QVERIFY(component);
        QCOMPARE(component.internalKind(), DomType::ScriptTemplateLiteral);

        auto fileLocations = FileLocations::treeOf(component)->info();

        const auto left = fileLocations.regions[FileLocationRegion::LeftBacktickTokenRegion];
        QCOMPARE(left, expectedLeftBacktick);

        const auto right = fileLocations.regions[FileLocationRegion::RightBacktickTokenRegion];
        QCOMPARE(right, expectedRightBacktick);

        QCOMPARE(fileLocations.regions.size(), 3); // left and right "`" and "main" region
    }
    void templateLiteralExpressionParts_data()
    {
        QTest::addColumn<int>("variableIndex");
        QTest::addColumn<int>("literalPartIndex");
        QTest::addColumn<SourceLocation>("expectedDollarLeftBrace");
        QTest::addColumn<SourceLocation>("expectedRightBrace");

        const QString code = fileContent(baseDir + u"/templateLiterals.qml"_s);

        QTest::addRow("bInNormality")
                << 0 << 1 << fromText(code, 7, 27, 2) << fromText(code, 7, 30, 1);
        QTest::addRow("bInNohead")
                << 1 << 0 << fromText(code, 8, 23, 2) << fromText(code, 8, 26, 1);
        QTest::addRow("dInNoTail")
                << 2 << 3 << fromText(code, 9, 29, 2) << fromText(code, 9, 32, 1);

        QTest::addRow("helloWorldInmanyLines2")
                << 8 << 1 << fromText(code, 18, 8, 2) << fromText(code, 18, 20, 1);
    }

    void templateLiteralExpressionParts()
    {
        QFETCH(int, variableIndex);
        QFETCH(int, literalPartIndex);
        QFETCH(SourceLocation, expectedDollarLeftBrace);
        QFETCH(SourceLocation, expectedRightBrace);

        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem component = fileObject.field(Fields::components)
                .key(QString())
                .index(0)
                .field(Fields::objects)
                .index(0)
                .field(Fields::methods)
                .key(u"f"_s)
                .index(0)
                .field(Fields::body)
                .field(Fields::scriptElement)
                .field(Fields::statements)
                .index(variableIndex)
                .field(Fields::declarations)
                .index(0)
                .field(Fields::initializer)
                .field(Fields::components)
                .index(literalPartIndex);

        QVERIFY(component);
        QCOMPARE(component.internalKind(), DomType::ScriptTemplateExpressionPart);

        auto fileLocations = FileLocations::treeOf(component)->info();

        const auto left = fileLocations.regions[FileLocationRegion::DollarLeftBraceTokenRegion];
        QCOMPARE(left, expectedDollarLeftBrace);

        const auto right = fileLocations.regions[FileLocationRegion::RightBraceRegion];
        QCOMPARE(right, expectedRightBrace);

        QCOMPARE(fileLocations.regions.size(), 3); // "${", "}" and "main" region
    }

    void templateLiteralStringParts_data()
    {
        QTest::addColumn<int>("variableIndex");
        QTest::addColumn<int>("literalPartIndex");
        QTest::addColumn<SourceLocation>("expectedLocation");

        const QString code = fileContent(baseDir + u"/templateLiterals.qml"_s);

        QTest::addRow("aInNormality") << 0 << 0 << fromText(code, 7, 26, 1);
        QTest::addRow("cInNormality") << 0 << 2 << fromText(code, 7, 31, 1);
        QTest::addRow("eInNormality") << 0 << 4 << fromText(code, 7, 36, 1);
        const constexpr quint32 newlineSize =
        #ifdef Q_OS_WIN
                2;
        #else
                1;
        #endif
        QTest::addRow("manyLinesFirstPart")
                << 5 << 0 << fromText(code, 12, 26, 18 + 2 * newlineSize);
        QTest::addRow("manyLines2FirstPart") << 8 << 0 << fromText(code, 17, 27, 13 + newlineSize);
        QTest::addRow("manyLines2LastPart") << 8 << 2 << fromText(code, 18, 21, newlineSize);
    }

    void templateLiteralStringParts()
    {
        QFETCH(int, variableIndex);
        QFETCH(int, literalPartIndex);
        QFETCH(SourceLocation, expectedLocation);

        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem component = fileObject.field(Fields::components)
                .key(QString())
                .index(0)
                .field(Fields::objects)
                .index(0)
                .field(Fields::methods)
                .key(u"f"_s)
                .index(0)
                .field(Fields::body)
                .field(Fields::scriptElement)
                .field(Fields::statements)
                .index(variableIndex)
                .field(Fields::declarations)
                .index(0)
                .field(Fields::initializer)
                .field(Fields::components)
                .index(literalPartIndex);

        QVERIFY(component);
        QCOMPARE(component.internalKind(), DomType::ScriptTemplateStringPart);

        auto fileLocations = FileLocations::treeOf(component)->info();

        const auto location = fileLocations.regions[FileLocationRegion::MainRegion];
        qDebug() << location.startLine;
        qDebug() << location.startColumn;
        qDebug() << location.offset;
        qDebug() << location.length;
        QCOMPARE(location, expectedLocation);

        QCOMPARE(fileLocations.regions.size(), 1); // "main" region
    }

    void newExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/newExpressions.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);
        {
            const DomItem newExpression = statements.index(0).field(Fields::expression);
            QVERIFY(newExpression);
            QCOMPARE(newExpression.internalKind(), DomType::ScriptNewExpression);
            const DomItem expression = newExpression.field((Fields::expression));
            QVERIFY(expression);
            QCOMPARE(expression.value().toInteger(), 4);
        }
    }

    void newMemberExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/newExpressions.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"g"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);
        {
            const DomItem newMemberExpression = statements.index(0).field(Fields::expression);
            QVERIFY(newMemberExpression);
            QCOMPARE(newMemberExpression.internalKind(), DomType::ScriptNewMemberExpression);
            const DomItem base = newMemberExpression.field((Fields::base));
            QVERIFY(base);
            QCOMPARE(base.value().toString(), u"Base"_s);
            const DomItem arguments = newMemberExpression.field((Fields::arguments));
            QCOMPARE(arguments.indexes(), 1);
            QCOMPARE(arguments.index(0).value().toString(), u"argument");
        }
    }

    void emptyTemplateLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem empty = statements.index(4)
                                          .field(Fields::declarations)
                                          .index(0)
                                          .field(Fields::initializer);
            QVERIFY(empty);
            QCOMPARE(empty.internalKind(), DomType::ScriptTemplateLiteral);
            QVERIFY(empty.field(Fields::components));
            QCOMPARE(empty.field(Fields::components).indexes(), 0);
        }
    }

    void multiLineTemplateLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem manyLines = statements.index(5)
                                              .field(Fields::declarations)
                                              .index(0)
                                              .field(Fields::initializer);
            QVERIFY(manyLines);
            QCOMPARE(manyLines.internalKind(), DomType::ScriptTemplateLiteral);
            QVERIFY(manyLines.field(Fields::components));
            QCOMPARE(manyLines.field(Fields::components).indexes(), 1);
            QCOMPARE(manyLines.field(Fields::components)
                             .index(0)
                             .value()
                             .toString(),
                     u"line 1\nline 2\nline 3");

            // should not be a string, or a part of the string.
            const DomItem afterManyLines = statements.index(6);
            QCOMPARE(afterManyLines.internalKind(), DomType::ScriptVariableDeclaration);
        }
    }

    void taggedTemplateLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/templateLiterals.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem taggedTemplate = statements.index(7)
                                                   .field(Fields::declarations)
                                                   .index(0)
                                                   .field(Fields::initializer);
            QVERIFY(taggedTemplate);
            QCOMPARE(taggedTemplate.internalKind(), DomType::ScriptTaggedTemplate);
            {
                const DomItem templateLiteral = taggedTemplate.field(Fields::templateLiteral);
                QVERIFY(templateLiteral);
                QVERIFY(templateLiteral.field(Fields::components));
                // That might change in future if we end up needing the non-script parts inside of
                // the templates in the Dom.
                QCOMPARE(templateLiteral.field(Fields::components).indexes(), 1);
                QCOMPARE(templateLiteral.field(Fields::components).index(0).value().toString(),
                         u"a normal string"_s);
            }
            {
                const DomItem base = taggedTemplate.field(Fields::callee);
                QVERIFY(base);
                QCOMPARE(base.internalKind(), DomType::ScriptIdentifierExpression);
                QCOMPARE(base.value().toString(), u"its"_s);
            }
        }
    }

    void thisExpression()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/thisExpressions.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem thisLiteral = statements.index(0)
                                          .field(Fields::declarations)
                                          .index(0)
                                          .field(Fields::initializer);
            QVERIFY(thisLiteral);
            QCOMPARE(thisLiteral.internalKind(), DomType::ScriptThisExpression);
        }
        {
            const DomItem thisAccess = statements.index(1)
                                          .field(Fields::left)
                                          .field(Fields::left);
            QVERIFY(thisAccess);
            QCOMPARE(thisAccess.internalKind(), DomType::ScriptThisExpression);
        }
    }

    void superLiteral()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/superLiteral.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem superCall = statements.index(0).field(Fields::callee);
            QVERIFY(superCall);
            QCOMPARE(superCall.internalKind(), DomType::ScriptSuperLiteral);
        }

        {
            const DomItem superAccess = statements.index(1)
                    .field(Fields::declarations)
                    .index(0)
                    .field(Fields::initializer)
                    .field(Fields::callee)
                    .field(Fields::left);
            QVERIFY(superAccess);
            QCOMPARE(superAccess.internalKind(), DomType::ScriptSuperLiteral);
        }
    }

    void metaProperty()
    {
        using namespace Qt::StringLiterals;
        const QString testFile = baseDir + u"/metaProperty.qml"_s;
        const DomItem fileObject = rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject();
        const DomItem statements = fileObject.field(Fields::components)
                                           .key(QString())
                                           .index(0)
                                           .field(Fields::objects)
                                           .index(0)
                                           .field(Fields::methods)
                                           .key(u"f"_s)
                                           .index(0)
                                           .field(Fields::body)
                                           .field(Fields::scriptElement)
                                           .field(Fields::statements);

        {
            const DomItem newTarget = statements.index(0).field(Fields::condition);
            QVERIFY(newTarget);
            QCOMPARE(newTarget.internalKind(), DomType::ScriptBinaryExpression);
        }
    }

    void environmentSetLoadPaths()
    {
        auto envPtr = DomEnvironment::create(
                QStringList{},
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies,
                Extended);

        auto semanticAnalysis = envPtr->semanticAnalysis();
        QVERIFY(semanticAnalysis.m_mapper->isEmpty());
        envPtr->setLoadPaths(QStringList { baseDir + u"/buildFolderWithQrc"_s });
        QVERIFY(!semanticAnalysis.m_mapper->isEmpty());
        QVERIFY(semanticAnalysis.m_mapper->isFile(u"/qt/qml/MyModule/qml/HelloWorld.qml"_s));
    }

    void methodSignature_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<QString>("expectedSignature");

        QTest::addRow("function") << u"f"_s << u"(a: int, b: string): bool"_s;
        QTest::addRow("signal") << u"f2"_s << u"(int a, b: string)"_s;
        QTest::addRow("noArgs") << u"noArgs"_s << u""_s;
        QTest::addRow("returnVoid") << u"returnVoid"_s << u"(): void"_s;
        QTest::addRow("defaultArgs") << u"defaultArgs"_s << u"(x = 12345, y = { x: 44, y: \"hello\", z: x => x }): void"_s;
        QTest::addRow("deconstruction") << u"deconstruction"_s << u"({ x = 4 }): void"_s;
        QTest::addRow("functionWithComments") << u"comments"_s << u"(a: int, b): int"_s;
    }

    void methodSignature()
    {
        QFETCH(QString, name);
        QFETCH(QString, expectedSignature);

        const QString testFile = baseDir + u"/methods.qml"_s;
        const DomItem rootQmlObject = rootQmlObjectFromFile(testFile, qmltypeDirs);

        const DomItem methods = rootQmlObject.field(Fields::methods);
        const DomItem method = methods.key(name)[0];
        const MethodInfo *methodInfo = method.as<MethodInfo>();

        QCOMPARE(methodInfo->signature(method), expectedSignature);
    }

    void commentsFileLocations()
    {
        const QString testFile = baseDir + u"/Comments.qml"_s;
        const auto fileLocations = FileLocations::treeOf(rootQmlObjectFromFile(testFile, qmltypeDirs).fileObject());
        const auto commentsPath = Path::fromString(u".components[\"\"][0].objects[0].children[0].comments.regionComments");
        const auto commentsForItem = FileLocations::find(fileLocations, commentsPath);

        QVERIFY(commentsForItem);
        {
            // Hello World! comment
            const auto preCommentPath = Path::fromString(u"[\"IdentifierRegion\"].preComments[0]");
            const auto preCommentFLocPtr = FileLocations::find(commentsForItem, preCommentPath);

            QVERIFY(preCommentFLocPtr);
            QCOMPARE(preCommentFLocPtr->info().fullRegion.startLine, 4);
            QCOMPARE(preCommentFLocPtr->info().fullRegion.startColumn, 5);
        }
        {
            // Goodbye World! comment
            const auto postCommentPath = Path::fromString(u"[\"MainRegion\"].postComments[0]");
            const auto postCommentFLocPtr = FileLocations::find(commentsForItem, postCommentPath);

            QVERIFY(postCommentFLocPtr);
            QCOMPARE(postCommentFLocPtr->info().fullRegion.startLine, 6);
            QCOMPARE(postCommentFLocPtr->info().fullRegion.startColumn, 5);
        }
        {
            // multi line comment
            const auto postCommentPath = Path::fromString(u"[\"MainRegion\"].postComments[1]");
            const auto postCommentFLocPtr = FileLocations::find(commentsForItem, postCommentPath);

            QVERIFY(postCommentFLocPtr);
            QCOMPARE(postCommentFLocPtr->info().fullRegion.startLine, 7);
            QCOMPARE(postCommentFLocPtr->info().fullRegion.startColumn, 5);
        }
    }

private:
    QString baseDir;
    QStringList qmltypeDirs;
    std::shared_ptr<DomUniverse> universePtr;
    std::shared_ptr<DomEnvironment> envPtr;
    DomItem env;
    std::shared_ptr<MockOwner> testOwnerPtr;
    DomItem tOwner;
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif
