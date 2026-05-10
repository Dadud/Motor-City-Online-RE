# Motor City Online — Private Server Emulator Plan

_Draft: 2026-05-10_
_Status: Planning_

## Goal

Create a patch + server emulator stack that allows the final retail game to boot and play without EA's servers.

---

## Executive Summary

MCO requires four distinct server components:

| Component | Port | Purpose | Status |
|-----------|------|---------|--------|
| Login Server | 8226 | Authentication, persona management | 🔴 Needs building |
| Lobby Server (PLS) | 7003 | Game room coordination, chat | 🔴 Needs building |
| MCOTS | 43300 | Game state transactions | 🔴 Needs building |
| Game Servers | 9000-9014 | In-race communication | 🔴 Needs building |

**We have:** Molly's comprehensive protocol spec (2000+ lines), partial NPS/Castanet source reconstructions.

**We need:** Full server implementations + client patch to redirect connections.

---

## What We Have

### Protocol Documentation
- `docs/research/NETWORK_PROTOCOL.md` — 2000+ line cleanroom specification donated by Molly
  - Full NPS message catalog (0x1xx-0x6xx)
  - MCOTS message catalog (100+ message types)
  - Connection flows, encryption (DES-CBC for NPS, ARC4 for MCOTS)
  - Packet framing, serialization, error codes

### Source Reconstructions (partial)
- `src/npslib/castanet.c/h` — CASTANET protocol (1400+ lines)
  - 12-byte header: magic(4) + version(2) + msgType(2) + length(4)
  - Send/Receive with proper blocking
  - Error codes
- `src/npslib/nps.c/h` — Working NPS API (1700+ lines)
  - NPSConnect, NPSLogin, NPSLogout
  - Persona management
  - Buddy list, Mail, Chat, Rooms

### File Format Decoders
- BNK audio: ✅ EA XA ADPCM works (bnk2wav.py)
- BIG/VIV: ✅ Working
- FSH: ✅ All variants decoded
- FCE: ✅ 12B/vertex, 56B/triangle

---

## Architecture

### Option A: Full Private Server (Recommended)

```
┌─────────────────────────────────────────────────────────────┐
│                        GAME CLIENT                          │
│                  (patched to point to localhost)            │
└────────────────┬────────────────────────────────────────────┘
                 │ TCP
                 ▼
┌─────────────────────────────────────────────────────────────┐
│                     PROXY / PATCH LAYER                      │
│         (redirects connections to local servers)            │
│         OR: client-side hosts file manipulation              │
└────────────────┬────────────────────────────────────────────┘
                 │
     ┌───────────┼───────────┬────────────┐
     ▼           ▼           ▼            ▼
┌─────────┐ ┌─────────┐ ┌─────────┐ ┌──────────┐
│  Login  │ │  Lobby  │ │  MCOTS  │ │   Game   │
│ Server  │ │ Server  │ │ Server  │ │  Server  │
│  :8226  │ │  :7003  │ │ :43300  │ │ :9000+   │
└─────────┘ └─────────┘ └─────────┘ └──────────┘
```

### Option B: Minimal Auth Bypass (Faster, Limited)

Only emulate what's needed to get past the login screen:
- Fake Login Server → accepts any credentials
- Fake Lobby Server → returns minimal success responses
- Game itself runs against AI (offline mode)

---

## Implementation Phases

### Phase 1: Login Server Emulator (Priority: CRITICAL)

**Goal:** Unblock signing in — client can authenticate and select persona.

**What it does:**
- Listen on port 8226
- Accept NPS_USER_LOGIN (0x501) with username/password
- Return NPS_USER_VALID (0x601) with fake personas and shard list
- Handle token validation requests

**Key messages to implement:**
- 0x501 NPS_USER_LOGIN → 0x601 NPS_USER_VALID
- 0x502 NPS_GAME_LOGIN
- 0x503 NPS_REGISTER_GAME_LOGIN
- 0x506 NPS_NEW_EA_ACCOUNT
- 0x507 NPS_NEW_GAME_ACCOUNT
- 0x512 NPS_DELETE_GAME_PERSONA
- 0x519 NPS_GET_PERSONA_INFO
- 0x533 NPS_VALIDATE_PERSONA_NAME
- 0x534 NPS_CHECK_TOKEN

