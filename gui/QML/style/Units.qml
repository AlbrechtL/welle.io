pragma Singleton

import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Private 1.0
import QtQuick.Layouts 1.1

// Source: http://www.mimec.org/node/399 14. May 2016

QtObject {
    function dp( x ) {
        return Math.round( x * Settings.dpiScaleFactor );
    }

    function em( x ) {
        //return Math.round( x * TextSingleton.font.pixelSize );
        return Math.round( x * Settings.dpiScaleFactor * 12 ); // Scale the font to a pixel size of 12 pixels
    }

    function pad(n, width, z) {
      z = z || '0';
      n = n + '';
      return n.length >= width ? n : new Array(width - n.length + 1).join(z) + n;
    }

    Component.onCompleted: {
          console.log("GUI dpiScaleFactor: " + Settings.dpiScaleFactor)
       }
}
