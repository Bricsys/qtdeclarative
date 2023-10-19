// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
//![0]
import QtQuick

Rectangle {
   id: myItem
   signal message(string msg)

   width: 100; height: 100

   MouseArea {
       anchors.fill: parent
       onClicked: myItem.message("clicked!")
   }
}
//![0]
