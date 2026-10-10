/*
 *    This file is part of the welle.io.
 *    This file was generated with an AI assistant (Claude Code).
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

import QtQuick

// Slides of the MOT slide show (SLS) of the station that plays, including
// the categories of a categorized slide show. MainView has one instance,
// every SlideShow shows it.
QtObject {
    id: model

    // Image to show (image://SLS/...), empty if there is none
    property string source: ""

    // Categories: categoryTitle, categoryId, currentPictureIndex.
    // Category 0 ("Latest") always shows the latest slide.
    property ListModel categories: ListModel {}
    property int currentCategoryId: 0

    // Number of pictures of the selected category
    readonly property int pictureCount: (slideList.length > currentCategoryListPos)
                                        ? slideList[currentCategoryListPos][2].length : 0

    // [[categoryTitle, categoryId, [[slideId, pictureName], ...]], ...]
    property var slideList: []
    property int currentCategoryListPos: 0
    property string latestPictureName: ""

    property Connections guiHelperConnection: Connections {
        target: guiHelper

        function onMotChanged(pictureName, categoryTitle, categoryId, slideId) {
            model.latestPictureName = pictureName

            // Display slide show only of latest tab is select
            if (model.currentCategoryId === 0)
                model.source = "image://SLS/" + pictureName

            // Handle categories if exist
            if (categoryTitle !== "" && categoryId !== 0) {
                // Check if category exists, if not create it
                var foundCategory = false
                for (var i = 0; i < model.slideList.length; ++i) {
                    if (model.slideList[i][1] === categoryId)
                        foundCategory = true
                }

                if (!foundCategory) {
                    model.categories.append({"categoryTitle": categoryTitle, "categoryId": categoryId, "currentPictureIndex": 0})
                    model.slideList.push([categoryTitle, categoryId, []])
                }

                // Put picture name into category
                for (i = 0; i < model.slideList.length; ++i) {
                    if (model.slideList[i][1] !== categoryId)
                        continue

                    // Replace the picture if it is already in the list
                    var foundPicture = false
                    for (var j = 0; j < model.slideList[i][2].length; j++) {
                        if (model.slideList[i][2][j][0] === slideId) {
                            model.slideList[i][2][j][1] = pictureName
                            foundPicture = true
                        }
                    }

                    if (!foundPicture)
                        model.slideList[i][2].push([slideId, pictureName])
                }
                model.slideListChanged()
            }
        }

        function onMotReseted() {
            model.slideList = []
            model.categories.clear()
            model.currentCategoryId = 0
            model.currentCategoryListPos = 0
            model.latestPictureName = ""
            model._initCategories()
            model.source = ""
        }
    }

    onSlideListChanged: _updateSlide()

    function selectCategory(categoryId) {
        currentCategoryId = categoryId
        _updateSlide()
    }

    // Previous or next picture of the selected category
    function previous() { _step(-1) }
    function next() { _step(1) }

    function _step(direction) {
        if (pictureCount < 2)
            return
        var category = categories.get(currentCategoryListPos)
        var index = (category.currentPictureIndex + direction + pictureCount) % pictureCount
        category.currentPictureIndex = index
        source = "image://SLS/" + slideList[currentCategoryListPos][2][index][1]
    }

    function _updateSlide() {
        // Find the position of the selected category
        for (var i = 0; i < slideList.length; ++i) {
            if (slideList[i][1] === currentCategoryId) {
                currentCategoryListPos = i
                break
            }
        }

        // The latest slide, also if there are no categories
        if (currentCategoryId === 0) {
            if (latestPictureName !== "")
                source = "image://SLS/" + latestPictureName
            return
        }

        // Last picture of the selected category
        var pictures = slideList[currentCategoryListPos][2]
        if (pictures.length > 0) {
            categories.get(currentCategoryListPos).currentPictureIndex = pictures.length - 1
            source = "image://SLS/" + pictures[pictures.length - 1][1]
        }
    }

    // The "Latest" category. motReseted() can come before onCompleted.
    function _initCategories() {
        if (categories.count > 0)
            return
        categories.append({"categoryTitle": "Latest", "categoryId": 0, "currentPictureIndex": 0})
        slideList.push(["Latest", 0, []])
    }

    Component.onCompleted: _initCategories()
}
