import QtQuick

ListView {
    id: view

    reuseItems: true
    width: 100
    height: 100

    model: 200
    clip: true
    cacheBuffer: 0

    delegate: Item {
        width: 1
        height: 1
        ListView.onPooled: {
            // When pooled, immediately trigger drain by geometry change
            --view.width
        }
        onParentChanged: {
            // When unparented as result of drain, trigger another drain
            if (parent === null)
                ++view.width
        }
    }

    Timer {
        running: true
        repeat: true
        interval: 1
        onTriggered: {
            --view.height
            ++view.contentY
        }
    }
}
