; 小熊音乐播放器 — Inno Setup 安装脚本
; 编译：ISCC.exe packaging\inno\xiaoxiong_music_player.iss

#define MyAppName "小熊音乐播放器"
#define MyAppNameEn "Xiaoxiong Music Player"
#define MyAppVersion "1.0.1"
#define MyAppPublisher "Xiaoxiong"
#define MyAppExeName "xxMusic.exe"
#define MyAppUserModelId "Xiaoxiong.XiaoxiongMusicPlayer"
#define ReleaseDir "..\..\build\Release"
#define RedistFile "..\redist\vc_redist.x64.exe"

#ifexist "..\redist\vc_redist.x64.exe"
  #define VCRedistBundled
#endif

#ifexist "..\..\build\Release\xxMusic.exe"
#else
  #error "Release build not found. Run: cmake --build build --config Release --target xxMusic --parallel"
#endif

#ifexist "..\..\build\Release\tag.dll"
#else
  #error "TagLib tag.dll not found in build\Release. Rebuild Release after enabling shared TagLib (CMake POST_BUILD should copy DLLs)."
#endif

#ifexist "..\..\build\Release\tag_c.dll"
#else
  #error "TagLib tag_c.dll not found in build\Release. Rebuild Release after enabling shared TagLib (CMake POST_BUILD should copy DLLs)."
#endif

[Setup]
AppId={{8F3C2A1B-4D5E-6F70-8192-A3B4C5D6E7F8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\{#MyAppNameEn}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
OutputDir=output
OutputBaseFilename={#MyAppName}_{#MyAppVersion}_Setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
CloseApplications=force
ChangesAssociations=no

[Languages]
Name: "chinesesimplified"; MessagesFile: "languages\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加图标:"; Flags: unchecked

[Files]
; Release 构建产物（排除链接器中间文件）
Source: "{#ReleaseDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.exp,*.lib,*.pdb"
; TagLib 动态库（元数据 / 内嵌封面；须与 xxMusic.exe 同目录）
Source: "{#ReleaseDir}\tag.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ReleaseDir}\tag_c.dll"; DestDir: "{app}"; Flags: ignoreversion
#ifdef VCRedistBundled
Source: "{#RedistFile}"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: NeedsVCRedist
#endif

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; AppUserModelID: "{#MyAppUserModelId}"
Name: "{group}\卸载 {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; AppUserModelID: "{#MyAppUserModelId}"; Tasks: desktopicon

[Run]
#ifdef VCRedistBundled
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "正在安装 Visual C++ 运行库..."; Check: NeedsVCRedist; Flags: waituntilterminated
#endif
Filename: "{app}\{#MyAppExeName}"; Description: "启动 {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsVCRedist: Boolean;
begin
  Result := not (FileExists(ExpandConstant('{sys}\vcruntime140.dll'))
    and FileExists(ExpandConstant('{sys}\vcruntime140_1.dll')));
end;

function InitializeSetup: Boolean;
begin
  if NeedsVCRedist then
  begin
#ifndef VCRedistBundled
    if MsgBox('本机未检测到 Visual C++ 2015–2022 运行库 (x64)。' + #13#10 + #13#10 +
      '若目标用户电脑也可能缺失，请将 vc_redist.x64.exe 放入 packaging\redist\ 后重新打包。' + #13#10 + #13#10 +
      '是否仍继续安装？',
      mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
      Exit;
    end;
#endif
  end;

  Result := True;
end;

var
  UninstallDeleteUserData: Boolean;

function InitializeUninstall: Boolean;
begin
  UninstallDeleteUserData := MsgBox(
    '是否同时删除个人设置、歌单、歌词缓存与封面缓存？' + #13#10 + #13#10 +
    '数据目录：' + ExpandConstant('{userappdata}\Xiaoxiong\XiaoxiongMusicPlayer'),
    mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES;
  Result := True;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  UserDataDir: String;
begin
  if (CurUninstallStep = usPostUninstall) and UninstallDeleteUserData then
  begin
    UserDataDir := ExpandConstant('{userappdata}\Xiaoxiong\XiaoxiongMusicPlayer');
    if DirExists(UserDataDir) then
      DelTree(UserDataDir, True, True, True);
  end;
end;
