# Motor City Online - Patch System Analysis

## Overview

The patch files (engpatch.viv) contain **patched audio files** for Motor City Online. These are not simple archives but **patch/delta files** that update specific audio content in the game.

## engpatch.viv Structure

**File:** `Installed Game/Data/Audio/SFX/engpatch.viv`  
**Size:** 32,346,468 bytes (30.8 MB)  
**Magic:** `BIGF` (BIG File format variant)

### Contents

| Type | Count |
|------|-------|
| BNK (audio banks) | 499 |
| RPM (engine RPM data) | 1 |
| CTB, LTB, BTB, HTB | 4 |

**Total:** 504 audio patch entries

### Filename Convention

Files follow a naming pattern that identifies car/engine combinations:
```
snblelo.bnk    - "sn" car, "blelo" variant
blxlxhi.bnk    - "blx" car, "lxhi" variant  
czzcxid.bnk    - "czz" car, "cxid" variant
engine.rpm     - engine RPM curve data
engine.ctb     - engine control table
```

## Header Structure

```
Offset  Size  Description
------  ----  -----------
0x00    4     Magic: "BIGF"
0x04    4     Version/info (0x6491ed01)
0x08    4     Some count or flags
0x0C    4     Offset to filename table (0x2784)
0x10    4     Data offset (0x2784)
...
```

## Filename Entry Format

Each entry appears to be 16 bytes:
- Filename (null-terminated, up to 12 chars + null = 13 bytes)
- 3 bytes metadata

The filename table starts at offset 0x18 and entries are spaced 16 bytes apart.

## 8-Byte Metadata Format

After the filename list (at 0x2700 area), entries use 8-byte metadata:
- Bytes 0-3: Some identifier or index (big-endian?)
- Bytes 4-7: Size or offset indicator

Example for `engine.htb`:
- v1: 0x00000042 (66) - possibly an index
- v2: 0x4E4B6C04 - contains "NKl\0" bytes (BNK header signature)

## BNK File Locations

BNK headers (BNKl) are scattered throughout the file:
```
0x2784   - First BNK header
0x16E04  - Second BNK header  
0x21FAC  - Third BNK header
... (499 total)
```

## Purpose

These patch files were used by EA to:
1. **Update audio** without releasing full game patches
2. **Fix engine sounds** for specific car configurations
3. **Add new audio content** for the online multiplayer

The patches were delivered through the NPS update system (PushPatch, SystemPatch, etc.).

## Key Functions (from binary)

From mcacity.exe strings:
```
EnQPatch() - Queue a patch
PushPatch - Push patch to client
SystemPatch - System-level patch
PBAPatch - PBA patch application
GamePatch - Game data patch
Patching error. - Error handling
PatchServerIP - Patch server configuration
```

## Related Files

| File | Purpose |
|------|---------|
| `nps/mco.exe` | Contains patch client code |
| `nps/PBA.exe` | Patch Buddy Application - applies patches |
| `nps/ProxyTool.exe` | Proxy for patch downloads |
| `MCity_Update.7z` | Update archive (47MB) |
| `MCity_Launcher.7z` | Launcher archive (132KB) |

## Patching Flow

1. Game connects to patch server (configured in NPS)
2. `EnQPatch()` queues required patches
3. `PushPatch` downloads patch data
4. `PBA.exe` applies patches to game files
5. Game restarts with updated content

## Status

**Partially documented** - We understand:
- The file is a patch archive containing 504 audio files
- It uses BIGF container format
- BNK files contain audio track data

**Not yet decoded:**
- Exact meaning of 8-byte metadata fields
- How patches are applied (PBA.exe not fully analyzed)
- Whether patches are delta/compressed or full replacements
