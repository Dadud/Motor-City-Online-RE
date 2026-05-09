# VIV — Per-Car Archive Format

> Car-specific archive containing FCE geometry + texture data.

**Location:** `Data/models/*.viv` (loose files in betas, or inside `cars.big` in final release)

## Two VIV Variants

The game shipped with two different VIV formats depending on the build:

### Variant 1: Standard VIV (Beta 1 / Oct 09 prototype)

Direct concatenation of FCE + FSH data, no wrapper.

- FCE geometry starts at byte 0 with `FCE4` magic
- FSH texture data follows (EIMA or HRDR magic)
- Used in: Beta 1, Oct 09 prototype

### Variant 2: BIGF-Wrapped VIV (Offline version / 2012 builds)

Archive-wrapped format using BIGF header at byte 0.

- BIGF magic at byte 0 (same archive format as `.big` files)
- FCE geometry at offset **0x8c** (140 bytes in)
- FCE identified by version marker `0x00101015`
- Textures embedded as **SHPI/GIMX** format (GameCube texture port)
- Used in: offline version builds (2012 community releases)

```
Offset 0x00: BIGF header (140 bytes)
Offset 0x8C: FCE4M geometry data
             (1193-1529 vertices, 1431-3652 triangles)
Offset N:    SHPI/GIMX texture data (after FCE)
```

## Archive Structure (Standard Variant)

```
+------------------+
| FCE4M header     |  (8256 bytes)
+------------------+
| Vertex table      |  (num_vertices × 12 bytes)
+------------------+
| Normal table      |  (num_vertices × 12 bytes)
+------------------+
| Triangle table    |  (num_triangles × 44 bytes)
+------------------+
| Part table        |  (num_parts × 4 bytes)
+------------------+
| FSH texture data  |  (EIMA or HRDR format)
+------------------+
```

## Example: 53chevy.viv (Offline version, BIGF-wrapped)

| Part | Description | Vertices | Triangles |
|------|-------------|-----------|-----------|
| `part.fce` | Car body geometry | 1193 | 1431 |

Texturing: embedded SHPI/GIMX (GameCube format texture)

## Car Parts (FCE files inside VIV)

| Part | Description |
|------|-------------|
| `part.fce` | Main body exterior geometry |
| `dash.fce` | Dashboard / interior geometry |
| `spoiler.fce` | Rear spoiler geometry |

Not all cars have all parts. Some have just `part.fce`.

## Naming Convention

VIV files follow a short naming convention: 6 characters + `.viv`

```
32ford.viv    # 1932 Ford
34ford.viv    # 1934 Ford
53chevy.viv  # 1953 Chevrolet
57tbird.viv   # 1957 Thunderbird
64stang.viv  # 1964 Mustang
96supra.viv  # 1996 Toyota Supra
```

The naming scheme is `<year><make><model abbreviation>.viv`.

## Extraction

```bash
# Standard VIV (Beta 1 / Oct 09)
python3 viv_extract.py Data/models/53chevy.viv extracted/

# Offline version VIV (BIGF-wrapped)
python3 viv_extract.py Data/Models/53chevy.viv extracted/
```

## Beta vs Final

| | Beta 1 | Oct 09 | Final |
|---|---|---|---|
| VIV count | 304 | 628 | ~50 in cars.big |
| Storage | Loose files | Loose files | Packed in cars.big |
| Format | Standard | Standard | Standard |
| Textures | EIMA/HRDR | EIMA/HRDR | G264 |

In **Beta 1** and **Oct 09**, VIV files are stored as loose files in `Data/models/`.  
In the **final release**, all VIV files are packed into `cars.big`.  
In the **offline version**, VIV files use the BIGF-wrapped format with GIMX textures.
