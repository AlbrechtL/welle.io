/*
 *    Copyright (C) 2017 - 2021
 *    Albrecht Lohofener (albrechtloh@gmx.de)
 *
 *    This file is part of the welle.io.
 *    Many of the ideas as implemented in welle.io are derived from
 *    other work, made available through the GNU general Public License.
 *    All copyrights of the original authors are recognized.
 *
 *    welle.io is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    welle.io is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with welle.io; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
pragma Singleton

import QtQuick 2.2
//import QtQuick.Controls 1.2
//import QtQuick.Controls.Private 1.0
//import QtQuick.Layouts 1.1

// Source: http://www.mimec.org/node/399 14. May 2016

QtObject {
    function dp( x ) {
//        return Math.round( x * Settings.dpiScaleFactor );
        return x; // No scaling
    }

    function em( x ) {
        //return Math.round( x * TextSingleton.font.pixelSize );
//        return Math.round( x * Settings.dpiScaleFactor * 12 ); // Scale the font to a pixel size of 12 pixels
        return x * 12; // No scaling
    }

//    Component.onCompleted: {
//          console.debug("dpiScaleFactor: " + Settings.dpiScaleFactor)
//       }
}
