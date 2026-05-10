# Motor City Online - Game Systems Source Reconstruction

Source paths from binary strings:
```
C:\mcity\Game\PSimAero.c
C:\mcity\Game\PSimAI.c
C:\mcity\Game\psimbrke.c
C:\mcity\Game\PSimCalc.c
C:\mcity\Game\psimcar.c
C:\mcity\Game\PSimControl.c
C:\mcity\Game\PSimDamage.c
C:\mcity\Game\PSimEngine.c
C:\mcity\Game\PSimSuspension.c
C:\mcity\Game\PSimWAG.c
C:\mcity\Game\aivehicl.c
C:\mcity\Game\cars.c
C:\mcity\Game\carload.c
C:\mcity\Game\Collide.c
C:\mcity\Game\force.c
C:\mcity\Game\MathNfs.c
C:\mcity\Game\trkload.c
C:\mcity\Game\Track.c
C:\mcity\Game\TrackFx.c
```

---

## Physics Simulation System (PSim)

### Core Files

| File | Description |
|------|-------------|
| `PSimCalc.c` | Core physics calculations |
| `psimcar.c` | Car physics model |
| `PSimControl.c` | Player input handling |
| `PSimEngine.c` | Engine simulation |
| `psimbrke.c` | Brake simulation |
| `PSimSuspension.c` | Suspension system |
| `pSimAero.c` | Aerodynamics |
| `PSimWAG.c` | Weight and grip |
| `PSimDamage.c` | Damage modeling |
| `PSimAI.c` | AI physics (shared with player) |

### Physics Data Structures

```c
// From strings: engine.rpm, engine.htb, engine.btb, engine.ctb, engine.ltb
// htb = high torque band, btb = boost torque band, etc.

typedef struct {
    float rpm;           // Current RPM
    float redline;       // Rev limit
    float idle;          // Idle RPM
    float power;         // Horsepower at current RPM
    float torque;        // Torque at current RPM
    float htb;          // High torque band start
    float ltb;          // Low torque band start
    float ctb;          // Constant torque band
    float btb;          // Boost torque band
} EngineState;

typedef struct {
    float throttle;      // 0.0 - 1.0
    float brake;         // 0.0 - 1.0
    float clutch;        // 0.0 - 1.0
    float steer;         // -1.0 to 1.0
    int gear;           // Current gear (0=N, 1-6=Gears)
} ControlInputs;

typedef struct {
    float speed;         // Forward velocity
    float lateralSpeed;  // Sideways velocity
    float angularVel;    // Yaw rate
    
    float posX, posY, posZ;  // World position
    float rotX, rotY, rotZ;   // Rotation (Euler angles)
    
    // Velocity vectors
    float velX, velY, velZ;
    float angVelX, angVelY, angVelZ;
    
    // Forces acting on car
    float driveForce;    // Traction force
    float brakeForce;    // Braking force
    float dragForce;     // Air resistance
    float lateralForce;  // Sideways grip
} CarPhysicsState;

typedef struct {
    float spring;        // Spring rate
    float damping;       // Damping coefficient
    float compression;    // Current compression
    float rebound;       // Rebound rate
    float travel;        // Suspension travel
    float rideHeight;    // Target height
} SuspensionData;

typedef struct {
    float downforce;     // Aerodynamic downforce
    float drag;         // Drag coefficient
    float lift;         // Lift coefficient (negative = downforce)
    float wingAngle;    // Rear wing angle
} AeroData;
```

### Key Physics Functions

```c
// From debug strings
void NoticeFixedObjectCollision(void* car, void* object, float* impact);
void NoticeWallCollision(void* car, float* normal, float* impact);
void NoticeObjectCollision_f(void* car, void* object, float* point, float* normal);
void NoticeObjectCollision_b(void* car, void* object, float* point, float* normal);
void NoticeObjectCollision_r(void* car, void* object, float* point, float* normal);

// Physics update loop
void PSim_Calculate(CarPhysicsState* car, float dt);
void PSim_IntegrateForces(CarPhysicsState* car, float dt);
void PSim_ApplySuspension(CarPhysicsState* car, SuspensionData* susp[4]);
void PSim_ApplyAero(CarPhysicsState* car, AeroData* aero, float speed);

// Engine simulation
void PSimEngine_Update(EngineState* eng, ControlInputs* ctrl, float dt);
float PSimEngine_GetPower(EngineState* eng, float rpm);
float PSimEngine_GetTorque(EngineState* eng, float rpm);
void PSimEngine_ShiftGear(EngineState* eng, int newGear);

// Brake simulation  
void PSimBrake_Apply(float* brakeForce, float brake, float load, float friction);
void PSimBrake_CalculateFriction(float* friction, float temp, float wear);

// Suspension simulation
void PSimSuspension_Update(SuspensionData* susp[4], CarPhysicsState* car, float* roadHeight);
float PSimSuspension_GetLoad(SuspensionData* susp, float compression);

// Weight and grip (WAG)
void PSimWAG_Calculate(CarPhysicsState* car, float* weightTransfer, float* grip);
void PSimWAG_UpdateWeightTransfer(CarPhysicsState* car, float accel, float lateral);
```

