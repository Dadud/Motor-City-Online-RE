# PROJECT_STATE.md

_Status: documentation and preservation archive_
_Date: 2026-05-23_

## Summary

This repository is a **documentation and preservation archive** for Motor City Online. It collects historical media, file-format research, extracted reference data, archival tooling, and reconstructed reference notes — not a runnable game server or client.

Runnable server/client work belongs in separate community projects (for example [mcos](https://github.com/drazisil-codecov/mcos), [AZMCO](https://github.com/americusmaximus/AZMCO)).

## What is in the repo

### Documentation
- `docs/media/` — articles, reviews, screenshots
- `docs/formats/` — BIG, VIV, FSH, FCE, and related format notes
- `docs/research/` — protocol notes, build comparisons, emulator planning references

### Reference data
- `data/*.csv` — cars, parts, brands, engines (extracted from retail data)

### Archival tools
- `tools/*.py` — extraction and conversion helpers (user-supplied installs only)
- `tools/mco_scan/` — install scanner and `asset_manifest.json` generator

### Reference code (not a product)
- `src/` — annotated C stubs and notes reconstructed during research

## What is intentionally out of scope here

- Local shard / private server implementation
- Account, garage, lobby, or race runtime
- Bundled game assets or retail binaries
- CI for application servers

## Constraints

- Do not commit EA assets or leaked source
- Require user-supplied installs for tooling
- Mark speculative behavior as speculation in docs
