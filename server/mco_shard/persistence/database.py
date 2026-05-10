from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Iterable


SCHEMA_STATEMENTS: list[str] = [
    """
    CREATE TABLE IF NOT EXISTS accounts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        username TEXT NOT NULL UNIQUE,
        password TEXT NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS profiles (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        account_id INTEGER NOT NULL,
        driver_name TEXT NOT NULL UNIQUE,
        cash_balance INTEGER NOT NULL DEFAULT 0,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(account_id) REFERENCES accounts(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS garages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile_id INTEGER NOT NULL UNIQUE,
        name TEXT NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS cars (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        model_key TEXT NOT NULL UNIQUE,
        display_name TEXT NOT NULL,
        base_price INTEGER NOT NULL,
        class_name TEXT,
        dealership_only INTEGER NOT NULL DEFAULT 1
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS owned_cars (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile_id INTEGER NOT NULL,
        garage_id INTEGER NOT NULL,
        car_id INTEGER NOT NULL,
        nickname TEXT,
        acquired_at TEXT DEFAULT CURRENT_TIMESTAMP,
        active INTEGER NOT NULL DEFAULT 1,
        FOREIGN KEY(profile_id) REFERENCES profiles(id),
        FOREIGN KEY(garage_id) REFERENCES garages(id),
        FOREIGN KEY(car_id) REFERENCES cars(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS car_builds (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        owned_car_id INTEGER NOT NULL,
        slot_name TEXT NOT NULL,
        owned_part_id INTEGER,
        installed_at TEXT DEFAULT CURRENT_TIMESTAMP,
        UNIQUE(owned_car_id, slot_name),
        FOREIGN KEY(owned_car_id) REFERENCES owned_cars(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS owned_parts (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile_id INTEGER NOT NULL,
        part_id INTEGER NOT NULL,
        state TEXT NOT NULL DEFAULT 'inventory',
        owned_car_id INTEGER,
        slot_name TEXT,
        acquired_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(profile_id) REFERENCES profiles(id),
        FOREIGN KEY(owned_car_id) REFERENCES owned_cars(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS inventory_items (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile_id INTEGER NOT NULL,
        item_type TEXT NOT NULL,
        ref_id INTEGER NOT NULL,
        quantity INTEGER NOT NULL DEFAULT 1,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS parts_catalog (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        part_key TEXT NOT NULL UNIQUE,
        display_name TEXT NOT NULL,
        slot_name TEXT NOT NULL,
        price INTEGER NOT NULL,
        performance_note TEXT
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS dealerships (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL UNIQUE,
        location TEXT,
        kind TEXT NOT NULL
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS dealership_inventory (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        dealership_id INTEGER NOT NULL,
        item_type TEXT NOT NULL,
        ref_id INTEGER NOT NULL,
        price INTEGER NOT NULL,
        stock INTEGER NOT NULL DEFAULT 1,
        FOREIGN KEY(dealership_id) REFERENCES dealerships(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS transactions (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        profile_id INTEGER NOT NULL,
        transaction_type TEXT NOT NULL,
        amount_delta INTEGER NOT NULL,
        balance_after INTEGER NOT NULL,
        reference_type TEXT,
        reference_id INTEGER,
        note TEXT,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS events (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        event_type TEXT NOT NULL,
        track_key TEXT NOT NULL,
        entry_fee INTEGER NOT NULL,
        reward INTEGER NOT NULL,
        laps INTEGER NOT NULL,
        max_players INTEGER NOT NULL DEFAULT 6
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS race_lobbies (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        event_id INTEGER NOT NULL,
        host_profile_id INTEGER NOT NULL,
        lobby_name TEXT NOT NULL,
        state TEXT NOT NULL DEFAULT 'open',
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        started_at TEXT,
        FOREIGN KEY(event_id) REFERENCES events(id),
        FOREIGN KEY(host_profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS race_lobby_members (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lobby_id INTEGER NOT NULL,
        profile_id INTEGER NOT NULL,
        ready INTEGER NOT NULL DEFAULT 0,
        joined_at TEXT DEFAULT CURRENT_TIMESTAMP,
        UNIQUE(lobby_id, profile_id),
        FOREIGN KEY(lobby_id) REFERENCES race_lobbies(id),
        FOREIGN KEY(profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS race_results (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lobby_id INTEGER NOT NULL,
        profile_id INTEGER NOT NULL,
        position INTEGER NOT NULL,
        finish_time_ms INTEGER NOT NULL,
        reward_paid INTEGER NOT NULL DEFAULT 0,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(lobby_id) REFERENCES race_lobbies(id),
        FOREIGN KEY(profile_id) REFERENCES profiles(id)
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS chat_messages (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        lobby_id INTEGER,
        profile_id INTEGER,
        message TEXT NOT NULL,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP
    )
    """,
    """
    CREATE TABLE IF NOT EXISTS server_settings (
        key TEXT PRIMARY KEY,
        value TEXT NOT NULL
    )
    """,
]


class Database:
    def __init__(self, path: Path):
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)

    def connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.path)
        conn.row_factory = sqlite3.Row
        return conn

    def init_schema(self) -> None:
        with self.connect() as conn:
            for statement in SCHEMA_STATEMENTS:
                conn.execute(statement)
            conn.commit()

    def execute_script(self, statements: Iterable[str]) -> None:
        with self.connect() as conn:
            for statement in statements:
                conn.execute(statement)
            conn.commit()
