# BNK — Sound Bank Format

> Audio banks for tracks and game sound effects.

**Magic:** `BNKl` (EA Sound Bank, little-endian)  
**Audio codec:** EA XA ADPCM  
**Typical contents:** Engine sounds, ambient noise, crowd reactions

## Header Structure

| Offset | Size | Value | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `BNKl` | Magic (EA Sound Bank little-endian) |
| 0x04 | 2 | `4` | Version |
| 0x06 | 2 | varies | Platform flags (0x0080 = Win32) |
| 0x08 | 4 | varies | Header size |
| 0x0C | 2 | varies | Number of entries |
| 0x10 | 4 | varies | Offset or ID |
| 0x14 | 4 | varies | Padding |

## Audio Block Structure

Audio is organized in blocks. Each block starts with a marker:

```
[FF 00 00 00] [50 54] [PT header data...] [audio frames...]
     or
[FF 00] [50 54] [PT header data...] [audio frames...]
```

Where:
- `FF 00 00 00` = Sync marker (start of first block)
- `FF 00` = Separator between blocks
- `50 54` = "PT" codec ID

### PT Header (Variable-Length Encoded)

After "PT", the header uses variable-length encoding with subheaders:

| Code | Name | Description |
|------|------|-------------|
| 0x82 | channels | Number of channels (1=mono, 2=stereo) |
| 0x83 | compression | Compression type (0=none, 7=EA ADPCM) |
| 0x84 | sample_rate | Sample rate in Hz |
| 0x85 | num_samples | Number of samples |
| 0x86 | loop_offset | Loop start offset |
| 0x87 | loop_length | Loop length |
| 0x88 | data_start | Data start offset |
| 0x8A | (end) | End of subheader |
| 0xFF | (end) | End of header |

The header ends at the first `0xFF` byte.

### EA XA ADPCM Codec

After the PT header, audio frames follow:

- **Frame size:** 15 bytes (mono) or 30 bytes (stereo)
- **Samples per frame:** 28
- **Compression:** 4-bit ADPCM with predictor

**Decoding algorithm** (from vgmstream):

```python
EA_XA_TABLE = [0, 240, 460, 392, 0, 0, -208, -220, 0, 1, 3, 4, 7, 8, 10, 11, 0, -1, -3, -4]

# For each frame:
frame_info = audio_data[offset]  # First byte
coef_idx = frame_info >> 4
coef1 = EA_XA_TABLE[coef_idx + 0]
coef2 = EA_XA_TABLE[coef_idx + 4]
shift = (frame_info & 0x0F) + 8

# For each sample in frame:
sample_n = (nibble << 28) >> shift
new_sample = sample_n + coef1 * hist1 + coef2 * hist2 + 128
new_sample = clamp(new_sample >> 8, -32768, 32767)
```

## MCO BNK Files

MCO BNK files contain car engine and track audio:

| File | Description | Duration |
|------|-------------|----------|
| track.bnk | Track ambient sounds | Varies by track |
| gencar.bnk | Generic car sounds | ~16-17 seconds |
| induslot.bnk | Industrial slot sounds | Varies |

## Open Questions

- How different RPM/engine sounds are selected during gameplay
- Full block header format (what do the extra fields mean?)
- Exact sample rate encoding in PT header when the header omits an explicit rate