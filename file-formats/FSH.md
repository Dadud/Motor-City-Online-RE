# FSH — EA Texture / Image Format

> Textures for cars, tracks, and the user interface.

**Magic:** `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox), `SHPG` (GameCube/Wii)  
**Full name:** EA SSH FSH Image (Type 1)  
**Reference:** [Reverse Engineering Wiki — EA SSH FSH Image (Type 1)](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))

## Header (16 bytes)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | char[4] | Magic: `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox) |
| 0x04 | 4 | uint32 LE | Total file size in bytes |
| 0x08 | 4 | uint32 LE | Number of image entries |
| 0x0C | 4 | char[4] | Format version: `G264`, `GIMX`, `G354`, etc. |
| 0x10 | ... | char[] | Filename (null-terminated), padded to 16 bytes total |
| 0x? | 8 | char[8] | `Buy ERTS` copyright string |

## Directory (8 bytes per entry)

After the 16-byte header, the directory lists all image entries:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | char[4] | Entry tag (e.g., `1001`, `1002`, `Buy `) |
| +0x04 | 4 | uint32 | Absolute offset to entry data (from file start) |

## Entry Data (per texture image)

Each entry starts with a 16-byte header, followed by the image data:

```
byte[0]:  record_id
bytes[1-3]: 3-byte int24 LE — offset to next binary attachment
uint16[2]:  width
uint16[3]:  height
int16[4]:   center X
int16[5]:   center Y
uint16[6]:  shape flags
uint16[7]:  shape Y
```

After the 16-byte header, image data follows. The total data size equals the distance to the next entry's offset.

### Record Type IDs (known)

| ID (dec) | ID (hex) | Format | Notes |
|----------|----------|--------|-------|
| 1 | 0x01 | PAL4_RGBA8888 | 4-bit palette, RGBA output |
| 2 | 0x02 | PAL8_RGBA8888 | 8-bit palette, RGBA output |
| 3 | 0x03 | RGBA5551 | 16-bit, 1-bit alpha |
| 4 | 0x04 | RGB888 | 24-bit RGB |
| 5 | 0x05 | RGBA8888 | 32-bit RGBA |
| 6 | 0x06 | IPU internal tiled 32bpp | PS2-specific |
| 109 | 0x6D | Uncompressed | Track textures |
| 125 | 0x7D | G264 paletted | **Offline version car textures** |
| 126 | 0x7E | (custom) | Oct09 prototype textures |
| 251 | 0xFB | (custom) | NFS Porsche Unleashed |
| 253 | 0xFD | GIMX | **MCO primary format** (car/UI textures) |

## MCO Texture Formats

### GIMX Format (record_id 0xFD) — Beta 1 / Oct 09

This is MCO's primary texture format for car models and UI elements in Beta 1 and Oct 09.

**Structure (observed):**
- N-byte GIMX header (80-96 bytes, varies by entry)
- RGB565 pixel data (2 bytes per pixel, little-endian word order)

The GIMX header appears to be 80 bytes for most entries. Some entries have no header at all.

**Verified samples:**

| File | Entry | Dims | Data bytes | Expected RGB565 | Status |
|------|-------|------|-----------|----------------|--------|
| HUD50.fsh (Beta 1) | `xamas` | 44×44 | 3952 | 3872 | ✅ Works (raw RGB565) |
| HUD50.fsh (Beta 1) | `gear` | 88×38 | 4568 | 6688 | ✅ Works (80B header) |
| HUD50.fsh (Beta 1) | `metr` | 20×208 | 5320 | 8320 | ✅ Works (80B header) |
| HUD50.fsh (Beta 1) | `4444` | 256×256 | 84328 | 131072 | ❌ ratio=0.64 |
| part.shpi (Oct09) | `hc07` | 64×64 | 8208 | 8192 | ❌ Unknown format |
| 53chevy.shpi (offline) | `0000` | 256×256 | 1157758 | 131072 | ❌ G264 format |

The first 4 bytes of GIMX data (`10 FB 00 xx`) are often actual RGB565 pixel data, not a format marker. The header structure is not yet fully decoded.

### G264 Format (record_id 0x7D) — Offline Version

Used in offline version car VIV textures. **NOT YET DECODED.**

Structure (partially understood):
- Index array (428 × 4 = 1712 bytes) at data_start
- 256-entry BGRA palette starting at offset 0x6D8 (for 53chevy.shpi)
- Remaining data is pixel data (possibly compressed or with metadata)

### Oct09 Format (record_id 0x7E)

Used in Oct09 prototype. Different from GIMX. 64×64 dimensions.

### Track Textures (record_id 0x6D)

Used in `track.fsh`. Dimensions: 241×198 pixels. No compression.

## Platform Variants

| Platform | Magic | Format |
|----------|-------|--------|
| PC (beta) | `SHPI` | GIMX |
| PC (offline) | `SHPI` | G264 |
| PlayStation 2 | `SHPS` | GIMX |
| Xbox | `SHPX` | — |
| GameCube | unknown | G354 |

## Extraction

The `fsh2png.py` tool extracts SHPI textures to PPM format:

```bash
python3 tools/fsh2png.py HUD50.fsh extracted/
```

**Status:**
- ✅ GIMX format (record_id 0xFD): xamas, gear, metr decode correctly
- ❌ 4444 (GIMX, 256×256): fails (ratio=0.64, possibly compressed)
- ❌ Oct09 0x7E format: not yet implemented
- ❌ G264 0x7D format: not yet implemented

## Open Questions

- Full GIMX header structure (80-96 bytes — what each byte means)
- How to decode record_id 0x7E (Oct09 prototype)
- How to decode the 256×256 GIMX texture (HUD50.fsh `4444`)
- G264 paletted format (record_id 0x7D) — complete decode
- How texture palettes are stored for PAL4/PAL8 formats
