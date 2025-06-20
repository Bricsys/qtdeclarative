// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qtest.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickanimation_p.h>
#include <QtQuick/private/qquicklistview_p.h>
#include <QtQuick/private/qquickrepeater_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQml/private/qqmlexpression_p.h>
#include <QtQml/private/qqmlsignalnames_p.h>
#include <QQmlComponent>

#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtranslator.h>
#include <QSignalSpy>

#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace Qt::StringLiterals;

Q_DECLARE_METATYPE(QList<int>)
Q_DECLARE_METATYPE(QList<QVariantHash>)

#define RUNEVAL(object, string) \
    QVERIFY(QMetaObject::invokeMethod(object, "runEval", Q_ARG(QVariant, QString(string))));

inline QVariant runexpr(QQmlEngine *engine, const QString &str)
{
    QQmlExpression expr(engine->rootContext(), nullptr, str);
    return expr.evaluate();
}

#define RUNEXPR(string) runexpr(&engine, QString(string))

static bool isValidErrorMessage(const QString &msg, bool dynamicRoleTest)
{
    bool valid = true;

    if (msg.isEmpty()) {
        valid = false;
    } else if (dynamicRoleTest) {
        if (msg.contains("Can't assign to existing role") || msg.contains("Can't create role for unsupported data type"))
            valid = false;
    }

    return valid;
}

class tst_qqmllistmodel : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmllistmodel()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
        qRegisterMetaType<QVector<int> >();
    }

private:
    int roleFromName(const QQmlListModel *model, const QString &roleName);

    static bool compareVariantList(const QVariantList &testList, QVariant object);

private slots:
    void static_types();
    void static_types_data();
    void static_i18n();
    void static_i18n_data();
    void dynamic_i18n();
    void dynamic_i18n_data();
    void static_nestedElements();
    void static_nestedElements_data();
    void dynamic_data();
    void dynamic();
    void enumerate();
    void error_data();
    void error();
    void syncError();
    void get();
    void set_data();
    void set();
    void get_data();
    void get_nested();
    void get_nested_data();
    void crash_model_with_multiple_roles();
    void crash_model_with_unknown_roles();
    void crash_model_with_dynamic_roles();
    void set_model_cache();
    void property_changes();
    void property_changes_data();
    void clear_data();
    void clear();
    void signal_handlers_data();
    void signal_handlers();
    void role_mode_data();
    void role_mode();
    void string_to_list_crash();
    void empty_element_warning();
    void empty_element_warning_data();
    void datetime();
    void datetime_data();
    void url();
    void about_to_be_signals();
    void modify_through_delegate();
    void bindingsOnGetResult();
    void stringifyModelEntry();
    void qobjectTrackerForDynamicModelObjects();
    void crash_append_empty_array();
    void dynamic_roles_crash_QTBUG_38907();
    void nestedListModelIteration();
    void undefinedAppendShouldCauseError();
    void nullPropertyCrash();
    void objectDestroyed();
    void destroyObject();
    void emptyStringNotUndefined();
    void listElementWithTemplateString();
    void destroyComponentObject();
    void objectOwnershipFlip();
    void enumsInListElement();
    void protectQObjectFromGC();
    void nestedLists();
    void deadModelData();
};

bool tst_qqmllistmodel::compareVariantList(const QVariantList &testList, QVariant object)
{
    bool allOk = true;

    QQmlListModel *model = qobject_cast<QQmlListModel *>(object.value<QObject *>());
    if (model == nullptr)
        return false;

    if (model->count() != testList.size())
        return false;

    for (int i=0 ; i < testList.size() ; ++i) {
        const QVariant &testVariant = testList.at(i);
        if (testVariant.typeId() != QMetaType::QVariantMap)
            return false;
        const QVariantMap &map = testVariant.toMap();

        const QHash<int, QByteArray> roleNames = model->roleNames();

        QVariantMap::const_iterator it = map.begin();
        QVariantMap::const_iterator end = map.end();

        while (it != end) {
            const QString &testKey = it.key();
            const QVariant &testData = it.value();

            int roleIndex = roleNames.key(testKey.toUtf8(), -1);
            if (roleIndex == -1)
                return false;

            const QVariant &modelData = model->data(i, roleIndex);

            if (testData.typeId() == QMetaType::QVariantList) {
                const QVariantList &subList = testData.toList();
                allOk = allOk && compareVariantList(subList, modelData);
            } else {
                allOk = allOk && (testData == modelData);
            }

            ++it;
        }
    }

    return allOk;
}

int tst_qqmllistmodel::roleFromName(const QQmlListModel *model, const QString &roleName)
{
    return model->roleNames().key(roleName.toUtf8(), -1);
}

void tst_qqmllistmodel::static_types_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("error");

    QTest::newRow("string")
        << "ListElement { foo: \"bar\" }"
        << QVariant(QString("bar"))
        << QString();

    QTest::newRow("real")
        << "ListElement { foo: 10.5 }"
        << QVariant(10.5)
        << QString();

    QTest::newRow("real0")
        << "ListElement { foo: 0 }"
        << QVariant(double(0))
        << QString();

    QTest::newRow("bool")
        << "ListElement { foo: false }"
        << QVariant(false)
        << QString();

    QTest::newRow("bool")
        << "ListElement { foo: true }"
        << QVariant(true)
        << QString();

    QTest::newRow("enum")
        << "ListElement { foo: Text.AlignHCenter }"
        << QVariant(double(QQuickText::AlignHCenter))
        << QString();

    QTest::newRow("Qt enum")
        << "ListElement { foo: Qt.AlignBottom }"
        << QVariant(double(Qt::AlignBottom))
        << QString();

    QTest::newRow("negative enum")
        << "ListElement { foo: Animation.Infinite }"
        << QVariant(double(QQuickAbstractAnimation::Infinite))
        << QString();

    QTest::newRow("role error")
        << "ListElement { foo: 1 } ListElement { foo: 'string' }"
        << QVariant()
        << QString("<Unknown File>: Can't assign to existing role 'foo' of different type [String -> Number]");

    QTest::newRow("list type error")
        << "ListElement { foo: 1 } ListElement { foo: ListElement { bar: 1 } }"
        << QVariant()
        << QString("<Unknown File>: Can't assign to existing role 'foo' of different type [List -> Number]");
}

void tst_qqmllistmodel::static_types()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);
    QFETCH(QString, error);

    qml = "import QtQuick 2.0\nItem { property variant test: model.get(0).foo; ListModel { id: model; " + qml + " } }";

    if (!error.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, error.toLatin1());
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    QVERIFY(!component.isError());

    std::unique_ptr<QObject> obj { component.create() };
    QVERIFY(obj);

    if (error.isEmpty()) {
        QVariant actual = obj->property("test");

        QCOMPARE(actual, value);
        QCOMPARE(actual.toString(), value.toString());
    }
}

void tst_qqmllistmodel::static_i18n_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("error");

    QTest::newRow("QT_TR_NOOP")
        << QString::fromUtf8("ListElement { foo: QT_TR_NOOP(\"na\303\257ve\") }")
        << QVariant(QString::fromUtf8("na\303\257ve"))
        << QString();

    QTest::newRow("QT_TR_NOOP_disambig")
            << QString::fromUtf8("ListElement { foo: QT_TR_NOOP(\"na\303\257ve\", \"disambig\") }")
            << QVariant(QString::fromUtf8("na\303\257ve"))
            << QString();

    QTest::newRow("QT_TRANSLATE_NOOP")
        << "ListElement { foo: QT_TRANSLATE_NOOP(\"MyListModel\", \"hello\") }"
        << QVariant(QString("hello"))
        << QString();

    QTest::newRow("QT_TRANSLATE_NOOP_disambig")
            << "ListElement { foo: QT_TRANSLATE_NOOP(\"MyListModel\", \"hello\", \"greeting\") }"
            << QVariant(QString("hello"))
            << QString();

    QTest::newRow("QT_TRID_NOOP")
        << QString::fromUtf8("ListElement { foo: QT_TRID_NOOP(\"qtn_1st_text\") }")
        << QVariant(QString("qtn_1st_text"))
        << QString();

    QTest::newRow("QT_TR_NOOP extra param")
            << QString::fromUtf8("ListElement { foo: QT_TR_NOOP(\"hello\",\"world\", \"!\") }")
            << QVariant(QString())
            << QString("ListElement: cannot use script for property value");

    QTest::newRow("QT_TRANSLATE_NOOP missing params")
        << "ListElement { foo: QT_TRANSLATE_NOOP() }"
        << QVariant(QString())
        << QString("ListElement: cannot use script for property value");

    QTest::newRow("QT_TRID_NOOP missing param")
        << QString::fromUtf8("ListElement { foo: QT_TRID_NOOP() }")
        << QVariant(QString())
        << QString("ListElement: cannot use script for property value");
}

void tst_qqmllistmodel::static_i18n()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);
    QFETCH(QString, error);

    qml = "import QtQuick 2.0\nItem { property variant test: model.get(0).foo; ListModel { id: model; " + qml + " } }";

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    if (!error.isEmpty()) {
        QVERIFY(component.isError());
        QCOMPARE(component.errors().at(0).description(), error);
        return;
    }

    QVERIFY(!component.isError());

    std::unique_ptr<QObject> obj { component.create() };
    QVERIFY(obj);

    QVariant actual = obj->property("test");

    QCOMPARE(actual, value);
    QCOMPARE(actual.toString(), value.toString());
}

