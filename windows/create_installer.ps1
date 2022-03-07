# Copyright (C) 2017 - 2022
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
[string]$welleExePath = "..\..\build-welle.io-Desktop_Qt_6_2_3_MinGW_64_bit-Release\src\welle-gui\release",
[string]$QTPath = "C:\Qt\6.2.3\mingw_64\bin",
[string]$ToolsPath = "C:\Qt\Tools\mingw900_64\bin",
[string]$InnoSetupPath = "C:\Program Files (x86)\Inno Setup 6"
)

# Get current GIT hash and date
& "git" "-C" "..\" "rev-parse" "--short" "HEAD" | Tee-Object -Variable gitHash | Out-Null
Write-Host "Current GIT hash $gitHash"  -ForegroundColor Green

$Date = Get-Date -Format FileDate
Write-Host "Current date $Date"  -ForegroundColor Green

# Set PATH
$env:path += "$QTPath;$ToolsPath;$InnoSetupPath;"

# Delete old folder and create a new one
if(Test-Path bin)
{
	Write-Host "*** Delete old bin folder ***" -ForegroundColor Red
	Remove-Item -Recurse -Force bin
}

Write-Host "*** Create new bin folder ***" -ForegroundColor Red
New-Item -ItemType directory -Path bin

Write-Host "*** Copy non QT DLLs from welle.io-win-libs repository ***" -ForegroundColor Red
Copy-Item ..\..\welle.io-win-libs\x64\*.dll bin  -recurse

Write-Host "*** Copy welle-io binary files ***" -ForegroundColor Red
Copy-Item $welleExePath\welle-io.exe bin

# Deploy QT and related plugins
Write-Host "*** Deploy QT and related plugins ***" -ForegroundColor Red
& windeployqt bin\welle-io.exe --qmldir ..\src\welle-gui\QML\ --no-translations

# For some reason windeployqt deploys the wrong DLL on AppVeyor
Copy-Item $QTPath\libgcc_s_seh-1.dll bin
Copy-Item $QTPath\libwinpthread-1.dll bin
Copy-Item $QTPath\libstdc++-6.dll bin

# Run inno setup.exe
$Filename = $Date + "_" + $gitHash + "_Windows_welle-io-setup_x64"
& "ISCC" "/F$Filename" "installer.iss"

# Store file name to a environment variable
$env:welle_io_filename = $Filename
