"""
MCO file format magic byte signatures.
Clean-room implementation based on format research.
"""

from dataclasses import dataclass
from enum import Enum
from typing import Optional


class MCOFileType(Enum):
    UNKNOWN = "unknown"
    VIV_BIG4 = "viv_big4"
    VIV_BIGF = "viv_bigf"
    VIV_BIGH = "viv_bigh"
    FCE3 = "fce3"
    FCE4 = "fce4"
    FCE4M = "fce4m"
    FSH = "fsh"
    BNK = "bnk"
    FRD = "frd"
    FST = "fst"
    INI = "ini"
    BLF = "blf"
    MDB = "mdb"
    ENG = "eng"
    CARP = "carp"
    DAT = "dat"


# Magic bytes: (offset, bytes, file_type)
MAGIC_SIGNATURES = [
    (0, b"BIG4", MCOFileType.VIV_BIG4),
    (0, b"BIGF", MCOFileType.VIV_BIGF),
    (0, b"BIGH", MCOFileType.VIV_BIGH),
    (0, b"FCE3", MCOFileType.FCE3),
    (0, b"FCE4", MCOFileType.FCE4),
    (0, b"SHPI", MCOFileType.FSH),
    (0, b"FCE4M", MCOFileType.FCE4M),
]

# FST detection: GUID at known offset in FST files
# e0134678-c995-d111-960a-0010-5ae42069
FST_GUID = bytes([
    0xe0, 0x13, 0x46, 0x78, 0xc9, 0x95, 0xd1, 0x11,
    0x96, 0x0a, 0x00, 0x10, 0x5a, 0xe4, 0x20, 0x69
])

# FRD detection: DEADBEEF appears as first uint32 in block header
# We detect FRD by checking if file contains the DEADBEEF marker
FRD_MAGIC = b"\xEF\xBE\xAD\xDE"  # little-endian

# BNK detection: EA XA audio bank — check for RIFF-like header or known EA patterns
# Most BNK files start with EA XA header (look for "BNK" string or specific bank ID)
BNK_SIGNATURE_candidates = [
    b"BNK",       # Some BNK formats start with BNK header
    b"RIFF",      # Some use RIFF container
]


@dataclass
class FileSignature:
    file_type: MCOFileType
    confidence: str  # "high", "medium", "low"
    note: Optional[str] = None


def read_magic(path: str, offset: int = 0, size: int = 16) -> bytes:
    """Read bytes from a file at offset."""
    with open(path, "rb") as f:
        f.seek(offset)
        return f.read(size)


def detect_by_magic(path: str) -> FileSignature:
    """
    Detect file type by reading magic bytes.
    Returns a FileSignature with type and confidence.
    """
    try:
        with open(path, "rb") as f:
            header = f.read(16)
    except (OSError, IOError):
        return FileSignature(MCOFileType.UNKNOWN, "none", "cannot read file")

    if len(header) == 0:
        return FileSignature(MCOFileType.UNKNOWN, "none", "empty file")

    # Check fixed-offset signatures
    for offset, magic, ftype in MAGIC_SIGNATURES:
        if header[offset:offset + len(magic)] == magic:
            return FileSignature(ftype, "high", None)

    # Check for FST GUID — appears at offset 0x54 in FST files
    # GUESSED: based on FST research, the GUID appears in the header section
    if len(header) >= 16:
        # Check FST GUID at offset 0x54 (84 decimal)
        try:
            with open(path, "rb") as f:
                f.seek(0x54)
                guid_check = f.read(16)
                if guid_check == FST_GUID:
                    return FileSignature(MCOFileType.FST, "high", "FST GUID match at 0x54")
        except (OSError, IOError):
            pass

    # Check for FRD DEADBEEF block marker — scan first 256 bytes
    # GUESSED: DEADBEEF appears as block separator in FRD files
    try:
        with open(path, "rb") as f:
            chunk = f.read(256)
            if FRD_MAGIC in chunk:
                return FileSignature(MCOFileType.FRD, "high", "DEADBEEF block marker")
    except (OSError, IOError):
        pass

    # Check for BNK (EA XA audio bank)
    # GUESSED: BNK files have "BNK" or RIFF headers
    if header.startswith(b"BNK") or header.startswith(b"RIFF"):
        return FileSignature(MCOFileType.BNK, "medium", "BNK/RIFF header")

    # Check for INI (text file starting with '[' or ';')
    if header and header[0] in (ord("["), ord(";")):
        try:
            with open(path, "rb") as f:
                chunk = f.read(128).decode("ascii", errors="ignore")
            if "[" in chunk or ";" in chunk:
                return FileSignature(MCOFileType.INI, "medium", "INI section or comment detected")
        except (OSError, IOError):
            pass

    # Check extension as fallback
    ext = path.lower().split(".")[-1]
    ext_map = {
        "viv": MCOFileType.VIV_BIG4,
        "fce": MCOFileType.FCE4,
        "fsh": MCOFileType.FSH,
        "bnk": MCOFileType.BNK,
        "frd": MCOFileType.FRD,
        "fst": MCOFileType.FST,
        "ini": MCOFileType.INI,
        "blf": MCOFileType.BLF,
        "mdb": MCOFileType.MDB,
        "eng": MCOFileType.ENG,
        "dat": MCOFileType.DAT,
    }
    if ext in ext_map:
        return FileSignature(ext_map[ext], "low", f"inferred from .{ext} extension")

    return FileSignature(MCOFileType.UNKNOWN, "none", "no matching signature")


def is_archive_type(ft: MCOFileType) -> bool:
    """Return True if this is a container/archive format that may contain nested files."""
    return ft in (
        MCOFileType.VIV_BIG4,
        MCOFileType.VIV_BIGF,
        MCOFileType.VIV_BIGH,
        MCOFileType.FCE3,
        MCOFileType.FCE4,
        MCOFileType.FCE4M,
    )


def is_geometry_type(ft: MCOFileType) -> bool:
    """Return True if this is a geometry/mesh format."""
    return ft in (
        MCOFileType.FCE3,
        MCOFileType.FCE4,
        MCOFileType.FCE4M,
        MCOFileType.FST,
        MCOFileType.FRD,
    )


def is_audio_type(ft: MCOFileType) -> bool:
    """Return True if this is an audio format."""
    return ft in (MCOFileType.BNK,)


def is_config_type(ft: MCOFileType) -> bool:
    """Return True if this is a config/data format."""
    return ft in (
        MCOFileType.INI,
        MCOFileType.BLF,
        MCOFileType.MDB,
        MCOFileType.ENG,
        MCOFileType.CARP,
        MCOFileType.DAT,
    )
