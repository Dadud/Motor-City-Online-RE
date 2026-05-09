# Motor City Online — Reverse Engineering Wiki

> Reverse-engineered documentation for EA Seattle's 2001 online racing MMO.

## Quick Links

- [File Format Reference](file-formats/README.md) — All binary formats documented
- [Track Format](file-formats/FRD.md) — Track geometry (FRD) and track textures (FSH)
- [Car Model Format](file-formats/FCE.md) — Car geometry (FCE), textures (FSH), VIV archives
- [BIG Archive Format](file-formats/BIG.md) — EA's archive format
- [Audio Format](file-formats/BNK.md) — Sound banks and EA XA ADPCM audio
- [Database](file-formats/DATABASE.md) — Online.mdb Access database structure
- [Tools](tools/README.md) — Extraction and conversion utilities
- [Research: Beta 1](research/BETA1.md) — June 27, 2001 beta vs final release
- [Research: Oct 09 Prototype](research/OCT09.md) — October 9, 2001 WebBeta 2 analysis

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
| Offline patch | Available (workaround for server closure) |

## Directory Structure (Final Release)

```
Motor City Online/
├── MCity.exe          # Main game executable (release)
├── MCity_d.exe        # Debug executable (symbols)
├── Data/
│   ├── cars.big       # Car models + textures
│   ├── tracks.big     # Track geometry + textures
│   ├── audio.big      # Music + SFX banks
│   ├── patch.big      # Engine patches + patches
│   ├── DB.big         # Database (Online.mdb)
│   ├── GUI.big        # UI textures
│   ├── feArt.big      # Front-end art
│   └── (other BIGs)   # Additional archives
├── lang/              # Localization
└── SaveData/          # Player saves
```

## Key File Types

| Extension | Type | Description |
|---|---|---|
| `.fce` | 3D Model | Car geometry (Full Car Editor format) |
| `.fsh` | Texture | Image/texture (EA FSH / SHPI format) |
| `.frd` | Track | Track road surface geometry |
| `.big` | Archive | EA archive (BIGF format) |
| `.viv` | Archive | Per-car archive containing FCE+FSH |
| `.bnk` | Audio | Sound bank (EA XA ADPCM audio) |
| `.mdb` | Database | Microsoft Access database (car stats) |
| `.trk` | Track | Track routing/graph data |
| `.trn` | Track | Track barrier/net geometry |
| `.fst` | Unknown | Unidentified format (possibly LOD/scene) |
| `.blf` | Car | Bill of materials (vertex segmentation) |
| `.lod` | Car | Level-of-detail distance thresholds |

## Supported Tools

- **fce2obj.py** — FCE car model → Wavefront OBJ converter
- **frd2obj.py** — FRD track geometry → OBJ converter
- **big_extract.py** — BIG archive extractor
- **viv_extract.py** — VIV per-car archive extractor
- **FSH → PNG** — Texture conversion (spec-based)
- **BNK → WAV** — Audio extraction (EA XA ADPCM decoder)

## Common Tasks

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
# First extract the .fce from the VIV
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
- Network protocol for online multiplayer
- Script/event system (if any .scm files exist?)
- Full BLF chunk type enumeration
- Track AI racing line / rubber-banding formula
- `Dirtoval` extra files (`track.fce`, `spdF0.bin`, `spdR0.bin`) purpose
