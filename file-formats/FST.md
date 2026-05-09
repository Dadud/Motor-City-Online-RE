# FST — Unknown Format

> Purpose and structure unknown. Possibly LOD/scene data.

**Magic:** `0xE0134678` (big-endian — unusual for an otherwise little-endian game)  
**Location:** Inside each car VIV as `part.fst`  
**Typical size:** 400–800 KB  
**Variants:** Different sizes per car (53chevy: 513 KB, 96supra: 791 KB)

## Header Fields

| Offset | Value (53chevy) | Description |
|--------|-----------------|-------------|
| 0x00 | `0xE0134678` | Big-endian magic |
| 0x04 | — | Header fields (unknown structure) |

Header values observed (at offset 0x10+):

| Index | 53chevy | 8ball | 96supra | Description |
|-------|---------|--------|---------|-------------|
| 0 | 5 | 5 | 5 | Unknown (constant) |
| 1 | 43 | 28 | 43 | Matches FCE `NumParts` |
| 2 | 56 | 167 | 40 | Unknown |
| 3 | 951 | 1086 | 1675 | Possibly triangle count? |
| 4 | 352 | 259 | 491 | Unknown |
| 5 | 352 | 259 | 491 | Same as index 4 (symmetric pairs?) |
| 6 | 2853 | ? | ? | Unknown |
| 7 | 952 | ? | ? | Unknown |

## Hypotheses

Given that FST appears only inside VIV car archives alongside FCE and BLF:

1. **LOD (Level of Detail) data** — but LOD already handled by `part.lod` (28 bytes)
2. **Per-vertex skin weights** — for bone-based animation (not used in MCO?)
3. **Shader/vertex program data** — GPU program or shader parameters
4. **Collision mesh** — separate from visual mesh (but collision is in `tr.col`)
5. **Scene graph** — spatial organization of car parts

## Status

**NOT YET DECODED** — requires more analysis or reference to EA documentation.
