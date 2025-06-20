// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtQuickTest/quicktest.h>

#include <QAbstractItemModelTester>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickheaderview_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <private/qquickheaderview_p_p.h>
#include <private/qquickheaderviewdelegate_p.h>

using namespace QQuickVisualTestUtils;

class TestTableModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex &index = QModelIndex()) const override
    {
        if (index.isValid())
            return 0;
        return m_rows;
    }
    virtual void setRowCount(int count)
    {
        beginResetModel();
        m_rows = count;
        emit rowCountChanged();
        endResetModel();
    }

    int columnCount(const QModelIndex &index = QModelIndex()) const override
    {
        if (index.isValid())
            return 0;
        return m_cols;
    }
    virtual void setColumnCount(int count)
    {
        beginResetModel();
        m_cols = count;
        emit columnCountChanged();
        endResetModel();
    }

    int indexValue(const QModelIndex &index) const
    {
        return index.row() + (index.column() * rowCount());
    }

    Q_INVOKABLE QModelIndex toQModelIndex(int serialIndex)
    {
        return createIndex(serialIndex % rowCount(), serialIndex / rowCount());
    }

    Q_INVOKABLE QVariant data(int row, int col)
    {
        return data(createIndex(row, col), Qt::DisplayRole);
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return QString("%1, %2, checked: %3 ")
                .arg(index.row())
                .arg(index.column())
                .arg(m_checkedCells.contains(indexValue(index)));
        case Qt::EditRole:
            return m_checkedCells.contains(indexValue(index));
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value,
        int role = Qt::EditRole) override
    {

        if (role != Qt::EditRole)
            return false;

        int i = indexValue(index);
        bool checked = value.toBool();
        if (checked == m_checkedCells.contains(i))
            return false;

        if (checked)
            m_checkedCells.insert(i);
        else
            m_checkedCells.remove(i);

        emit dataChanged(index, index, { role });
        return true;
    }

    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override
    {
        return {
            { Qt::DisplayRole, "display" },
            { Qt::EditRole, "edit" }
        };
    }

signals:
    void rowCountChanged();
    void columnCountChanged();

private:
    int m_rows = 0;
    int m_cols = 0;

    QSet<int> m_checkedCells;
};

class TestTableModelWithHeader : public TestTableModel {

    Q_OBJECT
public:
    enum Role {
        CustomRole = Qt::UserRole
    };

    void setRowCount(int count) override
    {
        vData.resize(count);
        TestTableModel::setRowCount(count);
    }

    void setColumnCount(int count) override
    {
        hData.resize(count);
        TestTableModel::setColumnCount(count);
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch (role) {
        case CustomRole:
            return QString("%1-%2").arg(index.column()).arg(index.row());
        default:
            return TestTableModel::data(index, role);
        }
    }

    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override
    {
        auto sectionCount = orientation == Qt::Horizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return QVariant();
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            auto &data = orientation == Qt::Horizontal ? hData : vData;
            return data[section].toString();
        }
        case CustomRole:
            return (orientation == Qt::Horizontal ? "c" : "r") + QString::number(section);
        default:
            return QVariant();
        }
    }
    Q_INVOKABLE bool setHeaderData(int section, Qt::Orientation orientation,
        const QVariant &value, int role = Qt::EditRole) override
    {
        qDebug() << Q_FUNC_INFO
                 << "section:" << section
                 << "orient:" << orientation
                 << "value:" << value
                 << "role:" << QAbstractItemModel::roleNames()[role];
        auto sectionCount = orientation == Qt::Horizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return false;
        auto &data = orientation == Qt::Horizontal ? hData : vData;
        data[section] = value;
        emit headerDataChanged(orientation, section, section);
        return true;
    }

    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override
    {
        auto names = TestTableModel::roleNames();
        names[CustomRole] = "customRole";
        return names;
    }

private:
    QList<QVariant> hData, vData;
};

class tst_QQuickHeaderView : public QQmlDataTest {
    Q_OBJECT

public:
    tst_QQuickHeaderView();

    QPoint getContextRowAndColumn(const QQuickItem *item) const;

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void init() override;
    void cleanup();

    void defaults();
    void testHeaderDataProxyModel();
    void testOrientation();
    void testModel();
    void listModel();

    void resizableHandlerBlockingEvents();
    void headerData();
    void warnMissingDefaultRole();
    void dragInvalidItemDuringReorder();
    void horizontalHeaderViewWithListModel_data();
    void horizontalHeaderViewWithListModel();
    void reorderEmptyModel();

private:
    QQmlEngine *engine;
    QString errorString;

    std::unique_ptr<QObject> rootObjectFromQml(const char *file)
    {
        auto component = new QQmlComponent(engine);
        component->loadUrl(testFileUrl(file));
        auto root = component->create();
        if (!root)
            errorString = component->errorString();
        return std::unique_ptr<QObject>(new QObject(root));
    }
};

tst_QQuickHeaderView::tst_QQuickHeaderView()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
}

QPoint tst_QQuickHeaderView::getContextRowAndColumn(const QQuickItem *item) const
{
    const auto context = qmlContext(item);
    const int row = context->contextProperty("row").toInt();
    const int column = context->contextProperty("column").toInt();
    return QPoint(column, row);
}

void tst_QQuickHeaderView::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestTableModel>("TestTableModel", 0, 1, "TestTableModel");
    qmlRegisterType<TestTableModelWithHeader>("TestTableModelWithHeader", 0, 1, "TestTableModelWithHeader");
    qmlRegisterType<QHeaderDataProxyModel>("HeaderDataProxyModel", 0, 1, "HeaderDataProxyModel");
}

void tst_QQuickHeaderView::cleanupTestCase()
{
}

void tst_QQuickHeaderView::init()
{
    QQmlDataTest::init();

    engine = new QQmlEngine(this);
}

void tst_QQuickHeaderView::cleanup()
{
    if (engine) {
        delete engine;
        engine = nullptr;
    }
}

void tst_QQuickHeaderView::defaults()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);
    auto tm = root->findChild<TestTableModel *>("tableModel");
    QVERIFY(tm);
    auto pm = root->findChild<QHeaderDataProxyModel *>("proxyModel");
    QVERIFY(pm);
    auto tv = root->findChild<QQuickTableView *>("tableView");
    QVERIFY(tv);
}

void tst_QQuickHeaderView::testHeaderDataProxyModel()
{
    TestTableModel model;
    model.setColumnCount(10);
    model.setRowCount(7);
    QHeaderDataProxyModel model2;
    model2.setSourceModel(&model);
    QAbstractItemModelTester tester(&model2, QAbstractItemModelTester::FailureReportingMode::QtTest);
}

void tst_QQuickHeaderView::testOrientation()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    // Make sure that the window is shown at this point, so that the test
    // behaves similarly on all platforms
    QVERIFY(QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())));

    // If we want to make use of syncDirection, we need to set syncView as well.
    // For that we need to create a second dummy table view.
    QQuickTableView otherView;

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    QCOMPARE(hhv->columns(), 10);
    QCOMPARE(hhv->rows(), 1);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);

    hhv->setSyncView(&otherView);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*Setting syncDirection other than Qt::Horizontal is invalid."));
    hhv->setSyncDirection(Qt::Vertical);
    QVERIFY(QQuickTest::qWaitForPolish(hhv));

    vhv->setSyncView(&otherView);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*Setting syncDirection other than Qt::Vertical is invalid."));
    vhv->setSyncDirection(Qt::Horizontal);
    QVERIFY(QQuickTest::qWaitForPolish(vhv));

    // Explicitly setting a different synDirection is ignored
    QCOMPARE(hhv->syncDirection(), Qt::Horizontal);
    QCOMPARE(hhv->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(vhv->syncDirection(), Qt::Vertical);
    QCOMPARE(vhv->flickableDirection(), QQuickFlickable::VerticalFlick);
}

void tst_QQuickHeaderView::testModel()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto thm = root->findChild<TestTableModel *>("tableHeaderModel");
    QVERIFY(thm);
    auto pm = root->findChild<QHeaderDataProxyModel *>("proxyModel");
    QVERIFY(pm);

    QSignalSpy modelChangedSpy(hhv, SIGNAL(modelChanged()));
    QVERIFY(modelChangedSpy.isValid());

    hhv->setModel(QVariant::fromValue(thm));
    QCOMPARE(modelChangedSpy.size(), 0);

    hhv->setModel(QVariant::fromValue(pm));
    QCOMPARE(modelChangedSpy.size(), 1);

    TestTableModel ttm2;
    ttm2.setRowCount(100);
    ttm2.setColumnCount(30);
    hhv->setModel(QVariant::fromValue(&ttm2));
    QCOMPARE(modelChangedSpy.size(), 2);
}

