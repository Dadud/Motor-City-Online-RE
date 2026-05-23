# Original MCO Client Compatibility

_Research: 2026-05-11_
_Status: Evidence-based; marked sections are GUESSED_

---

## Overview

Enabling the original MCO client to connect to a private shard server requires:
1. **Client-side redirects** — registry patches, SSL cert injection, hosts file
2. **Server-side emulators** — auth, login, lobby, MCOTS, and game servers
3. **Protocol gateway** — adapter between the original client protocol and a private server implementation (see external projects such as mcos)

This document covers what's known, what's guessed, and what still needs research.

---

## Client Configuration Architecture

### Registry Keys (from `authlogin.dll` decompilation)

The game reads auth server configuration from:
```
HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Motor City\AuthAuth\
  Auth_NPS_AAI_Hostname   (REG_SZ) — auth server hostname
  Auth_NPS_AAI_Port        (REG_SZ) — auth server port (default: 443)
  AuthLoginDllpath         (REG_SZ) — path to authlogin.dll
  AuthLoginBaseService     (REG_SZ) — GUESSED: base service URL
```

**Redirect method:** Change `Auth_NPS_AAI_Hostname` to your private server IP/hostname.

### Web Server Endpoints (from kylemann/mco-server)

The private server runs a web server on port 80 that serves client configuration files:

| Endpoint | File | Purpose |
|----------|------|---------|
| `https://<server>/registry` | `mco.reg` | Windows registry file to redirect all EA server addresses |
| `https://<server>/key` | `pub.key` | Public key for client↔server encryption |
| `https://<server>/cert` | `cert.pem` | SSL certificate (add to Trusted Root Store) |

### `.reg` File Format (GUESSED from kylemann client docs)

The `mco.reg` file is a standard Windows registry export containing at minimum:
```reg
Windows Registry Editor Version 5.00

[HKEY_LOCAL_MACHINE\SOFTWARE\Electronic Arts\Motor City\AuthAuth]
"Auth_NPS_AAI_Hostname"="<private-server-ip>"
"Auth_NPS_AAI_Port"="<port>"
```

When the user double-clicks `mco.reg`, it imports these values into the registry, redirecting the client's auth connection to the private server.

### SSL Certificate Installation (from kylemann docs)

1. Download `cert.pem` from `https://<server>/cert`
2. Open `MMC > Certificates > Trusted Root Certification Authorities`
3. Import the cert

This is required because the client uses HTTPS/WinINET for the initial auth connection.

---

## Server Port Map

| Port | Protocol | Service | Status |
|------|----------|---------|--------|
| **80** | HTTP | Web server (registry/key/cert download) | 🔴 Needs building |
| **443** | HTTPS | Auth server (WinINET connection from authlogin.dll) | 🔴 Needs building |
| **8226** | TCP | Login Server (NPS authentication) | 🔴 Needs building |
| **8227** | TCP | Login Server alternate | 🔴 Needs building |
| **8228** | TCP | Login Server alternate | 🔴 Needs building |
| **7003** | TCP | Lobby Server / PLS (NPS room/chat/race) | 🔴 Needs building |
| **43200** | TCP | MCOTS (transaction server) | 🔴 Needs building |
| **43300** | TCP | MCOTS alternate | 🔴 Needs building |
| **43400** | TCP | MCOTS alternate | 🔴 Needs building |
| **53303** | ? | Unknown | 🔴 Unknown |
| **9000–9014** | TCP | Game servers (in-race messages) | 🔴 Needs building |
| **9500–9508** | UDP | Game servers (fast position/state sync) | 🔴 Needs building |

---

## Connection Flow

### Original EA Flow
```
1. Client → Auth Server (HTTPS, WinINET, port 443)
   GET /SubscribeEntry.jsp?prodID=REG-MCO
   or similar EA account login URL
   
2. Auth Server → Client: session token / login ticket

3. Client → Login Server (NPS port 8226)
   NPSLogin with session token
   Authenticate persona

4. Client → Lobby Server (NPS port 7003)
   Join/create room, chat, see player list

5. Client → MCOTS (port 43300)
   Vehicle data, purchases, inventory

6. When race starts:
   Client → Game Server (port 9000+)
   NPS messages + UDP position sync (port 9500+)
```

### Private Server Flow (Target)
```
1. Client → Private Auth Web Server (HTTP port 80)
   GET /registry → downloads mco.reg
   User double-clicks to import registry

2. Client → Private Auth Server (HTTPS, WinINET)
   authlogin.dll reads registry → connects to private auth
   Returns session token

3. Client → Private Login Server (NPS port 8226)
   Login with any credentials → returns success

4. Client → Private Lobby Server (NPS port 7003)
   Room create/join, chat, race lobby

5. Client → Private MCOTS (port 43300)
   Vehicle/inventory transactions

6. Race → Private Game Server (port 9000+)
```

---

## Protocol Details

### NPS (Network Platform Services)
- **Header**: 12 bytes (version 257) or 4 bytes (version 0 legacy)
  - message-id (2 bytes, big-endian)
  - message-length (2 bytes, big-endian)
  - message-version (2 bytes, big-endian)
  - unknown (2 bytes, always 0)
  - checksum (4 bytes, big-endian)
- **Byte order**: Network (big-endian)
- **NPS message IDs**: 0x100–0x600 range
- **Reference**: `docs/research/NETWORK_PROTOCOL.md` sections 4.x

### MCOTS (Motor City Online Transaction Server)
- **Framing header**: 11 bytes
  - body-length (2 bytes, little-endian)
  - signature "TOMC" (4 bytes)
  - sequence (4 bytes, signed, little-endian)
  - flags (1 byte)
