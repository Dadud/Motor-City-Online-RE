# Motor City Online - Source Tree Analysis

Complete source path list recovered from mcacity.exe binary:

## Source Structure

```
C:\mcity\
├── Game\                          # Core game engine
├── Frontend\                      # UI/menus
├── Fei\                           # Frontend interface
├── Dbf\                           # Database framework
├── Server\                        # Server code (in executable)
└── NPS\                          # Network services
```

## Game Engine (`C:\mcity\Game\`)

### Physics Simulation
| File | Purpose |
|------|---------|
| `PSimCalc.c` | Core physics calculations |
| `psimcar.c` | Car physics model |
| `PSimEngine.c` | Engine simulation |
| `psimbrke.c` | Brake simulation |
| `PSimSuspension.c` | Suspension system |
| `pSimAero.c` | Aerodynamics |
| `PSimWAG.c` | Weight and grip |
| `PSimControl.c` | Player input |
| `PSimDamage.c` | Damage modeling |
| `PSimAI.c` | AI physics |
| `PSimDbg.c` | Physics debugging |
| `pSimPart.c` | Part physics |

### Car Systems
| File | Purpose |
|------|---------|
| `cars.c` | Main car system |
| `carload.c` | Car loading |
| `r3dcar.c` | 3D car rendering |

### Track Systems
| File | Purpose |
|------|---------|
| `Track.c` | Main track system |
| `trkload.c` | Track file loading |
| `TrackFx.c` | Track visual effects |

### Collision & Physics
| File | Purpose |
|------|---------|
| `Collide.c` | Collision detection |
| `force.c` | Force calculations |

### Rendering & Graphics
| File | Purpose |
|------|---------|
| `animation.cpp` | Animation system |
| `Texture.c` | Texture management |
| `lightfx.c` | Lighting effects |
| `surface.c` | Surface rendering |
| `geometry.c` | Geometry utilities |
| `drawacc.c` | Accelerate drawing |
| `fog.c` | Fog effects |

### Audio
| File | Purpose |
|------|---------|
| `audio.c` | Main audio |
| `audioeng.c` | Engine audio |
| `Audioamb.c` | Ambient audio |
| `audioclc.c` | Audio calculations |
| `audiocmn.c` | Audio common |
| `Audiocop.c` | Audio cops |
| `TrgSfx.c` | Track SFX |
| `spkblt.c` | Speaker layout |

### Networking
| File | Purpose |
|------|---------|
| `Packet.c` | Packet handling |
| `NetDial.c` | Network dialup |

### World & Environment
| File | Purpose |
|------|---------|
| `BWorld.c` | Base world |
| `bworldsm.c` | World smoke |
| `Weather.c` | Weather system |
| `particles.c` | Particle effects |
| `SkidMark.c` | Skid marks |
| `spray.c` | Tire spray |
| `boom.c` | Explosions |

### UI Elements
| File | Purpose |
|------|---------|
| `huddraw.c` | HUD drawing |
| `hud98.c` | HUD98 system |
| `hudmap.c` | HUD map |
| `hudclip.c` | HUD clipping |
| `texfont.c` | Text fonts |

### Math Utilities
| File | Purpose |
|------|---------|
| `MathNfs.c` | NFS math |
| `quatern.c` | Quaternions |
| `Fastrand.c` | Fast random |
| `quad.c` | Quadratic |

### Other
| File | Purpose |
|------|---------|
| `loading.c` | Loading screens |
| `file.c` | File I/O |
| `device.c` | Device management |
| `screen.c` | Screen management |
| `control.c` | Control system |
| `input.c` | Input handling |
| `Sched.c` | Scheduling |
| `platfor.c` | Platform detection |
| `synctest.c` | Sync testing |

## Frontend (`C:\mcity\Frontend\`)

### Database
| File | Purpose |
|------|---------|
| `dbparts.c` | Parts database |
| `dbperson.c` | Persona database |
| `dbtrkrec.c` | Track records |
| `DBVehRecord.c` | Vehicle records |
| `DBUsedLot.c` | Used car lot |
| `dbAwards.c` | Awards system |
| `dbBadges.c` | Badges system |
| `DBClubs.cpp` | Clubs |
| `dbjunkyd.c` | Junkyard |
| `dbdorace.c` | Do race |
| `dbclass.c` | Classification |
| `dbtrnsfr.c` | Transfers |
| `dbtest.c` | Database tests |
| `DBAudio.c` | Audio DB |
| `DBApt.c` | APT database |
| `DBAsyncEvent.C` | Async events |
| `DBSchedEvents.cpp` | Scheduled events |
| `DBMem.cpp` | Memory DB |
| `DBQuery.c` | Query system |
| `DBPhysics.c` | Physics DB |
| `dbcode.c` | Code DB |
| `DBGazette.c` | Gazette |

