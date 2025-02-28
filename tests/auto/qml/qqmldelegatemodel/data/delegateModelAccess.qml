import QtQml

DelegateModel {
    id: root

    property Component typedDelegate: QtObject {
        required property QtObject model

        required property real x

        property real immediateX: x
        property real modelX: model.x

        function writeImmediate() {
            x = 1;
        }

        function writeThroughModel() {
            model.x = 3;
        }
    }

    property Component untypedDelegate: QtObject {
        property real immediateX: x
        property real modelX: model.x

        function writeImmediate() {
            x = 1;
        }

        function writeThroughModel() {
            model.x = 3;
        }
    }

    property ListModel singularModel: ListModel {
        ListElement {
            x: 11
        }
        ListElement {
            x: 12
        }
    }

    property ListModel listModel: ListModel {
        ListElement {
            x: 11
            y: 12
        }
        ListElement {
            x: 15
            y: 16
        }
    }

    property var array: [
        {x: 11, y: 12}, {x: 19, y: 20}
    ]

    property QtObject object: QtObject {
        property int x: 11
        property int y: 12
    }

    property int n: -1
    property int o: -1

    model: {
        switch (n) {
        case 0: return singularModel
        case 1: return listModel
        case 2: return array
        case 3: return object
        }
        return undefined;
    }

    delegate: {
        switch (o) {
        case 0: return untypedDelegate
        case 1: return typedDelegate
        }
        return null
    }
}
