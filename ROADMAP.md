# ROADMAP.md

## Status legend
- [x] complete
- [~] in progress
- [ ] pending

## Milestones

- [x] **M0** Repo audit → `PROJECT_STATE.md`
- [x] **M1** Server skeleton + SQLite + account/login + seed data
- [x] **M2** Data models + migrations + persistence tests
- [x] **M3** Preservation client (CLI/TUI first)
- [ ] **M4** Race lobby flow (create/join/ready/start)
- [x] **M5** Driveable race scene (placeholder acceptable)
- [x] **M6** Asset integration via `mco-scan`
- [ ] **M7** Original-client compatibility research
- [ ] **M8** LAN multiplayer prototype

## This pass

### Completed
- audited repo and identified greenfield implementation path
- selected Python local shard stack for fastest clean-room progress
- created M0/M1 docs and implementation directories

### Completed
- HTTP local shard server
- SQLite schema + seed data
- account/profile/garage/dealership/event/lobby flows
- CLI preservation client proof
- asset scan stub and one-command local startup script

### Completed (this pass)
- `mco-scan` tool: magic-byte detection for VIV/BIG, FCE, FSH, BNK, FRD, FST, INI, BLF
- asset manifest generation (`asset_manifest.json`)
- car/track/parts classification by path patterns
- DB import utility to add discovered assets to shard
- `docs/tools/mco_scan.md` tool documentation

## Recommended next priorities
1. M7: original-client protocol compatibility research (NPS/MCOTS gateway)
2. M8: LAN multiplayer prototype (two clients on same shard)
3. Add WebSocket support for live lobby/chat/race state updates
4. Add real placeholder race scene (pygame or text-based drive loop)
