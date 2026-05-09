# VIV — Per-Car Archive Format

> Car-specific archive containing FCE geometry + FSH textures.

**Format:** Identical to BIGF archive  
**Location:** `Data/models/*.viv` (280 files in final release)  
**Compression:** Implode (PKWARE) or stored

## Archive Structure

VIV files use the same BIGF format as `.big` archives. See [BIG.md](BIG.md) for the full specification.

The only difference is content: VIV files contain car-specific model parts, not generic game data.

## Example: 53chevy.viv

Extracted contents:

| Filename | Size | Description |
|----------|------|-------------|
| `dash.fce` | 268 KB | Dashboard geometry |
| `dash.fsh` | 257 KB | Dashboard texture |
| `part.blf` | 34 KB | Bill of materials (vertex segmentation) |
| `part.fce` | 328 KB | Car body geometry |
| `part.fst` | 502 KB | FST unknown format |
| `part.lod` | 28 bytes | LOD distance thresholds |
| `part.fsh` | — | Car body texture |
| `spoiler.fce` | 12 KB | Spoiler geometry (if car has one) |

## Car Parts (FCE files inside VIV)

| Part | Description |
|------|-------------|
| `part.fce` | Main body exterior geometry |
| `dash.fce` | Dashboard / interior geometry |
| `spoiler.fce` | Rear spoiler geometry |
| `hood.fce` | Hood geometry (if separate) |
| `.*.fce` | Other separate parts |

Not all cars have all parts. Some have just `part.fce` + `part.fsh`.

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
python3 viv_extract.py Data/models/53chevy.viv extracted/53chevy/
```

Or manually treat as BIGF and extract with a BIG decoder.

## Beta vs Final

In **Beta 1**, VIV files are stored as loose files in `Data/models/`.  
In the **final release**, all VIV files are packed into `cars.big`.
