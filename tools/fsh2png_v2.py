#!/usr/bin/env python3
"""
SHPI texture extractor for Motor City Online.
Handles RefPack decompression and various pixel formats.

Supported formats:
- record_id 0xFD: RefPack + A8R8G8B8 (MCO Beta 1 / Oct 09 GIMX)
- record_id 0xFE: RefPack + A1R5G5B5 (Oct 09 BGRA5551)
- record_id 0x7D: Raw A8R8G8B8 (offline version G264)
- record_id 0x7E: Raw A1R5G5B5
- record_id 0xFB: Raw RGB565 (xamas entry)

Based on Xentax wiki specification for EA FSH format.
record_id byte: bit 7 = compression flag, lower 7 bits = record type
"""

import struct
import sys
import os
import importlib.util

# Load shared RefPack decompressor
spec = importlib.util.spec_from_file_location("refpack", 
    os.path.join(os.path.dirname(__file__), "refpack_decompress.py"))
refpack_mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(refpack_mod)
RefPackDecompressor = refpack_mod.RefPackDecompressor


# Format definitions (from Xentax wiki)
FORMAT_8888 = 'A8R8G8B8'  # 32-bit
FORMAT_5551 = 'A1R5G5B5'  # 16-bit
FORMAT_RGB565 = 'R5G6B5'  # 16-bit
FORMAT_4444 = 'A4R4G4B4'  # 16-bit


def parse_shpi(filepath):
    """Parse SHPI Type 1 image file."""
    with open(filepath, 'rb') as f:
        data = f.read()
    
    if data[0:4] != b'SHPI':
        raise ValueError(f"Not SHPI: magic={data[0:4]!r}")
    
    total_size = struct.unpack('<I', data[4:8])[0]
    num_entries = struct.unpack('<I', data[8:12])[0]
    format_ver = data[12:16].decode('ascii', errors='replace')
    
    # Parse directory
    entries = []
    for i in range(num_entries):
        base = 16 + i * 8
        tag = data[base:base+4]
        entry_offset = struct.unpack('<I', data[base+4:base+8])[0]
        entries.append({'tag': tag, 'offset': entry_offset})
    
    # Parse entry headers
    parsed = []
    for i, ent in enumerate(entries):
        off = ent['offset']
        if off >= len(data):
            continue
        
        ehdr = data[off:off+16]
        record_id = ehdr[0]
        record_type = record_id & 0x7F
        compressed = (record_id >> 7) & 1
        
        offset_next = ehdr[1] | (ehdr[2] << 8) | (ehdr[3] << 16)
        if offset_next & 0x800000:
            offset_next -= 0x1000000
        
        width = struct.unpack('<H', ehdr[4:6])[0]
        height = struct.unpack('<H', ehdr[6:8])[0]
        
        data_start = off + 16
        next_off = entries[i+1]['offset'] if i+1 < len(entries) else len(data)
        
        parsed.append({
            'tag': ent['tag'].decode('ascii', errors='replace').strip('\x00'),
            'record_id': record_id,
            'record_type': record_type,
            'compressed': compressed,
            'width': width,
            'height': height,
            'data_start': data_start,
            'data_end': next_off,
            'format': format_ver,
        })
    
    return {'data': data, 'format': format_ver, 'entries': parsed}


