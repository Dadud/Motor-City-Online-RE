# BNK — Sound Bank Format

> Audio banks for tracks and game sound effects.
> **Status: Unknown** — header structure confirmed; audio codec unidentified.

**Magic:** `BNKl` (EA Sound Bank, little-endian)
**Typical block size:** 488 bytes
**Header size:** 2632 bytes (0x0A48)

---

## Header Structure

Verified across all 16 track.bnk files in the final retail install.

| Offset | Size | Type | Value | Description |
|--------|------|------|-------|-------------|
| 0x00 | 4 | char[4] | `BNKl` | Magic |
| 0x04 | 2 | u16 LE | `4` | Version |
| 0x06 | 2 | u16 LE | `0x007A` | Platform flags |
| 0x08 | 4 | u32 LE | varies | Header size (0x0A48 = 2632) |
| 0x0C | 4 | u32 LE | varies | Format/codec identifier (0x001C2CEC) |
| 0x10 | 4 | u32 LE | varies | Audio bank ID |
| 0x14 | 4 | u32 LE | varies | Block size (488) |
| 0x18 | 8 | - | - | Reserved |

Audio data begins at offset 2632 (0x0A48).

---

## Audio Data — Status: Unknown

**The audio data is NOT any known codec.**

- No EA XA ADPCM sync markers (0xFF 0x00 0x00 0x00 0x50 0x54) — the earlier EA XA ADPCM claim is **incorrect** and is withdrawn
- No MPEG audio frame sync patterns
- No ATRAC3 sync words
- No OGG/Vorbis headers
- No standard WAV/PCM headers

**Evidence of encryption/compression:**
- Block entropy: ~6.3-6.5 bits/byte — consistent with encrypted data
- Format ID field `0x001C2CEC` does not decode to any known FourCC

The audio data appears to use a proprietary EA cipher or undocumented codec from the same era as NFS3/4.

---

## Block Structure

Each block is **488 bytes**. No consistent internal header pattern found — consistent with encrypted content.

---

## What Would Decode It

1. Find the audio decryption key in `MCity_d.exe` disassembly
2. Compare against EA Sports audio format keys from same-era titles (NFS, FIFA)
3. The format ID `0x001C2CEC` is the best lead for codec identification

---

## Tool Status

`tools/bnk2wav.py` does not produce valid audio. Codec must be identified before the tool can be fixed.

---

## See Also

- `docs/formats/engpatch.md` - engine audio system (related)
- `docs/research/network.md` - audio subsystem in NPS
