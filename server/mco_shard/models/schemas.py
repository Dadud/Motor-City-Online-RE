from __future__ import annotations

from pydantic import BaseModel
from typing import Any


class AccountCreate(BaseModel):
    username: str
    password: str


class LoginRequest(BaseModel):
    username: str
    password: str


class DriverCreate(BaseModel):
    account_id: int
    driver_name: str
    starting_cash: int = 20000


class DealershipPurchaseRequest(BaseModel):
    profile_id: int
    dealership_id: int
    car_id: int


class PartPurchaseRequest(BaseModel):
    profile_id: int
    dealership_id: int
    part_id: int


class InstallPartRequest(BaseModel):
    profile_id: int
    owned_part_id: int
    owned_car_id: int
    slot_name: str


class RemovePartRequest(BaseModel):
    profile_id: int
    owned_car_id: int
    slot_name: str


class LobbyCreateRequest(BaseModel):
    profile_id: int
    event_id: int
    lobby_name: str


class LobbyJoinRequest(BaseModel):
    profile_id: int


class LobbyReadyRequest(BaseModel):
    profile_id: int
    ready: bool


class LobbyLaunchRaceRequest(BaseModel):
    duration_seconds: int = 5
    scene_type: str = "text_loop"


class RaceResultSubmitRequest(BaseModel):
    lobby_id: int
    profile_id: int
    position: int
    finish_time_ms: int


class PlaceholderRaceRunRequest(BaseModel):
    profile_id: int
    commands: list[str] = []


class ApiMessage(BaseModel):
    status: str
    data: Any
