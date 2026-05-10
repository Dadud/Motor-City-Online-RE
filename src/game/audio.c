/**
 * audio.c - OpenAL Audio Implementation
 * 
 * Motor City Online - Audio Implementation using OpenAL
 */

#include "audio.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

// ============================================================================
// GLOBAL AUDIO CONTEXT
// ============================================================================

static AudioContext g_audio;
static BOOL g_initialized = FALSE;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static const char* ErrorString(ALenum error) {
    switch (error) {
        case AL_NO_ERROR: return "No error";
        case AL_INVALID_NAME: return "Invalid name";
        case AL_INVALID_ENUM: return "Invalid enum";
        case AL_INVALID_VALUE: return "Invalid value";
        case AL_INVALID_OPERATION: return "Invalid operation";
        case AL_OUT_OF_MEMORY: return "Out of memory";
        default: return "Unknown error";
    }
}

static void CheckALError(const char* operation) {
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        fprintf(stderr, "OpenAL Error during %s: %s\n", operation, ErrorString(error));
    }
}

// ============================================================================
// WAVEFORM GENERATION
// ============================================================================

void Audio_GenerateSine(float* buffer, int samples, float frequency, float amplitude) {
    float phase = 0.0f;
    float phaseIncrement = 2.0f * PI * frequency / 44100.0f;
    
    for (int i = 0; i < samples; i++) {
        buffer[i] = amplitude * sinf(phase);
        phase += phaseIncrement;
        if (phase > 2.0f * PI) phase -= 2.0f * PI;
    }
}

void Audio_GenerateNoise(float* buffer, int samples, float amplitude) {
    for (int i = 0; i < samples; i++) {
        buffer[i] = amplitude * (2.0f * (rand() / (float)RAND_MAX) - 1.0f);
    }
}

