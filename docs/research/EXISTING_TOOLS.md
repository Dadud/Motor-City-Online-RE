# Existing Tools for MCO Asset Extraction

## Overview

The NFS modding community has produced robust, well-maintained tools for EA's earlier formats (NFS3-NFS5 era). MCO sits between NFS4 and NFS5, so existing tools have **partial or full support**.

---

## VIV Archive Extraction

### bfut/unvivtool ⭐ **Recommended**
- **Repo**: https://github.com/bfut/unvivtool
- **What**: CLI + Python module for VIV/BIG archives
- **Formats**: BIGF, BIGH, **BIG4**, 0x8000FBC0, wwww
- **MCO status**: MCO uses BIG4 and BIGF variants — should be fully supported
- **Install**: `python -m pip install unvivtool`
- **Notes**: Single-pass buffered read, validates/recovers corrupted archives, memory efficient

### TheXDS/VivLib ⭐ **Also recommended**
- **Repo**: https://github.com/TheXDS/VivLib
- **What**: C# library — read/write VIV, FSH, QFS, BNK, FCE, ENG, CARP, DAT
- **MCO status**: VIV read/write, FSH/QFS read/write, BNK read/write, FCE read/write — good coverage
- **Platform**: .NET library, cross-platform via .NET
- **Notes**: Originally part of Vivianne; supports FCE3/FCE4/FCE4M conversion

### NFS Wizard
- **Download**: NFSAddons or phosphorus.games
- **What**: GUI VIV editor for NFS3/NFS4
- **MCO status**: May work on MCO VIVs with tweaking
- **Notes**: Windows, old tool (needs XP compatibility mode)

### VIVEdit
- **Download**: NFSAddons (nfsaddons.com/downloads/nfshp2/tools/7409/)
- **What**: GUI VIV editor for NFS3/4/6
- **MCO status**: Unclear

### Our viv_extract.py
- **Location**: `mco-wiki/tools/viv_extract.py`
- Handles MCO-specific VIV quirks (DCL compression, embedded textures)
- Complementary to unvivtool, not a replacement

---

## FCE Model Format (Car Geometry)

### bfut/fcecodec ⭐ **Recommended**
- **Repo**: https://github.com/bfut/fcecodec
- **What**: Full FCE codec — decode/encode/validate, Python/C extension
- **Formats**: FCE3 (NFS3), FCE4 (NFS4), **FCE4M (MCO)** ← explicitly documented
- **Install**: `python -m pip install fcecodec`
- **Features**:
  - `IoDecode_Fce4M()` / `IoEncode_Fce4M()` — full FCE4M read/write
  - `IoExportObj()` — export to Wavefront OBJ with materials
  - Scripts: Fce2Obj.py, Obj2Fce3.py, MergeAllParts.py, etc.
- **Blender addon**: https://github.com/bfut/fcecodec_blender
  - Import/export .fce files directly in Blender 3.6–5.x
  - Auto-installs fcecodec + tinyobjloader + unvivtool Python deps

### TheXDS/VivLib
- **Repo**: https://github.com/TheXDS/VivLib
- Read/write FCE3, FCE4, **FCE4M**
- FCE3/FCE4/FCE4M **conversion** with part auto-renaming
- FCE center editing, color tables, part/dummy renaming, damage generation

### TheXDS/Vivianne
- **Repo**: https://github.com/TheXDS/Vivianne
- **What**: .NET GUI app — VIV extraction, FSH/QFS conversion, FCE editing, FEDATA editing
- **MCO status**: FCE editing and VIV extraction likely work; FEDATA may need MCO-specific support
- **Platform**: Windows (.NET 8), build from source for Linux/macOS
- **Notes**: Spiritual successor to NFS Wizard; active development

### NFS Wizard
- **What**: GUI FCE viewer/editor, VIV extractor, FEDATA editor
- **MCO status**: May work; old Windows tool
- **Notes**: Install in Windows 98 compatibility mode, run in XP mode

---

## FSH/QFS Texture Format

### bfut/fcecodec (built-in FSH support)
- FSH is well-understood: SHPI container, 5 pixel formats, RefPack compression
- `IoDecode_Fsh()` / `IoEncode_Fsh()` — read/write FSH files

### TheXDS/VivLib
- Read/write FSH, QFS (uncompressed, file-compressed, blob-compressed)
- Support for GIMX format (used in MCO offline version textures)
- Several image formats: 8-bit palette, 16/24/32-bit, DXT1/DXT5, RefPack variants
- Edit texture coords, import/export textures, rename blobs

### QFS Suite / QFS2FSH
- **Download**: phosphorus.games tools list
- Convert QFS ↔ FSH ↔ BMP ↔ TGA

### Our fsh2png.py
- **Location**: `mco-wiki/tools/fsh2png.py`
- Pure Python (PIL-based), works for MCO textures

### AndyGura/nfs-resources-converter
- **Repo**: https://github.com/AndyGura/nfs-resources-converter
- Python GUI, NFS 1-6, FSH → PNG
- Requires: Python 3.9+, ffmpeg, Blender 4+

---

## Track/FRD Format

### T3ED (Track Editor) ⭐ **Relevant**
- **Download**: NFSAddons (latest version by JimDiabolo)
- **What**: NFS3/4 track editor with FRD support
- **MCO status**: JimDiabolo's version has **preliminary MCO track format support**
  - MCO Format: `char header[44]; long nBlocks; long nPos; //vroad entries`
  - MCO trackblock info differs from NFS3/4 format
- **Repo (source)**: https://github.com/akw0088/T3ED (original by Denis Auroux)
- **Notes**: Actively updated version at NFSAddons; Windows GUI

