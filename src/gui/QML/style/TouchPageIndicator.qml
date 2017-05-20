import QtQuick 2.7
import QtQuick.Controls 2.0


// Import custom styles
import "."

PageIndicator {
    id: indicator

    count: view.count
    currentIndex: view.currentIndex

    anchors.bottom: view.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    delegate: Rectangle {
        implicitWidth: Units.dp(10)
        implicitHeight: Units.dp(10)

        radius: width / 2
        color: "grey"

        opacity: index === indicator.currentIndex ? 0.95 : pressed ? 1 : 0.45

        Behavior on opacity {
            OpacityAnimator {
                duration: 100
            }
        }
    }
}
