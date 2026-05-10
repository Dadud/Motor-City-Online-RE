# ENGPATCH — Engine Audio Patch System

> Engine audio archive and patch selection system.
> **Status:** 🔶 Partial — archive structure confirmed; runtime selection unknown.

---

## Archive Structure

`engpatch.viv` is an EA **BIGF archive** containing engine audio banks and parameter tables.

**Verified contents:**
- 499 × `.bnk` audio bank files
- `engine.rpm` — engine RPM curve data
- `engine.htb` — high band torque table (?)
- `engine.btb` — bottom band torque table (?)
- `engine.ctb` — center band torque table (?)
- `engine.ltb` — low band torque table (?)

**NOT present:** `engine.txt` (referenced in EXE but not in archive)

### Archive Verification

```
Archive:     ENGPATCH.VIV (final retail)
Magic:       BIGF (verified)
Entries:     504 total
             499 × .bnk
             5 × engine.* files
SHA-1:       2f88a489a67318338de51de842a6044b8deadbbf
```

SHA-1 is **identical across all three builds** (Beta1, Oct09, Final) — engpatch.viv was not updated by any patch. [E2]

---

## MDB Engine Patch Keys

`Online.mdb` contains 19 unique `engpatch...` keys. These are structured 15-character selectors assembled from 5 three-character component tokens.

### 19 Unique Keys

| Key Family | Tokens | Notes |
|-----------|--------|-------|
| `engpatchaaxaaxmuxcuxfax` | aax/aax/mux/cux/fax | No populated rows |
| `engpatchaaximpmuxcuxdrg` | aax/imp/mux/cux/drg | |
| `engpatchaarimpmuxcuxcmx` | aar/imp/mux/cux/cmx | |
| `engpatchblxblxmuxcuxdrx` | blx/blx/mux/cux/drx | |
| `engpatchblzblzmuxcuxdrx` | blz/blz/mux/cux/drx | |
| `engpatchchxchshixcuxdrx` | chx/chs/hix/cux/drx | |
| `engpatchcsxcasecxluxfax` | csx/cas/ecx/lux/fax | 18 rows — most complete |
| `engpatchctxctkhixcuxdrx` | ctx/ctk/hix/cux/drx | |
| `engpatchcudcudmuxcuxdrg` | cud/cud/mux/cux/drg | |
| `engpatchcuxcudhixcuxdrx` | cux/cud/hix/cux/drx | |
| `engpatchcuxelchixblxdrg` | cux/elc/hix/blx/drg | |
| `engpatchcvxchvluxcuxdrg` | cvx/chv/lux/cux/drg | |
| `engpatchelxelcmuxblxcmx` | elx/elc/mux/blx/cmx | |
| `engpatchimpimpmuxcuxfat` | imp/imp/mux/cux/fat | |
| `engpatchimpimpmuxcuxfax` | imp/imp/mux/cux/fax | |
| `engpatchimximpmuxcuxfax` | imp/imp/mux/cux/fax | |
| `engpatchmuximpmuxcuxcmx` | mux/imp/mux/cux/cmx | |
| `engpatchsnxsnbecxblxdrg` | snx/snb/ecx/blx/drg | |
| `engpatchsnxsnbecxblxdrx` | snx/snb/ecx/blx/drx | |

18 of 19 keys have populated rows. Total populated rows: 59. [E2]

### Key Token Vocabulary

Every token in the MDB keys exists as a BNK filename prefix in the archive. Token positions map to specific component classes:

| Position | Tokens | Likely Meaning |
|----------|--------|----------------|
| Pos 1-2 | Various | Engine family/base |
| Pos 3 | mux/hix/ecx/lux | Curve/table selector |
| Pos 4 | cux/blx/lux | Load band selector |
| Pos 5 | drx/drg/fax/cmx/fat | Final audio bank suffix |

### BNK Suffix Classes

All 499 BNK files in the archive fall into two suffix classes:

**Full class (13 suffixes):**
`drg, fat, drx, cux, cmx, fax, ddx, dbx, dgx, drz, fbx, fix, frx`

**Reduced class (7 suffixes):**
`drx, drg, fax, cmx, drz, fbx, frx`

**Critical finding:** Position 5 (final token) always comes from the reduced 7-suffix class. [E2]

---

## MDB Row Structure

MDB rows associated with `engpatch...` keys contain:

```python
struct EngPatchRow:
    char   key[24];           # 24-byte fixed key (not null-terminated)
    # [unknown gap — Jet row encoding]
    int    int1;              # Range: 160001–160078 (row identifiers)
    int    int2;              # Always 1
    int    int3;              # Usually 2, sometimes 1
    int    int4;              # Range: 0–3 (engine tier/band selector)
    double double1;            # Range: 45–120 (likely RPM or power factor)
    double double2;           # Range: ~0.08–0.11
    double double3;           # Range: ~0.07–0.11
    double double4;           # Range: ~3.6–8.2
    double double5;           # Always 8.0
```

**Status:** Key vocabulary and value ranges are confirmed. Exact byte offsets within Jet rows are uncertain (requires proper Jet row parser). [E1]

---

## Engine Table Format

The five `engine.*` files inside the archive use **Creative Labs CRD format** (magic: `CRDl`).

### CRDl Header (all 5 files)

```
Offset 0x00: "CRDl" magic
Offset 0x04: version (2 for engine.rpm, 0x64000002 for table files)
Offset 0x0C: ??? = 500 (logical entry count)
Offset 0x10: ??? = 200 (logical bytes per entry)
Offset 0x14: ??? = 100 (some multiplier)
```

### Data Interpretation

The CRDl header claims 500 entries × 200 bytes = 100,000 bytes of logical data. Actual file sizes (1,384–6,504 bytes) are much smaller because the data is **sparse** — only non-zero entries are stored.

`engine.rpm` data begins with `0x63` (99 decimal) — possibly an RPM base value.

`engine.ltb` data shows a repeating pattern: `[index, index, 0xFF, index, index, 0xFF, ...]` — index pairs separated by `0xFF` delimiters, followed by descending byte sequences at offset ~264.

**Status:** Format is Creative Labs CRD (used in SoundBlaster parameter files). Sparse storage confirmed. Byte-level meaning of curve data unknown without CRD specification or disassembly. [E1]

---

## Component Banks

Five named BNK files outside the main 499 appear to be separately loaded component banks:

| Name | Likely Meaning |
|------|---------------|
| `hitap` / `mdtap` / `lotap` | Tappet/cam follower banks |
| `hirod` / `mdrod` / `lorod` | Connecting rod banks |
| `hihead` / `mdhead` / `lohead` | Cylinder head banks |
| `hiblow` / `mdblow` / `loblow` | Blower/supercharger banks |
| `shift` | Shift鸣 audio bank |

EXE strings confirm: `"Head:%d Rod:%d"` logging references these components. [E2]

---

## Runtime Loading

**Status:** 🔴 Unknown — not traced in disassembly.

EXE strings confirm:
- `engpatch.viv` is loaded via NPSPush/PushPatch path
- `engine.rpm`, `engine.htb`, `engine.btb`, `engine.ctb`, `engine.ltb` are read from the archive
- `engine.txt` is referenced (`%sengine.txt`) but not present in the shipped archive
- MDB `engpatch...` keys select which BNK files and engine parameters are used

The exact function mapping MDB keys → BNK selection → runtime audio has not been traced.

---

## See Also

- `tools/viv_extract.py` — archive extractor (works)
- `research/mco/16-engpatch-trace.md` — detailed evidence trace (local only)
