# Local Shard Design

## Goal

Provide a locally runnable Motor City Online-style shard focused on preservation workflow first, original-client compatibility later.

## Implementation choice

This implementation uses a **modern local HTTP API + CLI client** first.

Why:
- faster to iterate than reproducing original NPS/MCOTS on day one
- easier to test and debug locally
- clean separation between gameplay model and future protocol adapters

## Service layout

```text
server/
  mco_shard/
    app.py
    api/
    services/
    persistence/
    models/
```

Logical services in the first implementation:
- auth_service
- account_service
- garage_service
- inventory_service
- economy_service
- dealership_service
- event_service
- lobby_service
- race_session_service
- chat_service
- persistence

Initially these are implemented as Python service modules inside one process.

## Transport

### Current
- HTTP JSON API for command/query operations

### Planned
- WebSocket for lobby/chat/live race session state
- adapter layer for original NPS/MCOTS compatibility research

## Persistence

SQLite database file at `persistence/mco_local_shard.db`.

Goals:
- inspectable with sqlite3/DB Browser
- schema initialized automatically
- deterministic seed content for local play

## Seeded gameplay loop

First useful flow:
1. create account
2. login
3. create driver
4. receive starting cash
5. receive or buy starter car
6. inspect garage
7. buy/install part
8. view events
9. create lobby
10. join/ready/start placeholder race
11. submit race result
12. reward payout persists after restart

## Domain assumptions

### Known
- MCO had distinct flows for accounts/personas/cars/parts/lobbies/races
- protocol documents confirm race/lobby concepts and minimum fields

### Speculation
- local economy values, dealership stock, and starter rewards in this implementation are preservation-friendly approximations until backed by stronger evidence
- placeholder race sessions are synthetic until a driveable scene exists

All speculative behavior should stay documented as such.
