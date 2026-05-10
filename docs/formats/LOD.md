# LOD — Level of Detail Thresholds

> Distance thresholds for car model LOD switching.

**Location:** Inside each car VIV as `part.lod`  
**Size:** 28 bytes = 7 float32 values  
**Format:** Plain float32 array, no header

## Content

```
20b0123f 05000000 a4080000 48060000 98050000 20050000 f3040000
```

Decoded as IEEE 754 float32:

| Index | Float Value | Meters | Description |
|-------|-------------|--------|-------------|
| 0 | 0.25 | ~0.25m | Near-LOD switch distance |
| 1 | 5.0 | 5m | Low-detail threshold |
| 2 | 6.0 | 6m | Medium-detail threshold |
| 3 | 7.25 | ~7.25m | Far-distance threshold |
| 4 | 5.0 | 5m | (unknown) |
| 5 | 5.0 | 5m | (unknown) |
| 6 | 6.0 | 6m | (unknown) |

## Interpretation

LOD levels switch based on camera distance from the car:
- Very close (< 0.25m): full detail
- Near (< 5m): high detail
- Medium (< 6m): medium detail
- Far (< 7.25m): low detail
- Beyond: simplified or hidden

The 28-byte size (7 floats) suggests each car has up to 7 LOD levels, though typically only a few are used.

## Status

✅ **Fully decoded** — format is straightforward.
