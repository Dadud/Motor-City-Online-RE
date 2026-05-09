#!/usr/bin/env python3
"""
SHPI texture to PPM/PAM converter for Motor City Online.
Implements EA SSH FSH Image (Type 1) format.

GIMX Format (record_id 0xFD): 
  - 96-byte GIMX header before pixel data
  - Pixel data is RGB565 (2 bytes per pixel, big-endian or swapped)
  - Some entries may have palettes

Usage: python3 fsh2png.py <input.fsh> [output_dir]
"""
import struct
import sys
import os

def parse_shpi_type1(filepath):
    """Parse SHPI Type 1 image file."""
    with open(filepath, 'rb') as f:
        data = f.read()
    
    if data[0:4] != b'SHPI':
        raise ValueError(f"Not SHPI: magic={data[0:4]!r}")
    
    total_size = struct.unpack('<I', data[4:8])[0]
    num_entries = struct.unpack('<I', data[8:12])[0]
    format_ver = data[12:16].decode('ascii', errors='replace')
    
    entries = []
    for i in range(num_entries):
        base = 16 + i * 8
        tag = data[base:base+4]
        entry_offset = struct.unpack('<I', data[base+4:base+8])[0]
        entries.append({'tag': tag, 'offset': entry_offset})
    
    parsed_entries = []
    for i, ent in enumerate(entries):
        off = ent['offset']
        if off >= len(data):
            continue
        
        ehdr = data[off:off+16]
        record_id = ehdr[0]
        
        # int24 LE at bytes 1-3
        offset_next = ehdr[1] | (ehdr[2] << 8) | (ehdr[3] << 16)
        if offset_next & 0x800000:
            offset_next -= 0x1000000
        
        width = struct.unpack('<H', ehdr[4:6])[0]
        height = struct.unpack('<H', ehdr[6:8])[0]
        
        data_start = off + 16
        next_off = entries[i+1]['offset'] + 16 if i+1 < len(entries) else len(data)
        
        parsed_entries.append({
            'tag': ent['tag'].decode('ascii', errors='replace').strip('\x00'),
            'record_id': record_id,
            'offset_next': offset_next,
            'width': width,
            'height': height,
            'data_start': data_start,
            'data_end': next_off,
            'format': format_ver,
        })
    
    return {
        'total_size': total_size,
        'num_entries': num_entries,
        'format': format_ver,
        'entries': parsed_entries,
        'raw_data': data,
    }

def decode_rgb565_be(data, width, height):
    """Decode RGB565 big-endian data (MCO byte order)."""
    pixels = []
    for i in range(min(width * height, len(data) // 2)):
        w = data[i*2] | (data[i*2+1] << 8)  # little-endian read from file
        r = ((w >> 11) & 0x1F) * 8
        g = ((w >> 5) & 0x3F) * 4
        b = (w & 0x1F) * 8
        pixels.append((r, g, b, 255))
    return pixels

def save_ppm(pixels, width, height, outpath):
    """Save as PPM (P6 binary format)."""
    with open(outpath, 'wb') as f:
        f.write(f"P6\n{width} {height}\n255\n".encode())
        for r, g, b, a in pixels:
            f.write(bytes([r, g, b]))

def save_pam(pixels, width, height, outpath):
    """Save as PAM (RGB_ALPHA)."""
    with open(outpath, 'wb') as f:
        header = f"P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n"
        f.write(header.encode('ascii'))
        for r, g, b, a in pixels:
            f.write(bytes([r, g, b, a]))

def extract_entry(shpi_data, entry, outdir):
    """Extract a single SHPI entry as image."""
    tag = entry['tag']
    width = entry['width']
    height = entry['height']
    record_id = entry['record_id']
    data_start = entry['data_start']
    data_end = entry['data_end']
    
    if width == 0 or height == 0:
        print(f"  Skipping {tag}: invalid dimensions")
        return
    
    raw = shpi_data[data_start:data_end]
    
    pixels = None
    success = False
    
    if record_id == 0xFD:  # GIMX format
        # GIMX has 96-byte header, then RGB565 pixel data
        gimx_header_size = 96
        if len(raw) > gimx_header_size:
            pixel_data = raw[gimx_header_size:]
            expected = width * height * 2
            if len(pixel_data) == expected:
                pixels = decode_rgb565_be(pixel_data, width, height)
                success = True
            else:
                # Try other formats
                # Try no header
                if len(raw) == width * height * 2:
                    pixels = decode_rgb565_be(raw, width, height)
                    success = True
                # Try 8-byte GIMX header
                elif len(raw) > 8 and len(raw) - 8 == width * height * 2:
                    pixels = decode_rgb565_be(raw[8:], width, height)
                    success = True
                # Try without any header (as big-endian)
                elif len(raw) >= width * height * 2:
                    pixel_data = raw[:width * height * 2]
                    # Try big-endian RGB565
                    for i in range(min(width * height, len(pixel_data) // 2)):
                        w = (pixel_data[i*2] << 8) | pixel_data[i*2+1]
                        r = ((w >> 11) & 0x1F) * 8
                        g = ((w >> 5) & 0x3F) * 4
                        b = (w & 0x1F) * 8
                        pixels.append((r, g, b, 255))
                    if pixels:
                        success = True
        else:
            # Data too small for 96-byte header
            if len(raw) == width * height * 2:
                pixels = decode_rgb565_be(raw, width, height)
                success = True
    
    elif record_id in (0xFB,):  # Other Type 1 formats
        if len(raw) >= width * height * 2:
            pixel_data = raw[:width * height * 2]
            pixels = decode_rgb565_be(pixel_data, width, height)
            success = True
    
    # Try generic RGB565 if nothing worked
    if not pixels and len(raw) >= width * height * 2:
        pixel_data = raw[:width * height * 2]
        pixels = decode_rgb565_be(pixel_data, width, height)
        success = True
    
    if not pixels:
        print(f"  Could not decode {tag} ({len(raw)} bytes, {width}x{height})")
        return
    
    os.makedirs(outdir, exist_ok=True)
    base_path = os.path.join(outdir, f"{tag}_{width}x{height}")
    
    try:
        save_ppm(pixels, width, height, base_path + ".ppm")
        print(f"  Extracted: {tag}.ppm ({width}x{height})")
    except Exception as e:
        print(f"  PPM save failed: {e}")
        save_pam(pixels, width, height, base_path + ".pam")
        print(f"  Extracted: {tag}.pam ({width}x{height})")

def main():
    if len(sys.argv) < 2:
        print("Usage: fsh2png.py <input.fsh> [output_dir]")
        sys.exit(1)
    
    shpi_path = sys.argv[1]
    out_dir = sys.argv[2] if len(sys.argv) > 2 else shpi_path + "_extracted"
    
    print(f"Parsing SHPI: {shpi_path}")
    try:
        shpi = parse_shpi_type1(shpi_path)
    except Exception as e:
        print(f"Parse error: {e}")
        sys.exit(1)
    
    print(f"Format: {shpi['format']}, Entries: {shpi['num_entries']}")
    os.makedirs(out_dir, exist_ok=True)
    
    for entry in shpi['entries']:
        print(f"\nEntry: {entry['tag']} ({entry['width']}x{entry['height']}, record_id=0x{entry['record_id']:02x})")
        extract_entry(shpi['raw_data'], entry, out_dir)

if __name__ == '__main__':
    main()