void tst_QQuickHeaderView::listModel()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("ListModel.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    if (!QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())))
        QSKIP("Window failed to become active!");

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);

    auto getDelegate = [](QQuickHeaderViewBase *headerView, qreal x,
                          qreal y) -> QQuickHeaderViewDelegate * {
        auto *item = headerView->childAt(x, y)->childAt(x, y);
        return qobject_cast<QQuickHeaderViewDelegate*>(item);
    };

    auto hhvCell1 = getDelegate(hhv, 0, 0);
    QVERIFY(hhvCell1);
    QVERIFY(hhvCell1->contentItem());
    QCOMPARE(hhvCell1->contentItem()->property("text"), "AAA");

    auto hhvCell2 = getDelegate(hhv, hhvCell1->width() + 5, 0);
    QVERIFY(hhvCell2);
    QVERIFY(hhvCell2->contentItem());
    QCOMPARE(hhvCell2->contentItem()->property("text"), "BBB");

    auto vhvCell1 = getDelegate(vhv, 0, 0);
    QVERIFY(vhvCell1);
    QVERIFY(vhvCell1->contentItem());
    QCOMPARE(vhvCell1->contentItem()->property("text"), "111");

    auto vhvCell2 = getDelegate(vhv, 0, vhvCell1->height() + 5);
    QVERIFY(vhvCell2);
    QVERIFY(vhvCell2->contentItem());
    QCOMPARE(vhvCell2->contentItem()->property("text"), "222");
}

// A header shouldn't block events outside of itself.
void tst_QQuickHeaderView::resizableHandlerBlockingEvents()
{
    QQuickApplicationHelper helper(this, QStringLiteral("resizableHandlerBlockingEvents.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto mouseArea = window->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(mouseArea));
    QVERIFY(mouseArea->isPressed());
}

void tst_QQuickHeaderView::headerData()
{
    QQuickApplicationHelper helper(this, QStringLiteral("headerData.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto headerView = window->property("headerView").value<QQuickHeaderViewBase *>();
    QVERIFY(headerView);
    const auto firstHeaderCell = headerView->itemAtIndex(headerView->index(0, 0));
    QVERIFY(firstHeaderCell);
    const auto label = firstHeaderCell->findChild<QQuickLabel *>();
    QVERIFY(label);
    QCOMPARE(label->text(), "c0");
}

void tst_QQuickHeaderView::warnMissingDefaultRole()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*toolTip.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Required property.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("TableView.*"));
    QQuickApplicationHelper helper(this, QStringLiteral("DefaultRoles.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
}

void tst_QQuickHeaderView::dragInvalidItemDuringReorder()
{
    QQuickApplicationHelper helper(this, QStringLiteral("reorderHeader.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto hhv = window->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    const auto item = hhv->itemAtIndex(hhv->index(0, 0));
    QQuickWindow *itemWindow = item->window();

    QSignalSpy columnMovedSpy(hhv, SIGNAL(columnMoved(int, int, int)));
    QVERIFY(columnMovedSpy.isValid());

    const QPoint localPos = QPoint(item->width() - 5, item->height() - 5);
    const QPoint startPos = itemWindow->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(itemWindow, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(itemWindow, startPos + startDragDist);
    QTest::mouseMove(itemWindow, startPos + dragLength);
    QTest::mouseRelease(itemWindow, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
    QCOMPARE(columnMovedSpy.size(), 0);
}

void tst_QQuickHeaderView::horizontalHeaderViewWithListModel_data()
{
    QTest::addColumn<QVariant>("model");

    const QStringList stringModel = {"one", "two", "three"};

    QTest::newRow("Number model") << QVariant::fromValue(3);
    QTest::newRow("List model") << QVariant::fromValue(stringModel);
}

void tst_QQuickHeaderView::horizontalHeaderViewWithListModel()
{
    // Check that HorizontalHeaderView will be transposed when using a list model
    QFETCH(QVariant, model);

    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    hhv->setModel(model);
    QVERIFY(QQuickTest::qWaitForPolish(hhv));

    // Check that the list elements are laid out in a row-major order
    QCOMPARE(hhv->columns(), 3);
    QCOMPARE(hhv->rows(), 1);

    for (int col = 0; col < 3; ++col) {
        auto item = hhv->itemAtCell({col, 0});
        QVERIFY(item);
        const QPoint contextCell = getContextRowAndColumn(item);
        QCOMPARE(contextCell, QPoint(0, col));
    }
}

void tst_QQuickHeaderView::reorderEmptyModel()
{
    QQuickApplicationHelper helper(this, QStringLiteral("reorderEmptyModel.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto hhv = window->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    QSignalSpy columnMovedSpy(hhv, SIGNAL(columnMoved(int, int, int)));
    QVERIFY(columnMovedSpy.isValid());
    hhv->moveColumn(0, 1);
    QVERIFY(!columnMovedSpy.isEmpty());
}

QTEST_MAIN(tst_QQuickHeaderView)

#include "tst_qquickheaderview.moc"
