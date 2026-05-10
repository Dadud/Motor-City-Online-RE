# PROJECT_STATE.md

_Status: M0 audit complete_
_Date: 2026-05-10_

## Summary

This repository is primarily a reverse-engineering and preservation wiki for Motor City Online. It already contains:

- clean-room protocol research (`docs/research/NETWORK_PROTOCOL.md`)
- prior emulator planning docs
- reconstructed/reference C/C++ code for NPS/CASTANET and game systems under `src/`
- Python data extraction tools under `tools/`
- CSV gameplay/reference data under `data/`

It does **not** yet contain a runnable local shard/private server implementation for the preservation experience. This pass adds a new clean-room Python implementation alongside the research materials instead of reusing any copyrighted code.

## Existing repo shape

### Documentation
- `docs/research/NETWORK_PROTOCOL.md` — strongest protocol reference, donated clean-room spec
- `docs/research/SERVER_EMULATOR_PLAN.md` — earlier private server planning
- `docs/research/MCOS_OPEN_SOURCE_BUILD.md` — notes about mcos and message coverage
- `docs/formats/` — file format notes

### Reference / reconstructed code
- `src/npslib/` — CASTANET / NPS C implementation reference
- `src/game/` — physics/audio/render experiments and docs
- `src/authlogin/`, `src/mcity/` — reconstructed reference material

### Tooling / data
- `tools/*.py` — archive / texture / database extraction utilities
- `data/*.csv` — cars, parts, brands, attachment points, etc.

## Build/runtime audit

### Found build metadata before this pass
- No root `pyproject.toml`
- No root `package.json`
- `src/npslib/Makefile` exists for the C reference implementation

### Current implementation choice for local shard
Chosen stack for greenfield local shard:
- **Python 3.14** (already present in workspace)
- **FastAPI + SQLite**
- standard-library `sqlite3` for inspectable persistence
- CLI preservation client first, modern/original-client compatibility deferred

Reasoning:
- fastest path to ugly-but-working milestones
- simple local DB
- easy one-command startup target
- keeps clean separation from RE/reference code

## Relevant source files for shard work

### Highest-value research inputs
- `docs/research/NETWORK_PROTOCOL.md`
- `docs/research/SERVER_EMULATOR_PLAN.md`
- `docs/research/MCOS_OPEN_SOURCE_BUILD.md`
- `src/game/physics.md`
- `src/npslib/*`

### Gameplay/data seed candidates
- `data/Cars.csv`
- `data/Parts.csv`
- `data/Brand.csv`
- `data/DriverClass.csv`

## Broken or missing pieces before implementation
- no runnable server entrypoint
- no database schema/migrations for shard state
- no account/profile/garage APIs
- no preservation client
- no asset scanner for local install detection in the new stack
- no startup script for end-to-end local shard boot

## Constraints / guardrails
- Do not include EA assets or leaked source
- Require user-supplied installs/assets
- Keep RE notes separated from new implementation code
- Mark speculative behavior as speculation in docs

## M0 outcome

Recommended immediate next step: implement a Python local shard skeleton with:
1. config + SQLite init
2. seed data for dealerships/cars/parts/events
3. account/profile flow
4. garage/inventory/economy/dealership/event APIs
5. simple CLI proof of flow
