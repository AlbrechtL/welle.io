import QtQuick 2.0
import QtQuick.Layouts 1.1

// Import custom styles
import "."

Item{
    id: textExpert
    property alias name: nameView.text
    property alias text: textView.text

    height: textLayout.height
    width: textLayout.width

    RowLayout{
        id: textLayout

        Text{
            id: nameView
            font.pixelSize: TextStyle.textStandartSize
            font.family: TextStyle.textFont
            //color: TextStyle.textColor
        }
        Text{
            id: textView
            font.pixelSize: TextStyle.textStandartSize
            font.family: TextStyle.textFont
            //color: TextStyle.textColor
            verticalAlignment: Text.AlignVCenter
            Layout.maximumWidth: (parent.parent.parent.isServiceDetailsRawLayout == true) ? 
                parent.parent.parent.parent.parent.width - nameView.width - 16 :
                parent.parent.parent.width - nameView.width
            fontSizeMode: Text.Fit
            minimumPixelSize: 8;
        }
    }
}
