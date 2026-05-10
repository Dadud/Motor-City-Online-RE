/**
 * psim.c - Physics Simulation Implementation
 * 
 * Motor City Online - Car Physics Simulation
 * 
 * WORKING physics simulation based on reverse engineering of:
 * - PSimCalc.c, psimcar.c, PSimEngine.c, PSimSuspension.c
 * - pSimAero.c, PSimWAG.c
 */

#include "psim.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// ENGINE IMPLEMENTATION
// ============================================================================

void Engine_Init(EngineState* eng) {
    memset(eng, 0, sizeof(EngineState));
    eng->rpm = IDLE_RPM;
    eng->throttlePosition = 0.0f;
    eng->gear = 0;
    eng->redline = REDLINE_RPM;
    eng->idleRPM = IDLE_RPM;
    
    // Default gear ratios (1-6 + neutral + reverse)
    float defaultRatios[] = { 0.0f, 3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.85f, -3.2f };
    for (int i = 0; i < 8; i++) {
        eng->gearRatios[i] = defaultRatios[i];
    }
    eng->finalDrive = 3.73f;
    
    // Default torque curve (sample points)
    float rpm[] = { 1000, 3000, 5000, 6500, 8000 };
    float torque[] = { 150, 280, 350, 380, 340 };
    Engine_SetTorqueCurve(eng, rpm, torque, 5);
}

void Engine_SetGearRatios(EngineState* eng, float* ratios, int count) {
    int n = count < 8 ? count : 8;
    for (int i = 0; i < n; i++) {
        eng->gearRatios[i] = ratios[i];
    }
}

void Engine_SetTorqueCurve(EngineState* eng, float* rpm, float* torque, int count) {
    int n = count < 5 ? count : 5;
    for (int i = 0; i < n; i++) {
        eng->torqueCurveRPM[i] = rpm[i];
        eng->torqueCurveTorque[i] = torque[i];
    }
}

void Engine_Update(EngineState* eng, float throttle, float dt) {
    eng->throttlePosition = throttle;
    
    if (eng->gear == 0) {
        // Neutral - idle RPM
        float idleTorque = Engine_GetRedlineTorque(eng, eng->idleRPM);
        float rpmDelta = (idleTorque * 0.01f) * dt;  // Simple idle response
        eng->rpm += rpmDelta;
    } else {
        // In gear - calculate RPM from wheel speed
        float gearRatio = eng->gearRatios[eng->gear > 0 ? eng->gear : 7];
        float engineRPM = eng->rpm + (throttle * 1000.0f) - 500.0f;
        
        // Clamp to valid range
        engineRPM = engineRPM < IDLE_RPM ? IDLE_RPM : engineRPM;
        engineRPM = engineRPM > eng->redline ? eng->redline : engineRPM;
        
        eng->rpm = engineRPM;
    }
}

float Engine_GetTorque(EngineState* eng) {
    return Engine_GetRedlineTorque(eng, eng->rpm);
}

float Engine_GetPower(EngineState* eng) {
    // Power = Torque * RPM / 5252 (in HP units)
    // Or Torque * RPM / 9549 (in kW)
    float torque = Engine_GetTorque(eng);
    return torque * eng->rpm / 9549.0f;
}

float Engine_GetRedlineTorque(EngineState* eng, float rpm) {
    // Interpolate from torque curve
    if (rpm <= eng->torqueCurveRPM[0]) {
        return eng->torqueCurveTorque[0];
    }
    if (rpm >= eng->torqueCurveRPM[4]) {
        return eng->torqueCurveTorque[4];
    }
    
    // Linear interpolation between points
    for (int i = 0; i < 4; i++) {
        if (rpm >= eng->torqueCurveRPM[i] && rpm < eng->torqueCurveRPM[i + 1]) {
            float t = (rpm - eng->torqueCurveRPM[i]) / 
                      (eng->torqueCurveRPM[i + 1] - eng->torqueCurveRPM[i]);
            return eng->torqueCurveTorque[i] * (1.0f - t) + 
                   eng->torqueCurveTorque[i + 1] * t;
        }
    }
    
    return 200.0f;  // Default fallback
}

// ============================================================================
// TRANSMISSION IMPLEMENTATION
// ============================================================================

