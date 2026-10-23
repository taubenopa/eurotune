[Setup]
AppName=EuroTune
AppVersion=1.0.0
AppPublisher=EuroJunkie
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
DisableReadyPage=no
OutputBaseFilename=EuroTune-Setup-Windows
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallFilesDir={autopf}\EuroJunkie\EuroTune

[Files]
Source: "..\..\build\EuroTune_artefacts\Release\VST3\EuroTune.vst3\*"; DestDir: "{commoncf64}\VST3\EuroTune.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
