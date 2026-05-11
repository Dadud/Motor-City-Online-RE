"""
Asset type detection for MCO install scans.
Clean-room implementation based on format research and path conventions.
"""

import re
from pathlib import Path
from typing import Optional

# Known MCO car model keys (from Cars.csv and VIV names)
KNOWN_CARS: dict[str, str] = {
    "53chevy": "1953 Chevrolet",
    "55cameo": "1955 Cameo",
    "56ftruck": "1956 Fleetside Truck",
    "59impala": "1959 Impala",
    "62vett": "1962 Corvette",       # GUESSED
    "64nova": "1964 Nova",           # GUESSED
    "67camaro": "1967 Camaro",       # GUESSED
    "69camaro": "1969 Camaro",       # GUESSED
    "70chevelle": "1970 Chevelle",   # GUESSED
    "70cougar": "1970 Cougar",       # GUESSED
    "70cuda": "1970 Barracuda",      # GUESSED
    "70mustang": "1970 Mustang",     # GUESSED
    "70charger": "1970 Charger",     # GUESSED
    "72elcamino": "1972 El Camino",  # GUESSED
    "8ball": "8 Ball",
    "96supra": "1996 Supra",
    "97eclps": "1997 Eclipse",
    # Additional MCO cars — names from format docs
    "96tss": "1996 TSS",             # GUESSED
    "99viper": "1999 Viper",         # GUESSED
    "00viper": "2000 Viper",         # GUESSED
    "01Z06": "2001 Corvette Z06",    # GUESSED
    "01NSX": "2001 NSX",             # GUESSED
    "01GTR": "2001 GT-R",            # GUESSED
}

# Known MCO tracks
KNOWN_TRACKS: dict[str, str] = {
    "boothill": "Boothill",
    "water": "Waterfront",           # GUESSED
    "grove": "The Grove",            # GUESSED
    "grove_night": "The Grove Night",# GUESSED
    "woodward": "Woodward Ave",      # GUESSED
    "m1car": "M1 Car",              # GUESSED
    "factory": "Factory",            # GUESSED
    "airport": "Airport",            # GUESSED
    "downtown": "Downtown",          # GUESSED
}

# Car filename patterns
CAR_PATTERNS = [
    # Direct model key in filename
    re.compile(r"^([0-9]{2}[a-z]+)\.viv$", re.IGNORECASE),
    # cars/ subdirectory
    re.compile(r"[/\\]cars[/\\]([0-9]{2}[a-z]+)\.viv$", re.IGNORECASE),
    # cars directory
    re.compile(r"[/\\]([0-9]{2}[a-z]+)[/\\]", re.IGNORECASE),
    # Section-based FCE car naming: :Hbody in filename
    re.compile(r":Hbody", re.IGNORECASE),
]

# Track filename patterns
TRACK_PATTERNS = [
    re.compile(r"[/\\]tracks[/\\]([a-z0-9_]+)\.frd$", re.IGNORECASE),
    re.compile(r"[/\\]tracks[/\\]([a-z0-9_]+)[/\\]", re.IGNORECASE),
    re.compile(r"^([a-z_]+)_track\.frd$", re.IGNORECASE),
]

# Parts filename patterns
PARTS_PATTERNS = [
    re.compile(r"[/\\]parts[/\\]([a-z0-9_]+)\.fce$", re.IGNORECASE),
    re.compile(r"[/\\]parts[/\\]([a-z0-9_]+)\.viv$", re.IGNORECASE),
    re.compile(r"[/\\]perf[/\\]([a-z0-9_]+)\.fce$", re.IGNORECASE),
]

# Audio filename patterns
AUDIO_PATTERNS = [
    re.compile(r"[/\\]audio[/\\]([a-z0-9_]+)\.bnk$", re.IGNORECASE),
    re.compile(r"[/\\]sfx[/\\]([a-z0-9_]+)\.bnk$", re.IGNORECASE),
]


def extract_model_key(path_str: str) -> Optional[str]:
    """Try to extract a car model key from a file path."""
    path_str = path_str.replace("\\", "/").lower()

    # Try direct filename match
    basename = Path(path_str).name
    for pattern in CAR_PATTERNS:
        m = pattern.match(basename)
        if m:
            key = m.group(1) if m.lastindex else None
            if key and key in KNOWN_CARS:
                return key
            if key:
                return key

    # Try cars/ subdirectory match
    for pattern in CAR_PATTERNS:
        m = pattern.search(path_str)
        if m:
            key = m.group(1) if m.lastindex else None
            if key and key in KNOWN_CARS:
                return key
            if key:
                return key

    # Try to find known car keys anywhere in path
    for key in KNOWN_CARS:
        if key in path_str:
            return key

    return None


def extract_track_key(path_str: str) -> Optional[str]:
    """Try to extract a track key from a file path."""
    path_str = path_str.replace("\\", "/").lower()

    for pattern in TRACK_PATTERNS:
        m = pattern.search(path_str)
        if m:
            key = m.group(1) if m.lastindex else None
            if key and key in KNOWN_TRACKS:
                return key
            if key:
                return key

    # Try to find known track keys in path
    for key in KNOWN_TRACKS:
        if key in path_str:
            return key

    return None


def extract_parts_key(path_str: str) -> Optional[str]:
    """Try to extract a parts key from a file path."""
    path_str = path_str.replace("\\", "/").lower()

    for pattern in PARTS_PATTERNS:
        m = pattern.search(path_str)
        if m:
            return m.group(1) if m.lastindex else None

    return None


def classify_path(path_str: str) -> str:
    """
    Classify a file path as car / track / parts / audio / config / unknown.
    Returns one of: car, track, parts, audio, texture, config, unknown
    """
    path_str = path_str.replace("\\", "/").lower()

    # Car detection
    if extract_model_key(path_str):
        return "car"

    # Track detection
    if ".frd" in path_str and extract_track_key(path_str):
        return "track"
    if "/tracks/" in path_str or "\\tracks\\" in path_str:
        if ".frd" in path_str or ".fst" in path_str:
            return "track"

    # Parts detection
    if "/parts/" in path_str or "\\parts\\" in path_str:
        return "parts"
    if "/perf/" in path_str or "\\perf\\" in path_str:
        return "parts"

    # Audio detection
    if "/audio/" in path_str or "\\audio\\" in path_str:
        return "audio"
    if "/sfx/" in path_str or "\\sfx\\" in path_str:
        return "audio"
    if ".bnk" in path_str:
        return "audio"

    # Texture detection
    if ".fsh" in path_str:
        return "texture"

    # Config/data
    if ".ini" in path_str:
        return "config"
    if ".blf" in path_str:
        return "config"
    if ".mdb" in path_str:
        return "config"
    if ".eng" in path_str:
        return "config"

    return "unknown"


def get_display_name_for_car(model_key: str) -> str:
    """Return the display name for a known car model key."""
    return KNOWN_CARS.get(model_key.lower(), model_key)


def get_display_name_for_track(track_key: str) -> str:
    """Return the display name for a known track key."""
    return KNOWN_TRACKS.get(track_key.lower(), track_key)
