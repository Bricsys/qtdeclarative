import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Proxy 1.0

GridLayout{
    width: 500
    height: 300
    anchors.fill: parent
    rows: 4
    flow: GridLayout.TopToBottom

    SearchField {
        live: false
    }

    // TO-DO: Add a test case for autoSuggest property
    // SearchField {
    //     autoSuggest: true
    // }

    SearchField {
        suggestionModel: ListModel {
            ListElement { color: "blue" }
            ListElement { color: "green" }
            ListElement { color: "red" }
            ListElement { color: "yellow" }
            ListElement { color: "orange" }
            ListElement { color: "purple" }
            ListElement { color: "cyan" }
            ListElement { color: "magenta" }
            ListElement { color: "chartreuse" }
            ListElement { color: "aquamarine" }
            ListElement { color: "indigo" }
            ListElement { color: "black" }
            ListElement { color: "lightsteelblue" }
            ListElement { color: "violet" }
            ListElement { color: "grey" }
            ListElement { color: "springgreen" }
            ListElement { color: "salmon" }
            ListElement { color: "blanchedalmond" }
            ListElement { color: "forestgreen" }
            ListElement { color: "pink" }
            ListElement { color: "navy" }
            ListElement { color: "goldenrod" }
            ListElement { color: "crimson" }
            ListElement { color: "turquoise" }
        }
    }

    SearchField {
        suggestionModel: ["January", "February", "March", "April", "May", "June", "July", "August",
            "September", "October", "November", "December"]
    }

    SearchField {
        suggestionModel: ListModel {
            ListElement { name: "Apple"; color: "green" }
            ListElement { name: "Cherry"; color: "red" }
            ListElement { name: "Banana"; color: "yellow" }
            ListElement { name: "Orange"; color: "orange" }
            ListElement { name: "WaterMelon"; color: "pink" }
        }
        textRole: "color"
    }

    SearchField {
        id: searchField
        suggestionModel: QSortFilterProxyModel {
            id: colorModel
            sourceModel: ListModel {
                ListElement { color: "blue" }
                ListElement { color: "green" }
                ListElement { color: "red" }
                ListElement { color: "yellow" }
                ListElement { color: "orange" }
                ListElement { color: "purple" }
            }
        }
        onTextChanged: {
            colorModel.filterRegularExpression = new RegExp(searchField.text, "i")
        }
    }
}