void tst_qqmllistmodel::dynamic_i18n_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QVariant>("value");
    QTest::addColumn<QString>("error");

    QTest::newRow("qsTr")
        << QString::fromUtf8("ListElement { foo: qsTr(\"test\") }")
        << QVariant(QString::fromUtf8("test"))
        << QString();

    QTest::newRow("qsTrId")
        << "ListElement { foo: qsTrId(\"qtn_test\") }"
        << QVariant(QString("qtn_test"))
        << QString();
}

void tst_qqmllistmodel::dynamic_i18n()
{
    QFETCH(QString, qml);
    QFETCH(QVariant, value);
    QFETCH(QString, error);

    qml = "import QtQuick 2.0\nItem { property variant test: model.get(0).foo; ListModel { id: model; " + qml + " } }";

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));

    if (!error.isEmpty()) {
        QVERIFY(component.isError());
        QCOMPARE(component.errors().at(0).description(), error);
        return;
    }

    QVERIFY(!component.isError());

    std::unique_ptr<QObject> obj { component.create() };
    QVERIFY(obj);

    QVariant actual = obj->property("test");

    QCOMPARE(actual, value);
    QCOMPARE(actual.toString(), value.toString());
}

void tst_qqmllistmodel::static_nestedElements()
{
    QFETCH(int, elementCount);

    QStringList elements;
    for (int i=0; i<elementCount; i++)
        elements.append("ListElement { a: 1; b: 2 }");
    QString elementsStr = elements.join(",\n") + "\n";

    QString componentStr =
        "import QtQuick 2.0\n"
        "Item {\n"
        "    property variant count: model.get(0).attributes.count\n"
        "    ListModel {\n"
        "        id: model\n"
        "        ListElement {\n"
        "            attributes: [\n";
    componentStr += elementsStr.toUtf8().constData();
    componentStr +=
        "            ]\n"
        "        }\n"
        "    }\n"
        "}";

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(componentStr.toUtf8(), QUrl::fromLocalFile(""));

    std::unique_ptr<QObject> obj { component.create() };
    QVERIFY(obj);

    QVariant count = obj->property("count");
    QCOMPARE(count.typeId(), QMetaType::Int);
    QCOMPARE(count.toInt(), elementCount);
}

void tst_qqmllistmodel::static_nestedElements_data()
{
    QTest::addColumn<int>("elementCount");

    QTest::newRow("0 items") << 0;
    QTest::newRow("1 item") << 1;
    QTest::newRow("2 items") << 2;
    QTest::newRow("many items") << 5;
}

void tst_qqmllistmodel::dynamic_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
        bool dr = (i != 0);

        // Simple flat model
        QTest::newRow("count") << "count" << 0 << "" << dr;

        QTest::newRow("get1") << "{get(0) === undefined}" << 1 << "" << dr;
        QTest::newRow("get2") << "{get(-1) === undefined}" << 1 << "" << dr;
        QTest::newRow("get3") << "{append({'foo':123});get(0) != undefined}" << 1 << "" << dr;
        QTest::newRow("get4") << "{append({'foo':123});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("get5") << "{append({'foo':123});get(0) == get(0)}" << 1 << "" << dr;
        QTest::newRow("get-modify1") << "{append({'foo':123,'bar':456});get(0).foo = 333;get(0).foo}" << 333 << "" << dr;
        QTest::newRow("get-modify2") << "{append({'z':1});append({'foo':123,'bar':456});get(1).bar = 999;get(1).bar}" << 999 << "" << dr;

        QTest::newRow("append1") << "{append({'foo':123});count}" << 1 << "" << dr;
        QTest::newRow("append2") << "{append({'foo':123,'bar':456});count}" << 1 << "" << dr;
        QTest::newRow("append3a") << "{append({'foo':123});append({'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("append3b") << "{append({'foo':123});append({'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::newRow("append4a") << "{append(123)}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::newRow("append4b") << "{append([{'foo':123},{'foo':456},{'foo':789}]);count}" << 3 << "" << dr;
        QTest::newRow("append4c") << "{append([{'foo':123},{'foo':456},{'foo':789}]);get(1).foo}" << 456 << "" << dr;

        QTest::newRow("clear1") << "{append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::newRow("clear2") << "{append({'foo':123});append({'foo':456});clear();count}" << 0 << "" << dr;
        QTest::newRow("clear3") << "{append({'foo':123});clear()}" << 0 << "" << dr;

        QTest::newRow("remove1") << "{append({'foo':123});remove(0);count}" << 0 << "" << dr;
        QTest::newRow("remove2a") << "{append({'foo':123});append({'foo':456});remove(0);count}" << 1 << "" << dr;
        QTest::newRow("remove2b") << "{append({'foo':123});append({'foo':456});remove(0);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove2c") << "{append({'foo':123});append({'foo':456});remove(1);get(0).foo}" << 123 << "" << dr;
        QTest::newRow("remove3") << "{append({'foo':123});remove(0)}" << 0 << "" << dr;
        QTest::newRow("remove3a") << "{append({'foo':123});remove(-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [-1 - 0] out of range [0 - 1]" << dr;
        QTest::newRow("remove4a") << "{remove(0)}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::newRow("remove4b") << "{append({'foo':123});remove(0);remove(0);count}" << 0 << "<Unknown File>: QML ListModel: remove: indices [0 - 1] out of range [0 - 0]" << dr;
        QTest::newRow("remove4c") << "{append({'foo':123});remove(1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [1 - 2] out of range [0 - 1]" << dr;
        QTest::newRow("remove5a") << "{append({'foo':123});append({'foo':456});remove(0,2);count}" << 0 << "" << dr;
        QTest::newRow("remove5b") << "{append({'foo':123});append({'foo':456});remove(0,1);count}" << 1 << "" << dr;
        QTest::newRow("remove5c") << "{append({'foo':123});append({'foo':456});remove(1,1);count}" << 1 << "" << dr;
        QTest::newRow("remove5d") << "{append({'foo':123});append({'foo':456});remove(0,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove5e") << "{append({'foo':123});append({'foo':456});remove(1,1);get(0).foo}" << 123 << "" << dr;
        QTest::newRow("remove5f") << "{append({'foo':123});append({'foo':456});append({'foo':789});remove(0,1);remove(1,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("remove6a") << "{remove();count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::newRow("remove6b") << "{remove(1,2,3);count}" << 0 << "<Unknown File>: QML ListModel: remove: incorrect number of arguments" << dr;
        QTest::newRow("remove7a") << "{append({'foo':123});remove(0,0);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - 0] out of range [0 - 1]" << dr;
        QTest::newRow("remove7b") << "{append({'foo':123});remove(0,-1);count}" << 1 << "<Unknown File>: QML ListModel: remove: indices [0 - -1] out of range [0 - 1]" << dr;

        QTest::newRow("insert1") << "{insert(0,{'foo':123});count}" << 1 << "" << dr;
        QTest::newRow("insert2") << "{insert(1,{'foo':123});count}" << 0 << "<Unknown File>: QML ListModel: insert: index 1 out of range" << dr;
        QTest::newRow("insert3a") << "{append({'foo':123});insert(1,{'foo':456});count}" << 2 << "" << dr;
        QTest::newRow("insert3b") << "{append({'foo':123});insert(1,{'foo':456});get(0).foo}" << 123 << "" << dr;
        QTest::newRow("insert3c") << "{append({'foo':123});insert(1,{'foo':456});get(1).foo}" << 456 << "" << dr;
        QTest::newRow("insert3d") << "{append({'foo':123});insert(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::newRow("insert3e") << "{append({'foo':123});insert(0,{'foo':456});get(1).foo}" << 123 << "" << dr;
        QTest::newRow("insert4") << "{append({'foo':123});insert(-1,{'foo':456});count}" << 1 << "<Unknown File>: QML ListModel: insert: index -1 out of range" << dr;
        QTest::newRow("insert5a") << "{insert(0,123)}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("insert5b") << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);count}" << 3 << "" << dr;
        QTest::newRow("insert5c") << "{insert(0,[{'foo':11},{'foo':22},{'foo':33}]);get(2).foo}" << 33 << "" << dr;

        QTest::newRow("set1") << "{append({'foo':123});set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::newRow("set2") << "{append({'foo':123});set(0,{'foo':456});get(0).foo}" << 456 << "" << dr;
        QTest::newRow("set3a") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).foo}" << 999 << "" << dr;
        QTest::newRow("set3b") << "{append({'foo':123,'bar':456});set(0,{'foo':999});get(0).bar}" << 456 << "" << dr;
        QTest::newRow("set4a") << "{set(0,{'foo':456});count}" << 1 << "" << dr;
        QTest::newRow("set4c") << "{set(-1,{'foo':456})}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::newRow("set5a") << "{append({'foo':123,'bar':456});set(0,123);count}" << 1 << "<Unknown File>: QML ListModel: set: value is not an object" << dr;
        QTest::newRow("set5b") << "{append({'foo':123,'bar':456});set(0,[1,2,3]);count}" << 1 << "" << dr;
        QTest::newRow("set6") << "{append({'foo':123});set(1,{'foo':456});count}" << 2 << "" << dr;

        QTest::newRow("setprop1") << "{append({'foo':123});setProperty(0,'foo',456);count}" << 1 << "" << dr;
        QTest::newRow("setprop2") << "{append({'foo':123});setProperty(0,'foo',456);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("setprop3a") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).foo}" << 999 << "" << dr;
        QTest::newRow("setprop3b") << "{append({'foo':123,'bar':456});setProperty(0,'foo',999);get(0).bar}" << 456 << "" << dr;
        QTest::newRow("setprop4a") << "{setProperty(0,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index 0 out of range" << dr;
        QTest::newRow("setprop4b") << "{setProperty(-1,'foo',456)}" << 0 << "<Unknown File>: QML ListModel: set: index -1 out of range" << dr;
        QTest::newRow("setprop4c") << "{append({'foo':123,'bar':456});setProperty(1,'foo',456);count}" << 1 << "<Unknown File>: QML ListModel: set: index 1 out of range" << dr;
        QTest::newRow("setprop5") << "{append({'foo':123,'bar':456});append({'foo':111});setProperty(1,'bar',222);get(1).bar}" << 222 << "" << dr;

        QTest::newRow("move1a") << "{append({'foo':123});append({'foo':456});move(0,1,1);count}" << 2 << "" << dr;
        QTest::newRow("move1b") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("move1c") << "{append({'foo':123});append({'foo':456});move(0,1,1);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move1d") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(0).foo}" << 456 << "" << dr;
        QTest::newRow("move1e") << "{append({'foo':123});append({'foo':456});move(1,0,1);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move2a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);count}" << 3 << "" << dr;
        QTest::newRow("move2b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(0).foo}" << 789 << "" << dr;
        QTest::newRow("move2c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(1).foo}" << 123 << "" << dr;
        QTest::newRow("move2d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,1,2);get(2).foo}" << 456 << "" << dr;
        QTest::newRow("move3a") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,3);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3b") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,-1,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3c") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(1,0,-1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;
        QTest::newRow("move3d") << "{append({'foo':123});append({'foo':456});append({'foo':789});move(0,3,1);count}" << 3 << "<Unknown File>: QML ListModel: move: out of range" << dr;

        QTest::newRow("large1") << "{append({'a':1,'b':2,'c':3,'d':4,'e':5,'f':6,'g':7,'h':8});get(0).h}" << 8 << "" << dr;

        QTest::newRow("datatypes1") << "{append({'a':1});append({'a':'string'});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [String -> Number]" << dr;

        QTest::newRow("null") << "{append({'a':null});}" << 0 << "" << dr;
        QTest::newRow("setNull") << "{append({'a':1});set(0, {'a':null});}" << 0 << "" << dr;
        QTest::newRow("setString") << "{append({'a':'hello'});set(0, {'a':'world'});get(0).a == 'world'}" << 1 << "" << dr;
        QTest::newRow("setInt") << "{append({'a':5});set(0, {'a':10});get(0).a}" << 10 << "" << dr;
        QTest::newRow("setNumber") << "{append({'a':6});set(0, {'a':5.5});get(0).a < 5.6}" << 1 << "" << dr;
        QTest::newRow("badType0") << "{append({'a':'hello'});set(0, {'a':1});}" << 0 << "<Unknown File>: Can't assign to existing role 'a' of different type [Number -> String]" << dr;
        QTest::newRow("invalidInsert0") << "{insert(0);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("invalidAppend0") << "{append();}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;
        QTest::newRow("invalidInsert1") << "{insert(0, 34);}" << 0 << "<Unknown File>: QML ListModel: insert: value is not an object" << dr;
        QTest::newRow("invalidAppend1") << "{append(37);}" << 0 << "<Unknown File>: QML ListModel: append: value is not an object" << dr;

        // QObjects
        QTest::newRow("qobject0") << "{append({'a':dummyItem0});}" << 0 << "" << dr;
        QTest::newRow("qobject1") << "{append({'a':dummyItem0});set(0,{'a':dummyItem1});get(0).a == dummyItem1;}" << 1 << "" << dr;
        QTest::newRow("qobject2") << "{append({'a':dummyItem0});get(0).a == dummyItem0;}" << 1 << "" << dr;
        QTest::newRow("qobject3") << "{append({'a':dummyItem0});append({'b':1});}" << 0 << "" << dr;

        // JS objects
        QTest::newRow("js1") << "{append({'foo':{'prop':1}});count}" << 1 << "" << dr;
        QTest::newRow("js2") << "{append({'foo':{'prop':27}});get(0).foo.prop}" << 27 << "" << dr;
        QTest::newRow("js3") << "{append({'foo':{'prop':27}});append({'bar':1});count}" << 2 << "" << dr;
        QTest::newRow("js4") << "{append({'foo':{'prop':27}});append({'bar':1});set(0, {'foo':{'prop':28}});get(0).foo.prop}" << 28 << "" << dr;
        QTest::newRow("js5") << "{append({'foo':{'prop':27}});append({'bar':1});set(1, {'foo':{'prop':33}});get(1).foo.prop}" << 33 << "" << dr;
        QTest::newRow("js6") << "{append({'foo':{'prop':27}});clear();count}" << 0 << "" << dr;
        QTest::newRow("js7") << "{append({'foo':{'prop':27}});set(0, {'foo':null});count}" << 1 << "" << dr;
        QTest::newRow("js8") << "{append({'foo':{'prop':27}});set(0, {'foo':{'prop2':31}});get(0).foo.prop2}" << 31 << "" << dr;

        // Nested models
        QTest::newRow("nested-append1") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});count}" << 1 << "" << dr;
        QTest::newRow("nested-append2") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.get(1).a}" << 2 << "" << dr;
        QTest::newRow("nested-append3") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]});get(0).bars.append({'a':4});get(0).bars.get(3).a}" << 4 << "" << dr;

        QTest::newRow("nested-insert") << "{append({'foo':123});insert(0,{'bars':[{'a':1},{'b':2},{'c':3}]});get(0).bars.get(0).a}" << 1 << "" << dr;
        QTest::newRow("nested-set") << "{append({'foo':[{'x':1}]});set(0,{'foo':[{'x':123}]});get(0).foo.get(0).x}" << 123 << "" << dr;

        QTest::newRow("nested-count") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.count}" << 3 << "" << dr;
        QTest::newRow("nested-clear") << "{append({'foo':123,'bars':[{'a':1},{'a':2},{'a':3}]}); get(0).bars.clear(); get(0).bars.count}" << 0 << "" << dr;
    }

    QTest::newRow("jsarray") << "{append({'foo':['1', '2', '3']});get(0).foo.get(0)}" << 0 << "" << false;
}

