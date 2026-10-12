# Translating welle.io

## Introduction
To translate welle.io in your native language QT provides a set of tools. This description shows how to translate welle.io.  

For more information about the translation process please visit the official QT documentation https://doc.qt.io/qt-5/linguist-overview.html.

## How to
1. Add your language in i18n format to file "welle.io.pro". In this example I show how to translate the app into "new_language"
  ```
TRANSLATIONS = i18n/de_DE.ts i18n/new_language.ts
  ```
2. Run "lupdate". If you use QT creator, read [this page](https://doc.qt.io/qtcreator/creator-editor-external.html) ,please.
  ```
Updating 'i18n/de_DE.ts'...
    Found 142 source text(s) (0 new and 142 already existing)
Updating 'i18n/new_language.ts'...
    Found 142 source text(s) (142 new and 0 already existing)
  ```
3. Start "Qt Linguist" and load the file "i18n/new_language.ts"
4. Choose the target language and start the translation
5. Finish your translation
6. Run "lrelease" to create a "i18n/new_language.qm" which contains all the translations usable by welle.io
7. Add the QM-file to the resource file "resources.qrc"
  ```
<file>i18n/new_language.qm</file>
  ```
8. Compile welle.io and test the translation with the command line option "-L"  
More command line information: https://github.com/albrechtl/welle.io#usage
9. Add both files "i18n/new_language.ts" and "i18n/new_language.qm" the the git repository
10. Create a pull request to merge the new language into the main branch.

