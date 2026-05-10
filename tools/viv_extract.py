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
    """Extract from offline version VIV (BIGF-wrapped).
    
    Car VIVs contain MULTIPLE FCE sections (body, interior, wheels, etc.)
    stored consecutively. Each FCE4M magic marker (0x00101015) starts a new section.
    
    We extract ALL FCE sections found, naming them by their first part name.
    """
    files = {}
    if len(data) <= 0x8c:
        return files
    
    import struct
    magic = b'\x15\x10\x10\x00'
    pos = 0
    fce_index = 0
    
    while True:
        idx = data.find(magic, pos)
        if idx < 0 or idx + 0x60 > len(data):
            break
        
        # Validate: check nv/nt look reasonable
        try:
            nv = struct.unpack('<I', data[idx+12:idx+16])[0]
            nt = struct.unpack('<I', data[idx+8:idx+12])[0]
        except:
            nv = nt = 0
        
        # Find end of this FCE (next FCE magic or end of file)
        next_pos = data.find(magic, idx + 1)
        fce_end = next_pos if next_pos > 0 else len(data)
        
        # Validate: nv and nt should be reasonable (< 1M)
        if nv > 0 and nv < 1000000 and nt > 0 and nt < 2000000:
            # Get part name from FCE header
            part_name = "unknown"
            try:
                name_off = idx + 0x0E28
                name_bytes = data[name_off:name_off+64].rstrip(b'\x00')
                names = name_bytes.decode('ascii', errors='replace').split('\x00')
                names = [n.strip() for n in names if n.strip()]
                if names:
                    # First name is the main part (e.g., ':Hbody', ':F :L :S_DM driver mirror')
                    part_name = names[0].replace(':', '_').replace(' ', '_')
            except:
                pass
            
            fce_data = data[idx:fce_end]
            fname = f"{fce_index:02d}_{part_name}.fce"
            files[fname] = fce_data
            fce_index += 1
        
        pos = idx + 1
    
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
