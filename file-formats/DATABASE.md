# MDB — Access Database Format

> Car statistics, parts, physics parameters, and track data.

**Format:** Microsoft Access 2000 (Jet DB)  
**Location:** `Data/DB/Online.mdb`  
**Size:** 7,129,088 bytes (~6.96 MiB)  
**Game use:** All car stats, customization parts, physics tuning, track definitions

## Database Status: ✅ PARTIALLY DECODED

The database has been successfully parsed using pure Python binary analysis (no mdb-tools required). Core table extraction works, including full Cars-table variant recovery, but several larger gameplay/physics tables still need deeper field-level decoding.

**Page structure:**
- Page size: 4096 bytes
- Page types: 0x00 (header), 0x01 (data), 0x02 (TDEF/catalog), 0x03/0x04 (index), 0x05 (bitmap)
- Total: 1740 pages

**Extracted tables:**

| Table | Rows | Status |
|-------|------|--------|
| Brand | 78 | ✅ Fully decoded |
| Cars | 4056 | ✅ Fully extracted |
| StockEngines | 283 | ✅ Fully decoded |
| PlayerType | 5 | ✅ Fully decoded |
| SkinType | 10 | ✅ Fully decoded |
| AttachmentPoint | 12 | ✅ Fully decoded |
| DriverClass | 6 | ✅ Fully decoded |
| Parts | 117+ | ✅ Partially decoded |
| AbstractPartType | 15+ | ✅ Partially decoded |

## Extracted Data

### Brand (78 entries)
Parts manufacturers and brands (Edelbrock, Holley, GM, Rochester, etc.)

### Cars (4056 rows, 83 unique car IDs)
Playable car models and trim variants in the online version. Selected base-car examples:

| CarID | Description |
|-------|-------------|
| 57belair | 1957 Chevrolet Bel-Air |
| 57chevy | 1957 Chevrolet Bel-Air |
| 59impala | 1959 Chevrolet Impala |
| 67camaro | 1967 Chevrolet Camaro |
| 69vette | 1969 Chevrolet Corvette |
| 69torino | 1969 Ford Torino Cobra |
| 69roadrunner | 1969 Plymouth Road Runner |
| 69gto | 1969 Pontiac GTO |
| 70mustang | 1970 Ford Mustang |
| 70stang | 1970 Ford Mustang Mach 1 |
| 70cuda | 1970 Plymouth Cuda |
| 70hemicu | 1970 Plymouth Hemi Cuda |
| 71duster | 1971 Plymouth Duster |

The Cars table now has complete variant extraction:
- 83 unique car IDs recovered from 4056 rows
- 72 base car names identified
- Trim variants mapped across `1t`, `2t`, `3t`, plus named special trims

Useful outputs in the repo:
- `Cars.csv` — cleaned playable list
- `Cars-complete.csv` — base + trim column view
- `Cars-variants.csv` — one row per extracted variant
- `Cars-trim-summary.csv` — suffix/variant breakdown

### StockEngines (283 entries)
Engine names including: Turbo-Fire, Hemi, Boss 302, 440 Six Pack, 454 SS, 426 Hemi, etc.

### PlayerType (5 entries)
- 0: System
- 1: Admin
- 2: Player
- 3: Deleted Player
- 4: Escrow

### SkinType (10 entries)
Paint and skin types: Tinted, Custom, Cop, Flames, Scallops, Decals, Rust, Traffic, Bad-ass, Starter

### AttachmentPoint (12 entries)
Car modification attachment points: Left Front, Right Front, Left Rear, Right Rear, Left, Right, A, B, C, Front, Rear, (default)

### DriverClass (6 entries)
Racing classes: Street, Performance, Sports Car, Grand Prix, Hypercar, Tuner

## Extracted CSV Files

All extracted data is available in the repository:
- `Brand.csv` — Parts manufacturers
- `Cars.csv` — Cleaned playable car list
- `Cars-complete.csv` — Base cars with grouped trim columns
- `Cars-variants.csv` — Flat variant list
- `Cars-trim-summary.csv` — Variant suffix summary
- `StockEngines.csv` — Engine names
- `PlayerType.csv` — Player account types
- `SkinType.csv` — Paint/skin categories
- `AttachmentPoint.csv` — Car modification points
- `DriverClass.csv` — Racing classes
- `Parts.csv` — Part descriptions
- `AbstractPartTypes.csv` — Part type categories

## Accessing the Database

The database can be opened with:
- **Microsoft Access** (Windows only)
- **mdb-tools** (Linux: `apt install mdb-tools`)
- **UCanAccess** (Java, cross-platform)

### Linux command-line extraction:

```bash
# List tables
mdb-tables Online.mdb

# Export a table to CSV
mdb-export Online.mdb Cars > Cars.csv
mdb-export Online.mdb Brand > Brand.csv
mdb-export Online.mdb StockEngines > StockEngines.csv
```

## Row Format (Decoded)

Jet DB row structure discovered through binary analysis:
- Row header: 1 byte (value varies)
- Column values stored sequentially
- Variable-length text: null-terminated strings
- Fixed-length fields: 4-byte integers (little-endian)
- Rows stored in descending order on data pages
- Row offset table at end of page header (2 bytes per row)

Example Brand row (at page 38, offset 0x07de):
```
04 01 00 00 00 [BrandID=1] 09 [text_len=9] 46 6f 72 64 [Ford] 0e 0a [abbrev_len=10] 66 6f 72 64 [ford] ...
```

## Known Limitations

- PartStats (3952 rows) and large tables not yet fully decoded
- Car physics parameters (weight, power, torque) not yet decoded as numeric values
- Part compatibility matrix not yet decoded
- Track definitions not yet extracted
- HUD50.FSH entry "4444" (256x256) uses unknown compression (ratio=0.64, 84328 bytes vs expected 131072 for raw RGB565)

## Open Questions

- Full CarPhysics schema with numeric values
- How physics values map to gameplay behavior
- Part compatibility matrix structure
- Track segment data format
- DATABASE schema version and relationships