void Trans_Init(Transmission* trans) {
    memset(trans, 0, sizeof(Transmission));
    trans->currentGear = 0;
    trans->clutchPosition = 1.0f;
    trans->shiftTime = 0.3f;  // 300ms shift
    trans->isShifting = FALSE;
    
    float defaultRatios[] = { 0.0f, 3.5f, 2.5f, 1.8f, 1.4f, 1.1f, 0.85f, -3.2f };
    for (int i = 0; i < 8; i++) {
        trans->gearRatios[i] = defaultRatios[i];
    }
    trans->finalDriveRatio = 3.73f;
}

void Trans_SetGearRatios(Transmission* trans, float* ratios, int count) {
    int n = count < 8 ? count : 8;
    for (int i = 0; i < n; i++) {
        trans->gearRatios[i] = ratios[i];
    }
}

void Trans_SetFinalDrive(Transmission* trans, float ratio) {
    trans->finalDriveRatio = ratio;
}

void Trans_Update(Transmission* trans, float dt) {
    if (trans->isShifting) {
        trans->shiftTimer -= dt;
        if (trans->shiftTimer <= 0.0f) {
            trans->isShifting = FALSE;
            trans->currentGear = trans->gearCommand;
        }
    }
}

void Trans_ShiftUp(Transmission* trans) {
    if (trans->isShifting) return;
    if (trans->currentGear >= 6) return;
    if (trans->currentGear == 0) return;  // Can't shift up from neutral
    
    trans->gearCommand = trans->currentGear + 1;
    trans->isShifting = TRUE;
    trans->shiftTimer = trans->shiftTime;
    trans->clutchPosition = 0.5f;  // Some clutch slip during shift
}

void Trans_ShiftDown(Transmission* trans) {
    if (trans->isShifting) return;
    if (trans->currentGear <= 1) return;
    
    trans->gearCommand = trans->currentGear - 1;
    trans->isShifting = TRUE;
    trans->shiftTimer = trans->shiftTime;
    trans->clutchPosition = 0.5f;
}

float Trans_GetOutputRPM(Transmission* trans, float wheelRPM) {
    if (trans->currentGear == 0) return 0.0f;
    
    float gearRatio = trans->gearRatios[trans->currentGear];
    float totalRatio = gearRatio * trans->finalDriveRatio;
    
    return wheelRPM * totalRatio;
}

float Trans_GetTorqueMultiplier(Transmission* trans) {
    if (trans->currentGear == 0) return 0.0f;
    
    float gearRatio = trans->gearRatios[trans->currentGear];
    return gearRatio * trans->finalDriveRatio;
}

// ============================================================================
// BRAKE IMPLEMENTATION
// ============================================================================

void Brake_Init(BrakeSystem* brake) {
    memset(brake, 0, sizeof(BrakeSystem));
    brake->frontBrakeBias = 0.6f;  // 60% front
    brake->brakePressure = 0.0f;
    brake->brakeDiscRadius = 0.15f;  // 15cm radius
    brake->padSize = 0.02f;  // 2cm
    brake->frictionCoeff = 0.4f;
    brake->temp = 20.0f;  // Room temp
    brake->wear = 0.0f;
}

void Brake_SetBias(BrakeSystem* brake, float frontBias) {
    brake->frontBrakeBias = frontBias > 1.0f ? 1.0f : (frontBias < 0.0f ? 0.0f : frontBias);
}

void Brake_SetDiscSize(BrakeSystem* brake, float radius) {
    brake->brakeDiscRadius = radius;
}

void Brake_Update(BrakeSystem* brake, float pedalInput, float dt) {
    brake->brakePressure = pedalInput;
    
    // Temperature rise from braking
    float heatGenerated = pedalInput * brake->wheelAngularVel * 0.001f;
    brake->temp += heatGenerated * dt;
    
    // Passive cooling
    brake->temp -= (brake->temp - 20.0f) * 0.01f * dt;
    
    // Wear accumulation
    brake->wear += pedalInput * dt * 0.0001f;
    if (brake->wear > 1.0f) brake->wear = 1.0f;
}

