; -- 64Bit.iss --
; Demonstrates installation of a program built for the x64 (a.k.a. AMD64)
; architecture.
; To successfully run this installation and the program it installs,
; you must have a "x64" edition of Windows.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

; #define GitHash "unkonwn_git_hash" ; uncomment for debugging

[Setup]
AppName=welle.io
;AppVersion=2.7-unstable-{#GitHash}
AppVersion=2.7-{#GitHash}
WizardStyle=modern
DefaultDirName={autopf}\welle.io
DefaultGroupName=welle.io
UninstallDisplayIcon={app}\welle-io.exe
Compression=lzma2
SolidCompression=yes
OutputDir=.
; "ArchitecturesAllowed=x64compatible" specifies that Setup cannot run
; on anything but x64 and Windows 11 on Arm.
ArchitecturesAllowed=x64compatible
; "ArchitecturesInstallIn64BitMode=x64compatible" requests that the
; install be done in "64-bit mode" on x64 or Windows 11 on Arm,
; meaning it should use the native 64-bit Program Files directory and
; the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=welle-io_install_x64
LicenseFile=COPYING
SetupIconFile=src\welle-gui\icons\icon.ico

[Files]
Source: "installer/*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{group}\welle.io"; Filename: "{app}\welle-io.exe"

[Run]
Filename: {app}\vcredist_x64_2010.exe; Parameters: "/quiet /norestart"; StatusMsg: "Installing VC++ 2010 Redistributables..."
Filename: "{app}\welle-io.exe"; Description: "Start welle.io after finishing installation."; Flags: nowait postinstall skipifsilent