### Physics Constants (from strings)

```
MaxRPM:%5d CurRPM:%5d Bendrng:%4d Baseadd:%4d PitMul: %5d
```

- `MaxRPM` - Maximum engine RPM
- `CurRPM` - Current RPM
- `Bendrng` - Bend rating
- `Baseadd` - Base additive value
- `PitMul` - Pit multiplier (for pit lane speed limits)

---

## Car Handling System

### Core Files

| File | Description |
|------|-------------|
| `cars.c` | Main car system |
| `carload.c` | Car loading/initialization |
| `r3dcar.c` | 3D car rendering |

### Car Data Structures

```c
// From strings: engine.rpm, engine.htb, etc.
typedef struct {
    // Engine
    EngineState engine;
    
    // Drivetrain
    int driveType;      // 0=FWD, 1=RWD, 2=AWD
    float gearRatios[7];  // 0=Neutral, 1-6=Gears, 7=Reverse
    float finalDrive;
    float clutchSize;
    
    // Handling
    float weightDistribution;  // Front/rear balance
    float cgHeight;           // Center of gravity height
    float wheelbase;         // Wheelbase length
    float trackWidth;        // Track width
    
    // Tires
    char tireType[32];       // From string: "Tire type not found"
    float tireDiameter;      // From string: "Tire diam. not found"
    float gripCoefficient;   // Friction coefficient
    
    // Aero
    AeroData aero;
    
    // Current state
    CarPhysicsState physics;
    ControlInputs inputs;
} CarData;

// Car part types (from database)
typedef enum {
    PART_ENGINE = 0,
    PART_TRANSMISSION,
    PART_DIFFERENTIAL,
    PART_CLUTCH,
    PART_BRAKES,
    PART_SUSPENSION,
    PART_TIRES,
    PART_AERO,
    PART_STYLING
} PartType;

// Engine families (from strings)
typedef enum {
    ENGINE_FAMILY_STOCK = 0,
    ENGINE_FAMILY_SPORT,
    ENGINE_FAMILY_PERFORMANCE,
    ENGINE_FAMILY_RACING
} EngineFamily;
```

### Car Functions

```c
// From strings
void CarLoad_Init(CarData* car);
void CarLoad_FromFile(CarData* car, const char* filename);
void CarLoad_FromDatabase(CarData* car, int carId);

void Cars_Update(CarData* car, float dt);
void Cars_Render(CarData* car);

void ManualResetCar(CarData* car);  // Reset to starting position

// Accessors
CarData* GetCar(int slot);
void SetCar(int slot, CarData* car);

// Car status
typedef enum {
    CAR_OK = 0,
    CAR_NO_PARTS,
    CAR_DAMAGED,
    CAR_MISSING_PARTS,
    CAR_BAD_SPECS
} CarStatus;

int Car_IsValid(CarData* car);  // "Car has no parts", "Car is too damaged"
int Car_IsSimmable(CarData* car);  // "Car is Simmable"
```

### Car Attributes (from HUD/display strings)

```
carlightMaxR, carlightMaxG, carlightMaxB  // Car paint color max
carlightMinR, carlightMinG, carlightMinB  // Car paint color min
engineVolume, engineSounds                 // Audio settings
AirCleaner, Carburetor                    // Part names
EngineBlock                              // Engine block type
```

---

## AI System

### Core Files

| File | Description |
|------|-------------|
| `aivehicl.c` | AI vehicle logic |
| `aidebug.c` | AI debugging |
| `PSimAI.c` | Shared AI physics |

### AI Data Structures