### FRD2Blender
- **Download**: NFSAddons
- **What**: Convert NFS4 FRD → Lightwave LWO
- **MCO status**: NFS4 only, NOT MCO

### SpeedTools for Blender
- **Repo**: https://github.com/e-rk/speedtools
- **What**: Blender addon — import NFS4 HS tracks and cars directly
- **MCO status**: NFS4 High Stakes only, NOT MCO
- **Features**: Full geometry, objects, animations, lights, replay cameras, night/weather variants

### Dr. Vannacut's MCO FRD Research
- **Blog**: https://vannacut.com/2025/06/25/research-mco-frd-file-format/
- MCO FRD uses **DEADBEEF block markers** (0xDEADBEEF)
- Structure: `header[44]` → `nBlocks` → `nPos` → basename references
- No tool yet, but format is being documented

### Our frd2obj.py
- **Location**: `mco-wiki/tools/frd2obj.py`
- Research implementation — works partially (road surface decoded; native mesh topology unknown)
- Vertex int overflow in conversion for some tracks (extent values like 10^62)

---

## BNK / Audio Format

### TheXDS/VivLib
- **Repo**: https://github.com/TheXDS/VivLib
- Read PCM / EA ADPCM audio data
- Write PCM data, edit loop data
- Read/write BNK files (EA XA ADPCM — same codec MCO uses)

### Our bnk2wav.py
- **Location**: `mco-wiki/tools/bnk2wav.py`
- Pure Python, produces valid WAV from MCO BNK files
- EA XA ADPCM codec implementation

---

## Performance / FEDATA / Car Config

### TheXDS/VivLib
- Read/write .DAT car perf files
- Read/write .ENG engine data
- Read/write .BRI front-end car data
- Read/write CARP.TXT (text car performance)

### Viviane / NFS Wizard
- FEDATA editor built in
- Car classification and performance editing

### NFS HS CARP Generator
- **Download**: NFSAddons
- Spreadsheet tool for generating CARP.TXT from scratch

---

## Full Import Pipelines

### bfut/fcecodec + Blender addon
1. `unvivtool` → extract .fce from VIV
2. `bfut_fcecodec_blender` → import directly into Blender
3. Export as OBJ/GLB if needed

### SpeedTools for Blender (NFS4 only)
- Single addon imports track + car with textures, lights, cameras
- Not MCO-compatible but shows what a full pipeline looks like

### AndyGura/nfs-resources-converter
- NFS 1-6 full pipeline: 3D → OBJ/BLEND/GLB, audio → WAV, fonts, images → PNG
- **May** work with MCO if formats align

---

## Comprehensive Tool Collections

### phosphorus.games — Need for Speed Tools
- **URL**: https://phosphorus.games/tools-downloads/need-for-speed-tools/
- **Lists 20+ tools** including: NFS Wizard, T3ED, FRD2Blender, TRK2Blender, ZModeler 1/2/3, QFS Suite, CarEditor, Car2NFS3, NFS FCE Converter, and more

### NFSAddons Downloads
- **URL**: https://www.nfsaddons.com/
- Major repository for NFS modding tools (NFS3 through NFS6)

### NFS Modder's Corner Blog
- **URL**: https://nfsmodderscorner.blogspot.com/
- Tutorials, tool guides, format documentation for NFS3/4/HS

---

## What Still Has No Good Tool

| Format | Status | Notes |
|--------|--------|-------|
| **FST** (part.fst) | No tool | EA proprietary "Feature/Settings Table" — actual visible car exterior encoded here |
| **MCO FRD** | Partial | DEADBEEF structure known; no working converter yet |
| **LOD** (level-of-detail) | Partial | LOD format partially documented; no turnkey extraction |
| **FCE visible exterior** | No tool | FCE = collision geometry; visible exterior in FST bulk records |
| **MCO-specific ENG** | Unknown | VivLib reads ENG but MCO ENG variant needs verification |

---

## Recommendations by Task

| Task | Best Tool |
|------|-----------|
| Extract VIV archives | **unvivtool** (`pip install unvivtool`) |
| FCE → OBJ / Blender | **fcecodec** (`pip install fcecodec`) + **fcecodec_blender** addon |
| Edit VIV/FCE/FEDATA (GUI) | **Vivianne** (.NET, NFS3/4 focused, may extend to MCO) |
| Read/write VIV/FSH/BNK/FCE programmatically | **VivLib** (C#/.NET library) |
| Extract NFS3/4 tracks | **T3ED** or **FRD2Blender** |
| MCO track format research | Dr. Vannacut's blog + T3ED MCO mode |
| BNK audio → WAV | **bnk2wav.py** (our script) or VivLib |
| Textures (FSH → PNG) | **fsh2png.py** (our script) or fcecodec built-in |
| Full NFS1-6 pipeline | **nfs-resources-converter** (Python GUI) |

---

## Sources & Links

- https://github.com/bfut/fcecodec — FCE4M codec (NFS3/4/MCO)
- https://github.com/bfut/unvivtool — VIV/BIG decoder (BIG4/BIGF)
- https://github.com/bfut/fcecodec_blender — Blender addon
- https://github.com/TheXDS/VivLib — C# library (VIV, FSH, BNK, FCE, ENG, CARP)
- https://github.com/TheXDS/Vivianne — NFS3/4 GUI editor
- https://github.com/e-rk/speedtools — NFS4 HS Blender addon
- https://github.com/AndyGura/nfs-resources-converter — NFS 1-6 GUI converter
- https://phosphorus.games/tools-downloads/need-for-speed-tools/ — comprehensive tool list
- https://www.nfsaddons.com/ — major NFS modding repository
- https://vannacut.com/2025/06/25/research-mco-frd-file-format/ — MCO FRD research
- https://nfsmodderscorner.blogspot.com/ — NFS3/4/HS modding guides
