#!/usr/bin/env python3
"""
Motor City Online Asset Viewer - Headless Version
Generates PNG previews and an HTML gallery for MCO assets.
Requires: Pillow, numpy

Usage:
  python3 mco_asset_viewer.py                    # Generate all previews
  python3 mco_asset_viewer.py car.obj            # Preview single model
  python3 mco_asset_viewer.py --serve           # Generate + open HTML viewer
"""

import os
import sys
import math
import struct
import json
import base64
import traceback
from pathlib import Path
from datetime import datetime

# --- Dependencies ---
try:
    from PIL import Image
    import numpy as np
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install Pillow numpy")
    sys.exit(1)

# --- 3D Math ---
class Vec3:
    __slots__ = ('x', 'y', 'z')
    def __init__(self, x=0.0, y=0.0, z=0.0):
        self.x, self.y, self.z = x, y, z
    def __add__(self, o): return Vec3(self.x+o.x, self.y+o.y, self.z+o.z)
    def __sub__(self, o): return Vec3(self.x-o.x, self.y-o.y, self.z-o.z)
    def __mul__(self, s): return Vec3(self.x*s, self.y*s, self.z*s)
    def dot(self, o): return self.x*o.x + self.y*o.y + self.z*o.z
    def cross(self, o):
        return Vec3(self.y*o.z - self.z*o.y,
                    self.z*o.x - self.x*o.z,
                    self.x*o.y - self.y*o.x)
    def length(self): return math.sqrt(self.x**2 + self.y**2 + self.z**2)
    def normalize(self):
        l = self.length()
        return Vec3(self.x/l, self.y/l, self.z/l) if l > 0 else Vec3()

