import QtQuick.Controls

Pane {
    width: 400
    height: 400

    property alias textField: textField

    TextField {
        id: textField
        text: "Memento mori"
        width: parent.width
        anchors.centerIn: parent
    }
}
