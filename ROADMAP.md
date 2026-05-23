# ROADMAP.md

Preservation archive priorities. This repo does **not** track runnable server or client milestones.

## Status legend

- [x] complete
- [~] in progress
- [ ] pending

## Archive milestones

- [x] Media coverage catalog (`docs/media/INDEX.md`)
- [x] Car and parts CSV extracts (`data/`)
- [x] Core format documentation (BIG, VIV, FSH, FCE, INI, LOD)
- [~] Partial formats (FRD, FST, BLF, MDB, BNK)
- [ ] TRK (AI racing line) analysis
- [x] Build comparison notes (Beta 1, Oct09, retail, offline patch)
- [x] Network protocol research index
- [x] Archival extraction tools (`tools/`)
- [x] Install scanner manifest tool (`tools/mco_scan/`)

## Recommended next work (archive-only)

1. Finish FRD / FST / BLF format semantics where evidence exists
2. Document TRK format from retail builds
3. Expand `docs/media/` with sourced citations only
4. Keep CSV data aligned with documented schema (`docs/formats/DATABASE.md`)
5. Link to external server projects instead of duplicating runtime code here

## Related implementation efforts (external)

- [mcos](https://github.com/drazisil-codecov/mcos) — TypeScript server recreation
- [AZMCO](https://github.com/americusmaximus/AZMCO) — open-source client/engine effort
