# Economy Model

## Goal

Provide a simple, inspectable local economy that supports the preservation loop without pretending to be exact where evidence is missing.

## Currency

Single local cash balance per driver profile.

## Starting state

- new driver receives starting cash
- default in this pass: **20,000** credits

## Transactions supported

- buy car
- sell car
- buy part
- race reward payout
- optional admin/seed grants

## Rules for first implementation

### Buy car
- subtract dealership price
- create owned car record
- log transaction

### Sell car
- add resale value
- remove ownership or mark inactive
- log transaction

### Buy part
- subtract part price
- create owned part/inventory record
- log transaction

### Race reward
- winner/finisher gets synthetic reward in M1
- reward amount stored with result and transaction row

## Data integrity

Every economy-changing action writes a `transactions` row with:
- profile_id
- transaction_type
- amount_delta
- balance_after
- reference_type/reference_id
- note

## Speculation

Prices and payouts in this pass are placeholder preservation values, not claims about exact retail balance.
