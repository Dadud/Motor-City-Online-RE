# Research Notes

Game system analysis, build comparisons, and protocol documentation.

## Build Analysis

| Topic | File | Status |
|-------|------|--------|
| Beta 1 (Jun 2001) | [beta1.md](beta1.md) | Verified |
| Oct09 Prototype | [oct09.md](oct09.md) | Verified |

## System Analysis

| Topic | File | Status |
|-------|------|--------|
| Patch System | [patch-system.md](patch-system.md) | Partial — pipeline confirmed; offline mode partial |
| Network Protocol | [network.md](network.md) | Partial — donated by Molly; serialization unknown |
| EXE Architecture | [exe-architecture.md](exe-architecture.md) | Partial — NPS subsystem mapped |

## Community Documentation

The `research/NETWORK_PROTOCOL.md` file in the repo root contains a fuller protocol specification donated by Molly. That file is not yet integrated into this structure.

## Build Comparison

Key differences between Beta 1 (Jun 2001), Oct09 Prototype, and Final Retail:

| Feature | Beta 1 | Oct09 | Final |
|---------|--------|-------|-------|
| Archive format | Loose files | Mixed | BIG archives |
| Tracks | 17 tracks | 16 (Derby removed) | 16 + new tracks |
| Tracks removed | — | Derby, Gravel, Obstacle | — |
| Tracks added | — | Hazard | Hazard, Boval |
| Debug EXE | Yes | Yes | Yes (MCity_d.exe) |
| engpatch.viv | Yes | Yes | Yes (identical SHA-1) |
