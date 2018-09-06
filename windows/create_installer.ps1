# Parameter
param(
[string]$welleExe = "..\..\build-welle.io-Desktop_Qt_5_9_3_MinGW_32bit-Release\release\welle-io.exe",
[string]$QTPath = "C:\Qt\5.9.3\mingw53_32\bin",
[string]$ToolsPath = "C:\Qt\Tools\mingw530_32\bin",
[string]$InstallerPath = "C:\Qt\Tools\QtInstallerFramework\2.0\bin\"
)

# Get current GIT hash and date
& "git" "-C" "..\" "rev-parse" "--short" "HEAD" | Tee-Object -Variable gitHash | Out-Null
Write-Host "Current GIT hash $gitHash"  -ForegroundColor Green

$Date = Get-Date -Format FileDate
Write-Host "Current date $Date"  -ForegroundColor Green

# Set PATH
$env:path += "$QTPath;$ToolsPath;$InstallerPath"

# Delete old folder and create a new one
if(Test-Path installer)
{
	Write-Host "*** Delete old installer folder ***" -ForegroundColor Red
	Remove-Item -Recurse -Force installer
}

Write-Host "*** Create new installer folder ***" -ForegroundColor Red
New-Item -ItemType directory -Path installer

# Copy installer data to installer folder
Write-Host "*** Copy installer data to installer folder ***" -ForegroundColor Red
Copy-Item config installer\config -recurse
Copy-Item packages installer\packages -recurse

Write-Host "*** Copy non QT DLLs from welle.io-win-libs repository ***" -ForegroundColor Red
Copy-Item ..\..\welle.io-win-libs\x86_install\* installer\packages\io.welle.welle\data  -recurse

Write-Host "*** Copy welle-io.exe ***" -ForegroundColor Red
Copy-Item $WelleExe installer\packages\io.welle.welle\data\welle-io.exe

# Deploy QT and related plugins
Write-Host "*** Deploy QT and related plugins ***" -ForegroundColor Red
& windeployqt installer\packages\io.welle.welle\data\welle-io.exe --plugindir installer\packages\io.welle.welle\data\plugins\ --no-translations
& windeployqt installer\packages\io.welle.welle\data\welle-io.exe --dir installer\packages\io.welle.welle\data\qml\ --qmldir ..\src\welle-gui\QML\ --no-translations --no-plugins

Copy-Item installer\packages\io.welle.welle\data\qml\Qt5QuickControls2.dll installer\packages\io.welle.welle\data
Copy-Item installer\packages\io.welle.welle\data\qml\Qt5QuickTemplates2.dll installer\packages\io.welle.welle\data
Remove-Item installer\packages\io.welle.welle\data\qml\*.dll

# Run binarycreator.exe
$Filename = $Date + "_" + $gitHash + "_Windows_welle-io-setup.exe"

Write-Host "*** Creating $Filename ***" -ForegroundColor Red
& "binarycreator" "--offline-only" "--config" "installer\config\config.xml" "--packages" "installer\packages" "$Filename"

# Store file name to a enviroment variable
$env:welle_io_filename = $Filename
