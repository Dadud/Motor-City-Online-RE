#!/usr/bin/env python3
"""
Motor City Online Asset Viewer - Headless Version
Pure Python software renderer. Generates PNG previews and HTML gallery.
Requires: Pillow, numpy

Usage:
  python3 mco_asset_viewer.py                    # List assets
  python3 mco_asset_viewer.py model.obj         # Preview single model
  python3 mco_asset_viewer.py --all --html      # Generate all previews + gallery
"""

import os, sys, math, traceback
from pathlib import Path
from PIL import Image
import numpy as np

# -------------------------------------------------------------------------
# Simple software rasterizer
# -------------------------------------------------------------------------

def mat4mul(a, b):
    r = [[0]*4 for _ in range(4)]
    for i in range(4):
        for j in range(4):
            r[i][j] = sum(a[i][k]*b[k][j] for k in range(4))
    return r

def mat4Perspective(fovDeg, aspect, near, far):
    f = 1.0 / math.tan(math.radians(fovDeg / 2.0))
    return [[f/aspect, 0, 0, 0],[0, f, 0, 0],[0, 0, 2, -(far+near)],[0, 0, 0, far-near]]

def mat4LookAt(eye, target, up):
    # z_axis = normalize(target - eye) = direction camera looks
    fx, fy, fz = target[0]-eye[0], target[1]-eye[1], target[2]-eye[2]
    fl = math.sqrt(fx*fx + fy*fy + fz*fz)
    fx, fy, fz = fx/fl, fy/fl, fz/fl
    rx, ry, rz = up[1]*fz-up[2]*fy, up[2]*fx-up[0]*fz, up[0]*fy-up[1]*fx
    rl = math.sqrt(rx*rx + ry*ry + rz*rz)
    rx, ry, rz = rx/rl, ry/rl, rz/rl
    ux, uy, uz = fy*rz-fz*ry, fz*rx-fx*rz, fx*ry-fy*rx
    tx = -(rx*eye[0] + ux*eye[1] + fx*eye[2])
    ty = -(ry*eye[0] + uy*eye[1] + fy*eye[2])
    tz = -(rz*eye[0] + uz*eye[1] + fz*eye[2])
    return [[rx, ux, fx, tx],[ry, uy, fy, ty],[rz, uz, fz, tz],[0, 0, 0, 1]]

def mat4Scale(s):
    return [[s,0,0,0],[0,s,0,0],[0,0,s,0],[0,0,0,1]]

def mat4RotY(a):
    c, s = math.cos(a), math.sin(a)
    return [[c,0,s,0],[0,1,0,0],[-s,0,c,0],[0,0,0,1]]

def transformPoint(m, vx, vy, vz):
    return (m[0][0]*vx+m[0][1]*vy+m[0][2]*vz+m[0][3],
            m[1][0]*vx+m[1][1]*vy+m[1][2]*vz+m[1][3],
            m[2][0]*vx+m[2][1]*vy+m[2][2]*vz+m[2][3],
            m[3][0]*vx+m[3][1]*vy+m[3][2]*vz+m[3][3])

def project(wx, wy, wz, ww, W, H):
    if ww <= 0: return None
    cx, cy, cz = wx/ww, wy/ww, wz/ww
    if not (-1.0 <= cx <= 1.0 and -1.0 <= cy <= 1.0 and -1.0 <= cz <= 1.0):
        return None
    sx = int((cx * 0.5 + 0.5) * W)
    sy = int((cy * 0.5 + 0.5) * H)
    return (sx, sy, cz)

def barycentric(ax, ay, bx, by, cx, cy, px, py):
    denom = (by-cy)*(ax-cx) + (cx-bx)*(ay-cy)
    if abs(denom) < 0.001: return 0, 0, 0
    w0 = ((by-cy)*(px-cx) + (cx-bx)*(py-cy)) / denom
    w1 = ((cy-ay)*(px-cx) + (ax-cx)*(py-cy)) / denom
    w2 = 1.0 - w0 - w1
    return w0, w1, w2

def lerp(a, b, t): return a + (b-a)*t

# -------------------------------------------------------------------------
# OBJ loader
# -------------------------------------------------------------------------

def loadOBJ(path):
    verts, normals, texcoords, faces = [], [], [], []
    with open(path) as f:
        for line in f:
            parts = line.strip().split()
            if not parts: continue
            if parts[0] == 'v':
                verts.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vn':
                normals.append((float(parts[1]), float(parts[2]), float(parts[3])))
            elif parts[0] == 'vt':
                texcoords.append((float(parts[1]), float(parts[2])))
            elif parts[0] == 'f':
                face = []
                for p in parts[1:]:
                    vi = ti = ni = 0
                    segs = p.split('/')
                    vi = int(segs[0]) - 1
                    if len(segs) > 1 and segs[1]: ti = int(segs[1]) - 1
                    if len(segs) > 2 and segs[2]: ni = int(segs[2]) - 1
                    face.append((vi, ti, ni))
                faces.append(face)
    print(f"  {Path(path).stem}: {len(verts)} verts, {len(faces)} faces")
    return verts, normals, texcoords, faces

