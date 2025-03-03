import QtQuick

Item {
    property bool useObjectModel: false

    ObjectModel {
        id: objectModel
        Item {}
        Item {}
    }

    DelegateModel {
        id: delegateModel
        model: [1, 2, 3]
    }

    ListView {
        id: view
        anchors.fill: parent
        model: useObjectModel ? objectModel : delegateModel
    }

    Component {
        id: delegate
        Rectangle {
            width: 100
            height: 100
            color: "green"
        }
    }

    property int count: view.count

    // Set the delegate after the model
    Component.onCompleted: view.delegate = delegate
}
