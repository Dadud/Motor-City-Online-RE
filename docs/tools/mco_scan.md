# mco-scan — Asset Manifest Tool

Scans a user-supplied MCO install directory, detects file types by magic bytes, and generates an `asset_manifest.json` describing all discovered assets.

## Usage

```bash
# Basic scan (from repo root)
PYTHONPATH=. python3 -m tools.mco_scan.cli /path/to/mco/install

# Custom output path
python -m tools.mco_scan.cli /path/to/mco/install --manifest my_manifest.json

# Limit files scanned
python -m tools.mco_scan.cli /path/to/mco/install --max-files 10000

# JSON output only
python -m tools.mco_scan.cli /path/to/mco/install --json-only
```

## How It Works

1. **Recursive scan** — walks the install directory tree, skips system/temp files
2. **Magic byte detection** — reads file headers to identify format, not just extension
3. **Path classification** — matches file paths against known car/track/parts naming patterns
4. **Manifest generation** — outputs JSON describing every discovered asset

## Magic Byte Signatures

| Format | Magic Bytes | Detection |
|--------|-------------|-----------|
| BIG4 (VIV archive) | `BIG4` @ offset 0 | High confidence |
| BIGF (VIV archive) | `BIGF` @ offset 0 | High confidence |
| BIGH (VIV archive) | `BIGH` @ offset 0 | High confidence |
| FCE3 (collision geo) | `FCE3` @ offset 0 | High confidence |
| FCE4 (collision geo) | `FCE4` @ offset 0 | High confidence |
| FCE4M (collision geo) | `FCE4M` @ offset 0 | High confidence |
| FSH (texture) | `SHPI` @ offset 0 | High confidence |
| FST (car exterior) | GUID `e0134678-c995-d111-960a-0010-5ae42069` @ offset 0x54 | High confidence |
| FRD (track) | `DEADBEEF` block marker in first 256 bytes | High confidence |
| BNK (audio) | `BNK` or `RIFF` header | Medium confidence |
| INI (config) | Starts with `[` or `;` | Medium confidence |

## Asset Types

| Type | Detection Method |
|------|-----------------|
| `car` | VIV filename matches known car key (e.g., `53chevy.viv`), or `:Hbody` FCE section |
| `track` | FRD/FST file in tracks/ directory |
| `parts` | FCE/VIV file in parts/ or perf/ directory |
| `audio` | BNK file in audio/ or sfx/ directory |
| `texture` | FSH file anywhere |
| `config` | INI, BLF, MDB, ENG files |
| `unknown` | Cannot classify |

## Manifest Format

```json
{
  "version": "1.0",
  "scan_root": "/path/to/mco",
  "scanned_at": "2026-05-11T00:00:00Z",
  "total_files_scanned": 1234,
  "total_bytes": 987654321,
  "summary": {
    "cars": 8,
    "tracks": 3,
    "parts": 24,
    "audio": 12,
    "textures": 150,
    "configs": 20,
    "unknown": 45
  },
  "assets": [
    {
      "type": "car",
      "path": "cars/53chevy.viv",
      "absolute_path": "/mco/cars/53chevy.viv",
      "file_type": "viv_big4",
      "file_type_confidence": "high",
      "size_bytes": 2048576,
      "model_key": "53chevy",
      "display_name": "1953 Chevrolet",
      "magic_hex": "42494734...",
      "known": true,
      "note": null
    }
  ]
}
```

## Database Import

Discovered assets can be imported into the local shard database:

```python
from tools.mco_scan.db_import import import_manifest_to_shard_db

stats = import_manifest_to_shard_db(
    manifest_path="asset_manifest.json",
    db_path="local_shard.db",
    provenance="user_supplied"
)
print(stats)
# {'cars_added': 5, 'tracks_added': 2, 'skipped': 3}
```

Or via CLI (future enhancement):
```bash
python -m tools.mco_scan.cli /path/to/mco/install --import-to-db local_shard.db
```

## Files

- `tools/mco_scan/cli.py` — CLI entry point
- `tools/mco_scan/scanner.py` — main scan logic
- `tools/mco_scan/magic.py` — magic byte signatures
- `tools/mco_scan/detectors.py` — path-based classification
- `tools/mco_scan/db_import.py` — DB import utility

## Notes

- This is a **preservation-only tool** — no copyrighted assets are bundled
- Users must provide their own MCO install
- Detected file types are inferred from magic bytes and path patterns
- Unknown files are still logged with their magic bytes for future research
- All format detection is clean-room — written from format research, not from copying existing tools
