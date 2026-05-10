from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from urllib import request
from urllib.error import HTTPError

from rich.console import Console
from rich.panel import Panel
from rich.prompt import Confirm, IntPrompt, Prompt
from rich.table import Table


BASE_URL = "http://127.0.0.1:8765"
console = Console()


@dataclass
class SessionState:
    account_id: int | None = None
    profile_id: int | None = None
    username: str | None = None
    driver_name: str | None = None


state = SessionState()


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
        raise RuntimeError(f"HTTP {exc.code}: {body}") from exc


def render_snapshot(snapshot: dict) -> None:
    profile = snapshot["profile"]
    console.print(Panel.fit(f"[bold]{profile['driver_name']}[/bold]\nCash: ${profile['cash_balance']}", title="Driver"))

    car_table = Table(title="Garage")
    car_table.add_column("Owned Car ID")
    car_table.add_column("Model")
    car_table.add_column("Class")
    car_table.add_column("Installed Parts")
    build_map: dict[int, list[str]] = {}
    for build in snapshot["car_builds"]:
        build_map.setdefault(build["owned_car_id"], []).append(f"{build['slot_name']} -> part #{build['owned_part_id']}")
    for car in snapshot["cars"]:
        car_table.add_row(str(car["id"]), car["display_name"], car["class_name"] or "-", "\n".join(build_map.get(car["id"], ["-"])) )
    console.print(car_table)

    part_table = Table(title="Owned Parts")
    part_table.add_column("Owned Part ID")
    part_table.add_column("Part")
    part_table.add_column("Slot")
    part_table.add_column("State")
    for part in snapshot["owned_parts"]:
        part_table.add_row(str(part["id"]), part["display_name"], part["slot_name"], part["state"])
    console.print(part_table)


def show_dealerships() -> list[dict]:
    dealers = _call("GET", "/dealerships")["data"]
    for dealer in dealers:
        table = Table(title=f"{dealer['name']} ({dealer['kind']})")
        table.add_column("Ref ID")
        table.add_column("Type")
        table.add_column("Price")
        table.add_column("Stock")
        for item in dealer["inventory"]:
            table.add_row(str(item["ref_id"]), item["item_type"], f"${item['price']}", str(item["stock"]))
        console.print(table)
    return dealers


def show_events() -> list[dict]:
    events = _call("GET", "/events")["data"]
    table = Table(title="Events")
    table.add_column("ID")
    table.add_column("Name")
    table.add_column("Type")
    table.add_column("Fee")
    table.add_column("Reward")
    table.add_column("Laps")
    for event in events:
        table.add_row(str(event["id"]), event["name"], event["event_type"], f"${event['entry_fee']}", f"${event['reward']}", str(event["laps"]))
    console.print(table)
    return events


def show_lobbies() -> list[dict]:
    lobbies = _call("GET", "/lobbies")["data"]
    if not lobbies:
        console.print("[yellow]No lobbies yet.[/yellow]")
        return []
    for lobby in lobbies:
        table = Table(title=f"Lobby #{lobby['id']} - {lobby['lobby_name']} [{lobby['state']}]")
        table.add_column("Profile ID")
        table.add_column("Driver")
        table.add_column("Ready")
        for member in lobby["members"]:
            table.add_row(str(member["profile_id"]), member["driver_name"], "yes" if member["ready"] else "no")
        console.print(table)
        if lobby.get("race_session"):
            session = lobby["race_session"]
            console.print(f"Race session: {session['status']} / {session['scene_type']} / {session['config']}")
    return lobbies


def ensure_login() -> None:
    if state.profile_id:
        return
    console.print(Panel.fit("MCO Preservation Client", subtitle="Local shard"))
    choice = Prompt.ask("[l]ogin or [r]egister", choices=["l", "r"], default="r")
    username = Prompt.ask("Username", default="demo")
    password = Prompt.ask("Password", default="demo")
    if choice == "r":
        account = _call("POST", "/accounts", {"username": username, "password": password})["data"]
    else:
        account = _call("POST", "/login", {"username": username, "password": password})["data"]
    state.account_id = account["account_id"]
    state.username = username

    profiles = _call("GET", f"/accounts/{state.account_id}/profiles")["data"]
    if profiles:
        profile = profiles[0]
    else:
        driver_name = Prompt.ask("Driver name", default=f"{username.title()}Driver")
        profile = _call("POST", "/drivers", {"account_id": state.account_id, "driver_name": driver_name})["data"]
    state.profile_id = profile["profile_id"] if "profile_id" in profile else profile["id"]
    state.driver_name = profile.get("driver_name")