### Networking (Frontend)
| File | Purpose |
|------|---------|
| `INet.cpp` | Internet |
| `INet_AsyncOps.cpp` | Async ops |
| `INet_Contacts.cpp` | Contacts |
| `INet_Mail.cpp` | Mail |
| `INet_Persona.cpp` | Persona |

### UI/Frontend
| File | Purpose |
|------|---------|
| `Chat.c` | Chat |
| `ChatCmd.cpp` | Chat commands |
| `cNPS_DirectChat.cpp` | Direct chat |
| `Browser.c` | Browser |
| `Menucom.c` | Menu commands |
| `madcow.c` | Madcow UI |
| `Amf.cpp` | AMF |
| `labels.c` | Labels |
| `startup.c` | Startup |
| `eaInput.c` | EA Input |
| `Mouse.c` | Mouse |
| `KeyMap.c` | Key mapping |
| `gfxMode.c` | Graphics mode |
| `stattool.c` | Statistics tool |
| `texview.c` | Texture viewer |
| `FindMap.cpp` | Find map |
| `Fetools.c` | FE tools |
| `GetSpecs.c` | Get specs |
| `flatfile.c` | Flat file |
| `IPC.c` | IPC |
| `DecodeStack.c` | Decode stack |
| `Textsys.c` | Text system |
| `OleVariant.cpp` | OLE variant |
| `tstamp.c` | Timestamp |
| `Tunes.c` | Tunes |

### Other Frontend
| File | Purpose |
|------|---------|
| `dynamictex.c` | Dynamic textures |
| `dyno2000.c` | Dyno 2000 |
| `FePartcl.c` | FE Particles |
| `NFSabortmsg.c` | Abort message |

## Database Framework (`C:\mcity\Dbf\`)

| File | Purpose |
|------|---------|
| `DBF_Table.cpp` | Table management |
| `DBF_Record.cpp` | Record management |
| `DBF_Index.cpp` | Indexing |
| `DBF_Filter.cpp` | Filtering |

## Frontend Interface (`C:\mcity\Fei\`)

| File | Purpose |
|------|---------|
| `FEI.cpp` | Main FEI |

## Network Platform Services (`C:\nps\Common\NPSLib\Src\`)

| File | Purpose |
|------|---------|
| `NPSMsg.cpp` | NPS Messages |
| `NPS_Serialize.cpp` | Serialization |
| `NPSLog.cpp` | Logging |
| `MsgPack.cpp` | Message packing |

## Server Code (`C:\MCity\Server\CommLib\`)

| File | Purpose |
|------|---------|
| `TCPMgr.cpp` | TCP Manager |
| `SockMgr.cpp` | Socket Manager |
| `KList.h` | Keep-alive list |

## Key Functions (Verified from Strings)

### Physics
- `Physics_Select()`
- `Physics_SelectByOridinal()`
- `Physics_SelectBranded()`
- `NoticeFixedObjectCollision()`
- `NoticeObjectCollision_f/b/r`
- `NoticeWallCollision()`

### Car
- `CarLoad_Init()`
- `CarLoad_FromFile()`
- `CarLoad_FromDatabase()`
- `ManualResetCar()`

### Track
- `Track_Load()`
- `Track_Unload()`
- `Track_GetPosition()`
- `Track_GetSegment()`
- `Track_GetLapDistance()`

### Audio
- `Audio_Init()`
- `EngineSmokeBlack/Steam/Oil`

## Database Tables (from SQL queries)

### Core Tables
- `Part` - Parts inventory
- `BrandedPart` - Brand-specific parts
- `AbstractPartType` - Part categories
- `PartType` - Part types
- `StockAssembly` - Stock configurations
- `Vehicle` - Player vehicles
- `Player` - Player info
- `Persona` - Game personas

### Race Tables
- `Event` - Race events
- `EventInvitation` - Event invites
- `VehicleHistory` - Race history
- `VehicleRecords` - Time records

### Shop Tables
- `Brand` - Car brands
- `Model` - Car models
- `UsedLot` - Used car lot

## Implemented Components

### ✅ Complete
- **NPS/Castanet Protocol** - Working TCP implementation
- **Physics Engine** - Working PBR-inspired physics
- **OpenAL Audio** - 3D positional audio
- **OpenGL Renderer** - Modern PBR rendering
- **File Formats** - FCE, FRD, BIG, VIV, BNK, FSH, FST, LOD, DATABASE

### ⚠️ Partial
- **TRK Format** - Header structure known, track data partially decoded
- **Car Format** - Basic structure known, physics data in files

### ❌ Not Done
- DirectX 8 renderer integration
- Input system (DirectInput)
- Track file loading code
- Full car customization system
- Race event system
- Multiplayer networking (beyond NPS protocol)
