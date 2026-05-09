#!/usr/bin/env python3
"""
VIV extractor for Motor City Online car archives.

Two VIV variants exist:

1. Standard VIV (Beta 1 / Oct 09 style):
   Direct concatenation of FCE + FSH data. FCE starts at byte 0.
   Format: FCE4 + FCE data... [+ EIMA/HRDR texture data]

2. Offline version VIV (BIGF variant):
   Archive-wrapped format. BIGF header at byte 0.
   FCE geometry starts at offset 0x8c (140 bytes into file).
   Textures may be embedded SHPI (GIMX console texture format).

Usage: python3 viv_extract.py <input.viv> <output_dir>
"""
import struct
import sys
import os

def extract_standard_viv(data):
    """Extract from standard VIV (Beta 1 / Oct 09 style)."""
    files = {}
    fce_pos = data.find(b'FCE4')
    if fce_pos >= 0:
        files['part.fce'] = data[fce_pos:]
    eima_pos = data.find(b'EIMA')
    if eima_pos >= 0:
        files['part.fsh'] = data[eima_pos:]
    hrdr_pos = data.find(b'HRDR')
    if hrdr_pos >= 0:
        files['part.fsh'] = data[hrdr_pos:]
    return files

def extract_offline_viv(data):
    """Extract from offline version VIV (BIGF-wrapped). FCE at 0x8c."""
    files = {}
    if len(data) <= 0x8c:
        return files
    # FCE4M version marker at offset 0x8c
    fce_pos = data.find(b'\x15\x10\x10\x00')
    if fce_pos >= 0:
        files['part.fce'] = data[fce_pos:]
    # SHPI texture (GIMX / GameCube format)
    shpi_pos = data.find(b'SHPI')
    if shpi_pos >= 0:
        files['part.shpi'] = data[shpi_pos:]
    return files

def viv_extract(viv_path, out_dir):
    os.makedirs(out_dir, exist_ok=True)
    with open(viv_path, 'rb') as f:
        data = f.read()
    magic = data[0:4] if len(data) >= 4 else b''
    files = extract_offline_viv(data) if magic == b'BIGF' else extract_standard_viv(data)
    print(f'Extracted {len(files)} files from {os.path.basename(viv_path)}')
    for name, content in files.items():
        out_path = os.path.join(out_dir, name)
        with open(out_path, 'wb') as f:
            f.write(content)
        print(f'  -> {name} ({len(content)} bytes)')
    return files

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('Usage: viv_extract.py <input.viv> <output_dir>')
        sys.exit(1)
    viv_extract(sys.argv[1], sys.argv[2])
