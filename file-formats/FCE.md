# FCE — Full Car Editor 3D Model Format

> Car body geometry, dashboard geometry, spoiler geometry.

**Format:** FCE4M (Motor City Online variant)  
**Magic:** `0x00101015` (little-endian uint32)  
**Header size:** `0x2038` (8256 bytes)  
**Used in:** VIV car archives (part.fce, dash.fce, spoiler.fce)

## Header Structure

Located at file offset `0x00`, 8256 bytes total:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `Version` | Magic: `0x00101015` (FCE4M variant) |
| 0x04 | 4 | `Unknown1` | Reserved, usually 0 |
| 0x08 | 4 | `NumTriangles` | Triangle count |
| 0x0C | 4 | `NumVertices` | Vertex count |
| 0x10 | 4 | `NumArts` | Texture page count (usually 1) |
| 0x14 | 4 | `VertTblOffset` | Vertex table offset (relative to 0x2038) |
| 0x18 | 4 | `NormTblOffset` | Normal table offset (relative to 0x2038) |
| 0x1C | 4 | `TriaTblOffset` | Triangle table offset (relative to 0x2038) |
| 0x20 | 4 | `Reserve1Offset` | Reserved section 1 offset |
| 0x24 | 4 | `Reserve2Offset` | Reserved section 2 offset |
| 0x28 | 4 | `Reserve3Offset` | Reserved section 3 offset |
| 0x2C | 4 | `UndamgdVertOffset` | Undamaged vertex data offset |
| 0x30 | 4 | `UndamgdNormOffset` | Undamaged normal data offset |
| 0x34 | 4 | `DamgdVertOffset` | Damaged vertex data offset |
| 0x38 | 4 | `DamgdNormOffset` | Damaged normal data offset |
| 0x3C | 4 | `Reserve4Offset` | Reserved section 4 offset |
| 0x40 | 4 | `AnimTblOffset` | Animation table offset |
| 0x44 | 4 | `Reserve5Offset` | Reserved section 5 offset |
| 0x48 | 4 | `Reserve6Offset` | Reserved section 6 offset |
| 0x4C | 12 | `HalfSize[3]` | Bounding box half-size (X, Y, Z) for collision |
| 0x58 | 4 | `NumDummies` | Number of dummy/light objects (0–16) |
| 0x5C | 48 | `Dummies[16]` | Dummy object XYZ coordinates (16 × 3 × float) |
| 0x11C | 4 | `NumParts` | Number of car parts (0–64) |
| 0x120 | 768 | `PartPos[64]` | Part position XYZ array (64 × 3 × float) |
| 0x420 | 256 | `P1stVertices[64]` | First vertex index per part (uint32[64]) |
| 0x520 | 256 | `PNumVertices[64]` | Vertex count per part (uint32[64]) |
| 0x620 | 256 | `P1stTriangles[64]` | First triangle index per part (uint32[64]) |
| 0x720 | 256 | `PNumTriangles[64]` | Triangle count per part (uint32[64]) |
| 0x0E28 | 4096 | `PartNames[64]` | Part names (64 bytes each, ASCIIZ) |
| 0x1E28 | 560 | (padding) | Filled with zeros |

## Data Tables

Absolute file offsets for each table:

```
VertTbl = 0x2038 + VertTblOffset
NormTbl = 0x2038 + NormTblOffset
TriaTbl = 0x2038 + TriaTblOffset
```

### Vertex Table

`NumVertices` entries, 12 bytes each (3 × float32):

```
struct Vertex {
    float x, y, z;    // Model-space coordinates
}
```

### Normal Table

`NumVertices` entries, 12 bytes each (3 × float32):

```
struct Normal {
    float nx, ny, nz;  // Normalized normal vector
}
```

### Triangle Table

`NumTriangles` entries, 56 bytes each:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `TexPage` | Texture page / material ID |
| 0x04 | 12 | `VIdx[3]` | Vertex indices (local to part, 0-based) |
| 0x10 | 12 | `Unknown[3]` | Always `0xFF00FFFF` or similar (padding/flags) |
| 0x1C | 4 | `Flag` | Rendering flags (see below) |
| 0x20 | 12 | `U[3]` | U texture coordinates per vertex |
| 0x2C | 12 | `V[3]` | V texture coordinates per vertex (**stored as 1-V**) |

### Triangle Flags

| Flag | Meaning |
|------|---------|
| `0x0000` | Default (body paint) |
| `0x0001` | Matte (no chrome reflection) |
| `0x0002` | High chrome (windows, chrome trim) |
| `0x0004` | No cull (two-sided rendering) |
| `0x0008` | Semi-transparent (windows) |
| `0x0010` | Partial (unknown) |
| `0x0020` | All windows |
| `0x0040` | Front window |
| `0x0080` | Left window |
| `0x0100` | Back window |
| `0x0200` | Right window |
| `0x0400` | Broken window |
| `0x1000` | FST marker |

## Example: 53chevy/part.fce

| Property | Value |
|---|---|
| Vertices | 1222 |
| Triangles | 2271 |
| Parts | 43 |
| Vertex data offset | `0x2038` (14664) |
| Normal data offset | `0x5980` (22912) |
| Triangle data offset | `0x92C8` (37608) |

## Part Names (from 53chevy)

`body`, `hood`, `trunk`, `lf_door`, `rf_door`, `lr_door`, `rr_door`, ` windshield`, `r_win`, `l_win`, `b_win`, `front_clip`, `rear_clip`, `lf_wheel`, `rf_wheel`, `lr_wheel`, `rr_wheel`, `engine`, `interior`, `dash`, `steering_wheel`, `lr_exhaust`, `rr_exhaust`, `spoiler`, `lf_headlight`, `rf_headlight`, `lb_taillight`, `rb_taillight`, `hood_ornament`, `antenna`, `spare_tire`, `soft_top`, `convertible_top`, `undercarriage`, and others.

## Extraction

Use `fce2obj.py` to convert FCE files to Wavefront OBJ format:

```bash
python3 fce2obj.py input.fce output.obj
```

The converter:
1. Reads the FCE4M header
2. Extracts vertex, normal, and triangle tables
3. Outputs a `.obj` file with vertices, normals, and UV-mapped triangles

## Open Questions

- Animation table structure (`AnimTblOffset`) — unused in MCO?
- Reserve sections 1–6 — what data did EA store there?
- `UndamgdVertOffset` / `DamgdVertOffset` — how does damage deformation work exactly?
- `NumArts` field — is it always 1 or can cars have multiple texture pages?