```c
// From source path: CC:\mcity\Game\aivehicl.c

typedef enum {
    AI_STATE_LANE_FOLLOW = 0,
    AI_STATE_PASSING,
    AI_STATE_DEFENSIVE,
    AI_STATE_AGGRESSIVE,
    AI_STATE_PIT_STOP,
    AI_STATE_CRASH_RECOVER
} AIState;

typedef struct {
    AIState state;
    
    // Path following
    float targetSpeed;
    float targetLane;
    float targetPoint[3];    // World position
    
    // Racing line
    float racingLine[3];     // Optimal path
    float brakingPoint[3];
    float turnInPoint[3];
    
    // opponent awareness
    float opponentPositions[8][3];  // Up to 8 opponents
    float opponentSpeeds[8];
    
    // Skill levels (from strings)
    float skillLevel;        // 0.0 - 1.0
    float aggressionLevel;    // 0.0 - 1.0
    float reactionTime;       // Seconds
    
    // State machine
    float stateTimer;
    float decisionTimer;
} AIVehicleData;

// AI debug info (from CC:\mcity\Game\aidebug.c)
typedef struct {
    float pathError;         // Distance from optimal line
    float speedError;        // Difference from target speed
    float steerError;        // Steering correction needed
    int currentDecision;     // Current AI decision
} AIDebugInfo;
```

### AI Functions

```c
// From strings: "ALWAYSFACEPLAYERCAR", "FACETRIGGERCARDURINGUNLOAD"
void AI_Update(AIVehicleData* ai, CarData* car, float dt);
void AI_Init(AIVehicleData* ai, int skillLevel);

void AI_FollowPath(AIVehicleData* ai, CarData* car, float* path, int pathLength);
void AI_PerformOvertake(AIVehicleData* ai, CarData* car);
void AI_DefendPosition(AIVehicleData* ai, CarData* car);

void AI_ReactToCollision(AIVehicleData* ai, CarData* car, float* impactNormal);
void AI_AvoidObstacle(AIVehicleData* ai, CarData* car);

float AI_CalculateBrakingPoint(float speed, float cornerAngle);
float AI_CalculateTurnIn(float speed, float turnRadius);

// Debug functions
void AI_DebugDraw(AIDebugInfo* info);
void AI_DrawPath(AIVehicleData* ai);
```

### AI Constants (from strings)

```c
#define AIS_NOLOOP  // AI flag - no looping behavior
```

---

## Track System

### Core Files

| File | Description |
|------|-------------|
| `Track.c` | Main track system |
| `trkload.c` | Track loading |
| `TrackFx.c` | Track effects |

### Track Data Structures

```c
// From strings: "Bad track data", "trackann%d", "carlap: %d Gamelaps: %d"
typedef struct {
    char name[64];           // Track name
    char country[32];        // Country/location
    float length;            // Track length in meters
    int numLaps;             // Laps for complete race
    int numCheckpoints;      // Checkpoint count
    int numSectors;         // Sector count
    
    // Geometry
    float startPosition[3];  // Grid position
    float startAngle;        // Starting heading
    float finishLine[3];     // Finish line position
    
    // Track segments (from FST/FCE analysis)
    TrackSegment* segments;
    int numSegments;
    
    // AI racing line
    float racingLine[512][3];  // Waypoints
    
    // Track features
    int hasPitLane;
    int hasDRSZone;
    float trackWidth;
} TrackInfo;

typedef struct {
    int id;                  // Segment ID
    float length;            // Segment length
    float startPos[3];       // Start position
    float endPos[3];         // End position
    float startHeading;      // Direction
    float endHeading;
    float curvature;         // 1/radius (0 = straight)
    int type;               // STRAIGHT, CORNER, CHICANE, etc.
    int isLapStart;          // Is this the start of a lap?
    int isFinishLine;        // Is this the finish line?
} TrackSegment;

// Track effects (from TrackFx.c)
typedef struct {
    float trackTemp;         // Asphalt temperature
    float trackGrip;        // Grip level
    float windSpeed;        // Wind speed
    float windDirection;    // Wind heading
    int weatherCondition;   // DRY, WET, RAIN
} TrackConditions;
```

### Track Functions

```c
// From strings: "Bad track data", "TrackAnnOK:%d", "trackann%d"
void Track_Load(TrackInfo* track, const char* filename);
void Track_Unload(TrackInfo* track);

void Track_Update(TrackInfo* track, float dt);
void Track_Render(TrackInfo* track);

void Track_GetPosition(TrackInfo* track, float dist, float* pos, float* heading);
void Track_GetSegment(TrackInfo* track, float dist, TrackSegment** segment);

float Track_GetLapDistance(TrackInfo* track, CarData* car);
void Track_GetCheckpointTimes(TrackInfo* track, int* times);

// Track effects
void TrackFx_Update(TrackEffects* fx, TrackConditions* conditions);
void TrackFx_RenderRain(TrackInfo* track);
void TrackFx_RenderSpray(CarData* car);

// From strings
int TrackAnnounce(TrackInfo* track, int lap);  // "trackann%d"
int TrackAnnounceOK(int announcementId);       // "TrackAnnOK:%d"
```

