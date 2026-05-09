#!/usr/bin/env python3
"""Convert Motor City Online .frd track files into a triangulated road-strip OBJ.

Current decoder status:
- Confidently parses the primary 0x54-byte road sample table at 0x30
- Builds a triangulated strip using sample position + local right vector + left/right widths
- Optionally exports the first secondary polygon patch blocks discovered near 0x19860

This is a best-effort preservation/research tool, not a final authoritative FRD renderer.
"""

from __future__ import annotations

import argparse
import math
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

PTR_BASE = 0x03FB0000
PRIMARY_START = 0x30
PRIMARY_STRIDE = 0x54
PRIMARY_COUNT = 1240
SECONDARY_BLOCKS = [
    0x19860,
    0x19F24,
    0x1B370,
]


@dataclass
class Sample:
    flags: int
    section_flag: int
    unk04: int
    pos: Tuple[float, float, float]
    up: Tuple[float, float, float]
    right: Tuple[float, float, float]
    forward: Tuple[float, float, float]
    params: Tuple[float, float, float, float]
    material_or_link: int
    group_a: int
    group_b: int


@dataclass
class PatchBlock:
    offset: int
    count: int
    ptr_a: int
    ptr_b: int
    vertices: List[Tuple[float, float, float]]


def unpack_vec3(data: bytes, off: int) -> Tuple[float, float, float]:
    return struct.unpack_from('<3f', data, off)


def pointer_to_offset(ptr: int) -> int:
    return ptr - PTR_BASE


def parse_primary_samples(data: bytes) -> List[Sample]:
    samples: List[Sample] = []
    for i in range(PRIMARY_COUNT):
        off = PRIMARY_START + i * PRIMARY_STRIDE
        fields = struct.unpack_from('<hhI', data, off)
        pos = unpack_vec3(data, off + 0x08)
        up = unpack_vec3(data, off + 0x14)
        right = unpack_vec3(data, off + 0x20)
        forward = unpack_vec3(data, off + 0x2C)
        params = struct.unpack_from('<4f', data, off + 0x38)
        tail = struct.unpack_from('<III', data, off + 0x48)
        samples.append(Sample(fields[0], fields[1], fields[2], pos, up, right, forward, params, *tail))
    return samples


def parse_patch_block(data: bytes, off: int) -> PatchBlock:
    count, ptr_a, ptr_b = struct.unpack_from('<III', data, off)
    vertices = [unpack_vec3(data, off + 0x0C + i * 12) for i in range(count)]
    return PatchBlock(off, count, ptr_a, ptr_b, vertices)


def build_road_strip(samples: Sequence[Sample], close_loop: bool = False):
    vertices: List[Tuple[float, float, float]] = []
    normals: List[Tuple[float, float, float]] = []
    uvs: List[Tuple[float, float]] = []
    faces: List[Tuple[Tuple[int, int, int], Tuple[int, int, int], Tuple[int, int, int]]] = []

    distances = [0.0]
    for i in range(1, len(samples)):
        ax, ay, az = samples[i - 1].pos
        bx, by, bz = samples[i].pos
        distances.append(distances[-1] + math.dist((ax, ay, az), (bx, by, bz)))
    total = distances[-1] if distances[-1] > 0 else 1.0

    for i, s in enumerate(samples):
        left_w, right_w, _, _ = s.params
        px, py, pz = s.pos
        rx, ry, rz = s.right
        left = (px - rx * left_w, py - ry * left_w, pz - rz * left_w)
        right = (px + rx * right_w, py + ry * right_w, pz + rz * right_w)
        vertices.extend([left, right])
        normals.extend([s.up, s.up])
        v = distances[i] / total
        uvs.extend([(0.0, v), (1.0, v)])

    seg_count = len(samples) if close_loop else len(samples) - 1
    for i in range(seg_count):
        j = (i + 1) % len(samples)
        a = i * 2
        b = a + 1
        c = j * 2
        d = c + 1
        faces.append(((a, a, a), (c, c, c), (b, b, b)))
        faces.append(((b, b, b), (c, c, c), (d, d, d)))

    return vertices, normals, uvs, faces


def fan_triangulate(vertices: Sequence[Tuple[float, float, float]]):
    if len(vertices) < 3:
        return [], [], [], []
    out_verts = list(vertices)
    normals = [(0.0, 1.0, 0.0)] * len(out_verts)
    uvs = [(0.0, 0.0)] * len(out_verts)
    faces = []
    for i in range(1, len(out_verts) - 1):
        faces.append(((0, 0, 0), (i, i, i), (i + 1, i + 1, i + 1)))
    return out_verts, normals, uvs, faces


def write_obj(path: Path, objects):
    with path.open('w', encoding='utf-8') as f:
        f.write('# MCO FRD road extraction\n')
        v_base = 1
        vt_base = 1
        vn_base = 1
        for name, vertices, normals, uvs, faces in objects:
            f.write(f'\no {name}\n')
            for vx, vy, vz in vertices:
                f.write(f'v {vx:.6f} {vy:.6f} {vz:.6f}\n')
            for u, v in uvs:
                f.write(f'vt {u:.6f} {v:.6f}\n')
            for nx, ny, nz in normals:
                f.write(f'vn {nx:.6f} {ny:.6f} {nz:.6f}\n')
            for tri in faces:
                parts = []
                for vi, vti, vni in tri:
                    parts.append(f'{v_base + vi}/{vt_base + vti}/{vn_base + vni}')
                f.write('f ' + ' '.join(parts) + '\n')
            v_base += len(vertices)
            vt_base += len(uvs)
            vn_base += len(normals)


def main() -> int:
    ap = argparse.ArgumentParser(description='Convert Motor City Online FRD road data to OBJ')
    ap.add_argument('input', help='Input Tr.frd path')
    ap.add_argument('output', help='Output OBJ path')
    ap.add_argument('--close-loop', action='store_true', help='Connect final strip segment back to the first sample')
    ap.add_argument('--with-patches', action='store_true', help='Also export the known secondary polygon patch blocks as triangle fans')
    args = ap.parse_args()

    data = Path(args.input).read_bytes()
    samples = parse_primary_samples(data)
    road = build_road_strip(samples, close_loop=args.close_loop)
    objects = [('road_strip', *road)]

    if args.with_patches:
        for off in SECONDARY_BLOCKS:
            block = parse_patch_block(data, off)
            patch = fan_triangulate(block.vertices)
            if patch[0]:
                objects.append((f'patch_{off:06x}_n{block.count}', *patch))

    write_obj(Path(args.output), objects)
    print(f'Wrote {args.output}')
    print(f'Primary samples: {len(samples)}')
    if args.with_patches:
        for off in SECONDARY_BLOCKS:
            block = parse_patch_block(data, off)
            print(f'Patch block 0x{off:X}: count={block.count} ptrA=0x{block.ptr_a:08X} ptrB=0x{block.ptr_b:08X}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