**Deliverable:** `mco-server login` — single binary that accepts any login.

### Phase 2: Lobby Server (PLS) Emulator (Priority: HIGH)

**Goal:** Client can create/join rooms and see other players.

**What it does:**
- Listen on port 7003
- Handle NPS_LOGIN (0x100) after sending NPS_OK_TO_LOGIN (0x230) handshake
- Room management: create, join, leave
- Chat, user list, ready state
- Game start sequencing

**Key messages to implement:**
- 0x230 NPS_OK_TO_LOGIN (server→client, unsolicited handshake)
- 0x100 NPS_LOGIN → 0x120 NPS_LOGIN_RESP
- 0x103 NPS_SET_MY_USER_DATA
- 0x106 NPS_OPEN_COMM_CHANNEL → 0x214 NPS_CHANNEL_GRANTED
- 0x108 NPS_START_GAME
- 0x109 NPS_READY_FOR_GAME
- 0x211 NPS_USER_LIST
- 0x214 NPS_CHANNEL_GRANTED
- 0x224 NPS_GAME_SERVER_STATE_CHANGE

**Deliverable:** `mco-server lobby` — handles room coordination.

### Phase 3: MCOTS Transaction Server (Priority: HIGH)

**Goal:** Game state transactions work — lobbies, car purchases, race results.

**What it does:**
- Listen on port 43300
- Handle encrypted MCOTS messages (ARC4 after DH key exchange)
- Vehicle transactions, race management, club operations

**Key messages to implement (first pass):**
- 105 LoginMsg → 213 LoginCompleteMsg
- 136 MC_GET_LOBBIES → 325 MC_LOBBIES + 408 MC_ENTRYFEE_PURSE_INFO
- 438 ClientConnectMsg
- Various MCOTS transaction messages

**Deliverable:** `mco-server mcots` — handles game transactions.

### Phase 4: Game Server (Priority: MEDIUM)

**Goal:** Actual racing with other players.

**What it does:**
- Listen on ports 9000-9014 (TCP) and 9500-9508 (UDP)
- Race state synchronization
- Car physics broadcast
- Position updates

**Deliverable:** `mco-server game` — handles in-race communication.

### Phase 5: Client Patch / Redirect (Priority: CRITICAL)

**Goal:** Game client connects to our servers instead of EA's.

**Options:**
1. **Hosts file** — redirect `ea.com` or auth server hostname to 127.0.0.1
2. **Proxy DLL** — replace `authlogin.dll` with our own that connects locally
3. **Hex patch** — modify `mcity.exe` to change hardcoded server addresses

**Recommended:** Option 2 (proxy DLL) as it's most maintainable.

**Deliverable:** Patched `authlogin.dll` or `wsock32.dll` redirecting to localhost.

---

## Client Connection Flow

From NETWORK_PROTOCOL.md §6.1-6.2:

```
1. Client ---TCP Connect 8226---> Login Server
2. Client ---0x501 NPS_USER_LOGIN--> Login Server
3. Client <--0x601 NPS_USER_VALID--- Login Server
   (receives personas, shard list)

4. Client ---TCP Connect 43300---> MCOTS Server
5. Client ---438 ClientConnectMsg--> MCOTS Server
6. Client <--GenericReply (ACK)---- MCOTS Server
   (encryption active after this)

7. Client ---105 LoginMsg---------> MCOTS Server
8. Client <--213 LoginCompleteMsg-- MCOTS Server
   (server list, tunables)

9. Client ---TCP Connect 7003----> Lobby Server
10. Client <--0x230 NPS_OK_TO_LOGIN- Lobby Server
    (unsolicited handshake - client blocks waiting for this!)

11. Client ---0x100 NPS_LOGIN-----> Lobby Server
12. Client <--0x120 NPS_LOGIN_RESP-- Lobby Server
```

**CRITICAL:** The NPS_OK_TO_LOGIN (0x230) is server-pushed and the client WILL BLOCK waiting for it. This must be sent immediately upon TCP connect.

---

## Build Artifacts

