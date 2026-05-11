# Motor City Online — Preservation Archive

Documentation, media, and archival resources for **Motor City Online** (EA Seattle, 2001–2003), a PC online racing MMO and spin-off of the Need for Speed franchise.

> This project collects and preserves historical information about a game whose official servers shut down in August 2003. All work is for preservation and educational purposes.

---

## What This Archive Contains

### 📰 Media Coverage — `docs/media/`

Preserved articles, reviews, screenshots, and promotional material from 2001–2019.

| Source | Date | Type |
|--------|------|------|
| CNN / GamePro | Jun 2001 | Preview |
| IGN | Feb–Nov 2001 | Preview + Review (7.9/10) |
| GameSpot | Feb–Oct 2001 | Preview + Review (7.6/10) |
| Metacritic | Nov–Dec 2001 | Aggregated score (73/100, 12 critics) |
| PC Games (Germany) | 2001 | Review (88%) |
| My Abandonware | — | Preservation guide + Offline Launcher |
| Quarter to Three | May 2001 | E3 Preview |
| PC Zone Issue 109 | Dec 2001 | Preview |
| PC Powerplay Issue 064 | Sep 2001 | Reader Q&A |
| Old-Games.RU | — | **47 Screenshots** |
| Hidden Palace | 2026 | Prototype info + 2 screenshots |
| AssemblerGames | 2010–2019 | Preservation thread |
| Blue's News | Sep 2001 | Beta download (428MB) |
| GameSurge | Sep 2000 | Rename announcement |

Also includes:
- North American box art
- **My Abandonware preservation guide** — Full installation instructions for modern Windows
- **Community preservation efforts** — MCO Re-Fired, Offline Launcher, post-shutdown history
- **Original Soundtrack** — Full track listing, credits, and review quotes
- **Beta download history** — Blue's News open beta (428MB, Sep 2001)
- **Computer Gaming World Issue 210** (Jan 2002) PDF reference — MCO preview on page 36
- **PC Gamer Issue 87** (Aug 2001) OCR reference — E3 wrap-up coverage

See [`docs/media/INDEX.md`](docs/media/INDEX.md) for the full catalog.

---

### 📋 Game Database — `data/`

Extracted and preserved game data from the retail build:

| File | Contents |
|------|----------|
| `Cars-complete.csv` | 83 unique car IDs, 72 base models |
| `Brand.csv` | 103 manufacturers |
| `StockEngines.csv` | 26 stock engine configurations |
| `Parts.csv` | Part definitions |
| `AbstractPartTypes.csv` | Part categories |
| `AttachmentPoint.csv` | 12 attachment points |

---

### 🖼️ Car Previews — `data/cars/`

PNG previews of 13 in-game car models extracted from the retail data.

---

### 📁 File Format Documentation — `docs/formats/`

Documented file formats used by the game. These are living notes based on analysis of the retail build and community research.

| Format | Status | Notes |
|--------|--------|-------|
| BIG / VIV | Documented | EA archive containers (BIG4 + BIGF variants) |
| FCE | Documented | Car geometry (FCE4M variant) |
| FSH | Documented | Textures (SHPI container, multiple pixel formats) |
| LOD | Documented | Model LOD distance thresholds |
| INI | Documented | Track configuration files |
| FRD | Partial | Road surface documented; native mesh topology unknown |
| FST | Partial | Header documented; full record semantics unclear |
| BLF | Partial | Vertex segmentation structure known |
| MDB | Partial | Schema extracted from Access database |
| BNK | Partial | Header parsed; audio codec not identified |
| TRK | Unknown | AI racing line; no analysis yet |

---

### 🔧 Archival Tools — `tools/`

Small utilities for inspecting game files. These are helper scripts, not a reconstruction project.

| Tool | Purpose |
|------|---------|
| `viv_extract.py` | Extract files from VIV archives |
| `big_extract.py` | Extract files from BIG archives |
| `fsh2png.py` | Convert FSH textures to PNG |
| `iso_extract.py` | Inspect game ISO layout |
| `refpack_decompress.py` | Decompress RefPack-compressed data |
| `mdb_extract.py` | Read small tables from Online.mdb |
| `bnk2wav.py` | Attempt BNK audio extraction (codec unknown) |

---

### 📝 Research Notes — `docs/research/`

Historical analysis and build comparisons:

| Topic | File |
|-------|------|
| Beta 1 Build (Jun 2001) | `beta1.md` |
| Oct09 Prototype | `oct09.md` |
| Patch System History | `patch-system.md` |
| Network Protocol Overview | `network.md` |
| EXE Structure Notes | `exe-architecture.md` |
| Community Tools Catalog | `EXISTING_TOOLS.md` |
| MCO Update Notices Archive | `mco-update-notices-archive-scrape-20020608.md` |

---

### 🎮 Client/Server Code — `src/`, `client/`, `server/`

**Note:** These are reconstructed stubs and reference implementations created during earlier research. They are not original source code and are not under active development. They are preserved here as-is for historical reference.

- `src/` — Annotated C stubs reconstructed from disassembly
- `client/` — Reference preservation client (Python)
- `server/` — Reference shard server (Python)

---

## Build Versions Documented

| Build | Date | Notes |
|-------|------|-------|
| Beta 1 | Jun 27, 2001 | Loose files, 17 tracks, debug EXE |
| Oct09 Prototype | Oct 09, 2001 | Offline patch available, mco_log.txt |
| Final Retail | Oct 31, 2001 | BIG archives, MCity_d.exe debug symbols |
| Offline Patch | ~Mar 2003 | Content overlay for single-player |

---

## Community & Post-Shutdown Preservation

After official servers shut down in August 2003, the community kept the game alive:

- **Offline Launcher** — Community patch enabling single-player AI races on modern Windows. Included with the My Abandonware release.
- **MCO Re-Fired** — Fan project that developed a custom launcher, Car Builder tool, and mods for new cars/tracks. Forum: mcorefired.com
- **'99 Dodge Viper mod** — Restores a car cut from the final game (1 MB).
- **Reddit:** r/MotorCityGameMemories — Community memories and reconnections.
- **Facebook:** "Motor City Online" group — Active file-sharing and support community.

See `docs/media/16-community-preservation-efforts/` for full details.

---

## Related Preservation Projects

- **[mcos](https://github.com/drazisil-codecov/mcos)** — Open-source MCO server recreation (TypeScript/Node.js, connects to lobby)
- **[AZMCO](https://github.com/americusmaximus/AZMCO)** — Open-source MCO recreation with renderer, physics, and audio
- **[EA BIG Archive — rewiki](https://rewiki.miraheze.org/wiki/EA_BIG_Archive)** — Community documentation for EA archive formats
- **[EA SSH FSH Image — rewiki](https://rewiki.miraheze.org/wiki/EA_SSH_FSH_Image_(Type_1))** — Community texture format documentation

---

## Contributing

This archive accepts:
- Historical articles, reviews, or screenshots
- Corrections to documentation with supporting evidence
- References to additional preservation efforts

This archive does NOT accept:
- Game ROMs or copyrighted assets
- Speculation presented as fact

---

*Last updated: 2026-05-11*
