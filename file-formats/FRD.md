# FRD — Track Road Geometry Format

> 3D road surface mesh for each track.

**Magic:** `0x19EB 0xEEFE` (two-magic, little-endian: `EB 19 FE EE`)  
**Variant:** Tr.frd (road surface), Trn.frd (barriers/nets)  
**Typical size:** 1–4 MB per track

## Header Structure

| Offset | Size | Description |
|--------|------|-------------|
| 0x00 | 2 | Magic byte 1: `0xEB19` |
| 0x02 | 2 | Magic byte 2: `0xEEFE` |
| 0x04 | 4 | Header size / index table offset (?) |
| 0x08 | 4 | Vertex count or table offset |
| 0x0C | 4 | Triangle/segment count |
| 0x10 | 4 | Unknown (often `0x00000000`) |
| 0x14 | 4 | Coordinate scale? (often `0x00000000`) |
| 0x18 | 4 | Float: some Y coordinate reference |
| 0x1C | 4 | Float: another coordinate reference |
| 0x20 | 4 | Float: Z coordinate reference |
| 0x24 | 4 | Float: `0.0` |
| 0x28 | 4 | Float: `0.0` |
| 0x2C | 4 | Unknown |
| 0x30 | — | Data begins here (vertices, then indices) |

## Coordinate System

Track coordinates are world-space floats. Verified sample values:
- `(-705.22, 0.21, 643.89)` — track segment center
- `(-0.0159, 16.20, 16.29)` — track width/dimensions
- Y appears to be elevation; track sits at Y ≈ 0
- X/Z are horizontal plane

## Vertex Format

Appears to be 48 bytes per vertex (could be 3 floats + extras):
- 12 bytes: XYZ position (float[3])
- Additional data: normals, UVs, or per-vertex flags

**Confirmed:** float triplets at `0x30+` are world-space XYZ coordinates.

## Index Format

Track mesh uses an indexed triangle list. Index size unknown (could be 16-bit or 32-bit indices).

## Track Types

| File | Purpose |
|------|---------|
| `Tr.frd` | Road surface geometry |
| `Trn.frd` | Barrier / net / wall geometry |

## Per-Track Directory Structure

Each track directory contains:

| File | Description |
|------|-------------|
| `Tr.frd` | Road surface mesh |
| `Trn.frd` | Barrier geometry |
| `TrCam.ini` | Camera waypoints |
| `TrCamNW.ini` | (Northwest?) camera variant |
| `TrSFX.FSH` | Track segment textures |
| `TrW.ini` | (Water?) configuration |
| `TrNW.ini` | (Northwest?) configuration |
| `audio.ini` | Track audio settings |
| `audioN.ini` | Northern audio variant |
| `audioNW.ini` | Northwest audio variant |
| `audioW.ini` | Western audio variant |
| `boom.ini` | Explosion/crash audio |
| `pavement.ini` | Surface physics parameters |
| `track.fsh` | Track texture atlas |
| `track.fce` | Track 3D model (some tracks) |
| `track.bnk` | Track audio bank |
| `Trmap.bin` | Track minimap data |
| `trmap.txt` | Track minimap text |
| `spdF0.bin` | Speed trap front (F0) data |
| `spdR0.bin` | Speed trap rear (R0) data |
| `slides.fsh` | Slide/skid mark textures |
| `data.csv` | Per-track CSV data table |
| `info.ini` | Track metadata (ID, name, segments, speed traps) |
| `tr.col` | Collision data |
| `tr.trk` | Track routing / AI graph |

## Extraction

Use `frd2obj.py` to convert FRD files to OBJ format:

```bash
python3 frd2obj.py track.frd output.obj
```

The script parses the header, extracts vertices and indices, and outputs a Wavefront OBJ with the road mesh.

## Open Questions

- Exact vertex format (stride, attributes)
- Index buffer format and size
- How the road surface connects (spline-based or discrete mesh?)
- Relationship between Tr.frd and the road texture atlas (track.fsh)
- Purpose of `Trmap.bin` / `trmap.txt` (minimap format)
- `spdF0.bin` / `spdR0.bin` speed trap data structure
- How track AI racing lines are defined