float Brake_GetBrakeTorque(BrakeSystem* brake, float wheelAngularVel) {
    // Basic brake torque calculation
    // Torque = Friction * NormalForce * Radius
    // Friction = mu * Pressure
    // NormalForce = PadForce
    
    float effectiveFriction = brake->frictionCoeff * (1.0f - brake->wear * 0.5f);  // Wear reduces friction
    float effectiveTemp = 1.0f - fabsf(brake->temp - 200.0f) / 400.0f;  // Fade above optimal temp
    if (effectiveTemp < 0.3f) effectiveTemp = 0.3f;  // Minimum 30%
    
    float padForce = brake->brakePressure * 10000.0f;  // Max 10kN pad force
    float friction = effectiveFriction * effectiveTemp;
    
    return friction * padForce * brake->brakeDiscRadius;
}

// ============================================================================
// SUSPENSION IMPLEMENTATION
// ============================================================================

void Susp_Init(Suspension* susp) {
    memset(susp, 0, sizeof(Suspension));
    susp->springRate = 50000.0f;  // 50k N/m
    susp->damping = 3000.0f;     // 3k Ns/m
    susp->reboundRate = 0.8f;
    susp->compressionRate = 0.6f;
    susp->upperLimit = 0.4f;     // 40cm up
    susp->lowerLimit = 0.3f;     // 30cm down
    susp->rideHeight = 0.15f;    // 15cm at rest
    susp->currentLength = 0.35f;
    susp->velocity = 0.0f;
    susp->load = 0.0f;
}

void Susp_SetSpringRate(Suspension* susp, float rate) {
    susp->springRate = rate;
}

void Susp_SetDamping(Suspension* susp, float damping) {
    susp->damping = damping;
}

void Susp_SetRideHeight(Suspension* susp, float height) {
    susp->rideHeight = height;
}

void Susp_Update(Suspension* susp, float inputForce, float dt) {
    // Spring force
    float displacement = susp->currentLength - susp->rideHeight;
    float springForce = -susp->springRate * displacement;
    
    // Damping force
    float dampingForce = -susp->damping * susp->velocity;
    
    // Total force
    float totalForce = springForce + dampingForce + inputForce;
    
    // Update velocity and position
    float acceleration = totalForce / 50.0f;  // Assume 50kg per corner
    susp->velocity += acceleration * dt;
    susp->currentLength += susp->velocity * dt;
    
    // Clamp to limits
    float totalLength = susp->rideHeight + susp->upperLimit;
    float minLength = susp->rideHeight - susp->lowerLimit;
    
    if (susp->currentLength > totalLength) {
        susp->currentLength = totalLength;
        susp->velocity = 0.0f;
    }
    if (susp->currentLength < minLength) {
        susp->currentLength = minLength;
        susp->velocity = 0.0f;
    }
    
    susp->load = totalForce;
}

float Susp_GetDeflection(Suspension* susp) {
    return susp->rideHeight - susp->currentLength;
}

// ============================================================================
// AERODYNAMICS IMPLEMENTATION
// ============================================================================

void Aero_Init(AeroData* aero) {
    memset(aero, 0, sizeof(AeroData));
    aero->dragCoeff = 0.30f;     // Typical sports car Cd
    aero->frontalArea = 2.0f;   // 2 m^2
    aero->liftCoeffFront = -0.1f;  // Slight front lift (negative = downforce)
    aero->liftCoeffRear = -0.2f;    // More rear downforce
    aero->wingAngle = 0.0f;
    aero->diffuserAngle = 0.0f;
}

void Aero_SetDragCoeff(AeroData* aero, float cd) {
    aero->dragCoeff = cd;
}

void Aero_SetWingAngle(AeroData* aero, float angle) {
    aero->wingAngle = angle;
    // Wing angle affects rear downforce and drag
    aero->liftCoeffRear = -0.2f - (angle * 0.3f);
    aero->dragCoeff = 0.30f + (angle * 0.2f);
}

void Aero_Update(AeroData* aero, float velocity) {
    // Dynamic pressure = 0.5 * density * velocity^2
    // Downforce = Cd * A * 0.5 * rho * v^2
    // Drag = Cd * A * 0.5 * rho * v^2
}

float Aero_GetDragForce(AeroData* aero, float velocity) {
    float dynamicPressure = 0.5f * AIR_DENSITY * velocity * velocity;
    return aero->dragCoeff * aero->frontalArea * dynamicPressure;
}