void Audio_GenerateSawtooth(float* buffer, int samples, float frequency, float amplitude) {
    float phase = 0.0f;
    float phaseIncrement = frequency / 44100.0f;
    
    for (int i = 0; i < samples; i++) {
        buffer[i] = amplitude * (2.0f * phase - 1.0f);
        phase += phaseIncrement;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

void Audio_GenerateEngineSound(float* buffer, int samples, float rpm, float throttle) {
    // Engine sound is combination of multiple frequencies
    // Base frequency is RPM / 60 (4-cylinder 4-stroke = 2 revolutions per cycle)
    float baseFreq = rpm / 60.0f * 2.0f;  // Firing frequency
    
    // Generate multiple harmonics
    float totalSamples = (float)samples;
    
    for (int i = 0; i < samples; i++) {
        float t = (float)i / 44100.0f;
        float sample = 0.0f;
        
        // Fundamental frequency
        sample += sinf(2.0f * PI * baseFreq * t) * 0.5f;
        
        // 2nd harmonic
        sample += sinf(2.0f * PI * baseFreq * 2.0f * t) * 0.3f;
        
        // 3rd harmonic
        sample += sinf(2.0f * PI * baseFreq * 3.0f * t) * 0.15f;
        
        // 4th harmonic (adds roughness)
        sample += sinf(2.0f * PI * baseFreq * 4.0f * t) * 0.1f;
        
        // Add some noise for texture
        sample += (2.0f * (rand() / (float)RAND_MAX) - 1.0f) * 0.05f;
        
        // Scale by throttle
        buffer[i] = sample * throttle * 0.7f;
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

BOOL Audio_Init(void) {
    if (g_initialized) {
        return TRUE;
    }
    
    memset(&g_audio, 0, sizeof(g_audio));
    
    // Open audio device
    g_audio.device = alcOpenDevice(NULL);
    if (!g_audio.device) {
        fprintf(stderr, "Failed to open OpenAL device\n");
        return FALSE;
    }
    
    // Create context
    ALCint attributes[] = {
        ALC_FREQUENCY, 44100,
        ALC_STEREO_ROOM, ALC_STEREO,
        ALC_DEFAULT_ALL, ALC_TRUE,
        0
    };
    
    g_audio.context = alcCreateContext(g_audio.device, attributes);
    if (!g_audio.context) {
        fprintf(stderr, "Failed to create OpenAL context\n");
        alcCloseDevice(g_audio.device);
        return FALSE;
    }
    
    alcMakeContextCurrent(g_audio.context);
    
    // Check for errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        fprintf(stderr, "OpenAL error after init: %s\n", ErrorString(error));
    }
    
    // Set distance model
    alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
    alDopplerFactor(1.0f);
    alSpeedOfSound(343.0f);
    
    // Initialize listener
    g_audio.listenerPos = Vec3_Create(0, 0, 0);
    g_audio.listenerVel = Vec3_Create(0, 0, 0);
    g_audio.listenerForward = Vec3_Create(0, 0, 1);
    g_audio.listenerUp = Vec3_Create(0, 1, 0);
    alListener3f(AL_POSITION, 0, 0, 0);
    alListener3f(AL_VELOCITY, 0, 0, 0);
    ALfloat orient[] = { 0, 0, 1,  0, 1, 0 };
    alListenerfv(AL_ORIENTATION, orient);
    
    // Set default volumes
    g_audio.masterVolume = 1.0f;
    g_audio.musicVolume = 0.8f;
    g_audio.sfxVolume = 1.0f;
    g_audio.engineVolume = 1.0f;
    g_audio.ambientVolume = 0.6f;
    
    // Initialize engine sound
    Audio_InitEngineSounds();
    
    g_audio.initialized = TRUE;
    g_initialized = TRUE;
    
    printf("OpenAL Audio initialized\n");
    printf("  Device: %s\n", alcGetString(g_audio.device, ALC_DEVICE_SPECIFIER));
    
    return TRUE;
}

void Audio_Shutdown(void) {
    if (!g_initialized) return;
    
    // Stop all sources
    for (int i = 0; i < g_audio.sourceCount; i++) {
        if (g_audio.sources[i]) {
            alSourceStop(g_audio.sources[i]->source);
            alDeleteSources(1, &g_audio.sources[i]->source);
            free(g_audio.sources[i]);
            g_audio.sources[i] = NULL;
        }
    }
    
    // Delete buffers
    alDeleteBuffers(g_audio.bufferCount, g_audio.buffers);
    
    // Shutdown OpenAL
    alcMakeContextCurrent(NULL);
    alcDestroyContext(g_audio.context);
    alcCloseDevice(g_audio.device);
    
    g_initialized = FALSE;
    g_audio.initialized = FALSE;
    
    printf("OpenAL Audio shutdown complete\n");
}

void Audio_UpdateListener(Vec3 position, Vec3 velocity, Vec3 forward, Vec3 up) {
    g_audio.listenerPos = position;
    g_audio.listenerVel = velocity;
    g_audio.listenerForward = forward;
    g_audio.listenerUp = up;
    
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALfloat orient[] = { forward.x, forward.y, forward.z, up.x, up.y, up.z };
    alListenerfv(AL_ORIENTATION, orient);
}

void Audio_Update(float dt) {
    if (!g_initialized) return;
    
    // Update engine sound
    if (g_audio.engineRPM > 0) {
        Audio_UpdateEngineSound(g_audio.engineRPM, g_audio.engineThrottle, 0);
    }
}

ALvoid AL_APIENTRY RenderEngineSoundStatic(ALvoid *buffer, ALCcontext *context, ALsizei freq, ALenum format) {
    // This is a callback for streaming - simplified here
}

BOOL Audio_InitEngineSounds(void) {
    // Generate engine sound buffers at different RPMs
    // In a real implementation, you'd generate these procedurally or load samples
    
    // Create engine buffers (these would be generated or loaded)
    alGenBuffers(4, g_audio.engineBuffers);
    
    // Generate 4 engine sound buffers at different RPM ranges
    const int sampleRate = 44100;
    const int bufferSize = sampleRate;  // 1 second buffers
    float samples[bufferSize];
    
    // Buffer 0: Very low RPM (idle)
    Audio_GenerateEngineSound(samples, bufferSize, 800.0f, 0.3f);
    alBufferData(g_audio.engineBuffers[0], AL_FORMAT_MONO16, samples, 
                 bufferSize * sizeof(short), sampleRate);
    
    // Buffer 1: Medium-low RPM
    Audio_GenerateEngineSound(samples, bufferSize, 3000.0f, 0.6f);
    alBufferData(g_audio.engineBuffers[1], AL_FORMAT_MONO16, samples,
                 bufferSize * sizeof(short), sampleRate);
    
    // Buffer 2: Medium-high RPM
    Audio_GenerateEngineSound(samples, bufferSize, 5500.0f, 0.8f);
    alBufferData(g_audio.engineBuffers[2], AL_FORMAT_MONO16, samples,
                 bufferSize * sizeof(short), sampleRate);
    
    // Buffer 3: High RPM (redline)
    Audio_GenerateEngineSound(samples, bufferSize, 7500.0f, 1.0f);
    alBufferData(g_audio.engineBuffers[3], AL_FORMAT_MONO16, samples,
                 bufferSize * sizeof(short), sampleRate);
    
    // Create engine source
    alGenSources(1, &g_audio.engineSource);
    alSourcei(g_audio.engineSource, AL_BUFFER, g_audio.engineBuffers[0]);
    alSourcei(g_audio.engineSource, AL_LOOPING, AL_TRUE);
    alSourcef(g_audio.engineSource, AL_GAIN, g_audio.engineVolume);
    alSourcef(g_audio.engineSource, AL_PITCH, 1.0f);
    
    g_audio.engineRPM = 0;
    g_audio.engineThrottle = 0;
    
    return TRUE;
}

void Audio_UpdateEngineSound(float rpm, float throttle, float speed) {
    if (!g_initialized) return;
    
    g_audio.engineRPM = rpm;
    g_audio.engineThrottle = throttle;
    
    // Calculate which buffer to use based on RPM
    int bufferIndex = 0;
    if (rpm >= 7000) bufferIndex = 3;
    else if (rpm >= 4500) bufferIndex = 2;
    else if (rpm >= 2000) bufferIndex = 1;
    else bufferIndex = 0;
    
    // Get current buffer
    ALint currentBuffer;
    alGetSourcei(g_audio.engineSource, AL_BUFFER, &currentBuffer);
    
    // Switch buffers if RPM range changed
    if (currentBuffer != (ALint)g_audio.engineBuffers[bufferIndex]) {
        alSourceStop(g_audio.engineSource);
        alSourcei(g_audio.engineSource, AL_BUFFER, g_audio.engineBuffers[bufferIndex]);
        alSourcePlay(g_audio.engineSource);
    }
    
    // Calculate pitch based on exact RPM
    // Base RPM is 800 (idle), pitch is relative to that
    float pitch = rpm / 800.0f;
    if (pitch < 0.5f) pitch = 0.5f;
    if (pitch > 2.0f) pitch = 2.0f;
    
    alSourcef(g_audio.engineSource, AL_PITCH, pitch);
    
    // Volume based on throttle
    float volume = g_audio.engineVolume * g_audio.masterVolume * throttle;
    alSourcef(g_audio.engineSource, AL_GAIN, volume);
}

void Audio_PlayEngineSound(Vec3 position) {
    if (!g_initialized) return;
    
    alSource3f(g_audio.engineSource, AL_POSITION, position.x, position.y, position.z);
    alSourcePlay(g_audio.engineSource);
}

void Audio_StopEngineSound(void) {
    if (!g_initialized) return;
    
    alSourceStop(g_audio.engineSource);
    g_audio.engineRPM = 0;
    g_audio.engineThrottle = 0;
}

// ============================================================================
// VOLUME CONTROL
// ============================================================================

void Audio_SetMasterVolume(float volume) {
    g_audio.masterVolume = volume > 1.0f ? 1.0f : (volume < 0.0f ? 0.0f : volume);
    
    // Update all sources
    for (int i = 0; i < g_audio.sourceCount; i++) {
        if (g_audio.sources[i] && g_audio.sources[i]->playing) {
            alSourcef(g_audio.sources[i]->source, AL_GAIN, 
                     g_audio.sources[i]->gain * g_audio.masterVolume);
        }
    }
}

void Audio_SetMusicVolume(float volume) {
    g_audio.musicVolume = volume > 1.0f ? 1.0f : (volume < 0.0f ? 0.0f : volume);
}

void Audio_SetSFXVolume(float volume) {
    g_audio.sfxVolume = volume > 1.0f ? 1.0f : (volume < 0.0f ? 0.0f : volume);
}

void Audio_SetEngineVolume(float volume) {
    g_audio.engineVolume = volume > 1.0f ? 1.0f : (volume < 0.0f ? 0.0f : volume);
    alSourcef(g_audio.engineSource, AL_GAIN, g_audio.engineVolume * g_audio.masterVolume);
}

void Audio_SetAmbientVolume(float volume) {
    g_audio.ambientVolume = volume > 1.0f ? 1.0f : (volume < 0.0f ? 0.0f : volume);
}

void Audio_SetMuted(BOOL muted) {
    g_audio.muted = muted;
    if (muted) {
        alListenerf(AL_GAIN, 0.0f);
    } else {
        alListenerf(AL_GAIN, g_audio.masterVolume);
    }
}

// ============================================================================
// SOUND LOADING AND PLAYBACK
// ============================================================================

AudioSource Audio_LoadSound(const char* filename, const char* name) {
    if (!g_initialized) return NULL;
    
    // This is a simplified WAV loader
    // A full implementation would support OGG, MP3, etc. using libvorbis/libmpg123
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Audio: Could not open file %s\n", filename);
        return NULL;
    }
    
    // Read WAV header
    char chunkID[4];
    unsigned int chunkSize;
    char format[4];
    char subchunk1ID[4];
    unsigned int subchunk1Size;
    short audioFormat;
    short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    short blockAlign;
    short bitsPerSample;
    char subchunk2ID[4];
    unsigned int subchunk2Size;
    
    fread(chunkID, 1, 4, file);
    fread(&chunkSize, 4, 1, file);
    fread(format, 1, 4, file);
    fread(subchunk1ID, 1, 4, file);
    fread(&subchunk1Size, 4, 1, file);
    fread(&audioFormat, 2, 1, file);
    fread(&numChannels, 2, 1, file);
    fread(&sampleRate, 4, 1, file);
    fread(&byteRate, 4, 1, file);
    fread(&blockAlign, 2, 1, file);
    fread(&bitsPerSample, 2, 1, file);
    fread(subchunk2ID, 1, 4, file);
    fread(&subchunk2Size, 4, 1, file);
    
    if (strncmp(chunkID, "RIFF", 4) != 0 || strncmp(format, "WAVE", 4) != 0) {
        fprintf(stderr, "Audio: Not a valid WAV file: %s\n", filename);
        fclose(file);
        return NULL;
    }
    
    // Read audio data
    unsigned char* data = malloc(subchunk2Size);
    fread(data, 1, subchunk2Size, file);
    fclose(file);
    
    // Create OpenAL buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    
    ALenum formatType = (numChannels == 1) ? 
        ((bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
        ((bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16);
    
    alBufferData(buffer, formatType, data, subchunk2Size, sampleRate);
    
    free(data);
    
    // Create source
    AudioSource source = (AudioSource)malloc(sizeof(struct audio_source_s));
    memset(source, 0, sizeof(struct audio_source_s));
    
    alGenSources(1, &source->source);
    alSourcei(source->source, AL_BUFFER, buffer);
    alSourcef(source->source, AL_PITCH, 1.0f);
    alSourcef(source->source, AL_GAIN, g_audio.sfxVolume);
    alSource3f(source->source, AL_POSITION, 0, 0, 0);
    alSource3f(source->source, AL_VELOCITY, 0, 0, 0);
    source->buffer = buffer;
    source->gain = 1.0f;
    source->pitch = 1.0f;
    source->playing = FALSE;
    strncpy(source->name, name, sizeof(source->name) - 1);
    
    // Add to pool
    if (g_audio.sourceCount < AUDIO_MAX_SOURCES) {
        g_audio.sources[g_audio.sourceCount++] = source;
    }
    
    return source;
}

AudioSource Audio_CreateSource(SoundEffect sound, Vec3* position, BOOL looping) {
    if (!g_initialized) return NULL;
    
    // Generate a procedural sound based on type
    const int sampleRate = 44100;
    const int bufferSize = sampleRate / 10;  // 100ms buffer
    float samples[bufferSize];
    
    // Generate sound based on type
    switch (sound) {
        case SOUND_TIRE_SCREECH:
            Audio_GenerateSawtooth(samples, bufferSize, 800.0f, 0.3f);
            break;
        case SOUND_COLLISION_LIGHT:
            Audio_GenerateSine(samples, bufferSize, 200.0f, 0.5f);
            break;
        case SOUND_COLLISION_MEDIUM:
            Audio_GenerateSine(samples, bufferSize, 150.0f, 0.6f);
            break;
        case SOUND_COLLISION_HEAVY:
            Audio_GenerateNoise(samples, bufferSize, 0.7f);
            break;
        case SOUND_WIND:
            Audio_GenerateNoise(samples, bufferSize, 0.2f);
            break;
        case SOUND_MENU_SELECT:
            Audio_GenerateSine(samples, bufferSize, 880.0f, 0.3f);
            break;
        case SOUND_MENU_CONFIRM:
            Audio_GenerateSine(samples, bufferSize, 1320.0f, 0.4f);
            break;
        case SOUND_BUTTON_CLICK:
            Audio_GenerateSine(samples, bufferSize, 660.0f, 0.2f);
            break;
        default:
            Audio_GenerateSine(samples, bufferSize, 440.0f, 0.3f);
            break;
    }
    
    // Create buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    short shortSamples[bufferSize];
    for (int i = 0; i < bufferSize; i++) {
        shortSamples[i] = (short)(samples[i] * 32767.0f);
    }
    alBufferData(buffer, AL_FORMAT_MONO16, shortSamples, sizeof(shortSamples), sampleRate);
    
    // Create source
    AudioSource source = (AudioSource)malloc(sizeof(struct audio_source_s));
    memset(source, 0, sizeof(struct audio_source_s));
    
    alGenSources(1, &source->source);
    alSourcei(source->source, AL_BUFFER, buffer);
    alSourcef(source->source, AL_PITCH, 1.0f);
    alSourcef(source->source, AL_GAIN, g_audio.sfxVolume);
    alSourcei(source->source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    
    if (position) {
        alSource3f(source->source, AL_POSITION, position->x, position->y, position->z);
        source->position = *position;
    } else {
        alSource3f(source->source, AL_POSITION, 0, 0, 0);
    }
    
    alSource3f(source->source, AL_VELOCITY, 0, 0, 0);
    source->buffer = buffer;
    source->gain = 1.0f;
    source->pitch = 1.0f;
    source->looping = looping;
    source->playing = FALSE;
    
    // Add to pool
    if (g_audio.sourceCount < AUDIO_MAX_SOURCES) {
        g_audio.sources[g_audio.sourceCount++] = source;
    }
    
    return source;
}

void Audio_PlaySound(SoundEffect sound, Vec3* position, float gain, float pitch) {
    AudioSource source = Audio_CreateSource(sound, position, FALSE);
    if (source) {
        source->gain = gain;
        source->pitch = pitch;
        alSourcef(source->source, AL_GAIN, gain * g_audio.sfxVolume * g_audio.masterVolume);
        alSourcef(source->source, AL_PITCH, pitch);
        alSourcePlay(source->source);
        source->playing = TRUE;
    }
}

void Audio_Play(AudioSource source) {
    if (!source) return;
    alSourcePlay(source->source);
    source->playing = TRUE;
}

void Audio_Stop(AudioSource source) {
    if (!source) return;
    alSourceStop(source->source);
    source->playing = FALSE;
}

void Audio_Pause(AudioSource source) {
    if (!source) return;
    alSourcePause(source->source);
    source->playing = FALSE;
}

void Audio_SetPosition(AudioSource source, Vec3 position) {
    if (!source) return;
    source->position = position;
    alSource3f(source->source, AL_POSITION, position.x, position.y, position.z);
}

void Audio_SetVelocity(AudioSource source, Vec3 velocity) {
    if (!source) return;
    source->velocity = velocity;
    alSource3f(source->source, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
}

void Audio_SetPitch(AudioSource source, float pitch) {
    if (!source) return;
    source->pitch = pitch;
    alSourcef(source->source, AL_PITCH, pitch);
}

void Audio_SetGain(AudioSource source, float gain) {
    if (!source) return;
    source->gain = gain;
    alSourcef(source->source, AL_GAIN, gain * g_audio.masterVolume);
}

BOOL Audio_IsPlaying(AudioSource source) {
    if (!source) return FALSE;
    ALint state;
    alGetSourcei(source->source, AL_SOURCE_STATE, &state);
    source->playing = (state == AL_PLAYING);
    return source->playing;
}

void Audio_DestroySource(AudioSource source) {
    if (!source) return;
    
    alSourceStop(source->source);
    alDeleteSources(1, &source->source);
    if (source->buffer) {
        alDeleteBuffers(1, &source->buffer);
    }
    
    // Remove from pool
    for (int i = 0; i < g_audio.sourceCount; i++) {
        if (g_audio.sources[i] == source) {
            for (int j = i; j < g_audio.sourceCount - 1; j++) {
                g_audio.sources[j] = g_audio.sources[j + 1];
            }
            g_audio.sourceCount--;
            break;
        }
    }
    
    free(source);
}

// ============================================================================
// 3D AUDIO
// ============================================================================

void Audio_Update3D(void) {
    if (!g_initialized) return;
    
    // OpenAL automatically handles 3D positioning based on listener and source positions
    // This function can be used for any additional processing
}

void Audio_SetDistanceModel(ALenum model) {
    alDistanceModel(model);
}

void Audio_SetReferenceDistance(float distance) {
    alListenerf(AL_REFERENCE_DISTANCE, distance);
}

void Audio_SetMaxDistance(float distance) {
    alListenerf(AL_MAX_DISTANCE, distance);
}

// ============================================================================
// MUSIC STREAMING
// ============================================================================

static ALuint g_musicSource = 0;
static ALuint g_musicBuffer = 0;
static BOOL g_musicPlaying = FALSE;

BOOL Audio_PlayMusic(const char* filename, BOOL looping) {
    if (!g_initialized) return FALSE;
    
    // Stop current music
    Audio_StopMusic();
    
    // Load music file (simplified - just loads as buffer)
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Audio: Could not open music file %s\n", filename);
        return FALSE;
    }
    
    // Read WAV header
    char chunkID[4], format[4], subchunk2ID[4];
    unsigned int chunkSize, subchunk2Size;
    short audioFormat, numChannels;
    unsigned int sampleRate, byteRate;
    short bitsPerSample, blockAlign;
    
    fread(chunkID, 1, 4, file);
    fread(&chunkSize, 4, 1, file);
    fread(format, 1, 4, file);
    char subchunk1ID[4];
    fread(subchunk1ID, 1, 4, file);
    unsigned int subchunk1Size;
    fread(&subchunk1Size, 4, 1, file);
    fread(&audioFormat, 2, 1, file);
    fread(&numChannels, 2, 1, file);
    fread(&sampleRate, 4, 1, file);
    fread(&byteRate, 4, 1, file);
    fread(&blockAlign, 2, 1, file);
    fread(&bitsPerSample, 2, 1, file);
    fread(subchunk2ID, 1, 4, file);
    fread(&subchunk2Size, 4, 1, file);
    
    if (strncmp(chunkID, "RIFF", 4) != 0 || strncmp(format, "WAVE", 4) != 0) {
        fclose(file);
        fprintf(stderr, "Audio: Music file is not a valid WAV: %s\n", filename);
        return FALSE;
    }
    
    // Read audio data
    unsigned char* data = malloc(subchunk2Size);
    fread(data, 1, subchunk2Size, file);
    fclose(file);
    
    // Create buffer and source
    alGenBuffers(1, &g_musicBuffer);
    ALenum formatType = (numChannels == 1) ?
        ((bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16) :
        ((bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16);
    
    alBufferData(g_musicBuffer, formatType, data, subchunk2Size, sampleRate);
    free(data);
    
    alGenSources(1, &g_musicSource);
    alSourcei(g_musicSource, AL_BUFFER, g_musicBuffer);
    alSourcei(g_musicSource, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
    alSourcef(g_musicSource, AL_GAIN, g_audio.musicVolume * g_audio.masterVolume);
    
    alSourcePlay(g_musicSource);
    g_musicPlaying = TRUE;
    
    return TRUE;
}

void Audio_StopMusic(void) {
    if (g_musicSource) {
        alSourceStop(g_musicSource);
        alDeleteSources(1, &g_musicSource);
        g_musicSource = 0;
    }
    if (g_musicBuffer) {
        alDeleteBuffers(1, &g_musicBuffer);
        g_musicBuffer = 0;
    }
    g_musicPlaying = FALSE;
}

void Audio_PauseMusic(void) {
    if (g_musicSource && g_musicPlaying) {
        alSourcePause(g_musicSource);
        g_musicPlaying = FALSE;
    }
}

void Audio_ResumeMusic(void) {
    if (g_musicSource && !g_musicPlaying) {
        alSourcePlay(g_musicSource);
        g_musicPlaying = TRUE;
    }
}

void Audio_SetMusicPosition(float position) {
    if (g_musicSource) {
        alSourcef(g_musicSource, AL_SEC_OFFSET, position);
    }
}

float Audio_GetMusicPosition(void) {
    if (!g_musicSource) return 0.0f;
    ALfloat pos;
    alGetSourcef(g_musicSource, AL_SEC_OFFSET, &pos);
    return pos;
}

// ============================================================================
// AMBIENT SOUNDS
// ============================================================================

void Audio_PlayAmbient(SoundEffect sound, Vec3* position) {
    AudioSource source = Audio_CreateSource(sound, position, TRUE);
    if (source) {
        source->gain = g_audio.ambientVolume;
        alSourcef(source->source, AL_GAIN, g_audio.ambientVolume * g_audio.masterVolume);
        alSourcePlay(source->source);
        source->playing = TRUE;
    }
}

void Audio_UpdateAmbient(Vec3* carPosition, float speed) {
    // Update doppler based on speed
    // This would update source velocities for proper doppler effect
}

// ============================================================================
// LOW-LEVEL ACCESS
// ============================================================================

ALCdevice* Audio_GetDevice(void) {
    return g_audio.device;
}

ALCcontext* Audio_GetContext(void) {
    return g_audio.context;
}

ALenum Audio_GetError(void) {
    return alGetError();
}

const char* Audio_GetErrorString(ALenum error) {
    return ErrorString(error);
}
