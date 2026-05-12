# Archive Progress

Last updated: 2026-05-12

## File Formats Documented

| Format | Status | Notes |
|--------|--------|-------|
| BIG / VIV | Documented | BIG4 + BIGF variants, DCL compression |
| FSH | Documented | SHPI container, 5 pixel formats, RefPack |
| INI | Documented | Standard INI, human-readable |
| BNK | Partial | Header parsed; audio codec unidentified |
| FRD | Partial | Road surface documented; native mesh unknown |
| BLF | Partial | Structure known; chunk list partial |
| MDB | Partial | Schema extracted; some fields unknown |
| TRK | Unknown | Not analyzed |

## Data Preserved

| Data | Rows | Location |
|------|------|----------|
| Car database | 72 unique | `data/Cars-complete.csv` |
| Brands | 103 | `data/Brand.csv` |
| Stock engines | 26 | `data/StockEngines.csv` |
| Attachment points | 12 | `data/AttachmentPoint.csv` |
| Player types | 5 | `data/PlayerType.csv` |
| Skin types | 10 | `data/SkinType.csv` |
| Driver classes | 3 | `data/DriverClass.csv` |

## Media Archived

| Source | Items | Location |
|--------|-------|----------|
| Old-Games.RU | 47 screenshots | `docs/media/11-old-games-ru-screenshots/` |
| Hidden Palace | 2 screenshots | `docs/media/08-hidden-palace-prototypes/` |
| GameSpot | 2 thumbnails | `docs/media/09-gamespot-preview-2001/` |
| Box art | 1 | `docs/media/box-art.jpg` |
| Articles | **42** text captures | `docs/media/INDEX.md` |

## Builds Documented

| Build | Date | Key Notes |
|-------|------|----------|
| Beta 1 | Jun 27, 2001 | Loose files, 17 tracks, no BIG archives |
| Oct09 Prototype | Oct 09, 2001 | Offline patch ready, mco_log.txt available |
| Final Retail | Oct 2001 | BIG archives, MCity_d.exe debug symbols |
| Offline Patch | ~Mar 2003 | Content overlay; does not replace engpatch |

**engpatch.viv SHA-1** is identical across all three game builds:
`2f88a489a67318338de51de842a6044b8deadbbf`

## Network Protocol

| Item | Status |
|------|--------|
| CASTANET error codes (40+) | Documented (from EXE strings) |
| Auth server flow | Documented (from EXE strings) |
| NPS message types | Partial (donated documentation) |
| CASTANET serialization | Unknown |
| Packet encryption | Unverified (donated doc) |

## External Resources

- [mcos](https://github.com/drazisil-codecov/mcos) — Open-source server recreation (TypeScript/Node.js)
- [AZMCO](https://github.com/americusmaximus/AZMCO) — Open-source game recreation
- CGW Issue 210 PDF — MCO preview on page 36 (cgwmuseum.org)
- PC Gamer Issue 87 OCR — E3 2001 coverage (archive.org)
- PC Gamer Issue 79 OCR — Early preview (archive.org)
- PC Accelerator Final Issue (Jun 2000) — Early preview as NFS: Motor City (archive.org)
- SEC Filings (10-K FY2003, 10-Qs 2002–2003) — EA.com restructuring documentation
