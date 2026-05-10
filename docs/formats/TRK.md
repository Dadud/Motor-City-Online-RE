# TRK — Track Routing / AI Graph Format

> Track racing line and AI opponent routing data.

**Location:** `Data/Tracks/<name>/tr.trk`  
**Format:** Binary, unknown structure  
**Purpose:** Defines the racing line, AI opponent paths, and checkpoint order

## Observations

- Present in every track directory
- Contains repeated marker values that may correspond to track segments
- Contains `0xFFAB5A55` repeating markers (similar to `sliceInfo.bin`)
- May contain per-segment AI speed limits or steering data

## Header Fields (estimated)

| Field | Description |
|-------|-------------|
| `SliceCount` | Number of track segments / slices |
| `AIWaypoints` | Array of 3D points for AI racing line |
| `Speeds` | Per-segment speed limits for AI |
| `Checkpoints` | Lap / checkpoint detection zones |

## Open Questions

- Exact binary structure (uint16 vs uint32 indices?)
- Whether the AI racing line is embedded or generated
- How pit lane routing is defined
- How rubber-banding / catch-up mechanics are parameterized

## Status

❌ **Not decoded** — requires more analysis.