# --- OBJ Loader ---
class OBJModel:
    def __init__(self):
        self.vertices = []
        self.normals = []
        self.texcoords = []
        self.faces = []
        self.name = ""

    def load(self, path):
        self.vertices, self.normals, self.texcoords, self.faces = [], [], [], []
        self.name = Path(path).stem
        with open(path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split()
                if not parts: continue
                cmd = parts[0]
                if cmd == 'v':
                    self.vertices.append(Vec3(float(parts[1]), float(parts[2]), float(parts[3])))
                elif cmd == 'vn':
                    self.normals.append(Vec3(float(parts[1]), float(parts[2]), float(parts[3])))
                elif cmd == 'vt':
                    self.texcoords.append((float(parts[1]), float(parts[2])))
                elif cmd == 'f':
                    face = []
                    for v in parts[1:]:
                        pv = v.split('/')
                        vi = int(pv[0]) - 1 if pv[0] else 0
                        ti = int(pv[1]) - 1 if len(pv) > 1 and pv[1] else 0
                        ni = int(pv[2]) - 1 if len(pv) > 2 and pv[2] else 0
                        face.append((vi, ti, ni))
                    self.faces.append(face)
        print(f"  {self.name}: {len(self.vertices)} verts, {len(self.faces)} faces")
        return self

# --- Software Renderer ---
class Renderer:
    def __init__(self, width=640, height=480):
        self.W = width
        self.H = height
        self.clear()

    def clear(self):
        self.buf = np.zeros((self.H, self.W, 3), dtype=np.uint8)
        self.zbuf = np.full((self.H, self.W), 1e9, dtype=np.float32)

    def clear_color(self, r, g, b):
        self.buf[:] = [r, g, b]

    def raster_tri(self, p0, p1, p2, color):
        """Render a flat-colored triangle with z-buffer."""
        # Bounding box
        xs = sorted([p0[0], p1[0], p2[0]])
        ys = sorted([p0[1], p1[1], p2[1]])
        x0, x1 = max(0, int(xs[0])), min(self.W-1, int(xs[2])+1)
        y0, y1 = max(0, int(ys[0])), min(self.H-1, int(ys[2])+1)

        # Triangle area
        def area(ax, ay, bx, by, cx, cy):
            return (bx-ax)*(cy-ay) - (by-ay)*(cx-ax)
        area_tot = area(p0[0], p0[1], p1[0], p1[1], p2[0], p2[1])
        if abs(area_tot) < 0.5:
            return

        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                # Barycentric weights
                w0 = area(p1[0], p1[1], p2[0], p2[1], x, y) / area_tot
                w1 = area(p2[0], p2[1], p0[0], p0[1], x, y) / area_tot
                w2 = area(p0[0], p0[1], p1[0], p1[1], x, y) / area_tot
                if w0 >= 0 and w1 >= 0 and w2 >= 0:
                    z = w0*p0[2] + w1*p1[2] + w2*p2[2]
                    if z < self.zbuf[y, x] and z > 0:
                        self.zbuf[y, x] = z
                        self.buf[y, x] = color

    def project(self, v, mvp):
        """Project a Vec3 through 4x4 matrix."""
        x = mvp[0][0]*v.x + mvp[0][1]*v.y + mvp[0][2]*v.z + mvp[0][3]
        y = mvp[1][0]*v.x + mvp[1][1]*v.y + mvp[1][2]*v.z + mvp[1][3]
        z = mvp[2][0]*v.x + mvp[2][1]*v.y + mvp[2][2]*v.z + mvp[2][3]
        w = mvp[3][0]*v.x + mvp[3][1]*v.y + mvp[3][2]*v.z + mvp[3][3]
        if w <= 0 or not math.isfinite(w): return None
        x, y, z = x/w, y/w, z/w
        if not (math.isfinite(x) and math.isfinite(y) and math.isfinite(z)):
            return None
        # To screen
        sx = int((x * 0.5 + 0.5) * self.W)
        sy = int((y * 0.5 + 0.5) * self.H)
        return (sx, sy, z)

    def render_model(self, model, rot_y=0.0, scale=1.0, light=(0.5, 0.8, 0.6)):
        """Render an OBJ model and return as PIL Image."""
        self.clear()
        self.clear_color(35, 35, 42)

        # Matrices
        aspect = self.W / self.H
        fov = math.tan(math.radians(22.5))
        f = 1.0 / fov
        nf = 1.0 / 0.1
        proj = [[f/aspect,0,0,0],[0,f,0,0],[0,0,(1+1)*nf,-1],[0,0,2*0.1*1*nf,0]]

        cam_r = 4.0 / scale
        eye = (cam_r*math.sin(rot_y), 1.5, cam_r*math.cos(rot_y))
        # Look at origin
        fx,fy,fz = -eye[0], -eye[1], -eye[2]
        fl = math.sqrt(fx**2+fy**2+fz**2)
        fx,fy,fz = fx/fl, fy/fl, fz/fl
        # Right = forward x up(0,1,0)
        rx,ry,rz = fz*1 - fx*0, 0, -fx*1  # cross((0,1,0), f)
        rx,ry,rz = ry*fz - rz*fy, rz*fx - rx*fz, rx*fy - ry*fx
        rl = math.sqrt(rx**2+ry**2+rz**2)
        rx,ry,rz = rx/rl, ry/rl, rz/rl
        # Up = right x forward
        ux,uy,uz = ry*fz - rz*fy, rz*fx - rx*fz, rx*fy - ry*fx

        view = [
            [rx, ux, -fx, -(rx*eye[0]+ux*eye[1]-fx*eye[2])],
            [ry, uy, -fy, -(ry*eye[0]+uy*eye[1]-fy*eye[2])],
            [rz, uz, -fz, -(rz*eye[0]+uz*eye[1]-fz*eye[2])],
            [0, 0, 0, 1]
        ]

        # Model rotY
        c, s = math.cos(rot_y), math.sin(rot_y)
        model_mat = [[c,0,s,0],[0,1,0,0],[-s,0,c,0],[0,0,0,1]]

        # MVP
        def mul4(a, b):
            r = [[0]*4 for _ in range(4)]
            for i in range(4):
                for j in range(4):
                    r[i][j] = sum(a[i][k]*b[k][j] for k in range(4))
            return r
        mv = mul4(model_mat, view)
        mvp = mul4(proj, mv)

        lx, ly, lz = light
        ln = math.sqrt(lx**2+ly**2+lz**2)
        lx, ly, lz = lx/ln, ly/ln, lz/ln

        for face in model.faces:
            if len(face) < 3: continue
            pts = []
            for vi, ti, ni in face[:3]:
                if vi >= len(model.vertices):
                    continue
                v = model.vertices[vi]
                # Clamp very large coords to prevent overflow
                if abs(v.x) > 1e6 or abs(v.y) > 1e6 or abs(v.z) > 1e6:
                    v = Vec3(max(-1e6, min(1e6, v.x)),
                             max(-1e6, min(1e6, v.y)),
                             max(-1e6, min(1e6, v.z)))
                p = self.project(v, mvp)
                if p: pts.append(p)

            if len(pts) < 3: continue

            # Flat shading: compute normal from verts
            v0, v1, v2 = (model.vertices[face[j][0]] for j in range(3))
            ex, ey, ez = v1.x-v0.x, v1.y-v0.y, v1.z-v0.z
            fx, fy, fz = v2.x-v0.x, v2.y-v0.y, v2.z-v0.z
            nx, ny, nz = ey*fz-ez*fy, ez*fx-ex*fz, ex*fy-ey*fx
            nl = math.sqrt(nx**2+ny**2+nz**2)
            if nl < 1e-10: continue
            nx, ny, nz = nx/nl, ny/nl, nz/nl
            diff = max(0, nx*lx + ny*ly + nz*lz)
            bright = int(255 * (0.25 + 0.75 * diff))
            color = (min(255, bright + 30), min(255, bright + 15), min(255, bright))

            self.raster_tri(pts[0], pts[1], pts[2], color)

        img = Image.fromarray(self.buf, 'RGB')
        return img

# --- Find assets ---
def find_assets(base=None):
    if base is None:
        base = Path(__file__).parent.parent
    roots = [
        base / "mco-wiki" / "data" / "car_models",
        base / "mco-wiki" / "data" / "tracks",
        base / "mco-files" / "all_cars",
    ]
    categories = []
    for root in roots:
        if root.exists():
            objs = sorted(root.glob("*.obj"))
            if objs:
                cat = "tracks" if "track" in str(root) else "cars"
                categories.append((str(root), objs, cat))
    return categories

# --- Generate HTML Gallery ---
def generate_html(assets, output_dir):
    """Generate an HTML gallery of all assets."""
    html = """<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<title>MCO Asset Gallery</title>
<style>
* { margin: 0; padding: 0; box-sizing: border-box; }
body { background: #1a1a1f; color: #ccc; font-family: monospace; }
h1 { background: #252530; padding: 15px; border-bottom: 1px solid #333; }
h2 { padding: 15px 15px 5px; color: #e0a060; font-size: 14px; }
.gallery { display: grid; grid-template-columns: repeat(auto-fill, minmax(220px, 1fr)); gap: 15px; padding: 15px; }
.card { background: #222228; border-radius: 6px; overflow: hidden; border: 1px solid #333; }
.card img { width: 100%; aspect-ratio: 4/3; object-fit: cover; display: block; }
.card .name { padding: 8px 10px; font-size: 12px; color: #aaa; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.card .meta { padding: 0 10px 8px; font-size: 11px; color: #666; }
.preview { background: #151518; display: flex; align-items: center; justify-content: center; color: #444; font-size: 11px; }
.tag { display: inline-block; background: #333; padding: 2px 6px; border-radius: 3px; margin: 2px; font-size: 10px; }
.tag-cars { background: #2a3a2a; color: #8c8; }
.tag-tracks { background: #2a2a3a; color: #88c; }
</style>
</head><body>
<h1>🎮 Motor City Online — Asset Gallery</h1>
"""

    total = 0
    for path, objs, cat in assets:
        html += f'<h2>{cat.upper()} <span class="tag tag-{cat}">{len(objs)} models</span></h2>\n<div class="gallery">\n'
        for obj in objs:
            total += 1
            stem = obj.stem
            preview_path = Path(output_dir) / cat / f"{stem}.png"
            rel_preview = preview_path.relative_to(output_dir) if preview_path.exists() else None

            meta = obj.stat().st_size
            meta_str = f"{meta/1024:.1f} KB"
            if rel_preview:
                img_tag = f'<img src="{rel_preview}" alt="{stem}" loading="lazy">'
            else:
                img_tag = '<div class="preview">no preview</div>'
            html += f'''  <div class="card">
  {img_tag}
  <div class="name" title="{stem}">{stem}</div>
  <div class="meta">{meta_str}</div>
</div>\n'''
        html += '</div>\n'

    html += f'<p style="padding:20px;color:#555;font-size:11px">Generated {datetime.now().strftime("%Y-%m-%d %H:%M")} — {total} assets</p></body></html>'

    return html

# --- Batch render ---
def render_all(assets, output_dir, renderer):
    Path(output_dir).mkdir(parents=True, exist_ok=True)
    results = []

    for path, objs, cat in assets:
        cat_dir = Path(output_dir) / cat
        cat_dir.mkdir(exist_ok=True)
        print(f"\n[{cat.upper()}] {path}")
        for obj in objs:
            stem = obj.stem
            out_path = cat_dir / f"{stem}.png"
            if out_path.exists():
                print(f"  {stem}: already rendered, skipping")
                results.append((str(obj), str(out_path), cat))
                continue
            try:
                model = OBJModel().load(str(obj))
                img = renderer.render_model(model, rot_y=0.4, scale=1.0)
                img.save(out_path)
                print(f"  {stem}: OK")
                results.append((str(obj), str(out_path), cat))
            except Exception as e:
                print(f"  {stem}: ERROR {e}")
                traceback.print_exc()
    return results

# --- Single preview ---
def preview_one(path, scale=1.0):
    renderer = Renderer(800, 600)
    model = OBJModel().load(path)
    return renderer.render_model(model, rot_y=0.4, scale=scale)

# --- Main ---
def main():
    import argparse
    parser = argparse.ArgumentParser(description="MCO Asset Viewer")
    parser.add_argument('file', nargs='?', help='OBJ file to preview')
    parser.add_argument('--output', '-o', default='./mco-gallery', help='Output directory for previews')
    parser.add_argument('--html', action='store_true', help='Generate HTML gallery')
    parser.add_argument('--scale', type=float, default=1.0, help='Zoom scale')
    parser.add_argument('--all', action='store_true', help='Render all assets')
    args = parser.parse_args()

    if args.file:
        # Single file preview
        img = preview_one(args.file, args.scale)
        out = Path(args.file).with_suffix('.preview.png')
        img.save(out)
        print(f"Saved: {out}")
        return

    if args.html or args.all:
        assets = find_assets()
        if not assets:
            print("No assets found. Run fce2obj.py/frd2obj.py first.")
            return
        renderer = Renderer(640, 480)
        results = render_all(assets, args.output, renderer)
        html = generate_html(assets, args.output)
        html_path = Path(args.output) / "index.html"
        with open(html_path, 'w') as f:
            f.write(html)
        print(f"\nGallery: {html_path}")
        print(f"Open in browser: file://{html_path.absolute()}")
        return

    # Default: find and list
    assets = find_assets()
    print("Motor City Online Asset Viewer")
    print("=" * 50)
    if assets:
        print(f"\nFound {sum(len(o) for _,o,_ in assets)} models:")
        for path, objs, cat in assets:
            print(f"  [{cat}] {path}/")
            for o in objs[:5]:
                print(f"    {o.name}")
            if len(objs) > 5:
                print(f"    ... +{len(objs)-5} more")
    else:
        print("\nNo assets found.")
    print("\nUsage:")
    print("  mco_asset_viewer.py --all --html   # Generate gallery")
    print("  mco_asset_viewer.py model.obj      # Preview single model")
    print("  mco_asset_viewer.py --all          # Render all without HTML")

if __name__ == '__main__':
    main()
