// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtTest
import QtQuick.Controls

TestCase {
    id: testCase
    width: 300
    height: 300
    visible: true
    when: windowShown
    name: "Tumbler"

    property var tumbler: null
    readonly property real implicitTumblerWidth: 60
    readonly property real implicitTumblerHeight: 200
    readonly property real defaultImplicitDelegateHeight: implicitTumblerHeight / 3
    readonly property real defaultListViewTumblerOffset: -defaultImplicitDelegateHeight
    readonly property real tumblerDelegateHeight: tumbler ? tumbler.availableHeight / tumbler.visibleItemCount : 0
    property Item tumblerView: null

    Component {
        id: defaultTumbler

        Tumbler {}
    }

    Component {
        id: tumblerComponent

        Tumbler {
            visibleItemCount: 3
        }
    }

    Component {
        id: itemComponent

        Item {
            anchors.fill: parent
        }
    }

    function createTumbler(args) {
        tumbler = createTemporaryObject(tumblerComponent, testCase, args);
        verify(tumbler, "Tumbler: failed to create an instance");
        tumblerView = findView(tumbler);
        verify(tumblerView);
    }

    function tumblerXCenter() {
        return tumbler.leftPadding + tumbler.width / 2;
    }

    function tumblerYCenter() {
        return tumbler.topPadding + tumbler.height / 2;
    }

    // visualItemIndex is from 0 to the amount of visible items.
    function itemCenterPos(visualItemIndex) {
        let halfDelegateHeight = tumblerDelegateHeight / 2;
        let yCenter = tumbler.y + tumbler.topPadding + halfDelegateHeight
            + (tumblerDelegateHeight * visualItemIndex);
        return Qt.point(tumblerXCenter(), yCenter);
    }

    function itemTopLeftPos(visualItemIndex) {
        return Qt.point(tumbler.leftPadding, tumbler.topPadding + (tumblerDelegateHeight * visualItemIndex));
    }

    function checkItemSizes() {
        let contentChildren = tumbler.wrap ? tumblerView.children : tumblerView.contentItem.children;
        verify(contentChildren.length >= tumbler.count);
        for (let i = 0; i < contentChildren.length; ++i) {
            compare(contentChildren[i].width, tumbler.availableWidth);
            compare(contentChildren[i].height, tumblerDelegateHeight);
        }
    }

    function findView(parent) {
        for (let i = 0; i < parent.children.length; ++i) {
            let child = parent.children[i];
            if (child.hasOwnProperty("currentIndex")) {
                return child;
            }

            let grandChild = findView(child);
            if (grandChild)
                return grandChild;
        }

        return null;
    }

    function findDelegateWithText(parent, text) {
        for (let i = 0; i < parent.children.length; ++i) {
            let child = parent.children[i];
            if (child.hasOwnProperty("text") && child.text === text) {
                return child;
            }

            let grandChild = findDelegateWithText(child, text);
            if (grandChild)
                return grandChild;
        }

        return null;
    }

    property Component noAttachedPropertiesDelegate: Text {
        text: modelData
    }

    function init() {
        failOnWarning(/.?/)
    }

    function test_defaults() {
        let control = createTemporaryObject(defaultTumbler, testCase)
        verify(control)
    }

    function test_wrapWithoutAttachedProperties() {
        createTumbler();
        verify(tumbler.wrap);

        tumbler.delegate = noAttachedPropertiesDelegate;
        // Shouldn't assert.
        tumbler.wrap = false;
        verify(findView(tumbler));
    }

    // TODO: test that currentIndex is maintained between contentItem changes...
//    function tst_dynamicContentItemChange() {
//    }

    function test_currentIndex() {
        createTumbler();
        compare(tumbler.contentItem.parent, tumbler);

        tumbler.model = 5;

        compare(tumbler.currentIndex, 0);
        waitForRendering(tumbler);

        // Set it through user interaction.
        let pos = Qt.point(tumblerXCenter(), tumbler.height / 2);
        mouseDrag(tumbler, pos.x, pos.y, 0, tumbler.height / 3, Qt.LeftButton, Qt.NoModifier, 200);
        tryCompare(tumblerView, "offset", 1);
        compare(tumbler.currentIndex, 4);
        compare(tumblerView.currentIndex, 4);

        // Set it manually.
        tumbler.currentIndex = 2;
        tryCompare(tumbler, "currentIndex", 2);
        compare(tumblerView.currentIndex, 2);

        tumbler.model = null;
        tryCompare(tumbler, "currentIndex", -1);
        // PathView will only use 0 as the currentIndex when there are no items.
        compare(tumblerView.currentIndex, 0);

        tumbler.model = ["A", "B", "C"];
        tryCompare(tumbler, "currentIndex", 0);

        // Setting a negative current index should have no effect, because the model isn't empty.
        tumbler.currentIndex = -1;
        compare(tumbler.currentIndex, 0);

        tumbler.model = 1;
        compare(tumbler.currentIndex, 0);

        tumbler.model = 5;
        compare(tumbler.count, 5);
        tumblerView = findView(tumbler);
        tryCompare(tumblerView, "count", 5);
        tumbler.currentIndex = 4;
        compare(tumbler.currentIndex, 4);
        compare(tumblerView.currentIndex, 4);

        --tumbler.model;
        compare(tumbler.count, 4);
        compare(tumblerView.count, 4);
        // Removing an item from an integer-based model will cause views to reset their currentIndex to 0.
        compare(tumbler.currentIndex, 0);
        compare(tumblerView.currentIndex, 0);

        tumbler.model = 0;
        compare(tumbler.currentIndex, -1);
    }

    Component {
        id: currentIndexTumbler

        Tumbler {
            model: 5
            currentIndex: 2
            visibleItemCount: 3
        }
    }

    Component {
        id: currentIndexTumblerNoWrap

        Tumbler {
            model: 5
            currentIndex: 2
            wrap: false
            visibleItemCount: 3
        }
    }

    Component {
        id: currentIndexTumblerNoWrapReversedOrder

        Tumbler {
            model: 5
            wrap: false
            currentIndex: 2
            visibleItemCount: 3
        }
    }

    Component {
        id: negativeCurrentIndexTumblerNoWrap

        Tumbler {
            model: 5
            wrap: false
            currentIndex: -1
            visibleItemCount: 3
        }
    }

    Component {
        id: currentIndexTooLargeTumbler

        Tumbler {
            objectName: "currentIndexTooLargeTumbler"
            model: 10
            currentIndex: 10
        }
    }


    function test_currentIndexAtCreation_data() {
        return [
            { tag: "wrap: implicit, expected currentIndex: 2", currentIndex: 2, wrap: true, component: currentIndexTumbler },
            { tag: "wrap: false, expected currentIndex: 2", currentIndex: 2, wrap: false, component: currentIndexTumblerNoWrap },
            // Order of property assignments shouldn't matter
            { tag: "wrap: false, expected currentIndex: 2, reversed property assignment order",
                currentIndex: 2, wrap: false, component: currentIndexTumblerNoWrapReversedOrder },
            { tag: "wrap: false, expected currentIndex: 0", currentIndex: 0, wrap: false, component: negativeCurrentIndexTumblerNoWrap },
            { tag: "wrap: implicit, expected currentIndex: 0", currentIndex: 0, wrap: true, component: currentIndexTooLargeTumbler }
        ]
    }

    function test_currentIndexAtCreation(data) {
        // Test setting currentIndex at creation time
        tumbler = createTemporaryObject(data.component, testCase);
        verify(tumbler);
        // A "statically declared" currentIndex will be pending until the count has changed,
        // which happens when the model is set, which happens on the TumblerView's next polish.
        tryCompare(tumbler, "currentIndex", data.currentIndex);

        tumblerView = findView(tumbler);
        tryVerify(function() { return tumblerView.currentItem });
        compare(tumblerView.currentIndex, data.currentIndex);
        compare(tumblerView.currentItem.text, data.currentIndex.toString());

        if (data.wrap) {
            tryCompare(tumblerView, "offset", data.currentIndex > 0 ? tumblerView.count - data.currentIndex : 0);
        } else {
            tryCompare(tumblerView, "contentY", tumblerDelegateHeight * data.currentIndex - tumblerView.preferredHighlightBegin);
        }
    }

    function test_keyboardNavigation() {
        createTumbler();

        tumbler.model = 5;
        tumbler.forceActiveFocus();
        tumblerView.highlightMoveDuration = 0;

        // Navigate upwards through entire wheel.
        for (let j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Up, Qt.NoModifier);
            tryCompare(tumblerView, "offset", j + 1);
            compare(tumbler.currentIndex, tumbler.count - 1 - j);
        }

        keyClick(Qt.Key_Up, Qt.NoModifier);
        tryCompare(tumblerView, "offset", 0);
        compare(tumbler.currentIndex, 0);

        // Navigate downwards through entire wheel.
        for (let j = 0; j < tumbler.count - 1; ++j) {
            keyClick(Qt.Key_Down, Qt.NoModifier);
            tryCompare(tumblerView, "offset", tumbler.count - 1 - j);
            compare(tumbler.currentIndex, j + 1);
        }

        keyClick(Qt.Key_Down, Qt.NoModifier);
        tryCompare(tumblerView, "offset", 0);
        compare(tumbler.currentIndex, 0);
    }

    function test_itemsCorrectlyPositioned() {
        createTumbler();

        tumbler.model = 4;
        tumbler.height = 120;
        compare(tumblerDelegateHeight, 40);
        checkItemSizes();

        wait(tumblerView.highlightMoveDuration);
        let firstItemCenterPos = itemCenterPos(1);
        let firstItem = tumblerView.itemAt(firstItemCenterPos.x, firstItemCenterPos.y);
        let actualPos = testCase.mapFromItem(firstItem, 0, 0);
        compare(actualPos.x, tumbler.leftPadding);
        compare(actualPos.y, tumbler.topPadding + 40);

        tumbler.forceActiveFocus();
        keyClick(Qt.Key_Down);
        tryCompare(tumblerView, "offset", 3.0);
        tryCompare(tumbler, "moving", false);
        firstItemCenterPos = itemCenterPos(0);
        firstItem = tumblerView.itemAt(firstItemCenterPos.x, firstItemCenterPos.y);
        verify(firstItem);
        // Test QTBUG-40298.
        actualPos = testCase.mapFromItem(firstItem, 0, 0);
        fuzzyCompare(actualPos.x, tumbler.leftPadding, 0.0001);
        fuzzyCompare(actualPos.y, tumbler.topPadding, 0.0001);

        let secondItemCenterPos = itemCenterPos(1);
        let secondItem = tumblerView.itemAt(secondItemCenterPos.x, secondItemCenterPos.y);
        verify(secondItem);
        verify(firstItem.y < secondItem.y);

        let thirdItemCenterPos = itemCenterPos(2);
        let thirdItem = tumblerView.itemAt(thirdItemCenterPos.x, thirdItemCenterPos.y);
        verify(thirdItem);
        verify(firstItem.y < thirdItem.y);
        verify(secondItem.y < thirdItem.y);
    }

    function test_focusPastTumbler() {
        tumbler = createTemporaryObject(tumblerComponent, testCase);
        verify(tumbler);

        let mouseArea = createTemporaryQmlObject(
            "import QtQuick; TextInput { activeFocusOnTab: true; width: 50; height: 50 }", testCase, "");

        tumbler.forceActiveFocus();
        verify(tumbler.activeFocus);

        keyClick(Qt.Key_Tab);
        verify(!tumbler.activeFocus);
        verify(mouseArea.activeFocus);
    }

    function test_datePicker() {
        let component = Qt.createComponent("TumblerDatePicker.qml");
        compare(component.status, Component.Ready, component.errorString());
        tumbler = createTemporaryObject(component, testCase);
        // Should not be any warnings.

        tryCompare(tumbler.dayTumbler, "currentIndex", 0);
        compare(tumbler.dayTumbler.count, 31);
        compare(tumbler.monthTumbler.currentIndex, 0);
        compare(tumbler.monthTumbler.count, 12);
        compare(tumbler.yearTumbler.currentIndex, 0);
        tryCompare(tumbler.yearTumbler, "count", 100);

        verify(findView(tumbler.dayTumbler).children.length >= tumbler.dayTumbler.visibleItemCount);
        verify(findView(tumbler.monthTumbler).children.length >= tumbler.monthTumbler.visibleItemCount);
        // TODO: do this properly somehow
        wait(100);
        verify(findView(tumbler.yearTumbler).children.length >= tumbler.yearTumbler.visibleItemCount);

        // March.
        tumbler.monthTumbler.currentIndex = 2;
        tryCompare(tumbler.monthTumbler, "currentIndex", 2);

        // 30th of March.
        tumbler.dayTumbler.currentIndex = 29;
        tryCompare(tumbler.dayTumbler, "currentIndex", 29);

        // February.
        tumbler.monthTumbler.currentIndex = 1;
        tryCompare(tumbler.monthTumbler, "currentIndex", 1);
        tryCompare(tumbler.dayTumbler, "currentIndex", 27);
    }

    Component {
        id: timePickerComponent

        Row {
            property alias minuteTumbler: minuteTumbler
            property alias amPmTumbler: amPmTumbler

            Tumbler {
                id: minuteTumbler
                currentIndex: 6
                model: 60
                width: 50
                height: 150
            }

            Tumbler {
                id: amPmTumbler
                model: ["AM", "PM"]
                width: 50
                height: 150
                contentItem: ListView {
                    anchors.fill: parent
                    model: amPmTumbler.model
                    delegate: amPmTumbler.delegate
                }
            }
        }
    }

    function test_listViewTimePicker() {
        let root = createTemporaryObject(timePickerComponent, testCase);
        verify(root);

        mouseDrag(root.minuteTumbler, root.minuteTumbler.width / 2, root.minuteTumbler.height / 2, 0, 50);
        // Shouldn't crash.
        mouseDrag(root.amPmTumbler, root.amPmTumbler.width / 2, root.amPmTumbler.height / 2, 0, 50);
    }

    function test_displacement_data() {
        let data = [
            // At 0 offset, the first item is current.
            { count: 6, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 6, index: 1, offset: 0, expectedDisplacement: -1 },
            { count: 6, index: 5, offset: 0, expectedDisplacement: 1 },
            // When we start to move the first item down, the second item above it starts to become current.
            { count: 6, index: 0, offset: 0.25, expectedDisplacement: -0.25 },
            { count: 6, index: 1, offset: 0.25, expectedDisplacement: -1.25 },
            { count: 6, index: 5, offset: 0.25, expectedDisplacement: 0.75 },
            { count: 6, index: 0, offset: 0.5, expectedDisplacement: -0.5 },
            { count: 6, index: 1, offset: 0.5, expectedDisplacement: -1.5 },
            { count: 6, index: 5, offset: 0.5, expectedDisplacement: 0.5 },
            // By this stage, the delegate at index 1 is destroyed, so we can't test its displacement.
            { count: 6, index: 0, offset: 0.75, expectedDisplacement: -0.75 },
            { count: 6, index: 5, offset: 0.75, expectedDisplacement: 0.25 },
            { count: 6, index: 0, offset: 4.75, expectedDisplacement: 1.25 },
            { count: 6, index: 1, offset: 4.75, expectedDisplacement: 0.25 },
            { count: 6, index: 0, offset: 4.5, expectedDisplacement: 1.5 },
            { count: 6, index: 1, offset: 4.5, expectedDisplacement: 0.5 },
            { count: 6, index: 0, offset: 4.25, expectedDisplacement: 1.75 },
            { count: 6, index: 1, offset: 4.25, expectedDisplacement: 0.75 },
            // count == visibleItemCount
            { count: 3, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 3, index: 1, offset: 0, expectedDisplacement: -1 },
            { count: 3, index: 2, offset: 0, expectedDisplacement: 1 },
            // count < visibleItemCount
            { count: 2, index: 0, offset: 0, expectedDisplacement: 0 },
            { count: 2, index: 1, offset: 0, expectedDisplacement: 1 },
            // count == 1
            { count: 1, index: 0, offset: 0, expectedDisplacement: 0 }
        ];
        for (let i = 0; i < data.length; ++i) {
            let row = data[i];
            row.tag = "count=" + row.count
                + " delegate" + row.index
                + " offset=" + row.offset
                + " expectedDisplacement=" + row.expectedDisplacement;
        }
        return data;
    }

    property Component displacementDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        opacity: 0.2 + Math.max(0, 1 - Math.abs(Tumbler.displacement)) * 0.8
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Text {
            text: parent.displacement.toFixed(2)
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }

        property real displacement: Tumbler.displacement
    }

    function test_displacement(data) {
        createTumbler();

        // TODO: test setting these in the opposite order (delegate after model
        // doesn't seem to cause a change in delegates in PathView)
        tumbler.wrap = true;
        tumbler.delegate = displacementDelegate;
        tumbler.model = data.count;
        compare(tumbler.count, data.count);

        let delegate = findChild(tumblerView, "delegate" + data.index);
        verify(delegate);

        tumblerView.offset = data.offset;
        compare(delegate.displacement, data.expectedDisplacement);

        // test displacement after adding and removing items
    }

    function test_wrap() {
        createTumbler();

        tumbler.model = 5;
        compare(tumbler.count, 5);

        tumbler.currentIndex = 2;
        compare(tumblerView.currentIndex, 2);

        tumbler.wrap = false;
        tumblerView = findView(tumbler);
        compare(tumbler.count, 5);
        compare(tumbler.currentIndex, 2);
        // Tumbler's count hasn't changed (the model hasn't changed),
        // but the new view needs time to instantiate its items.
        tryCompare(tumblerView, "count", 5);
        compare(tumblerView.currentIndex, 2);
    }

    Component {
        id: twoItemTumbler

        Tumbler {
            model: 2
        }
    }

    Component {
        id: tenItemTumbler

        Tumbler {
            model: 10
        }
    }

    function test_countWrap() {
        tumbler = createTemporaryObject(tumblerComponent, testCase);
        verify(tumbler);

        // Check that a count that is less than visibleItemCount results in wrap being set to false.
        verify(2 < tumbler.visibleItemCount);
        tumbler.model = 2;
        compare(tumbler.count, 2);
        compare(tumbler.wrap, false);
    }

    function test_explicitlyNonwrapping() {
        // Check that explicitly setting wrap to false works even when it was implicitly false.
        let explicitlyNonWrapping = createTemporaryObject(twoItemTumbler, testCase);
        verify(explicitlyNonWrapping);
        tryCompare(explicitlyNonWrapping, "wrap", false);

        explicitlyNonWrapping.wrap = false;
        // wrap shouldn't be set to true now that there are more items than there are visible ones.
        verify(10 > explicitlyNonWrapping.visibleItemCount);
        explicitlyNonWrapping.model = 10;
        compare(explicitlyNonWrapping.wrap, false);

        // Test resetting wrap back to the default behavior.
        explicitlyNonWrapping.wrap = undefined;
        compare(explicitlyNonWrapping.wrap, true);
    }

    function test_explicitlyWrapping() {
        // Check that explicitly setting wrap to true works even when it was implicitly true.
        let explicitlyWrapping = createTemporaryObject(tenItemTumbler, testCase);
        verify(explicitlyWrapping);
        compare(explicitlyWrapping.wrap, true);

        explicitlyWrapping.wrap = true;
        // wrap shouldn't be set to false now that there are more items than there are visible ones.
        explicitlyWrapping.model = 2;
        compare(explicitlyWrapping.wrap, true);

        // Test resetting wrap back to the default behavior.
        explicitlyWrapping.wrap = undefined;
        compare(explicitlyWrapping.wrap, false);
    }

    Component {
        id: pathViewComponent

        PathView {
        }
    }

    Component {
        id: listViewComponent

        ListView {
        }
    }

    function test_defaultFlickDecelerationWrap() {
        createTumbler();

        tumbler.wrap = true;
        tumblerView = findView(tumbler);

        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);
        // when wrapping, the default flickDeceleration should
        // match the default flickDeceleration of PathView
        let pathView = createTemporaryObject(pathViewComponent, testCase);
        compare(tumbler.flickDeceleration, pathView.flickDeceleration);
    }

    function test_defaultFlickDecelerationNoWrap() {
        createTumbler();

        tumbler.wrap = false;
        tumblerView = findView(tumbler);

        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);
        // when not wrapping, the default flickDeceleration should
        // match the default flickDeceleration of ListView
        let listView = createTemporaryObject(listViewComponent, testCase);
        compare(tumbler.flickDeceleration, listView.flickDeceleration);
    }

    function test_explicitFlickDeceleration() {
        createTumbler();

        tumbler.wrap = false;
        tumbler.flickDeceleration = 0.2;
        compare(tumbler.flickDeceleration, 0.2);
        tumblerView = findView(tumbler);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);

        tumbler.wrap = true;
        compare(tumbler.flickDeceleration, 0.2);
        tumblerView = findView(tumbler);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);

        tumbler.flickDeceleration = 1.2;
        compare(tumbler.flickDeceleration, 1.2);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);

        tumbler.wrap = false;
        compare(tumbler.flickDeceleration, 1.2);
        tumblerView = findView(tumbler);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);
    }

    function test_resetFlickDeceleration() {
        createTumbler();

        tumbler.wrap = true;
        tumbler.flickDeceleration = 0.2;

        compare(tumbler.flickDeceleration, 0.2);
        waitForRendering(tumblerView);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);

        tumbler.flickDeceleration = undefined;

        let pathView = createTemporaryObject(pathViewComponent, testCase);
        compare(tumbler.flickDeceleration, pathView.flickDeceleration);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);

        tumbler.wrap = false;
        let listView = createTemporaryObject(listViewComponent, testCase);
        compare(tumbler.flickDeceleration, listView.flickDeceleration);
        tumblerView = findView(tumbler);
        tryCompare(tumblerView, "flickDeceleration", tumbler.flickDeceleration);
    }

    Component {
        id: signalSpy
        SignalSpy { }
    }

    function test_flickDecelerationChangedSignalOnWrapChanged() {
        createTumbler();

        tumbler.wrap = true;
        let oldFlickDeceleration = tumbler.flickDeceleration

        let spy = signalSpy.createObject(tumbler, { target: tumbler, signalName: "flickDecelerationChanged" });
        verify(spy.valid);

        tumbler.wrap = false;

        let expected = oldFlickDeceleration !== tumbler.flickDeceleration ? 1 : 0;
        compare(spy.count, expected);
    }

    Component {
        id: customListViewTumblerComponent

        Tumbler {
            id: listViewTumbler

            contentItem: ListView {
                anchors.fill: parent
                model: listViewTumbler.model
                delegate: listViewTumbler.delegate

                snapMode: ListView.SnapToItem
                highlightRangeMode: ListView.StrictlyEnforceRange
                preferredHighlightBegin: height / 2 - (height / listViewTumbler.visibleItemCount / 2)
                preferredHighlightEnd: height / 2 + (height / listViewTumbler.visibleItemCount / 2)
                clip: true
            }
        }
    }

    Component {
        id: customPathViewTumblerComponent

        Tumbler {
            id: pathViewTumbler

            contentItem: PathView {
                id: pathView
                model: pathViewTumbler.model
                delegate: pathViewTumbler.delegate
                clip: true
                pathItemCount: pathViewTumbler.visibleItemCount + 1
                preferredHighlightBegin: 0.5
                preferredHighlightEnd: 0.5
                dragMargin: width / 2

                path: Path {
                    startX: pathView.width / 2
                    startY: -pathView.delegateHeight / 2
                    PathLine {
                        x: pathView.width / 2
                        y: pathView.pathItemCount * pathView.delegateHeight - pathView.delegateHeight / 2
                    }
                }

                property real delegateHeight: pathViewTumbler.availableHeight / pathViewTumbler.visibleItemCount
            }
        }
    }

    function test_customContentItemAtConstruction_data() {
        return [
            { tag: "ListView", component: customListViewTumblerComponent },
            { tag: "PathView", component: customPathViewTumblerComponent }
        ];
    }

    function test_customContentItemAtConstruction(data) {
        let tumbler = createTemporaryObject(data.component, testCase);
        // Shouldn't assert.

        tumbler.model = 5;
        compare(tumbler.count, 5);

        tumbler.currentIndex = 2;
        let tumblerView = findView(tumbler);
        compare(tumblerView.currentIndex, 2);

        tumblerView.incrementCurrentIndex();
        compare(tumblerView.currentIndex, 3);
        compare(tumbler.currentIndex, 3);

        // Shouldn't have any affect.
        tumbler.wrap = false;
        compare(tumbler.count, 5);
        compare(tumblerView.currentIndex, 3);
        compare(tumbler.currentIndex, 3);
    }

    function findFirstDelegateWithText(view, text) {
        let delegate = null;
        let contentItem = view.hasOwnProperty("contentItem") ? view.contentItem : view;
        for (let i = 0; i < contentItem.children.length && !delegate; ++i) {
            let child = contentItem.children[i];
            if (child.hasOwnProperty("text") && child.text === text)
                delegate = child;
        }
        return delegate;
    }

    function test_customContentItemAfterConstruction_data() {
        return [
            { tag: "ListView", componentPath: "TumblerListView.qml" },
            { tag: "PathView", componentPath: "TumblerPathView.qml" }
        ];
    }

    function test_customContentItemAfterConstruction(data) {
        createTumbler();

        tumbler.model = 5;
        compare(tumbler.count, 5);

        tumbler.currentIndex = 2;
        compare(tumblerView.currentIndex, 2);

        let contentItemComponent = Qt.createComponent(data.componentPath);
        compare(contentItemComponent.status, Component.Ready);

        let customContentItem = createTemporaryObject(contentItemComponent, tumbler);
        tumbler.contentItem = customContentItem;
        compare(tumbler.count, 5);
        tumblerView = findView(tumbler);
        compare(tumblerView.currentIndex, 2);

        let delegate = findFirstDelegateWithText(tumblerView, "Custom2");
        verify(delegate);
        compare(delegate.height, defaultImplicitDelegateHeight);
        tryCompare(delegate.Tumbler, "displacement", 0);

        tumblerView.incrementCurrentIndex();
        compare(tumblerView.currentIndex, 3);
        compare(tumbler.currentIndex, 3);
    }

    function test_displacementListView_data() {
        let offset = defaultListViewTumblerOffset;

        let data = [
            // At 0 contentY, the first item is current.
            { contentY: offset, expectedDisplacements: [
                { index: 0, displacement: 0 },
                { index: 1, displacement: -1 },
                { index: 2, displacement: -2 } ]
            },
            // When we start to move the first item down, the second item above it starts to become current.
            { contentY: offset + defaultImplicitDelegateHeight * 0.25, expectedDisplacements: [
                { index: 0, displacement: 0.25 },
                { index: 1, displacement: -0.75 },
                { index: 2, displacement: -1.75 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 0.5, expectedDisplacements: [
                { index: 0, displacement: 0.5 },
                { index: 1, displacement: -0.5 },
                { index: 2, displacement: -1.5 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 0.75, expectedDisplacements: [
                { index: 0, displacement: 0.75 },
                { index: 1, displacement: -0.25 } ]
            },
            { contentY: offset + defaultImplicitDelegateHeight * 3.5, expectedDisplacements: [
                { index: 3, displacement: 0.5 },
                { index: 4, displacement: -0.5 } ]
            }
        ];
        for (let i = 0; i < data.length; ++i) {
            let row = data[i];
            row.tag = "contentY=" + row.contentY;
        }
        return data;
    }

    function test_displacementListView(data) {
        createTumbler();

        tumbler.wrap = false;
        tumbler.delegate = displacementDelegate;
        tumbler.model = 5;
        compare(tumbler.count, 5);
        // Ensure assumptions about the tumbler used in our data() function are correct.
        tumblerView = findView(tumbler);
        compare(tumblerView.contentY, -defaultImplicitDelegateHeight);
        let delegateCount = 0;
        let listView = tumblerView;
        let listViewContentItem = tumblerView.contentItem;

        // We use the mouse instead of setting contentY directly, otherwise the
        // items snap back into place. This doesn't seem to be an issue for
        // PathView for some reason.
        //
        // I tried lots of things to get this test to work with small changes
        // in ListView's contentY (to match the tests for a PathView-based Tumbler), but they didn't work:
        //
        // - Pressing once and then directly moving the mouse to the correct location
        // - Pressing once and interpolating the mouse position to the correct location
        // - Pressing once and doing some dragging up and down to trigger the
        //   overThreshold of QQuickFlickable
        //
        // Even after the last item above, QQuickFlickable wouldn't consider it a drag.
        // It seems that overThreshold is set too late, and because the drag distance is quite small
        // to begin with, nothing changes (the displacement was always very close to 0 in the end).

        // Ensure that we at least cover the distance required to reach the desired contentY.
        let distanceToReachContentY = data.contentY - defaultListViewTumblerOffset;
        let distance = Math.abs(distanceToReachContentY) + tumbler.height / 2;
        // If distanceToReachContentY is 0, we're testing 0 displacement, so we don't need to do anything.
        if (distanceToReachContentY != 0) {
            mousePress(tumbler, tumblerXCenter(), tumblerYCenter());

            let dragDirection = distanceToReachContentY > 0 ? -1 : 1;
            for (let i = 0; i < distance && Math.floor(listView.contentY) !== Math.floor(data.contentY); ++i) {
                mouseMove(tumbler, tumblerXCenter(), tumblerYCenter() + i * dragDirection);
                wait(1); // because Flickable pays attention to velocity, we need some time between movements (qtdeclarative ebf07c3)
            }
        }

        for (let i = 0; i < data.expectedDisplacements.length; ++i) {
            let delegate = findChild(listViewContentItem, "delegate" + data.expectedDisplacements[i].index);
            verify(delegate);
            compare(delegate.height, defaultImplicitDelegateHeight);
            // Due to the way we must perform this test, we can't expect high precision.
            let expectedDisplacement = data.expectedDisplacements[i].displacement;
            fuzzyCompare(delegate.displacement, expectedDisplacement, 0.1,
                "Delegate of ListView-based Tumbler at index " + data.expectedDisplacements[i].index
                    + " has displacement of " + delegate.displacement + " when it should be " + expectedDisplacement);
        }

        if (distanceToReachContentY != 0)
            mouseRelease(tumbler, tumblerXCenter(), itemCenterPos(1).y + (data.contentY - defaultListViewTumblerOffset), Qt.LeftButton);
    }

    function test_listViewFlickAboveBounds_data() {
        // Tests that flicking above the bounds when already at the top of the
        // tumbler doesn't result in an incorrect displacement.
        let data = [];
        // Less than two items doesn't make sense. The default visibleItemCount
        // is 3, so we test a bit more than double that.
        for (let i = 2; i <= 7; ++i) {
            data.push({ tag: i + " items", model: i });
        }
        return data;
    }

    function test_listViewFlickAboveBounds(data) {
        createTumbler();

        tumbler.wrap = false;
        tumbler.delegate = displacementDelegate;
        tumbler.model = data.model;
        tumblerView = findView(tumbler);

        mousePress(tumbler, tumblerXCenter(), tumblerYCenter());

        // Ensure it's stationary.
        let listView = tumblerView;
        compare(listView.contentY, defaultListViewTumblerOffset);

        // We could just move up until the contentY changed, but this is safer.
        let distance = tumbler.height;
        let changed = false;

        for (let i = 0; i < distance && !changed; ++i) {
            mouseMove(tumbler, tumblerXCenter(), tumblerYCenter() + i, 10);

            // Don't test until the contentY has actually changed.
            if (Math.abs(listView.contentY) - listView.preferredHighlightBegin > 0.01) {

                for (let delegateIndex = 0; delegateIndex < Math.min(tumbler.count, tumbler.visibleItemCount); ++delegateIndex) {
                    let delegate = findChild(listView.contentItem, "delegate" + delegateIndex);
                    verify(delegate);

                    verify(delegate.displacement <= -delegateIndex, "Delegate at index " + delegateIndex + " has a displacement of "
                        + delegate.displacement + " when it should be less than or equal to " + -delegateIndex);
                    verify(delegate.displacement > -delegateIndex - 0.1, "Delegate at index 0 has a displacement of "
                        + delegate.displacement + " when it should be greater than ~ " + -delegateIndex - 0.1);
                }

                changed = true;
            }
        }

        // Sanity check that something was actually tested.
        verify(changed);

        mouseRelease(tumbler, tumblerXCenter(), tumbler.topPadding);
    }

    property Component objectNameDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    function test_visibleItemCount_data() {
        let data = [
            // e.g. {0: 2} = {delegate index: y pos / delegate height}
            // Skip item at index 3, because it's out of view.
            { model: 6, visibleItemCount: 5, expectedYPositions: {0: 2, 1: 3, 2: 4, 4: 0} },
            { model: 5, visibleItemCount: 3, expectedYPositions: {0: 1, 1: 2, 4: 0} },
            // Takes up the whole view.
            { model: 2, visibleItemCount: 1, expectedYPositions: {0: 0} },
        ];

        for (let i = 0; i < data.length; ++i) {
            data[i].tag = "items=" + data[i].model + ", visibleItemCount=" + data[i].visibleItemCount;
        }
        return data;
    }

    function test_visibleItemCount(data) {
        createTumbler();

        tumbler.delegate = objectNameDelegate;
        tumbler.visibleItemCount = data.visibleItemCount;

        tumbler.model = data.model;
        compare(tumbler.count, data.model);

        for (let delegateIndex = 0; delegateIndex < data.visibleItemCount; ++delegateIndex) {
            if (data.expectedYPositions.hasOwnProperty(delegateIndex)) {
                let delegate = findChild(tumblerView, "delegate" + delegateIndex);
                verify(delegate, "Delegate found at index " + delegateIndex);
                let expectedYPos = data.expectedYPositions[delegateIndex] * tumblerDelegateHeight;
                compare(delegate.mapToItem(tumbler.contentItem, 0, 0).y, expectedYPos);
            }
        }
    }

    property Component wrongDelegateTypeComponent: QtObject {
        property real displacement: Tumbler.displacement
    }

    property Component noParentDelegateComponent: Item {
        property real displacement: Tumbler.displacement
    }

    function test_attachedProperties() {
        tumbler = createTemporaryObject(tumblerComponent, testCase);
        verify(tumbler);

        // TODO: crashes somewhere in QML's guts
//        tumbler.model = 5;
//        tumbler.delegate = wrongDelegateTypeComponent;
//        ignoreWarning("Attached properties of Tumbler must be accessed from within a delegate item");
//        // Cause displacement to be changed. The warning isn't triggered if we don't do this.
//        tumbler.contentItem.offset += 1;

        ignoreWarning(/.*Tumbler: attached properties must be accessed through a delegate item that has a parent/);
        createTemporaryObject(noParentDelegateComponent, null);

        ignoreWarning(/.*Tumbler: attempting to access attached property on item without an \"index\" property/);
        let object = createTemporaryObject(noParentDelegateComponent, testCase);
        verify(object);
    }

    property Component paddingDelegate: Text {
        objectName: "delegate" + index
        text: modelData
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Rectangle {
            anchors.fill: parent
            color: "transparent"
            border.color: "red"
            border.width: 1
        }
    }

    function test_padding_data() {
        let data = [];

        data.push({ padding: 0 });
        data.push({ padding: 10 });
        data.push({ left: 10, top: 10 });
        data.push({ right: 10, bottom: 10 });

        for (let i = 0; i < data.length; ++i) {
            let tag = "";

            if (data[i].padding !== undefined)
                tag += "padding: " + data[i].padding + " ";
            if (data[i].left !== undefined)
                tag += "left: " + data[i].left + " ";
            if (data[i].right !== undefined)
                tag += "right: " + data[i].right + " ";
            if (data[i].top !== undefined)
                tag += "top: " + data[i].top + " ";
            if (data[i].bottom !== undefined)
                tag += "bottom: " + data[i].bottom + " ";
            tag = tag.slice(0, -1);

            data[i].tag = tag;
        }

        return data;
    }

    function test_padding(data) {
        createTumbler();

        tumbler.delegate = paddingDelegate;
        tumbler.model = 5;
        compare(tumbler.padding, 0);
        compare(tumbler.leftPadding, 0);
        compare(tumbler.rightPadding, 0);
        compare(tumbler.topPadding, 0);
        compare(tumbler.bottomPadding, 0);
        compare(tumbler.contentItem.x, 0);
        compare(tumbler.contentItem.y, 0);

        if (data.padding !== undefined)
            tumbler.padding = data.padding;
        if (data.left !== undefined)
            tumbler.leftPadding = data.left;
        if (data.right !== undefined)
            tumbler.rightPadding = data.right;
        if (data.top !== undefined)
            tumbler.topPadding = data.top;
        if (data.bottom !== undefined)
            tumbler.bottomPadding = data.bottom;

        compare(tumbler.availableWidth, tumbler.implicitWidth - tumbler.leftPadding - tumbler.rightPadding);
        compare(tumbler.availableHeight, tumbler.implicitHeight - tumbler.topPadding - tumbler.bottomPadding);
        compare(tumbler.contentItem.x, tumbler.leftPadding);
        compare(tumbler.contentItem.y, tumbler.topPadding);

        let pathView = tumbler.contentItem;
        let expectedDelegateHeight = tumbler.availableHeight / tumbler.visibleItemCount;
        let itemIndicesInVisualOrder = [4, 0, 1];
        for (let i = 0; i < itemIndicesInVisualOrder.length; ++i) {
            let delegate = findChild(pathView, "delegate" + itemIndicesInVisualOrder[i]);
            verify(delegate, "Couldn't find delegate at index " + itemIndicesInVisualOrder[i]
                + " (iteration " + i + " out of " + (pathView.children.length - 1) + ")");

            compare(delegate.width, tumbler.availableWidth);
            compare(delegate.height, expectedDelegateHeight);

            let expectedY = tumbler.topPadding + i * expectedDelegateHeight;
            let mappedPos = delegate.mapToItem(null, delegate.width / 2, 0);
            fuzzyCompare(mappedPos.y, expectedY, 0.5,
                "Tumbler's PathView delegate at index " + itemIndicesInVisualOrder[i]
                    + " should have a y pos of " + expectedY + ", but it's actually " + mappedPos.y.toFixed(20));

            let expectedX = tumbler.leftPadding;
            compare(delegate.mapToItem(null, 0, 0).x, expectedX,
                "Tumbler's PathView delegate at index " + itemIndicesInVisualOrder[i]
                    + " should have a x pos of " + expectedX + ", but it's actually " + mappedPos.x.toFixed(20));
        }

        // Force new items to be created, as there was a bug where the path was correct until this happened.
        compare(tumblerView.offset, 0);
        ++tumbler.currentIndex;
        tryCompare(tumblerView, "offset", 4, tumblerView.highlightMoveDuration * 2);
    }

    // QTBUG-127613
    function test_autoWrappingWithDelegate() {
        createTumbler();
        tumbler.delegate = paddingDelegate;
        tumbler.model = 7;
        tumbler.padding = 10;

        let expectedWidth = tumbler.availableWidth;
        let expectedHeight = tumbler.availableHeight / tumbler.visibleItemCount;

        tumbler.model = tumbler.visibleItemCount
        compare(tumbler.wrap, true);

        --tumbler.model;
        compare(tumbler.wrap, false);

        let delegate = findChild(tumbler.contentItem, "delegate0");
        compare(delegate.width, expectedWidth);
        compare(delegate.height, expectedHeight);

        ++tumbler.model;
        compare(tumbler.wrap, true);

        delegate = findChild(tumbler.contentItem, "delegate0");
        compare(delegate.width, expectedWidth);
        compare(delegate.height, expectedHeight);
    }

    function test_moving_data() {
        return [
            { tag: "wrap:true", wrap: true },
            { tag: "wrap:false", wrap: false }
        ]
    }

    function test_moving(data) {
        createTumbler({wrap: data.wrap, model: 5})
        compare(tumbler.wrap, data.wrap)
        compare(tumbler.moving, false)

        waitForRendering(tumbler)

        mousePress(tumbler, tumbler.width / 2, tumbler.height / 2, Qt.LeftButton)
        compare(tumbler.moving, false)

        for (let y = tumbler.height / 2; y >= tumbler.height / 4; y -= 10)
            mouseMove(tumbler, tumbler.width / 2, y, 1)
        compare(tumbler.moving, true)

        mouseRelease(tumbler, tumbler.width / 2, tumbler.height / 4, Qt.LeftButton)
        compare(tumbler.moving, true)
        tryCompare(tumbler, "moving", false)
    }

    Component {
        id: qtbug61374Component

        Row {
            property alias tumbler: tumbler
            property alias label: label

            Component.onCompleted: {
                tumbler.currentIndex = 2
            }

            Tumbler {
                id: tumbler
                model: 5
                // ...
            }

            Label {
                id: label
                text: tumbler.currentItem.text
            }
        }
    }

    function test_qtbug61374() {
        let row = createTemporaryObject(qtbug61374Component, testCase);
        verify(row);

        let tumbler = row.tumbler;
        tryCompare(tumbler, "currentIndex", 2);

        tumblerView = findView(tumbler);

        let label = row.label;
        compare(label.text, "2");
    }

    function test_positionViewAtIndex_data() {
        return [
            // Should be 20, 21, ... but there is a documented limitation for this in positionViewAtIndex()'s docs.
            { tag: "wrap=true, mode=Beginning", wrap: true, mode: Tumbler.Beginning, expectedVisibleIndices: [21, 22, 23, 24, 25] },
            { tag: "wrap=true, mode=Center", wrap: true, mode: Tumbler.Center, expectedVisibleIndices: [18, 19, 20, 21, 22] },
            { tag: "wrap=true, mode=End", wrap: true, mode: Tumbler.End, expectedVisibleIndices: [16, 17, 18, 19, 20] },
            // Same as Beginning; should start at 20.
            { tag: "wrap=true, mode=Contain", wrap: true, mode: Tumbler.Contain, expectedVisibleIndices: [21, 22, 23, 24, 25] },
            { tag: "wrap=true, mode=SnapPosition", wrap: true, mode: Tumbler.SnapPosition, expectedVisibleIndices: [18, 19, 20, 21, 22] },
            { tag: "wrap=false, mode=Beginning", wrap: false, mode: Tumbler.Beginning, expectedVisibleIndices: [20, 21, 22, 23, 24] },
            { tag: "wrap=false, mode=Center", wrap: false, mode: Tumbler.Center, expectedVisibleIndices: [18, 19, 20, 21, 22] },
            { tag: "wrap=false, mode=End", wrap: false, mode: Tumbler.End, expectedVisibleIndices: [16, 17, 18, 19, 20] },
            { tag: "wrap=false, mode=Visible", wrap: false, mode: Tumbler.Visible, expectedVisibleIndices: [16, 17, 18, 19, 20] },
            { tag: "wrap=false, mode=Contain", wrap: false, mode: Tumbler.Contain, expectedVisibleIndices: [16, 17, 18, 19, 20] },
            { tag: "wrap=false, mode=SnapPosition", wrap: false, mode: Tumbler.SnapPosition, expectedVisibleIndices: [18, 19, 20, 21, 22] }
        ]
    }

    function test_positionViewAtIndex(data) {
        createTumbler({ wrap: data.wrap, model: 40, visibleItemCount: 5 })
        compare(tumbler.wrap, data.wrap)

        waitForRendering(tumbler)

        tumbler.positionViewAtIndex(20, data.mode)
        tryCompare(tumbler, "moving", false)

        compare(tumbler.visibleItemCount, 5)
        for (let i = 0; i < 5; ++i) {
            // Find the item through its text, as that's easier than child/itemAt().
            let text = data.expectedVisibleIndices[i].toString()
            let item = findDelegateWithText(tumblerView, text)
            verify(item, "found no item with text \"" + text + "\"")
            compare(item.text, data.expectedVisibleIndices[i].toString())

            // Ensure that it's at the position we expect.
            let expectedPos = itemTopLeftPos(i)
            let actualPos = testCase.mapFromItem(item, 0, 0)
            compare(actualPos.x, expectedPos.x, "expected delegate with text " + item.text
                + " to have an x pos of " + expectedPos.x + " but it was " + actualPos.x)
            compare(actualPos.y, expectedPos.y, "expected delegate with text " + item.text
                + " to have an y pos of " + expectedPos.y + " but it was " + actualPos.y)
        }
    }

    Component {
        id: setCurrentIndexOnImperativeModelChangeComponent

        Tumbler {
            onModelChanged: currentIndex = model - 2
        }
    }

    function test_setCurrentIndexOnImperativeModelChange_data() {
        return [
            { tag: "default wrap", setWrap: false, initialWrap: false, newWrap: true },
            { tag: "wrap=false", setWrap: true, initialWrap: false, newWrap: false },
            { tag: "wrap=true", setWrap: true, initialWrap: true, newWrap: true },
        ]
    }

    function test_setCurrentIndexOnImperativeModelChange(data) {
        let tumbler = createTemporaryObject(setCurrentIndexOnImperativeModelChangeComponent, testCase,
                                            data.setWrap ? {wrap: data.initialWrap} : {})
        verify(tumbler)

        let model = 4
        let expectedCurrentIndex = model - 2

        tumbler.model = model

        compare(tumbler.count, model)
        compare(tumbler.wrap, data.initialWrap)
        compare(tumbler.currentIndex, expectedCurrentIndex)

        let tumblerView = findView(tumbler)
        verify(tumblerView)
        tryCompare(tumblerView, "count", model)
        tryCompare(tumblerView, "currentIndex", expectedCurrentIndex)

        model = 5
        expectedCurrentIndex = model - 2

        tumbler.model = model

        compare(tumbler.count, model)
        compare(tumbler.wrap, data.newWrap)
        compare(tumbler.currentIndex, expectedCurrentIndex)

        tumblerView = findView(tumbler)
        verify(tumblerView)
        tryCompare(tumblerView, "count", model)
        tryCompare(tumblerView, "currentIndex", expectedCurrentIndex)
    }

    Component {
        id: setCurrentIndexOnDeclarativeModelChangeComponent

        Item {
            property alias tumbler: tumbler

            required property int modelValue
            property alias tumblerWrap: tumbler.wrap

            Tumbler {
                id: tumbler
                model: modelValue
                onModelChanged: currentIndex = model - 2
            }
        }
    }

    function test_setCurrentIndexOnDeclarativeModelChange_data() {
        return [
            { tag: "default wrap", setWrap: false, initialWrap: false, newWrap: true },
            { tag: "wrap=false", setWrap: true, initialWrap: false, newWrap: false },
            { tag: "wrap=true", setWrap: true, initialWrap: true, newWrap: true },
        ]
    }

    function test_setCurrentIndexOnDeclarativeModelChange(data) {
        let model = 4
        let expectedCurrentIndex = model - 2

        let root = createTemporaryObject(setCurrentIndexOnDeclarativeModelChangeComponent, testCase,
                                         data.setWrap ? {modelValue: model, tumblerWrap: data.initialWrap}
                                                      : {modelValue: model})
        verify(root)

        let tumbler = root.tumbler
        verify(tumbler)

        compare(tumbler.count, model)
        compare(tumbler.wrap, data.initialWrap)
        compare(tumbler.currentIndex, expectedCurrentIndex)

        let tumberView = findView(tumbler)
        verify(tumbler)
        tryCompare(tumberView, "count", model)
        tryCompare(tumberView, "currentIndex", expectedCurrentIndex)

        model = 5
        expectedCurrentIndex = model - 2

        root.modelValue = model

        compare(tumbler.count, model)
        compare(tumbler.wrap, data.newWrap)
        compare(tumbler.currentIndex, expectedCurrentIndex)

        tumberView = findView(tumbler)
        verify(tumbler)
        tryCompare(tumberView, "count", model)
        tryCompare(tumberView, "currentIndex", expectedCurrentIndex)
    }

    function test_displacementAfterResizing() {
        createTumbler({
            width: 200,
            wrap: false,
            delegate: displacementDelegate,
            model: 30,
            visibleItemCount: 7,
            currentIndex: 15
        })

        let delegate = findChild(tumblerView, "delegate15")
        verify(delegate)

        tryCompare(delegate, "displacement", 0)

        // Resizing the Tumbler shouldn't affect the displacement.
        tumbler.height *= 1.4
        tryCompare(delegate, "displacement", 0)
    }

    //QTBUG-84426
    Component {
        id: initialCurrentIndexTumbler

        Tumbler {
            anchors.centerIn: parent
            width: 60
            height: 200
            delegate: Text {text: modelData}
            model: 10
            currentIndex: 1
        }
    }
    //QTBUG-127315
    Component {
        id: initialCurrentIndexTumblerNoDelegate

        Tumbler {
            anchors.centerIn: parent
            width: 60
            height: 200
            model: 10
        }
    }

    function test_initialCurrentIndex_data() {
        return [
            { tag: "delegate: true, wrap: true, currentIndex: 1",
                component: initialCurrentIndexTumbler, wrap: true, currentIndex: 1 },
            { tag: "delegate: true, wrap: false, currentIndex: 1",
                component: initialCurrentIndexTumbler, wrap: false, currentIndex: 1 },
            { tag: "delegate: false, wrap: true, currentIndex: 1",
                component: initialCurrentIndexTumblerNoDelegate, wrap: true, currentIndex: 1 },
            { tag: "delegate: false, wrap: false, currentIndex: 1",
                component: initialCurrentIndexTumblerNoDelegate, wrap: false, currentIndex: 1 },
            { tag: "delegate: true, wrap: true, currentIndex: 4",
                component: initialCurrentIndexTumbler, wrap: true, currentIndex: 4 },
            { tag: "delegate: true, wrap: false, currentIndex: 4",
                component: initialCurrentIndexTumbler, wrap: false, currentIndex: 4 },
            { tag: "delegate: false, wrap: true, currentIndex: 4",
                component: initialCurrentIndexTumblerNoDelegate, wrap: true, currentIndex: 4 },
            { tag: "delegate: false, wrap: false, currentIndex: 4",
                component: initialCurrentIndexTumblerNoDelegate, wrap: false, currentIndex: 4 },
        ]
    }

    function test_initialCurrentIndex(data) {
        let tumbler = createTemporaryObject(data.component, testCase,
                                            {wrap: data.wrap, currentIndex: data.currentIndex});
        compare(tumbler.currentIndex, data.currentIndex);
    }

    // QTBUG-109995
    Component {
        id: flickTumbler
        Flickable {
          width: 50
          height: 200
          interactive: true
          contentHeight: 400
          property alias tumblerItem: noWrapTumbler
          Tumbler {
              id: noWrapTumbler
              anchors.fill: parent
              model: 20
              wrap: false
          }
       }
    }

    function test_flick() {
        let control = createTemporaryObject(flickTumbler, testCase)
        verify(control)

        let tumbler = control.tumblerItem
        compare(tumbler.count, 20)
        compare(tumbler.wrap, false)

        let touch = touchEvent(tumbler)
        let tumblerView = findView(tumbler)
        let delegateHeight = tumblerView.children[0].children[0].height

        // Move delegates through touch operation and check the current index
        touch.press(0, tumblerView, control.width / 2, control.height / 2).commit()
        // Move slowly, otherwise its considered as flick which cause current index
        // to be varied according to its velocity
        let scrollOffset = control.height / 2
        for (; scrollOffset > delegateHeight / 2; scrollOffset-=5) {
            touch.move(0, tumblerView, control.width / 2, scrollOffset).commit()
        }
        touch.release(0, tumblerView, control.width / 2, scrollOffset).commit()
        tryCompare(tumblerView, "currentIndex", 2)
    }
}
