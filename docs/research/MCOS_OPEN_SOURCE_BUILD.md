# MCO Open Source Server — Implementation Plan

_Draft: 2026-05-10_
_Status: Planning_

## Goal

Build a complete, verified open-source MCO private server by forking the existing mcos project and systematically implementing missing message handlers, fixing incorrect implementations, and building out game server functionality.

---

## What Exists

### mcos (drazisil-codecov/mcos)
TypeScript/Node.js monorepo. Has achieved lobby connection (Oct 2023).

**Packages:**
- `nps` — NPS protocol (EncryptionSession, UserStatusManager)
- `mcots` — MCOTS transaction server (partial)
- `shard` — Game server (basic)
- `gateway` — SSL gateway (handles old ciphers)
- `database` — PostgreSQL (MikroORM)
- `shared-packets` — Packet definitions (ISerializable interface)
- `connection` — Connection handling
- `patch` — Client patch tools
- `cli` — CLI tools

**Implemented MCOTS message handlers:**
- `processClientConnect.ts` — msg 438 (ClientConnect)
- `processClientTracking.ts` — tracking
- `processServerLogin.ts` — msg 105 (Login)
- `processSetOptions.ts` — msg 109 (SetOptions)
- `processStockCarInfo.ts` — msg 141 (StockCarInfo)
- `sendSuccess.ts` — msg 101 (Success)

**Implemented MCOTS payloads:**
- `ClientConnectMessage`, `ClientTrackingMessage`, `LoginMessage`, `LoginCompleteMessage`, `SetOptionsMessage`, `StockCar`

### Molly's Protocol Spec (`docs/research/NETWORK_PROTOCOL.md`)
2000+ line comprehensive specification donated by Molly.

**Covers:**
- NPS header format, message IDs 0x100–0x600
- MCOTS message catalog: 40+ message types across 5 categories
- Connection flows (login, lobby, MCOTS selection)
- Encryption: DES-CBC for NPS, ARC4 for MCOTS
- Compression: PKWARE DCL inflate/deflate
- Race protocol: create, join, start, complete, results
- Full binary structure for all major messages

### Our Source Reconstructions (`src/`)
- `npslib/castanet.c/h` — 1400+ lines, working CASTANET implementation
- `npslib/nps.c/h` — 1700+ lines, NPS API (login, persona, buddy, mail, chat, rooms)
- `game/physics.md` — Physics simulation system (PSimAero, PSimEngine, etc.)
- `game/psim.c/h` — Physics engine source
- `game/audio.c/h` — Audio engine
- `game/render.c/h` — Rendering system
- `authlogin/authlogin.cpp` — authlogin.dll decompilation
- `mcity/mcity_core.cpp` — mcity.exe core decompilation

---

## Critical Gap: LoginCompleteMessage (msg 213)

**Molly's spec (section 5.6):**
```
LoginCompleteMsg =
    msgNo: WORD ,
    serverTime: DWORD ,
    flags: BYTE ,              (* bit 0: firstTime, bit 1: paycheckWaiting *)
    secondsTillShutdown: WORD ,
    shardGNP: double64 ,       (* 8 bytes FLOAT64 *)
    shardCarsSold: DWORD ,
    shardAveSalary: DWORD ,
    shardAveCarsOwned: DWORD ,
    shardAvePlayerLevel: DWORD ,
    serverList: 4 * MCOTSListEntry ,
    webCookie: DWORD ,
    nextTallyDate: Timestamp ,
    nextPaycheckDate: Timestamp ;
```

**mcos implementation:** Reads `shardGNP` as `UInt32LE` (4 bytes) — WRONG size.
Also missing `serverList: 4 * MCOTSListEntry` entirely.

This explains why login succeeds but client may not get proper server list.

---

## MCOTS Message Implementation Status

### Connection & Session (6 messages)

