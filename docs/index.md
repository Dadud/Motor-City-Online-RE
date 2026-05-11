# Motor City Online — Documentation Index

## File Formats

### Documented

| Format | File | Description |
|--------|------|-------------|
| BIG / VIV | `big.md` | EA archive container. Both BIG4 (big-endian index) and BIGF (little-endian index) variants. |
| FCE | `fce.md` | Car geometry (FCE4M variant, magic `0x00101015`). 12 bytes/vertex, 56 bytes/triangle. |
| FSH | `fsh.md` | Texture images (SHPI container). Supports GIMX, G264, A8R8G8B8, A1R5G5B5 formats. |
| LOD | `lod.md` | Model LOD distance thresholds (7 float values). |
| INI | `ini.md` | Track configuration (info.ini, audio.ini, etc.) |

### Partial

| Format | File | Description | Unknowns |
|--------|------|-------------|----------|
| FRD | `frd.md` | Track road geometry. Road surface documented. | Native mesh topology, pointer-linked index data |
| FST | `fst.md` | Per-car part metadata. Header documented. | Post-pad bulk record semantics |
| BLF | `blf.md` | Car vertex segmentation. | Full chunk type list |
| MDB | `database.md` | Car database. Schema extracted. | Physics table field meanings |
| BNK | `bnk.md` | Audio banks. Header parsed. | Audio codec unidentified |

### Unknown

| Format | File | Description |
|--------|------|-------------|
| TRK | `trk.md` | AI racing line. Not analyzed. |

---

## Research Notes

| Topic | File | Status |
|-------|------|--------|
| Patch System | `research/patch-system.md` | Partial |
| Beta 1 Analysis | `research/beta1.md` | Documented |
| Oct09 Prototype | `research/oct09.md` | Documented |
| Network Protocol | `research/network.md` | Partial |
| EXE Architecture | `research/exe-architecture.md` | Partial |
| engpatch System | `formats/engpatch.md` | Partial |
| Community Tools | `research/EXISTING_TOOLS.md` | Catalogued |
| Update Notices Archive | `research/mco-update-notices-archive-scrape-20020608.md` | Archived |

---

## Media Archive

| Folder | Contents |
|--------|----------|
| `media/` | Articles, reviews, screenshots, box art — see `media/INDEX.md` |
