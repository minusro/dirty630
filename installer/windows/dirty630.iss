; Inno Setup script: installs the Windows VST3 bundle into the standard
; system-wide VST3 folder (C:\Program Files\Common Files\VST3).
;
; Build:  iscc /DVersion=<m.n.p> /DVst3Dir=<dir containing dirty630.vst3> dirty630.iss

#ifndef Version
  #define Version "0.0"
#endif
#ifndef Vst3Dir
  #define Vst3Dir "..\..\build\dirty630_artefacts\Release\VST3"
#endif

[Setup]
AppName=dirty630
AppVersion={#Version}
AppPublisher=minus
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64compatible
ArchitecturesAllowed=x64compatible
Compression=lzma2
SolidCompression=yes
OutputBaseFilename=dirty630-{#Version}-windows
LicenseFile=..\..\LICENSE

[Files]
Source: "{#Vst3Dir}\dirty630.vst3\*"; DestDir: "{commoncf64}\VST3\dirty630.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
