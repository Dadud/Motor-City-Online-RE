# Patch System

> How Motor City Online downloaded and applied updates.
> **Status:** 🔶 Partial — pipeline confirmed; offline mode partially understood.

---

## Patch Server

**URL:** `http://downloads.mco.ea.com:80/games/EA_Seattle/MotorCity/UpdateInfo`

Confirmed from live network trace in the Oct09 prototype `mco_log.txt`. [E2]

---

## UpdateInfo Structure

The game queries `UpdateInfo` to find the latest patch package. The structure is not fully decoded, but EXE strings reveal:

- `EnQPatch` — primary patch enqueue function
- `NPSPatch` — NPS patch subsystem
- `PatchUI` — patch progress UI
- `GamePatch` / `PushPatch` — specific patch types
- `Stage` — patch staging directory

---

## Multi-Stage Pipeline

```
UpdateInfo (version check)
    ↓
push.ver (version metadata file)
    ↓
Download patch package(s)
    ↓
Verify checksum
    ↓
Decompress (PKWARE DCL Implode)
    ↓
PBA.exe (Patch Boot Agent) — applies patch
    ↓
Restart game
```

Confirmed from EXE strings + `mco_log.txt` corroboration. [E2]

---

## Offline Patch

The offline patch (`MCity_Update.7z`) is an **overlay**, not a standalone install:

- Drops onto a final retail install
- Does NOT include `engpatch.viv` — retail ENGPATCH.VIV is required
- Adds new tracks, cars, bug fixes
- Contains updated `MCity_Launcher.exe`

The offline patch **does not replace** the need for a retail install.

---

## Key EXE Strings

```
EnQPatch(..., games/EA_Seattle/MotorCity/UpdateInfo)
Patching file:
Warning: EnQPatch()... pkg->maxBandwidth = %d
Error: EnQPatch()... pkg->name is empty
Error: EnQPatch()... pkg->dir is empty
Can't find Module Handle in order to version check patcher
), use PBA to finish patch
Motor City must now restart in order to complete the patching process.
Patching completed, press <Continue>.
Patch server is busy. Please try again later.
```

---

## PBA.exe

**Patch Boot Agent** — a small executable that runs before the main game to apply patches. Referenced in the offline patch flow.

```
%s\nps\PBA.exe
-PID=%u -PKG="%s" -PKG="%s\nps" -ONEXIT="%s"
```

---

## push.ver

Present in both the Oct09 prototype and final retail install. Likely contains:
- Current game version
- Required patch version
- File checksums

Binary format not analyzed.

---

## See Also

- `mco_log.txt` — live patch trace from Oct09 prototype
- `docs/formats/engpatch.md` — engpatch.viv (shipped patch archive)
