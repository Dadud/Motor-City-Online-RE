# File Format Reference

Complete documentation for all binary file formats used in Motor City Online.

## Format List

### 3D & Graphics

| Format | Description | Status |
|---|---|---|
| [FCE](FCE.md) | Car body geometry | ✅ Fully decoded |
| [FSH](FSH.md) | Textures / images | ✅ Decoded |
| [FRD](FRD.md) | Track road geometry | ✅ Fully decoded |
| [FST](FST.md) | Part feature/settings table companion to FCE | ✅ Partially decoded |
| [BLF](BLF.md) | Bill of materials / vertex segmentation | ✅ Partially decoded |
| [LOD](LOD.md) | Level-of-detail distance thresholds | ✅ Decoded |

### Archives

| Format | Description | Status |
|---|---|---|
| [BIG](BIG.md) | EA archive format (.big, .viv) | ✅ Decoded |
| [VIV](VIV.md) | Per-car archive (FCE + FSH bundle) | ✅ Decoded |

### Audio

| Format | Description | Status |
|---|---|---|
| [BNK](BNK.md) | Sound bank (EA XA ADPCM) | ✅ Decoded |
| [ASF](ASF.md) | Music format (EA MPEG-derived) | 🔄 In progress |

### Data

| Format | Description | Status |
|---|---|---|
| [MDB](DATABASE.md) | Access database (car stats) | 🔄 In progress |
| [.ini files](INI.md) | Track config (info, audio, boom) | ✅ Documented |
| [.trk](TRK.md) | Track routing / AI graph | 🔄 In progress |

## Common Patterns

### Strings in Binary Files

Files often embed ASCII strings for debugging, file references, and copyright notices:
- `"dash.fce"` — embedded in VIV to reference parts
- `"Buy ERTS"` — EA's internal EAGL runtime system copyright in FSH headers
- `"CC:\mcity\Game\..."` — build path strings in MCity.exe

### Endianness

- **All integer fields** are **little-endian** (Intel byte order)
- **Some magic bytes** are stored big-endian (e.g., FST magic `0x78 0x46 0x13 0xE0`)
- **Float values** are IEEE 754 single-precision

### Implode Compression

Some files use the `Implode` compression algorithm (also called `PKWARE Implode`). The `IMPLODE.DLL` library in the game directory handles this.

Compression flag in BIGF headers:
- `0x02` = uncompressed (stored)
- `0x08` = implode compressed

Decompression is handled by the `implode_decompress` function in `COMPRESS.DLL`.

## File Identification

```
Magic bytes → Format:
  "BIGF"          → VIV / BIG archive
  "SHPI"          → FSH texture (PC)
  "BNKl"          → BNK sound bank
  0x00101015      → FCE4M 3D model
  0x19EB 0xEEFE   → FRD track geometry (two-magic)
  0x00000B52      → BLF bill of materials
  0xE0134678      → FST unknown format (big-endian magic)
```

## Platform Markers

EA used platform markers in some file headers:
- `"GIMX"` — PlayStation 2 texture variant
- `"G264"` — PC / older console texture
- `"G354"` — GameCube texture
- `"SHP"` + one char: `I`=PS1, `P`=PS2, `X`=Xbox
