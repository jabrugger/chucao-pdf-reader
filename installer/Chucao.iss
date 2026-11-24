; Chucao PDF Reader installer. Build with scripts/package-installer.ps1 so the
; version and the portable folder are generated first.

#ifndef MyVersion
  #define MyVersion "0.0.0"
#endif
#define MyAppName "Chucao PDF Reader"
#define MyAppPublisher "Jorge Andres Brugger"
#define MyAppURL "https://github.com/jabrugger/chucao-pdf-reader"
#define MyAppExeName "Chucao.exe"

[Setup]
AppId={{7C4A3E2B-9D1F-4B6A-8E3C-5F2A1B0D9C47}
AppName={#MyAppName}
AppVersion={#MyVersion}
AppVerName={#MyAppName} {#MyVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
VersionInfoVersion={#MyVersion}
DefaultDirName={localappdata}\Programs\Chucao
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
OutputDir=..\dist
OutputBaseFilename=Chucao-{#MyVersion}-setup
SetupIconFile=..\assets\branding\chucao.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\LICENSE

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "associatepdf"; Description: "Abrir los archivos PDF con Chucao"; GroupDescription: "Integración con Windows:"; Flags: unchecked
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\dist\Chucao-{#MyVersion}-win-x64\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[InstallDelete]
; 0.2.0 shipped without the embedded manifest. Remove any external copy left
; next to the executable so it can never override the one inside the EXE.
Type: files; Name: "{app}\Chucao.exe.manifest"

[UninstallDelete]
Type: files; Name: "{app}\Chucao.exe.manifest"

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Desinstalar Chucao"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Classes\Chucao.PDF"; ValueType: string; ValueName: ""; ValueData: "Documento PDF"; Flags: uninsdeletekey; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\Chucao.PDF\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\Chucao.PDF\Application"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#MyAppName}"; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\Chucao.PDF\Application"; ValueType: string; ValueName: "ApplicationIcon"; ValueData: "{app}\{#MyAppExeName},0"; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\Chucao.PDF\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\.pdf\OpenWithProgids"; ValueType: none; ValueName: "Chucao.PDF"; Flags: uninsdeletevalue; Tasks: associatepdf
Root: HKCU; Subkey: "Software\Classes\.pdf"; ValueType: string; ValueName: ""; ValueData: "Chucao.PDF"; Tasks: associatepdf

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Abrir {#MyAppName}"; Flags: nowait postinstall skipifsilent shellexec

[Code]
const
  SHCNE_ASSOCCHANGED = $08000000;
  SHCNF_IDLIST = $0000;

procedure SHChangeNotify(EventId: Longint; Flags: Longint; Item1: Longint; Item2: Longint);
  external 'SHChangeNotify@shell32.dll stdcall';

procedure NotifyAssociations;
begin
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('associatepdf') then
    NotifyAssociations;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  ProgId: String;
begin
  if CurUninstallStep = usUninstall then
  begin
    // Clear Windows' UserChoice only if it points to Chucao.
    if RegQueryStringValue(HKCU,
         'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pdf\UserChoice',
         'ProgId', ProgId) and (CompareText(ProgId, 'Chucao.PDF') = 0) then
      RegDeleteKeyIncludingSubkeys(HKCU,
        'Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts\.pdf\UserChoice');
    // Only clear the extension default when it still points to Chucao.
    if RegQueryStringValue(HKCU, 'Software\Classes\.pdf', '', ProgId) and
       (CompareText(ProgId, 'Chucao.PDF') = 0) then
      RegDeleteValue(HKCU, 'Software\Classes\.pdf', '');
    RegDeleteValue(HKCU, 'Software\Classes\.pdf\OpenWithProgids', 'Chucao.PDF');
    RegDeleteKeyIncludingSubkeys(HKCU, 'Software\Classes\Chucao.PDF');
    NotifyAssociations;
  end;
end;
