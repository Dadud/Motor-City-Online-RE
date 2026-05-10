# Original Client Compatibility

## Status

Research track only for now. The local shard implementation in this pass does **not** try to speak original client protocols yet.

## What is already known

From `docs/research/NETWORK_PROTOCOL.md` and other RE notes:

- Login server: port 8226
- Lobby/PLS: port 7003
- MCOTS: port 43300
- Game servers: 9000-9014 TCP, 9500-9508 UDP
- client expects unsolicited `NPS_OK_TO_LOGIN` (0x230) on lobby connect
- NPS uses big-endian framing; MCOTS uses little-endian framing
- encryption/compression details are partially documented

## Compatibility path

### Phase A: modern shard first
Build and verify gameplay/state logic behind a clean local API.

### Phase B: protocol adapter
Add a shim/proxy process that maps original client protocol messages onto the local shard domain model.

### Phase C: patch/redirect tooling
Investigate:
- registry keys
- INI or config endpoints
- DNS/hosts redirection
- RSA public key/cert requirements
- login / shard list expectations

## Biggest risks

1. encryption/session setup correctness
2. exact message layouts for edge cases
3. client assumptions around timing and bundled responses
4. game-server behavior once lobbies transition into live race sessions

## Immediate implementation stance

Do not block local preservation flow on original-client support.

## Evidence-backed notes

- `NPS_OK_TO_LOGIN` handshake is critical.
- LoginComplete/shard list layout matters for MCOTS compatibility.
- Race/lobby flow requires both transaction-layer and room/game-layer coordination.

## Speculation

The cleanest long-term design is likely:
- `core shard domain` (current Python implementation)
- `original protocol gateway` translating NPS/MCOTS <-> core domain events

This keeps preservation logic testable even if original-client emulation remains incomplete.
