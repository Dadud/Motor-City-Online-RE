"""
Main MCO install scanner.
Scans a directory tree, detects file types by magic bytes,
and builds an asset manifest.
Clean-room implementation.
"""

from __future__ import annotations

import json
import os
from dataclasses import dataclass, asdict
from datetime import datetime, timezone
from pathlib import Path

from tools.mco_scan.magic import (
    detect_by_magic,
    MCOFileType,
    is_archive_type,
    is_geometry_type,
)
from tools.mco_scan.detectors import (
    classify_path,
    extract_model_key,
    extract_track_key,
    extract_parts_key,
    get_display_name_for_car,
    get_display_name_for_track,
    KNOWN_CARS,
    KNOWN_TRACKS,
)


@dataclass
class ScannedAsset:
    type: str            # car, track, parts, audio, texture, config, unknown
    path: str            # relative to scan root
    absolute_path: str
    file_type: str        # viv_big4, fce4, etc.
    file_type_confidence: str
    size_bytes: int
    model_key: str | None = None
    track_key: str | None = None
    part_key: str | None = None
    display_name: str | None = None
    magic_hex: str | None = None
    known: bool = False
    note: str | None = None

    def to_dict(self) -> dict:
        d = asdict(self)
        return d


@dataclass
class ScanManifest:
    version: str
    scan_root: str
    scanned_at: str
    total_files_scanned: int
    total_bytes: int
    summary: dict
    assets: list[dict]

    def to_dict(self) -> dict:
        return asdict(self)


class MCOInstallScanner:
    """
    Scans an MCO install directory and generates an asset manifest.
    """

    # File extensions to always skip (system files)
    SKIP_EXTENSIONS = {
        ".tmp", ".bak", ".cache", ".pyc", ".pyo",
        ".log", ".ds_store", ".thumbs.db",
    }

    # Directories to always skip
    SKIP_DIRS = {
        "__pycache__", ".git", ".svn", "node_modules",
        "windows", "system32",  # guard against scanning OS directories
    }

    def __init__(self, root_path: str | Path):
        self.root = Path(root_path).resolve()
        self.assets: list[ScannedAsset] = []
        self.stats = {
            "total_files": 0,
            "total_bytes": 0,
            "car": 0,
            "track": 0,
            "parts": 0,
            "audio": 0,
            "texture": 0,
            "config": 0,
            "unknown": 0,
        }

    def _should_skip(self, path: Path) -> bool:
        """Return True if this file/directory should be skipped."""
        # Skip system directories
        parts = path.parts
        for skip in self.SKIP_DIRS:
            if skip in parts:
                return True

        # Skip temp files
        if path.suffix.lower() in self.SKIP_EXTENSIONS:
            return True

        return False

    def scan(self, max_files: int = 50000) -> ScanManifest:
        """
        Scan the install directory.
        Stops after max_files to avoid runaway scans.
        """
        self.assets.clear()
        self.stats = {k: 0 for k in self.stats}
        total_files = 0
        total_bytes = 0

        for path in self.root.rglob("*"):
            if max_files and total_files >= max_files:
                break

            if self._should_skip(path):
                continue

            if not path.is_file():
                continue

            total_files += 1
            rel_path = str(path.relative_to(self.root))
            file_size = path.stat().st_size
            total_bytes += file_size

            try:
                asset = self._scan_file(path, rel_path, file_size)
            except Exception as exc:
                asset = ScannedAsset(
                    type="unknown",
                    path=rel_path,
                    absolute_path=str(path),
                    file_type="unknown",
                    file_type_confidence="none",
                    size_bytes=file_size,
                    known=False,
                    note=f"scan error: {exc}",
                )

            self.assets.append(asset)
            self.stats[asset.type] = self.stats.get(asset.type, 0) + 1

        summary = {
            "cars": self.stats.get("car", 0),
            "tracks": self.stats.get("track", 0),
            "parts": self.stats.get("parts", 0),
            "audio": self.stats.get("audio", 0),
            "textures": self.stats.get("texture", 0),
            "configs": self.stats.get("config", 0),
            "unknown": self.stats.get("unknown", 0),
            "total_files": total_files,
        }

        return ScanManifest(
            version="1.0",
            scan_root=str(self.root),
            scanned_at=datetime.now(timezone.utc).isoformat(),
            total_files_scanned=total_files,
            total_bytes=total_bytes,
            summary=summary,
            assets=[a.to_dict() for a in self.assets],
        )

    def _scan_file(self, path: Path, rel_path: str, file_size: int) -> ScannedAsset:
        """Scan a single file and return a ScannedAsset."""
        sig = detect_by_magic(str(path))

        asset_type = classify_path(rel_path)
        model_key = extract_model_key(rel_path)
        track_key = extract_track_key(rel_path)
        part_key = extract_parts_key(rel_path)

        display_name = None
        known = False

        if asset_type == "car" and model_key:
            display_name = get_display_name_for_car(model_key)
            known = model_key in KNOWN_CARS

        if asset_type == "track" and track_key:
            display_name = get_display_name_for_track(track_key)
            known = track_key in KNOWN_TRACKS

        # Build magic hex for diagnostics
        magic_hex = None
        try:
            with open(path, "rb") as f:
                magic_hex = f.read(8).hex().upper()
        except (OSError, IOError):
            pass

        note = None
        if sig.note:
            note = sig.note

        return ScannedAsset(
            type=asset_type,
            path=rel_path,
            absolute_path=str(path),
            file_type=sig.file_type.value,
            file_type_confidence=sig.confidence,
            size_bytes=file_size,
            model_key=model_key,
            track_key=track_key,
            part_key=part_key,
            display_name=display_name,
            magic_hex=magic_hex,
            known=known,
            note=note,
        )


def scan_install(root_path: str | Path, max_files: int = 50000) -> ScanManifest:
    """Convenience function to scan an MCO install."""
    scanner = MCOInstallScanner(root_path)
    return scanner.scan(max_files=max_files)


def load_manifest(path: str | Path) -> ScanManifest:
    """Load a saved manifest from disk."""
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    return ScanManifest(**data)
