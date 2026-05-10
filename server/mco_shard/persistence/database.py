from __future__ import annotations

import sqlite3
from pathlib import Path
from typing import Iterable


LATEST_SCHEMA_VERSION = 2


MIGRATIONS: dict[int, dict[str, list[str]]] = {
    1: {
        "upgrade": [
            """
            CREATE TABLE accounts (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT NOT NULL UNIQUE,
                password TEXT NOT NULL,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
            """,
            """
            CREATE TABLE profiles (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                account_id INTEGER NOT NULL,
                driver_name TEXT NOT NULL UNIQUE,
                cash_balance INTEGER NOT NULL DEFAULT 0,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(account_id) REFERENCES accounts(id)
            )
            """,
            """
            CREATE TABLE garages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                profile_id INTEGER NOT NULL UNIQUE,
                name TEXT NOT NULL,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(profile_id) REFERENCES profiles(id)
            )
            """,
            """
            CREATE TABLE cars (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                model_key TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                base_price INTEGER NOT NULL,
                class_name TEXT,
                dealership_only INTEGER NOT NULL DEFAULT 1
            )
            """,
            """
            CREATE TABLE owned_cars (
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
            CREATE TABLE car_builds (
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
            CREATE TABLE owned_parts (
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
            CREATE TABLE inventory_items (
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
            CREATE TABLE parts_catalog (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                part_key TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                slot_name TEXT NOT NULL,
                price INTEGER NOT NULL,
                performance_note TEXT
            )
            """,
            """
            CREATE TABLE dealerships (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL UNIQUE,
                location TEXT,
                kind TEXT NOT NULL
            )
            """,
            """
            CREATE TABLE dealership_inventory (
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
            CREATE TABLE transactions (
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
            CREATE TABLE events (
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
            CREATE TABLE race_lobbies (
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
            CREATE TABLE race_lobby_members (
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
            CREATE TABLE race_results (
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
            CREATE TABLE chat_messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                lobby_id INTEGER,
                profile_id INTEGER,
                message TEXT NOT NULL,
                created_at TEXT DEFAULT CURRENT_TIMESTAMP
            )
            """,
            """
            CREATE TABLE server_settings (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """,
        ],
        "downgrade": [
            "DROP TABLE IF EXISTS chat_messages",
            "DROP TABLE IF EXISTS race_results",
            "DROP TABLE IF EXISTS race_lobby_members",
            "DROP TABLE IF EXISTS race_lobbies",
            "DROP TABLE IF EXISTS events",
            "DROP TABLE IF EXISTS transactions",
            "DROP TABLE IF EXISTS dealership_inventory",
            "DROP TABLE IF EXISTS dealerships",
            "DROP TABLE IF EXISTS parts_catalog",
            "DROP TABLE IF EXISTS inventory_items",
            "DROP TABLE IF EXISTS owned_parts",
            "DROP TABLE IF EXISTS car_builds",
            "DROP TABLE IF EXISTS owned_cars",
            "DROP TABLE IF EXISTS cars",
            "DROP TABLE IF EXISTS garages",
            "DROP TABLE IF EXISTS profiles",
            "DROP TABLE IF EXISTS accounts",
            "DROP TABLE IF EXISTS server_settings",
        ],
    },
    2: {
        "upgrade": [
            """
            CREATE TABLE race_sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                lobby_id INTEGER NOT NULL UNIQUE,
                status TEXT NOT NULL DEFAULT 'created',
                scene_type TEXT NOT NULL DEFAULT 'text_loop',
                config_json TEXT NOT NULL DEFAULT '{}',
                launched_at TEXT DEFAULT CURRENT_TIMESTAMP,
                completed_at TEXT,
                FOREIGN KEY(lobby_id) REFERENCES race_lobbies(id)
            )
            """,
        ],
        "downgrade": [
            "DROP TABLE IF EXISTS race_sessions",
        ],
    },
}


class Database:
    def __init__(self, path: Path):
        self.path = path
        self.path.parent.mkdir(parents=True, exist_ok=True)

    def connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.path)
        conn.row_factory = sqlite3.Row
        return conn

    def init_schema(self) -> None:
        self.upgrade()

    def upgrade(self, target_version: int | None = None) -> int:
        target = target_version or LATEST_SCHEMA_VERSION
        if target < 0 or target > LATEST_SCHEMA_VERSION:
            raise ValueError(f"unsupported target version: {target}")
        with self.connect() as conn:
            self._ensure_settings_table(conn)
            current = self._get_schema_version(conn)
            for version in range(current + 1, target + 1):
                for statement in MIGRATIONS[version]["upgrade"]:
                    conn.execute(statement)
                self._set_schema_version(conn, version)
            conn.commit()
            return self._get_schema_version(conn)

    def downgrade(self, target_version: int = 0) -> int:
        if target_version < 0 or target_version > LATEST_SCHEMA_VERSION:
            raise ValueError(f"unsupported target version: {target_version}")
        with self.connect() as conn:
            self._ensure_settings_table(conn)
            current = self._get_schema_version(conn)
            for version in range(current, target_version, -1):
                for statement in MIGRATIONS[version]["downgrade"]:
                    conn.execute(statement)
                self._set_schema_version(conn, version - 1)
            conn.commit()
            return self._get_schema_version(conn)

    def execute_script(self, statements: Iterable[str]) -> None:
        with self.connect() as conn:
            for statement in statements:
                conn.execute(statement)
            conn.commit()

    def schema_version(self) -> int:
        with self.connect() as conn:
            self._ensure_settings_table(conn)
            return self._get_schema_version(conn)

    @staticmethod
    def _ensure_settings_table(conn: sqlite3.Connection) -> None:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS server_settings (
                key TEXT PRIMARY KEY,
                value TEXT NOT NULL
            )
            """
        )

    @staticmethod
    def _get_schema_version(conn: sqlite3.Connection) -> int:
        row = conn.execute(
            "SELECT value FROM server_settings WHERE key='schema_version'"
        ).fetchone()
        return int(row["value"]) if row else 0

    @staticmethod
    def _set_schema_version(conn: sqlite3.Connection, version: int) -> None:
        conn.execute(
            """
            INSERT INTO server_settings(key, value)
            VALUES ('schema_version', ?)
            ON CONFLICT(key) DO UPDATE SET value=excluded.value
            """,
            (str(version),),
        )