void tst_qqmllistmodel::dynamic()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);
    QFETCH(bool, dynamicRoles);

    QQuickItem dummyItem0, dummyItem1;
    QQmlEngine engine;
    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextObject(&model);
    engine.rootContext()->setContextProperty("dummyItem0", QVariant::fromValue(&dummyItem0));
    engine.rootContext()->setContextProperty("dummyItem1", QVariant::fromValue(&dummyItem1));
    QQmlExpression e(engine.rootContext(), &model, script);
    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    QSignalSpy spyCount(&model, SIGNAL(countChanged()));

    int actual = e.evaluate().toInt();
    if (e.hasError())
        qDebug() << e.error(); // errors not expected

    QCOMPARE(actual,result);

    if (model.count() > 0)
        QVERIFY(spyCount.size() > 0);
}

void tst_qqmllistmodel::enumerate()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("enumerate.qml"));
    QVERIFY(!component.isError());
    std::unique_ptr<QQuickItem> item { qobject_cast<QQuickItem*>(component.create()) };
    QVERIFY(item);

    QLatin1String expectedStrings[] = {
        QLatin1String("val1=1Y"),
        QLatin1String("val2=2Y"),
        QLatin1String("val3=strY"),
        QLatin1String("val4=falseN"),
        QLatin1String("val5=trueY")
    };

    int expectedStringCount = sizeof(expectedStrings) / sizeof(expectedStrings[0]);

    QStringList r = item->property("result").toString().split(QLatin1Char(':'));

    int matchCount = 0;
    for (int i=0 ; i < expectedStringCount ; ++i) {
        const QLatin1String &expectedString = expectedStrings[i];

        QStringList::const_iterator it = r.begin();
        QStringList::const_iterator end = r.end();

        while (it != end) {
            if (it->compare(expectedString) == 0) {
                ++matchCount;
                break;
            }
            ++it;
        }
    }

    QCOMPARE(matchCount, expectedStringCount);
}

void tst_qqmllistmodel::error_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("error");

    QTest::newRow("id not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { id: fred } }"
        << "ListElement: cannot use reserved \"id\" property";

    QTest::newRow("id allowed in ListModel")
        << "import QtQuick 2.0\nListModel { id:model }"
        << "";

    QTest::newRow("random properties not allowed in ListModel")
        << "import QtQuick 2.0\nListModel { foo:123 }"
        << "ListModel: undefined property 'foo'";

    QTest::newRow("random properties allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { foo:123 } }"
        << "";

    QTest::newRow("bindings not allowed in ListElement")
        << "import QtQuick 2.0\nRectangle { id: rect; ListModel { ListElement { foo: rect.color } } }"
        << "ListElement: cannot use script for property value";

    QTest::newRow("random object list properties allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { foo: [ ListElement { bar: 123 } ] } }"
        << "";

    QTest::newRow("default properties not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { Item { } } }"
        << "ListElement: cannot contain nested elements";

    QTest::newRow("QML elements not allowed in ListElement")
        << "import QtQuick 2.0\nListModel { ListElement { a: Item { } } }"
        << "ListElement: cannot contain nested elements";

    QTest::newRow("qualified ListElement supported")
        << "import QtQuick 2.0 as Foo\nFoo.ListModel { Foo.ListElement { a: 123 } }"
        << "";

    QTest::newRow("qualified ListElement required")
        << "import QtQuick 2.0 as Foo\nFoo.ListModel { ListElement { a: 123 } }"
        << "ListElement is not a type";

    QTest::newRow("unknown qualified ListElement not allowed")
        << "import QtQuick 2.0\nListModel { Foo.ListElement { a: 123 } }"
        << "Foo.ListElement - Foo is neither a type nor a namespace";
}

void tst_qqmllistmodel::error()
{
    QFETCH(QString, qml);
    QFETCH(QString, error);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));
    if (error.isEmpty()) {
        QVERIFY(!component.isError());
    } else {
        QVERIFY(component.isError());
        QList<QQmlError> errors = component.errors();
        QCOMPARE(errors.size(),1);
        QCOMPARE(errors.at(0).description(),error);
    }
}

void tst_qqmllistmodel::syncError()
{
    QString qml = "import QtQuick 2.0\nListModel { id: lm; Component.onCompleted: lm.sync() }";
    QString error = "file:dummy.qml:2:1: QML ListModel: List sync() can only be called from a WorkerScript";

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(),
                      QUrl::fromLocalFile(QString("dummy.qml")));
    QTest::ignoreMessage(QtWarningMsg,error.toUtf8());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY2(obj, qPrintable(component.errorString()));
}

