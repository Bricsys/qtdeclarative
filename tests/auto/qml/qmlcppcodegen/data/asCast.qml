// Copyright (C) 2021 The Qt Company Ltd.

pragma Strict
pragma ValueTypeBehavior: Assertable

import QtQuick
import QtQml as QQ

Item {
    QtObject { id: object }
    Item { id: item }
    Rectangle { id: rectangle }
    Dummy { id: dummy }

    property string string: "a"
    property url url: "http://example.com"
    property date date: new Date(1996, 2, 3)

    property QtObject objectAsObject: object as QtObject
    property QtObject objectAsItem: object as Item
    property QtObject objectAsRectangle: object as Rectangle
    property QtObject objectAsDummy: object as Dummy

    property QtObject itemAsObject: item as QtObject
    property QtObject itemAsItem: item as Item
    property QtObject itemAsRectangle: item as Rectangle
    property QtObject itemAsDummy: item as Dummy

    property QtObject rectangleAsObject: rectangle as QtObject
    property QtObject rectangleAsItem: rectangle as Item
    property QtObject rectangleAsRectangle: rectangle as Rectangle
    property QtObject rectangleAsDummy: rectangle as Dummy

    property QtObject dummyAsObject: dummy as QtObject
    property QtObject dummyAsItem: dummy as Item
    property QtObject dummyAsRectangle: dummy as Rectangle
    property QtObject dummyAsDummy: dummy as Dummy

    property QtObject nullAsObject: null as QtObject
    property QtObject nullAsItem: null as Item
    property QtObject nullAsRectangle: null as Rectangle
    property QtObject nullAsDummy: null as Dummy

    property QtObject undefinedAsObject: undefined as QtObject
    property QtObject undefinedAsItem: undefined as Item
    property QtObject undefinedAsRectangle: undefined as Rectangle
    property QtObject undefinedAsDummy: undefined as Dummy

    property var stringAsString: string as QQ.string
    property var urlAsUrl: url as QQ.url
    property var dateAsDate: date as QQ.date
}
