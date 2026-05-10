from __future__ import annotations

import json
import sys
from pathlib import Path


def scan_install(path: Path) -> dict:
    files = sorted(p for p in path.rglob("*") if p.is_file())
    interesting = [str(p.relative_to(path)) for p in files if p.suffix.lower() in {".exe", ".dll", ".viv", ".fsh", ".bnk", ".ini"}][:200]
    return {
        "root": str(path),
        "file_count": len(files),
        "interesting_files": interesting,
        "status": "ok" if files else "empty",
        "note": "Asset scanning is preservation-only; no assets are bundled here.",
    }


def main() -> None:
    if len(sys.argv) < 2:
        raise SystemExit("usage: mco-scan /path/to/mco/install")
    root = Path(sys.argv[1]).expanduser().resolve()
    result = scan_install(root)
    out_path = Path("asset_manifest.json")
    out_path.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(f"\nWrote {out_path}")
