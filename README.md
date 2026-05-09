# Motor City Online — Reverse Engineering Wiki

> Reverse-engineered documentation for EA Seattle's 2001 online racing MMO.

## Extracted Geometry

Converted car models and tracks from the offline version (2012 community release). All geometry extracted using the tools in this repo. OBJ format with UV coordinates.

### Car Models (`extracted/car_models/`)

13 car geometry files from the offline version VIV archives. Textures not included (see [FSH format](file-formats/FSH.md) for texture format status).

| File | Vertices | Triangles | Notes |
|------|----------|-----------|-------|
| `53chevy.obj` | 1193 | 1431 | 1953 Chevrolet |
| `55cameo.obj` | 1201 | 2480 | 1955 Chevrolet Cameo |
| `56ftruck.obj` | 1091 | 2061 | 1956 Chevrolet Pickup |
| `59impala.obj` | 1326 | 1609 | 1959 Chevrolet Impala |
| `8ball.obj` | 1120 | 1716 | Custom / novelty |
| `96supra.obj` | 1529 | 3652 | 1996 Toyota Supra |
| `97eclps.obj` | 33 | 60 | Partial geometry (spoiler/side mirror) |
| `hc59-hc64.obj` | 15 | 12 | Template variants |

### Tracks (`extracted/tracks/`)

16 track geometry files (FRD format). Includes Derby, Foundry, GasTown, Hazard, and all final release tracks.

All tracks extracted from offline version FRD files. Each OBJ has ~1240 sample points.

## Quick Links

- [File Format Reference](file-formats/README.md) — All binary formats documented
- [Track Format](file-formats/FRD.md) — Track geometry (FRD) and track textures (FSH)
- [Car Model Format](file-formats/FCE.md) — Car geometry (FCE), textures (FSH), VIV archives
- [BIG Archive Format](file-formats/BIG.md) — EA's archive format
- [Audio Format](file-formats/BNK.md) — Sound banks and EA XA ADPCM audio
- [Database](file-formats/DATABASE.md) — Online.mdb Access database structure
- [Tools](tools/README.md) — Extraction and conversion utilities
- [Research: Beta 1](research/BETA1.md) — June 27, 2001 beta analysis
- [Research: Oct 09 Prototype](research/OCT09.md) — October 9, 2001 WebBeta 2 analysis
- [Extracted Geometry](extracted/) — Converted OBJ files

## What Is This?

Motor City Online (MCO) was an online racing MMO released by EA Seattle in October 2001, shut down in August 2003. It was built on a custom engine derived from the Need for Speed series, using EA's proprietary binary formats for 3D models, track geometry, textures, and audio.

This wiki documents the file formats, tools, and findings from reverse-engineering the game's binary data files. All documentation is derived from analyzing the actual game files — no source code or proprietary documentation was used.

## Game Overview

| Property | Value |
|---|---|
| Developer | EA Seattle |
| Publisher | Electronic Arts |
| Release | October 31, 2001 |
| Platform | PC (Windows) |
| Engine | Modified Need for Speed engine |
| Server shutdown | August 2003 |
| Offline patch | 2012 community release (workaround for server closure) |

## Build Comparison

| | Beta 1 (Jun 27) | Oct 09 (Oct 9) | Final (Oct 31) | Offline (2012) |
|---|---|---|---|---|
| VIV count | 304 | 628 | ~50 (in cars.big) | 13 |
| BIG archives | No | No | Yes | No |
| Installed size | 1.3 GB | 616 MB | BIG-packed | ~1 GB |
| Executable | `MCity.exe` | `mcity.exe` (lowercase!) | `MCity.exe` | `MCity_d.exe` |
| Debug symbols | No | No | No | Yes — 2012 RE build |
| Tracks | 17 | 16 | 15 | 16 |

**Notable:** Only the Oct 09 build has a lowercase executable name (`mcity.exe`). The `MCity_d.exe` in the offline version is a 2012 community rebuild, not an original EA debug build.

## Available Builds

### Beta 1 — June 27, 2001
First public beta. 280 car VIVs as loose files, 17 tracks including Derby and Dirtoval (both removed from final).

Download: MediaFire (user-shared archive)

### Oct 09 Prototype — October 9, 2001
Second and final beta (WebBeta 2). 628 VIVs, 16 tracks. Installed size actually *smaller* than Beta 1 despite more VIV files. Tracks: Derby, Gravel, Obstacle removed; Hazard added; ParkA renamed to Parka.

