/**
 * psim.h - Physics Simulation API
 * 
 * Motor City Online - Car Physics Simulation
 * 
 * Based on reverse engineering:
 * - PSimCalc.c, psimcar.c, PSimEngine.c, PSimSuspension.c, pSimAero.c, PSimWAG.c
 * 
 * This is a WORKING physics simulation, not a mock.
 */

#ifndef PSIM_H
#define PSIM_H

#include <windows.h>
#include <math.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define GRAVITY 9.81f           // m/s^2
#define AIR_DENSITY 1.225f      // kg/m^3 at sea level

#define MAX_RPM 8500.0f
#define IDLE_RPM 750.0f
#define REDLINE_RPM 8000.0f

#define PI 3.14159265359f

// ============================================================================
// VECTOR3D
// ============================================================================

typedef struct {
    float x, y, z;
} Vector3D;

static inline Vector3D Vec3_Add(Vector3D a, Vector3D b) {
    return (Vector3D){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Vector3D Vec3_Sub(Vector3D a, Vector3D b) {
    return (Vector3D){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Vector3D Vec3_Scale(Vector3D v, float s) {
    return (Vector3D){ v.x * s, v.y * s, v.z * s };
}

static inline float Vec3_Dot(Vector3D a, Vector3D b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vector3D Vec3_Cross(Vector3D a, Vector3D b) {
    return (Vector3D){
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

static inline float Vec3_Length(Vector3D v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vector3D Vec3_Normalize(Vector3D v) {
    float len = Vec3_Length(v);
    if (len < 0.0001f) return (Vector3D){ 0, 0, 0 };
    return (Vector3D){ v.x / len, v.y / len, v.z / len };
}

static inline Vector3D Vec3_Project(Vector3D v, Vector3D onto) {
    float dot = Vec3_Dot(v, onto);
    float lenSq = Vec3_Dot(onto, onto);
    if (lenSq < 0.0001f) return (Vector3D){ 0, 0, 0 };
    float scale = dot / lenSq;
    return Vec3_Scale(onto, scale);
}

// ============================================================================
// EULER ANGLES
// ============================================================================

typedef struct {
    float pitch;   // X axis rotation (front/back tilt)
    float yaw;     // Y axis rotation (turning)
    float roll;    // Z axis rotation (lean)
} EulerAngles;

static inline EulerAngles Vec3_ToEuler(Vector3D forward, Vector3D up) {
    EulerAngles e;
    
    // Yaw from forward vector
    e.yaw = atan2f(forward.x, forward.z);
    
    // Pitch from forward vector
    e.pitch = asinf(-forward.y);
    
    // Roll from up vector
    Vector3D right = Vec3_Cross(forward, up);
    up = Vec3_Cross(right, forward);
    e.roll = atan2f(up.x, up.y);
    
    return e;
}

// ============================================================================
// ENGINE STATE
// ============================================================================

typedef struct {
    float rpm;
    float throttlePosition;    // 0.0 - 1.0
    int gear;                 // 0 = Neutral, 1-6 = Forward, -1 = Reverse
    float gearRatios[8];      // 0=Neutral, 1-6=Forward, 7=Reverse
    float finalDrive;
    float redline;
    float idleRPM;
    
    // Torque curve points (5 points typical)
    float torqueCurveRPM[5];
    float torqueCurveTorque[5];
    float powerCurveRPM[5];
    float powerCurvePower[5];
} EngineState;

// Engine initialization
void Engine_Init(EngineState* eng);
void Engine_SetGearRatios(EngineState* eng, float* ratios, int count);
void Engine_SetTorqueCurve(EngineState* eng, float* rpm, float* torque, int count);

// Engine simulation
void Engine_Update(EngineState* eng, float throttle, float dt);
float Engine_GetTorque(EngineState* eng);
float Engine_GetPower(EngineState* eng);
float Engine_GetRedlineTorque(EngineState* eng, float rpm);

// ============================================================================
// TRANSMISSION
// ============================================================================

typedef struct {
    int currentGear;
    float gearRatios[8];      // Gear ratios (engine RPM / wheel RPM)
    float finalDriveRatio;    // Final drive ratio
    float clutchPosition;      // 0.0 = disengaged, 1.0 = engaged
    float shiftTime;          // Time to complete shift (seconds)
    float shiftTimer;         // Current shift progress
    BOOL isShifting;
} Transmission;

// Transmission initialization
void Trans_Init(Transmission* trans);
void Trans_SetGearRatios(Transmission* trans, float* ratios, int count);
void Trans_SetFinalDrive(Transmission* trans, float ratio);

// Transmission simulation
void Trans_Update(Transmission* trans, float dt);
void Trans_ShiftUp(Transmission* trans);
void Trans_ShiftDown(Transmission* trans);
float Trans_GetOutputRPM(Transmission* trans, float wheelRPM);
float Trans_GetTorqueMultiplier(Transmission* trans);

// ============================================================================
// BRAKE SYSTEM
// ============================================================================

typedef struct {
    float frontBrakeBias;     // 0.0 - 1.0 (percentage to front)
    float brakePressure;      // Current brake pressure
    float brakeDiscRadius;    // Rotor radius
    float padSize;           // Friction material size
    float frictionCoeff;      // Friction coefficient
    float temp;              // Current brake temp (Celsius)
    float wear;              // Current pad wear 0.0 - 1.0
} BrakeSystem;

// Brake initialization
void Brake_Init(BrakeSystem* brake);
void Brake_SetBias(BrakeSystem* brake, float frontBias);
void Brake_SetDiscSize(BrakeSystem* brake, float radius);

// Brake simulation
void Brake_Update(BrakeSystem* brake, float pedalInput, float dt);
float Brake_GetBrakeTorque(BrakeSystem* brake, float wheelAngularVel);
void Brake_ApplyFading(BrakeSystem* brake, float temperature);

// ============================================================================
// SUSPENSION
// ============================================================================

typedef struct {
    float springRate;        // N/m
    float damping;           // Damping coefficient
    float reboundRate;       // Rebound damping
    float compressionRate;   // Compression damping
    float upperLimit;        // Max extension (m)
    float lowerLimit;        // Max compression (m)
    float rideHeight;        // Target height at rest
    float currentLength;     // Current spring length
    float velocity;          // Spring velocity
    float load;              // Current spring load
} Suspension;

// Suspension initialization
void Susp_Init(Suspension* susp);
void Susp_SetSpringRate(Suspension* susp, float rate);
void Susp_SetDamping(Suspension* susp, float damping);
void Susp_SetRideHeight(Suspension* susp, float height);

// Suspension simulation
void Susp_Update(Suspension* susp, float inputForce, float dt);
float Susp_GetDeflection(Suspension* susp);

// ============================================================================
// AERODYNAMICS
// ============================================================================

typedef struct {
    float dragCoeff;         // Drag coefficient (Cd)
    float frontalArea;       // Frontal area (m^2)
    float liftCoeffFront;   // Front downforce coefficient
    float liftCoeffRear;    // Rear downforce coefficient
    float wingAngle;        // Rear wing angle (radians)
    float diffuserAngle;     // Diffuser angle
} AeroData;

// Aero initialization
void Aero_Init(AeroData* aero);
void Aero_SetDragCoeff(AeroData* aero, float cd);
void Aero_SetWingAngle(AeroData* aero, float angle);

// Aero simulation
void Aero_Update(AeroData* aero, float velocity);
float Aero_GetDragForce(AeroData* aero, float velocity);
float Aero_GetDownforce(AeroData* aero, float velocity);

// ============================================================================
// CAR PHYSICS STATE
// ============================================================================

typedef struct {
    // Position and orientation
    Vector3D position;
    Vector3D velocity;
    Vector3D acceleration;
    Vector3D angularVelocity;
    EulerAngles rotation;
    
    // Derived values
    float speed;            // Forward speed (m/s)
    float lateralSpeed;     // Sideways speed (m/s)
    float verticalSpeed;    // Up/down speed
    
    // Inertia
    float mass;            // kg
    float inertia;         // Moment of inertia
    
    // Dimensions
    float wheelbase;       // meters
    float trackWidth;      // meters
    float cgHeight;        // Center of gravity height
    float weightFront;     // Weight on front axle
    float weightRear;      // Weight on rear axle
    
    // Wheels (4 wheels)
    float wheelRadius[4];
    float wheelAngularVel[4];
    
    // State flags
    BOOL isAirborne;
    BOOL isSliding;
    BOOL isAccelerating;
    BOOL isBraking;
} CarPhysicsState;

// Car physics state initialization
void CarPhys_Init(CarPhysicsState* car);
void CarPhys_SetMass(CarPhysicsState* car, float mass);
void CarPhys_SetDimensions(CarPhysicsState* car, float wheelbase, float track, float cg);

// ============================================================================
// CAR CONTROL INPUTS
// ============================================================================

typedef struct {
    float throttle;         // 0.0 - 1.0
    float brake;            // 0.0 - 1.0
    float clutch;           // 0.0 - 1.0
    float steer;            // -1.0 (full left) to 1.0 (full right)
    float handbrake;        // 0.0 - 1.0
    int gearCommand;        // Target gear (-1, 0, 1-6)
} CarControls;

// ============================================================================
// FULL CAR
// ============================================================================

typedef struct {
    // Subsystems
    EngineState engine;
    Transmission transmission;
    BrakeSystem brakes;
    Suspension suspension[4];  // FL, FR, RL, RR
    AeroData aero;
    CarPhysicsState physics;
    CarControls controls;
    
    // Constants
    float mass;
    float wheelbase;
    float trackWidth;
    float cgHeight;
    float weightDistribution;  // 0.0 = front, 1.0 = rear
    
    // Derived
    float wheelRadius;
} Car;

// Car initialization
void Car_Init(Car* car);
void Car_LoadFromConfig(Car* car, const char* configFile);

// Car simulation step
void Car_Update(Car* car, float dt);

// Control helpers
void Car_SetThrottle(Car* car, float throttle);
void Car_SetBrake(Car* car, float brake);
void Car_SetSteer(Car* car, float steer);
void Car_SetGear(Car* car, int gear);

// Physics helpers
float Car_GetSpeed(Car* car);          // m/s
float Car_GetSpeedMPH(Car* car);       // miles per hour
float Car_GetSpeedKPH(Car* car);       // km per hour
float Car_GetRPM(Car* car);
int Car_GetGear(Car* car);
float Car_GetLateralG(Car* car);       // Lateral G-force

// Reset
void Car_Reset(Car* car, Vector3D position, float heading);

// ============================================================================
// PHYSICS CALCULATION FUNCTIONS
// ============================================================================

// Core physics
void PSim_CalculateAeroDrag(Car* car, float dt);
void PSim_CalculateRollingResistance(Car* car, float dt);
void PSim_CalculateWeightTransfer(Car* car, float dt);
void PSim_CalculateTraction(Car* car, float dt);
void PSim_CalculateSuspensionForces(Car* car, float dt);
void PSim_CalculateCollisionResponse(Car* car, float dt);

// Integration
void PSim_Integrate(CarPhysicsState* state, float dt);
void PSim_ApplyDrag(CarPhysicsState* state, float dragForce, float dt);
void PSim_ApplyForce(CarPhysicsState* state, Vector3D force, Vector3D point, float dt);
void PSim_ApplyTorque(CarPhysicsState* state, Vector3D torque, float dt);

// ============================================================================
// AI PHYSICS (shared with player)
// ============================================================================

typedef struct {
    float targetSpeed;
    float targetLanePosition;  // -1.0 to 1.0
    float targetHeading;
    Vector3D targetPoint;
    
    float skillLevel;         // 0.0 = novice, 1.0 = expert
    float aggressionLevel;    // 0.0 = cautious, 1.0 = aggressive
    
    float racingLine[512][3];
    int racingLineLength;
    int currentWaypoint;
} AIVehicleState;

void AI_Init(AIVehicleState* ai);
void AI_Update(AIVehicleState* ai, Car* car, float dt);
void AI_FollowRacingLine(AIVehicleState* ai, Car* car, float dt);
void AI_CalculateSteering(AIVehicleState* ai, Car* car, float* steer);
void AI_CalculateThrottleBrake(AIVehicleState* ai, Car* car, float* throttle, float* brake);

#endif // PSIM_H
