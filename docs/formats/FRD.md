# FRD — Track Road Geometry Format

> 3D road surface mesh for each track.
> **Status: Partial** — road surface extraction works; native mesh index format unknown.

**Magic:** `0xFEEEEB19` (little-endian uint32)
**Files:** `Tr.frd` (road surface), `Trn.frd` (barriers/nets)

---

## Format Identification

```python
with open("Tr.frd", "rb") as f:
    magic = struct.unpack('<I', f.read(4))[0]
assert magic == 0xFEEEEB19
```

---

## Header Structure

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | u32 LE | Magic: 0xFEEEEB19 |
| 0x04 | 4 | u32 LE | Version/flags |
| 0x08 | 4 | u32 LE | Unknown (token A) |
| 0x0C | 4 | u32 LE | Unknown (token B) |
| 0x30 | - | - | Primary road table begins here |

---

## Primary Road Table

Road samples begin at offset **0x30**. Each record is **84 bytes (0x54)**.

```c
struct FrdRoadSample {
    uint16_t flags;           // lane/edge/surface mask
    uint16_t section;
    uint32_t unk04;
    float    pos_x;           // world X
    float    pos_y;           // world Y (elevation)
    float    pos_z;           // world Z
    float    basis_right[3];  // right vector
    float    basis_up[3];     // up vector
    float    basis_fwd[3];    // forward vector
    float    width_left;      // half-width to left edge
    float    width_right;     // half-width to right edge
    float    param2;           // unknown (often 5.0)
    float    param3;           // unknown (often 5.0)
    uint32_t material;         // surface material
    uint32_t group_a;          // usually 1 or 2
    uint32_t group_b;          // usually 1
};
```

### Road Strip Reconstruction

```
left  = (pos_x, pos_y, pos_z) - basis_right * width_left
right = (pos_x, pos_y, pos_z) + basis_right * width_right

triangle A: left_i, left_i+1, right_i
triangle B: right_i, left_i+1, right_i+1
```

See `tools/frd2obj.py` — working implementation.

---

## Secondary Structure — Status: Unknown

After the primary road table, the format has a polygon block system around offset **~0x19860**:

```
Offset ~0x19860: Polygon block header
  uint32_t point_count
  uint32_t ptr_a (base 0x03FB0000)
  uint32_t ptr_b (base 0x03FB0000)
  float[point_count][3]  — vertex cloud

Pointer tables (after polygon blocks):
  0x03FCxxxx — polygon vertex refs
  0x03FDxxxx — unknown
  0x03FExxxx — likely index/connectivity
  0x03FFxxxx — unknown
```

The **native mesh index format** has not been decoded. The road surface triangulation produces a drivable surface but is not the original authored mesh.

---

## See Also

- `tools/frd2obj.py` — working road mesh extractor
- `data/tracks/` — extracted OBJ meshes
