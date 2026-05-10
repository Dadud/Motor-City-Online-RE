# FSH — EA Texture / Image Format

> Textures for cars, tracks, and the user interface.

**Magic:** `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox), `SHPG` (GameCube/Wii)  
**Full name:** EA SSH FSH Image (Type 1)  
**Reference:** [XentaxWiki — EA SSH FSH Image](https://wiki.xentax.spektr.name/index.php/EA_SSH_FSH_Image)

## Header (16 bytes)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | char[4] | Magic: `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox) |
| 0x04 | 4 | uint32 LE | Total file size in bytes |
| 0x08 | 4 | uint32 LE | Number of image entries |
| 0x0C | 4 | char[4] | Format version: `G264`, `GIMX`, `G354`, etc. |

## Directory (8 bytes per entry)

After the 16-byte header, the directory lists all image entries:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | char[4] | Entry tag (e.g., `1001`, `gear`, `xamas`) |
| +0x04 | 4 | uint32 | Absolute offset to entry data (from file start) |

## Entry Data (per texture image)

Each entry starts with a 16-byte header:

```
byte[0]:  record_id (bit 7 = compression flag, lower 7 bits = record type)
bytes[1-3]: 3-byte int24 LE — offset to next binary attachment
uint16[2]:  width
uint16[3]:  height
int16[4]:   center X
int16[5]:   center Y
uint16[6]:  shape flags (referenced, swizzled, transposed)
uint16[7]:  shape Y
```

**Key insight from Xentax wiki:** The compression flag is bit 7 of the record_id byte. So:
- `0x7D` = record type 0x7D, no compression
- `0xFD` = record type 0x7D, WITH RefPack compression
- `0x7E` = record type 0x7E, no compression
- `0xFE` = record type 0x7E, WITH RefPack compression

### Record Type IDs (from Xentax wiki)

| ID (hex) | Format | Description |
|----------|--------|-------------|
| 0x01 | PAL4 | 4-bit palette, RGBA output |
| 0x02 | PAL8 | 8-bit palette, RGBA output |
| 0x04 | RGB888 | 24-bit RGB |
| 0x05 | RGBA8888 | 32-bit RGBA |
| 0x0E | — | 8-bit with custom EA swizzle (PS2) |
| 0x60 | DXT1 | Compressed, 4×4 blocks, 1-bit alpha |
| 0x61 | DXT3 | Compressed, 4×4 blocks, 4-bit alpha |
| 0x62 | DXT5 | Compressed |
| 0x6D | 4444 | 16-bit A4R4G4B4 |
| 0x6E | — | (unknown) |
| 0x7D | **8888** | **32-bit A8R8G8B8** — MCO offline car textures |
| 0x7E | **5551** | **16-bit A1R5G5B5** — Oct09 prototype textures |
| 0x7F | 888 | 24-bit A0R8G8B8 |
| 0x82 | — | RefPack compressed (SSX Tricky) |
| 0x83 | — | RefPack compressed 16-bpp (FIFA 09 PS2) |
| 0xFB | RGB565 | Custom/special (used by some games for raw RGB565) |
| 0xFD | **8888+RefPack** | **MCO Beta 1 / Oct 09 GIMX format** |

## MCO Texture Formats

### GIMX Format (record_id 0xFD) — Beta 1 / Oct 09

MCO's primary texture format for car models and UI elements.

**Key insight:** record_id `0xFD` = record_type `0x7D` (8888 = A8R8G8B8) with compression flag set. All GIMX data uses RefPack compression.

**Decoding workflow:**
1. Read entry header, check record_id
2. If bit 7 set (e.g., 0xFD): RefPack compressed
3. Decompress with RefPack
4. Decode as A8R8G8B8 (4 bytes per pixel, little-endian)

### G264 Format (record_id 0x7D) — Offline Version Car Textures

Used in offline version car VIV textures. Same pixel format as GIMX (A8R8G8B8) but **NOT compressed**.

**Decoding workflow:**
1. Read entry header
2. record_id 0x7D = raw A8R8G8B8 (no compression)
3. Decode directly as A8R8G8B8

### Oct09 Prototype Format (record_id 0xFE) — BGRA5551 + RefPack

Oct09 uses RefPack-compressed BGRA5551.

**Decoding workflow:**
1. Read entry header
2. record_id 0xFE = record_type 0x7E with compression
3. Decompress with RefPack
4. Decode as BGRA5551 (16-bit, 5 bits per channel + 1-bit alpha)

### xamas Format — Raw RGB565

Some games use record_id 0xFB for raw RGB565 textures. In MCO HUD50.fsh, xamas actually uses record_id 0xFD (RefPack + A8R8G8B8).

### RefPack Compression

All MCO compressed textures use EA's RefPack compression (same as QFS files).

**Header format:**
```
flags(1) + 0xFB(1) + compressed_size(3) + decompressed_size(3)
```

**Reference:** `tools/refpack_decompress.py` — pure Python implementation based on Niotso Wiki spec.

## Verified Samples

| File | Entry | Dims | record_id | Compressed | Format | Status |
|------|-------|------|-----------|------------|--------|--------|
| HUD50.fsh (Beta 1) | `xamas` | 44×44 | 0xFD | Yes | 8888 | ✅ Works (RefPack + A8R8G8B8) |
| HUD50.fsh (Beta 1) | `gear` | 88×38 | 0xFD | Yes | 8888 | ✅ Works |
| HUD50.fsh (Beta 1) | `metr` | 20×208 | 0xFD | Yes | 8888 | ✅ Works |
| HUD50.fsh (Beta 1) | `4444` | 256×256 | 0xFD | Yes | 8888 | ✅ Works |
| +partview.fsh (Oct09) | all | 84×80 | 0xFE | Yes | 5551 | ✅ Works |
| 53chevy/dash.fsh | 0000 | 256×256 | 0x7D | No | 8888 | ✅ Works |
| 53chevy/part.fsh | 0000 | 256×256 | 0x7D | No | 8888 | ✅ Works |

## Extraction Tool

`tools/fsh2png_v2.py` extracts all supported FSH formats:

```bash
python3 tools/fsh2png_v2.py HUD50.fsh output/
```

**Supported formats:**
- RefPack + 8888 (GIMX) — Beta 1 / Oct 09
- RefPack + 5551 (BGRA5551) — Oct 09
- Raw 8888 (G264) — Offline version
- Raw RGB565 (xamas)

## Platform Variants

| Platform | Magic | Format |
|----------|-------|--------|
| PC (beta) | `SHPI` | GIMX |
| PC (offline) | `SHPI` | G264 |
| PlayStation 2 | `SHPS` | GIMX |
| Xbox | `SHPX` | — |
| GameCube | `SHPG` | G354 |
