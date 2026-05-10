# Research Progress

Last updated: 2026-05-10

## Status Key

| Tag | Meaning |
|-----|---------|
| Verified | Confirmed on multiple samples; working implementation |
| Partial | Core structure known; some unknowns remain |
| Unknown | Decoding blocked or not started |

---

## File Formats

| Format | Status | Core Achievement |
|--------|--------|-----------------|
| BIG / VIV | Verified | BIG4 + BIGF variants, DCL compression |
| FCE | Verified | FCE4M, 12B/vertex, 56B/triangle, material flags |
| FSH | Verified | SHPI container, 5 pixel formats, RefPack |
| LOD | Verified | 7-float format, trivial |
| INI | Verified | Standard INI, human-readable |
| FRD | Partial | Road surface decoded; native mesh index unknown |
| FST | Partial | GUID header (`e0134678-c995-d111`...); part.fst (513KB) = actual visible car exterior; not yet decoded |
| BLF | Partial | Structure known; chunk type list partial |
| MDB | Partial | Schema extracted; PartStats fields unknown |
| ENGPATCH | Partial | Archive + MDB keys decoded; runtime selection unknown |
| BNK | Verified | EA XA ADPCM codec; bnk2wav.py works, produces valid WAV |
| TRK | **Unknown** | Not analyzed |

---

## Unknowns — Priority Order

2. **Engine.* CRDl tables** — Creative Labs CRD format; sparse storage; byte meaning unknown
3. **Runtime engpatch selection** — how MDB keys map to BNK files at runtime
4. **FRD native mesh topology** — pointer-linked index data not decoded
5. **CASTANET serialization** — message binary format unknown; live capture impossible
6. **FST car exterior format** — EA proprietary GUID-based; 513KB per car; not reverse-engineered

---

## Builds Documented

| Build | Date | Key Differences |
|-------|------|-----------------|
| Beta 1 | Jun 27, 2001 | Loose files, 17 tracks, no BIG archives |
| Oct09 Prototype | Oct 09, 2001 | Offline patch ready, mco_log.txt available |
| Final Retail | Oct 2001 | BIG archives, MCity_d.exe debug symbols |
| Offline Patch | ~Mar 2003 | Content overlay; does not replace engpatch |

**engpatch.viv SHA-1** is identical across all three game builds:
`2f88a489a67318338de51de842a6044b8deadbbf`

---

## Data Extracted

| Data | Rows | Location |
|------|------|----------|
| Car models | 72 unique | `data/Cars-complete.csv` |
| Brands | 103 | `data/Brand.csv` |
| Stock engines | 26 | `data/StockEngines.csv` |
| Car OBJ meshes | 13 | `data/car_models/` — some cars extracted from wrong FCE section (interior vs body); see docs/research/ASSET_VIEWER_INVESTIGATION.md |
| Track road meshes | 6 extracted (16 in game) | `data/tracks/` |
| Attachment points | 12 | `data/AttachmentPoint.csv` |
| Player types | 5 | `data/PlayerType.csv` |
| Skin types | 10 | `data/SkinType.csv` |
| Driver classes | 3 | `data/DriverClass.csv` |

---

## Network Protocol

| Item | Status |
|------|--------|
| CASTANET error codes (40+) | Verified (EXE strings) |
| Auth server flow | Verified (EXE strings) |
| NPS message types | Partial (donated by Molly) |
| CASTANET serialization | **Unknown** |
| Packet encryption | Unverified (donated doc) |
