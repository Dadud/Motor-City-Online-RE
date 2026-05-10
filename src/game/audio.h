/**
 * audio.h - OpenAL Audio System
 * 
 * Motor City Online - Audio Implementation using OpenAL
 * 
 * Replaces the original DirectSound/Windows mixer based audio with OpenAL.
 * Supports 3D positional audio for immersive racing experience.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define AUDIO_MAX_SOURCES    32
#define AUDIO_MAX_BUFFERS    64
#define AUDIO_MAX_STREAMS     8

#define PI 3.14159265359f

// Audio listener (player) position
#define LISTENER_FORWARD_X  0.0f
#define LISTENER_FORWARD_Z  1.0f
#define LISTENER_UP_X       0.0f
#define LISTENER_UP_Y       1.0f
#define LISTENER_UP_Z       0.0f

// ============================================================================
// VECTOR3D (for 3D audio positioning)
// ============================================================================

typedef struct {
    float x, y, z;
} Vec3;

static inline Vec3 Vec3_Create(float x, float y, float z) {
    Vec3 v = {x, y, z};
    return v;
}

static inline Vec3 Vec3_Add(Vec3 a, Vec3 b) {
    Vec3 v = {a.x + b.x, a.y + b.y, a.z + b.z};
    return v;
}

static inline Vec3 Vec3_Sub(Vec3 a, Vec3 b) {
    Vec3 v = {a.x - b.x, a.y - b.y, a.z - b.z};
    return v;
}

static inline Vec3 Vec3_Scale(Vec3 v, float s) {
    Vec3 result = {v.x * s, v.y * s, v.z * s};
    return result;
}

static inline float Vec3_Dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float Vec3_Length(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline Vec3 Vec3_Normalize(Vec3 v) {
    float len = Vec3_Length(v);
    if (len < 0.0001f) return Vec3_Create(0, 0, 0);
    return Vec3_Create(v.x / len, v.y / len, v.z / len);
}

// ============================================================================
// SOUND EFFECT TYPES
// ============================================================================

typedef enum {
    // Engine sounds
    SOUND_ENGINE_IDLE = 0,
    SOUND_ENGINE_REV,
    SOUND_ENGINE_HIGH,
    SOUND_ENGINE_SHIFT,
    
    // Tire sounds
    SOUND_TIRE_SCREECH,
    SOUND_TIRE_RUMBLE,
    
    // Collision sounds
    SOUND_COLLISION_LIGHT,
    SOUND_COLLISION_MEDIUM,
    SOUND_COLLISION_HEAVY,
    
    // Environmental
    SOUND_WIND,
    SOUND_RAIN,
    SOUND_CROWD,
    
    // UI sounds
    SOUND_MENU_SELECT,
    SOUND_MENU_CONFIRM,
    SOUND_BUTTON_CLICK,
    
    // Car specific
    SOUND_HORN,
    SOUND_PIT_LAP,
    SOUND_CHECKER_FLAG,
    
    // Count
    SOUND_COUNT
} SoundEffect;

// ============================================================================
// AUDIO SOURCE HANDLE
// ============================================================================

typedef struct audio_source_s* AudioSource;

struct audio_source_s {
    ALuint source;
    ALuint buffer;
    Vec3 position;
    Vec3 velocity;
    float pitch;
    float gain;
    BOOL looping;
    BOOL playing;
    char name[64];
};

// ============================================================================
// AUDIO CONTEXT
// ============================================================================

typedef struct {
    // OpenAL handles
    ALCdevice* device;
    ALCcontext* context;
    
    // Source pool
    AudioSource sources[AUDIO_MAX_SOURCES];
    int sourceCount;
    
    // Buffer pool
    ALuint buffers[AUDIO_MAX_BUFFERS];
    int bufferCount;
    
    // Listener
    Vec3 listenerPos;
    Vec3 listenerVel;
    Vec3 listenerForward;
    Vec3 listenerUp;
    
    // Master volumes
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    float engineVolume;
    float ambientVolume;
    
    // Engine sound synthesis
    ALuint engineSource;
    ALuint engineBuffers[4];  // Multiple buffers for engine
    float engineRPM;
    float engineThrottle;
    
    // State
    BOOL initialized;
    BOOL muted;
    
} AudioContext;

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * Initialize OpenAL audio system
 * 
 * @return TRUE on success, FALSE on failure
 */
BOOL Audio_Init(void);

/**
 * Shutdown OpenAL audio system
 */
void Audio_Shutdown(void);

/**
 * Update audio listener position (player car position)
 * 
 * @param position Listener world position
 * @param velocity Listener velocity (for Doppler)
 * @param forward Listener forward direction
 * @param up Listener up direction
 */
void Audio_UpdateListener(Vec3 position, Vec3 velocity, 
                         Vec3 forward, Vec3 up);

/**
 * Update audio system (call every frame)
 */
void Audio_Update(float dt);

// ============================================================================
// VOLUME CONTROL
// ============================================================================

/**
 * Set master volume
 * @param volume 0.0 - 1.0
 */
void Audio_SetMasterVolume(float volume);

/**
 * Set music volume
 * @param volume 0.0 - 1.0
 */
void Audio_SetMusicVolume(float volume);

/**
 * Set SFX volume
 * @param volume 0.0 - 1.0
 */
void Audio_SetSFXVolume(float volume);

/**
 * Set engine sound volume
 * @param volume 0.0 - 1.0
 */
void Audio_SetEngineVolume(float volume);

/**
 * Set ambient sound volume
 * @param volume 0.0 - 1.0
 */
void Audio_SetAmbientVolume(float volume);

/**
 * Mute/unmute all audio
 */
void Audio_SetMuted(BOOL muted);

