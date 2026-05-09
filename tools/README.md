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

**Location:** `research/mco/fce2obj.py`

#### frd2obj.py

Converts FRD track geometry files to OBJ format.

```bash
python3 frd2obj.py Data/Tracks/Boothill/Tr.frd output.obj
```

**Location:** `research/mco/frd2obj.py`

### Archive Extraction

#### big_extract.py

Extracts all files from a BIG archive.

```bash
python3 big_extract.py cars.big extracted/
```

**Reference:** [Camconn BIG decoder gist](https://gist.github.com/camconn/f9cf6ee31103070296f9bec89aa97831)

#### viv_extract.py

Extracts all entries from a VIV car archive (same as BIGF format).

```bash
python3 viv_extract.py Data/models/53chevy.viv extracted/
```

### Image Conversion

#### fsh2png.py (not yet written)

Converts FSH texture files to PNG format.

Requires implementing the SHPI / record type decoding per the [rewiki spec](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1)).

### Audio Conversion

#### bnk2wav.py (not yet written)

Converts BNK sound banks to WAV files.

Requires implementing the EA XA ADPCM decoder.

### Database

```bash
# List all tables
mdb-tables Online.mdb

# Export table to CSV
mdb-export Online.mdb CarModels > CarModels.csv
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

## Implode Decompression

Some BIG/VIV entries use PKWARE Implode compression. The game includes `IMPLODE.DLL` for this purpose.

Python reference implementation:
```python
# Use the implode library or reimplement from spec
# Parameters: dictionary_size=4096, compression_ratio=3
```

## Missing Tools

- FSH → PNG converter
- BNK → WAV converter
- ASF → WAV/MP3 converter
- FST format parser
- Online.mdb schema dumper
- Track minimap generator (from Trmap.bin)
