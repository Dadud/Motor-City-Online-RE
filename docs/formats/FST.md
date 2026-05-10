# FST — Part Feature/Settings Table

> Per-car mesh annotation and feature metadata. Companion to FCE geometry.
> **Status: Partial** — header confirmed; post-pad bulk records unknown.

**Magic:** `E0 13 46 78` (big-endian)
**Location:** Inside car VIV as `part.fst`
**Typical size:** 400-800 KB

---

## Format Identification

```python
with open("part.fst", "rb") as f:
    magic_be = struct.unpack('>I', f.read(4))[0]
assert magic_be == 0xE0134678
```

---

## File Structure

```
Offset 0x00:  Big-endian magic (4 bytes)
Offset 0x04:  Big-endian signature block (12 bytes)
Offset 0x10:  Little-endian header (32 bytes)
Offset 0x30:  Descriptor table (8 bytes per entry)
[0x0300 padding]: Reserved sentinel words
[bulk data]:      Post-pad 16-byte records
```

---

## Header (Little-Endian, 32 bytes)

| Offset | Type | Description |
|--------|------|-------------|
| 0x10 | u32 | Format version (=5) |
| 0x14 | u32 | **Number of FCE parts** (matches FCE header exactly) |
| 0x18 | u32 | Descriptor count x 4 = descriptor table size |
| 0x1C | u32 | 0x0300 padding word count |
| 0x20 | u32 | Unknown aggregate A |
| 0x24 | u32 | Duplicate of 0x20 |
| 0x28 | u32 | = 0x1C x 3 (derived) |
| 0x2C | u32 | Unknown aggregate C |

**Confirmed invariant:** FST num_parts (offset 0x14) == companion FCE num_parts (offset 0x11C).

---

## Descriptor Table

Begins at offset 0x30. Each entry is **8 bytes** (4 x uint16 LE):

```c
struct FstEntry {
    uint16_t f0;   // Usually 0
    uint16_t f1;   // Varies (0-26)
    uint16_t f2;   // Type/category (0, 1, 2, or 6)
    uint16_t f3;   // Usually 0
};
```

---

## 0x0300 Padding Region

Filled with 0x0300 (LE uint16) sentinel words. Purpose: unknown (reserved or alignment).

---

## Post-Pad Bulk Data — Status: Unknown

16-byte records following the padding. Structure:

```c
struct FstPostPadRecord {
    uint16_t f0, f1, f2, f3, f4, f5, f6, f7;
    // f4 == f6, f5 == f7 (confirmed invariant)
};
```

**Record count:** (file_size - padding_end) / 16

**Hypothesis (unverified):** Per-triangle annotations.

Evidence: 53chevy has 2,271 triangles and 31,954 FST records. Ratio ~14.07 — suggests ~14 records per triangle. Without FCE triangle index comparison, this remains speculative.

---

## Acronym — Unverified

"FST" stands for "Feature/Settings Table" — this is a plausible guess, not a confirmed expansion. Do not treat it as fact.

---

## See Also

- `docs/formats/fce.md` — companion FCE geometry
- `docs/formats/blf.md` — related per-car segmentation
