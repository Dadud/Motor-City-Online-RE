# FSH Texture Decoder Research

## Status: Partially Decoded

Working:
- GIMX format (record_id 0xFD): RGB565 with variable header (0B for xamas, 80B for gear/metr)
- Basic SHPI parsing

Not Working:
- **4444 entry** (record_id 0x7D, compression=1): RefPack decompresses to 262144 bytes but data looks wrong
- **Oct09 0x7E** format: Unknown
- **G264 format** in offline car textures: Palettized, structure partially known

## Research Findings

### From Rewiki (EA SSH FSH Image Type 1)

FSH entry header format:
```
1 byte: 7-bit record_id + 1-bit compression_flag
3 bytes (int24): offset to next binary attachment
2 bytes: width
2 bytes: height
2 bytes: center X
2 bytes: center Y
2 bytes: shape X (12-bit pos + flags)
2 bytes: shape Y (12-bit pos + 4-bit mipmap count)
```

### From Xentax Wiki - Entry Types

| ID | Format | Notes |
|----|--------|-------|
| 0x7D | 8888 | 32-bit A8R8G8B8 (SimCity 4, etc.) |
| 0x7E | 5551 | 16-bit A1R5G5B5 |
| 0x82 | refpack | RefPack compressed (no palette) |
| 0x83 | refpack | RefPack compressed 16-bpp |

### From Niotso Wiki - RefPack Compression

RefPack is EA's LZ77-variant compression:
- Header: flags(1) + 0xFB(1) + [compressed_size(3)] + decompressed_size(3)
- Flags: L=large files, U=unknown, C=compressed size present
- Commands: 1-4 byte opcodes with length-distance pairs

Reference implementation: `RefPack.cpp` by Frank Barchard (original author)

## HUD50.FSH 4444 Entry Analysis

```
Entry: 4444, Size: 256x256
Record ID: 0x7D (compression=1)
Data size: 84328 bytes
First bytes: 10 fb 04 00 00 e0 40 87 00 ff...

RefPack header:
  flags = 0x10 (C=0, L=0)
  magic = 0xFB
  decompressed_size = 0x040000 = 262144 (256*256*4)

Decompression result:
  Produces 262144 bytes
  But data mostly zeros with sparse values
  Doesn't look like valid 32-bit ARGB image
```

## Questions

1. Is record_id 0x7D in GIMX format the same as SimCity 4's 8888 format?
2. Does GIMX use different byte ordering for 32-bit formats?
3. Is there an additional header inside the RefPack-compressed data?

## Tools Available

- `refpack_decompress.py` - Python RefPack decompressor (based on Niotso spec)
- `fsh2png.py` - FSH extractor (works for 0xFD entries)

## Next Steps

1. Find GIMX-specific format documentation
2. Try implementing RefPack in C and comparing output
3. Look at actual GIMX texture files from GameCube games for reference
4. Check if the "4444" texture works in-game to confirm it's valid