Download: Hidden Palace (files.hiddenpalace.org CDN returned 404 at time of research)

### Final Release — October 31, 2001
Standard BIG-packed distribution. 50 cars in `cars.big`, 15 tracks. Includes `MCity_d.exe` — actually a 2012 offline patch rebuild, not original EA debug.

### Offline Version — 2012
Community release for playing without servers. Uses BIGF-wrapped VIV format with GIMX (GameCube) texture ports. 13 car models, 16 tracks with completely different names (Derby, Foundry, GasTown, etc.).

## Directory Structure (Final Release)

```
Motor City Online/
├── MCity.exe          # Main game executable
├── MCity_d.exe        # 2012 offline patch rebuild (not original EA debug)
├── Data/
│   ├── cars.big       # Car models + textures
│   ├── tracks.big     # Track geometry + textures
│   ├── audio.big      # Music + SFX banks
│   ├── patch.big      # Engine patches
│   ├── DB.big         # Database (Online.mdb)
│   ├── GUI.big        # UI textures
│   ├── feArt.big      # Front-end art
│   └── (other BIGs)   # Additional archives
├── lang/              # Localization
└── SaveData/          # Player saves
```

## Directory Structure (Offline Version)

```
Offline/
├── Data/
│   ├── Models/        # 13 BIGF-wrapped VIV car files
│   ├── Skins/         # GIMX-format texture files
│   ├── Tracks/        # 16 tracks (Derby, Foundry, GasTown, etc.)
│   ├── GUI/           # UI textures
│   └── DB/            # Database
├── MCity_d.exe        # Offline patch executable
└── MCity_Launcher.exe
```

## Key File Types

| Extension | Type | Description |
|---|---|---|
| `.fce` | 3D Model | Car geometry (Full Car Editor format) |
| `.fsh` | Texture | Image/texture (EA FSH / SHPI format) |
| `.frd` | Track | Track road surface geometry |
| `.big` | Archive | EA archive (BIGF format) |
| `.viv` | Archive | Per-car archive (two variants — see VIV.md) |
| `.bnk` | Audio | Sound bank (EA XA ADPCM audio) |
| `.mdb` | Database | Microsoft Access database (car stats) |
| `.trk` | Track | Track routing/graph data |
| `.trn` | Track | Track barrier/net geometry |
| `.blf` | Car | Bill of materials (vertex segmentation) |
| `.lod` | Car | Level-of-detail distance thresholds |

## Supported Tools

- **fce2obj.py** — FCE car model → Wavefront OBJ converter
- **frd2obj.py** — FRD track geometry → OBJ converter
- **big_extract.py** — BIG archive extractor
- **viv_extract.py** — VIV extractor (handles both standard and BIGF-wrapped variants)
- **iso_extract.py** — ISO 9660 extractor (no root/loopback required)
- **FSH → PNG** — Texture conversion (spec-based, not yet fully working)
- **BNK → WAV** — Audio extraction (EA XA ADPCM decoder)

## Common Tasks

### Extract ISO contents (no root needed)
```bash
python3 iso_extract.py Motor\ City\ Online.iso extracted/
```

### Extract BIG archives
```bash
python3 big_extract.py cars.big extracted/
```

### Extract a VIV car archive
```bash
python3 viv_extract.py 53chevy.viv 53chevy/
```

### Convert a car model to OBJ
```bash
python3 viv_extract.py 53chevy.viv temp/
python3 fce2obj.py temp/part.fce 53chevy_body.obj
```

### Convert a track to OBJ
```bash
python3 frd2obj.py Data/Tracks/Boothill/Tr.frd boothill.obj
```

### Export database tables (Linux)
```bash
mdb-tables Online.mdb                  # List tables
mdb-export Online.mdb CarModels        # Export to CSV
mdb-export Online.mdb CarPhysics       # Export physics table
```

## Open Questions

- FST format purpose and structure (largest remaining unknown)
- SHPI/GIMX texture decoder (for converting textures to PNG)
- Network protocol for online multiplayer
- Script/event system (if any .scm files exist?)
- Full BLF chunk type enumeration
- Track AI racing line / rubber-banding formula
- `Dirtoval` extra files (`track.fce`, `spdF0.bin`, `spdR0.bin`) purpose