# -------------------------------------------------------------------------
# Software renderer
# -------------------------------------------------------------------------

class Renderer:
    def __init__(self, W=640, H=480):
        self.W, self.H = W, H
        self.clear()

    def clear(self):
        self.color = np.full((self.H, self.W, 3), 35, dtype=np.uint8)
        self.depth = np.full((self.H, self.W), 1e9, dtype=np.float32)

    def drawTriangle(self, p0, p1, p2, col0, col1, col2, mvp, view, light, verts, draw_edges=False):
        v0 = verts[p0[0]]; v1 = verts[p1[0]]; v2 = verts[p2[0]]
        s0 = project(*transformPoint(mvp, v0[0], v0[1], v0[2]), self.W, self.H)
        s1 = project(*transformPoint(mvp, v1[0], v1[1], v1[2]), self.W, self.H)
        s2 = project(*transformPoint(mvp, v2[0], v2[1], v2[2]), self.W, self.H)
        if not (s0 and s1 and s2): return

        (x0,y0,z0), (x1,y1,z1), (x2,y2,z2) = s0, s1, s2
        x0,y0,x1,y1,x2,y2 = int(x0),int(y0),int(x1),int(y1),int(x2),int(y2)

        # Flat shading: face normal
        ex, ey, ez = v1[0]-v0[0], v1[1]-v0[1], v1[2]-v0[2]
        fx, fy, fz = v2[0]-v0[0], v2[1]-v0[1], v2[2]-v0[2]
        nx, ny, nz = ey*fz-ez*fy, ez*fx-ex*fz, ex*fy-ey*fx
        nl = math.sqrt(nx*nx+ny*ny+nz*nz)
        if nl < 1e-10: return
        nx, ny, nz = nx/nl, ny/nl, nz/nl
        diffuse = max(0.0, nx*light[0] + ny*light[1] + nz*light[2])
        bright = int(255 * (0.2 + 0.8 * diffuse))
        base = (min(255, bright+10), min(255, bright+5), bright)

        # Bounding box
        xmin = max(0, min(x0, x1, x2))
        xmax = min(self.W-1, max(x0, x1, x2))
        ymin = max(0, min(y0, y1, y2))
        ymax = min(self.H-1, max(y0, y1, y2))
        if xmin > xmax or ymin > ymax: return

        def edge(px, py, ax, ay, bx, by):
            return (px-ax)*(by-ay) - (py-ay)*(bx-ax)

        area = edge(x0, y0, x1, y1, x2, y2)
        if abs(area) < 0.5: return

        for y in range(ymin, ymax+1):
            for x in range(xmin, xmax+1):
                w0 = edge(x1, y1, x2, y2, x, y) / area
                w1 = edge(x2, y2, x0, y0, x, y) / area
                w2 = edge(x0, y0, x1, y1, x, y) / area
                if w0 >= 0 and w1 >= 0 and w2 >= 0:
                    z = w0*z0 + w1*z1 + w2*z2
                    if z < self.depth[y, x] and -1.0 < z < 1.0:
                        self.depth[y, x] = z
                        self.color[y, x] = base

    def render(self, verts, faces, rotY=0.0, scale=1.0, light=(0.5, 0.7, 0.5)):
        """Render OBJ to PIL Image."""
        self.clear()
        W, H = self.W, self.H

        # Compute model bounding box
        xs = [v[0] for v in verts]; ys = [v[1] for v in verts]; zs = [v[2] for v in verts]
        extent = max(max(xs)-min(xs), max(ys)-min(ys), max(zs)-min(zs))
        cx = (min(xs)+max(xs))/2; cy = (min(ys)+max(ys))/2; cz = (min(zs)+max(zs))/2

        # Camera: orbit around origin at fixed distance (was working before)
        # Scale camera for large tracks so they fit in frame
        # Camera: orbit at fixed distance, look at origin
        # Scale camera for tracks so they fit in frame
        if extent > 50.0:
            extent_ratio = max(1.0, extent / 3.0)
            cam_dist = (3.0 / scale) * extent_ratio
            model_scale = 50.0 / scale
            far = 200000.0
        else:
            cam_dist = 3.0 / scale
            model_scale = 50.0 / scale
            far = 200.0

        eye_x = cam_dist * math.sin(rotY)
        eye_y = 0.5
        eye_z = -cam_dist * math.cos(rotY)

        proj = mat4Perspective(45.0, W/H, 0.1, far)
        view = mat4LookAt((eye_x, eye_y, eye_z), (0, 0, 0), (0, 1, 0))
        model = mat4Scale(model_scale)
        mv = mat4mul(model, view)
        mvp = mat4mul(proj, mv)

        light_n = math.sqrt(sum(l*l for l in light))
        light = (light[0]/light_n, light[1]/light_n, light[2]/light_n)

        for face in faces:
            if len(face) < 3: continue
            p0 = face[0]
            for i in range(1, len(face)-1):
                p1, p2 = face[i], face[i+1]
                self.drawTriangle(p0, p1, p2,
                                  (200,180,160), (200,180,160), (200,180,160),
                                  mvp, view, light, verts, draw_edges=False)

        return Image.fromarray(self.color, 'RGB')

