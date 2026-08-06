[Setup]
AppName=Soundboard
AppVersion=1.0.1
DefaultDirName={autopf}\Soundboard
DefaultGroupName=Soundboard
UninstallDisplayIcon={app}\Soundboard.exe
Compression=lzma2
SolidCompression=yes
OutputDir=Release
OutputBaseFilename=Soundboard_Setup
PrivilegesRequired=admin

[Files]
Source: "Release\Soundboard.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Soundboard"; Filename: "{app}\Soundboard.exe"
Name: "{autodesktop}\Soundboard"; Filename: "{app}\Soundboard.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop icon"; GroupDescription: "Additional icons:"

[Run]
Filename: "{app}\Soundboard.exe"; Description: "Launch Soundboard"; Flags: nowait postinstall skipifsilent
