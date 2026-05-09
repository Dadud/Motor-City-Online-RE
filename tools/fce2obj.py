#!/usr/bin/env python3
"""
FCE4M to OBJ converter for Motor City Online car models.
Based on bfut/fcecodec FCE specification and MCO-specific tweaks.

Usage: python3 fce2obj.py <input.fce> <output.obj>
"""
import struct
import sys
import os

def read_cstring(data, offset, max_len=64):
    """Read null-terminated C string from data."""
    end = offset
    while end < len(data) and data[end] != 0 and (end - offset) < max_len:
        end += 1
    return data[offset:end].decode('ascii', errors='replace')

def parse_fce4m_header(data):
    """Parse FCE4M header from binary data."""
    if len(data) < 0x2038:
        raise ValueError(f"File too small: {len(data)} < 0x2038 (8256 bytes)")
    
    version = struct.unpack('<I', data[0x00:0x04])[0]
    if version not in [0x00101014, 0x00101015]:
        raise ValueError(f"Unknown FCE version: 0x{version:08X}")
    
    header = {
        'version': version,
        'unknown1': struct.unpack('<I', data[0x04:0x08])[0],
        'num_triangles': struct.unpack('<I', data[0x08:0x0C])[0],
        'num_vertices': struct.unpack('<I', data[0x0C:0x10])[0],
        'num_arts': struct.unpack('<I', data[0x10:0x14])[0],
        'vert_tbl_offset': struct.unpack('<I', data[0x14:0x18])[0],
        'norm_tbl_offset': struct.unpack('<I', data[0x18:0x1C])[0],
        'tria_tbl_offset': struct.unpack('<I', data[0x1C:0x20])[0],
        'reserve1_offset': struct.unpack('<I', data[0x20:0x24])[0],
        'reserve2_offset': struct.unpack('<I', data[0x24:0x28])[0],
        'reserve3_offset': struct.unpack('<I', data[0x28:0x2C])[0],
        'undamgd_vert_offset': struct.unpack('<I', data[0x2C:0x30])[0],
        'undamgd_norm_offset': struct.unpack('<I', data[0x30:0x34])[0],
        'damgd_vert_offset': struct.unpack('<I', data[0x34:0x38])[0],
        'damgd_norm_offset': struct.unpack('<I', data[0x38:0x3C])[0],
        'reserve4_offset': struct.unpack('<I', data[0x3C:0x40])[0],
        'anim_tbl_offset': struct.unpack('<I', data[0x40:0x44])[0],
        'reserve5_offset': struct.unpack('<I', data[0x44:0x48])[0],
        'reserve6_offset': struct.unpack('<I', data[0x48:0x4C])[0],
        'half_size': struct.unpack('<fff', data[0x4C:0x58]),
        'num_dummies': struct.unpack('<I', data[0x58:0x5C])[0],
        'num_parts': struct.unpack('<I', data[0x11C:0x120])[0],
    }
    
    HDR_BASE = 0x2038
    header['vert_start'] = HDR_BASE + header['vert_tbl_offset']
    header['norm_start'] = HDR_BASE + header['norm_tbl_offset']
    header['tria_start'] = HDR_BASE + header['tria_tbl_offset']
    
    # Read dummies
    header['dummies'] = []
    for i in range(header['num_dummies']):
        off = 0x5C + i * 12
        header['dummies'].append(struct.unpack('<fff', data[off:off+12]))
    
    # Read part names
    header['part_names'] = []
    for i in range(header['num_parts']):
        off = 0x0E28 + i * 64
        name = read_cstring(data, off, 64)
        header['part_names'].append(name if name else f"part_{i}")
    
    # Read P1stVertices and PNumVertices
    header['p1st_vertices'] = []
    header['pnum_vertices'] = []
    for i in range(64):
        off = 0x420 + i * 4
        header['p1st_vertices'].append(struct.unpack('<I', data[off:off+4])[0])
        header['pnum_vertices'].append(struct.unpack('<I', data[0x520+i*4:0x520+i*4+4])[0])
    
    # Read P1stTriangles and PNumTriangles
    header['p1st_triangles'] = []
    header['pnum_triangles'] = []
    for i in range(64):
        off = 0x620 + i * 4
        header['p1st_triangles'].append(struct.unpack('<I', data[off:off+4])[0])
        header['pnum_triangles'].append(struct.unpack('<I', data[0x720+i*4:0x720+i*4+4])[0])
    
    return header

