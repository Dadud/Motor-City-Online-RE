# BIG / BIGF — EA Archive Format

> Generic archive format used throughout EA's mid-2000s games.

**Magic:** `BIGF` (4 bytes ASCII)  
**Used in:** `cars.big`, `tracks.big`, `audio.big`, `patch.big`, `DB.big`, `GUI.big`, `feArt.big`, `*.viv`  
**Compression:** Stored or Implode (PKWARE)

## Archive Structure

### Header (12 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | Magic | `BIGF` (0x42 0x49 0x47 0x46) |
| 0x04 | 4 | Version | Format version |
| 0x08 | 4 | DataSize | Size of data section after header |

### File Table Entry (16 bytes each)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | Offset | Absolute offset to file data (from archive start) |
| +0x04 | 4 | Size | Uncompressed size of this file |
| +0x08 | 4 | NameLen | Length of filename (including null terminator) |
| +0x0C | 4 | CompSize | Compressed size (0 if uncompressed) |

**Note:** `CompSize = 0` means the file is stored uncompressed. Otherwise:
- `CompSize & 0x02` ≠ 0 → uncompressed (stored)
- `CompSize & 0x08` ≠ 0 → Implode compressed

### Filename

Immediately after the entry header: `NameLen` bytes (ASCII, null-terminated).

### Data Section

File data starts at the offset specified in the entry. If compressed, data follows the Implode format.

## Entry Count

The number of entries is implied by the header size and data section layout. To find all entries, either:
1. Scan the file sequentially for `BIGF` magic (not always at start of file)
2. Use the known table size from the game code
3. Scan for valid filename patterns (`*.fce`, `*.fsh`, etc.)

## Implode Decompression

The `Implode` algorithm (PKWARE) is used for compressed entries. Decompress using the `IMPLODE.DLL` or `COMPRESS.DLL` included with the game.

Algorithm parameters (from MCO code):
- Dictionary size: 4096
-舒
- Compression ratio: 3-byte minimum match

## Per-Car VIV Format

VIV files (`Data/models/*.viv`) are BIGF archives with car-specific contents:

**Example: 53chevy.viv contains:**

| Filename | Size | Description |
|----------|------|-------------|
| `dash.fce` | 268 KB | Dashboard geometry |
| `dash.fsh` | 257 KB | Dashboard texture |
| `part.blf` | 34 KB | Bill of materials (vertex segmentation) |
| `part.fce` | 328 KB | Car body geometry |
| `part.fst` | 502 KB | FST unknown format |
| `part.lod` | 28 bytes | LOD distance thresholds |
| `part.fsh` | — | Car body texture |
| `spoiler.fce` | 12 KB | Spoiler geometry (if present) |

Not all cars have all parts — some have just `part.fce` + `part.fsh`.

## BIG Archives in Final Release

| Archive | Contents |
|---------|----------|
| `cars.big` | All car VIV files |
| `tracks.big` | All track data |
| `audio.big` | Music + SFX banks |
| `patch.big` | Engine patches (engpatch.viv, bam.viv, etc.) |
| `DB.big` | Online.mdb |
| `GUI.big` | UI textures |
| `feArt.big` | Front-end art |
| `text.big` | Localization text |
| `movies.big` | Video files |

## Extraction Tools

Use `big_extract.py` to extract all entries from a BIG archive:

```bash
python3 big_extract.py cars.big output_dir/
```

Or use the [Camconn Gist BIG decoder](https://gist.github.com/camconn/f9cf6ee31103070296f9bec89aa97831) (reference implementation).

## Open Questions

- BIG format version field meaning (multiple variants?)
- Whether there are other compression types beyond Implode
- How the game locates the file table (vs. scanning)
- If there is a hash/index table for fast lookup
