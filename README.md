# EuroTune by EuroJunkie

Autotune VST3 (JUCE 8 / C++17) für Underground-Rap-Vocals.

## Features
- YIN-Pitch-Detection + pitch-synchroner Shifter (sauber, kein Granular-Matsch)
- 4 Modes: **Clean**, **Classic**, **Robot**, **Extreme** (digitaler Treppen-Robot)
- Retune Speed, Amount, Tolerance (Flex), Humanize, Detune, Vibrato (Depth/Rate), Gate, Mix, Output
- Key + 14 Skalen (inkl. Phrygian Dominant, Blues, Pentatonik) + individuelle Noten per Klick
- Range (Auto/Low/Mid/High), A4-Referenz, Bypass
- Animierte UI: Bild reagiert auf Pegel, Glitch, RGB-Split, Neon-Grid, Pitch-Graph live

## Build über GitHub
1. Ordner in ein GitHub-Repo pushen (Branch `main`).
2. Tab **Actions** → Workflow *Build EuroTune* → nach ~10 Min unter **Artifacts**:
   - `EuroTune-Windows-Installer` (Setup .exe → installiert nach `C:\Program Files\Common Files\VST3`)
   - `EuroTune-macOS-Installer` (.pkg → `/Library/Audio/Plug-Ins/VST3`)
3. Release: `git tag v1.0.0 && git push --tags` → Installer landen automatisch im GitHub-Release.

## Hinweise
- Unsigniert: Windows SmartScreen / macOS Gatekeeper warnen (Rechtsklick → Öffnen).
- JUCE ist AGPL/kommerziell: für Closed-Source-Verkauf ist eine JUCE-Lizenz nötig.
- Latenz ~17,5 ms (wird der DAW gemeldet).
- Formanten werden nicht separat erhalten (bei kleinen Korrekturen unkritisch).