/*
    Test model changes from set() are available to the view
*/
void tst_qqmllistmodel::set_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qqmllistmodel::set()
{
    QFETCH(bool, dynamicRoles);

    QQmlEngine engine;
    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextProperty("model", &model);

    RUNEXPR("model.append({test:false})");
    RUNEXPR("model.set(0, {test:true})");

    QCOMPARE(RUNEXPR("model.get(0).test").toBool(), true); // triggers creation of model cache
    QCOMPARE(model.data(0, 0), QVariant::fromValue(true));

    RUNEXPR("model.set(0, {test:false})");
    QCOMPARE(RUNEXPR("model.get(0).test").toBool(), false); // tests model cache is updated
    QCOMPARE(model.data(0, 0), QVariant::fromValue(false));

    QString warning = QString::fromLatin1("<Unknown File>: Can't create role for unsupported data type");
    if (isValidErrorMessage(warning, dynamicRoles))
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());
    QVariant invalidData = QColor();
    model.setProperty(0, "test", invalidData);
}

/*
    Test model changes on values returned by get() are available to the view
*/
void tst_qqmllistmodel::get()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick 2.0\n"
        "ListModel {}\n", QUrl());
    std::unique_ptr<QQmlListModel> model { qobject_cast<QQmlListModel*>(component.create()) };
    model->setDynamicRoles(dynamicRoles);
    engine.rootContext()->setContextProperty("model", model.get());

    RUNEXPR("model.append({roleA: 100})");
    RUNEXPR("model.append({roleA: 200, roleB: 400})");
    RUNEXPR("model.append({roleA: 200, roleB: 400})");
    RUNEXPR("model.append({roleC: {} })");
    RUNEXPR("model.append({roleD: [ { a:1, b:2 }, { c: 3 } ] })");

    QSignalSpy spy(model.get(), SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)));
    QQmlExpression expr(engine.rootContext(), model.get(), expression);
    expr.evaluate();
    QVERIFY(!expr.hasError());

    int role = roleFromName(model.get(), roleName);
    QVERIFY(role >= 0);

    if (roleValue.typeId() == QMetaType::QVariantList) {
        const QVariantList &list = roleValue.toList();
        QVERIFY(compareVariantList(list, model->data(index, role)));
    } else {
        QCOMPARE(model->data(index, role), roleValue);
    }

    QCOMPARE(spy.size(), 1);

    QList<QVariant> spyResult = spy.takeFirst();
    QCOMPARE(spyResult.at(0).value<QModelIndex>(), model->index(index, 0, QModelIndex()));
    QCOMPARE(spyResult.at(1).value<QModelIndex>(), model->index(index, 0, QModelIndex()));  // only 1 item is modified at a time
    QCOMPARE(spyResult.at(2).value<QVector<int> >(), (QVector<int>() << role));
}

void tst_qqmllistmodel::get_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<QVariant>("roleValue");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
        bool dr = (i != 0);

        QTest::newRow("simple value") << "get(0).roleA = 500" << 0 << "roleA" << QVariant(500) << dr;
        QTest::newRow("simple value 2") << "get(1).roleB = 500" << 1 << "roleB" << QVariant(500) << dr;

        QVariantMap map;
        QVariantList list;
        map.clear(); map["a"] = 50; map["b"] = 500;
        list << map;
        map.clear(); map["c"] = 1000;
        list << map;
        QTest::newRow("list of objects") << "get(2).roleD = [{'a': 50, 'b': 500}, {'c': 1000}]" << 2 << "roleD" << QVariant::fromValue(list) << dr;
    }
}

/*
    Test that the tests run in get() also work for nested list data
*/
void tst_qqmllistmodel::get_nested()
{
    QFETCH(QString, expression);
    QFETCH(int, index);
    QFETCH(QString, roleName);
    QFETCH(QVariant, roleValue);
    QFETCH(bool, dynamicRoles);

    if (roleValue.typeId() == QMetaType::QVariantMap)
        return;

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick 2.0\n"
        "ListModel {}", QUrl());
    std::unique_ptr<QQmlListModel> model { qobject_cast<QQmlListModel*>(component.create()) };
    model->setDynamicRoles(dynamicRoles);
    QVERIFY(component.errorString().isEmpty());
    QQmlListModel *childModel;
    engine.rootContext()->setContextProperty("model", model.get());

    RUNEXPR("model.append({ listRoleA: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "] })\n");

    RUNEXPR("model.append({ listRoleA: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "],\n"
                            "listRoleB: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "],\n"
                            "listRoleC: [\n"
                            "{ roleA: 100 },\n"
                            "{ roleA: 200, roleB: 400 },\n"
                            "{ roleA: 200, roleB: 400 }, \n"
                            "{ roleC: {} }, \n"
                            "{ roleD: [ { a: 1, b:2 }, { c: 3 } ] } \n"
                            "] })\n");

    // Test setting the inner list data for:
    //  get(0).listRoleA
    //  get(1).listRoleA
    //  get(1).listRoleB
    //  get(1).listRoleC

    QList<std::pair<int, QString> > testData;
    testData << std::make_pair(0, QString("listRoleA"));
    testData << std::make_pair(1, QString("listRoleA"));
    testData << std::make_pair(1, QString("listRoleB"));
    testData << std::make_pair(1, QString("listRoleC"));

    for (int i=0; i<testData.size(); i++) {
        int outerListIndex = testData[i].first;
        QString outerListRoleName = testData[i].second;
        int outerListRole = roleFromName(model.get(), outerListRoleName);
        QVERIFY(outerListRole >= 0);

        childModel = qobject_cast<QQmlListModel*>(model->data(outerListIndex, outerListRole).value<QObject*>());
        QVERIFY(childModel);

        QString extendedExpression = QString("get(%1).%2.%3").arg(outerListIndex).arg(outerListRoleName).arg(expression);
        QQmlExpression expr(engine.rootContext(), model.get(), extendedExpression);

        QSignalSpy spy(childModel, SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)));
        expr.evaluate();
        QVERIFY(!expr.hasError());

        int role = roleFromName(childModel, roleName);
        QVERIFY(role >= 0);
        if (roleValue.typeId() == QMetaType::QVariantList) {
            QVERIFY(compareVariantList(roleValue.toList(), childModel->data(index, role)));
        } else {
            QCOMPARE(childModel->data(index, role), roleValue);
        }
        QCOMPARE(spy.size(), 1);

        QList<QVariant> spyResult = spy.takeFirst();
        QCOMPARE(spyResult.at(0).value<QModelIndex>(), childModel->index(index, 0, QModelIndex()));
        QCOMPARE(spyResult.at(1).value<QModelIndex>(), childModel->index(index, 0, QModelIndex()));  // only 1 item is modified at a time
        QCOMPARE(spyResult.at(2).value<QVector<int> >(), (QVector<int>() << role));
    }
}

void tst_qqmllistmodel::get_nested_data()
{
    get_data();
}

//QTBUG-13754
void tst_qqmllistmodel::crash_model_with_multiple_roles()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("multipleroles.qml"));
    std::unique_ptr<QObject> rootItem { component.create() };
    QVERIFY(component.errorString().isEmpty());
    QVERIFY(rootItem);
    QQmlListModel *model = rootItem->findChild<QQmlListModel*>("listModel");
    QVERIFY(model != nullptr);

    // used to cause a crash
    model->setProperty(0, "black", true);
}

void tst_qqmllistmodel::crash_model_with_unknown_roles()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("multipleroles.qml"));
    QScopedPointer<QObject> rootItem(component.create());
    QVERIFY(component.errorString().isEmpty());
    QVERIFY(rootItem != nullptr);
    QQmlListModel *model = rootItem->findChild<QQmlListModel*>("listModel");
    QVERIFY(model != nullptr);

    // used to cause a crash in debug builds
    model->index(0, 0, QModelIndex()).data(Qt::DisplayRole);
    model->index(0, 0, QModelIndex()).data(Qt::UserRole);
}

