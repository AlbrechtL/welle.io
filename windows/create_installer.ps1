# Copyright (C) 2017
# Albrecht Lohofener (albrechtloh@gmx.de)
#
# This file is part of the welle.io.
# Many of the ideas as implemented in welle.io are derived from
# other work, made available through the GNU general Public License.
# All copyrights of the original authors are recognized.
#
# welle.io is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# welle.io is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with welle.io; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# Parameter
param(
[string]$welleExePath = "build\bin",
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

Write-Host "*** Copy welle-io binary files ***" -ForegroundColor Red
$welleExePath = $welleExePath + "\*" 
Copy-Item $welleExePath installer\packages\io.welle.welle\data\ -recurse

# Deploy QT and related plugins
Write-Host "*** Deploy QT and related plugins ***" -ForegroundColor Red
& windeployqt installer\packages\io.welle.welle\data\welle-io.exe --plugindir installer\packages\io.welle.welle\data\plugins\ --no-translations
& windeployqt installer\packages\io.welle.welle\data\welle-io.exe --dir installer\packages\io.welle.welle\data\qml\ --qmldir ..\src\welle-gui\QML\ --no-translations --no-plugins

Copy-Item installer\packages\io.welle.welle\data\qml\Qt5QuickControls2.dll installer\packages\io.welle.welle\data
Copy-Item installer\packages\io.welle.welle\data\qml\Qt5QuickTemplates2.dll installer\packages\io.welle.welle\data
Remove-Item installer\packages\io.welle.welle\data\qml\*.dll

# For some reason windeployqt deploys the wrong DLL on AppVeyor
Copy-Item $QTPath\libgcc_s_dw2-1.dll installer\packages\io.welle.welle\data
Copy-Item $QTPath\libstdc++-6.dll installer\packages\io.welle.welle\data

# Run binarycreator.exe
$Filename = $Date + "_" + $gitHash + "_Windows_welle-io-setup.exe"

Write-Host "*** Creating $Filename ***" -ForegroundColor Red
& "binarycreator" "--offline-only" "--config" "installer\config\config.xml" "--packages" "installer\packages" "$Filename"

# Store file name to a enviroment variable
$env:welle_io_filename = $Filename
