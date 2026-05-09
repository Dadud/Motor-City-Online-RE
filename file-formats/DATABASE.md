# MDB — Access Database Format

> Car statistics, parts, physics parameters, and track data.

**Format:** Microsoft Access 2000 (Jet DB)  
**Location:** `Data/DB/Online.mdb`  
**Game use:** All car stats, customization parts, physics tuning, track definitions

## Database Tables

The `Online.mdb` database contains tables for:

| Table | Description |
|-------|-------------|
| `CarMakes` | Manufacturer brands (Ford, Chevy, etc.) |
| `CarModels` | Individual car model definitions |
| `CarPhysics` | Per-model physics parameters (weight, power, etc.) |
| `CarParts` | Customization parts (body kits, hoods, wheels) |
| `PartStats` | Part statistics and unlock requirements |
| `Tracks` | Track definitions and metadata |
| `TrackSegments` | Per-segment track data |
| `AudioTracks` | Music track listings |
| `Textures` | Texture references and assignments |

## CarPhysics Fields (estimated)

| Field | Description |
|-------|-------------|
| `ModelID` | Car model identifier |
| `Weight` | Curb weight in pounds |
| `Power` | Horsepower |
| `Torque` | Torque in lb-ft |
| `TopSpeed` | Maximum speed in mph |
| `Acceleration` | 0-60 time or acceleration curve |
| `Handling` | Grip / handling rating |
| `Durability` | Damage resistance |

## CarParts Fields (estimated)

| Field | Description |
|-------|-------------|
| `PartID` | Unique part identifier |
| `ModelID` | Which car model it belongs to |
| `Category` | Body, wheels, exhaust, etc. |
| `Price` | In-game cost |
| `UnlockLevel` | Required player level |

## Accessing the Database

The database can be opened with:
- **Microsoft Access** (Windows only)
- **mdb-tools** (Linux: `apt install mdb-tools`)
- **UCanAccess** (Java, cross-platform)

### Linux command-line extraction:

```bash
# List tables
mdb-tables Online.mdb

# Export a table to CSV
mdb-export Online.mdb CarModels > CarModels.csv
mdb-export Online.mdb CarPhysics > CarPhysics.csv
mdb-export Online.mdb Tracks > Tracks.csv
```

### Python access:

```python
# Requires pyodbc or mdb-tools + subprocess
import subprocess
result = subprocess.run(['mdb-export', 'Online.mdb', 'CarModels'],
                       capture_output=True, text=True)
print(result.stdout)
```

## Open Questions

- Full table schema (field names and types)
- How physics values map to gameplay behavior
- Car part compatibility matrix
- How track segment data defines the racing line
- Whether the database has any deleted/unused records
