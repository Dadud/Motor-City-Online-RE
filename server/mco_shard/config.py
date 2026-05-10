from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import os


@dataclass(slots=True)
class Settings:
    host: str = os.getenv("MCO_SERVER_HOST", "127.0.0.1")
    port: int = int(os.getenv("MCO_SERVER_PORT", "8765"))
    db_path: Path = Path(os.getenv("MCO_DB_PATH", "persistence/mco_local_shard.db"))
    seed_on_start: bool = os.getenv("MCO_SEED_ON_START", "1") == "1"


settings = Settings()
