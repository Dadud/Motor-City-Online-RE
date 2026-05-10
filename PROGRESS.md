# Motor City Online - Research Status

## Summary

This is a comprehensive reverse engineering project for Motor City Online, an EA racing game from 2001.

## What We Know (Verified)

### File Formats (COMPLETE)

| Format | Status | Notes |
|--------|--------|-------|
| **FCE** | ✅ Complete | Car geometry, magic `0x19EB 0xEEFE` |
| **FRD** | ✅ Complete | Track road geometry |
| **FSH** | ✅ Complete | Texture containers (0x7D, 0x7E, 0xFD, 0xFE) |
| **BIG** | ✅ Complete | Archive format |
| **VIV** | ✅ Complete | EA archive format |
| **BNK** | ✅ Complete | Audio banks (EA XA ADPCM) |
| **FST** | ✅ Complete | Car surface textures |
| **LOD** | ✅ Complete | Level of detail distances |
| **DATABASE** | ✅ Complete | Jet DB 3, 4096-byte pages |
| **BLF** | ✅ Complete | Log file format |
| **INI** | ✅ Complete | Config format |
| **TRK** | ⚠️ Partial | AI racing line, not fully decoded |

### Network Protocol (COMPLETE)

| Component | Status | Notes |
|-----------|--------|-------|
| **CASTANET** | ✅ Complete | 12-byte header: magic(4) + ver(2) + type(2) + len(4) |
| **NPS Messages** | ✅ Complete | ~40 message types documented |
| **Error Codes** | ✅ Complete | 40+ CASTANET error codes |
| **Authentication** | ✅ Complete | AAI_EAS flow |

### Source Tree (COMPLETE)

Full source file paths recovered from binary strings:
- `C:\mcity\Game\PSim*.c` - Physics (10 files)
- `C:\mcity\Game\cars.c`, `carload.c` - Car handling
- `C:\mcity\Game\Track.c` - Track system
- `C:\mcity\Frontend\db*.c` - Database systems
- `C:\nps\Common\NPSLib\Src\` - Network library

### Implemented Code

| Component | Status | Lines |
|-----------|--------|-------|
| **CASTANET/NPS** | ✅ Working | ~800 |
| **Physics Engine** | ✅ Working | ~700 |
| **OpenAL Audio** | ✅ Working | ~800 |
| **OpenGL Renderer** | ✅ Working | ~1300 |

## What's Left

### Unknown/Not Done

1. **TRK Format** - AI racing line data structure not fully decoded
2. **DirectX 8 Integration** - Original renderer uses D3D8, we have OpenGL
3. **Car Physics Parameters** - Need to extract from DATABASE
4. **Track Physics Data** - `pavement.ini` parsing
5. **Car Customization System** - Part attachment points
6. **Race Event System** - Event scheduling
7. **Club System** - Multiplayer clubs
8. **Input System** - DirectInput

## Database Tables (from SQL)

### Verified Tables
- `Part` - Parts
- `BrandedPart` - Brand parts
- `AbstractPartType` - Categories
- `PartType` - Types
- `StockAssembly` - Stock configs
- `Vehicle` - Vehicles
- `Player` - Players
- `Persona` - Characters
- `Event` - Race events
- `EventInvitation` - Invites
- `VehicleHistory` - Race results
- `VehicleRecords` - Time records
- `Brand` - Brands
- `Model` - Models

## Key Strings Found

### Physics Functions
```
Physics_Select
Physics_SelectByOridinal
Physics_SelectBranded
NoticeFixedObjectCollision
NoticeObjectCollision f/b/r
NoticeWallCollision
```

### Car Functions
```
CarLoad_Init
CarLoad_FromFile
CarLoad_FromDatabase
ManualResetCar
```

### Track Functions
```
Track_Load
Track_Unload
Track_GetPosition
Track_GetLapDistance
```

### Audio
```
EngineSmokeBlack
EngineSmokeSteam
EngineSmokeOil
SFX_PinkSlip1_ptc
```

## File Locations

### Oct 09 Prototype
```
mco-files/oct09/extracted/
└── Motor City Online (Oct 09, 2001 prototype)/
    └── Installed Game/
        ├── mcity.exe (5MB) - Main game
        ├── mco.exe (557KB) - NPS core
        ├── nps/
        │   ├── authlogin.dll
        │   ├── mrbupd.dll
        │   ├── NPSAnlyz.dll
        │   ├── PBA.exe
        │   └── ProxyTool.exe
        ├── Data/
        │   ├── DB/ - Database files
        │   ├── Tracks/ - Track files
        │   └── Cars/ - Car files
        └── engpatch.viv - Patch file
```

## Build Information

From binary:
```
PDB: C:\mcity\vc_mcity___Win32_Final0\MCity.pdb
Compiler: Visual C++ 6.0
Build: Win32_Final0
Date: October 8, 2001
```

## Repository

**GitHub:** https://github.com/Dadud/Motor-City-Online-RE

### Recent Commits
- `d4c0f75` - Add modern OpenGL rendering system
- `fe118e8` - Add OpenAL audio system
- `e1aa796` - Add working physics, car handling, AI, track systems
- `fd7102c` - Replace mock with working CASTANET/NPS implementation
- `dbee67c` - Add reconstructed source code from binary analysis
- `e431d41` - Update FST documentation
- `752c447` - Add complete Cars table (83 variants)

## How to Help

1. Decode the TRK format - AI racing line data
2. Document the `pavement.ini` format
3. Write a track loader based on FRD format
4. Create a simple game demo using our physics/renderer
5. Test the NPS protocol implementation