// ============================================================================
// SOUND PLAYBACK
// ============================================================================

/**
 * Load sound from file (WAV, OGG, etc.)
 * 
 * @param filename Sound file path
 * @param name Unique name for the sound
 * @return AudioSource handle, or NULL on failure
 */
AudioSource Audio_LoadSound(const char* filename, const char* name);

/**
 * Load sound from memory
 * 
 * @param data Sound data (WAV format)
 * @param size Data size
 * @param name Unique name
 * @return AudioSource handle
 */
AudioSource Audio_LoadSoundFromMemory(const void* data, size_t size, const char* name);

/**
 * Play a sound effect
 * 
 * @param sound Sound effect enum
 * @param position World position (for 3D audio), or NULL for 2D
 * @param gain Volume 0.0 - 1.0
 * @param pitch Pitch multiplier (1.0 = normal)
 */
void Audio_PlaySound(SoundEffect sound, Vec3* position, float gain, float pitch);

/**
 * Create a playing source from a sound
 * 
 * @param sound Sound effect enum
 * @param position World position (NULL for 2D)
 * @param looping Loop the sound
 * @return AudioSource handle
 */
AudioSource Audio_CreateSource(SoundEffect sound, Vec3* position, BOOL looping);

/**
 * Play a source
 */
void Audio_Play(AudioSource source);

/**
 * Stop a source
 */
void Audio_Stop(AudioSource source);

/**
 * Pause a source
 */
void Audio_Pause(AudioSource source);

/**
 * Set source position
 */
void Audio_SetPosition(AudioSource source, Vec3 position);

/**
 * Set source velocity (for Doppler effect)
 */
void Audio_SetVelocity(AudioSource source, Vec3 velocity);

/**
 * Set source pitch
 */
void Audio_SetPitch(AudioSource source, float pitch);

/**
 * Set source gain/volume
 */
void Audio_SetGain(AudioSource source, float gain);

/**
 * Check if source is playing
 */
BOOL Audio_IsPlaying(AudioSource source);

/**
 * Destroy a source
 */
void Audio_DestroySource(AudioSource source);

// ============================================================================
// 3D AUDIO HELPERS
// ============================================================================

/**
 * Update positional audio for all sources
 */
void Audio_Update3D(void);

/**
 * Set distance model for 3D audio
 * 
 * @param model AL_INVERSE_DISTANCE, AL_LINEAR_DISTANCE, AL_EXPONENT_DISTANCE, etc.
 */
void Audio_SetDistanceModel(ALenum model);

/**
 * Set reference distance for 3D audio
 */
void Audio_SetReferenceDistance(float distance);

/**
 * Set max distance for 3D audio
 */
void Audio_SetMaxDistance(float distance);

// ============================================================================
// ENGINE SOUND SYNTHESIS
// ============================================================================

/**
 * Initialize engine sound synthesis
 * Creates procedural engine sounds based on RPM
 */
BOOL Audio_InitEngineSounds(void);

/**
 * Update engine sound based on car state
 * 
 * @param rpm Engine RPM (0 - 8000)
 * @param throttle Throttle position (0.0 - 1.0)
 * @param speed Car speed for doppler
 */
void Audio_UpdateEngineSound(float rpm, float throttle, float speed);

/**
 * Play engine sound at position
 */
void Audio_PlayEngineSound(Vec3 position);

/**
 * Stop engine sound
 */
void Audio_StopEngineSound(void);

// ============================================================================
// MUSIC STREAMING
// ============================================================================

/**
 * Play music stream from file
 * 
 * @param filename Music file path (OGG, MP3, WAV)
 * @param looping Loop the music
 */
BOOL Audio_PlayMusic(const char* filename, BOOL looping);

/**
 * Stop music playback
 */
void Audio_StopMusic(void);

/**
 * Pause music playback
 */
void Audio_PauseMusic(void);

/**
 * Resume paused music
 */
void Audio_ResumeMusic(void);

/**
 * Set music position (for streaming files)
 * @param position Position in seconds
 */
void Audio_SetMusicPosition(float position);

/**
 * Get music position
 * @return Position in seconds
 */
float Audio_GetMusicPosition(void);

// ============================================================================
// AMBIENT SOUNDS
// ============================================================================

/**
 * Play ambient sound (rain, wind, crowd, etc.)
 * 
 * @param sound Ambient sound type
 * @param position World position (or NULL for global)
 */
void Audio_PlayAmbient(SoundEffect sound, Vec3* position);

/**
 * Update ambient sound (for moving sources like passing cars)
 */
void Audio_UpdateAmbient(Vec3* carPosition, float speed);

// ============================================================================
// LOW-LEVEL OPENAL ACCESS
// ============================================================================

/**
 * Get OpenAL device handle
 */
ALCdevice* Audio_GetDevice(void);

/**
 * Get OpenAL context handle
 */
ALCcontext* Audio_GetContext(void);

/**
 * Get last OpenAL error code
 */
ALenum Audio_GetError(void);

/**
 * Get error string for OpenAL error code
 */
const char* Audio_GetErrorString(ALenum error);

// ============================================================================
// WAVEFORM GENERATION (for procedural sounds)
// ============================================================================

/**
 * Generate a sine wave
 */
void Audio_GenerateSine(float* buffer, int samples, float frequency, float amplitude);

/**
 * Generate white noise
 */
void Audio_GenerateNoise(float* buffer, int samples, float amplitude);

/**
 * Generate a sawtooth wave
 */
void Audio_GenerateSawtooth(float* buffer, int samples, float frequency, float amplitude);

/**
 * Generate engine-like sound using multiple harmonics
 */
void Audio_GenerateEngineSound(float* buffer, int samples, float rpm, float throttle);

#endif // AUDIO_H
