from __future__ import annotations

from pathlib import Path

import pytest

from server.mco_shard.persistence.database import Database, LATEST_SCHEMA_VERSION
from server.mco_shard.services.shard_service import ShardService


@pytest.fixture()
def service(tmp_path: Path) -> ShardService:
    db = Database(tmp_path / "test_shard.db")
    svc = ShardService(db, Path(__file__).resolve().parents[1])
    svc.init(seed=True)
    return svc


def _create_profile(service: ShardService, username: str, driver_name: str, starting_cash: int = 20000) -> dict:
    account = service.create_account(username, "secret")
    return service.create_driver(account["account_id"], driver_name, starting_cash)


def test_schema_upgrade_and_downgrade(tmp_path: Path) -> None:
    db = Database(tmp_path / "migration.db")
    assert db.upgrade() == LATEST_SCHEMA_VERSION
    assert db.schema_version() == LATEST_SCHEMA_VERSION
    assert db.downgrade(1) == 1
    assert db.upgrade() == LATEST_SCHEMA_VERSION


def test_duplicate_username_rejected(service: ShardService) -> None:
    service.create_account("demo", "secret")
    with pytest.raises(Exception):
        service.create_account("demo", "secret")


def test_login_wrong_password_rejected(service: ShardService) -> None:
    service.create_account("demo", "secret")
    with pytest.raises(ValueError, match="invalid credentials"):
        service.login("demo", "wrong")


def test_purchase_flow_debits_car_and_updates_cash(service: ShardService) -> None:
    profile = _create_profile(service, "buyer", "Buyer")
    dealers = service.list_dealerships()
    car_dealer = next(dealer for dealer in dealers if dealer["kind"] == "cars")
    car = next(item for item in car_dealer["inventory"] if item["item_type"] == "car")

    result = service.purchase_car(profile["profile_id"], car_dealer["id"], car["ref_id"])
    snapshot = service.get_profile_snapshot(profile["profile_id"])

    assert result["owned_car_id"] > 0
    assert snapshot["profile"]["cash_balance"] == profile["cash_balance"] - car["price"]
    assert len(snapshot["cars"]) == 1


def test_install_part_handles_slot_conflict(service: ShardService) -> None:
    profile = _create_profile(service, "builder", "Builder")
    dealers = service.list_dealerships()
    car_dealer = next(dealer for dealer in dealers if dealer["kind"] == "cars")
    part_dealer = next(dealer for dealer in dealers if dealer["kind"] == "parts")
    car = next(item for item in car_dealer["inventory"] if item["item_type"] == "car")
    engine_parts = [item for item in part_dealer["inventory"] if item["item_type"] == "part"]

    owned_car = service.purchase_car(profile["profile_id"], car_dealer["id"], car["ref_id"])
    first_part = service.purchase_part(profile["profile_id"], part_dealer["id"], engine_parts[0]["ref_id"])
    second_part = service.purchase_part(profile["profile_id"], part_dealer["id"], engine_parts[2]["ref_id"])

    service.install_part(profile["profile_id"], first_part["owned_part_id"], owned_car["owned_car_id"], "engine")
    service.install_part(profile["profile_id"], second_part["owned_part_id"], owned_car["owned_car_id"], "engine")
    snapshot = service.get_profile_snapshot(profile["profile_id"])

    build = next(row for row in snapshot["car_builds"] if row["slot_name"] == "engine")
    reverted_part = next(row for row in snapshot["owned_parts"] if row["id"] == first_part["owned_part_id"])
    active_part = next(row for row in snapshot["owned_parts"] if row["id"] == second_part["owned_part_id"])

    assert build["owned_part_id"] == second_part["owned_part_id"]
    assert reverted_part["state"] == "inventory"
    assert active_part["state"] == "installed"


def test_lobby_state_transition_join_ready_start(service: ShardService) -> None:
    host = _create_profile(service, "host", "Host")
    guest = _create_profile(service, "guest", "Guest")
    event = service.list_events()[0]

    lobby = service.create_lobby(host["profile_id"], event["id"], "Test Lobby")
    service.join_lobby(lobby["lobby_id"], guest["profile_id"])
    assert service.set_lobby_ready(lobby["lobby_id"], host["profile_id"], True)["state"] == "open"
    assert service.set_lobby_ready(lobby["lobby_id"], guest["profile_id"], True)["state"] == "ready"
    assert service.start_lobby(lobby["lobby_id"])["state"] == "in_race"


def test_race_result_pays_out_cash(service: ShardService) -> None:
    profile = _create_profile(service, "racer", "Racer")
    event = service.list_events()[0]
    lobby = service.create_lobby(profile["profile_id"], event["id"], "Race Night")
    service.set_lobby_ready(lobby["lobby_id"], profile["profile_id"], True)
    service.start_lobby(lobby["lobby_id"])

    before = service.get_profile_snapshot(profile["profile_id"])["profile"]["cash_balance"]
    result = service.submit_race_result(lobby["lobby_id"], profile["profile_id"], 1, 99999)
    after = service.get_profile_snapshot(profile["profile_id"])["profile"]["cash_balance"]

    assert result["reward"] == event["reward"]
    assert after == before + event["reward"]


def test_placeholder_race_launch_and_completion(service: ShardService) -> None:
    profile = _create_profile(service, "driver", "Driver")
    event = service.list_events()[0]
    lobby = service.create_lobby(profile["profile_id"], event["id"], "Launch Test")
    service.set_lobby_ready(lobby["lobby_id"], profile["profile_id"], True)

    launch = service.launch_race(lobby["lobby_id"], duration_seconds=3)
    result = service.run_placeholder_race(lobby["lobby_id"], profile["profile_id"], ["accelerate", "accelerate", "steer_left"])

    assert launch["race_session"]["duration_seconds"] == 3
    assert result["reward"] == event["reward"]
    assert "Reward credited" in result["reward_notification"]
