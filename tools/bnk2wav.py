#!/usr/bin/env python3
"""
BNK to WAV converter for Motor City Online.
Handles EA XA ADPCM audio codec.

BNK structure:
- Header: BNKl magic + metadata
- Audio blocks with separators (FF 00) or sync markers (FF 00 00 00) + "PT" codec ID
- PT header (variable-length encoded) + EA XA ADPCM frames

Based on vgmstream's ea_xa_decoder.c algorithm and EA format documentation
from MultimediaWiki.
"""

import struct
import sys
import os
import math

# EA XA ADPCM coefficient table from vgmstream
EA_XA_TABLE = [
    0,  240,  460,  392,   # 0-3: positive coefficients
    0,    0, -208, -220,   # 4-7: negative coefficients
    0,    1,    3,    4,   # 8-11: more coefficients
    7,    8,   10,   11,   # 12-15
    0,   -1,   -3,   -4    # 16-19
]

# Default sample rate when not specified in header
DEFAULT_SAMPLE_RATE = 22050


def clamp16(value):
    """Clamp value to 16-bit signed range."""
    if value > 32767:
        return 32767
    if value < -32768:
        return -32768
    return value


def read_pt_bytes(data, offset, count):
    """Read count bytes as big-endian integer."""
    result = 0
    for i in range(count):
        result = (result << 8) | data[offset + i]
    return result


def parse_pt_header(pt_data):
    """Parse EA PT header (variable-length encoded).
    
    Based on MultimediaWiki EA Formats documentation.
    
    PT header contains:
    - 0xFF: end of header
    - 0xFE, 0xFC: skip byte
    - 0xFD: subheader starts (contains codes 0x80-0xFF)
      - 0x82: channels
      - 0x83: compression
      - 0x84: sample rate
      - 0x85: num samples
      - 0x86: loop offset
      - 0x87: loop length
      - 0x88: data start
      - 0x8A: end of subheader
    """
    pos = 0
    result = {}
    
    # Skip initial padding bytes (often 00 00 after "PT")
    while pos < len(pt_data) and pt_data[pos] == 0x00:
        pos += 1
    
    while pos < len(pt_data):
        byte = pt_data[pos]
        pos += 1
        
        if byte == 0xFF:
            # End of header
            break
        elif byte == 0xFE or byte == 0xFC:
            # Skip byte
            continue
        elif byte == 0xFD:
            # Subheader starts
            while pos < len(pt_data):
                sub_byte = pt_data[pos]
                pos += 1
                
                if sub_byte == 0xFF:
                    # End of subheader
                    break
                elif sub_byte == 0x8A:
                    # End of subheader
                    break
                elif sub_byte >= 0x80:
                    # Subheader with code
                    code = sub_byte
                    length = pt_data[pos]
                    pos += 1
                    value = read_pt_bytes(pt_data, pos, length)
                    pos += length
                    
                    # Map known codes
                    if code == 0x82:
                        result['channels'] = value
                    elif code == 0x83:
                        result['compression'] = value
                    elif code == 0x84:
                        result['sample_rate'] = value
                    elif code == 0x85:
                        result['num_samples'] = value
                    elif code == 0x86:
                        result['loop_offset'] = value
                    elif code == 0x87:
                        result['loop_length'] = value
                    elif code == 0x88:
                        result['data_start'] = value
                    elif code == 0x92:
                        result['bytes_per_sample'] = value
                    elif code == 0x80:
                        result['split'] = value
                    elif code == 0xA0:
                        result['split_compression'] = value
                    else:
                        result[f'unknown_{code}'] = value
                else:
                    # Regular byte, skip
                    pass
        else:
            # Regular data byte, skip
            pass
    
    return result


