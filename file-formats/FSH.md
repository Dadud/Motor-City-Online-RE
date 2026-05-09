# FSH — EA Texture / Image Format

> Textures for cars, tracks, and the user interface.

**Magic:** `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox)  
**Full name:** EA SSH FSH Image (Type 1)  
**Reference:** [Reverse Engineering Wiki — EA SSH FSH Image (Type 1)](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))

## Header (24 bytes)

| Offset | Size | Type | Description |
|--------|------|------|-------------|
| 0x00 | 4 | char[4] | Magic: `SHPI` (PC), `SHPP` (PS1), `SHPS` (PS2), `SHPX` (Xbox) |
| 0x04 | 4 | uint32 | Total file size in bytes |
| 0x08 | 4 | uint32 | Number of image entries |
| 0x0C | 4 | char[4] | Format version: `G264` (PC/older), `GIMX` (PS2), `G354` (GameCube) |
| 0x10 | 8 | char[8] | GIMX signature (usually `1000H\0\0\0` for PS2) |

## Directory (8 bytes per entry)

After the 24-byte header, the directory lists all image entries:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | char[4] | Entry tag (e.g., `1001`, `1002`, `Buy `) |
| +0x04 | 4 | uint32 | Absolute offset to entry data (from file start) |

## Entry Data (per texture image)

Each entry starts with a 16-byte header, followed by the image data:

```
byte[0]:  record_id (7 bits) + compression_flag (1 bit)
bytes[1-3]: 3-byte int24 — relative offset to next binary attachment
uint16[2]:  width
uint16[3]:  height
int16[4]:   center X
int16[5]:   center Y
uint16[6]:  shape flags (12-bit X pos + swizzle/transpose bits)
uint16[7]:  shape Y (12-bit Y pos + 4-bit mipmap count)
```

### Record Type IDs

| ID | Format | Description |
|----|--------|-------------|
| 1 | PAL4_RGBA8888 | 4-bit palette, 32-bit RGBA output |
| 2 | PAL8_RGBA8888 | 8-bit palette, 32-bit RGBA output |
| 3 | RGBA5551 | 16-bit, 1-bit alpha |
| 4 | RGB888 | 24-bit RGB |
| 5 | RGBA8888 | 32-bit RGBA |
| 6 | IPU internal tiled 32bpp | PS2-specific |
| 109 (0x6D) | Uncompressed | Used in track textures |

### MCO-Specific Formats

**Track textures (GIMX):** Used in `track.fsh`
- Dimensions: 241×198 pixels
- record_id: 109 (0x6D)
- No compression
- Format: `GIMX`

**Car textures (G264):** Used in VIV-embedded texture data
- Format: `G264`
- Found inside BIGF-wrapped VIV files (offline version)
- "Buy ERTS" copyright string embedded in header

**Car skin textures (GIMX):** Used in loose `Data/Skins/*.fsh` files (offline version)
- Format: `GIMX` (GameCube texture port for PC)
- "Buy ERTS" copyright string also present

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

- Full list of record type IDs and their decoding algorithms
- How "Buy ERTS" string relates to the texture data
- How texture palettes are stored for PAL4/PAL8 formats
- Mipmap storage format