| Code | Name | Status | Priority |
|------|------|--------|----------|
| 101 | MC_SUCCESS | ✅ mcos (sendSuccess.ts) | — |
| 102 | MC_FAILED | ❌ Missing | HIGH |
| 105 | MC_LOGIN | ✅ mcos (processServerLogin.ts) | — |
| 106 | MC_LOGOUT | ❌ Missing | MED |
| 213 | MC_LOGIN_COMPLETE | ⚠️ Wrong (double64 bug, missing serverList) | CRITICAL |
| 438 | MC_CLIENT_CONNECT_MSG | ✅ mcos | — |

### Player & Persona (5 messages)

| Code | Name | Status | Priority |
|------|------|--------|----------|
| 108 | MC_GET_PLAYER_INFO | ❌ Missing | HIGH |
| 109 | MC_SET_OPTIONS | ✅ mcos | — |
| 122 | MC_PLAYER_INFO | ❌ Missing | HIGH |
| 164 | MC_GET_PLAYER_NAME | ❌ Missing | MED |
| 165 | MC_PLAYER_NAME | ❌ Missing | MED |

### Vehicles & Parts (15 messages)

| Code | Name | Status | Priority |
|------|------|--------|----------|
| 123 | MC_VEHICLE_INFO | ❌ Missing | HIGH |
| 141 | MC_STOCK_CAR_INFO | ✅ mcos | — |
| 142 | MC_PURCHASE_STOCK_CAR | ❌ Missing | HIGH |
| 145 | MC_GET_COMPLETE_VEHICLE_INFO | ❌ Missing | HIGH |
| 161 | MC_GET_PLAYERS_VEHICLES | ❌ Missing | MED |
| 163 | MC_UPDATE_CACHED_VEHICLE | ❌ Missing | MED |
| 172 | MC_GET_OWNED_VEHICLES | ❌ Missing | HIGH |
| 173 | MC_OWNED_VEHICLES_LIST | ❌ Missing | HIGH |
| 174 | MC_GET_OWNED_PARTS | ❌ Missing | MED |
| 175 | MC_OWNED_PARTS_LIST | ❌ Missing | MED |
| 176 | MC_BUY_NEW_PART | ❌ Missing | HIGH |
| 181 | MC_INSTALL_PART | ❌ Missing | HIGH |
| 182 | MC_REMOVE_PART | ❌ Missing | MED |

### Races (8 messages)

| Code | Name | Status | Priority |
|------|------|--------|----------|
| 218 | MC_RACE_JOIN | ❌ Missing | CRITICAL |
| 223 | MC_RACE_CREATE_OK | ❌ Missing | CRITICAL |
| 224 | MC_RACE_JOIN_OK | ❌ Missing | CRITICAL |
| 230 | MC_CREATE_STANDARD_RACE | ❌ Missing | CRITICAL |
| 232 | MC_RACE_START | ❌ Missing | CRITICAL |
| 233 | MC_RACE_STARTED | ❌ Missing | CRITICAL |
| 234 | MC_RACER_COMPLETED_RACE | ❌ Missing | CRITICAL |
| 237 | MC_RACE_FINAL_RESULTS | ❌ Missing | CRITICAL |

### Trade Window (6 messages)

| Code | Name | Status | Priority |
|------|------|--------|----------|
| 338 | MC_TW_HOST_SESSION | ❌ Missing | LOW |
| 339 | MC_TW_SESSION_HOSTED | ❌ Missing | LOW |
| 340 | MC_TW_JOIN_REQUEST | ❌ Missing | LOW |
| 341 | MC_TW_JOIN_SESSION | ❌ Missing | LOW |
| 350 | MC_TW_REQUEST_OFFER_CHANGE | ❌ Missing | LOW |
| 351 | MC_TW_OFFER_CHANGE | ❌ Missing | LOW |

---

## Implementation Priorities

### Phase 1: Fix Critical Bugs
1. **Fix LoginCompleteMessage** — shardGNP is double64, add serverList
2. **Add MC_FAILED** (102) — generic failure response
3. **Verify msg 105 (Login)** payload parsing against spec

