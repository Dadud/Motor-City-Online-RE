# FSH — EA Texture / Image Format

> Textures for cars, tracks, and the user interface.

**Magic:** `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox)  
**Full name:** EA SSH FSH Image (Type 1)  
**Reference:** [Reverse Engineering Wiki — EA SSH FSH Image (Type 1)](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))

## Header (16 bytes)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | char[4] | Magic: `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox), `SHPG` (GameCube/Wii) |
| 0x04 | 4 | uint32 LE | Total file size in bytes |
| 0x08 | 4 | uint32 LE | Number of image entries |
| 0x0C | 4 | char[4] | Format version: `G264`, `GIMX`, `G354`, etc. |
| 0x10 | ... | char[] | Filename (null-terminated), padded to 16 bytes total |
| 0x? | 8 | char[8] | `Buy ERTS` copyright string |

**Note:** After `Buy ERTS` padding, the first entry's 16-byte header begins. The directory entries (8 bytes each) are located between the header and the `Buy ERTS` padding.

After the 24-byte header, the directory lists all image entries:

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
uint16[6]:  shape flags (12-bit X pos + swizzle/transpose bits)
uint16[7]:  shape Y (12-bit Y pos + 4-bit mipmap count)
```

After the 16-byte header, image data follows. The total data size (header + image data) equals the distance to the next entry's offset.

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
| 126 | 0x7E | (custom) | Oct09 prototype textures |
| 251 | 0xFB | (custom) | NFS Porsche Unleashed |
| 253 | 0xFD | GIMX | **MCO primary format** (car/UI textures) |

### GIMX Format (record_id 0xFD)

This is MCO's primary texture format for car models and UI elements.

**Structure:**
- 96-byte GIMX header
- RGB565 pixel data (2 bytes per pixel, little-endian word order)

**Verified samples:**

| File | Entry | Dimensions | record_id | Notes |
|------|-------|-----------|-----------|-------|
| HUD50.fsh (Beta 1) | `xmas` | 44×44 | 0xFD | **Confirmed working decode** — 96-byte header + RGB565 |
| HUD50.fsh (Beta 1) | `gear` | 88×38 | 0xFD | Not yet decoded |
| HUD50.fsh (Beta 1) | `metr` | 20×528 | 0xFD | Not yet decoded |
| HUD50.fsh (Beta 1) | `4444` | 256×256 | 0xFD | Not yet decoded |
| part.shpi (Oct09) | `hc07` | 64×64 | 0x7E | Different format (record_id=0x7E) |

The 96-byte GIMX header structure is not yet fully understood. The first 4 bytes are typically `10 FB 00 xx` (similar across all entries), followed by format-specific metadata.

### MCO-Specific Formats

**Car textures (GIMX, record_id 0xFD):**
- Used in HUD textures, car skins, UI elements
- Format: `GIMX` (GameCube/Wii texture port for PC)
- 96-byte GIMX header + RGB565 pixel data
- "Buy ERTS" copyright string present in header

**Car textures (GIMX, record_id 0x7E):**
- Used in Oct09 prototype
- Different structure (not RGB565 with 96-byte header)
- 64×64 dimensions (power of 2)

**Track textures (GIMX):** Used in `track.fsh`
- Dimensions: 241×198 pixels (observed)
- record_id: 109 (0x6D)
- No compression

### Binary Attachments (from spec)

These attachment types follow the image data:

| ID | Type | Description |
|----|------|-------------|
| 0x69 | Metal bin | EAGL64 texture management metadata |
| 0x6F | Comment | Comment string |
| 0x70 | Image name | e.g., `tp01` |
| 0x6B | TPage | Texture page data |
| 0x6E | Palette anim | Palette animation |
| 0x7C | Hot spot | Pixel region / hotspot |

## Platform Variants

| Platform | Magic | Format |
|----------|-------|--------|
| PC | `SHPI` | G264 |
| PlayStation | `SHPP` | — |
| PlayStation 2 | `SHPS` | GIMX |
| Xbox | `SHPX` | — |
| GameCube | unknown | G354 |

## Extraction

FSH → PNG conversion requires:
1. Parsing the SHPI header and directory
2. Reading the per-entry header (16 bytes)
3. Decoding the image data based on record_id
4. Converting to a standard image format (PNG/BMP/TGA)

The [rewiki spec](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1)) has full details on the image decoding algorithms for each record type.

## Open Questions

- Full GIMX 96-byte header structure (what each byte means)
- How to decode 0x7E record_id format (Oct09 prototype)
- How to decode the 88×38, 20×528, 256×256 GIMX textures from HUD50.fsh
- How texture palettes are stored for PAL4/PAL8 formats
- Mipmap storage format
- How swizzled GameCube textures are handled
