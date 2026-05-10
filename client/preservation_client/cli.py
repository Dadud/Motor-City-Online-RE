from __future__ import annotations

import json
import sys
from urllib import request
from urllib.error import HTTPError


BASE_URL = "http://127.0.0.1:8765"


def _call(method: str, path: str, payload: dict | None = None):
    data = None if payload is None else json.dumps(payload).encode("utf-8")
    req = request.Request(
        f"{BASE_URL}{path}",
        data=data,
        headers={"Content-Type": "application/json"},
        method=method,
    )
    try:
        with request.urlopen(req) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        raise SystemExit(f"HTTP {exc.code}: {body}") from exc


def run_demo() -> None:
    print("== MCO Local Shard demo ==")
    account = _call("POST", "/accounts", {"username": "demo", "password": "demo"})
    account_id = account["data"]["account_id"]
    print("created account", account["data"])

    login = _call("POST", "/login", {"username": "demo", "password": "demo"})
    print("logged in", login["data"])

    driver = _call("POST", "/drivers", {"account_id": account_id, "driver_name": "DemoDriver"})
    profile_id = driver["data"]["profile_id"]
    print("created driver", driver["data"])

    dealers = _call("GET", "/dealerships")
    print("dealerships loaded", len(dealers["data"]))
    car_dealer = next(d for d in dealers["data"] if d["kind"] == "cars")
    part_dealer = next(d for d in dealers["data"] if d["kind"] == "parts")
    first_car = next(i for i in car_dealer["inventory"] if i["item_type"] == "car")
    first_part = next(i for i in part_dealer["inventory"] if i["item_type"] == "part")

    owned_car = _call("POST", "/dealerships/buy-car", {"profile_id": profile_id, "dealership_id": car_dealer["id"], "car_id": first_car["ref_id"]})
    owned_car_id = owned_car["data"]["owned_car_id"]
    print("bought car", owned_car["data"])

    owned_part = _call("POST", "/dealerships/buy-part", {"profile_id": profile_id, "dealership_id": part_dealer["id"], "part_id": first_part["ref_id"]})
    owned_part_id = owned_part["data"]["owned_part_id"]
    print("bought part", owned_part["data"])

    installed = _call("POST", "/garage/install-part", {"profile_id": profile_id, "owned_part_id": owned_part_id, "owned_car_id": owned_car_id, "slot_name": "engine"})
    print("installed part", installed["data"])

    events = _call("GET", "/events")
    first_event = events["data"][0]
    print("events loaded", len(events["data"]))

    lobby = _call("POST", "/lobbies", {"profile_id": profile_id, "event_id": first_event["id"], "lobby_name": "Demo Lobby"})
    lobby_id = lobby["data"]["lobby_id"]
    print("created lobby", lobby["data"])

    ready = _call("POST", f"/lobbies/{lobby_id}/ready", {"profile_id": profile_id, "ready": True})
    print("ready state", ready["data"])

    started = _call("POST", f"/lobbies/{lobby_id}/start")
    print("started lobby", started["data"])

    result = _call("POST", "/race-results", {"lobby_id": lobby_id, "profile_id": profile_id, "position": 1, "finish_time_ms": 123456})
    print("submitted result", result["data"])

    snapshot = _call("GET", f"/profiles/{profile_id}/snapshot")
    print(json.dumps(snapshot["data"], indent=2))


def main() -> None:
    command = sys.argv[1] if len(sys.argv) > 1 else "demo"
    if command == "demo":
        run_demo()
        return
    raise SystemExit(f"unknown command: {command}")
