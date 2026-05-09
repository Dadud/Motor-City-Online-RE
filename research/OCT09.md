# Oct 09 Prototype (WebBeta 2) — Research Notes

> Second and final beta build of Motor City Online, released ~3 weeks before the Oct 31, 2001 final release.

## Overview

| Property | Value |
|---|---|
| Build date | October 9, 2001 |
| Build name | WebBeta 2 |
| Zip size | 1.3 GB |
| Installed size | 616 MB |
| Executable | `mcity.exe` (4.9 MB, Oct 9 2001) |
| Debug exe | **None** |
| File storage | **Loose files** (no BIG archives) |

## How to Get

Available on Hidden Palace:
```
https://hiddenpalace.org/Motor_City_Online_(Oct_9,_2001_prototype)
```
Note: `files.hiddenpalace.org` CDN returned 404 at time of research. Try again later or check for mirrors.

## Executable Comparison

| | Beta 1 (Jun 27) | Oct 09 (Oct 9) | Final |
|---|---|---|---|
| Filename | `MCity.exe` | `mcity.exe` | `MCity.exe` |
| File size | 4.1 MB | 4.9 MB | 5.1 MB |
| Build date | Jun 27 2001 | Oct 9 2001 | Oct 2001 |
| Case | PascalCase | lowercase | PascalCase |

The Oct 09 `mcity.exe` (lowercase) is notable — this is the only build with a lowercase executable name.

## Track Changes (Beta 1 → Oct 09)

| Track | Beta 1 | Oct 09 |
|---|---|---|
| Derby | ✅ | ❌ Removed |
| Dirtoval | ✅ | ✅ |
| Gravel | ✅ | ❌ Removed |
| Hazard | ❌ | ✅ **New** |
| Obstacle | ✅ | ❌ Removed |
| ParkA | ✅ | ✅ (renamed to Parka) |
| All others | ✅ | ✅ |

**Summary:** 4 tracks changed — 3 removed (Derby, Gravel, Obstacle), 1 renamed (ParkA→Parka), 1 added (Hazard).

## File Count Comparison

| | Beta 1 | Oct 09 | Delta |
|---|---|---|---|
| Total VIV files | 304 | 628 | +324 |
| Tracks | 17 | 16 | -1 |
| Installed size | 1.3 GB | 616 MB | -53% |

The installed size dropped significantly despite more VIV files, suggesting better compression or removal of debug/unused data.

## Key Observations

1. **No BIG archives** — Like Beta 1, this prototype ships with all files loose. The BIG packing was added only in the final release distribution.
2. **Debug exe still absent** — `MCity_d.exe` first appears in the final release, not in either beta. (Note: likely a 2012 offline patch rebuild rather than an original EA debug build.)
3. **Case change in exe name** — Only the Oct 09 build has `mcity.exe` (lowercase). Both Beta 1 and Final use `MCity.exe` (PascalCase).
4. **Engine patch file** — `engpatch.viv` is present (same as Beta 1), confirming it was always part of the plan.
