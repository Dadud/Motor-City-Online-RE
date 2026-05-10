# Tools

Extraction and conversion utilities for Motor City Online file formats.

## Available Tools

### Conversion Tools

#### fce2obj.py

Converts FCE4M car geometry files to Wavefront OBJ format.

```bash
python3 fce2obj.py Data/models/53chevy/part.fce output.obj
```

Output: OBJ file with vertices, normals, and UV-mapped triangles.

Works on: standard VIV-extracted FCE files (Beta 1, Oct 09) and BIGF-wrapped VIV FCE files (offline version).

#### frd2obj.py

Converts FRD track geometry files to OBJ format.

```bash
python3 frd2obj.py Data/Tracks/Boothill/Tr.frd output.obj
```

### Archive Extraction

#### big_extract.py

Extracts all files from a BIG archive.

```bash
python3 big_extract.py cars.big extracted/
```

**Reference:** [Camconn BIG decoder gist](https://gist.github.com/camconn/f9cf6ee31103070296f9bec89aa97831)

#### viv_extract.py

Extracts FCE geometry from a VIV car archive. Handles **both** VIV variants:

1. **Standard VIV** (Beta 1 / Oct 09): direct FCE+FSH concatenation
2. **BIGF-wrapped VIV** (offline version): FCE at offset 0x8c inside BIGF wrapper

```bash
# Works on both variants automatically
python3 viv_extract.py Data/Models/53chevy.viv extracted/
```

Output: `part.fce` (geometry) and optionally `part.shpi` (texture, offline version only).

#### iso_extract.py

Extracts files from a raw ISO 9660 image without requiring root or loopback.

```bash
python3 iso_extract.py Motor\ City\ Online.iso extracted/
```

Parses the ISO 9660 volume descriptors and recursively extracts all files and directories.

### Image Conversion

#### fsh2png.py

Converts SHPI Type 1 texture files to PPM (P6 binary) format.

```bash
python3 fsh2png_v2.py HUD50.fsh extracted/
```

Output: PPM files named `{tag}_{width}x{height}.ppm` for each entry.

**Status:**
- ✅ SHPI header parsing
- ✅ `fsh2png_v2.py` decodes MCO's known texture variants:
  - record_id `0xFD` = RefPack + A8R8G8B8 (Beta 1 / HUD50 including `xamas`, `gear`, `metr`, `4444`)
  - record_id `0xFE` = RefPack + A1R5G5B5 (Oct 09 prototype)
  - record_id `0x7D` = raw A8R8G8B8 (`G264`, offline car textures)
- ⚠️ `fsh2png.py` is the older experimental decoder; prefer `fsh2png_v2.py`

**Reference:** [rewiki — EA SSH FSH Image (Type 1)](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))

### Audio Conversion

#### bnk2wav.py

Converts BNK sound banks to WAV files.

```bash
python3 bnk2wav.py track.bnk track.wav
```

Status:
- ✅ BNK container parsing
- ✅ EA XA ADPCM decoding
- ✅ WAV output
- ✅ Tested against real MCO `track.bnk` files

### Database

```bash
# List all tables
mdb-tables Online.mdb

# Export table to CSV
mdb-export Online.mdb CarModels > CarModels.csv
```

## Quick Start: Extracting Car Models

```bash
# 1. Extract ISO contents (no root needed)
python3 iso_extract.py "Motor City Online.iso" iso_extracted/

# 2. Extract BIG archives
python3 big_extract.py iso_extracted/Data/cars.big cars/

# 3. Extract individual VIV files
for viv in cars/*.viv; do
  name=$(basename $viv .viv)
  python3 viv_extract.py "$viv" "car_parts/$name/"
done

# 4. Convert FCE to OBJ
python3 fce2obj.py car_parts/53chevy/part.fce 53chevy.obj
```

## File Dependencies

To extract content from the final release BIG archives, first extract them:

```bash
# Extract cars.big
python3 big_extract.py cars.big cars_extracted/

# The VIV files inside can then be extracted
for viv in cars_extracted/*.viv; do
  python3 viv_extract.py "$viv" "car_parts/$(basename $viv .viv)/"
done
```

## Missing Tools

- ASF → WAV/MP3 converter
- FST format parser
- Online.mdb schema dumper
- Track minimap generator (from Trmap.bin)
- Implode decompression (for compressed BIG/VIV entries)