# -------------------------------------------------------------------------
# Find assets
# -------------------------------------------------------------------------

def findAssets():
    base = Path(__file__).parent.parent
    roots = [
        base/"mco-wiki"/"data"/"car_models",
        base/"mco-wiki"/"data"/"tracks",
        base/"mco-files"/"all_cars",
    ]
    cats = []
    for r in roots:
        if r.exists():
            objs = sorted(r.glob("*.obj"))
            if objs:
                cat = "tracks" if "track" in str(r) else "cars"
                cats.append((str(r), objs, cat))
    return cats

# -------------------------------------------------------------------------
# HTML gallery
# -------------------------------------------------------------------------

def makeGallery(assets, outDir):
    Path(outDir).mkdir(parents=True, exist_ok=True)
    total = 0
    html = """<!DOCTYPE html>
<html><head><meta charset="utf-8">
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
.tag { display: inline-block; background: #333; padding: 2px 6px; border-radius: 3px; margin: 2px; font-size: 10px; }
.tag-cars { background: #2a3a2a; color: #8c8; }
.tag-tracks { background: #2a2a3a; color: #88c; }
</style></head><body>
<h1>Motor City Online — Asset Gallery</h1>"""
    for path, objs, cat in assets:
        html += f'<h2>{cat.upper()} <span class="tag tag-{cat}">{len(objs)} models</span></h2>\n<div class="gallery">\n'
        for obj in objs:
            total += 1
            stem = obj.stem
            preview = Path(outDir)/cat/f"{stem}.png"
            rel = preview.relative_to(outDir) if preview.exists() else None
            meta = obj.stat().st_size
            img_tag = f'<img src="{rel}" alt="{stem}">' if rel else '<div style="background:#111;color:#444;width:100%;aspect-ratio:4/3;display:flex;align-items:center;justify-content:center">no preview</div>'
            html += f'  <div class="card">{img_tag}<div class="name">{stem}</div><div class="meta">{meta/1024:.1f} KB</div></div>\n'
        html += '</div>\n'
    html += f'<p style="padding:20px;color:#555;font-size:11px">Generated — {total} assets</p></body></html>'
    outPath = Path(outDir)/"index.html"
    with open(outPath, 'w') as f: f.write(html)
    return outPath, total

# -------------------------------------------------------------------------
# Batch render
# -------------------------------------------------------------------------

def renderAll(assets, outDir, W=640, H=480):
    Path(outDir).mkdir(parents=True, exist_ok=True)
    rnd = Renderer(W, H)
    for path, objs, cat in assets:
        catDir = Path(outDir)/cat
        catDir.mkdir(exist_ok=True)
        print(f"\n[{cat.upper()}]")
        for obj in objs:
            stem = obj.stem
            outPng = catDir/f"{stem}.png"
            if outPng.exists():
                print(f"  {stem}: skip")
                continue
            try:
                verts, normals, texcoords, faces = loadOBJ(str(obj))
                if not verts: continue
                img = rnd.render(verts, faces, rotY=0.5, scale=1.0)
                img.save(outPng)
                print(f"  {stem}: OK")
            except Exception as e:
                print(f"  {stem}: ERROR {e}")

# -------------------------------------------------------------------------
# Main
# -------------------------------------------------------------------------

if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('file', nargs='?', help='OBJ file to preview')
    ap.add_argument('--all', action='store_true', help='Render all models')
    ap.add_argument('--html', action='store_true', help='Generate HTML gallery')
    ap.add_argument('--output', '-o', default='./mco-gallery', help='Output dir')
    ap.add_argument('--scale', type=float, default=1.0, help='Model scale')
    ap.add_argument('--rotY', type=float, default=0.5, help='Y rotation')
    ap.add_argument('--size', '-s', default='640x480', help='WxH')
    args = ap.parse_args()

    W, H = map(int, args.size.split('x'))

    if args.file:
        verts, normals, texcoords, faces = loadOBJ(args.file)
        rnd = Renderer(W, H)
        img = rnd.render(verts, faces, rotY=args.rotY, scale=args.scale)
        out = str(Path(args.file).with_suffix('.png'))
        img.save(out)
        print(f"Saved: {out}")
        sys.exit(0)

    assets = findAssets()
    if not assets:
        print("No assets found. Run fce2obj.py first.")
        sys.exit(1)

    total_models = sum(len(o) for _,o,_ in assets)
    print(f"MCO Asset Viewer — {total_models} models found:")
    for p, objs, cat in assets:
        print(f"  [{cat}] {p}/ ({len(objs)} files)")

    if args.all or args.html:
        renderAll(assets, args.output, W, H)
        galPath, total = makeGallery(assets, args.output)
        print(f"Gallery: {galPath}")
    else:
        print("\nUsage:")
        print("  --all --html   Render everything + gallery")
        print("  model.obj      Preview single model")