def read_vertices(data, header):
    """Read vertex and normal tables."""
    vertices = []
    normals = []
    nv = header['num_vertices']
    vs = header['vert_start']
    ns = header['norm_start']
    
    for i in range(nv):
        vx, vy, vz = struct.unpack('<fff', data[vs+i*12:vs+i*12+12])
        vertices.append((vx, vy, vz))
        nx, ny, nz = struct.unpack('<fff', data[ns+i*12:ns+i*12+12])
        normals.append((nx, ny, nz))
    
    return vertices, normals

def read_triangles(data, header):
    """Read triangle table."""
    triangles = []
    nt = header['num_triangles']
    ts = header['tria_start']
    
    for i in range(nt):
        off = ts + i * 56
        vidx = struct.unpack('<iii', data[off+0x04:off+0x10])
        unk = struct.unpack('<iii', data[off+0x10:off+0x1C])
        flag = struct.unpack('<I', data[off+0x1C:off+0x20])[0]
        U = struct.unpack('<fff', data[off+0x20:off+0x2C])
        V = struct.unpack('<fff', data[off+0x2C:off+0x38])
        
        # V texture coordinate is stored as (1 - V), so flip it back
        V = (1.0 - V[0], 1.0 - V[1], 1.0 - V[2])
        
        triangles.append({
            'vidx': vidx,
            'flag': flag,
            'U': U,
            'V': V,
        })
    
    return triangles

def write_obj(vertices, normals, triangles, header, output_path):
    """Write OBJ file."""
    with open(output_path, 'w') as f:
        f.write(f"# FCE4M to OBJ - Motor City Online\n")
        f.write(f"# NumVertices: {len(vertices)}\n")
        f.write(f"# NumTriangles: {len(triangles)}\n")
        f.write(f"# NumParts: {header['num_parts']}\n")
        f.write(f"# HalfSize: ({header['half_size'][0]:.4f}, {header['half_size'][1]:.4f}, {header['half_size'][2]:.4f})\n\n")
        
        f.write("o MCity_Car\n\n")
        
        # Write vertices
        f.write("# Vertices\n")
        for vx, vy, vz in vertices:
            f.write(f"v {vx:.6f} {vy:.6f} {vz:.6f}\n")
        
        # Write normals
        f.write("\n# Normals\n")
        for nx, ny, nz in normals:
            f.write(f"vn {nx:.6f} {ny:.6f} {nz:.6f}\n")
        
        # Write texture coordinates and triangles
        f.write("\n# Texture coordinates + Triangles\n")
        f.write("usemtl car_body\n")
        f.write("s 1\n")
        
        for tri in triangles:
            vidx = tri['vidx']
            U = tri['U']
            V = tri['V']
            
            # Wavefront OBJ uses 1-based indexing
            i1, i2, i3 = vidx[0] + 1, vidx[1] + 1, vidx[2] + 1
            ni1, ni2, ni3 = i1, i2, i3  # normals aligned with vertices
            
            # V is already flipped from (1-V) to V during reading
            f.write(f"vt {U[0]:.6f} {V[0]:.6f}\n")
            f.write(f"vt {U[1]:.6f} {V[1]:.6f}\n")
            f.write(f"vt {U[2]:.6f} {V[2]:.6f}\n")
            
            # Triangle with explicit UV/normal references
            f.write(f"f {i1}/{i1*3-2}/{ni1} {i2}/{i2*3-1}/{ni2} {i3}/{i3*3}/{ni3}\n")
    
    print(f"Wrote {len(vertices)} vertices, {len(triangles)} triangles to {output_path}")

def main():
    if len(sys.argv) < 3:
        print("Usage: fce2obj.py <input.fce> <output.obj>")
        sys.exit(1)
    
    input_path = sys.argv[1]
    output_path = sys.argv[2]
    
    if not os.path.exists(input_path):
        print(f"Error: {input_path} not found")
        sys.exit(1)
    
    print(f"Reading {input_path}...")
    with open(input_path, 'rb') as f:
        data = f.read()
    
    print("Parsing FCE4M header...")
    header = parse_fce4m_header(data)
    
    print(f"  Version: 0x{header['version']:08X}")
    print(f"  Vertices: {header['num_vertices']}")
    print(f"  Triangles: {header['num_triangles']}")
    print(f"  Parts: {header['num_parts']}")
    print(f"  VertTbl at: 0x{header['vert_start']:x}")
    print(f"  NormTbl at: 0x{header['norm_start']:x}")
    print(f"  TriaTbl at: 0x{header['tria_start']:x}")
    
    print("Reading vertices and normals...")
    vertices, normals = read_vertices(data, header)
    
    print("Reading triangles...")
    triangles = read_triangles(data, header)
    
    print(f"Writing OBJ to {output_path}...")
    write_obj(vertices, normals, triangles, header, output_path)
    
    print("Done!")

if __name__ == '__main__':
    main()
