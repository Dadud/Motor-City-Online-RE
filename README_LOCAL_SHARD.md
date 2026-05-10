# MCO Local Shard (M1)

## Quick start

```bash
./scripts/dev_local_shard.sh
```

This will:
1. install the local Python package in editable mode
2. start the local shard server on `127.0.0.1:8765`
3. run the CLI preservation client demo flow

## Commands

### Start server
```bash
python3 -m server.mco_shard.app
```

### Run demo client
```bash
python3 -m client.preservation_client.cli demo
```

### Scan user-supplied assets
```bash
python3 -m tools.mco_scan.cli /path/to/MotorCityOnline
```

## Notes
- SQLite DB: `persistence/mco_local_shard.db`
- This implementation is clean-room and does not bundle EA assets.
- Original-client compatibility is documented separately and not yet implemented.
