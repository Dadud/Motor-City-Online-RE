/**
 * test_physics.c - Physics Simulation Test
 * 
 * Tests the PSim physics implementation.
 */

#include <stdio.h>
#include "psim.h"

int main() {
    printf("Motor City Online - Physics Simulation Test\n");
    printf("============================================\n\n");
    
    // Create a car
    Car car;
    Car_Init(&car);
    
    printf("Car initialized:\n");
    printf("  Mass: %.0f kg\n", car.mass);
    printf("  Wheelbase: %.2f m\n", car.wheelbase);
    printf("  Initial RPM: %.0f\n", car.engine.rpm);
    printf("  Initial Gear: %d\n\n", car.transmission.currentGear);
    
    // Set to 1st gear
    Car_SetGear(&car, 1);
    printf("Shifted to gear 1\n");
    
    // Simulate acceleration
    printf("\nSimulating 5 seconds of acceleration:\n");
    printf("%-5s %-8s %-8s %-8s %-8s %-8s\n", 
           "Time", "Speed", "MPH", "RPM", "Gear", "Throttle");
    printf("%-5s %-8s %-8s %-8s %-8s %-8s\n",
           "-----", "------", "------", "-----", "----", "--------");
    
    float dt = 0.016f;  // 60 FPS
    float time = 0.0f;
    
    for (int i = 0; i < 300; i++) {  // 5 seconds at 60 FPS
        Car_SetThrottle(&car, 1.0f);  // Full throttle
        Car_SetSteer(&car, 0.0f);     // Straight
        Car_Update(&car, dt);
        
        if (i % 30 == 0) {  // Print every 0.5 seconds
            printf("%5.1f %7.1f %7.1f %7.0f %6d %8.0f%%\n",
                   time,
                   Car_GetSpeed(&car),
                   Car_GetSpeedMPH(&car),
                   Car_GetRPM(&car),
                   Car_GetGear(&car),
                   car.controls.throttle * 100.0f);
        }
        
        time += dt;
        
        // Auto shift at redline
        if (car.engine.rpm >= car.engine.redline - 500.0f && Car_GetGear(&car) < 6) {
            Car_SetGear(&car, Car_GetGear(&car) + 1);
            printf("\n>>> SHIFT! <<<\n\n");
        }
    }
    
    // Test braking
    printf("\n\nTesting braking:\n");
    Car_SetGear(&car, 4);
    Car_SetThrottle(&car, 0.0f);
    
    for (int i = 0; i < 100; i++) {  // 1.6 seconds
        Car_SetBrake(&car, 0.8f);
        Car_Update(&car, dt);
        
        if (i % 20 == 0) {
            printf("Speed: %.1f mph, Braking: %.0f%%\n",
                   Car_GetSpeedMPH(&car),
                   car.controls.brake * 100.0f);
        }
    }
    
    // Test steering
    printf("\n\nTesting cornering (simulated):\n");
    Car_Reset(&car, (Vector3D){0, 0, 0}, 0.0f);
    Car_SetGear(&car, 2);
    Car_SetThrottle(&car, 0.7f);
    
    for (int i = 0; i < 200; i++) {
        // Simulate a turn
        Car_SetSteer(&car, 0.3f * sinf(i * 0.05f));
        Car_Update(&car, dt);
        
        if (i % 40 == 0) {
            printf("Speed: %.1f mph, Lateral G: %.2f, Sliding: %s\n",
                   Car_GetSpeedMPH(&car),
                   Car_GetLateralG(&car),
                   car.physics.isSliding ? "YES" : "NO");
        }
    }
    
    printf("\n\nPhysics test complete!\n");
    return 0;
}
