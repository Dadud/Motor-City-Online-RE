# File Format Documentation

Binary format documentation for Motor City Online's asset files. All formats are reverse-engineered from the final retail build.

## Status Key

| Tag | Meaning |
|-----|---------|
| Verified | Confirmed on multiple samples; working implementation exists |
| Partial | Core structure understood; some fields or variants unknown |
| Unknown | Format/purpose identified; decoding blocked or not started |

## Format Index

### Verified — Working implementations

| Format | File | Description |
|--------|------|-------------|
| BIG / VIV | [BIG.md](BIG.md) | EA archive container (BIG4 and BIGF variants) |
| FCE | [FCE.md](FCE.md) | Car geometry (FCE4M, magic 0x00101015) |
| FSH | [FSH.md](FSH.md) | Texture images (SHPI container) |
| LOD | [LOD.md](LOD.md) | Model LOD distance thresholds |
| INI | [INI.md](INI.md) | Track configuration files |

### Partial — Core structure known; unknowns documented

| Format | File | Description | Unknowns |
|--------|------|-------------|----------|
| FRD | [FRD.md](FRD.md) | Track road geometry | Native mesh index; pointer chain |
| FST | [FST.md](FST.md) | Per-car part metadata | Post-pad bulk record semantics |
| BLF | [BLF.md](BLF.md) | Car vertex segmentation | Full chunk type list |
| MDB | [DATABASE.md](DATABASE.md) | Car/parts database | Physics table field meanings |
| ENGPATCH | [engpatch.md](engpatch.md) | Engine audio archive | Runtime BNK selection |

### Unknown — Decoding blocked

| Format | File | Description | Blocker |
|--------|------|-------------|---------|
| BNK | [BNK.md](BNK.md) | Audio banks | Audio codec unidentified; data appears encrypted |
| TRK | [TRK.md](TRK.md) | AI racing line | Not analyzed |

## Corrections Applied (2026-05-10)

Several format docs previously contained incorrect claims that have been corrected:

- **BNK audio codec** — was "EA XA ADPCM", now correctly marked as **Unknown**. The earlier claim was withdrawn after entropy analysis showed the data is encrypted.
- **FRD native mesh** — was "fully decoded", now correctly marked as **Partial**. Road surface works; native mesh index format is unknown.
- **FST acronym** — "Feature/Settings Table" is an unverified guess; acronym expansion is no longer stated as fact.
- **MDB Part Stats** — previously claimed as "fully decoded"; corrected to **Partial** with physics field meanings noted as unknown.
