# EXE Architecture

> MCO's multi-component executable structure.
> **Status:** 🔶 Partial — NPS subsystem mapped from EXE strings.

---

## Components

| File | Size | Role |
|------|------|------|
| `MCity.exe` | ~4 MB | Main game (release) |
| `MCity_d.exe` | ~6 MB | Main game debug symbols |
| `mco.exe` | 557 KB | NPS core library |
| `authlogin.dll` | 65 KB | Authentication handler |
| `mrbupd.dll` | 110 KB | Registration/boot update |
| `NPSAnlyz.dll` | 41 KB | Protocol analysis (dev only) |
| `NPSPush.dll` | — | Real-time server communication |
| `PBA.exe` | 32 KB | Patch Boot Agent |

---

## Architecture

```
MCity.exe / MCity_d.exe
    ↓ loads
mco.exe (NPS Core)
    ↓ calls
authlogin.dll → EA auth servers
mrbupd.dll → registration
NPSPush.dll → game server communication
```

### DirectX Renderers

```
dx7z.dll — DirectX 7 renderer (NOT used by MCO)
dx8z.dll — DirectX 8 renderer (used by MCO)
voodoo2z.dll — 3dfx Voodoo renderer (fallback)
```

Two versions of `dx8z.dll` shipped: release (Sep 05, 2001) and patched (Nov 07, 2001).

---

## Debug Symbols

Final retail includes `MCity_d.exe` (debug build) with PDB path:

```
C:\mcity\vc_mcity___Win32_Final0\MCity.pdb
```

Build: Visual C++ 6.0, October 8, 2001.

---

## Key Imports

```
KERNEL32.dll   — Windows core
USER32.dll     — UI
WININET.dll    — HTTP (patch, auth)
WSOCK32.dll    — TCP/IP networking
DDRAW.dll      — DirectX 7
d3d8.dll       — DirectX 8
DSOUND.dll     — Audio
DINPUT.dll     — Controller input
DINPUT8.dll   — DirectInput 8
```

---

## Key NPS Exports (from mco.exe)

| Export | Purpose |
|---------|---------|
| `NPSLogin` | User authentication |
| `NPSPush` | Real-time server push |
| `NPSPatch` | Patch/check for updates |
| `NPSGetPersonaMaps` | Get character list |
| `NPSCreateGamePersona` | Create character |
| `NPSGetMail` / `NPSSendMail` | In-game messaging |
| `NPSGetBuddyList` | Friends list |
| `NPSGetGameServersList` | Server list |
| `NPSStartGameServer` | Host game |

---

## Key EXE Strings

```
CC:\mcity\Game\PSim*.c        — Physics (10 files)
CC:\mcity\Game\cars.c         — Car handling
CC:\mcity\Game\carload.c      — Car loading
CC:\mcity\Game\Track.c        — Track system
CC:\mcity\Frontend\db*.c     — Database systems
CC:\nps\Common\NPSLib\Src\     — Network library
```