//QTBUG-35639
void tst_qqmllistmodel::crash_model_with_dynamic_roles()
{
    {
        // setting a dynamic role to a QObject value, then triggering dtor
        QQmlEngine eng;
        QQmlComponent component(&eng, testFileUrl("dynamicroles.qml"));
        std::unique_ptr<QObject> rootItem { component.create() };
        qWarning() << component.errorString();
        QVERIFY(component.errorString().isEmpty());
        QVERIFY(rootItem.get() != 0);
        QQmlListModel *model = rootItem->findChild<QQmlListModel*>("listModel");
        QVERIFY(model != 0);

        QMetaObject::invokeMethod(model, "appendNewElement");

        model->setProperty(0, "obj", QVariant::fromValue<QObject*>(std::make_unique<QObject>().get()));

        // Let root item go out of scope to let the model dtor run.
        // Previously, this would crash as it attempted to delete the already-deleted temporary QObject.
    }

    {
        // setting a dynamic role to a QObject value, then triggering
        // DynamicRoleModelNode::updateValues() to trigger unsafe qobject_cast
        QQmlEngine eng;
        QQmlComponent component(&eng, testFileUrl("dynamicroles.qml"));
        std::unique_ptr<QObject> rootItem { component.create() };
        qWarning() << component.errorString();
        QVERIFY(component.errorString().isEmpty());
        QVERIFY(rootItem.get() != 0);
        QQmlListModel *model = rootItem->findChild<QQmlListModel*>("listModel");
        QVERIFY(model != 0);

        QMetaObject::invokeMethod(model, "appendNewElement");

        model->setProperty(0, "obj", QVariant::fromValue<QObject*>(std::make_unique<QObject>().get()));

        QMetaObject::invokeMethod(model, "setElementAgain");
    }

    {
        // setting a dynamic role to a QObject value, then triggering
        // DynamicRoleModelNodeMetaObject::propertyWrite()

        /*
           XXX TODO: I couldn't reproduce this one simply - I think it
           requires a WorkerScript sync() call, and that's non-trivial.
           I thought I could do it simply via:

        QQmlEngine eng;
        QQmlComponent component(&eng, testFileUrl("dynamicroles.qml"));
        QObject *rootItem = component.create();
        qWarning() << component.errorString();
        QVERIFY(component.errorString().isEmpty());
        QVERIFY(rootItem != 0);
        QQmlListModel *model = rootItem->findChild<QQmlListModel*>("listModel");
        QVERIFY(model != 0);

        QMetaObject::invokeMethod(model, "appendNewElement");

        QObject *testObj = new QObject;
        model->setProperty(0, "obj", QVariant::fromValue<QObject*>(testObj));
        delete testObj;
        QObject *testObj2 = new QObject;
        model->setProperty(0, "obj", QVariant::fromValue<QObject*>(testObj2));

           But it turns out that that doesn't work (the setValue() call within
           setProperty() doesn't seem to trigger the right codepath, for some
           reason), and you can't trigger it manually via:

        QObject *testObj2 = new QObject;
        void *a[] = { testObj2, 0 };
        QMetaObject::metacall(dynamicNodeModel, QMetaObject::WriteProperty, 0, a);

           because the dynamicNodeModel for that index cannot be retrieved
           using the public API.

           But, anyway, I think the above two test cases are sufficient to
           show that QObject* values should be guarded internally.
        */
    }
}

//QTBUG-15190
void tst_qqmllistmodel::set_model_cache()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("setmodelcachelist.qml"));
    QScopedPointer<QObject> model(component.create());
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(model != nullptr);
    QVERIFY(model->property("ok").toBool());
}

void tst_qqmllistmodel::property_changes()
{
    QFETCH(QString, script_setup);
    QFETCH(QString, script_change);
    QFETCH(QString, roleName);
    QFETCH(int, listIndex);
    QFETCH(bool, itemsChanged);
    QFETCH(QString, testExpression);
    QFETCH(bool, dynamicRoles);

    QQmlEngine engine;
    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine::setContextForObject(&model, engine.rootContext());
    engine.rootContext()->setContextObject(&model);

    QQmlExpression expr(engine.rootContext(), &model, script_setup);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));

    QString signalHandler = QQmlSignalNames::propertyNameToChangedHandlerName(roleName);
    QString qml = "import QtQuick 2.0\n"
                  "Connections {\n"
                        "property bool gotSignal: false\n"
                        "target: model.get(" + QString::number(listIndex) + ")\n"
                        + signalHandler + ": gotSignal = true\n"
                  "}\n";

    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), QUrl::fromLocalFile(""));
    engine.rootContext()->setContextProperty("model", &model);
    std::unique_ptr<QObject> connectionsObject { component.create() };
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));

    QSignalSpy spyItemsChanged(&model, SIGNAL(dataChanged(QModelIndex,QModelIndex,QList<int>)));

    expr.setExpression(script_change);
    expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));

    // test the object returned by get() emits the correct signals
    QCOMPARE(connectionsObject->property("gotSignal").toBool(), itemsChanged);

    // test itemsChanged() is emitted correctly
    if (itemsChanged) {
        QCOMPARE(spyItemsChanged.size(), 1);
        QCOMPARE(spyItemsChanged.at(0).at(0).value<QModelIndex>(), model.index(listIndex, 0, QModelIndex()));
        QCOMPARE(spyItemsChanged.at(0).at(1).value<QModelIndex>(), model.index(listIndex, 0, QModelIndex()));
    } else {
        QCOMPARE(spyItemsChanged.size(), 0);
    }

    expr.setExpression(testExpression);
    QCOMPARE(expr.evaluate().toBool(), true);
}

void tst_qqmllistmodel::property_changes_data()
{
    QTest::addColumn<QString>("script_setup");
    QTest::addColumn<QString>("script_change");
    QTest::addColumn<QString>("roleName");
    QTest::addColumn<int>("listIndex");
    QTest::addColumn<bool>("itemsChanged");
    QTest::addColumn<QString>("testExpression");
    QTest::addColumn<bool>("dynamicRoles");

    for (int i=0 ; i < 2 ; ++i) {
        bool dr = (i != 0);

        QTest::newRow("set: plain") << "append({'a':123, 'b':456, 'c':789});" << "set(0,{'b':123});"
                << "b" << 0 << true << "get(0).b == 123" << dr;
        QTest::newRow("setProperty: plain") << "append({'a':123, 'b':456, 'c':789});" << "setProperty(0, 'b', 123);"
                << "b" << 0 << true << "get(0).b == 123" << dr;

        QTest::newRow("set: plain, no changes") << "append({'a':123, 'b':456, 'c':789});" << "set(0,{'b':456});"
                << "b" << 0 << false << "get(0).b == 456" << dr;
        QTest::newRow("setProperty: plain, no changes") << "append({'a':123, 'b':456, 'c':789});" << "setProperty(0, 'b', 456);"
                << "b" << 0 << false << "get(0).b == 456" << dr;

        QTest::newRow("set: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "set(1, {'a':456});"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("setProperty: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "setProperty(1, 'a', 456);"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("get: inserted item")
                << "{append({'a':123, 'b':456, 'c':789}); get(0); insert(0, {'a':0, 'b':0, 'c':0});}"
                << "get(1).a = 456;"
                << "a" << 1 << true << "get(1).a == 456" << dr;
        QTest::newRow("set: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "set(0, {'a':456});"
                << "a" << 0 << true << "get(0).a == 456" << dr;
        QTest::newRow("setProperty: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "setProperty(0, 'a', 456);"
                << "a" << 0 << true << "get(0).a == 456" << dr;
        QTest::newRow("get: removed item")
                << "{append({'a':0, 'b':0, 'c':0}); append({'a':123, 'b':456, 'c':789}); get(1); remove(0);}"
                << "get(0).a = 456;"
                << "a" << 0 << true << "get(0).a == 456" << dr;

        // Following tests only call set() since setProperty() only allows plain
        // values, not lists, as the argument.
        // Note that when a list is changed, itemsChanged() is currently always
        // emitted regardless of whether it actually changed or not.

        QTest::newRow("nested-set: list, new size") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2" << dr;

        QTest::newRow("nested-set: list, empty -> non-empty") << "append({'a':123, 'b':[], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, non-empty -> empty") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[]});"
                << "b" << 0 << true << "get(0).b.count == 0" << dr;

        QTest::newRow("nested-set: list, same size, different values") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':222},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 222 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, no changes") << "append({'a':123, 'b':[{'a':1},{'a':2},{'a':3}], 'c':789});" << "set(0,{'b':[{'a':1},{'a':2},{'a':3}]});"
                << "b" << 0 << true << "get(0).b.get(0).a == 1 && get(0).b.get(1).a == 2 && get(0).b.get(2).a == 3" << dr;

        QTest::newRow("nested-set: list, no changes, empty") << "append({'a':123, 'b':[], 'c':789});" << "set(0,{'b':[]});"
                << "b" << 0 << true << "get(0).b.count == 0" << dr;
    }
}

void tst_qqmllistmodel::clear_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qqmllistmodel::clear()
{
    QFETCH(bool, dynamicRoles);

    QQmlEngine engine;
    QQmlListModel model;
    model.setDynamicRoles(dynamicRoles);
    QQmlEngine::setContextForObject(&model, engine.rootContext());
    engine.rootContext()->setContextProperty("model", &model);

    model.clear();
    QCOMPARE(model.count(), 0);

    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    QCOMPARE(model.count(), 1);

    model.clear();
    QCOMPARE(model.count(), 0);

    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\"})");
    QCOMPARE(model.count(), 2);

    model.clear();
    QCOMPARE(model.count(), 0);

    // clearing does not remove the roles
    RUNEXPR("model.append({propertyA: \"value a\", propertyB: \"value b\", propertyC: \"value c\"})");
    QHash<int, QByteArray> roleNames = model.roleNames();
    model.clear();
    QCOMPARE(model.count(), 0);
    QCOMPARE(model.roleNames(), roleNames);
    QCOMPARE(roleNames[0], QByteArray("propertyA"));
    QCOMPARE(roleNames[1], QByteArray("propertyB"));
    QCOMPARE(roleNames[2], QByteArray("propertyC"));
}