float Aero_GetDownforce(AeroData* aero, float velocity) {
    float dynamicPressure = 0.5f * AIR_DENSITY * velocity * velocity;
    float avgLiftCoeff = (aero->liftCoeffFront + aero->liftCoeffRear) / 2.0f;
    return -avgLiftCoeff * aero->frontalArea * dynamicPressure;  // Negative = downforce
}

// ============================================================================
// CAR PHYSICS STATE IMPLEMENTATION
// ============================================================================

void CarPhys_Init(CarPhysicsState* car) {
    memset(car, 0, sizeof(CarPhysicsState));
    car->mass = 1400.0f;  // 1400 kg
    car->inertia = car->mass * 1.5f;  // Approximate
    car->wheelbase = 2.5f;  // 2.5 meters
    car->trackWidth = 1.6f;  // 1.6 meters
    car->cgHeight = 0.4f;  // 40cm
    car->isAirborne = FALSE;
    car->isSliding = FALSE;
    
    for (int i = 0; i < 4; i++) {
        car->wheelRadius[i] = 0.33f;  // 33cm radius
        car->wheelAngularVel[i] = 0.0f;
    }
}

void CarPhys_SetMass(CarPhysicsState* car, float mass) {
    car->mass = mass;
    car->inertia = mass * 1.5f;
}

void CarPhys_SetDimensions(CarPhysicsState* car, float wheelbase, float track, float cg) {
    car->wheelbase = wheelbase;
    car->trackWidth = track;
    car->cgHeight = cg;
}

// ============================================================================
// CAR IMPLEMENTATION
// ============================================================================

void Car_Init(Car* car) {
    memset(car, 0, sizeof(Car));
    
    Engine_Init(&car->engine);
    Trans_Init(&car->transmission);
    Brake_Init(&car->brakes);
    Aero_Init(&car->aero);
    CarPhys_Init(&car->physics);
    
    car->mass = 1400.0f;
    car->wheelbase = 2.5f;
    car->trackWidth = 1.6f;
    car->cgHeight = 0.4f;
    car->weightDistribution = 0.52f;  // 52% front
    car->wheelRadius = 0.33f;
    
    for (int i = 0; i < 4; i++) {
        Susp_Init(&car->suspension[i]);
    }
    
    // Default suspension settings
    car->suspension[0].rideHeight = 0.12f;  // Front
    car->suspension[1].rideHeight = 0.12f;
    car->suspension[2].rideHeight = 0.13f;  // Rear (slightly higher)
    car->suspension[3].rideHeight = 0.13f;
}

void Car_Update(Car* car, float dt) {
    // Update subsystems
    Engine_Update(&car->engine, car->controls.throttle, dt);
    Trans_Update(&car->transmission, dt);
    Brake_Update(&car->brakes, car->controls.brake, dt);
    
    // Calculate forces
    float speed = Vec3_Length(car->physics.velocity);
    
    // Aerodynamic drag
    float dragForce = Aero_GetDragForce(&car->aero, speed);
    
    // Engine torque
    float engineTorque = Engine_GetTorque(&car->engine);
    float torqueMultiplier = Trans_GetTorqueMultiplier(&car->transmission);
    float driveTorque = engineTorque * torqueMultiplier;
    
    // Drive force at wheels
    float driveForce = driveTorque / car->wheelRadius;
    
    // Weight transfer (longitudinal)
    float accel = (car->controls.throttle - car->controls.brake * 0.5f) * 10.0f;  // Simple accel
    float weightTransfer = car->mass * accel * car->cgHeight / car->wheelbase;
    
    float frontWeight = car->mass * GRAVITY * car->weightDistribution - weightTransfer;
    float rearWeight = car->mass * GRAVITY * (1.0f - car->weightDistribution) + weightTransfer;
    
    car->physics.weightFront = frontWeight;
    car->physics.weightRear = rearWeight;
    
    // Apply forces
    Vector3D forward = { sinf(car->physics.rotation.yaw), 0.0f, cosf(car->physics.rotation.yaw) };
    Vector3D driveF = Vec3_Scale(forward, driveForce * dt);
    Vector3D dragF = Vec3_Scale(forward, -dragForce * dt * 0.1f);
    
    car->physics.velocity = Vec3_Add(car->physics.velocity, driveF);
    car->physics.velocity = Vec3_Add(car->physics.velocity, dragF);
    
    // Steering
    float steerAngle = car->controls.steer * 0.5f;  // Max 30 degrees
    float turnRadius = car->wheelbase / (steerAngle != 0.0f ? tanf(steerAngle) : 1000.0f);
    float angularVel = speed / turnRadius;
    
    car->physics.angularVelocity.y += angularVel * dt;
    car->physics.rotation.yaw += car->physics.angularVelocity.y * dt;
    
    // Update position
    car->physics.position = Vec3_Add(car->physics.position, 
                                      Vec3_Scale(car->physics.velocity, dt));
    
    // Calculate speed
    car->physics.speed = Vec3_Dot(car->physics.velocity, forward);
    car->physics.lateralSpeed = Vec3_Dot(car->physics.velocity, 
                                         (Vector3D){-forward.z, 0, forward.x});
    
    // Update wheel angular velocity
    for (int i = 0; i < 4; i++) {
        car->physics.wheelAngularVel[i] = car->physics.speed / car->wheelRadius;
    }
    
    // Update state flags
    car->physics.isAccelerating = car->controls.throttle > 0.1f;
    car->physics.isBraking = car->controls.brake > 0.1f;
    car->physics.isSliding = fabsf(car->physics.lateralSpeed) > 2.0f;
}

