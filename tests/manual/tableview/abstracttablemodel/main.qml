// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Window
import QtQml.Models
import TestTableModel
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.qmlmodels

ApplicationWindow {
    id: window
    width: 1000
    height: 600
    visible: true

    readonly property var currentIndex: tableView.selectionModel.currentIndex
    readonly property point positionOffset: useOffset.checked ? Qt.point(10, 10) : Qt.point(0, 0)
    readonly property rect positionSubRect: useSubRect.checked ? Qt.rect(10, 10, 10, 10) : Qt.rect(0, 0, 0, 0)
    property int hiddenColumn: -1
    property int hiddenRow: -1

    ScrollView {
        id: menu
        height: window.height
        width: 230

        readonly property real menuMargin: 10

        ColumnLayout {
            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.topMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    ComboBox {
                        id: delegateComboBox
                        model: ["Default Delegate", "Custom Delegate", "Custom Item"]
                        Layout.fillWidth: true
                        currentIndex: 2
                    }

                    CheckBox {
                        text: "Use Syncview"
                        checkable: true
                        checked: true
                        Layout.fillWidth: false
                        onCheckedChanged: {
                            if (checked) {
                                leftHeader.syncView = tableView
                                topHeader.syncView = tableView
                            } else {
                                leftHeader.syncView = null
                                topHeader.syncView = null
                            }
                        }
                    }

                    CheckBox {
                        id: flickingMode
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Interactive"
                    }

                    CheckBox {
                        id: indexNavigation
                        checkable: true
                        checked: true
                        Layout.fillWidth: false
                        text: "Enable navigation"
                    }

                    CheckBox {
                        id: resizableRowsEnabled
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Resizable rows"
                    }

                    CheckBox {
                        id: resizableColumnsEnabled
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Resizable columns"
                    }

                    CheckBox {
                        id: movableColumnEnabled
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Reorder columns"
                    }

                    CheckBox {
                        id: movableRowEnabled
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Reorder rows"
                    }

                    CheckBox {
                        id: enableAnimation
                        checkable: true
                        checked: true
                        Layout.fillWidth: false
                        text: "Enable animation"
                    }

                    CheckBox {
                        id: drawText
                        checkable: true
                        checked: true
                        Layout.fillWidth: false
                        text: "Draw text"
                    }

                    CheckBox {
                        id: useRandomColor
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Use colors"
                    }

                    CheckBox {
                        id: useLargeCells
                        checkable: true
                        checked: false
                        text: "Use large cells"
                        Layout.fillWidth: false
                        onCheckedChanged: Qt.callLater(tableView.forceLayout)
                    }

                    CheckBox {
                        id: useSubRect
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Use subRect"
                    }

                    CheckBox {
                        id: useOffset
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Use offset"
                    }

                    CheckBox {
                        id: highlightCurrentRow
                        checkable: true
                        checked: false
                        Layout.fillWidth: false
                        text: "Highlight row/col"
                    }

                    CheckBox {
                        id: showSelectionHandles
                        checkable: true
                        checked: tableView.interactive
                        text: "Show selection handles"
                        Layout.fillWidth: false
                    }

                    ComboBox {
                        id: selectionCombo
                        currentIndex: 1
                        model: [
                            "SelectionDisabled",
                            "SelectCells",
                            "SelectRows",
                            "SelectColumns",
                        ]
                        Layout.fillWidth: false
                    }

                    ComboBox {
                        id: selectionModeTv
                        textRole: "text"
                        valueRole: "value"
                        implicitContentWidthPolicy: ComboBox.WidestText
                        currentIndex: 2
                        model: [
                            { text: "SingleSelection", value: TableView.SingleSelection },
                            { text: "ContiguousSelection", value: TableView.ContiguousSelection },
                            { text: "ExtendedSelection", value: TableView.ExtendedSelection }
                        ]
                        Layout.fillWidth: false
                    }

                    ComboBox {
                        id: selectionModeCombo
                        enabled: selectionCombo.currentText != "SelectionDisabled"
                        model: [
                            "Auto",
                            "Drag",
                            "PressAndHold",
                        ]
                        Layout.fillWidth: false
                    }
                    ComboBox {
                        id: editCombo
                        currentIndex: 2
                        model: [
                            "NoEditTriggers",
                            "SingleTapped",
                            "DoubleTapped",
                            "SelectedTapped",
                            "EditKeyPressed",
                            "AnyKeyPressed",
                        ]
                        Layout.fillWidth: false
                    }
                    Button {
                        text: "Clear selection"
                        onClicked: tableView.selectionModel.clearSelection()
                    }
                    Button {
                        text: "Clear column reordering"
                        onClicked: tableView.clearColumnReordering()
                    }
                    Button {
                        text: "Clear row reordering"
                        onClicked: tableView.clearRowReordering()
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                GridLayout {
                    columns: 2
                    Label { text: "Model size:" }
                    SpinBox {
                        id: modelSize
                        from: 0
                        to: 1000
                        value: 200
                        editable: true
                        Layout.fillWidth: false
                    }
                    Label { text: "Spacing:" }
                    SpinBox {
                        id: spaceSpinBox
                        from: -100
                        to: 100
                        value: 1
                        editable: true
                        Layout.fillWidth: false
                    }
                    Label { text: "Margins:" }
                    SpinBox {
                        id: marginsSpinBox
                        from: 0
                        to: 100
                        value: 0
                        editable: true
                        Layout.fillWidth: false
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                RowLayout {
                    id: positionRow
                    Button {
                        text: "<<"
                        Layout.fillWidth: false
                        onClicked: {
                            tableView.positionViewAtRow(0, Qt.AlignTop, -tableView.topMargin)
                            tableView.positionViewAtColumn(0, Qt.AlignLeft, -tableView.leftMargin)
                        }
                    }

                    Button {
                        text: ">>"
                        Layout.fillWidth: false
                        onClicked: {
                            tableView.positionViewAtRow(tableView.rows - 1, Qt.AlignBottom, tableView.bottomMargin)
                            tableView.positionViewAtColumn(tableView.columns - 1, Qt.AlignRight, tableView.rightMargin)
                        }
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    Button {
                        text: "Add row"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: tableView.model.insertRows(currentIndex.row, 1)
                    }

                    Button {
                        text: "Remove row"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: tableView.model.removeRows(currentIndex.row, 1)
                    }

                    Button {
                        text: "Add column"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: tableView.model.insertColumns(currentIndex.column, 1)
                    }

                    Button {
                        text: "Remove column"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: tableView.model.removeColumns(currentIndex.column, 1)
                    }

                    Button {
                        text: "Hide column"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            hiddenColumn = currentIndex.column
                            tableView.forceLayout()
                        }
                    }

                    Button {
                        text: "Hide row"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            hiddenRow = currentIndex.row
                            tableView.forceLayout()
                        }
                    }

                    Button {
                        text: "Show hidden column"
                        enabled: hiddenColumn >= 0
                        Layout.fillWidth: false
                        onClicked: {
                            hiddenColumn = -1
                            tableView.forceLayout()
                        }
                    }

                    Button {
                        text: "Show hidden row"
                        enabled: hiddenRow >= 0
                        Layout.fillWidth: false
                        onClicked: {
                            hiddenRow = -1
                            tableView.forceLayout()
                        }
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    Button {
                        text: "Current to top-left"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            let cell = Qt.point(currentIndex.column, currentIndex.row)
                            tableView.positionViewAtCell(cell, Qt.AlignTop | Qt.AlignLeft, positionOffset, positionSubRect)
                        }
                    }

                    Button {
                        text: "Current to center"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            let cell = Qt.point(currentIndex.column, currentIndex.row)
                            tableView.positionViewAtCell(cell, Qt.AlignCenter, positionOffset, positionSubRect)
                        }
                    }

                    Button {
                        text: "Current to bottom-right"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            let cell = Qt.point(currentIndex.column, currentIndex.row)
                            tableView.positionViewAtCell(cell, Qt.AlignBottom | Qt.AlignRight, positionOffset, positionSubRect)
                        }
                    }

                    Button {
                        text: "Current to Visible"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            let cell = Qt.point(currentIndex.column, currentIndex.row)
                            tableView.positionViewAtCell(cell, TableView.Visible, positionOffset, positionSubRect)
                        }
                    }

                    Button {
                        text: "Current to Contain"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            let cell = Qt.point(currentIndex.column, currentIndex.row)
                            tableView.positionViewAtCell(cell, TableView.Contain, positionOffset, positionSubRect)
                        }
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    Button {
                        text: "Open editor"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            const currentItem = tableView.itemAtIndex(currentIndex)
                            // check item is visible
                            if (!currentItem)
                                return
                            tableView.edit(currentIndex)
                        }
                    }
                    Button {
                        text: "Close editor"
                        enabled: currentIndex.valid
                        Layout.fillWidth: false
                        onClicked: {
                            tableView.closeEditor()
                        }
                    }
                    Button {
                        text: "Set current index"
                        Layout.fillWidth: false
                        onClicked: {
                            let index = tableView.index(1, 1);
                            tableView.selectionModel.setCurrentIndex(index, ItemSelectionModel.NoUpdate)
                        }
                    }
                }
            }

            GroupBox {
                Layout.minimumWidth: menu.availableWidth - (menu.menuMargin * 2)
                Layout.rightMargin: menu.menuMargin
                Layout.leftMargin: menu.menuMargin
                Layout.bottomMargin: menu.menuMargin
                Layout.fillWidth: false
                Layout.fillHeight: false
                ColumnLayout {
                    Button {
                        text: "Fast-flick table"
                        Layout.fillWidth: false
                        onClicked: {
                            tableView.contentX += tableView.width * 1.2
                        }
                    }

                    Button {
                        text: "Fast-flick headers"
                        Layout.fillWidth: false
                        onClicked: {
                            topHeader.contentX += tableView.width * 1.2
                            leftHeader.contentY += tableView.height * 1.2
                        }
                    }

                    Button {
                        text: "ForceLayout()"
                        Layout.fillWidth: false
                        onClicked: tableView.forceLayout()
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    HorizontalHeaderView {
        id: topHeader
        objectName: "topHeader"
        anchors.left: centerScrollView.left
        anchors.right: centerScrollView.right
        anchors.top: menu.top
        height: 30
        clip: true

        model: TestTableModel {
            rowCount: 1
            columnCount: modelSize.value
        }

        delegate: [defaultHorizontalHeaderDelegate, customHorizontalHeaderDelegate,
            customHorizontalItem][delegateComboBox.currentIndex]

        columnSpacing: 1
        rowSpacing: 1

        syncView: tableView
        syncDirection: Qt.Horizontal
        movableColumns: movableColumnEnabled.checked
        resizableColumns: resizableColumnsEnabled.checked

        Component {
            id: defaultHorizontalHeaderDelegate
            HorizontalHeaderViewDelegate { }
        }

        Component {
            id: customHorizontalHeaderDelegate
            HorizontalHeaderViewDelegate {
                id: horizontalHeaderDelegate

                required property bool containsDrag
                required property int column

                background: Rectangle {
                    color: window.palette.alternateBase
                    border.width: horizontalHeaderDelegate.containsDrag ? 1 : 0
                    border.color: horizontalHeaderDelegate.containsDrag
                                  ? window.palette.text
                                  : window.palette.alternateBase
                }

                contentItem: Label {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    visible: drawText.checked
                    text: horizontalHeaderDelegate.column
                    font.pointSize: 8
                }
            }
        }

        Component {
            id: customHorizontalItem

            Rectangle {
                required property bool containsDrag
                implicitHeight: topHeader.height
                implicitWidth: 20
                border.width: containsDrag ? 1 : 0
                border.color: containsDrag ? window.palette.text : window.palette.alternateBase
                color: window.palette.alternateBase
                Label {
                    anchors.centerIn: parent
                    visible: drawText.checked
                    text: column
                    font.pointSize: 8
                }
            }
        }
    }

    VerticalHeaderView {
        id: leftHeader
        objectName: "leftHeader"
        anchors.left: menu.right
        anchors.top: centerScrollView.top
        anchors.bottom: centerScrollView.bottom
        width: 30
        clip: true

        model: TestTableModel {
            rowCount: modelSize.value
            columnCount: 1
        }

        delegate: [defaultVerticalHeaderDelegate, customVerticalHeaderDelegate,
            customVerticalItem][delegateComboBox.currentIndex]

        columnSpacing: 1
        rowSpacing: 1

        syncView: tableView
        syncDirection: Qt.Vertical
        movableRows: movableRowEnabled.checked
        resizableRows: resizableRowsEnabled.checked

        Component {
            id: defaultVerticalHeaderDelegate
            VerticalHeaderViewDelegate { }
        }

        Component {
            id: customVerticalHeaderDelegate
            VerticalHeaderViewDelegate {
                id: verticalHeaderDelegate

                required property bool containsDrag
                required property int row

                background: Rectangle {
                    color: window.palette.alternateBase
                    border.width: verticalHeaderDelegate.containsDrag ? 1 : 0
                    border.color: verticalHeaderDelegate.containsDrag
                                  ? window.palette.text
                                  : window.palette.alternateBase
                }

                contentItem: Label {
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    visible: drawText.checked
                    text: verticalHeaderDelegate.row
                    font.pointSize: 8
                }
            }
        }

        Component {
            id: customVerticalItem

            Rectangle {
                required property bool containsDrag
                implicitHeight: 50
                implicitWidth: leftHeader.width
                border.width: containsDrag ? 1 : 0
                border.color: containsDrag ? window.palette.text
                                           : window.palette.alternateBase
                color: window.palette.alternateBase
                Label {
                    anchors.centerIn: parent
                    visible: drawText.checked
                    text: row
                    font.pointSize: 8
                }
            }
        }
    }

    Item {
        id: centerScrollView
        anchors.left: leftHeader.right
        anchors.right: parent.right
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom
        anchors.topMargin: 1
        anchors.leftMargin: 1
        anchors.rightMargin: 10
        anchors.bottomMargin: 10

        TableView {
            id: tableView
            anchors.fill: parent
            objectName: "tableview"
            clip: true
            delegate: [defaultTableViewDelegate, customTableViewDelegate, customDelegateItem][delegateComboBox.currentIndex]
            columnSpacing: spaceSpinBox.value
            rowSpacing: spaceSpinBox.value
            interactive: flickingMode.checked
            keyNavigationEnabled: indexNavigation.checked
            pointerNavigationEnabled: indexNavigation.checked
            resizableRows: resizableRowsEnabled.checked
            resizableColumns: resizableColumnsEnabled.checked
            animate: enableAnimation.checked
            selectionBehavior: {
                switch (selectionCombo.currentText) {
                case "SelectCells": return TableView.SelectCells
                case "SelectRows": return TableView.SelectRows
                case "SelectColumns": return TableView.SelectColumns
                }
                return TableView.SelectionDisabled
            }
            selectionMode: selectionModeTv.currentValue
            editTriggers: {
                switch (editCombo.currentText) {
                case "NoEditTriggers": return TableView.NoEditTriggers
                case "SingleTapped": return TableView.SingleTapped
                case "DoubleTapped": return TableView.DoubleTapped
                case "SelectedTapped": return TableView.SelectedTapped
                case "EditKeyPressed": return TableView.EditKeyPressed
                case "AnyKeyPressed": return TableView.AnyKeyPressed
                }
                return TableView.SelectionDisabled
            }

            leftMargin: marginsSpinBox.value
            topMargin: marginsSpinBox.value
            rightMargin: marginsSpinBox.value
            bottomMargin: marginsSpinBox.value

            columnWidthProvider: function(column) {
                return (column === window.hiddenColumn) ? 0 : explicitColumnWidth(column)
            }
            rowHeightProvider: function(row) {
                return (row === window.hiddenRow) ? 0 : explicitRowHeight(row)
            }

            ScrollBar.horizontal: ScrollBar { visible: !flickingMode.checked }
            ScrollBar.vertical: ScrollBar { visible: !flickingMode.checked }

            model: TestTableModel {
                rowCount: modelSize.value
                columnCount: modelSize.value
            }

            selectionModel: ItemSelectionModel {}
        }
    }

    SelectionRectangle {
        id: selectionRectangle
        target: tableView
        enabled: selectionCombo.currentText != "SelectionDisabled"
        topLeftHandle: Rectangle {
            width: 20
            height: 20
            radius: 10
            color: "blue"
            visible: showSelectionHandles.checked && selectionRectangle.active
        }

        bottomRightHandle: Rectangle {
            width: 20
            height: 20
            radius: 10
            color: "green"
            visible: showSelectionHandles.checked && selectionRectangle.active
        }

        selectionMode: {
            switch (selectionModeCombo.currentText) {
            case "Drag": return SelectionRectangle.Drag
            case "PressAndHold": return SelectionRectangle.PressAndHold
            }
            return SelectionRectangle.Auto
        }
    }

    Component {
        id: defaultTableViewDelegate
        TableViewDelegate { }
    }

    Component {
        id: customTableViewDelegate
        TableViewDelegate {
            id: delegate
            implicitWidth: useLargeCells.checked ? 1000 : 50
            implicitHeight: useLargeCells.checked ? 1000 : 30

            property var randomColor: Qt.rgba(0.6 + (0.4 * Math.random()), 0.6 + (0.4 * Math.random()), 0.6 + (0.4 * Math.random()), 1)
            property color backgroundColor: selected ? window.palette.highlight
                                                     : (highlightCurrentRow.checked && (row === tableView.currentRow || column === tableView.currentColumn)) ? "lightgray"
                                                     : useRandomColor.checked ? randomColor
                                                     : model.display === "added" ? "lightblue"
                                                     : window.palette.window.lighter(1.3)
            onBackgroundColorChanged: background.color = backgroundColor

            contentItem.visible: drawText.checked && !editing

            Component.onCompleted: {
                contentItem.verticalAlignment = Text.AlignVCenter
                contentItem.horizontalAlignment = Text.AlignHCenter
            }

            Rectangle {
                x: positionSubRect.x
                y: positionSubRect.y
                width: positionSubRect.width
                height: positionSubRect.height
                border.color: "red"
                visible: useSubRect.checked
            }
        }
    }

    Component {
        id: customDelegateItem
        Rectangle {
            id: delegate
            implicitWidth: useLargeCells.checked ? 1000 : 50
            implicitHeight: useLargeCells.checked ? 1000 : 30
            border.width: current ? 3 : 0
            border.color: window.palette.highlight
            property var randomColor: Qt.rgba(0.6 + (0.4 * Math.random()), 0.6 + (0.4 * Math.random()), 0.6 + (0.4 * Math.random()), 1)
            color: selected ? window.palette.highlight
                            : (highlightCurrentRow.checked && (row === tableView.currentRow || column === tableView.currentColumn)) ? "lightgray"
                            : useRandomColor.checked ? randomColor
                            : model.display === "added" ? "lightblue"
                            : window.palette.window.lighter(1.3)

            required property bool selected
            required property bool current
            required property bool editing

            Rectangle {
                x: positionSubRect.x
                y: positionSubRect.y
                width: positionSubRect.width
                height: positionSubRect.height
                border.color: "red"
                visible: useSubRect.checked
            }

            Label {
                anchors.centerIn: parent
                visible: drawText.checked && !editing
                text: model.display
            }

            TableView.editDelegate: TextField {
                anchors.fill: parent
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                text: display

                TableView.onCommit: display = text
                Component.onCompleted: selectAll()
           }
        }
    }

}