void tst_qqmllistmodel::signal_handlers_data()
{
    QTest::addColumn<bool>("dynamicRoles");

    QTest::newRow("staticRoles") << false;
    QTest::newRow("dynamicRoles") << true;
}

void tst_qqmllistmodel::signal_handlers()
{
    QFETCH(bool, dynamicRoles);

    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("signalhandlers.qml"));
    QScopedPointer<QObject> model(component.create());
    QQmlListModel *lm = qobject_cast<QQmlListModel *>(model.data());
    QVERIFY(lm != nullptr);
    lm->setDynamicRoles(dynamicRoles);
    QVERIFY2(component.errorString().isEmpty(), qPrintable(component.errorString()));
    QVERIFY(model != nullptr);
    QVERIFY(model->property("ok").toBool());
}

void tst_qqmllistmodel::role_mode_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<int>("result");
    QTest::addColumn<QString>("warning");

    QTest::newRow("default0") << "{dynamicRoles}" << 0 << "";
    QTest::newRow("default1") << "{append({'a':1});dynamicRoles}" << 0 << "";

    QTest::newRow("enableDynamic0") << "{dynamicRoles=true;dynamicRoles}" << 1 << "";
    QTest::newRow("enableDynamic1") << "{append({'a':1});dynamicRoles=true;dynamicRoles}" << 0 << "<Unknown File>: QML ListModel: unable to enable dynamic roles as this model is not empty";
    QTest::newRow("enableDynamic2") << "{dynamicRoles=true;append({'a':1});dynamicRoles=false;dynamicRoles}" << 1 << "<Unknown File>: QML ListModel: unable to enable static roles as this model is not empty";
}

void tst_qqmllistmodel::role_mode()
{
    QFETCH(QString, script);
    QFETCH(int, result);
    QFETCH(QString, warning);

    QQmlEngine engine;
    QQmlListModel model;
    QQmlEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextObject(&model);
    QQmlExpression e(engine.rootContext(), &model, script);
    if (!warning.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    int actual = e.evaluate().toInt();
    if (e.hasError())
        qDebug() << e.error(); // errors not expected

    QCOMPARE(actual,result);
}

void tst_qqmllistmodel::string_to_list_crash()
{
    QQmlEngine engine;
    QQmlListModel model;
    QQmlEngine::setContextForObject(&model,engine.rootContext());
    engine.rootContext()->setContextObject(&model);
    QString script = QLatin1String("{append({'a':'data'});get(0).a = [{'x':123}]}");
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: Can't assign to existing role 'a' of different type [String -> List]");
    QQmlExpression e(engine.rootContext(), &model, script);
    // Don't crash!
    e.evaluate();
}

void tst_qqmllistmodel::empty_element_warning_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<bool>("warning");

    QTest::newRow("empty") << "import QtQuick 2.0\nListModel {}" << false;
    QTest::newRow("withid") << "import QtQuick 2.0\nListModel { id: model }" << false;
    QTest::newRow("emptyElement") << "import QtQuick 2.0\nListModel { ListElement {} }" << true;
    QTest::newRow("emptyElements") << "import QtQuick 2.0\nListModel { ListElement {} ListElement {} }" << true;
    QTest::newRow("role1") << "import QtQuick 2.0\nListModel { ListElement {a:1} }" << false;
    QTest::newRow("role2") << "import QtQuick 2.0\nListModel { ListElement {} ListElement {a:1} ListElement {} }" << false;
    QTest::newRow("role3") << "import QtQuick 2.0\nListModel { ListElement {} ListElement {a:1} ListElement {b:2} }" << false;
}

void tst_qqmllistmodel::empty_element_warning()
{
    QFETCH(QString, qml);
    QFETCH(bool, warning);

    if (warning) {
        QString warningString = QLatin1String("file:dummy.qml:2:1: QML ListModel: All ListElement declarations are empty, no roles can be created unless dynamicRoles is set.");
        QTest::ignoreMessage(QtWarningMsg, warningString.toLatin1());
    }

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml.toUtf8(), QUrl::fromLocalFile(QString("dummy.qml")));
    QVERIFY(!component.isError());

    std::unique_ptr<QObject> obj { component.create() };
    QVERIFY(obj);
}

void tst_qqmllistmodel::datetime_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QDateTime>("expected");

    QDateTime dt;
    QDateTime dt0(QDate(1900,  1,  2), QTime( 8, 14));
    QDateTime dt1(QDate(2000, 11, 22), QTime(10, 45));

    QTest::newRow("dt0") << "{append({'date':dt0});get(0).date}" << dt0;
    QTest::newRow("dt1") << "{append({'date':dt0});get(0).date=dt1;get(0).date}" << dt1;
    QTest::newRow("dt2") << "{append({'date':dt0});set(0,{'date':dt1});get(0).date}" << dt1;
    QTest::newRow("dt3") << "{append({'date':dt0});get(0).date=undefined;get(0).date}" << dt;
    QTest::newRow("dt4") << "{append({'date':dt0});setProperty(0,'date',dt1);get(0).date}" << dt1;
}

void tst_qqmllistmodel::url()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("urls.qml"));
    QScopedPointer<QObject> o {comp.create()};
    QVERIFY(o);
    QCOMPARE(o->property("result1").toUrl(), QUrl("http://qt.io"));
    QCOMPARE(o->property("result2").toUrl(), QUrl("http://qt-project.org"));
    QCOMPARE(o->property("alive1").toString(), QStringLiteral("indeed"));
    QCOMPARE(o->property("alive2").toString(), QStringLiteral("and kicking"));
}

void tst_qqmllistmodel::datetime()
{
    QFETCH(QString, qml);
    QFETCH(QDateTime, expected);

    QQmlEngine engine;
    QQmlListModel model;
    QQmlEngine::setContextForObject(&model,engine.rootContext());
    QDateTime dt0(QDate(1900,  1,  2), QTime( 8, 14));
    QDateTime dt1(QDate(2000, 11, 22), QTime(10, 45));
    engine.rootContext()->setContextProperty("dt0", dt0);
    engine.rootContext()->setContextProperty("dt1", dt1);
    engine.rootContext()->setContextObject(&model);
    QQmlExpression e(engine.rootContext(), &model, qml);
    QVariant result = e.evaluate();
    QDateTime dtResult = result.toDateTime();
    QCOMPARE(expected, dtResult);
}

class RowTester : public QObject
{
    Q_OBJECT
public:
    RowTester(QAbstractItemModel *model) : QObject(model), model(model)
    {
        reset();
        connect(model, SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(rowsAboutToBeInserted()));
        connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted()));
        connect(model, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(rowsAboutToBeRemoved()));
        connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsRemoved()));
        connect(model, SIGNAL(rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(rowsAboutToBeMoved()));
        connect(model, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(rowsMoved()));
    }

    void reset()
    {
        rowsAboutToBeInsertedCalls = 0;
        rowsAboutToBeInsertedCount = 0;
        rowsInsertedCalls = 0;
        rowsInsertedCount = 0;
        rowsAboutToBeRemovedCalls = 0;
        rowsAboutToBeRemovedCount = 0;
        rowsRemovedCalls = 0;
        rowsRemovedCount = 0;
        rowsAboutToBeMovedCalls = 0;
        rowsAboutToBeMovedData.clear();
        rowsMovedCalls = 0;
        rowsMovedData.clear();
    }

    int rowsAboutToBeInsertedCalls;
    int rowsAboutToBeInsertedCount;
    int rowsInsertedCalls;
    int rowsInsertedCount;
    int rowsAboutToBeRemovedCalls;
    int rowsAboutToBeRemovedCount;
    int rowsRemovedCalls;
    int rowsRemovedCount;
    int rowsAboutToBeMovedCalls;
    QVariantList rowsAboutToBeMovedData;
    int rowsMovedCalls;
    QVariantList rowsMovedData;

private slots:
    void rowsAboutToBeInserted()
    {
        rowsAboutToBeInsertedCalls++;
        rowsAboutToBeInsertedCount = model->rowCount();
    }

    void rowsInserted()
    {
        rowsInsertedCalls++;
        rowsInsertedCount = model->rowCount();
    }

    void rowsAboutToBeRemoved()
    {
        rowsAboutToBeRemovedCalls++;
        rowsAboutToBeRemovedCount = model->rowCount();
    }

    void rowsRemoved()
    {
        rowsRemovedCalls++;
        rowsRemovedCount = model->rowCount();
    }

    void rowsAboutToBeMoved()
    {
        rowsAboutToBeMovedCalls++;
        for (int i = 0; i < model->rowCount(); ++i)
            rowsAboutToBeMovedData += model->data(model->index(i, 0), 0);
    }

    void rowsMoved()
    {
        rowsMovedCalls++;
        for (int i = 0; i < model->rowCount(); ++i)
            rowsMovedData += model->data(model->index(i, 0), 0);
    }

private:
    QAbstractItemModel *model;
};

