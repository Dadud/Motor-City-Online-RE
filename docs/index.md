# Motor City Online — Documentation Index

**⚠️ Confidence ratings are strict.** "Complete" means verified on multiple samples. "Partial" means the core structure is understood but some fields or variants are not.

---

## File Formats

### ✅ Verified — Working implementations exist

| Format | File | Description |
|--------|------|-------------|
| BIG / VIV | `big.md` | EA archive container. Both BIG4 (big-endian index) and BIGF (little-endian index) variants. |
| FCE | `fce.md` | Car geometry (FCE4M variant, magic `0x00101015`). 12 bytes/vertex, 56 bytes/triangle. |
| FSH | `fsh.md` | Texture images (SHPI container). Supports GIMX, G264, A8R8G8B8, A1R5G5B5 formats. |
| LOD | `lod.md` | Model LOD distance thresholds (7 float values). |
| INI | `ini.md` | Track configuration (info.ini, audio.ini, etc.) |

### 🔶 Partial — Core structure understood; some unknowns remain

| Format | File | Description | Unknowns |
|--------|------|-------------|----------|
| FRD | `frd.md` | Track road geometry. Road surface decoded to OBJ. | Native mesh topology, pointer-linked index data |
| FST | `fst.md` | Per-car part metadata. Header decoded. | Post-pad bulk record semantics (~14 per triangle, purpose unknown) |
| BLF | `blf.md` | Car vertex segmentation. | Full chunk type list for type-2 entries |
| MDB | `database.md` | Car database. Schema extracted. | Physics table field meanings |
| ENGPATCH | `engpatch.md` | Engine audio archive (BIGF, 504 members). | Runtime BNK selection, CRDl table format |

### 🔴 Unknown — Format/purpose identified; decoding blocked

| Format | File | Description | Blocker |
|--------|------|-------------|---------|
| BNK | `bnk.md` | Audio banks. Header parsed. | Audio codec unidentified (encrypted proprietary) |
| TRK | `trk.md` | AI racing line. | No progress |

---

## Research Notes

| Topic | File | Status |
|-------|------|--------|
| Patch System | `research/patch-system.md` | 🔶 Partial |
| Beta 1 Analysis | `research/beta1.md` | ✅ Verified |
| Oct09 Prototype | `research/oct09.md` | ✅ Verified |
| Network Protocol | `research/network.md` | 🔶 Partial |
| EXE Architecture | `research/exe-architecture.md` | 🔶 Partial |
| engpatch System | `formats/engpatch.md` | 🔶 Partial |

---

## Evidence Levels

Used throughout this documentation:

| Level | Tag | Meaning |
|-------|-----|---------|
| 0 | Unverified | Guess or indirect clue |
| 1 | Observed | Seen in one sample |
| 2 | Reproduced | Confirmed on multiple samples |
| 3 | Cross-checked | Confirmed through independent paths |
| 4 | Operational | Suitable for implementation |

Claims are tagged like: `[E2]` — meaning Level 2, reproduced.
