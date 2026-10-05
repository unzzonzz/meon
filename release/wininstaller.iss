; MEON Windows 설치 프로그램 (Inno Setup 6)
; distwin.sh 가 MEON\ 폴더에 파일을 모은 뒤 iscc /DSBVERSION=x.y.z wininstaller.iss 로 만든다.
; 64비트 전용: 독립 앱 + VST3 플러그인.

#ifndef SBVERSION
  #define SBVERSION "0.0.0"
#endif

[Setup]
AppId={{465082CB-FEF8-4130-80D0-8B6471AAAA30}
AppName=MEON
AppVersion={#SBVERSION}
AppPublisher=Meon
MinVersion=6.1
WizardStyle=modern
DefaultDirName={autopf}\MEON
DefaultGroupName=MEON
UninstallDisplayIcon={app}\MEON.exe
SetupIconFile=..\images\icons\MEON-installer.ico
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=MEON-{#SBVERSION}-Installer
SetupLogging=yes
DisableWelcomePage=yes
DisableDirPage=auto
DisableProgramGroupPage=yes
ShowComponentSizes=no
CloseApplications=yes

[Types]
Name: "full"; Description: "전체 설치"
Name: "custom"; Description: "사용자 지정"; Flags: iscustom

[Components]
Name: "app"; Description: "MEON 앱"; Types: full custom; Flags: fixed
Name: "vst3"; Description: "VST3 플러그인 (DAW 에서 사용)"; Types: full custom

[Tasks]
Name: "desktopicon"; Description: "바탕 화면에 바로 가기 만들기"; Components: app

[Files]
Source: "MEON\MEON.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "MEON\README.txt"; DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "MEON\Plugins\VST3\MEON.vst3\*"; DestDir: "{commoncf64}\VST3\MEON.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\MEON"; Filename: "{app}\MEON.exe"
Name: "{autodesktop}\MEON"; Filename: "{app}\MEON.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\MEON.exe"; Description: "MEON 실행"; Flags: nowait postinstall skipifsilent
