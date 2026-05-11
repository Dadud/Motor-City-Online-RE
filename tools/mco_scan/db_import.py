"""
Import scanned assets into the local shard database.
Adds discovered cars and tracks as user-supplied assets.
Clean-room implementation.
"""

from __future__ import annotations

import json
import sqlite3
from pathlib import Path
from typing import Any

from tools.mco_scan.scanner import ScanManifest


def import_manifest_to_shard_db(
    manifest_path: str | Path,
    db_path: str | Path,
    provenance: str = "user_supplied",
) -> dict[str, int]:
    """
    Read a scanned asset manifest and import discovered cars/tracks
    into the local shard SQLite database.

    Adds discovered cars to the `cars` table and tracks to an `asset_tracks`
    table (created if it doesn't exist).

    Returns a summary dict: {"cars_added": N, "tracks_added": N, "skipped": N}
    """
    manifest_path = Path(manifest_path)
    db_path = Path(db_path)

    with open(manifest_path, "r", encoding="utf-8") as f:
        data = json.load(f)
    manifest = ScanManifest(**data)

    if not db_path.exists():
        raise FileNotFoundError(f"Database not found: {db_path}")

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    stats = {"cars_added": 0, "tracks_added": 0, "skipped": 0}

    # Ensure asset_tracks table exists
    conn.execute("""
        CREATE TABLE IF NOT EXISTS asset_tracks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            track_key TEXT UNIQUE NOT NULL,
            display_name TEXT,
            path TEXT,
            file_type TEXT,
            size_bytes INTEGER,
            provenance TEXT DEFAULT 'user_supplied',
            imported_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
    """)

    # Import cars
    car_assets = [a for a in manifest.assets if a["type"] == "car" and a.get("model_key")]
    for asset in car_assets:
        model_key = asset["model_key"]
        display_name = asset.get("display_name") or model_key
        path = asset.get("path", "")
        size_bytes = asset.get("size_bytes", 0)

        # Check if already exists
        existing = conn.execute(
            "SELECT id FROM cars WHERE model_key=?",
            (model_key,),
        ).fetchone()

        if existing:
            stats["skipped"] += 1
            continue

        conn.execute(
            """
            INSERT INTO cars(model_key, display_name, base_price, class_name, provenance, source_path)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (
                model_key,
                display_name,
                5000,   # GUESSED: default starter price for discovered cars
                "discovered",  # GUESSED: class
                provenance,
                path,
            ),
        )
        stats["cars_added"] += 1

    # Import tracks
    track_assets = [a for a in manifest.assets if a["type"] == "track" and a.get("track_key")]
    for asset in track_assets:
        track_key = asset["track_key"]
        display_name = asset.get("display_name") or track_key
        path = asset.get("path", "")
        size_bytes = asset.get("size_bytes", 0)

        existing = conn.execute(
            "SELECT id FROM asset_tracks WHERE track_key=?",
            (track_key,),
        ).fetchone()

        if existing:
            stats["skipped"] += 1
            continue

        conn.execute(
            """
            INSERT INTO asset_tracks(track_key, display_name, path, file_type, size_bytes, provenance)
            VALUES (?, ?, ?, ?, ?, ?)
            """,
            (
                track_key,
                display_name,
                path,
                asset.get("file_type", "unknown"),
                size_bytes,
                provenance,
            ),
        )
        stats["tracks_added"] += 1

    conn.commit()
    conn.close()

    return stats