### Phase 2: Player & Vehicle Core
4. **Add MC_GET_PLAYER_INFO (108)** + **MC_PLAYER_INFO (122)**
5. **Add MC_GET_OWNED_VEHICLES (172)** + **MC_OWNED_VEHICLES_LIST (173)**
6. **Add MC_GET_COMPLETE_VEHICLE_INFO (145)** + **MC_VEHICLE_INFO (123)**
7. **Add MC_PURCHASE_STOCK_CAR (142)**
8. **Add MC_BUY_NEW_PART (176)**, **MC_INSTALL_PART (181)**

### Phase 3: Race System
9. **Add race message handlers (218, 223, 224, 230, 232, 233, 234, 237)**
10. **Build race state machine** (waiting → countdown → racing → finished)
11. **Add race results tracking**

### Phase 4: Game Server (Shard)
12. **Build TCP game server** (port 9000+)
13. **Implement car state broadcast** (position, rotation, velocity)
14. **Add UDP position sync** for low-latency updates
15. **Physics validation** — server-side car physics to prevent cheating

### Phase 5: Client Patch
16. **Client redirect** (registry + hosts file)
17. **SSL cert injection** for old cipher support
18. **graphicsModeIndex workaround**

---

## Key Reference Data

### From Molly's spec (section 5.9–5.11):

**VehicleInfo structure (msg 123):**
```
VehicleInfoMsg =
    msgNo: WORD ,           (* 123 *)
    vehicleId: DWORD ,
    ownerId: DWORD ,
    flags: BYTE ,
    carTypeId: WORD ,
    paint: VehiclePaint ,
    partsInstalled: 20 * PartInstance ,
    padding: 4 * BYTE ;

VehiclePaint =
    paintId: WORD ,
    pearl: BOOL ,
    gloss: BOOL ,
    numColors: WORD ,
    colors: 4 * RGBA ;

PartInstance =
    slotId: WORD ,
    partId: DWORD ,
    condition: BYTE ;
```

**Vehicle purchase (msg 142):**
```
PurchaseStockCarMsg =
    msgNo: WORD ,           (* 142 *)
    carTypeId: WORD ,
    price: DWORD ,
    bodyColorIndex: BYTE ;
```

### Race Create (msg 230):
Full structure in Molly's spec section 9.4 — includes raceType, trackId, raceClass, entryFee, laps, passwordHash, maxPlayers, etc.

---

## Game Server Architecture

From `src/game/physics.md` and binary strings, we know the client physics runs in `PSim*` modules. The server needs to:

1. **Authoritative physics** — server runs physics, clients send inputs
2. **State broadcast** — 10+ times/sec, broadcast car positions
3. **Collision detection** — server detects car-car and car-wall collisions
4. **Race state** — checkpoint ordering, lap counting, finish detection

**Physics files (from binary strings):**
- `PSimCalc.c` — core physics loop
- `psimcar.c` — car model
- `PSimEngine.c` — engine simulation
- `psimbrke.c` — brake simulation
- `PSimSuspension.c` — suspension
- `pSimAero.c` — aerodynamics
- `PSimWAG.c` — weight/grip
- `Collide.c` — collision detection

**AI system (aivehicl.c):**
- Lane following, path planning, opponent awareness
- Skill levels with reaction time simulation

---

## Repository Strategy

**Fork mcos at:** `https://github.com/drazisil-codecov/mcos`

**New remote:** `https://github.com/Dadud/Motor-City-Online-RE` (our existing repo)

Work flow:
1. Fork mcos to our GitHub
2. Add as remote to existing repo
3. Implement missing handlers in fork
4. PR back to mcos when stable
5. Keep our repo as the "verified" version with our docs and tests

---

## External Resources

- mcos repo: https://github.com/drazisil-codecov/mcos
- Molly's spec: `mco-wiki/docs/research/NETWORK_PROTOCOL.md`
- Our protocol src: `mco-wiki/src/npslib/`
- Our physics src: `mco-wiki/src/game/`
- NFS modding community: NFSAddons, phosphorus.games
