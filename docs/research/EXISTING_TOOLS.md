# Existing Tools for MCO Asset Extraction

## Overview

The NFS modding community has produced robust, well-maintained tools for EA's earlier formats (NFS3-NFS5 era). MCO sits between NFS4 and NFS5, so existing tools have partial or full support.

---

## VIV Archive Extraction

### bfut/unvivtool ⭐ **Recommended**
- **Repo**: https://github.com/bfut/unvivtool
- **What**: CLI + Python module for VIV/BIG archives
- **Formats**: BIGF, BIGH, BIG4, 0x8000FBC0, wwww
- **MCO status**: MCO uses BIG4 and BIGF variants — should be fully supported
- **Install**: `python -m pip install unvivtool`
- **Notes**: Single-pass buffered read, validates/recovers corrupted archives, memory efficient

### Alternatives
- **NFS3 VIV tools** (nfseditor.de) — NFS2/3 era, may work on some MCO files
- **Our viv_extract.py** — handles MCO-specific VIV quirks (DCL compression, embedded textures)

---

## FCE Model Format

### bfut/fcecodec ⭐ **Recommended**
- **Repo**: https://github.com/bfut/fcecodec
- **What**: Full FCE codec — decode/encode/validate
- **Formats**: FCE3 (NFS3), FCE4 (NFS4), **FCE4M (MCO)** ← explicitly documented
- **Install**: `python -m pip install fcecodec`
- **Features**:
  - `IoDecode_Fce4M()` / `IoEncode_Fce4M()` — full FCE4M read/write
  - `IoExportObj()` — export to Wavefront OBJ with materials
  - Blender addon available: **fcecodec_blender**
- **Blender addon**: https://github.com/bfut/fcecodec_blender
  - Import/export .fce files directly in Blender 3.6-5.x
  - Handles NFS3 & NFS4 car models
  - Auto-installs fcecodec + tinyobjloader + unvivtool Python deps

### bfut/Fce2Obj script
- **Location**: https://github.com/bfut/fcecodec/tree/main/scripts
- **What**: Ready-to-use FCE→OBJ conversion script
- **Usage**: Works with installed fcecodec Python module

### TheXDS/Vivianne
- **Repo**: https://github.com/TheXDS/Vivianne
- **What**: .NET GUI app for NFS3/4 — VIV editing, FSH/QFS, FCE editing
- **MCO status**: May support MCO FCE via format conversion tools
- **Platform**: Windows (.NET 8), build from source for others

---

## Track/FRD Format

### No well-maintained standalone tools found
- FRD (Feature Road Data) is EA proprietary
- Our frd2obj.py was a research implementation, not production-ready
- Community tools focus on NFS3/4 track formats, not MCO's FRD variant

---

## FSH Texture Format

### bfut/fcecodec (via unvivtool + FSH decode)
- FSH is well-understood: SHPI container, 5 pixel formats, RefPack compression
- **Our fsh2png.py** works but uses PIL — could be replaced with fcecodec's FSH support

### nfs-resources-converter
- **Repo**: https://github.com/AndyGura/nfs-resources-converter
- Python GUI app, NFS 1-6 support
- FSH → PNG conversion
- Requires Blender + FFmpeg for full features

---

## 3D Model Conversion (OBJ/BLEND/GLB)

### bfut/fcecodec + Blender addon ⭐ **Best path**
- Export FCE → OBJ directly via Python API
- Or use Blender addon for direct import/export

### AndyGura/nfs-resources-converter
- Python GUI, NFS 1-6
- Supports 3D → OBJ/BLEND/GLB
- **May** work with MCO files if formats align
- Requires: Python 3.9+, ffmpeg, Blender 4+

---

## What Still Has No Good Tool

| Format | Status | Notes |
|--------|--------|-------|
| **FST** (part.fst) | No tool | EA proprietary "Feature/Settings Table" — actual visible car exterior encoded here |
| **FRD** (track) | No tool | FRD→OBJ not solved; vertex format unknown |
| **LOD** (level-of-detail) | Partial | LOD format partially documented |
| **FCE visible body** | No tool | FCE = collision geometry; visible exterior in FST bulk records |

---

## Recommendations

1. **For VIV extraction**: Use **unvivtool** — handles BIG4/BIGF properly
2. **For FCE→OBJ**: Use **fcecodec** Python API or **Blender addon**
3. **For textures**: Use **fcecodec** FSH decode or our fsh2png.py
4. **For FST/FSD visible car exterior**: Still needs custom research — no existing tool
5. **For track meshes**: FRD format research needed; no turnkey solution

---

## Sources
- https://github.com/bfut/fcecodec — FCE4M codec (NFS3/4/MCO)
- https://github.com/bfut/unvivtool — VIV/BIG decoder
- https://github.com/bfut/fcecodec_blender — Blender addon
- https://github.com/TheXDS/Vivianne — NFS3/4 GUI editor
- https://github.com/AndyGura/nfs-resources-converter — NFS 1-6 GUI converter
