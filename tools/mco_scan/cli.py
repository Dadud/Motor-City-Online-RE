"""
MCO Install Scanner — CLI
Detects file types by magic bytes, generates asset manifest.
Clean-room implementation.

Usage:
    python -m tools.mco_scan.cli /path/to/mco/install [--manifest out.json] [--max-files N]
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from tools.mco_scan.scanner import scan_install


def print_summary(manifest) -> None:
    """Print a human-readable scan summary."""
    s = manifest.summary
    print("\n=== MCO Install Scan Summary ===")
    print(f"Root:   {manifest.scan_root}")
    print(f"Scanned: {manifest.total_files_scanned} files, "
          f"{manifest.total_bytes / 1024 / 1024:.1f} MB")
    print(f"  Cars:      {s.get('cars', 0)}")
    print(f"  Tracks:    {s.get('tracks', 0)}")
    print(f"  Parts:     {s.get('parts', 0)}")
    print(f"  Audio:     {s.get('audio', 0)}")
    print(f"  Textures:  {s.get('textures', 0)}")
    print(f"  Configs:   {s.get('configs', 0)}")
    print(f"  Unknown:   {s.get('unknown', 0)}")

    known_cars = [a for a in manifest.assets if a["type"] == "car" and a.get("known")]
    if known_cars:
        print(f"\n  Known cars ({len(known_cars)}):")
        for a in known_cars:
            print(f"    {a['model_key']} — {a['display_name']}")

    known_tracks = [a for a in manifest.assets if a["type"] == "track" and a.get("known")]
    if known_tracks:
        print(f"\n  Known tracks ({len(known_tracks)}):")
        for a in known_tracks:
            print(f"    {a['track_key']} — {a['display_name']}")

    unknown_archives = [
        a for a in manifest.assets
        if a["type"] == "unknown"
        and a["file_type_confidence"] in ("high", "medium")
        and a["size_bytes"] > 1024
    ]
    if unknown_archives:
        print(f"\n  Unclassified archives ({len(unknown_archives)}):")
        for a in unknown_archives[:10]:
            print(f"    {a['path']} [{a['file_type']}, {a['size_bytes']} bytes]")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Scan MCO install, detect file types, generate asset manifest."
    )
    parser.add_argument(
        "install_path",
        help="Path to MCO install directory",
    )
    parser.add_argument(
        "--manifest",
        "-m",
        help="Output path for asset manifest JSON (default: asset_manifest.json)",
        default="asset_manifest.json",
    )
    parser.add_argument(
        "--max-files",
        type=int,
        default=50000,
        help="Maximum files to scan (default: 50000)",
    )
    parser.add_argument(
        "--json-only",
        action="store_true",
        help="Only print JSON, no summary",
    )

    args = parser.parse_args()

    install_path = Path(args.install_path).expanduser().resolve()
    if not install_path.exists():
        raise SystemExit(f"Error: path does not exist: {install_path}")
    if not install_path.is_dir():
        raise SystemExit(f"Error: not a directory: {install_path}")

    print(f"Scanning: {install_path}", file=sys.stderr)
    manifest = scan_install(install_path, max_files=args.max_files)

    # Write manifest
    out_path = Path(args.manifest)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(manifest.to_dict(), f, indent=2)

    if not args.json_only:
        print_summary(manifest)
        print(f"\nManifest written to: {out_path}")

        # Offer to import to DB
        if args.import_to_db:
            import_manifest_to_db(out_path, args.import_to_db)
    else:
        print(json.dumps(manifest.to_dict(), indent=2))


def import_manifest_to_db(manifest_path: Path, db_path: str) -> None:
    """
    Import discovered assets into the local shard database.
    Adds cars/tracks that aren't already in the DB.
    """
    # GUESSED: This function hooks into the shard service to add discovered assets
    try:
        from tools.mco_scan.db_import import import_manifest_to_shard_db
        import_manifest_to_shard_db(manifest_path, db_path)
        print(f"Assets imported to: {db_path}")
    except ImportError as exc:
        print(f"Note: DB import not available ({exc}). "
              "Assets are in the manifest only.", file=sys.stderr)


if __name__ == "__main__":
    main()