- **Body**: little-endian, tightly packed
- **Encryption**: ARC4/RC4 after initial handshake
- **Compression**: PKWARE DCL
- **Reference**: `docs/research/NETWORK_PROTOCOL.md` sections 5.x

### CASTANET (EA RPC Protocol)
- Wraps NPS calls in HTTP
- Used for auth and some server↔client communication
- Reference: `docs/research/network.md`

---

## Game Server Requirements

### Ports 9000–9014 (TCP)
- Reliable in-race messages
- Same NPS protocol as lobby with game-specific extensions
- Handles: car positions, collisions, checkpoint triggers, race state

### Ports 9500–9508 (UDP)
- Fast position/state synchronization
- Lower latency than TCP
- Broadcasts: car world position, rotation, velocity, damage

### Port 5050
- Explicitly reserved/ignored by the game
- Reference: `docs/research/NETWORK_PROTOCOL.md` section 4.6.1

---

## Client-Side Requirements

### Windows Version
- Windows XP has insufficient SSL cipher support
- Windows 7+ required for TLS/SSL connections to private server
- (from kylemann/mco-server client docs)

### Graphics Settings
In `<game dir>\SaveData\options.ini`:
```ini
graphicsModeIndex=4  ; 1280x960 recommended
```

Index mapping:
- `0` = 640x480
- `1` = 800x600
- `2` = 1024x768
- `3` = 1152x864
- `4` = 1280x960

### Movies
Delete `<game dir>\Data\Movies\` or launch with `-nomovie` flag.

### Compatibility Settings (Windows 10+)
- Disable fullscreen optimizations
- Run in windowed mode if fullscreen crashes

---

## Implementation Phases

### Phase 1: Auth Gateway (Priority: HIGH)
Build a minimal HTTPS server on port 443 that:
1. Serves the `.reg` file on `GET /registry`
2. Accepts any login credentials and returns a valid-looking session token
3. Serves `pub.key` and `cert.pem` files

This alone may allow the client to pass the auth step.

### Phase 2: Login Server Emulator (Priority: HIGH)
NPS server on port 8226:
1. Accepts any authenticated session
2. Returns persona list (can be empty or one default)
3. Returns persona selection success
4. Reference: `docs/research/NETWORK_PROTOCOL.md` sections 4.2–4.3

### Phase 3: Lobby Server / PLS (Priority: HIGH)
NPS server on port 7003:
1. Room create/join/leave
2. Chat messages
3. Ready/unready system
4. Race start triggers game server launch
5. Reference: `docs/research/NETWORK_PROTOCOL.md` sections 4.4–4.6

### Phase 4: MCOTS Server (Priority: MED)
Transaction server on port 43300:
1. Initial handshake (msg 438 ClientConnect)
2. Login (msg 105/213)
3. Vehicle info (msg 123, 145)
4. Parts/inventory (msg 172–182)
5. Race creation/join (msg 218, 230, etc.)
6. Reference: `docs/research/NETWORK_PROTOCOL.md` section 5

### Phase 5: Game Server (Priority: MED)
TCP server on port 9000+:
1. Accepts race participants
2. Broadcasts position updates
3. Detects checkpoint/order violations
4. Reports race completion

### Phase 6: UDP Sync (Priority: LOW)
UDP server on port 9500+:
1. Receives car state updates from each client
2. Broadcasts merged state to all participants
3. Lower latency than TCP

---

## Blockers

1. **CASTANET serialization** — How CASTANET wraps NPS RPC in HTTP is not fully documented. If auth uses CASTANET, this needs more research. (from `docs/research/network.md`)
2. **SSL cipher requirements** — Specific TLS version/cipher suites needed by the client are unknown
3. **Game server NPS extensions** — What additional NPS message types the game server handles beyond the lobby protocol
4. **UDP protocol** — Port 9500+ UDP format completely unknown
5. **MCOTS key derivation** — Diffie-Hellman key exchange for MCOTS ARC4 encryption is documented in Molly's spec but not verified

---

## References

- `docs/research/NETWORK_PROTOCOL.md` — Full NPS/MCOTS protocol donated by Molly
- `docs/research/network.md` — CASTANET and auth flow from EXE strings
- `docs/research/patch-system.md` — Patch server URL and flow
- `docs/research/SERVER_EMULATOR_PLAN.md` — Earlier server plan
- `src/authlogin/authlogin.cpp` — Auth DLL decompilation with registry keys
- kylemann/mco-server `docs/client.md` — Registry redirect and SSL cert setup
- kylemann/mco-server `docs/server.md` — Server port requirements

---

## Evidence Summary

| Item | Source | Evidence |
|------|--------|----------|
| Registry path `AuthAuth` | `authlogin.cpp` decompilation | Confirmed |
| Registry keys `Auth_NPS_AAI_Hostname`, `Auth_NPS_AAI_Port` | `authlogin.cpp` decompilation | Confirmed |
| Port 8226 Login, 7003 Lobby, 9000+ Game | `NETWORK_PROTOCOL.md` (Molly's spec) | Confirmed |
| MCOTS port 43300 | `NETWORK_PROTOCOL.md` | Confirmed |
| `.reg` file redirect | kylemann/mco-server client docs | Confirmed |
| SSL cert `/cert`, pub key `/key` | kylemann/mco-server client docs | Confirmed |
| HTTPS auth via WinINET | `authlogin.cpp` imports | Confirmed |
| UDP ports 9500–9508 | `NETWORK_PROTOCOL.md` | Confirmed |
| Port 5050 reserved | `NETWORK_PROTOCOL.md` | Confirmed |
| CASTANET HTTP wrapping | `network.md` | Guessed |
| Specific cipher suites | Unknown | Unknown |
