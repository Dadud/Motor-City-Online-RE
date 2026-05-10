# BLF — Bill of Materials Format

> Car part vertex segmentation for customization and damage modeling.

**Magic:** `0x00000B52` (little-endian uint32 = 2898 decimal)  
**Location:** Inside each car VIV as `part.blf`  
**Entry size:** 12 bytes  
**Purpose:** Maps vertex index ranges to car parts for per-vertex damage and customization

## Header

| Offset | Size | Value | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `0x00000B52` | Magic / entry count |

Total entries = magic value = 2898 (for 53chevy).

File size = `2898 × 12 + 4 = 34,780 bytes`.

## Entry Structure (12 bytes each)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `type` | Entry type (see below) |
| 0x04 | 4 | `start` | Start vertex index (1-based, matches FCE vertices 0–1221) |
| 0x08 | 4 | `end` | End vertex index (1-based) |

## Entry Types

| Type | Count (53chevy) | Description |
|------|----------------|-------------|
| 0 | 42 | Separators / markers between car sections |
| 1 | 2353 | Main car part definitions |
| 2 | 502 | Sub-parts within car sections |
| 2898 | 1 | Magic / sentinel entry at offset 0 |

## How It Works

The BLF segments the FCE vertex data into car parts. Each entry's `start` and `end` define a vertex index range belonging to a specific part.

Example (from 53chevy):
```
type=1, start=1, end=341    → hood vertices
type=1, start=342, end=512   → front windshield vertices
type=2, start=513, end=524   → hood scoop sub-part
```

**Overlapping ranges** indicate shared vertices (like wheel hubs that connect multiple body panels).

**Type 0 entries** are separators/markers placed between major car sections (e.g., between hood and windshield), used for UI display or damage zone boundaries.

## Status

✅ **Partially decoded** — structure is known. Full chunk type list for type 2 entries is still needed.
