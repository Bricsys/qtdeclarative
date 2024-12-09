// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    visible: true
    width: 200
    height: 200

    required property bool itemFocused

    property bool receivedKeyPress: false

    Item {
        objectName: "item"
        focus: itemFocused
        anchors.fill: parent

        Keys.onLeftPressed: receivedKeyPress = true
    }
}

