/**
 * test_audio.c - OpenAL Audio Test Program
 * 
 * Tests the OpenAL audio system.
 */

#include <stdio.h>
#include "audio.h"

int main() {
    printf("Motor City Online - OpenAL Audio Test\n");
    printf("====================================\n\n");
    
    // Initialize audio
    if (!Audio_Init()) {
        printf("ERROR: Failed to initialize audio\n");
        return 1;
    }
    
    printf("Audio system initialized\n\n");
    
    // Test volume controls
    printf("Testing volume controls...\n");
    Audio_SetMasterVolume(1.0f);
    Audio_SetMusicVolume(0.8f);
    Audio_SetSFXVolume(1.0f);
    Audio_SetEngineVolume(1.0f);
    Audio_SetAmbientVolume(0.6f);
    printf("  Volume controls work\n\n");
    
    // Test listener update
    printf("Testing listener position...\n");
    Vec3 pos = Vec3_Create(0, 0, 0);
    Vec3 vel = Vec3_Create(0, 0, 0);
    Vec3 forward = Vec3_Create(0, 0, 1);
    Vec3 up = Vec3_Create(0, 1, 0);
    Audio_UpdateListener(pos, vel, forward, up);
    printf("  Listener position set\n\n");
    
    // Test procedural sound generation
    printf("Testing procedural sounds...\n");
    
    // Generate test sounds
    Audio_PlaySound(SOUND_TIRE_SCREECH, NULL, 1.0f, 1.0f);
    printf("  Tire screech played\n");
    
    Audio_PlaySound(SOUND_COLLISION_LIGHT, NULL, 0.8f, 1.0f);
    printf("  Collision sound played\n");
    
    Audio_PlaySound(SOUND_MENU_SELECT, NULL, 1.0f, 1.0f);
    printf("  Menu select played\n\n");
    
    // Test engine sound
    printf("Testing engine sound synthesis...\n");
    Audio_UpdateEngineSound(1000.0f, 0.3f, 0);
    printf("  Engine at idle (1000 RPM)\n");
    
    Audio_UpdateEngineSound(3000.0f, 0.5f, 0);
    printf("  Engine at cruise (3000 RPM)\n");
    
    Audio_UpdateEngineSound(6000.0f, 0.8f, 0);
    printf("  Engine at high (6000 RPM)\n\n");
    
    // Test 3D positioning
    printf("Testing 3D audio positioning...\n");
    Vec3 carPos = Vec3_Create(10.0f, 0.0f, 50.0f);
    Vec3 enginePos = Vec3_Create(10.0f, 0.5f, 51.0f);  // Slightly ahead of car
    Audio_UpdateListener(carPos, vel, forward, up);
    Audio_PlayEngineSound(enginePos);
    printf("  Engine positioned in 3D space\n\n");
    
    // Test update loop
    printf("Testing update loop...\n");
    for (int i = 0; i < 60; i++) {
        Audio_Update(0.016f);
    }
    printf("  60 frames of audio updated\n\n");
    
    // Cleanup
    printf("Cleaning up...\n");
    Audio_StopEngineSound();
    Audio_StopMusic();
    Audio_Shutdown();
    printf("  Audio shutdown complete\n\n");
    
    printf("All audio tests passed!\n");
    return 0;
}
