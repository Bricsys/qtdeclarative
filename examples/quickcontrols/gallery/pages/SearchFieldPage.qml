// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ScrollablePage {
    id: page

    Column {
        spacing: 40
        width: parent.width

        Label {
            width: parent.width
            wrapMode: Label.Wrap
            horizontalAlignment: Qt.AlignHCenter
            text: qsTr("SearchField is a styled text input for searching, typically "
                       + "with a magnifier and clear icon.")
        }

        ListModel {
            id: colorModel
            ListElement { color: "blue" }
            ListElement { color: "green" }
            ListElement { color: "red" }
            ListElement { color: "yellow" }
            ListElement { color: "orange" }
            ListElement { color: "purple" }
        }

        SortFilterProxyModel {
            id: colorFilter
            sourceModel: colorModel
            filterRegularExpression: RegExp(colorSearch.text, "i")
        }

        SearchField {
            id: colorSearch
            suggestionModel: colorFilter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }
}