```
mco-private-server/
├── login_server/      # Phase 1
│   ├── main.go        # or main.c depending on language
│   ├── nps_handler.go
│   └── auth.go
├── lobby_server/      # Phase 2
│   ├── main.go
│   ├── nps_lobby.go
│   └── room.go
├── mcots_server/      # Phase 3
│   ├── main.go
│   ├── crypto.go      # ARC4 encryption
│   └── transactions.go
├── game_server/       # Phase 4
│   ├── main.go
│   ├── tcp_handler.go
│   └── udp_handler.go
├── client_patch/      # Phase 5
│   ├── authlogin.dll/ # Proxy DLL
│   └── README.md
├── shared/
│   ├── protocol.go    # Shared message definitions
│   ├── castanet.go    # CASTANET framing
│   └── types.go
└── README.md
```

---

## Language Choice

**Recommended: Go or Rust**

- Go: Faster development, good networking, easy deployment
- Rust: More safety, better performance, harder to write

**Alternative: C/C++**

- Matches existing codebase style
- More complex memory management
- Historical precedent (AZMCO uses C++)

**Recommendation:** Start with Go for rapid prototyping, then potentially rewrite performance-critical parts in Rust or C.

---

## Key Risks

1. **CASTANET serialization** — Binary format for some messages not fully documented
   - Status: Partially known (Molly's spec covers most)
   - Mitigation: Wireshark-style experimentation

2. **Encryption key management** — RSA for NPS, ARC4 for MCOTS
   - Status: RSA public key needed for NPS, DH key exchange for MCOTS
   - Mitigation: Extract from game binary or use known EA keys

3. **Client validation** — Server may send values the client validates
   - Status: Unknown which fields are validated
   - Mitigation: Hex editing client to skip checksums/checks

4. **Real-time requirements** — Game server needs low latency
   - Status: Manageable with modern hardware
   - Mitigation: Use async I/O (epoll/kqueue/IOCP)

---

## Quick Start Path

To get something working fast:

1. Build minimal login server (accept anything, return fake persona)
2. Build lobby server stub (return success for all operations)
3. Patch hosts file to redirect auth server to 127.0.0.1
4. Try launching game
5. Iterate based on failures

---

## External Resources

**The mcos project (formerly rustymotors/server) is a COMPLETE working implementation:**
- TypeScript/Node.js monorepo
- Already achieved lobby connection (Oct 12, 2023)
- GitHub: https://github.com/drazisil-codecov/mcos (forked from rustymotors)
- Docker-based deployment
- Client patch documented with registry redirect + SSL cert

### mcos Architecture
```
packages/
├── cli           # CLI tools
├── connection    # Connection handling
├── database     # PostgreSQL (MikroORM)
├── gateway      # SSL gateway (handles old cipher issues)
├── mcots        # MCOTS transaction server
├── nps          # NPS protocol
├── patch        # Client patch tools
├── shard        # Game/shard server
├── shared-packets   # Packet definitions
└── shared       # Shared utilities

src/
├── nps_server.ts    # Entry point (login + lobby)
└── chat/            # Chat module
```

### Ports (from mcos docker-compose):
- 80, 443 — Web/SSL gateway
- 6660, 7003 — Lobby Server (PLS)
- 8226, 8227, 8228 — Login Server
- 43200, 43300, 43400 — MCOTS
- 53303 — Shard/Game Server

### Client Setup (from mcos docs):
1. Import `mco.reg` registry file (redirects to server)
2. Copy `pub.key` to game directory
3. Install SSL cert into Trusted Root store
4. Set `graphicsModeIndex` in `SaveData/options.ini`

---

## Revised Strategy

**Don't rebuild from scratch — contribute to or fork mcos!**

The mcos project has:
- ✅ Complete server implementation (login, lobby, MCOTS, shard)
- ✅ SSL gateway for cipher compatibility
- ✅ Client patch tools
- ✅ Database schema
- ✅ Working lobby connection

What remains to be done (from their issue tracker):
- Full race functionality
- Car purchasing transactions
- Club features
- etc.

---

## References

- `docs/research/NETWORK_PROTOCOL.md` — Primary protocol reference (Molly's donation)
- `src/npslib/castanet.c` — Existing CASTANET implementation reference
- `src/npslib/nps.c` — Existing NPS API reference
- Game binary strings (`mcity.exe`, `mco.exe`, `authlogin.dll`)
- **mcos project**: https://github.com/drazisil-codecov/mcos (main implementation)
- **AZMCO**: https://github.com/americusmaximus/AZMCO (full game reimplementation, not server)