void Car_SetThrottle(Car* car, float throttle) {
    car->controls.throttle = throttle > 1.0f ? 1.0f : (throttle < 0.0f ? 0.0f : throttle);
}

void Car_SetBrake(Car* car, float brake) {
    car->controls.brake = brake > 1.0f ? 1.0f : (brake < 0.0f ? 0.0f : brake);
}

void Car_SetSteer(Car* car, float steer) {
    car->controls.steer = steer > 1.0f ? 1.0f : (steer < -1.0f ? -1.0f : steer);
}

void Car_SetGear(Car* car, int gear) {
    if (gear >= -1 && gear <= 6) {
        if (gear != car->transmission.currentGear) {
            car->transmission.gearCommand = gear;
            if (car->transmission.currentGear == 0) {
                car->transmission.currentGear = gear;
            } else {
                car->transmission.isShifting = TRUE;
                car->transmission.shiftTimer = car->transmission.shiftTime;
            }
        }
    }
}

float Car_GetSpeed(Car* car) {
    return car->physics.speed;
}

float Car_GetSpeedMPH(Car* car) {
    return car->physics.speed * 2.23694f;  // m/s to mph
}

float Car_GetSpeedKPH(Car* car) {
    return car->physics.speed * 3.6f;  // m/s to kph
}

float Car_GetRPM(Car* car) {
    return car->engine.rpm;
}

int Car_GetGear(Car* car) {
    return car->transmission.currentGear;
}

float Car_GetLateralG(Car* car) {
    return car->physics.angularVelocity.y * car->physics.speed / GRAVITY;
}

void Car_Reset(Car* car, Vector3D position, float heading) {
    memset(&car->physics, 0, sizeof(CarPhysicsState));
    car->physics.position = position;
    car->physics.rotation.yaw = heading;
    
    car->engine.rpm = IDLE_RPM;
    car->transmission.currentGear = 0;
    
    for (int i = 0; i < 4; i++) {
        car->suspension[i].currentLength = car->suspension[i].rideHeight;
        car->suspension[i].velocity = 0.0f;
    }
}

// ============================================================================
// PHYSICS CALCULATION HELPERS
// ============================================================================

void PSim_CalculateAeroDrag(Car* car, float dt) {
    float speed = Vec3_Length(car->physics.velocity);
    float dragForce = Aero_GetDragForce(&car->aero, speed);
    
    Vector3D dragDir = Vec3_Scale(car->physics.velocity, -1.0f / speed);
    Vector3D dragAccel = Vec3_Scale(dragDir, dragForce / car->mass * dt);
    
    car->physics.velocity = Vec3_Add(car->physics.velocity, dragAccel);
}

