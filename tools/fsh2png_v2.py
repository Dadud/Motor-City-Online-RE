#!/usr/bin/env python3
"""
SHPI texture to PNG converter v2 for Motor City Online.
Handles RefPack decompression and various pixel formats.

Key insight from analysis:
- xamas: raw RGB565 (no RefPack, just coincidental 0x10 0xFB bytes)
- gear/metr: RefPack compressed, decompresses to RGBA (88x38, 20x208)
- 4444: RefPack compressed, decompresses to 262144 bytes (256x256x4)

The decompressed RGBA data may still need format conversion.
"""

import struct
import sys
import os
import zlib

class RefPackDecompressor:
    """RefPack decompression implementation."""
    
    def decompress(self, data: bytes, max_out_size: int = 0) -> tuple:
        if len(data) < 5:
            raise ValueError(f"Data too short: {len(data)} bytes")
        
        inp = memoryview(data)
        outp = bytearray()
        
        signature = (inp[0] << 8) | inp[1]
        pos = 2
        
        has_compressed_size = signature & 0x0100
        large_files = signature & 0x8000
        
        if has_compressed_size:
            if large_files:
                compressed_size = struct.unpack('>I', inp[pos:pos+4])[0]
                pos += 4
            else:
                compressed_size = (inp[pos] << 16) | (inp[pos+1] << 8) | inp[pos+2]
                pos += 3
        
        if large_files:
            decompressed_size = struct.unpack('>I', inp[pos:pos+4])[0]
            pos += 4
        else:
            decompressed_size = (inp[pos] << 16) | (inp[pos+1] << 8) | inp[pos+2]
            pos += 3
        
        if max_out_size > 0 and decompressed_size > max_out_size:
            decompressed_size = max_out_size
        
        out_buf = bytearray(decompressed_size)
        out_pos = 0
        
        while out_pos < decompressed_size:
            if pos >= len(data):
                break
                
            byte0 = inp[pos]
            pos += 1
            
            if not (byte0 & 0x80):
                # 2-byte command
                if pos >= len(data):
                    break
                byte1 = inp[pos]
                pos += 1
                
                proc_len = byte0 & 0x03
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte0 & 0x60) << 3) + byte1 + 1
                ref_len = ((byte0 >> 2) & 0x07) + 3
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            elif not (byte0 & 0x40):
                # 3-byte command
                if pos + 1 >= len(data):
                    break
                byte1 = inp[pos]
                byte2 = inp[pos + 1]
                pos += 2
                
                proc_len = byte1 >> 6
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte1 & 0x3f) << 8) + byte2 + 1
                ref_len = (byte0 & 0x3f) + 4
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            elif not (byte0 & 0x20):
                # 4-byte command
                if pos + 2 >= len(data):
                    break
                byte1 = inp[pos]
                byte2 = inp[pos + 1]
                byte3 = inp[pos + 2]
                pos += 3
                
                proc_len = byte0 & 0x03
                for i in range(proc_len):
                    if out_pos >= decompressed_size:
                        break
                    out_buf[out_pos] = inp[pos]
                    pos += 1
                    out_pos += 1
                
                ref_dis = ((byte0 & 0x10) << 12) + (byte1 << 8) + byte2 + 1
                ref_len = ((byte0 & 0x0c) << 6) + byte3 + 5
                
                for i in range(ref_len):
                    if out_pos >= decompressed_size:
                        break
                    src_pos = out_pos - ref_dis
                    if src_pos < 0:
                        src_pos = 0
                    out_buf[out_pos] = out_buf[src_pos]
                    out_pos += 1
                    
            else:
                # 1-byte command
                proc_len = (byte0 & 0x1f) * 4 + 4
                
                if proc_len <= 0x70:
                    for i in range(proc_len):
                        if out_pos >= decompressed_size:
                            break
                        out_buf[out_pos] = inp[pos]
                        pos += 1
                        out_pos += 1
                else:
                    proc_len = byte0 & 0x03
                    for i in range(proc_len):
                        if out_pos >= decompressed_size:
                            break
                        out_buf[out_pos] = inp[pos]
                        pos += 1
                        out_pos += 1
                    break
        
        return bytes(out_buf[:out_pos]), pos
    
    def decompress_simple(self, data: bytes) -> bytes:
        result, _ = self.decompress(data)
        return result


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
        
        offset_next = ehdr[1] | (ehdr[2] << 8) | (ehdr[3] << 16)
        if offset_next & 0x800000:
            offset_next -= 0x1000000
        
        width = struct.unpack('<H', ehdr[4:6])[0]
        height = struct.unpack('<H', ehdr[6:8])[0]
        
        data_start = off + 16
        next_off = entries[i+1]['offset'] if i+1 < len(entries) else len(data)
        
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
    """Decode RGB565 big-endian data."""
    pixels = []
    for i in range(min(width * height, len(data) // 2)):
        w = data[i*2] | (data[i*2+1] << 8)
        r = ((w >> 11) & 0x1F) * 8
        g = ((w >> 5) & 0x3F) * 4
        b = (w & 0x1F) * 8
        pixels.append((r, g, b, 255))
    return pixels


def decode_argb8888(data, width, height):
    """Decode ARGB8888 data (little-endian uint32)."""
    pixels = []
    for i in range(min(width * height, len(data) // 4)):
        w = struct.unpack('<I', data[i*4:i*4+4])[0]
        a = (w >> 24) & 0xFF
        r = (w >> 16) & 0xFF
        g = (w >> 8) & 0xFF
        b = w & 0xFF
        pixels.append((r, g, b, a))
    return pixels


def decode_abgr8888(data, width, height):
    """Decode ABGR8888 data (little-endian, like BGRA)."""
    pixels = []
    for i in range(min(width * height, len(data) // 4)):
        w = struct.unpack('<I', data[i*4:i*4+4])[0]
        a = (w >> 24) & 0xFF
        b = (w >> 16) & 0xFF
        g = (w >> 8) & 0xFF
        r = w & 0xFF
        pixels.append((r, g, b, a))
    return pixels


def save_ppm(pixels, width, height, outpath):
    """Save as PPM (RGB, no alpha)."""
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


def extract_entry(shpi_data, entry, outdir, dec):
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
    
    total_data = data_end - data_start
    expected_rgb565 = width * height * 2
    expected_rgba = width * height * 4
    
    os.makedirs(outdir, exist_ok=True)
    base_path = os.path.join(outdir, f"{tag}_{width}x{height}")
    
    raw = shpi_data[data_start:data_end]
    
    # Try different decoding approaches
    decoded_pixels = None
    method = None
    
    # Approach 1: Check if data starts with RefPack signature
    if len(raw) >= 2 and raw[0] == 0x10 and raw[1] == 0xFB:
        try:
            dec_data = dec.decompress_simple(raw)
            print(f"  RefPack: compressed {len(raw)} -> {len(dec_data)}")
            
            # Check if decompressed size matches RGBA
            if len(dec_data) == expected_rgba:
                # Try as ARGB8888
                decoded_pixels = decode_argb8888(dec_data, width, height)
                nonzero = sum(1 for p in decoded_pixels if p[0] != 0 or p[1] != 0 or p[2] != 0)
                print(f"  ARGB8888: {nonzero}/{width*height} non-zero pixels")
                if nonzero > 0:
                    method = "RefPack + ARGB8888"
            
            # Also try as ABGR8888
            if decoded_pixels is None or method is None:
                decoded_pixels = decode_abgr8888(dec_data, width, height)
                nonzero = sum(1 for p in decoded_pixels if p[0] != 0 or p[1] != 0 or p[2] != 0)
                print(f"  ABGR8888: {nonzero}/{width*height} non-zero pixels")
                if nonzero > 0:
                    method = "RefPack + ABGR8888"
                    
        except Exception as e:
            print(f"  RefPack failed: {e}")
    
    # Approach 2: Try raw RGB565 (for entries like xamas that aren't actually compressed)
    if decoded_pixels is None and len(raw) >= expected_rgb565:
        decoded_pixels = decode_rgb565_be(raw, width, height)
        nonzero = sum(1 for p in decoded_pixels if p[0] != 0 or p[1] != 0 or p[2] != 0)
        print(f"  RGB565 raw: {nonzero}/{width*height} non-zero pixels")
        if nonzero > width * height * 0.1:  # At least 10% non-zero
            method = "raw RGB565"
    
    # Approach 3: Try with header skip
    if decoded_pixels is None:
        for skip in [96, 80, 64, 48, 32]:
            if len(raw) >= skip + expected_rgb565:
                pixel_data = raw[skip:skip + expected_rgb565]
                decoded_pixels = decode_rgb565_be(pixel_data, width, height)
                nonzero = sum(1 for p in decoded_pixels if p[0] != 0 or p[1] != 0 or p[2] != 0)
                print(f"  RGB565 skip {skip}: {nonzero}/{width*height} non-zero pixels")
                if nonzero > width * height * 0.1:
                    method = f"RGB565 + {skip}B header"
                    break
    
    # Save the result
    if decoded_pixels:
        try:
            # Use PAM for RGBA data, PPM for RGB
            if method and 'ARGB' in method or 'ABGR' in method:
                save_pam(decoded_pixels, width, height, base_path + ".pam")
                print(f"  Extracted: {tag}.pam ({width}x{height}) [{method}]")
            else:
                save_ppm(decoded_pixels, width, height, base_path + ".ppm")
                print(f"  Extracted: {tag}.ppm ({width}x{height}) [{method}]")
        except Exception as e:
            print(f"  Image save failed: {e}")
    else:
        print(f"  Could not decode {tag}")


def main():
    if len(sys.argv) < 2:
        print("Usage: fsh2png_v2.py <input.fsh> [output_dir]")
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
    
    dec = RefPackDecompressor()
    
    for entry in shpi['entries']:
        print(f"\nEntry: {entry['tag']} ({entry['width']}x{entry['height']}, record_id=0x{entry['record_id']:02x})")
        extract_entry(shpi['raw_data'], entry, out_dir, dec)


if __name__ == '__main__':
    main()