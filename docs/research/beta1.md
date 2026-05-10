# Beta 1 (June 27, 2001) — Research Notes

> First public beta build of Motor City Online, released ~4 months before the Oct 31, 2001 final release.

## Overview

| Property | Value |
|---|---|
| Build date | June 27, 2001 |
| Build name | Public Beta 1 |
| ISO size | 679 MB |
| Executable | `MCity.exe` (4.1 MB, Jun 27 2001) |
| Debug exe | **None** (only in final release) |
| File storage | **Loose files** (no BIG archives) |

## How to Get

Download from MediaFire (user-shared):
```
https://www.mediafire.com/file/pjueomsyt1nzauc/Motor+City+Online+Beta+1+(June+27th,+2001).zip/file
```

ISO: `Motor City Online Beta 1 (June 27th 2001).iso` (679 MB)  
Extracted files: `Extracted Files/` directory inside the ISO

## Key Finding: No BIG Archives

The most significant structural difference from the final release:

| | **Beta 1** | **Final (Oct 2001)** |
|---|---|---|
| Car models | `Data/models/*.viv` (280 loose files) | `cars.big` |
| Car textures | `Data/skins/*.fsh` (259 loose files) | inside `cars.big` |
| Tracks | `Data/Tracks/<name>/` (loose per-track dirs) | `tracks.big` |
| Text | `Data/Text/text.ENG` (loose) | inside BIG archives |
| Audio | `Data/Audio/*.bnk` (loose) | `audio.big` |
| Database | `Data/DB/Online.mdb` (loose) | inside `DB.big` |

The BIG packing system already existed in the engine code (`FILE_loadbigheader`, `BIG_locateentry`, `CC:\mcity\Game\buildbig.c`) but the beta shipped with everything unpacked. The final release was the one that packed everything into BIG archives for distribution.

## Lost Tracks

Two tracks were present in the beta but **removed from the final release**:

- **Derby** — Derby Oval track (mentioned explicitly in Hidden Palace description)
- **Dirtoval** — Dirt oval track (Stunt Track variant)

### Derby Track Files

```
Data/Tracks/Derby/
├── Tr.frd              # Road surface
├── Trn.frd             # Barriers/nets
├── TrCam.ini           # Camera waypoints
├── audio.ini           # Track audio
├── boom.ini            # Crash/explosion sounds
├── data.csv            # Track data table
├── info.ini            # Track metadata
├── sky.fsh             # Skybox texture
├── tr.col              # Collision data
├── tr.trk              # Track routing
├── track.bnk           # Audio bank
├── track.fsh           # Track texture
├── tr0.fsh             # Segment texture
```

### Dirtoval Extra Files

`Dirtoval` has files not seen in final tracks:

| File | Purpose |
|------|---------|
| `track.fce` | 3D track model |
| `Trmap.bin` | Minimap data |
| `trmap.txt` | Minimap text labels |
| `spdF0.bin` | Speed trap (front) data |
| `spdR0.bin` | Speed trap (rear) data |
| `slides.fsh` | Skid/slide mark textures |

These suggest `Dirtoval` may have been a development test track or used for physics validation.

## Engine References in Beta Executable

The beta `MCity.exe` already references the engine patch system:

```
engpatch.viv    # Engine audio/texture patch
bam.viv         # Body artist model
hornz.viv       # Horn sounds
sancspch.viv    # Speech/sound
oppeng.viv      # Opponent engine

engine.rpm      # RPM curves
engine.htb      # Horsepower/torque table (high)
engine.btb      # Torque table (bottom)
engine.ctb      # Torque table (center?)
engine.ltb      # Torque table (low)
```

This confirms `engpatch.viv` was always part of the architecture — in the final release these are packaged inside `patch.big`.

## Beta-Specific Files

### Tracks: 17 vs 15

Beta has 17 tracks; final has 15. The two missing are Derby and Dirtoval.

### Audio

24 `.asf` music files are loose (not in audio.big):
- `agroove.asf`, `backseat.asf`, `badrat.asf`, `blown57.asf`, etc.
- Final release packs these into `audio.big`

7 `.bnk` SFX banks are also loose:
- `animsfx.bnk`, `carlot.bnk`, `damage.bnk`, `diner.bnk`, etc.

## Executable Differences

| | Beta (Jun 27 2001) | Final |
|---|---|---|
| `MCity.exe` size | 4.1 MB | ~4 MB |
| `MCity_d.exe` | **Not present** | ~6 MB (debug symbols) |
| Debug strings | Embedded in release exe | Stripped to separate debug exe |
| Build path | `CC:\mcity\Game\aidebug.c`, `buildbig.c` | Different |

The **debug executable (`MCity_d.exe`)** was first introduced in the final release, not the beta. Beta only has the release build with debug strings embedded.

## Comparison: FRD Header

FRD format is consistent between beta and final — same magic bytes (`0x19EB 0xEEFE`).

```
Beta Derby:  first bytes = 19 eb ee fe 00 d2 6b 32
Final Boothill: first bytes = 19 eb ee fe 00 97 13 2a
```

Header structure appears identical.

## What This Beta Tells Us

1. **BIG packing was a distribution choice, not a format change** — the engine supported both loose files and BIG from the beginning
2. **Two tracks were cut** — Derby and Dirtoval; neither appears in the final
3. **Engine patch system predates the final** — `engpatch.viv` was always intended, just packaged differently
4. **Final's debug exe is new** — beta has no separate debug build; EA introduced `MCity_d.exe` only for the final
5. **All car models were ready by June 2001** — 280 car VIV files were complete before beta release

## Open Questions

- Why were Derby and Dirtoval cut? (Content licensing? Quality issues? Consolidation?)
- What is the `Dirtoval` extra files' purpose? (Testing formats? Debug tools?)
- Was `Dirtoval`'s `track.fce` used for anything?
- What changed in `MCity_d.exe` vs `MCity.exe` besides symbols?

## File Listing

Full extracted file tree: [Extracted Files](Extracted-Files.md)
