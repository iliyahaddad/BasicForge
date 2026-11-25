; Inno Setup script for BasicForge IDE installer
; Save as: installer.iss

#define MyAppName "BasicForge IDE"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "BasicForge Team"
#define MyAppURL "https://github.com/basicforge/basicforge"
#define MyAppExeName "basicforge-ide.exe"

[Setup]
AppId={{BASICFORGE-IDE-001-0001-0000-000000000000}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
LicenseFile=LICENSE
OutputDir=release
OutputBaseFilename=BasicForgeIDE_Setup_{#MyAppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "quicklaunchicon"; Description: "{cm:CreateQuickLaunchIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked; OnlyBelowVersion: 0,6.1
Name: "associate_bf"; Description: "Associate .bf files with BasicForge IDE"; GroupDescription: "File Associations"; Flags: unchecked

[Files]
Source: "ide\target\release\basicforge-ide.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "compiler\build\Release\bfc.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "runtime\*"; DestDir: "{app}\runtime"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "libraries\*"; DestDir: "{app}\libraries"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "sdks\*"; DestDir: "{app}\sdks"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "examples\*"; DestDir: "{app}\examples"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: quicklaunchicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
; File association for .bf files
Root: HKCU; Subkey: "Software\Classes\.bf"; ValueType: string; ValueData: "BasicForge.Source"; Flags: uninsdeletekey; Tasks: associate_bf
Root: HKCU; Subkey: "Software\Classes\BasicForge.Source"; ValueType: string; ValueData: "BasicForge Source File"; Flags: uninsdeletekey; Tasks: associate_bf
Root: HKCU; Subkey: "Software\Classes\BasicForge.Source\DefaultIcon"; ValueType: string; ValueData: "{app}\{#MyAppExeName},0"; Flags: uninsdeletekey; Tasks: associate_bf
Root: HKCU; Subkey: "Software\Classes\BasicForge.Source\shell\open\command"; ValueType: string; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Flags: uninsdeletekey; Tasks: associate_bf

[UninstallDelete]
Type: filesandordirs; Name: "{app}"

[Code]
function InitializeSetup(): Boolean;
begin
  // Check for required runtime dependencies
  Result := True;
end;