def decode_ea_xa_frame(data, offset, is_stereo=False, channel=0, hist1=0, hist2=0):
    """
    Decode one EA XA ADPCM frame (28 samples).
    
    Args:
        data: bytearray of audio data
        offset: offset to frame start (frame_info byte)
        is_stereo: whether stereo
        channel: channel index for stereo (0=left, 1=right)
        hist1: history sample 1 (for predictor)
        hist2: history sample 2 (for predictor)
    
    Returns:
        tuple: (list of 28 PCM samples, bytes_consumed, new_hist1, new_hist2)
    """
    samples = []
    bytes_consumed = 0
    
    if is_stereo:
        # Stereo: frame size = 0x0f * 2 = 30 bytes
        # Header: 2 bytes (coefs for both channels)
        frame_info = data[offset]
        coef_idx = (frame_info >> 4) if channel == 0 else (frame_info & 0x0F)
        coef1 = EA_XA_TABLE[coef_idx + 0]
        coef2 = EA_XA_TABLE[coef_idx + 4]
        
        frame_info2 = data[offset + 1]
        shift = ((frame_info2 >> 4) if channel == 0 else (frame_info2 & 0x0F)) + 8
        
        # Samples start at offset + 2, interleaved
        for i in range(28):
            byte_off = offset + 2 + (i // 2) * 2 + channel
            if byte_off >= len(data):
                samples.append(0)
                continue
            byte = data[byte_off]
            nibble = (byte >> 4) if (i % 2 == 0) else (byte & 0x0F)
            # Sign extend nibble
            sample = (nibble << 28) >> shift
            samples.append(sample)
        bytes_consumed = 30
        return samples, bytes_consumed, hist1, hist2
    else:
        # Mono: frame size = 15 bytes
        # Header: 1 byte (coefs + shift)
        frame_info = data[offset]
        coef_idx = frame_info >> 4
        coef1 = EA_XA_TABLE[coef_idx + 0]
        coef2 = EA_XA_TABLE[coef_idx + 4]
        shift = (frame_info & 0x0F) + 8
        
        for i in range(28):
            byte_off = offset + 1 + i // 2
            if byte_off >= len(data):
                samples.append(0)
                continue
            byte = data[byte_off]
            nibble = (byte >> 4) if (i % 2 == 0) else (byte & 0x0F)
            # Sign extend to 32-bit
            sample_n = (nibble << 28) >> shift
            # Apply predictor
            new_sample = sample_n + coef1 * hist1 + coef2 * hist2 + 128
            new_sample = clamp16(new_sample >> 8)
            samples.append(new_sample)
            hist2 = hist1
            hist1 = new_sample
        
        bytes_consumed = 15
        return samples, bytes_consumed, hist1, hist2


def find_audio_blocks(data):
    """Find all audio block offsets in BNK data.
    
    Audio blocks start with either:
    - Sync marker: FF 00 00 00 PT (full sync)
    - Separator: FF 00 PT (separator between blocks)
    
    Returns list of (offset, is_sync) tuples.
    """
    blocks = []
    
    i = 0
    while i < len(data) - 6:
        # Check for sync marker
        if data[i] == 0xFF and data[i+1] == 0x00 and data[i+2] == 0x00 and data[i+3] == 0x00:
            if data[i+4:i+6] == b'PT':
                blocks.append((i, True))  # True = sync
                i += 6
                continue
        
        # Check for separator (FF 00 but NOT FF 00 00 00)
        if data[i] == 0xFF and data[i+1] == 0x00:
            if i + 5 < len(data) and data[i+2:i+4] != b'\x00\x00' and data[i+4:i+6] == b'PT':
                blocks.append((i, False))  # False = separator
                i += 4
                continue
        
        i += 1
    
    return blocks


def decode_ea_xa_block(block_data, is_stereo=False):
    """
    Decode an entire EA XA audio block.
    
    Block structure (MCO):
    - Either FF 00 00 00 PT (sync) or FF 00 PT (separator)
    - PT header (variable length, ends at 0xFF)
    - Audio data (EA XA ADPCM frames, 15 bytes each = 28 samples)
    """
    if len(block_data) < 6:
        return []
    
    # Skip marker and "PT"
    if block_data[0:2] == b'\xFF\x00':
        # Separator
        audio_data = block_data[4:]  # Skip "FF 00 PT"
    elif block_data[0:4] == b'\xFF\x00\x00\x00':
        # Sync
        audio_data = block_data[6:]  # Skip "FF 00 00 00 PT"
    else:
        return []
    
    # Find where PT header ends (look for 0xFF followed by non-zero)
    audio_start = 0
    for i in range(len(audio_data) - 1):
        if audio_data[i] == 0xFF:
            # Check if it's end of header or just data
            # End of header is typically followed by audio-like data
            if i + 1 < len(audio_data) and audio_data[i+1] != 0x00:
                # But if it's followed by 00 00, it's padding
                if audio_data[i+1] == 0x00 and i + 3 < len(audio_data) and audio_data[i+2] == 0x00:
                    continue
                audio_start = i + 1
                break
    
    if audio_start == 0:
        # No clear header end found, assume all is audio
        audio_start = 0
    
    # Decode all frames
    all_samples = []
    offset = audio_start
    hist1, hist2 = 0, 0
    
    while offset + 15 <= len(audio_data):
        samples, consumed, hist1, hist2 = decode_ea_xa_frame(
            audio_data, offset, is_stereo, 0, hist1, hist2
        )
        all_samples.extend(samples)
        offset += consumed
    
    return all_samples


def parse_bnk_header(data):
    """Parse BNK file header."""
    if data[0:4] != b'BNKl':
        raise ValueError(f"Invalid BNK magic: {data[0:4]!r}")
    
    version = struct.unpack('<H', data[4:6])[0]
    platform = struct.unpack('<H', data[6:8])[0]
    header_size = struct.unpack('<I', data[8:12])[0]
    
    return {
        'magic': 'BNKl',
        'version': version,
        'platform': platform,
        'header_size': header_size,
    }


def create_wav_header(num_samples, sample_rate=22050, channels=1, bits=16):
    """Create a valid WAV file header."""
    byte_rate = sample_rate * channels * bits // 8
    block_align = channels * bits // 8
    data_size = num_samples * channels * bits // 8
    
    wav = bytearray()
    wav.extend(b'RIFF')
    wav.extend(struct.pack('<I', 36 + data_size))
    wav.extend(b'WAVE')
    wav.extend(b'fmt ')
    wav.extend(struct.pack('<I', 16))  # Subchunk1Size
    wav.extend(struct.pack('<H', 1))   # AudioFormat (PCM)
    wav.extend(struct.pack('<H', channels))
    wav.extend(struct.pack('<I', sample_rate))
    wav.extend(struct.pack('<I', byte_rate))
    wav.extend(struct.pack('<H', block_align))
    wav.extend(struct.pack('<H', bits))
    wav.extend(b'data')
    wav.extend(struct.pack('<I', data_size))
    
    return bytes(wav)


def decode_bnk(bnk_path):
    """Decode BNK file to WAV."""
    with open(bnk_path, 'rb') as f:
        data = bytearray(f.read())
    
    header = parse_bnk_header(data)
    print(f"BNK Header: version={header['version']}, platform=0x{header['platform']:04X}")
    
    # Find audio blocks
    blocks = find_audio_blocks(data)
    print(f"Found {len(blocks)} audio blocks")
    
    if not blocks:
        print("No audio blocks found!")
        return None, 0, 1
    
    all_samples = []
    sample_rate = DEFAULT_SAMPLE_RATE
    
    for i, (block_off, is_sync) in enumerate(blocks):
        block_end = blocks[i+1][0] if i+1 < len(blocks) else len(data)
        block_data = data[block_off:block_end]
        
        # Parse PT header
        pt_data = block_data[6:] if is_sync else block_data[4:]
        pt_header = parse_pt_header(pt_data)
        
        if i == 0:
            print(f"\nPT Header (first block):")
            for k, v in sorted(pt_header.items()):
                print(f"  {k}: {v}")
            
            if 'sample_rate' in pt_header and pt_header['sample_rate'] > 1000:
                sample_rate = pt_header['sample_rate']
                print(f"  -> Sample rate: {sample_rate} Hz")
            else:
                print(f"  -> Default sample rate: {sample_rate} Hz")
        
        is_stereo = (pt_header.get('channels', 1) == 2)
        samples = decode_ea_xa_block(block_data, is_stereo=is_stereo)
        if samples:
            all_samples.extend(samples)
    
    return all_samples, sample_rate, 1


def main():
    if len(sys.argv) < 2:
        print("Usage: bnk2wav.py <input.bnk> [output.wav]")
        print("  Converts EA XA ADPCM audio in BNK files to WAV")
        sys.exit(1)
    
    bnk_path = sys.argv[1]
    wav_path = sys.argv[2] if len(sys.argv) > 2 else os.path.splitext(bnk_path)[0] + '.wav'
    
    print(f"Decoding: {bnk_path}")
    
    result = decode_bnk(bnk_path)
    if result is None:
        sys.exit(1)
    
    samples, sample_rate, channels = result
    
    if samples:
        print(f"\nTotal samples: {len(samples)}")
        print(f"Sample rate: {sample_rate} Hz")
        print(f"Channels: {channels}")
        print(f"Duration: {len(samples) / sample_rate:.2f} seconds")
        
        # Create WAV
        wav_header = create_wav_header(len(samples), sample_rate, channels)
        
        with open(wav_path, 'wb') as f:
            f.write(wav_header)
            for s in samples:
                f.write(struct.pack('<h', s))
        
        print(f"Wrote: {wav_path} ({os.path.getsize(wav_path)} bytes)")
    else:
        print("No audio decoded!")
        sys.exit(1)


if __name__ == '__main__':
    main()