def decode_argb8888(data, width, height):
    """Decode A8R8G8B8 (little-endian uint32)."""
    pixels = []
    for i in range(min(width * height, len(data) // 4)):
        w = struct.unpack('<I', data[i*4:i*4+4])[0]
        a = (w >> 24) & 0xFF
        r = (w >> 16) & 0xFF
        g = (w >> 8) & 0xFF
        b = w & 0xFF
        pixels.append((r, g, b, a))
    return pixels


def decode_bgra5551(data, width, height):
    """Decode A1R5G5B5 (little-endian uint16)."""
    pixels = []
    for i in range(min(width * height, len(data) // 2)):
        w = struct.unpack('<H', data[i*2:i*2+2])[0]
        a = 255 if (w >> 15) & 1 else 0
        b = ((w >> 10) & 0x1F) * 8
        g = ((w >> 5) & 0x1F) * 8
        r = (w & 0x1F) * 8
        pixels.append((r, g, b, a))
    return pixels


def decode_rgb565(data, width, height):
    """Decode R5G6B5 (big-endian uint16)."""
    pixels = []
    for i in range(min(width * height, len(data) // 2)):
        w = data[i*2] | (data[i*2+1] << 8)
        r = ((w >> 11) & 0x1F) * 8
        g = ((w >> 5) & 0x3F) * 4
        b = (w & 0x1F) * 8
        pixels.append((r, g, b, 255))
    return pixels


def save_ppm(pixels, width, height, outpath):
    """Save as PPM (RGB only, no alpha)."""
    with open(outpath, 'wb') as f:
        f.write(f"P6\n{width} {height}\n255\n".encode())
        for r, g, b, a in pixels:
            f.write(bytes([r, g, b]))


def save_pam(pixels, width, height, outpath):
    """Save as PAM (RGB + alpha)."""
    with open(outpath, 'wb') as f:
        header = f"P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n"
        f.write(header.encode('ascii'))
        for r, g, b, a in pixels:
            f.write(bytes([r, g, b, a]))


def extract_entry(data, entry, outdir, dec):
    """Extract a single SHPI entry."""
    tag = entry['tag']
    width = entry['width']
    height = entry['height']
    record_id = entry['record_id']
    record_type = entry['record_type']
    compressed = entry['compressed']
    data_start = entry['data_start']
    data_end = entry['data_end']
    
    if width == 0 or height == 0:
        print(f"  [SKIP] {tag}: invalid dimensions")
        return
    
    raw = data[data_start:data_end]
    os.makedirs(outdir, exist_ok=True)
    base = os.path.join(outdir, f"{tag}_{width}x{height}")
    
    # Select decoder based on record_type
    # 0x7D = 8888 = A8R8G8B8 (32-bit)
    # 0x7E = 5551 = A1R5G5B5 (16-bit)
    # 0xFB = RGB565 (xamas special case, no compression)
    
    if record_type == 0x7D:
        # A8R8G8B8 format
        expected_size = width * height * 4
        
        if compressed:
            # RefPack compressed
            dec_data = dec.decompress_simple(raw)
            print(f"  [0x7D+RefPack] {len(raw)} -> {len(dec_data)} bytes")
        else:
            # Raw
            dec_data = raw
            print(f"  [0x7D raw] {len(dec_data)} bytes")
        
        pixels = decode_argb8888(dec_data, width, height)
        method = "A8R8G8B8"
        
    elif record_type == 0x7E:
        # A1R5G5B5 format
        expected_size = width * height * 2
        
        if compressed:
            dec_data = dec.decompress_simple(raw)
            print(f"  [0x7E+RefPack] {len(raw)} -> {len(dec_data)} bytes")
        else:
            dec_data = raw
            print(f"  [0x7E raw] {len(dec_data)} bytes")
        
        pixels = decode_bgra5551(dec_data, width, height)
        method = "A1R5G5B5"
        
    elif record_type == 0xFB:
        # RGB565 format (raw, no compression)
        dec_data = raw
        print(f"  [0xFB RGB565] {len(dec_data)} bytes")
        pixels = decode_rgb565(dec_data, width, height)
        method = "R5G6B5"
        
    else:
        print(f"  [UNKNOWN] record_type=0x{record_type:02X}, record_id=0x{record_id:02X}")
        return
    
    # Check content
    nonzero = sum(1 for p in pixels if p[0] != 0 or p[1] != 0 or p[2] != 0)
    print(f"  [{method}] {nonzero}/{width*height} non-zero pixels")
    
    # Save
    save_pam(pixels, width, height, base + ".pam")
    print(f"  [OK] -> {tag}.pam")


def main():
    if len(sys.argv) < 2:
        print("Usage: fsh2png_v2.py <input.fsh> [output_dir]")
        sys.exit(1)
    
    shpi_path = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else shpi_path + "_extracted"
    
    print(f"Parsing: {shpi_path}")
    shpi = parse_shpi(shpi_path)
    print(f"Format: {shpi['format']}, Entries: {len(shpi['entries'])}")
    
    dec = RefPackDecompressor()
    
    for entry in shpi['entries']:
        print(f"\nEntry: {entry['tag']} ({entry['width']}x{entry['height']}, "
              f"record_id=0x{entry['record_id']:02X}, type=0x{entry['record_type']:02X}, "
              f"comp={entry['compressed']})")
        extract_entry(shpi['data'], entry, out_dir, dec)


if __name__ == '__main__':
    main()
