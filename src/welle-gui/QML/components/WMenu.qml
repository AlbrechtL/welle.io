import QtQuick 2.0
import QtQuick.Controls 2.3

import "../texts"

Menu {
    id: menu
    property bool sizeToContents
    property int menuWidth
    width: (sizeToContents) ? menuWidth + leftPadding + rightPadding : implicitWidth

    font.pixelSize: TextStyle.textStandartSize
    font.family: TextStyle.textFont

    onAboutToShow: {
        var itemwidth = 0;
        var leftpadding = 0;
        var rightpadding = 0;
        
        for(var i = 0; i < count; i++) {
            var item = itemAt(i);
            itemwidth = Math.max(item.contentItem.implicitWidth, itemwidth);
            leftpadding = Math.max(item.leftPadding, leftpadding);
            rightpadding = Math.max(item.rightPadding, rightpadding);
        }
        menuWidth = Math.ceil(itemwidth + leftpadding + rightpadding);
        
        for (var i = 0; i < count; i++) {
            itemAt(i).width = menuWidth
        }
    }
}
