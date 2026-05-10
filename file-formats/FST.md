# FST — Part Feature/Settings Table

> Per-car mesh annotation and feature metadata table. Companion to FCE geometry.

**Magic:** `E0 13 46 78` (big-endian)  
**Location:** Inside each car VIV as `part.fst`  
**Typical size:** 400–800 KB  
**Variants:** Different sizes per car (53chevy: 513 KB, 8ball: 452 KB)

## File Structure

```
+-------------------+
|  Big-endian magic | 0x00 (4 bytes): E0 13 46 78
+-------------------+
|  Magic bytes 2-4  | 0x04 (12 bytes): additional signature
+-------------------+
|  Little-endian    | 0x10 (8 words = 32 bytes)
|  Header           |
+-------------------+
|  Descriptor Table | variable (56 descriptor_units = 28 entries)
+-------------------+
|  0x0300 Padding  | 951 words (reserved/sentinel)
+-------------------+
|  Post-pad Bulk    | 31929 × 16-byte records
+-------------------+
```

## Header Fields (Little-Endian)

| Offset | Type | 53chevy | 8ball | 96supra | Description |
|--------|------|---------|-------|---------|-------------|
| 0x10 | u32 | 5 | 5 | 5 | Format version |
| 0x14 | u32 | 43 | 28 | 43 | **Number of FCE parts** (confirmed match) |
| 0x18 | u32 | 56 | 167 | 40 | Descriptor count (in 2-byte units) |
| 0x1C | u32 | 951 | 1086 | 1675 | Padding word count (0x0300 fill) |
| 0x20 | u32 | 352 | 259 | 491 | Unknown aggregate A |
| 0x24 | u32 | 352 | 259 | 491 | Duplicate of 0x20 |
| 0x28 | u32 | 2853 | — | — | = 0x1C × 3 (derived) |
| 0x2C | u32 | 952 | — | — | Unknown aggregate C |

## Descriptor Table

Each descriptor is **8 bytes** (4 × uint16):

```c
struct FstEntry {
    uint16_t f0;      // Usually 0
    uint16_t f1;      // Varies (vertex count? part size?)
    uint16_t f2;      // Usually 0, 1, 2, or 6 (type/category?)
    uint16_t f3;      // Usually 0
};
```

**Example from 53chevy (43 parts):**

| Part | f0 | f1 | f2 | f3 | Notes |
|------|----|----|----|----|-------|
| 0 | 0 | 59 | 0 | 0 | |
| 1 | 0 | 9 | 6 | 0 | f2=6 |
| 2 | 0 | 49 | 0 | 0 | |
| 3 | 0 | 2 | 1 | 0 | f2=1 |
| 4 | 0 | 2 | 0 | 0 | |
| ... | ... | ... | ... | ... | |
| 27 | 0 | 4 | 6 | 0 | f2=6 |

**Field f2 values observed:** 0, 1, 2, 6 (possibly material/type categories)

## Post-Pad Bulk Data

After the 0x0300 padding fill, a large data region begins at offset 0x95E (for 53chevy).

### Record Structure

Each record is **16 bytes** (8 × uint16):

```c
struct FstPostPadRecord {
    uint16_t f0;      // Vertex/index reference
    uint16_t f1;      // Vertex/index reference  
    uint16_t f2;      // Often == f3
    uint16_t f3;      // Often == f2
    uint16_t f4;      // ALWAYS == f6
    uint16_t f5;      // ALWAYS == f7
    uint16_t f6;      // ALWAYS == f4 (duplicate)
    uint16_t f7;      // ALWAYS == f5 (duplicate)
};
```

### Key Structural Finding

- **f4 ≡ f6 and f5 ≡ f7 for ALL records** (100% confirmed)
- f2 and f3 are equal ~66% of the time

This paired duplicate structure strongly suggests **mesh vertex index pairs** or **UV coordinate pairs**.

### Value Ranges (53chevy sample)

| Field | Min | Max | Unique (500 samples) | Likely meaning |
|-------|-----|-----|---------------------|---------------|
| f0 | 0 | 229 | 124 | Index reference |
| f1 | 1 | 228 | 106 | Index reference |
| f2 | 0 | 150 | 149 | Smoothing/normal index |
| f3 | 0 | 149 | 118 | Smoothing/normal index |
| f4 | 0 | 132 | 131 | UV or vertex index |
| f5 | 0 | 131 | 100 | UV or vertex index |
| f6 | 0 | 132 | 131 | Same as f4 |
| f7 | 0 | 131 | 100 | Same as f5 |

These ranges are too small for absolute vertex positions but consistent with **array indices**.

## Relationship to FCE

Confirmed:
- FST `num_parts` (offset 0x14) equals the FCE part count exactly

Example cars:
- 53chevy: 43 parts (both FST and FCE)
- 8ball: 28 parts
- 96supra: 43 parts

This strongly suggests FST is a **companion annotation file** for FCE geometry.

## Status

**PARTIALLY DECODED** ✓

| Aspect | Status |
|--------|--------|
| Header structure | ✓ Decoded |
| Descriptor table | ✓ Decoded (purpose partially understood) |
| 0x0300 padding | ✓ Confirmed as sentinel/reserved |
| Post-pad structure | ✓ 16-byte records with paired duplicates |
| Post-pad meaning | ◐ Partially understood (likely mesh indices) |
| FST acronym | ◐ Best guess: "Feature/Settings Table" |

## Recommended Next Steps

1. **Compare FST against FCE vertex data** — verify if post-pad indices match FCE vertex arrays
2. **Compare trim variants** — see how FST differs between base car and SS/RS variants
3. **Tool development** — write a tool to visualize FST as mesh to verify indices
4. **Executable analysis** — search MCO executable for FST loading code

## See Also

- [FCE format](./FCE.md) — Car geometry format (companion)
- [BLF format](./BLF.md) — Bill of materials (related car data)
