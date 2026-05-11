from __future__ import annotations

from pathlib import Path

import uvicorn
from fastapi import FastAPI, HTTPException

from server.mco_shard.config import settings
from server.mco_shard.models.schemas import (
    AccountCreate,
    DealershipPurchaseRequest,
    DriverCreate,
    InstallPartRequest,
    LobbyCreateRequest,
    LobbyJoinRequest,
    LobbyLaunchRaceRequest,
    LobbyReadyRequest,
    LoginRequest,
    PartPurchaseRequest,
    PlaceholderRaceRunRequest,
    InteractiveRaceRunRequest,
    RaceResultSubmitRequest,
    RemovePartRequest,
)
from server.mco_shard.persistence.database import Database
from server.mco_shard.services.shard_service import ShardService


repo_root = Path(__file__).resolve().parents[2]
database = Database(repo_root / settings.db_path)
service = ShardService(database, repo_root)
service.init(seed=settings.seed_on_start)

app = FastAPI(title="MCO Local Shard", version="0.1.0")


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok", "server": "mco-local-shard"}


@app.post("/accounts")
def create_account(payload: AccountCreate):
    try:
        return {"status": "ok", "data": service.create_account(payload.username, payload.password)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/login")
def login(payload: LoginRequest):
    try:
        return {"status": "ok", "data": service.login(payload.username, payload.password)}
    except Exception as exc:
        raise HTTPException(status_code=401, detail=str(exc)) from exc


@app.post("/drivers")
def create_driver(payload: DriverCreate):
    try:
        return {"status": "ok", "data": service.create_driver(payload.account_id, payload.driver_name, payload.starting_cash)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.get("/accounts/{account_id}/profiles")
def list_profiles(account_id: int):
    return {"status": "ok", "data": service.list_profiles(account_id)}


@app.get("/profiles/{profile_id}/snapshot")
def profile_snapshot(profile_id: int):
    try:
        return {"status": "ok", "data": service.get_profile_snapshot(profile_id)}
    except Exception as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc


@app.get("/dealerships")
def dealerships():
    return {"status": "ok", "data": service.list_dealerships()}


@app.post("/dealerships/buy-car")
def buy_car(payload: DealershipPurchaseRequest):
    try:
        return {"status": "ok", "data": service.purchase_car(payload.profile_id, payload.dealership_id, payload.car_id)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/dealerships/buy-part")
def buy_part(payload: PartPurchaseRequest):
    try:
        return {"status": "ok", "data": service.purchase_part(payload.profile_id, payload.dealership_id, payload.part_id)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/garage/install-part")
def install_part(payload: InstallPartRequest):
    try:
        return {"status": "ok", "data": service.install_part(payload.profile_id, payload.owned_part_id, payload.owned_car_id, payload.slot_name)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/garage/remove-part")
def remove_part(payload: RemovePartRequest):
    try:
        return {"status": "ok", "data": service.remove_part(payload.profile_id, payload.owned_car_id, payload.slot_name)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.get("/events")
def events():
    return {"status": "ok", "data": service.list_events()}


@app.get("/lobbies")
def lobbies():
    return {"status": "ok", "data": service.list_lobbies()}


@app.post("/lobbies")
def create_lobby(payload: LobbyCreateRequest):
    try:
        return {"status": "ok", "data": service.create_lobby(payload.profile_id, payload.event_id, payload.lobby_name)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/join")
def join_lobby(lobby_id: int, payload: LobbyJoinRequest):
    try:
        return {"status": "ok", "data": service.join_lobby(lobby_id, payload.profile_id)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/ready")
def ready_lobby(lobby_id: int, payload: LobbyReadyRequest):
    try:
        return {"status": "ok", "data": service.set_lobby_ready(lobby_id, payload.profile_id, payload.ready)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/start")
def start_lobby(lobby_id: int):
    try:
        return {"status": "ok", "data": service.start_lobby(lobby_id)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/launch-race")
def launch_race(lobby_id: int, payload: LobbyLaunchRaceRequest):
    try:
        return {"status": "ok", "data": service.launch_race(lobby_id, payload.duration_seconds, payload.scene_type)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.get("/lobbies/{lobby_id}/race-session")
def race_session(lobby_id: int):
    try:
        return {"status": "ok", "data": service.get_race_session(lobby_id)}
    except Exception as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/run-placeholder-race")
def run_placeholder_race(lobby_id: int, payload: PlaceholderRaceRunRequest):
    try:
        return {"status": "ok", "data": service.run_placeholder_race(lobby_id, payload.profile_id, payload.commands)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/lobbies/{lobby_id}/run-interactive-race")
def run_interactive_race(lobby_id: int, payload: InteractiveRaceRunRequest):
    try:
        return {"status": "ok", "data": service.run_interactive_race(lobby_id, payload.profile_id, payload.total_laps, payload.time_limit)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


@app.post("/race-results")
def submit_race_result(payload: RaceResultSubmitRequest):
    try:
        return {"status": "ok", "data": service.submit_race_result(payload.lobby_id, payload.profile_id, payload.position, payload.finish_time_ms)}
    except Exception as exc:
        raise HTTPException(status_code=400, detail=str(exc)) from exc


def main() -> None:
    uvicorn.run(app, host=settings.host, port=settings.port)


if __name__ == "__main__":
    main()
