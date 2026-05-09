#!/usr/bin/env python3
"""
BIG archive extractor for Motor City Online.
Handles the BIGF format used by MCO.

Usage: python3 big_extract.py <input.big> <output_dir>
"""
import struct
import sys
import os

def extract_big(filepath, outdir):
    with open(filepath, 'rb') as f:
        data = f.read()
    
    if data[:4] != b'BIGF':
        raise ValueError(f"Not a BIGF file: magic={data[:4]!r}")
    
    # Parse header
    # BIGF header: magic(4) + version(4) + num_entries(4) + toc_offset(4) = 16 bytes
    version = struct.unpack('<I', data[4:8])[0]
    num_entries = struct.unpack('<I', data[8:12])[0]
    toc_offset = struct.unpack('<I', data[12:16])[0]
    
    print(f"BIGF v{version:#x}, {num_entries} entries, TOC at {toc_offset:#x}")
    
    # Read TOC entries
    entries = []
    for i in range(num_entries):
        base = toc_offset + i * 12
        offset = struct.unpack('<I', data[base:base+4])[0]
        size = struct.unpack('<I', data[base+4:base+8])[0]
        name_off = struct.unpack('<I', data[base+8:base+12])[0]
        
        # Read null-terminated string
        end = name_off
        while end < len(data) and data[end] != 0:
            end += 1
        name = data[name_off:end].decode('ascii', errors='replace')
        
        entries.append((name, offset, size))
        print(f"  [{i}] {name}: offset={offset:#x}, size={size}")
    
    # Extract files
    os.makedirs(outdir, exist_ok=True)
    for name, offset, size in entries:
        if size == 0 or offset >= len(data):
            print(f"  Skipping {name}: invalid offset/size")
            continue
        
        outpath = os.path.join(outdir, name)
        with open(outpath, 'wb') as f:
            f.write(data[offset:offset+size])
        print(f"  Extracted: {name} ({size} bytes)")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: big_extract.py <input.big> <output_dir>")
        sys.exit(1)
    extract_big(sys.argv[1], sys.argv[2])