def buy_car_flow() -> None:
    dealers = show_dealerships()
    car_dealer = next(dealer for dealer in dealers if dealer["kind"] == "cars")
    car_id = IntPrompt.ask("Car ref id to buy")
    result = _call("POST", "/dealerships/buy-car", {"profile_id": state.profile_id, "dealership_id": car_dealer["id"], "car_id": car_id})["data"]
    console.print(f"[green]Bought car.[/green] Owned car id: {result['owned_car_id']} | Cash: ${result['cash_balance']}")


def buy_part_flow() -> None:
    dealers = show_dealerships()
    part_dealer = next(dealer for dealer in dealers if dealer["kind"] == "parts")
    part_id = IntPrompt.ask("Part ref id to buy")
    result = _call("POST", "/dealerships/buy-part", {"profile_id": state.profile_id, "dealership_id": part_dealer["id"], "part_id": part_id})["data"]
    console.print(f"[green]Bought part.[/green] Owned part id: {result['owned_part_id']} | Cash: ${result['cash_balance']}")


def install_part_flow() -> None:
    snapshot = _call("GET", f"/profiles/{state.profile_id}/snapshot")["data"]
    render_snapshot(snapshot)
    owned_part_id = IntPrompt.ask("Owned part id")
    owned_car_id = IntPrompt.ask("Owned car id")
    slot_name = Prompt.ask("Slot name", default="engine")
    result = _call("POST", "/garage/install-part", {"profile_id": state.profile_id, "owned_part_id": owned_part_id, "owned_car_id": owned_car_id, "slot_name": slot_name})["data"]
    console.print(f"[green]Installed part.[/green] {result}")


def lobby_flow() -> None:
    events = show_events()
    event_id = IntPrompt.ask("Event id", default=events[0]["id"])
    lobby_name = Prompt.ask("Lobby name", default="Preservation Lobby")
    lobby = _call("POST", "/lobbies", {"profile_id": state.profile_id, "event_id": event_id, "lobby_name": lobby_name})["data"]
    lobby_id = lobby["lobby_id"]
    console.print(f"[green]Lobby created.[/green] #{lobby_id}")
    if Confirm.ask("Set ready now?", default=True):
        ready = _call("POST", f"/lobbies/{lobby_id}/ready", {"profile_id": state.profile_id, "ready": True})["data"]
        console.print(f"Lobby state: {ready['state']}")
    if Confirm.ask("Launch placeholder race?", default=True):
        launch = _call("POST", f"/lobbies/{lobby_id}/launch-race", {"duration_seconds": 5, "scene_type": "text_loop"})["data"]
        console.print(f"Race launched: {launch['race_session']}")
        run_race_flow(lobby_id)


def run_race_flow(lobby_id: int | None = None) -> None:
    if lobby_id is None:
        lobbies = show_lobbies()
        if not lobbies:
            return
        lobby_id = IntPrompt.ask("Lobby id", default=lobbies[0]["id"])
    console.print("Placeholder drive: type commands like accelerate, brake, steer_left, steer_right. Blank line to finish.")
    commands: list[str] = []
    while True:
        command = Prompt.ask("command", default="").strip()
        if not command:
            break
        commands.append(command)
    result = _call("POST", f"/lobbies/{lobby_id}/run-placeholder-race", {"profile_id": state.profile_id, "commands": commands})["data"]
    console.print(Panel.fit(f"Finish time: {result['finish_time_ms']} ms\n{result['reward_notification']}", title="Race Complete"))


def view_snapshot() -> None:
    snapshot = _call("GET", f"/profiles/{state.profile_id}/snapshot")["data"]
    render_snapshot(snapshot)


def run_demo() -> None:
    console.print("== MCO Local Shard demo ==")
    ensure_login()
    view_snapshot()
    show_dealerships()
    show_events()


def main() -> None:
    command = sys.argv[1] if len(sys.argv) > 1 else "menu"
    if command == "demo":
        run_demo()
        return
    ensure_login()
    actions = {
        "1": ("View garage snapshot", view_snapshot),
        "2": ("View dealerships", show_dealerships),
        "3": ("Buy car", buy_car_flow),
        "4": ("Buy part", buy_part_flow),
        "5": ("Install part", install_part_flow),
        "6": ("View events", show_events),
        "7": ("Create lobby / launch race", lobby_flow),
        "8": ("View lobbies", show_lobbies),
        "9": ("Run placeholder race", run_race_flow),
        "0": ("Quit", None),
    }
    while True:
        console.print()
        console.print(Panel.fit(f"Logged in as {state.username} / profile {state.profile_id}"))
        for key, (label, _) in actions.items():
            console.print(f"{key}. {label}")
        choice = Prompt.ask("Choose", choices=list(actions.keys()), default="1")
        if choice == "0":
            return
        handler = actions[choice][1]
        try:
            if handler:
                handler()
        except Exception as exc:
            console.print(f"[red]{exc}[/red]")


if __name__ == "__main__":
    main()