void tst_qqmllistmodel::about_to_be_signals()
{
    QQmlEngine engine;
    QQmlListModel model;
    QQmlEngine::setContextForObject(&model,engine.rootContext());

    RowTester tester(&model);

    QQmlExpression e1(engine.rootContext(), &model, "{append({'test':0})}");
    e1.evaluate();

    QCOMPARE(tester.rowsAboutToBeInsertedCalls, 1);
    QCOMPARE(tester.rowsAboutToBeInsertedCount, 0);
    QCOMPARE(tester.rowsInsertedCalls, 1);
    QCOMPARE(tester.rowsInsertedCount, 1);

    QQmlExpression e2(engine.rootContext(), &model, "{append({'test':1})}");
    e2.evaluate();

    QCOMPARE(tester.rowsAboutToBeInsertedCalls, 2);
    QCOMPARE(tester.rowsAboutToBeInsertedCount, 1);
    QCOMPARE(tester.rowsInsertedCalls, 2);
    QCOMPARE(tester.rowsInsertedCount, 2);

    QQmlExpression e3(engine.rootContext(), &model, "{move(0, 1, 1)}");
    e3.evaluate();

    QCOMPARE(tester.rowsAboutToBeMovedCalls, 1);
    QCOMPARE(tester.rowsAboutToBeMovedData, QVariantList() << 0.0 << 1.0);
    QCOMPARE(tester.rowsMovedCalls, 1);
    QCOMPARE(tester.rowsMovedData, QVariantList() << 1.0 << 0.0);

    QQmlExpression e4(engine.rootContext(), &model, "{remove(0, 2)}");
    e4.evaluate();

    QCOMPARE(tester.rowsAboutToBeRemovedCalls, 1);
    QCOMPARE(tester.rowsAboutToBeRemovedCount, 2);
    QCOMPARE(tester.rowsRemovedCalls, 1);
    QCOMPARE(tester.rowsRemovedCount, 0);
}

void tst_qqmllistmodel::modify_through_delegate()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
        "import QtQuick 2.0\n"
        "Item {\n"
        "   ListModel {\n"
        "       id: testModel\n"
        "       objectName: \"testModel\"\n"
        "       ListElement { name: \"Joe\"; age: 22 }\n"
        "       ListElement { name: \"Doe\"; age: 33 }\n"
        "   }\n"
        "   ListView {\n"
        "       height: 100\n" \
        "       width: 100\n" \
        "       model: testModel\n"
        "       delegate: Item {\n"
        "           Component.onCompleted: model.age = 18;\n"
        "       }\n"
        "   }\n"
        "}\n", QUrl());

    QScopedPointer<QObject> scene(component.create());
    QQmlListModel *model = scene->findChild<QQmlListModel*>("testModel");

    const QHash<int, QByteArray> roleNames = model->roleNames();

    QCOMPARE(model->data(model->index(0, 0, QModelIndex()), roleNames.key("age")).toInt(), 18);
    QCOMPARE(model->data(model->index(1, 0, QModelIndex()), roleNames.key("age")).toInt(), 18);
}

void tst_qqmllistmodel::bindingsOnGetResult()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("bindingsOnGetResult.qml"));
    QVERIFY2(!component.isError(), qPrintable(component.errorString()));

    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());

    QVERIFY(obj->property("success").toBool());
}

void tst_qqmllistmodel::stringifyModelEntry()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                      "import QtQuick 2.0\n"
                      "Item {\n"
                      "   ListModel {\n"
                      "       id: testModel\n"
                      "       objectName: \"testModel\"\n"
                      "       ListElement { name: \"Joe\"; age: 22 }\n"
                      "   }\n"
                      "}\n", QUrl());
    QScopedPointer<QObject> scene(component.create());
    QQmlListModel *model = scene->findChild<QQmlListModel*>("testModel");
    QQmlExpression expr(engine.rootContext(), model, "JSON.stringify(get(0));");
    QVariant v = expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));
    const QString expectedString = QStringLiteral("{\"age\":22,\"name\":\"Joe\"}");
    QCOMPARE(v.toString(), expectedString);
}

void tst_qqmllistmodel::qobjectTrackerForDynamicModelObjects()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                      "import QtQuick 2.0\n"
                      "Item {\n"
                      "   ListModel {\n"
                      "       id: testModel\n"
                      "       objectName: \"testModel\"\n"
                      "       ListElement { name: \"Joe\"; age: 22 }\n"
                      "   }\n"
                      "}\n", QUrl());
    QScopedPointer<QObject> scene(component.create());
    QQmlListModel *model = scene->findChild<QQmlListModel*>("testModel");
    QQmlExpression expr(engine.rootContext(), model, "get(0);");
    QVariant v = expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));

    QObject *obj = v.value<QObject*>();
    QVERIFY(obj);

    QQmlData *ddata = QQmlData::get(obj, /*create*/false);
    QVERIFY(ddata);
    QVERIFY(!ddata->jsWrapper.isNullOrUndefined());
}

void tst_qqmllistmodel::crash_append_empty_array()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                "import QtQuick 2.0\n"
                "Item {\n"
                "   ListModel {\n"
                "      id: testModel\n"
                "      objectName: \"testModel\""
                "   }\n"
                "}\n", QUrl());
    QScopedPointer<QObject> scene(component.create());
    QQmlListModel *model = scene->findChild<QQmlListModel*>("testModel");
    QSignalSpy spy(model, &QQmlListModel::rowsAboutToBeInserted);
    QQmlExpression expr(engine.rootContext(), model, "append(new Array())");
    expr.evaluate();
    QVERIFY2(!expr.hasError(), qPrintable(expr.error().toString()));
    QCOMPARE(spy.size(), 0);
}

void tst_qqmllistmodel::dynamic_roles_crash_QTBUG_38907()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("qtbug38907.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QQuickItem> item(qobject_cast<QQuickItem*>(component.create()));
    QVERIFY(item != 0);

    QVariant retVal;

    QMetaObject::invokeMethod(item.data(),
                              "exec",
                              Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, retVal));

    QVERIFY(retVal.toBool());
}

void tst_qqmllistmodel::nestedListModelIteration()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    QTest::ignoreMessage(QtMsgType::QtDebugMsg ,R"({"subItems":[{"a":1,"b":0,"c":0},{"a":0,"b":2,"c":0},{"a":0,"b":0,"c":3}]})");
    component.setData(
            R"(import QtQuick 2.5
            Item {
                visible: true
                width: 640
                height: 480
                ListModel {
                    id : model
                }
                Component.onCompleted: {
                        var tempData = {
                            subItems: [{a: 1}, {b: 2}, {c: 3}]
                        }
                        model.insert(0, tempData)
                        console.log(JSON.stringify(model.get(0)))
                }
            })",
            QUrl());
    delete component.create();
}

// QTBUG-63569
void tst_qqmllistmodel::undefinedAppendShouldCauseError()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
            R"(import QtQuick 2.5
            Item {
                width: 640
                height: 480
                ListModel {
                    id : model
                }
                Component.onCompleted: {
                        var tempData = {
                            faulty: undefined
                        }
                        model.insert(0, tempData)
                        tempData.faulty = null
                        model.insert(0, tempData)
                }
            })",
            QUrl());
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: faulty is undefined. Adding an object with a undefined member does not create a role for it.");
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: faulty is null. Adding an object with a null member does not create a role for it.");
    delete component.create();
}

// QTBUG-89173
void tst_qqmllistmodel::nullPropertyCrash()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
            R"(import QtQuick 2.15
            ListView {
                model: ListModel { id: listModel }

                delegate: Item {}

                Component.onCompleted: {
                    listModel.append({"a": "value1", "b":[{"c":"value2"}]})
                    listModel.append({"a": "value2", "b":[{"c":null}]})
                }
            })",
            QUrl());
    QTest::ignoreMessage(QtMsgType::QtWarningMsg, "<Unknown File>: c is null. Adding an object with a null member does not create a role for it.");
    delete component.create();
}

// QTBUG-91390
void tst_qqmllistmodel::objectDestroyed()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
            R"(import QtQuick
                   ListModel {
                       id: model
                       Component.onCompleted: { model.append({"a": contextObject}); }
                   })",
            QUrl());

    std::unique_ptr<QObject> obj = std::make_unique<QObject>();
    connect(obj.get(), &QObject::destroyed, [&]() { obj.release(); });

    engine.rootContext()->setContextProperty(u"contextObject"_s, obj.get());
    engine.setObjectOwnership(obj.get(), QJSEngine::JavaScriptOwnership);

    delete component.create();
    QVERIFY(obj);
    engine.collectGarbage();
    QTest::qSleep(250);
    QVERIFY(obj);
    engine.evaluate(u"model.clear();"_s);
    engine.collectGarbage();
    QTRY_VERIFY(!obj);
}

void tst_qqmllistmodel::destroyObject()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
                R"(import QtQuick
                   ListModel {
                       id: model
                       Component.onCompleted: { model.append({"a": contextObject}); }
                   })",
                QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> element(new QObject);
    engine.rootContext()->setContextProperty(u"contextObject"_s, element.data());

    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQmlListModel *model = qobject_cast<QQmlListModel *>(o.data());
    QVERIFY(model);
    QCOMPARE(model->count(), 1);
    QCOMPARE(model->get(0).property("a").toQObject(), element.data());
    element.reset();
    QCOMPARE(model->get(0).property("a").toQObject(), nullptr);
}

void tst_qqmllistmodel::emptyStringNotUndefined()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(
            R"(import QtQuick
                   ListModel {
                       id: model
                       Component.onCompleted: { model.append({"a": ""}); }
                   })",
            QUrl());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);
    auto lm = qobject_cast<QQmlListModel *>(root.get());
    QVERIFY(lm);
    QJSValue val = lm->get(0);
    QVERIFY(val.hasProperty("a"));
    val = val.property("a");
    QVERIFY(!val.isUndefined());
    QVERIFY(val.isString());
    QCOMPARE(val.toString(), QString());
}

void tst_qqmllistmodel::listElementWithTemplateString()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
    import QtQuick
    ListModel {
        ListElement {
            prop: `test`
        }
    })", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());
}

