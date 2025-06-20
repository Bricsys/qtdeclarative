// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
pragma ComponentBehavior: Bound
import Charts
import QtQuick

Item {
    width: 300; height: 200

    PieChart {
        id: chart
        anchors.centerIn: parent
        width: 100; height: 100

        component Slice: PieSlice {
            parent: chart
            anchors.fill: parent
        }

        slices: [
            Slice {
                color: "red"
                fromAngle: 0
                angleSpan: 110
            },
            Slice {
                color: "black"
                fromAngle: 110
                angleSpan: 50
            },
            Slice {
                color: "blue"
                fromAngle: 160
                angleSpan: 100
            }
        ]
    }
}
//![0]
