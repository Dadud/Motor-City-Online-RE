from __future__ import annotations

from pathlib import Path
import csv
import json
import sqlite3
import time
from typing import Any

from server.mco_shard.persistence.database import Database, LATEST_SCHEMA_VERSION


class ShardService:
    def __init__(self, db: Database, repo_root: Path):
        self.db = db
        self.repo_root = repo_root

    def init(self, seed: bool = True) -> None:
        self.db.upgrade(LATEST_SCHEMA_VERSION)
        if seed:
            self.seed_data()

    def seed_data(self) -> None:
        with self.db.connect() as conn:
            self._seed_settings(conn)
            self._seed_dealerships(conn)
            self._seed_cars(conn)
            self._seed_parts(conn)
            self._seed_events(conn)
            conn.commit()

    def _seed_settings(self, conn: sqlite3.Connection) -> None:
        conn.execute("INSERT OR IGNORE INTO server_settings(key, value) VALUES ('server_name', 'MCO Local Shard')")

    def _seed_dealerships(self, conn: sqlite3.Connection) -> None:
        dealerships = [
            ("Downtown Motors", "Motor City", "cars"),
            ("Grease Monkey Performance", "Motor City", "parts"),
        ]
        conn.executemany(
            "INSERT OR IGNORE INTO dealerships(name, location, kind) VALUES (?, ?, ?)",
            dealerships,
        )

    def _seed_cars(self, conn: sqlite3.Connection) -> None:
        cars_csv = self.repo_root / "data" / "Cars.csv"
        inserted_ids: list[int] = []
        if cars_csv.exists():
            with cars_csv.open(newline="", encoding="utf-8-sig") as handle:
                reader = csv.DictReader(handle)
                for row in reader:
                    model_key = row.get("CarName") or row.get("Car") or row.get("Name")
                    if not model_key:
                        continue
                    display_name = row.get("FullName") or row.get("DisplayName") or model_key
                    class_name = row.get("DriverClass") or row.get("Class") or "unknown"
                    base_price = self._coerce_price(row.get("Price"), fallback=5000 + len(inserted_ids) * 250)
                    cur = conn.execute(
                        "INSERT OR IGNORE INTO cars(model_key, display_name, base_price, class_name) VALUES (?, ?, ?, ?)",
                        (model_key, display_name, base_price, class_name),
                    )
                    if cur.rowcount:
                        inserted_ids.append(cur.lastrowid)
                    if len(inserted_ids) >= 8:
                        break
        if not inserted_ids:
            fallback = [
                ("53chevy", "1953 Chevrolet", 7000, "street"),
                ("59impala", "1959 Impala", 9000, "street"),
                ("96supra", "1996 Supra", 12000, "sport"),
            ]
            conn.executemany(
                "INSERT OR IGNORE INTO cars(model_key, display_name, base_price, class_name) VALUES (?, ?, ?, ?)",
                fallback,
            )
        car_rows = conn.execute("SELECT id, base_price FROM cars ORDER BY id LIMIT 6").fetchall()
        car_dealer_id = conn.execute("SELECT id FROM dealerships WHERE kind='cars' LIMIT 1").fetchone()[0]
        for row in car_rows:
            conn.execute(
                "INSERT OR IGNORE INTO dealership_inventory(dealership_id, item_type, ref_id, price, stock) VALUES (?, 'car', ?, ?, 99)",
                (car_dealer_id, row["id"], row["base_price"]),
            )

    def _seed_parts(self, conn: sqlite3.Connection) -> None:
        parts = [
            ("starter_carb", "Starter Carburetor", "engine", 1200, "M1 placeholder upgrade"),
            ("street_tires", "Street Tires", "tires", 900, "M1 placeholder grip upgrade"),
            ("sport_exhaust", "Sport Exhaust", "exhaust", 1500, "M1 placeholder power upgrade"),
        ]
        conn.executemany(
            "INSERT OR IGNORE INTO parts_catalog(part_key, display_name, slot_name, price, performance_note) VALUES (?, ?, ?, ?, ?)",
            parts,
        )
        part_rows = conn.execute("SELECT id, price FROM parts_catalog ORDER BY id").fetchall()
        part_dealer_id = conn.execute("SELECT id FROM dealerships WHERE kind='parts' LIMIT 1").fetchone()[0]
        for row in part_rows:
            conn.execute(
                "INSERT OR IGNORE INTO dealership_inventory(dealership_id, item_type, ref_id, price, stock) VALUES (?, 'part', ?, ?, 99)",
                (part_dealer_id, row["id"], row["price"]),
            )

    def _seed_events(self, conn: sqlite3.Connection) -> None:
        events = [
            ("Woodward Warmup", "circuit", "placeholder_track_01", 500, 1500, 3, 6),
            ("Factory Sprint", "sprint", "placeholder_track_02", 750, 2200, 2, 6),
            ("Midnight Test & Tune", "time_trial", "placeholder_track_03", 0, 800, 1, 1),
        ]
        conn.executemany(
            "INSERT OR IGNORE INTO events(name, event_type, track_key, entry_fee, reward, laps, max_players) VALUES (?, ?, ?, ?, ?, ?, ?)",
            events,
        )

    def create_account(self, username: str, password: str) -> dict[str, Any]:
        with self.db.connect() as conn:
            cur = conn.execute(
                "INSERT INTO accounts(username, password) VALUES (?, ?)",
                (username, password),
            )
            conn.commit()
            return {"account_id": cur.lastrowid, "username": username}

    def login(self, username: str, password: str) -> dict[str, Any]:
        with self.db.connect() as conn:
            row = conn.execute(
                "SELECT id, username FROM accounts WHERE username=? AND password=?",
                (username, password),
            ).fetchone()
            if not row:
                raise ValueError("invalid credentials")
            return {"account_id": row["id"], "username": row["username"]}

    def create_driver(self, account_id: int, driver_name: str, starting_cash: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            cur = conn.execute(
                "INSERT INTO profiles(account_id, driver_name, cash_balance) VALUES (?, ?, ?)",
                (account_id, driver_name, starting_cash),
            )
            profile_id = cur.lastrowid
            garage_cur = conn.execute(
                "INSERT INTO garages(profile_id, name) VALUES (?, ?)",
                (profile_id, f"{driver_name}'s Garage"),
            )
            conn.execute(
                "INSERT INTO transactions(profile_id, transaction_type, amount_delta, balance_after, note) VALUES (?, 'starting_cash', ?, ?, 'Initial local shard grant')",
                (profile_id, starting_cash, starting_cash),
            )
            conn.commit()
            return {
                "profile_id": profile_id,
                "garage_id": garage_cur.lastrowid,
                "driver_name": driver_name,
                "cash_balance": starting_cash,
            }

    def list_profiles(self, account_id: int) -> list[dict[str, Any]]:
        with self.db.connect() as conn:
            rows = conn.execute(
                "SELECT id, driver_name, cash_balance FROM profiles WHERE account_id=? ORDER BY id",
                (account_id,),
            ).fetchall()
            return [dict(row) for row in rows]

    def list_dealerships(self) -> list[dict[str, Any]]:
        with self.db.connect() as conn:
            dealers = conn.execute("SELECT * FROM dealerships ORDER BY id").fetchall()
            out = []
            for dealer in dealers:
                inv = conn.execute(
                    "SELECT * FROM dealership_inventory WHERE dealership_id=? ORDER BY id",
                    (dealer["id"],),
                ).fetchall()
                out.append({**dict(dealer), "inventory": [dict(row) for row in inv]})
            return out

    def list_events(self) -> list[dict[str, Any]]:
        with self.db.connect() as conn:
            return [dict(row) for row in conn.execute("SELECT * FROM events ORDER BY id").fetchall()]

    def get_profile_snapshot(self, profile_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            profile = conn.execute("SELECT * FROM profiles WHERE id=?", (profile_id,)).fetchone()
            if not profile:
                raise ValueError("profile not found")
            garage = conn.execute("SELECT * FROM garages WHERE profile_id=?", (profile_id,)).fetchone()
            cars = conn.execute(
                """
                SELECT oc.id, oc.nickname, c.display_name, c.class_name, c.model_key
                FROM owned_cars oc
                JOIN cars c ON c.id = oc.car_id
                WHERE oc.profile_id=? AND oc.active=1
                ORDER BY oc.id
                """,
                (profile_id,),
            ).fetchall()
            parts = conn.execute(
                """
                SELECT op.id, pc.display_name, pc.slot_name, op.state, op.owned_car_id
                FROM owned_parts op
                JOIN parts_catalog pc ON pc.id = op.part_id
                WHERE op.profile_id=?
                ORDER BY op.id
                """,
                (profile_id,),
            ).fetchall()
            builds = conn.execute(
                "SELECT * FROM car_builds WHERE owned_car_id IN (SELECT id FROM owned_cars WHERE profile_id=?) ORDER BY owned_car_id, slot_name",
                (profile_id,),
            ).fetchall()
            return {
                "profile": dict(profile),
                "garage": dict(garage) if garage else None,
                "cars": [dict(row) for row in cars],
                "owned_parts": [dict(row) for row in parts],
                "car_builds": [dict(row) for row in builds],
            }

    def purchase_car(self, profile_id: int, dealership_id: int, car_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            dealer_item = conn.execute(
                "SELECT * FROM dealership_inventory WHERE dealership_id=? AND item_type='car' AND ref_id=?",
                (dealership_id, car_id),
            ).fetchone()
            if not dealer_item:
                raise ValueError("car not sold here")
            profile = conn.execute("SELECT * FROM profiles WHERE id=?", (profile_id,)).fetchone()
            if profile["cash_balance"] < dealer_item["price"]:
                raise ValueError("insufficient funds")
            garage = conn.execute("SELECT * FROM garages WHERE profile_id=?", (profile_id,)).fetchone()
            cur = conn.execute(
                "INSERT INTO owned_cars(profile_id, garage_id, car_id) VALUES (?, ?, ?)",
                (profile_id, garage["id"], car_id),
            )
            new_balance = profile["cash_balance"] - dealer_item["price"]
            conn.execute("UPDATE profiles SET cash_balance=? WHERE id=?", (new_balance, profile_id))
            conn.execute(
                "INSERT INTO transactions(profile_id, transaction_type, amount_delta, balance_after, reference_type, reference_id, note) VALUES (?, 'buy_car', ?, ?, 'car', ?, 'Dealership purchase')",
                (profile_id, -dealer_item["price"], new_balance, car_id),
            )
            conn.commit()
            return {"owned_car_id": cur.lastrowid, "cash_balance": new_balance}

    def purchase_part(self, profile_id: int, dealership_id: int, part_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            dealer_item = conn.execute(
                "SELECT * FROM dealership_inventory WHERE dealership_id=? AND item_type='part' AND ref_id=?",
                (dealership_id, part_id),
            ).fetchone()
            if not dealer_item:
                raise ValueError("part not sold here")
            profile = conn.execute("SELECT * FROM profiles WHERE id=?", (profile_id,)).fetchone()
            if profile["cash_balance"] < dealer_item["price"]:
                raise ValueError("insufficient funds")
            cur = conn.execute(
                "INSERT INTO owned_parts(profile_id, part_id, state) VALUES (?, ?, 'inventory')",
                (profile_id, part_id),
            )
            conn.execute(
                "INSERT INTO inventory_items(profile_id, item_type, ref_id, quantity) VALUES (?, 'part', ?, 1)",
                (profile_id, cur.lastrowid),
            )
            new_balance = profile["cash_balance"] - dealer_item["price"]
            conn.execute("UPDATE profiles SET cash_balance=? WHERE id=?", (new_balance, profile_id))
            conn.execute(
                "INSERT INTO transactions(profile_id, transaction_type, amount_delta, balance_after, reference_type, reference_id, note) VALUES (?, 'buy_part', ?, ?, 'part', ?, 'Dealership purchase')",
                (profile_id, -dealer_item["price"], new_balance, part_id),
            )
            conn.commit()
            return {"owned_part_id": cur.lastrowid, "cash_balance": new_balance}

    def install_part(self, profile_id: int, owned_part_id: int, owned_car_id: int, slot_name: str) -> dict[str, Any]:
        with self.db.connect() as conn:
            part = conn.execute("SELECT * FROM owned_parts WHERE id=? AND profile_id=?", (owned_part_id, profile_id)).fetchone()
            if not part:
                raise ValueError("owned part not found")
            current = conn.execute(
                "SELECT owned_part_id FROM car_builds WHERE owned_car_id=? AND slot_name=?",
                (owned_car_id, slot_name),
            ).fetchone()
            if current and current["owned_part_id"] and current["owned_part_id"] != owned_part_id:
                conn.execute(
                    "UPDATE owned_parts SET state='inventory', owned_car_id=NULL, slot_name=NULL WHERE id=? AND profile_id=?",
                    (current["owned_part_id"], profile_id),
                )
                conn.execute(
                    "INSERT INTO inventory_items(profile_id, item_type, ref_id, quantity) VALUES (?, 'part', ?, 1)",
                    (profile_id, current["owned_part_id"]),
                )
            conn.execute("DELETE FROM inventory_items WHERE profile_id=? AND item_type='part' AND ref_id=?", (profile_id, owned_part_id))
            conn.execute(
                "INSERT INTO car_builds(owned_car_id, slot_name, owned_part_id) VALUES (?, ?, ?) ON CONFLICT(owned_car_id, slot_name) DO UPDATE SET owned_part_id=excluded.owned_part_id, installed_at=CURRENT_TIMESTAMP",
                (owned_car_id, slot_name, owned_part_id),
            )
            conn.execute(
                "UPDATE owned_parts SET state='installed', owned_car_id=?, slot_name=? WHERE id=?",
                (owned_car_id, slot_name, owned_part_id),
            )
            conn.commit()
            return {"owned_part_id": owned_part_id, "owned_car_id": owned_car_id, "slot_name": slot_name}

    def remove_part(self, profile_id: int, owned_car_id: int, slot_name: str) -> dict[str, Any]:
        with self.db.connect() as conn:
            row = conn.execute(
                "SELECT owned_part_id FROM car_builds WHERE owned_car_id=? AND slot_name=?",
                (owned_car_id, slot_name),
            ).fetchone()
            if not row or row["owned_part_id"] is None:
                raise ValueError("no installed part in slot")
            owned_part_id = row["owned_part_id"]
            conn.execute("DELETE FROM car_builds WHERE owned_car_id=? AND slot_name=?", (owned_car_id, slot_name))
            conn.execute(
                "UPDATE owned_parts SET state='inventory', owned_car_id=NULL, slot_name=NULL WHERE id=? AND profile_id=?",
                (owned_part_id, profile_id),
            )
            conn.execute(
                "INSERT INTO inventory_items(profile_id, item_type, ref_id, quantity) VALUES (?, 'part', ?, 1)",
                (profile_id, owned_part_id),
            )
            conn.commit()
            return {"owned_part_id": owned_part_id, "slot_name": slot_name}

    def create_lobby(self, profile_id: int, event_id: int, lobby_name: str) -> dict[str, Any]:
        with self.db.connect() as conn:
            event = conn.execute("SELECT * FROM events WHERE id=?", (event_id,)).fetchone()
            if not event:
                raise ValueError("event not found")
            self._charge_entry_fee(conn, profile_id, event_id, event["entry_fee"])
            cur = conn.execute(
                "INSERT INTO race_lobbies(event_id, host_profile_id, lobby_name) VALUES (?, ?, ?)",
                (event_id, profile_id, lobby_name),
            )
            lobby_id = cur.lastrowid
            conn.execute(
                "INSERT INTO race_lobby_members(lobby_id, profile_id, ready) VALUES (?, ?, 0)",
                (lobby_id, profile_id),
            )
            conn.commit()
            return {"lobby_id": lobby_id, "state": "open"}

    def join_lobby(self, lobby_id: int, profile_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            lobby = conn.execute("SELECT * FROM race_lobbies WHERE id=?", (lobby_id,)).fetchone()
            if not lobby:
                raise ValueError("lobby not found")
            event = conn.execute("SELECT * FROM events WHERE id=?", (lobby["event_id"],)).fetchone()
            member_count = conn.execute(
                "SELECT COUNT(*) AS member_count FROM race_lobby_members WHERE lobby_id=?",
                (lobby_id,),
            ).fetchone()["member_count"]
            if member_count >= event["max_players"]:
                raise ValueError("lobby is full")
            self._charge_entry_fee(conn, profile_id, lobby["event_id"], event["entry_fee"])
            conn.execute(
                "INSERT OR IGNORE INTO race_lobby_members(lobby_id, profile_id, ready) VALUES (?, ?, 0)",
                (lobby_id, profile_id),
            )
            conn.commit()
            return {"lobby_id": lobby_id, "profile_id": profile_id}

    def set_lobby_ready(self, lobby_id: int, profile_id: int, ready: bool) -> dict[str, Any]:
        with self.db.connect() as conn:
            conn.execute(
                "UPDATE race_lobby_members SET ready=? WHERE lobby_id=? AND profile_id=?",
                (1 if ready else 0, lobby_id, profile_id),
            )
            rows = conn.execute("SELECT ready FROM race_lobby_members WHERE lobby_id=?", (lobby_id,)).fetchall()
            state = "ready" if rows and all(row["ready"] for row in rows) else "open"
            conn.execute("UPDATE race_lobbies SET state=? WHERE id=?", (state, lobby_id))
            conn.commit()
            return {"lobby_id": lobby_id, "state": state}

    def start_lobby(self, lobby_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            lobby = conn.execute("SELECT * FROM race_lobbies WHERE id=?", (lobby_id,)).fetchone()
            if not lobby:
                raise ValueError("lobby not found")
            if lobby["state"] != "ready":
                raise ValueError("lobby is not ready to start")
            conn.execute("UPDATE race_lobbies SET state='in_race', started_at=CURRENT_TIMESTAMP WHERE id=?", (lobby_id,))
            conn.commit()
            return {"lobby_id": lobby_id, "state": "in_race"}

    def launch_race(self, lobby_id: int, duration_seconds: int = 5, scene_type: str = "text_loop") -> dict[str, Any]:
        with self.db.connect() as conn:
            lobby = conn.execute("SELECT * FROM race_lobbies WHERE id=?", (lobby_id,)).fetchone()
            if not lobby:
                raise ValueError("lobby not found")
            if lobby["state"] not in {"ready", "in_race"}:
                raise ValueError("lobby is not ready for race launch")
            conn.execute("UPDATE race_lobbies SET state='in_race', started_at=COALESCE(started_at, CURRENT_TIMESTAMP) WHERE id=?", (lobby_id,))
            config = {"duration_seconds": duration_seconds, "laps": 1}
            conn.execute(
                """
                INSERT INTO race_sessions(lobby_id, status, scene_type, config_json)
                VALUES (?, 'launched', ?, ?)
                ON CONFLICT(lobby_id) DO UPDATE SET status='launched', scene_type=excluded.scene_type, config_json=excluded.config_json, launched_at=CURRENT_TIMESTAMP, completed_at=NULL
                """,
                (lobby_id, scene_type, json.dumps(config)),
            )
            conn.commit()
            return {"lobby_id": lobby_id, "state": "in_race", "race_session": {"scene_type": scene_type, **config}}

    def get_race_session(self, lobby_id: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            session = conn.execute("SELECT * FROM race_sessions WHERE lobby_id=?", (lobby_id,)).fetchone()
            if not session:
                raise ValueError("race session not found")
            data = dict(session)
            data["config"] = json.loads(data.pop("config_json"))
            return data

    def run_interactive_race(self, lobby_id: int, profile_id: int, total_laps: int = 1, time_limit: float = 120.0) -> dict[str, Any]:
        """Launch pygame interactive race scene. Falls back to text loop if pygame unavailable."""
        try:
            import subprocess, sys
            scene_path = Path(__file__).parent.parent.parent / "client" / "preservation_client" / "race_scene.py"
            result = subprocess.run(
                [sys.executable, str(scene_path)],
                capture_output=True,
                text=True,
                timeout=int(time_limit + 30),
                input="\n",  # newline to unblock stdin if pygame not available
            )
            # Read last line as JSON result
            lines = [l for l in result.stdout.strip().split("\n") if l.strip()]
            race_data = None
            for line in reversed(lines):
                if line.startswith("{"):
                    import json as _json
                    race_data = _json.loads(line)
                    break
            if race_data is None:
                raise ValueError(f"Could not parse race result from output: {result.stdout[:200]}")
        except subprocess.TimeoutExpired:
            raise RuntimeError("Race timed out")
        except Exception as exc:
            # Fallback to text race
            race_data = {
                "finish_time_ms": 0,
                "laps_completed": 0,
                "dnf": True,
                "best_lap_ms": None,
                "total_time_ms": int(time_limit * 1000),
            }

        with self.db.connect() as conn:
            conn.execute(
                "UPDATE race_sessions SET status='completed', completed_at=CURRENT_TIMESTAMP WHERE lobby_id=?",
                (lobby_id,),
            )
            conn.commit()

        finish_time_ms = race_data["finish_time_ms"]
        dnf = race_data.get("dnf", False)
        laps = race_data.get("laps_completed", 0)
        position = 1 if not dnf and laps >= total_laps else 99

        result = self.submit_race_result(lobby_id, profile_id, position, finish_time_ms)
        reward_note = f"Race complete! "
        if dnf:
            reward_note += f"DNF ({laps} lap(s)). "
        reward_note += f"Reward credited: ${result['reward']}"
        result["reward_notification"] = reward_note
        result["finish_time_ms"] = finish_time_ms
        result["laps_completed"] = laps
        result["dnf"] = dnf
        result["best_lap_ms"] = race_data.get("best_lap_ms")
        return result


        command_log = command_log or []
        session = self.get_race_session(lobby_id)
        duration = max(1, int(session["config"].get("duration_seconds", 5)))
        accelerate_count = sum(1 for command in command_log if command == "accelerate")
        brake_count = sum(1 for command in command_log if command == "brake")
        steer_count = sum(1 for command in command_log if command.startswith("steer"))
        # GUESSED: simple preservation-mode scoring that rewards active input and lightly penalizes braking.
        finish_time_ms = max(1500, duration * 1000 - accelerate_count * 120 + brake_count * 40 + steer_count * 10)
        with self.db.connect() as conn:
            conn.execute(
                "UPDATE race_sessions SET status='completed', completed_at=CURRENT_TIMESTAMP WHERE lobby_id=?",
                (lobby_id,),
            )
            conn.commit()
        result = self.submit_race_result(lobby_id, profile_id, position=1, finish_time_ms=finish_time_ms)
        result["reward_notification"] = f"Race complete. Reward credited: ${result['reward']}"
        result["finish_time_ms"] = finish_time_ms
        return result

    def submit_race_result(self, lobby_id: int, profile_id: int, position: int, finish_time_ms: int) -> dict[str, Any]:
        with self.db.connect() as conn:
            lobby = conn.execute("SELECT * FROM race_lobbies WHERE id=?", (lobby_id,)).fetchone()
            event = conn.execute("SELECT * FROM events WHERE id=?", (lobby["event_id"],)).fetchone()
            reward = event["reward"] if position == 1 else max(250, event["reward"] // 3)
            conn.execute(
                "INSERT INTO race_results(lobby_id, profile_id, position, finish_time_ms, reward_paid) VALUES (?, ?, ?, ?, ?)",
                (lobby_id, profile_id, position, finish_time_ms, reward),
            )
            profile = conn.execute("SELECT cash_balance FROM profiles WHERE id=?", (profile_id,)).fetchone()
            new_balance = profile["cash_balance"] + reward
            conn.execute("UPDATE profiles SET cash_balance=? WHERE id=?", (new_balance, profile_id))
            conn.execute(
                "INSERT INTO transactions(profile_id, transaction_type, amount_delta, balance_after, reference_type, reference_id, note) VALUES (?, 'race_reward', ?, ?, 'lobby', ?, 'Placeholder race payout')",
                (profile_id, reward, new_balance, lobby_id),
            )
            conn.execute("UPDATE race_lobbies SET state='completed' WHERE id=?", (lobby_id,))
            conn.commit()
            return {"lobby_id": lobby_id, "reward": reward, "cash_balance": new_balance}

    def list_lobbies(self) -> list[dict[str, Any]]:
        with self.db.connect() as conn:
            lobbies = conn.execute("SELECT * FROM race_lobbies ORDER BY id DESC").fetchall()
            out = []
            for lobby in lobbies:
                members = conn.execute(
                    "SELECT rlm.profile_id, rlm.ready, p.driver_name FROM race_lobby_members rlm JOIN profiles p ON p.id = rlm.profile_id WHERE rlm.lobby_id=? ORDER BY rlm.id",
                    (lobby["id"],),
                ).fetchall()
                race_session = conn.execute(
                    "SELECT status, scene_type, config_json, launched_at, completed_at FROM race_sessions WHERE lobby_id=?",
                    (lobby["id"],),
                ).fetchone()
                lobby_data = {**dict(lobby), "members": [dict(row) for row in members]}
                if race_session:
                    lobby_data["race_session"] = {
                        **dict(race_session),
                        "config": json.loads(race_session["config_json"]),
                    }
                    lobby_data["race_session"].pop("config_json", None)
                out.append(lobby_data)
            return out

    def _charge_entry_fee(self, conn: sqlite3.Connection, profile_id: int, event_id: int, entry_fee: int) -> None:
        if entry_fee <= 0:
            return
        exists = conn.execute(
            "SELECT 1 FROM transactions WHERE profile_id=? AND transaction_type='event_entry_fee' AND reference_type='event' AND reference_id=? LIMIT 1",
            (profile_id, event_id),
        ).fetchone()
        if exists:
            return
        profile = conn.execute("SELECT cash_balance FROM profiles WHERE id=?", (profile_id,)).fetchone()
        if not profile:
            raise ValueError("profile not found")
        if profile["cash_balance"] < entry_fee:
            raise ValueError("insufficient funds for event entry")
        new_balance = profile["cash_balance"] - entry_fee
        conn.execute("UPDATE profiles SET cash_balance=? WHERE id=?", (new_balance, profile_id))
        conn.execute(
            "INSERT INTO transactions(profile_id, transaction_type, amount_delta, balance_after, reference_type, reference_id, note) VALUES (?, 'event_entry_fee', ?, ?, 'event', ?, 'Event entry fee')",
            (profile_id, -entry_fee, new_balance, event_id),
        )

    @staticmethod
    def _coerce_price(raw: str | None, fallback: int) -> int:
        if raw is None:
            return fallback
        digits = "".join(ch for ch in str(raw) if ch.isdigit())
        return int(digits) if digits else fallback
