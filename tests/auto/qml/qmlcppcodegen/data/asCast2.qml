// Copyright (C) 2021 The Qt Company Ltd.

import QtQuick

Item {
    property QtObject nullAsObject: null as QtObject
    property QtObject nullAsItem: null as Item
    property QtObject nullAsRectangle: null as Rectangle
    property QtObject nullAsDummy: null as Dummy
    property QtObject undefinedAsObject: undefined as QtObject
    property QtObject undefinedAsItem: undefined as Item
    property QtObject undefinedAsRectangle: undefined as Rectangle
    property QtObject undefinedAsDummy: undefined as Dummy
}
