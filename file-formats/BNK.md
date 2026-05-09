# BNK — Sound Bank Format

> Audio banks for tracks and game sound effects.

**Magic:** `BNKl` (EA Sound Bank, little-endian)  
**Audio codec:** EA XA ADPCM  
**Typical contents:** Engine sounds, ambient noise, crowd reactions

## Header Structure

| Offset | Size | Value | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `BNKl` | Magic (EA Sound Bank little-endian) |
| 0x04 | 2 | `4` | Version? |
| 0x06 | 2 | `0x0080` | Platform flags (0x80 = Win32) |
| 0x08 | 4 | `32` | Header size (0x20) |
| 0x0C | 2 | `13` | Number of audio blocks? |
| 0x10 | 4 | `0x0021cefb` | Offset or ID |
| 0x14 | 4 | `0xFFFFFFFF` | Padding |
| 0x18 | 2 | `2` | Block count |
| 0x1C | 4 | `768` | Block size (0x0300) |

## Audio Data

Audio data starts at offset `0x0D20` (3360 bytes).

### EA XA ADPCM Codec

Each audio block contains EA XA ADPCM-compressed audio frames:

```
[Marker: 0xFF 0x00 0x00 0x00]
[Codec ID: "PT" (0x54 0x50) — EA XA audio]
[Sample rate: 44100 Hz encoded as uint32]
[Channel/bits info]
[Audio frames... (512 bytes per frame)]
```

### Decoding

EA XA ADPCM is a modified version of the CD-ROM XA ADPCM format:
- 4-bit ADPCM samples
- 18-byte mono / 36-byte stereo frame structure
- 512 bytes per frame ≈ 1024 (mono) or 512 (stereo) samples

To decode:
1. Strip the frame header (18 bytes for mono, 36 for stereo)
2. Decode 4-bit ADPCM → 16-bit PCM
3. Apply optional interleave for stereo

Tools needed: `bnk2wav.py` or similar EA XA ADPCM decoder.

## Audio Bank Contents (track.bnk example)

Track banks contain multiple sound effects:
- Engine loop sounds (different RPM levels)
- Ambient crowd noise
- Tire screech
- Crash/collision sounds
- Environmental audio (wind, announcer)

The `audio.ini` file in each track directory maps these sounds to events.

## Open Questions

- Exact frame header structure (18 vs 36 byte variants)
- How different RPM/engine sounds are selected during gameplay
- Whether BNK files embed WAV headers or are pure stream data
- Full block header format (what do the extra fields mean?)
