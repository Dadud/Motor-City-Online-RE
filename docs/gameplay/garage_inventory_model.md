# Garage and Inventory Model

## Core entities

### Garage
Represents a driver's owned storage space and car roster.

### Car
Owned vehicle entry tied to a profile.

### Car build
Current installed-part state for a specific car.

### Owned part
A persistent owned part instance.

### Inventory item
Generic bucket for loose parts and future consumables/items.

## First-pass rules

- each driver gets one default garage on creation
- cars belong to one profile and one garage
- installed parts are tracked through `car_builds`
- loose owned parts remain in inventory until installed
- removing a part places it back into loose inventory

## Minimum supported operations

- list garage cars
- view car build
- list owned loose parts
- buy part into inventory
- install part on selected car
- remove installed part back to inventory

## Simplifications in M1

- no garage capacity limit yet
- no part wear/damage modeling yet
- no attachment validation beyond part category/slot string
- no duplicate resolution complexity beyond owned IDs

## Speculation

Original MCO likely had richer slot validation, wear, and serialized vehicle state. This pass uses a simplified clean-room representation suitable for local preservation.
