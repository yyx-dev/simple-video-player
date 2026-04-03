#define MyAppName "简易播放器"
#define MyAppVersion "1.0"
#define MyAppPublisher "杨宇曦"
#define MyDefaultDirName "SimpleVideoPlayer"
#define MyAppExeName "SimpleVideoPlayer"

[Setup]
AppId={{53FE5EDB-8979-46AA-B858-97560253F82B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyDefaultDirName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=public
OutputBaseFilename=SimpleVideoPlayer_Setup
SetupIconFile=app.ico
Compression=lzma
SolidCompression=yes
ChangesAssociations=yes
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "chinese"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "fileassoc"; Description: "注册为默认视频播放器"; GroupDescription: "文件关联"; Flags: unchecked

[Files]
Source: "deploy\simple-video-player.exe"; DestDir: "{app}"; DestName: "SimpleVideoPlayer.exe"; Flags: ignoreversion
Source: "deploy\*.*"; Excludes: "simple-video-player.exe"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "app.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
Root: HKCR; Subkey: ".mp4"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: ".mkv"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: ".avi"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: ".mov"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: ".wmv"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCR; Subkey: ".flv"; ValueType: string; ValueName: ""; ValueData: "SimpleVideoPlayer"; Flags: uninsdeletevalue; Tasks: fileassoc

Root: HKCR; Subkey: "SimpleVideoPlayer"; ValueType: string; ValueName: ""; ValueData: "简易播放器"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKCR; Subkey: "SimpleVideoPlayer\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\app.ico"; Tasks: fileassoc
Root: HKCR; Subkey: "SimpleVideoPlayer\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: fileassoc

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent