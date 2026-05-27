#include "audio_system.h"
#include "../core/logger.h"
#include "../core/config.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <mutex>

namespace Sylva {

// Static member initialization
bool AudioSystem::initialized = false;
float AudioSystem::masterVolume = 1.0f;
std::unordered_map<AudioType, float> AudioSystem::typeVolumes = {
    {AudioType::SOUND_EFFECT, 1.0f},
    {AudioType::MUSIC, 1.0f},
    {AudioType::AMBIENT, 1.0f},
    {AudioType::VOICE, 1.0f}
};
std::unordered_map<AudioType, bool> AudioSystem::typeMuteStates = {
    {AudioType::SOUND_EFFECT, false},
    {AudioType::MUSIC, false},
    {AudioType::AMBIENT, false},
    {AudioType::VOICE, false}
};

// Internal structures and variables
namespace {
    ALCdevice* audioDevice = nullptr;
    ALCcontext* audioContext = nullptr;

    struct AudioTypeMeta {
        AudioType type;
        const char* name;
        const char* volumeKey;
        const char* muteKey;
    };
    constexpr AudioTypeMeta kAudioTypeMeta[] = {
        {AudioType::SOUND_EFFECT, "SOUND_EFFECT", "Audio.sound_effect_volume", "Audio.mute_sound_effects"},
        {AudioType::MUSIC,        "MUSIC",        "Audio.music_volume",        "Audio.mute_music"},
        {AudioType::AMBIENT,      "AMBIENT",      "Audio.ambient_volume",      "Audio.mute_ambient"},
        {AudioType::VOICE,        "VOICE",        "Audio.voice_volume",        "Audio.mute_voice"},
    };
    const AudioTypeMeta& metaFor(AudioType t) {
        for (const auto& e : kAudioTypeMeta) {
            if (e.type == t) return e;
        }
        return kAudioTypeMeta[0];
    }
    
    struct Sound {
        ALuint buffer;
        AudioType type;
        std::string filePath;
    };
    
    struct SoundInstance {
        ALuint source;
        std::string name;
        bool isPlaying;
        bool isPaused;
        bool isLooping;
        float volume;
        AudioType type;
    };
    
    std::unordered_map<std::string, Sound> soundLibrary;
    std::unordered_map<unsigned int, SoundInstance> activeSounds;
    unsigned int nextSoundId = 1;

    // Protects soundLibrary, activeSounds, nextSoundId, typeVolumes, typeMuteStates,
    // and masterVolume against concurrent access (e.g. update() running while
    // playSound() inserts).
    std::mutex audioMutex;
    
    // Helper function to check for OpenAL errors
    bool checkALError(const std::string& operation) {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::string errorMsg;
            switch (error) {
                case AL_INVALID_NAME: errorMsg = "AL_INVALID_NAME"; break;
                case AL_INVALID_ENUM: errorMsg = "AL_INVALID_ENUM"; break;
                case AL_INVALID_VALUE: errorMsg = "AL_INVALID_VALUE"; break;
                case AL_INVALID_OPERATION: errorMsg = "AL_INVALID_OPERATION"; break;
                case AL_OUT_OF_MEMORY: errorMsg = "AL_OUT_OF_MEMORY"; break;
                default: errorMsg = "Unknown OpenAL error"; break;
            }
            Logger::logError("OpenAL error during " + operation + ": " + errorMsg);
            return false;
        }
        return true;
    }
    
    // Helper to load audio file (stub - would need audio file loading library)
    bool loadAudioFile(const std::string& filePath, ALuint buffer) {
        // In a real implementation, this would use a library like libsndfile, stb_vorbis, etc.
        // to load different audio formats (WAV, OGG, MP3, etc.)
        
        // This is a placeholder that would be replaced with actual file loading code
        Logger::logInfo("Loading audio file: " + filePath);
        
        // Example with hardcoded data for demonstration - NOT ACTUAL IMPLEMENTATION
        // Just to show how the buffer would be filled
        const int sampleRate = 44100;
        const int seconds = 1;
        std::vector<short> samples(sampleRate * seconds);
        
        // Generate a simple sine wave as placeholder audio data
        for (int i = 0; i < samples.size(); i++) {
            samples[i] = short(32760 * sin(2.0f * 3.14159f * 440.0f * i / sampleRate));
        }
        
        // Upload audio data to OpenAL buffer
        alBufferData(buffer, AL_FORMAT_MONO16, samples.data(), samples.size() * sizeof(short), sampleRate);
        return checkALError("loading audio data");
    }
}

bool AudioSystem::initialize() {
    if (initialized) {
        Logger::logWarning("Audio system already initialized");
        return true;
    }
    
    // Open the default audio device
    audioDevice = alcOpenDevice(nullptr);
    if (!audioDevice) {
        Logger::logError("Failed to open audio device");
        return false;
    }
    
    // Create an audio context
    audioContext = alcCreateContext(audioDevice, nullptr);
    if (!audioContext) {
        Logger::logError("Failed to create audio context");
        alcCloseDevice(audioDevice);
        audioDevice = nullptr;
        return false;
    }
    
    // Make the context current
    if (!alcMakeContextCurrent(audioContext)) {
        Logger::logError("Failed to make audio context current");
        alcDestroyContext(audioContext);
        alcCloseDevice(audioDevice);
        audioContext = nullptr;
        audioDevice = nullptr;
        return false;
    }
    
    // Initialize volumes + mute states from config (per-type table-driven).
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        masterVolume = std::clamp(Config::getFloat("Audio.master_volume", masterVolume), 0.0f, 1.0f);
        for (const auto& e : kAudioTypeMeta) {
            typeVolumes[e.type]    = std::clamp(Config::getFloat(e.volumeKey, typeVolumes[e.type]), 0.0f, 1.0f);
            typeMuteStates[e.type] = Config::getBool(e.muteKey, typeMuteStates[e.type]);
        }
    }

    initialized = true;
    Logger::logInfo("Audio system initialized");
    return true;
}

void AudioSystem::shutdown() {
    if (!initialized) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(audioMutex);
        for (auto& pair : activeSounds) {
            alSourceStop(pair.second.source);
            alDeleteSources(1, &pair.second.source);
        }
        activeSounds.clear();
        for (auto& pair : soundLibrary) {
            alDeleteBuffers(1, &pair.second.buffer);
        }
        soundLibrary.clear();
    }
    
    // Destroy OpenAL context and close device
    alcMakeContextCurrent(nullptr);
    if (audioContext) {
        alcDestroyContext(audioContext);
        audioContext = nullptr;
    }
    if (audioDevice) {
        alcCloseDevice(audioDevice);
        audioDevice = nullptr;
    }
    
    initialized = false;
    Logger::logInfo("Audio system shut down");
}

bool AudioSystem::loadSound(const std::string& name, const std::string& filePath, AudioType type) {
    if (!initialized) {
        Logger::logError("Cannot load sound, audio system not initialized");
        return false;
    }

    std::lock_guard<std::mutex> lock(audioMutex);
    if (soundLibrary.find(name) != soundLibrary.end()) {
        Logger::logWarning("Sound already loaded: " + name);
        return true;
    }
    
    // Create a new OpenAL buffer
    ALuint buffer;
    alGenBuffers(1, &buffer);
    if (!checkALError("generating buffer")) {
        return false;
    }
    
    // Load audio file into buffer (implementation-specific)
    if (!loadAudioFile(filePath, buffer)) {
        alDeleteBuffers(1, &buffer);
        Logger::logError("Failed to load audio file: " + filePath);
        return false;
    }
    
    // Store the sound in our library
    Sound sound = {buffer, type, filePath};
    soundLibrary[name] = sound;
    
    Logger::logInfo("Sound loaded: " + name);
    return true;
}

void AudioSystem::unloadSound(const std::string& name) {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = soundLibrary.find(name);
    if (it == soundLibrary.end()) {
        Logger::logWarning("Cannot unload sound, not found: " + name);
        return;
    }
    
    // Stop any instances of this sound
    std::vector<unsigned int> toRemove;
    for (auto& pair : activeSounds) {
        if (pair.second.name == name) {
            alSourceStop(pair.second.source);
            alDeleteSources(1, &pair.second.source);
            toRemove.push_back(pair.first);
        }
    }
    
    for (unsigned int id : toRemove) {
        activeSounds.erase(id);
    }
    
    // Delete the buffer
    alDeleteBuffers(1, &it->second.buffer);
    soundLibrary.erase(it);
    
    Logger::logInfo("Sound unloaded: " + name);
}

void AudioSystem::unloadAllSounds() {
    if (!initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(audioMutex);
    for (auto& pair : activeSounds) {
        alSourceStop(pair.second.source);
        alDeleteSources(1, &pair.second.source);
    }
    activeSounds.clear();
    
    // Delete all buffers
    for (auto& pair : soundLibrary) {
        alDeleteBuffers(1, &pair.second.buffer);
    }
    soundLibrary.clear();
    
    Logger::logInfo("All sounds unloaded");
}

unsigned int AudioSystem::playSound(const std::string& name, bool loop, float volume) {
    if (!initialized) {
        Logger::logError("Cannot play sound, audio system not initialized");
        return 0;
    }

    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = soundLibrary.find(name);
    if (it == soundLibrary.end()) {
        Logger::logWarning("Sound not found: " + name);
        return 0;
    }
    
    // Check if this sound type is muted
    if (typeMuteStates[it->second.type]) {
        Logger::logInfo("Sound type is muted, not playing: " + name);
        return 0; // Return 0 to indicate no sound was played
    }
    
    // Create a source
    ALuint source;
    alGenSources(1, &source);
    if (!checkALError("generating source")) {
        return 0;
    }
    
    // Set source properties
    alSourcei(source, AL_BUFFER, it->second.buffer);
    alSourcef(source, AL_GAIN, volume * masterVolume * typeVolumes[it->second.type]);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    
    // Play the source
    alSourcePlay(source);
    if (!checkALError("playing sound")) {
        alDeleteSources(1, &source);
        return 0;
    }
    
    // Store the sound instance
    unsigned int soundId = nextSoundId++;
    SoundInstance instance = {
        source,
        name,
        true,   // isPlaying
        false,  // isPaused
        loop,   // isLooping
        volume, // volume
        it->second.type // Sound type
    };
    
    activeSounds[soundId] = instance;
    return soundId;
}

void AudioSystem::stopSound(unsigned int soundId) {
    if (!initialized) return;
    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = activeSounds.find(soundId);
    if (it == activeSounds.end()) return;
    alSourceStop(it->second.source);
    alDeleteSources(1, &it->second.source);
    activeSounds.erase(it);
}

void AudioSystem::pauseSound(unsigned int soundId) {
    if (!initialized) return;
    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = activeSounds.find(soundId);
    if (it == activeSounds.end() || it->second.isPaused) return;
    alSourcePause(it->second.source);
    it->second.isPaused = true;
    it->second.isPlaying = false;
}

void AudioSystem::resumeSound(unsigned int soundId) {
    if (!initialized) return;
    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = activeSounds.find(soundId);
    if (it == activeSounds.end() || !it->second.isPaused) return;
    alSourcePlay(it->second.source);
    it->second.isPaused = false;
    it->second.isPlaying = true;
}

void AudioSystem::setMasterVolume(float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        masterVolume = volume;
        for (auto& pair : activeSounds) {
            float finalVolume = pair.second.volume * masterVolume * typeVolumes[pair.second.type];
            alSourcef(pair.second.source, AL_GAIN, finalVolume);
        }
    }
    Config::set("Audio.master_volume", volume);
}

float AudioSystem::getMasterVolume() {
    std::lock_guard<std::mutex> lock(audioMutex);
    return masterVolume;
}

void AudioSystem::setTypeVolume(AudioType type, float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        typeVolumes[type] = volume;
        for (auto& pair : activeSounds) {
            if (pair.second.type == type) {
                float finalVolume = pair.second.volume * masterVolume * volume;
                alSourcef(pair.second.source, AL_GAIN, finalVolume);
            }
        }
    }
    Config::set(metaFor(type).volumeKey, volume);
}

float AudioSystem::getTypeVolume(AudioType type) {
    std::lock_guard<std::mutex> lock(audioMutex);
    return typeVolumes[type];
}

void AudioSystem::setListenerPosition(const float* position) {
    if (!initialized) {
        return;
    }
    
    alListener3f(AL_POSITION, position[0], position[1], position[2]);
}

void AudioSystem::setListenerOrientation(const float* forward, const float* up) {
    if (!initialized) {
        return;
    }
    
    // OpenAL expects a 6-float array with forward vector (at) followed by up vector
    float orientation[6] = {
        forward[0], forward[1], forward[2],
        up[0], up[1], up[2]
    };
    
    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioSystem::setSoundPosition(unsigned int soundId, const float* position) {
    if (!initialized) return;
    std::lock_guard<std::mutex> lock(audioMutex);
    auto it = activeSounds.find(soundId);
    if (it == activeSounds.end()) return;
    alSource3f(it->second.source, AL_POSITION, position[0], position[1], position[2]);
}

bool AudioSystem::isInitialized() {
    return initialized;
}

void AudioSystem::update() {
    if (!initialized) return;
    std::lock_guard<std::mutex> lock(audioMutex);
    std::vector<unsigned int> toRemove;
    for (auto& pair : activeSounds) {
        if (!pair.second.isLooping) {
            ALint state = 0;
            alGetSourcei(pair.second.source, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED) {
                alDeleteSources(1, &pair.second.source);
                toRemove.push_back(pair.first);
            }
        }
    }
    for (unsigned int id : toRemove) {
        activeSounds.erase(id);
    }
}

// Mute controls implementation
void AudioSystem::setTypeMuted(AudioType type, bool muted) {
    if (!initialized) {
        Logger::logWarning("Cannot set mute state, audio system not initialized");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(audioMutex);
        typeMuteStates[type] = muted;
        for (auto& pair : activeSounds) {
            if (pair.second.type != type) continue;
            if (muted) {
                if (pair.second.isPlaying && !pair.second.isPaused) {
                    alSourceStop(pair.second.source);
                    pair.second.isPlaying = false;
                }
            } else {
                if (!pair.second.isPaused) {
                    alSourcePlay(pair.second.source);
                    pair.second.isPlaying = true;
                }
            }
        }
    }
    const auto& meta = metaFor(type);
    Config::set<bool>(meta.muteKey, muted);
    Logger::logInfo(std::string("Audio type ") + meta.name + (muted ? " muted" : " unmuted"));
}

bool AudioSystem::isTypeMuted(AudioType type) {
    if (!initialized) {
        Logger::logWarning("Cannot check mute state, audio system not initialized");
        return false;
    }
    std::lock_guard<std::mutex> lock(audioMutex);
    return typeMuteStates[type];
}

void AudioSystem::muteAll() {
    if (!initialized) {
        Logger::logWarning("Cannot mute all, audio system not initialized");
        return;
    }
    
    setTypeMuted(AudioType::SOUND_EFFECT, true);
    setTypeMuted(AudioType::MUSIC, true);
    setTypeMuted(AudioType::AMBIENT, true);
    setTypeMuted(AudioType::VOICE, true);
    
    Logger::logInfo("All audio types muted");
}

void AudioSystem::unmuteAll() {
    if (!initialized) {
        Logger::logWarning("Cannot unmute all, audio system not initialized");
        return;
    }
    
    setTypeMuted(AudioType::SOUND_EFFECT, false);
    setTypeMuted(AudioType::MUSIC, false);
    setTypeMuted(AudioType::AMBIENT, false);
    setTypeMuted(AudioType::VOICE, false);
    
    Logger::logInfo("All audio types unmuted");
}

} // namespace Sylva 