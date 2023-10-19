// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
//![0]
import QtQuick

Item {
    Item {
        focus: true
        Keys.onPressed: (event)=> {
            console.log("KeyReader captured:",
                        event.text);
            event.accepted = true;
        }
    }
}
//![0]
