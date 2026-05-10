# MDB — Access Database (Online.mdb)

> Car statistics, parts, physics parameters, and track data.
> **Status: Partial** — schema extracted; physics table field meanings unknown.

**Format:** Microsoft Access 2000 (Jet DB v3)
**Location:** `Data/DB/Online.mdb`
**Size:** 7,129,088 bytes
**Page size:** 4,096 bytes (confirmed)

---

## Schema — Verified Tables

| Table | Rows | Status | Description |
|-------|------|--------|-------------|
| `AttachmentPoint` | 12 | Verified | Part attachment positions |
| `PlayerType` | 5 | Verified | System, Admin, Player, Deleted, Escrow |
| `SkinType` | 10 | Verified | Tinted, Custom, Cop, Flames, etc. |
| `Brand` | 103 | Verified | GM, Ford, Chrysler brands + IsStock |
| `DriverClass` | 3 | Verified | Street, Pro, Open Class |
| `StockEngines` | 26 | Verified | Stock engine configurations |
| `Cars` | 4,056 | Verified | All car variants (83 unique IDs) |

## Schema — Partial Tables

| Table | Rows | Status | Description |
|-------|------|--------|-------------|
| `AbstractPartType` | 64 | Partial | Part categories; some fields unknown |
| `PartType` | 2 | Partial | Part type definitions |
| `PartStats` | 3,952 | Partial | Per-part physics values; field meanings unknown |
| `AbstractAssembliesList` | 10,516 | Partial | Car assembly relationships |

---

## Cars Table — Fully Extracted

4,056 rows to 83 unique car IDs across 72 base models.

Sample:

| Debug Token | Full Name |
|-------------|-----------|
| `59impala` | 1959 Chevrolet Impala |
| `70CUDA` | 1970 Plymouth AAR Cuda |
| `70hemicu` | 1970 Plymouth Hemi Cuda |
| `70STANG` | 1970 Ford Mustang |
| `69torino` | 1969 Ford Torino Cobra |

Full list in `data/Cars-complete.csv`.

---

## Part Stats Table — Unknown

Schema known (50 double columns + 8 text columns), but field meanings are not documented. The table likely stores power/torque multipliers, weight changes, grip adjustments, and shift point changes per part. Without disassembly, exact field meanings cannot be determined.

---

## Format Notes

- **Page size:** 4,096 bytes (MCO-specific; standard Jet DB uses 2,048)
- **Page types:** 0x00 header, 0x01 data, 0x02 TDEF, 0x03/0x04 index, 0x05 bitmap
- **Total pages:** 1,740

---

## Extracted Data

All CSV exports in `data/`:

| File | Rows | Description |
|------|------|-------------|
| `Cars.csv` | 4,056 | All car variants |
| `Cars-complete.csv` | 83 | Unique cars with names |
| `Brand.csv` | 103 | Manufacturers |
| `StockEngines.csv` | 26 | Engine configs |
| `AttachmentPoint.csv` | 12 | Part positions |
| `PlayerType.csv` | 5 | Account types |
| `SkinType.csv` | 10 | Skin categories |
| `DriverClass.csv` | 3 | Race classes |
| `Parts.csv` | 117 | Part types |

---

## See Also

- `tools/mdb_extract.py` — database parser (small tables work; large tables partial)
- `docs/formats/engpatch.md` — engine patch system (related MDB data)
