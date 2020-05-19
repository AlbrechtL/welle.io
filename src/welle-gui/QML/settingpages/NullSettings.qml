import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "../texts"
import "../components"

Item {
    function initDevice(isAutoDevice) {
        if(!isAutoDevice)
            guiHelper.openNull()
    }
}
