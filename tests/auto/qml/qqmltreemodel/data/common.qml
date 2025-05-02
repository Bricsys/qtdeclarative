// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.qmlmodels

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel
    property alias treeView: treeView

    TreeView {
        id: treeView
        anchors.fill: parent
        model: TestModel {
            id: testModel
        }
        delegate: Text {
            text: model.display
        }
    }
}