### Track-Related Strings

```
great britain              // Track location
spanish-nicaragua         // Track location
english-jamaica           // Track location
english-caribbean         // Track location
track glows               // Visual effect
trackDetail               // LOD setting
```

---

## Race System

### Race Types (from strings)

```c
typedef enum {
    RACES_ARC_TIMETRIAL = 0,    // Arcade time trial
    RACES_SIM_TIMETRIAL,        // Simulation time trial
    RACES_SIM_DRAG,            // Drag racing
    RACES_SIM_PRO,             // Pro racing
    RACES_SIM_STREET           // Street race
} RaceMode;

typedef enum {
    RACERANDOM = 0             // Random race selection
} RaceSelection;
```

### Race Data

```c
// From strings: "Races Qualified:", "Cars Lost:", "Races Run:"
typedef struct {
    int racesQualified;
    int carsLost;
    int racesRun;
    int racesWon;
    float bestTime;           // For time trial
    int pinkSlipsWon;         // Pink slip races
    int pinkSlipsLost;
} RaceStats;

// Race results
typedef struct {
    int position;             // 1-8
    int carId;
    char driverName[32];
    float lapTimes[10];       // Up to 10 laps
    float totalTime;
    int lapsComplete;
} RaceResult;
```

### Race Functions

```c
void Race_Start(int mode, TrackInfo* track, CarData** cars, int numCars);
void Race_Update(float dt);
void Race_End(void);

void Race_GetResults(RaceResult* results, int* count);
int Race_GetPosition(CarData* car);

void Race_DoLap(CarData* car, int lapNumber);
int Race_CheckLapComplete(CarData* car);

// Pit stop
void Race_PitEntry(CarData* car);
void Race_PitExit(CarData* car);
float Race_GetPitSpeedLimit(TrackInfo* track);

// Pink slip races (from strings)
typedef enum {
    kTxtPinkSlipOverLimit = 0,
    kTxtPinkSlipWarn,
    kTxtPinkSlips
} PinkSlipText;
```

---

## Collision System

### Core Files

| File | Description |
|------|-------------|
| `Collide.c` | Collision detection |

### Collision Functions

```c
// From strings:
// NoticeFixedObjectCollision
// NoticeObjectCollision f/b/r  (front/back/right)
// NoticeWallCollision
void Collision_CheckCarWithCar(CarData* car1, CarData* car2, float* impact);
void Collision_CheckCarWithTrack(CarData* car, TrackInfo* track);
void Collision_CheckCarWithObject(CarData* car, void* object);

void Collision_ResolveOverlap(CarData* car1, CarData* car2, float* normal);
void Collision_ApplyImpact(CarData* car, float* impactForce, float* point);

float Collision_CalculateDamage(float impactSpeed, float impactAngle);
void Collision_ApplyDamage(CarData* car, float damageAmount);
```

---

## Audio System

### Car Audio (from strings)

```c
// From strings: engine.rpm, engineSounds, engineVolume
typedef struct {
    float rpm;                // For engine sound pitch
    float throttle;           // For engine sound volume
    float speed;              // For wind/tire noise
    
    // Sound effect names (from strings)
    // EngineSmokeBlack, EngineSmokeSteam, EngineSmokeOil
    // SFX_PinkSlip1_ptc, SFX_PinkSlip3_ptc, SFX_PinkSlip5_ptc
} CarAudio;
```

---

## Constants Summary

### Physics Constants
```
MaxRPM        - Maximum engine RPM
CurRPM        - Current RPM  
Bendrng       - Bend rating
Baseadd       - Base additive
PitMul        - Pit lane multiplier
```

### Race Constants
```
RACES_ARC_TIMETRIAL
RACES_SIM_TIMETRIAL
RACES_SIM_DRAG
RACES_SIM_PRO
RACES_SIM_STREET
RACERANDOM
```

### UI Strings
```
Cars Lost:     - Cars destroyed in race
Races Run:     - Total races
Races Qualified: - Races qualified for
Pink Slip Races Won: - Pink slip wins
```

---

## File Dependencies

```
cars.c
  ├── carload.c          // Car loading
  ├── PSimEngine.c      // Engine physics
  ├── PSimBrake.c       // Brake physics
  ├── PSimSuspension.c  // Suspension physics
  ├── pSimAero.c        // Aerodynamics
  ├── PSimWAG.c         // Weight/grip
  └── Collide.c         // Collision

aivehicl.c
  ├── PSimAI.c          // AI physics (shared)
  ├── MathNfs.c         // Math utilities
  └── Track.c           // Track info

trkload.c
  └── Track.c           // Track system
```
