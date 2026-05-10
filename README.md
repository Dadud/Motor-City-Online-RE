# Motor City Online — Reverse Engineering

Preservation documentation for Electronic Arts Seattle's 2001 online racing MMO. This repo documents the game's binary file formats, network systems, and executable architecture — reverse-engineered from the final retail build.

> This project is for preservation and educational purposes. Do not use it to play without purchasing the game or to infringe on EA's copyrights.

---

## Repository Status

This is a **live reverse-engineering effort**. Not everything is decoded. Every claim is tagged with its evidence level — see [Evidence Key](#evidence-key) below.

**What's solid:** BIG/VIV archives, FCE geometry, FSH textures, LOD, INI, extracted database schema, engpatch archive structure, patch pipeline.

**What's blocked:** BNK audio codec (encrypted), engine curve tables (proprietary format), runtime audio selection algorithm (no disassembly), CASTANET serialization format (no live capture possible).

---

## Evidence Key

| Tag | Meaning |
|-----|---------|
| **Verified** | Confirmed on multiple samples; working implementation exists |
| **Partial** | Core structure understood; some fields or variants unknown |
| **Unknown** | Format/purpose identified; decoding blocked or not started |

---

## Quick Reference

### File Formats — `docs/formats/`

| Format | Status | Notes |
|--------|--------|-------|
| BIG / VIV | Verified | Archive container; BIG4 + BIGF variants |
| FCE | Verified | Car geometry; 12B/vertex, 56B/triangle |
| FSH | Verified | Textures; multiple pixel formats |
| LOD | Verified | LOD distance thresholds |
| INI | Verified | Track configuration |
| FRD | Partial | Road surface decoded; native mesh unknown |
| FST | Partial | Header confirmed; bulk records unknown |
| BLF | Partial | Vertex segmentation; chunk list partial |
| MDB | Partial | Schema extracted; physics fields unknown |
| ENGPATCH | Partial | Archive decoded; runtime selection unknown |
| BNK | **Unknown** | Header parsed; codec unidentified (encrypted) |
| TRK | **Unknown** | Not analyzed |

### Game Systems — `docs/research/`

| Topic | Status |
|--------|--------|
| Patch System | Partial — pipeline confirmed; offline mode partial |
| Network Protocol | Partial — donated by Molly; serialization unknown |
| EXE Architecture | Partial — NPS subsystem mapped |
| Beta 1 (Jun 2001) | Verified |
| Oct09 Prototype | Verified |

### Tools — `tools/`

| Tool | Input → Output | Status |
|------|---------------|--------|
| `viv_extract.py` | .viv → files | Works |
| `big_extract.py` | .big → files | Works |
| `fce2obj.py` | part.fce → OBJ | Works |
| `frd2obj.py` | Tr.frd → OBJ mesh | Works |
| `fsh2png.py` | .fsh → PNG | Works |
| `mdb_extract.py` | Online.mdb → CSV | Partial — small tables only |
| `bnk2wav.py` | track.bnk → WAV | **Blocked** — codec unknown |

### Extracted Data — `data/`

| File | Contents |
|------|----------|
| `Cars-complete.csv` | 83 unique car IDs, 72 base models |
| `Brand.csv` | 103 manufacturers |
| `StockEngines.csv` | 26 stock engine configs |
| `cars/` | Extracted OBJ car meshes (16 models) |
| `tracks/` | Extracted OBJ track road meshes (17 tracks) |

---

## Build Versions Documented

| Build | Date | Key Notes |
|-------|------|----------|
| Beta 1 | Jun 27, 2001 | Loose files, 17 tracks, full debug EXE |
| Oct09 Prototype | Oct 09, 2001 | Offline patch ready, mco_log.txt available |
| Final Retail | Oct 2001 | BIG archives, MCity_d.exe debug symbols |
| Offline Patch | ~Mar 2003 | Content overlay; does not replace engpatch |

**engpatch.viv SHA-1 is identical across all three game builds** (2f88a489a67318338de51de842a6044b8deadbbf) — the engine audio patch archive was never updated by any patch.

---

## What Is NOT Decoded

These are the major gaps — understanding them would unlock significant new work:

1. **BNK audio codec** — blocked. The audio data is encrypted or uses a proprietary codec. No known codec signature found.
2. **Engine curve tables** (engine.ltb, engine.ctb, etc.) — Creative Labs CRD format, sparse storage. Content meaning unknown.
3. **Runtime engpatch selection** — how MDB keys map to BNK files at runtime. Not traced in disassembly.
4. **FRD native mesh** — pointer-linked index data not decoded; road surface triangulation works but is not the original mesh.
5. **CASTANET serialization** — message binary format unknown; no live capture possible.

---

## Source Code Stubs — `src/`

Disassembled and annotated stubs from `MCity_d.exe`:

```
src/
├── npslib/     — NPS network library (CASTANET + core)
├── game/       — Game subsystems (audio, physics, render)
├── mcity/      — MCity core (mcity_core.cpp)
└── authlogin/  — Authentication DLL
```

These are C stubs reconstructed from disassembly, not the original source code.

---

## Contributing

This repo accepts:
- Corrections with supporting evidence
- New format discoveries with hex evidence
- Working tool implementations

This repo does NOT accept:
- Game ROMs or copyrighted assets
- Claims without evidence
- Speculation presented as fact

---

## Related Projects

- [AZMCO](https://github.com/americusmaximus/AZMCO) — Open-source MCO recreation (renderer, physics, audio)
- [Rusty Motors](https://github.com/rustymotors/server) — MCO server recreation attempt
- [EA BIG Archive — rewiki](https://rewiki.miraheze.org/wiki/EA_BIG_Archive)
- [EA SSH FSH Image — rewiki](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))

---

*Last updated: 2026-05-10*