//QTBUG-95895
void tst_qqmllistmodel::destroyComponentObject()
{
    QQmlEngine eng;
    QQmlComponent component(&eng, testFileUrl("destroyObject.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> obj(component.create());
    QVERIFY(!obj.isNull());
    QQmlListModel *list = qvariant_cast<QQmlListModel *>(obj->property("projects"));
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 1);
    QPointer<QObject> created(qvariant_cast<QObject *>(obj->property("object")));
    QVERIFY(!created.isNull());
    QCOMPARE(list->get(0).property("obj").toQObject(), created.data());
    QVariant retVal;
    QMetaObject::invokeMethod(obj.data(),
                              "destroy",
                               Qt::DirectConnection,
                               Q_RETURN_ARG(QVariant, retVal));
    QVERIFY(retVal.toBool());
    QTRY_VERIFY(created.isNull());
    QTRY_VERIFY(list->get(0).property("obj").isNull());
    QCOMPARE(list->count(), 1);
}

// Used for objectOwnershipFlip
class TestItem : public QQuickItem
{
    Q_OBJECT
public:
    // To trigger QQmlData::setImplicitDestructible through QV4::CallArgument::toValue
    Q_INVOKABLE TestItem* dummy() { return this; }
};

void tst_qqmllistmodel::objectOwnershipFlip()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("objectOwnership.qml"));
    QVERIFY(!component.isError());
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());
    QQmlListModel *model = root->findChild<QQmlListModel*>("listModel");
    QVERIFY(model != nullptr);

    QScopedPointer<TestItem> item(new TestItem());
    item->setObjectName("cppOwnedItem");
    QJSEngine::setObjectOwnership(item.data(), QJSEngine::CppOwnership);
    QCOMPARE(QJSEngine::objectOwnership(item.data()), QJSEngine::CppOwnership);

    engine.rootContext()->setContextProperty("cppOwnedItem", item.data());

    QMetaObject::invokeMethod(model, "addItem");
    QCOMPARE(model->count(), 1);

    QMetaObject::invokeMethod(root.data(), "checkItem");

    QCOMPARE(QJSEngine::objectOwnership(item.data()), QJSEngine::CppOwnership);
}

void tst_qqmllistmodel::enumsInListElement()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("enumsInListElement.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QQuickListView *listView = qobject_cast<QQuickListView *>(root.data());
    QVERIFY(listView);
    QCOMPARE(listView->count(), 3);
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(listView->itemAtIndex(i)->property("text"), QVariant(QString::number(i)));
    }
}

void tst_qqmllistmodel::protectQObjectFromGC()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("protectQObjectFromGC.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(!root.isNull());

    QQmlListModel *listModel = qobject_cast<QQmlListModel *>(root.data());
    QVERIFY(listModel);
    QCOMPARE(listModel->count(), 10);

    for (int i = 0; i < 10; ++i) {
        QObject *element = qjsvalue_cast<QObject *>(listModel->get(i).property("path"));
        QVERIFY(element);
        QCOMPARE(element->property("name").toString(), QString::number(i));
    }
}

static QVariantList createLast7Days()
{
    QVariantList last7DaysList;
    for (int i = 0; i < 7; i++) {
        QVariantMap map;
        map.insert("_day", i);
        last7DaysList.append(map);
    }
    return last7DaysList;
}

static QVariantList createWeekChartModels()
{
    QVariantList list;
    for (int i = 0; i < 4; i++) {
        QVariantMap map;
        map.insert("_week", createLast7Days());
        list.append(map);
    }
    return list;
}

static QVariantList createVariantModel()
{
    QVariantMap element1;
    element1.insert("_headline", "Element 1");
    element1.insert("_weeks", createWeekChartModels());

    QVariantMap element2;
    element2.insert("_headline", "Element 2");
    element2.insert("_weeks", createWeekChartModels());

    QVariantMap element3;
    element3.insert("_headline", "Element 3");
    element3.insert("_weeks", createWeekChartModels());

    QVariantList list;
    list.append(element1);
    list.append(element2);
    list.append(element3);

    return list;
}

class Day : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int _day READ _day CONSTANT)
public:
    Day(int day, QObject *parent = nullptr) : QObject(parent), day(day) {}
    int _day() const { return day; }
private:
    int day = 0;
};

class Week : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Day> _week READ _week)
public:
    Week(QObject *parent = nullptr) : QObject(parent)
    {
        for (int i = 0; i < 7; ++i)
            week.append(new Day(i, this));
    }

    QQmlListProperty<Day> _week() { return QQmlListProperty<Day>(this, &week); }

private:
    QList<Day *> week;
};

class Month : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Week> _weeks READ _weeks)
    Q_PROPERTY(QString _headline READ _headline CONSTANT)
public:

    Month(int i, QObject *parent = nullptr)
        : QObject(parent)
        , headline(QLatin1String("Element ") + QString::number(i))
    {
        for (int i = 0; i < 4; ++i)
            weeks.append(new Week(this));
    }

    QQmlListProperty<Week> _weeks() { return QQmlListProperty<Week>(this, &weeks); }
    QString _headline() const { return headline; }

private:
    QList<Week *> weeks;
    QString headline;
};

static void verifyLists(const QVariantList &list, QQuickRepeater *topLevel)
{
    QVERIFY(topLevel);
    QCOMPARE(topLevel->count(), 3);

    for (int month = 0; month < 3; ++month) {
        const QVariantMap monthData = list[month].toMap();
        const QQuickItem *monthItem = topLevel->itemAt(month);
        QCOMPARE(monthItem->objectName(), monthData["_headline"].toString());
        const QQuickRepeater *monthRepeater = monthItem->findChild<QQuickRepeater *>("month");
        QVERIFY(monthRepeater);
        QCOMPARE(monthRepeater->count(), 4);
        const QVariantList weekList = monthData["_weeks"].toList();
        for (int week = 0; week < 4; ++week) {
            const QVariantList weekData = weekList[week].toMap()["_week"].toList();
            const QQuickItem *weekItem = monthRepeater->itemAt(week);
            QCOMPARE(weekItem->objectName(), QString::number(week));
            const QQuickRepeater *weekRepeater = weekItem->findChild<QQuickRepeater *>("week");
            QVERIFY(weekRepeater);
            QCOMPARE(weekRepeater->count(), 7);
            for (int day = 0; day < 7; ++day) {
                const QVariantMap dayData = weekData[day].toMap();
                const QQuickItem *dayItem = weekRepeater->itemAt(day);
                QCOMPARE(dayItem->objectName(), dayData["_day"]);
            }
        }
    }
}

void tst_qqmllistmodel::nestedLists()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("nestedLists.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQuickRepeater *topLevel = o->findChild<QQuickRepeater *>("topLevel");

    const QVariantList list = createVariantModel();
    QMetaObject::invokeMethod(o.data(), "load", Q_ARG(QVariant, QVariant::fromValue(list)));
    verifyLists(list, topLevel);

    const QObjectList objects {
        new Month(1, o.data()),
        new Month(2, o.data()),
        new Month(3, o.data())
    };

    QMetaObject::invokeMethod(o.data(), "load", Q_ARG(QVariant, QVariant::fromValue(objects)));
    verifyLists(list, topLevel);
}

void tst_qqmllistmodel::deadModelData()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("deadModelData.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());

    QQmlListModel *l1 = o->property("l1").value<QQmlListModel *>();
    QVERIFY(l1);
    QQmlListModel *l2 = o->property("l2").value<QQmlListModel *>();
    QVERIFY(l2);

    QCOMPARE(l1->count(), 3);
    QCOMPARE(l2->count(), 3);

    for (int i = 0; i < 3; ++i) {
        QObject *i1 = qjsvalue_cast<QObject *>(l1->get(i));
        QVERIFY(i1);
        QCOMPARE(i1->property("ident").value<double>(), i + 1);
        QCOMPARE(i1->property("buttonText").value<QString>(),
                 QLatin1String("B %1").arg(QLatin1Char('0' + i + 1)));

        QObject *i2 = qjsvalue_cast<QObject *>(l2->get(i));
        QVERIFY(i2);
        QCOMPARE(i2->property("ident").value<double>(), i + 4);
        QCOMPARE(i2->property("buttonText").value<QString>(),
                 QLatin1String("B %1").arg(QLatin1Char('0' + i + 4)));
    }

    for (int i = 0; i < 6; ++i) {
        QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(".*: ident is undefined. Adding an object with a undefined "
                                   "member does not create a role for it."));
        QTest::ignoreMessage(
                QtWarningMsg,
                QRegularExpression(".*: buttonText is undefined. Adding an object with a undefined "
                                   "member does not create a role for it."));
    }

    QMetaObject::invokeMethod(o.data(), "swapCorpses");

    // We get default-created values for all the roles now.

    QCOMPARE(l1->count(), 3);
    QCOMPARE(l2->count(), 3);

    for (int i = 0; i < 3; ++i) {
        QObject *i1 = qjsvalue_cast<QObject *>(l1->get(i));
        QVERIFY(i1);
        QCOMPARE(i1->property("ident").value<double>(), double());
        QCOMPARE(i1->property("buttonText").value<QString>(), QString());

        QObject *i2 = qjsvalue_cast<QObject *>(l2->get(i));
        QVERIFY(i2);
        QCOMPARE(i2->property("ident").value<double>(), double());
        QCOMPARE(i2->property("buttonText").value<QString>(), QString());
    }
}

QTEST_MAIN(tst_qqmllistmodel)

#include "tst_qqmllistmodel.moc"
