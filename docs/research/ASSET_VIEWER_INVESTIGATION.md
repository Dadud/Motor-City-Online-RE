# Asset Viewer Investigation — FCE Geometry Analysis

**Date:** 2026-05-10  
**Issue:** Car model renders look "abstract/broken" on GitHub gallery

---

## Summary

The committed car OBJ files are a MIX of correct and incorrect FCE sections:
- ✅ **96supra, 8ball**: Committed files ARE the `:Hbody` (correct body FCE)
- ❌ **53chevy**: Committed uses `:F :L :S_DM driver mirror` (interior) — WRONG
- ❌ **59impala**: Committed uses interior — WRONG  
- ❌ **97eclps**: Committed uses `:PHspoiler` (33v) — WRONG
- ⚠️ **55cameo, 56ftruck**: No `:Hbody` exists in VIV — only hood/steering available

Even the **correct** body FCE renders look abstract/broken because:
- FCE files appear to be **collision/interior geometry**, NOT the visible exterior
- The actual visible car body is in `part.fst` (513KB, EA proprietary GUID-based format)

---

## FCE Format (Confirmed)

FCE = "Full Car Editor" — car body/dashboard geometry for collision & interior

Each car VIV contains **multiple FCE sections** (body, interior, spoiler):

| Car | FCE Section | Vertices | Triangles | Part Name |
|-----|-------------|----------|-----------|-----------|
| 53chevy | [0] | 1193 | 1431 | `:F :L :S_DM driver mirror` (interior) |
| 53chevy | [1] | 1222 | 2271 | `:Hbody` |
| 53chevy | [2] | 18 | 26 | `:PHspoiler` |
| 96supra | [0] | 1529 | 3652 | `:Hbody` |
| 96supra | [1] | 37 | 46 | `:PHspoiler` |
| 8ball | [0] | 1120 | 1716 | `:Hbody` |
| 8ball | [1] | 1103 | 1306 | `:F :L :S_DM driver mirror` (interior) |
| 59impala | [0] | 1326 | 1609 | `:F :L :S_DM driver mirror` (interior) |
| 59impala | [1] | 1093 | 2355 | `:Hbody` |
| 55cameo | [0] | 1201 | 2480 | `:Hhood` (NO body FCE) |
| 56ftruck | [0] | 1091 | 2061 | `:Hsteer` (NO body FCE) |
| 97eclps | [0] | 33 | 60 | `:PHspoiler` |
| 97eclps | [1] | 1147 | 2272 | `:Hinterior` (NO exterior) |

**The old `viv_extract` tool only extracted the FIRST FCE section** — which was interior for some cars and body for others (inconsistent extraction).

---

## Root Cause: Wrong FCE Sections Committed

The committed OBJ files came from the first FCE section found in each VIV (old viv_extract behavior). This happened to be body for 96supra/8ball but interior for 53chevy/59impala/97eclps.

**Fixed:** Updated `viv_extract.py` to extract ALL FCE sections from each VIV, naming them by their part name.

---

## Why Body FCE Still Looks Abstract

Even the CORRECT body FCE (e.g., 96supra with 1529v, 3652t) renders as abstract shapes, NOT a recognizable car. This is because:

1. **FCE = collision/interior format**, not render geometry
2. The actual **visible exterior car body** is in `part.fst` (513KB GUID-based format)
3. FCE has `NumParts=43` (hood, doors, wheels, body panels, etc.) — all separate meshes in one file
4. These parts are likely positioned in world space via `PartPos[i]` offsets, NOT in a unified local coordinate system

The `part.fst` file (513KB for 53chevy) starts with a Microsoft UUID/GUID (`e0134678-c995-d111-960a-0010-5ae42069`) — an EA proprietary format that has NOT been reverse-engineered.

---

## FCE Header (Confirmed)

```
Offset  Size  Field
0x00    4     Magic: 0x00101015 (FCE4M variant)
0x04    4     Version
0x08    4     NumTriangles
0x0C    4     NumVertices
0x10    4     NumArts (texture pages)
0x14    4     VertTblOffset (relative to 0x2038)
0x18    4     NormTblOffset
0x1C    4     TriaTblOffset
0x4C    12    HalfSize[3] (bounding box)
0x58    4     NumDummies
0x5C    48    Dummies[16] (light positions)
0x11C   4     NumParts
0x120   768   PartPos[64] (part position XYZ)
0x420   256   P1stVertices[64] (first vertex index per part)
0x520   256   PNumVertices[64] (vertex count per part)
0x620   256   P1stTriangles[64]
0x720   256   PNumTriangles[64]
0x0E28  4096  PartNames[64] (64 bytes each, ASCIIZ)
0x2038  ...   Data tables start here
```

---

## Key Files

- `mco-wiki/tools/viv_extract.py` — **FIXED**: Now extracts ALL FCE sections
- `mco-wiki/tools/fce2obj.py` — Converts FCE to OBJ (uses local vertex indices correctly)
- `mco-wiki/data/car_models/` — Committed OBJ files (SOME have wrong FCE section)
- `mco-files/final/MCity_Update/Data/Models/*.viv` — Source car archives

---

## Next Steps

1. **Reverse-engineer `part.fst` format** — this is the actual visible car body (513KB per car)
   - Format: EA proprietary, starts with GUID `e0134678-c995-d111-960a-0010-5ae42069`
   - Likely related to EA's "Fast" model format used in GameCube games
   - Reference: `https://github.com/pleonex/tinke` (Nintendo file format tools)

2. **Combine FCE body parts with PartPos offsets** — if FCE body parts have world-space positions, they need to be combined properly

3. **Verify 55cameo/56ftruck** — these cars have no `:Hbody` FCE. The exterior body might be in `part.fst` only

4. **Fix committed car OBJ files** — replace 53chevy, 59impala, 97eclps with correct body FCE

---

## Files Created This Session

- `mco-gallery/index.html` — Updated gallery with correct body FCE renders
- `mco-gallery/*_FINAL.png` — Best renders from available body FCE
- `/tmp/car_bodies/*.obj` — Correct body FCE extractions
- `/tmp/53chevy_all/*.obj` — All FCE sections combined
