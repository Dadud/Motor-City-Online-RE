# Motor City Online - Executable Architecture

## Overview

MCO uses a **multi-component architecture** where the main game executable (`mcity.exe`) delegates all online services — authentication, patching, matchmaking, and chat — to the **NPS (Network Platform Services)** subsystem.

```
MCITY.EXE (Main Game)
    │
    ├── NPS Subsystem
    │   ├── mco.exe          (NPS Core Library)
    │   ├── authlogin.dll    (EA Authentication)
    │   ├── mrbupd.dll       (Registration/Boot Update)
    │   ├── NPSAnlyz.dll     (Protocol Analyzer)
    │   └── PBA.exe         (Patch Boot Agent)
    │
    ├── Renderer
    │   ├── dx7z.dll (DirectX 7)
    │   └── dx8z.dll (DirectX 8)
    │
    └── Storage (storage.dll)
```

## Component Breakdown

### mcacity.exe (5 MB)

The **main game executable**. Contains all gameplay, rendering, and UI logic.

**Key Functions (discovered from strings):**
- `TCPMgr::Connect`, `TCPMgr::Disconnect` — TCP networking
- `cNPS_GameRoomServer::sendDisconnect` — Room management
- `cNPS_Room::sendChat` — In-game chat
- `NPS_*` — All NPS API calls

**DLL Dependencies:**
| DLL | Purpose |
|-----|---------|
| `KERNEL32.dll` | Core Windows API |
| `USER32.dll` | Windows UI |
| `WININET.dll` | Internet access |
| `WSOCK32.dll` | TCP/IP sockets |
| `DDRAW.dll` | DirectX 7 graphics |
| `d3d8.dll` | DirectX 8 graphics |
| `DSOUND.dll` | DirectX audio |
| `DINPUT.dll` | Controller input |

---

### mco.exe — NPS Core (557 KB)

The **NPS library**. Handles all communication with EA's servers.

**Key Exports:**
| Function | Purpose |
|----------|---------|
| `NPSLogin` | Authenticate user |
| `NPSPatch` | Check for game patches |
| `NPSPush` | Real-time server push |
| `NPSGetPersonaMaps` | Get character list |
| `NPSCreateGamePersona` | Create new character |
| `NPSGetMail` / `NPSSendMail` | In-game messaging |
| `NPSGetBuddyList` | Friends list |
| `NPSGetGameServersList` | Get server list |
| `NPSStartGameServer` | Host multiplayer game |

---

### authlogin.dll (65 KB)

Handles **EA AAI (Authentication & Authorization Interface)** login.

**Key Config (from strings):**
- `Auth_NPS_AAI_Hostname` — EA authentication server (e.g., `ea.com`)
- `Auth_NPS_AAI_Port` — Authentication port
- `AuthUse_NPS_AAI` — Use AAI mode
- `AuthLoginServer` — Login server address
- `authenticated` / `Authentication` — State indicators

---

### mrbupd.dll (110 KB)

**MCO Registration/Boot Update** — Handles initial registration and boot-time verification.

---

### NPSAnlyz.dll (41 KB)

**NPS Protocol Analyzer** — Development/debugging tool for analyzing NPS protocol traffic.

---

### PBA.exe (32 KB)

**Patch Boot Agent** — Runs before the main game to check and apply patches.

---

## Launch Sequence

Based on code analysis:

```
1. MCITY.EXE starts
   ↓
2. Loads mco.exe (NPS Core)
   ↓
3. PBA.exe runs patch check
   ↓
4. NPS connects to Auth_NPS_AAI server
   ↓
5. User authenticates via authlogin.dll
   ↓
6. NPS validates game version
   ↓
7. All OK → MCITY launches
   ↓
   ERROR → Display error, exit
```

## Error Codes

MCO uses `kTxtNPS_*` prefixed error codes throughout:

| Error Code | Meaning |
|------------|---------|
| `kTxtNPS_AUTH_ERROR` | Authentication failed |
| `kTxtNPS_NOT_AUTHORIZED` | User not authorized |
| `kTxtNPS_CONNECTION_TIMEOUT` | Server connection timed out |
| `kTxtNPS_DATABASE_ERROR` | Server database error |
| `kTxtNPS_VERSION_INCOMPATABLE_MAJOR` | Game version mismatch |
| `kTxtNPS_LOBBY_NOT_CONNECTED` | Not connected to lobby |
| `kTxtNPS_ROOM_NOT_FOUND` | Game room not found |
| `kTxtNPS_TIMEOUT` | Operation timed out |

## Internal Protocol: CASTANET

NPS uses an internal protocol called **CASTANET** for server communication:

```
CASTANET_ERROR_CONNECT_FAILED
CASTANET_ERROR_HTTP_UNAUTHORIZED  
CASTANET_ERROR_INVALID_REPLY
CASTANET_ERROR_PROTOCOL
CASTANET_ERROR_MAGIC
CASTANET_ERROR_UNKNOWN_HOST
```

## Patch System

Patch files used by MCO:
- `engpatch.viv` — English patch archive
- `sengpatch.viv` — Server-side patch?

Error strings related to patching:
- `PatchRestart` — Patch requires game restart
- `DealerPatch` — Internal patch mechanism name
- `kTxtPatchingFiles` — "Patching files..."
- `ExitToPatchDance` — Exit to apply patches

## Build Information

From debug symbols found in executable:
- **PDB:** `MCity.pdb`
- **Build tag:** `vc_mcity___Win32_Final0`
- **Compiler:** Visual C++

## See Also

- [Network Protocol](../research/NETWORK_PROTOCOL.md) — MCOTS/NPS protocol documentation
- [File Formats](../file-formats/README.md) — Database, archive, and texture formats
