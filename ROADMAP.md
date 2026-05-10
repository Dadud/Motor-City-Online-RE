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
- [ ] **M6** Asset integration via `mco-scan`
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

## Recommended next priorities after this pass
1. add tests around persistence and economy rules
2. add websocket/session updates for lobby/chat state
3. add placeholder race session loop and reward payout
4. start asset-manifest generation from user-supplied install
