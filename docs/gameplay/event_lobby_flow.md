# Event and Lobby Flow

## Event list

Events are seeded server-side and exposed as joinable local content descriptors.

Each event includes:
- name
- event type
- track key
- entry fee
- reward
- lap count
- player limit

## Lobby flow

### M1/M4 target flow
1. player lists events
2. player creates a race lobby for an event
3. another client or same client can join
4. participants toggle ready/unready
5. host starts lobby when requirements are met
6. placeholder race session record is created
7. result submission pays out reward and closes session

## Current implementation boundaries

M1 includes data model and API support for:
- create lobby
- join lobby
- ready/unready
- start lobby

Race execution itself is still placeholder in this pass.

## State model

Lobby states:
- open
- ready
- in_race
- completed
- cancelled

Participant states:
- joined
- ready
- not_ready
- finished

## Speculation

The exact original room/game transition used NPS + MCOTS coordination. This clean-room shard models the user-facing intent first, then can map it later to original protocol semantics.
