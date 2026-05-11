# Motor City Online — My Abandonware Preservation Guide

**Source:** My Abandonware  
**URL:** https://www.myabandonware.com/game/motor-city-online-r4o  
**Archived:** 2026-05-11

## Overview

My Abandonware hosts the full game ISO plus community-made patches and mods for running MCO on modern Windows systems. This is one of the most complete preservation resources available.

## Files Available

| File | Size | Description |
|------|------|-------------|
| Game ISO | — | Full retail Windows version |
| Offline Launcher | 141 MB | Community patch enabling single-player vs. AI |
| '99 Dodge Viper mod | 1 MB | Restores a car cut from the final game |

## Installation Guide (Modern Windows)

### Prerequisites
- Enable **DirectPlay** on Windows 8/8.1/10/11 (Control Panel → Programs → Turn Windows features on/off → Legacy Components → DirectPlay)
- Install DirectX from the disc (Windows 7 or older)
- Recommended: Microsoft Visual C++ Redistributable, .NET Framework, K-Lite Codec Pack

### Steps
1. Mount the disc image (WinCDEmu, UltraISO, Daemon Tools, etc.)
2. Run `Setup.exe` from the disc (use compatibility mode with Windows 98/2000/XP if needed)
3. Select **Full** installation. Install to a non-system folder like `C:/Games/Motor City Online`
4. Extract Offline Launcher files into the game directory, replace original files
5. Edit `Motor City Online/3dsetup/3DSetup.ini`:
   - Uncheck "Read only" in Properties
   - Change `Hide_Resolutions=1` to `Hide_Resolutions=0`
6. Run `3DSetup.exe`, select resolution (800x600 recommended for first launch)
7. Run `MCity_Launcher.exe` (not `mcity.exe` or desktop shortcut)
8. Set Mode to **16** for AI races, press Go!

### Troubleshooting
- **Game crashes on first launch:** Config files are being created. Run again.
- **Installation won't start:** Run `Setup.exe` in Windows 98/2000/XP compatibility mode.
- **Framerate too high / cars move too fast:** Enable VSync in GPU control panel, or limit to 30 FPS.
- **Windowed mode:** Edit `SaveData/options.ini`, change `windowed=0` to `windowed=1`.
- **Widescreen support:** Use [dgVoodoo2](http://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/) — copy DLLs from `MS/x86`, `dgVoodooCpl.exe`, and `dgVoodoo.conf` to game folder.

## Community Projects Mentioned

- **MCO Re-Fired** — Fan attempt to revive multiplayer. Also includes Car Builder for customising cars. Forum: http://www.mcorefired.com/Forum/index.php
- **Discord server** — Community Discord for MCO Re-Fired

## External Links

- [MobyGames](https://www.mobygames.com/game/5465/motor-city-online/)
- [Wikipedia](https://en.wikipedia.org/wiki/Motor_City_Online)
- [PCGamingWiki](https://www.pcgamingwiki.com/wiki/Motor_City_Online)
- Gameplay video by Akor: https://youtu.be/c1ESOuoxt8U
- Gameplay video by LGR: https://youtu.be/YxyzVo6fR9s