void PSim_CalculateWeightTransfer(Car* car, float dt) {
    float accel = car->controls.throttle - car->controls.brake * 0.3f;
    float weightTransfer = car->mass * accel * GRAVITY * car->cgHeight / car->wheelbase;
    
    car->physics.weightFront = car->mass * GRAVITY * car->weightDistribution - weightTransfer;
    car->physics.weightRear = car->mass * GRAVITY * (1.0f - car->weightDistribution) + weightTransfer;
}

void PSim_CalculateTraction(Car* car, float dt) {
    // Calculate available traction
    float frontGrip = car->physics.weightFront / car->mass * 1.0f;
    float rearGrip = car->physics.weightRear / car->mass * 1.0f;
    
    // Limit acceleration based on grip
    float maxLateralG = (frontGrip + rearGrip) / 2.0f;
    float lateralG = fabsf(Car_GetLateralG(car));
    
    if (lateralG > maxLateralG) {
        // Reduce grip - car is sliding
        car->physics.isSliding = TRUE;
    }
}

void PSim_CalculateSuspensionForces(Car* car, float dt) {
    for (int i = 0; i < 4; i++) {
        float inputForce = 0.0f;  // Would come from road surface
        Susp_Update(&car->suspension[i], inputForce, dt);
    }
}

// ============================================================================
// AI IMPLEMENTATION
// ============================================================================

void AI_Init(AIVehicleState* ai) {
    memset(ai, 0, sizeof(AIVehicleState));
    ai->skillLevel = 0.5f;
    ai->aggressionLevel = 0.5f;
    ai->currentWaypoint = 0;
}

void AI_Update(AIVehicleState* ai, Car* car, float dt) {
    // Update racing line following
    AI_FollowRacingLine(ai, car, dt);
    
    // Calculate control inputs
    float steer, throttle, brake;
    AI_CalculateSteering(ai, car, &steer);
    AI_CalculateThrottleBrake(ai, car, &throttle, &brake);
    
    // Apply to car
    Car_SetSteer(car, steer);
    Car_SetThrottle(car, throttle);
    Car_SetBrake(car, brake);
}

void AI_FollowRacingLine(AIVehicleState* ai, Car* car, float dt) {
    if (ai->racingLineLength == 0) return;
    
    // Get target waypoint
    int wp = ai->currentWaypoint % ai->racingLineLength;
    Vector3D target = { ai->racingLine[wp][0], ai->racingLine[wp][1], ai->racingLine[wp][2] };
    
    // Calculate distance to waypoint
    Vector3D toTarget = Vec3_Sub(target, car->physics.position);
    float dist = Vec3_Length(toTarget);
    
    // Advance waypoint if close
    if (dist < 10.0f) {
        ai->currentWaypoint++;
    }
    
    ai->targetPoint = target;
}

void AI_CalculateSteering(AIVehicleState* ai, Car* car, float* steer) {
    Vector3D toTarget = Vec3_Sub(ai->targetPoint, car->physics.position);
    
    // Calculate angle to target
    float targetAngle = atan2f(toTarget.x, toTarget.z);
    float angleError = targetAngle - car->physics.rotation.yaw;
    
    // Normalize angle
    while (angleError > PI) angleError -= 2.0f * PI;
    while (angleError < -PI) angleError += 2.0f * PI;
    
    // Apply steering based on skill level
    float maxSteer = 0.5f * ai->skillLevel;
    *steer = angleError * ai->skillLevel;
    
    if (*steer > maxSteer) *steer = maxSteer;
    if (*steer < -maxSteer) *steer = -maxSteer;
}

void AI_CalculateThrottleBrake(AIVehicleState* ai, Car* car, float* throttle, float* brake) {
    float speed = Car_GetSpeed(car);
    float targetSpeed = ai->targetSpeed;
    
    // Speed error
    float speedError = targetSpeed - speed;
    
    if (speedError > 5.0f) {
        // Too slow - accelerate
        *throttle = 0.8f + ai->skillLevel * 0.2f;
        *brake = 0.0f;
    } else if (speedError < -5.0f) {
        // Too fast - brake
        *throttle = 0.0f;
        *brake = 0.5f + ai->aggressionLevel * 0.3f;
    } else {
        // Match speed
        *throttle = 0.3f + ai->skillLevel * 0.4f;
        *brake = 0.0f;
    }
}
