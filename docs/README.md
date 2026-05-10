# Motor City Online — Documentation

This directory contains the structured documentation for MCO reverse engineering.

```
docs/
├── README.md         — This file
├── index.md          — Full documentation index with evidence ratings
├── formats/         — Binary file format documentation
│   ├── README.md    — Format index
│   ├── BIG.md       — Archive container
│   ├── VIV.md       — Archive container (per-car)
│   ├── FCE.md       — Car geometry
│   ├── FST.md       — Per-car metadata
│   ├── FRD.md       — Track road geometry
│   ├── BNK.md       — Audio banks
│   ├── FSH.md       — Textures
│   ├── BLF.md       — Vertex segmentation
│   ├── LOD.md       — LOD distances
│   ├── INI.md       — Track config
│   ├── TRK.md       — AI racing line
│   ├── DATABASE.md  — Access database
│   └── engpatch.md  — Engine audio patches
└── research/        — Game system analysis
    ├── README.md    — Research index
    ├── beta1.md     — Beta 1 build analysis
    ├── oct09.md     — Oct09 prototype analysis
    ├── patch-system.md    — Patch pipeline
    ├── network.md         — Network protocol
    └── exe-architecture.md — EXE structure
```

## Evidence Levels

| Level | Tag | Meaning |
|-------|-----|---------|
| 0 | Unverified | Guess or indirect clue |
| 1 | Observed | Seen in one sample |
| 2 | Reproduced | Confirmed on multiple samples |
| 3 | Cross-checked | Confirmed through independent paths |
| 4 | Operational | Suitable for implementation |

Claims are tagged like: `[E2]` — Level 2, reproduced on multiple samples.
