#!/usr/bin/env python3
"""Extract files from a raw ISO 9660 image without requiring root/loopback."""
import struct, os, sys

iso_path = sys.argv[1] if len(sys.argv) > 1 else '/home/dadud/.openclaw/agents/retro-game-re/workspace/mco-files/final/MCity/MCity.iso'
out_dir = sys.argv[2] if len(sys.argv) > 2 else '/home/dadud/.openclaw/agents/retro-game-re/workspace/mco-files/final/MCity/extracted'
SECTOR_SIZE = 2048

def parse_name(entry):
    """Parse ISO 9660 file name, strip version ;N suffix."""
    name_len = entry[32]
    name = entry[33:33+name_len].decode('latin-1', errors='replace')
    # Strip ;N version suffix (ISO 9660)
    if ';' in name:
        name = name[:name.index(';')]
    return name

def is_dot_entry(name_bytes):
    """Check if entry name is just NUL (. or .. in ISO 9660)."""
    return len(name_bytes) == 1 and name_bytes[0] == 0 or name_bytes == b'\x01'

with open(iso_path, 'rb') as f:
    # Get root directory info from PVD
    f.seek(16 * SECTOR_SIZE + 158)
    root_lba = struct.unpack('<I', f.read(4))[0]
    f.seek(16 * SECTOR_SIZE + 166)
    root_size = struct.unpack('<I', f.read(4))[0]
    
    print(f'Root LBA={root_lba}, size={root_size}')
    
    def list_dir(f, dir_lba, dir_size):
        """List directory entries, returning (name, lba, size, dr_len)."""
        f.seek(dir_lba * SECTOR_SIZE)
        data = f.read((dir_size // SECTOR_SIZE + 2) * SECTOR_SIZE)
        entries = []
        pos = 0
        while pos < dir_size:
            dr_len = data[pos]
            if dr_len == 0:
                pos += 1
                continue
            if pos + dr_len > len(data):
                break
            entry = data[pos:pos+dr_len]
            if len(entry) < 33:
                break
            
            name_bytes = bytes(entry[33:33+entry[32]])
            if is_dot_entry(name_bytes):
                pos += dr_len
                continue
            
            lba = struct.unpack('<I', entry[2:6])[0]
            size = struct.unpack('<I', entry[10:14])[0]
            name = parse_name(entry)
            
            if name and name not in ('.', '..'):
                entries.append((name, lba, size, dr_len))
            pos += dr_len
        return entries
    
    def extract_file(f, lba, size, out_path):
        if lba < 24 or size == 0:
            return
        os.makedirs(os.path.dirname(out_path) if os.path.dirname(out_path) else out_dir, exist_ok=True)
        f.seek(lba * SECTOR_SIZE)
        remaining = size
        with open(out_path, 'wb') as out:
            while remaining > 0:
                chunk = min(remaining, SECTOR_SIZE)
                d = f.read(chunk)
                if not d:
                    break
                out.write(d)
                remaining -= chunk
    
    def extract_dir_recursive(f, dir_lba, dir_size, base_out):
        entries = list_dir(f, dir_lba, dir_size)
        for name, lba, size, dr_len in entries:
            out_path = os.path.join(base_out, name)
            if size == 2048 and lba != root_lba:  # Directory
                os.makedirs(out_path, exist_ok=True)
                print(f'  DIR: {name}/')
                extract_dir_recursive(f, lba, size, out_path)
            elif size > 0:
                print(f'  FILE: {name} ({size} bytes)')
                extract_file(f, lba, size, out_path)
    
    # List and extract root
    root_entries = list_dir(f, root_lba, root_size)
    
    for name, lba, size, dr_len in root_entries:
        if size == 2048:
            print(f'DIR: {name}/ (LBA={lba})')
        else:
            print(f'FILE: {name} (LBA={lba}, {size} bytes)')
    
    print(f'\nExtracting to {out_dir}/ ...')
    os.makedirs(out_dir, exist_ok=True)
    
    # Extract root-level files
    for name, lba, size, dr_len in root_entries:
        if size != 2048 and size > 0:
            out_path = os.path.join(out_dir, name)
            print(f'  FILE: {name}')
            extract_file(f, lba, size, out_path)
    
    # Extract root-level subdirectories
    subdirs = [(name, lba, size) for name, lba, size, dr_len in root_entries 
               if size == 2048 and lba != root_lba]
    
    for dirname, dir_lba, dir_size in subdirs:
        dir_out = os.path.join(out_dir, dirname)
        os.makedirs(dir_out, exist_ok=True)
        print(f'\nExtracting dir: {dirname}/')
        extract_dir_recursive(f, dir_lba, dir_size, dir_out)
    
    print('\nDone!')
