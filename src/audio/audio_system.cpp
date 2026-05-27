#include "audio_system.h"
#include "../core/logger.h"
#include "../core/config.h"

#include <AL/al.h>
#include <AL/alc.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

namespace Sylva {

namespace {

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

bool checkALError(const std::string& operation) {
    ALenum error = alGetError();
    if (error == AL_NO_ERROR) return true;
    const char* msg = "Unknown OpenAL error";
    switch (error) {
        case AL_INVALID_NAME:      msg = "AL_INVALID_NAME"; break;
        case AL_INVALID_ENUM:      msg = "AL_INVALID_ENUM"; break;
        case AL_INVALID_VALUE:     msg = "AL_INVALID_VALUE"; break;
        case AL_INVALID_OPERATION: msg = "AL_INVALID_OPERATION"; break;
        case AL_OUT_OF_MEMORY:     msg = "AL_OUT_OF_MEMORY"; break;
        default: break;
    }
    Logger::logError("OpenAL error during " + operation + ": " + msg);
    return false;
}

// --- WAV PCM loader (mono/stereo, 8 or 16 bit) -----------------------------

struct WavData {
    std::vector<uint8_t> samples;
    uint16_t channels = 0;
    uint16_t bitsPerSample = 0;
    uint32_t sampleRate = 0;
};

template <typename T>
bool readLE(std::ifstream& f, T& out) {
    return static_cast<bool>(f.read(reinterpret_cast<char*>(&out), sizeof(T)));
}

std::optional<WavData> loadWavFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return std::nullopt;

    char tag[5] = {0};
    uint32_t size = 0;

    f.read(tag, 4);
    if (std::strncmp(tag, "RIFF", 4) != 0) return std::nullopt;
    readLE(f, size);
    f.read(tag, 4);
    if (std::strncmp(tag, "WAVE", 4) != 0) return std::nullopt;

    WavData wav;
    bool gotFmt = false, gotData = false;
    while (f.read(tag, 4) && readLE(f, size)) {
        if (std::strncmp(tag, "fmt ", 4) == 0) {
            uint16_t audioFormat = 0, channels = 0, blockAlign = 0, bitsPerSample = 0;
            uint32_t sampleRate = 0, byteRate = 0;
            readLE(f, audioFormat);
            readLE(f, channels);
            readLE(f, sampleRate);
            readLE(f, byteRate);
            readLE(f, blockAlign);
            readLE(f, bitsPerSample);
            if (size > 16) f.ignore(static_cast<std::streamsize>(size - 16));
            if (audioFormat != 1) return std::nullopt; // PCM only
            wav.channels = channels;
            wav.sampleRate = sampleRate;
            wav.bitsPerSample = bitsPerSample;
            gotFmt = true;
        } else if (std::strncmp(tag, "data", 4) == 0) {
            wav.samples.resize(size);
            f.read(reinterpret_cast<char*>(wav.samples.data()), size);
            gotData = true;
            break;
        } else {
            f.ignore(static_cast<std::streamsize>(size + (size & 1)));
        }
    }
    if (!gotFmt || !gotData) return std::nullopt;
    return wav;
}

ALenum alFormatFor(uint16_t channels, uint16_t bits) {
    if (channels == 1 && bits == 8)  return AL_FORMAT_MONO8;
    if (channels == 1 && bits == 16) return AL_FORMAT_MONO16;
    if (channels == 2 && bits == 8)  return AL_FORMAT_STEREO8;
    if (channels == 2 && bits == 16) return AL_FORMAT_STEREO16;
    return 0;
}

bool loadAudioFile(const std::string& filePath, ALuint buffer) {
    Logger::logInfo("Loading audio file: " + filePath);

    const auto dot = filePath.find_last_of('.');
    std::string ext = (dot != std::string::npos) ? filePath.substr(dot) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != ".wav") {
        Logger::logWarning("Unsupported audio format (only PCM WAV is supported): " + filePath);
        return false;
    }

    const auto wav = loadWavFile(filePath);
    if (!wav) {
        Logger::logError("Failed to parse WAV file: " + filePath);
        return false;
    }

    const ALenum format = alFormatFor(wav->channels, wav->bitsPerSample);
    if (format == 0) {
        Logger::logError("Unsupported WAV channel/bit combination (" +
                         std::to_string(wav->channels) + "ch, " +
                         std::to_string(wav->bitsPerSample) + "bit): " + filePath);
        return false;
    }

    alBufferData(buffer, format,
                 wav->samples.data(),
                 static_cast<ALsizei>(wav->samples.size()),
                 static_cast<ALsizei>(wav->sampleRate));
    return checkALError("loading audio data");
}

} // namespace

// --- OpenALAudioSystem::State (pImpl) --------------------------------------

struct OpenALAudioSystem::State {
    ALCdevice*  device  = nullptr;
    ALCcontext* context = nullptr;
    bool ready = false;

    float masterVolume = 1.0f;
    std::unordered_map<AudioType, float> typeVolumes{
        {AudioType::SOUND_EFFECT, 1.0f},
        {AudioType::MUSIC, 1.0f},
        {AudioType::AMBIENT, 1.0f},
        {AudioType::VOICE, 1.0f},
    };
    std::unordered_map<AudioType, bool> typeMuteStates{
        {AudioType::SOUND_EFFECT, false},
        {AudioType::MUSIC, false},
        {AudioType::AMBIENT, false},
        {AudioType::VOICE, false},
    };

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

    // Guards all of the above against concurrent access (update() running
    // while playSound() inserts, etc.).
    mutable std::mutex mutex;
};

// --- ctor / dtor -----------------------------------------------------------

OpenALAudioSystem::OpenALAudioSystem() : m_state(std::make_unique<State>()) {
    m_state->device = alcOpenDevice(nullptr);
    if (!m_state->device) {
        Logger::logError("Failed to open audio device");
        return;
    }
    m_state->context = alcCreateContext(m_state->device, nullptr);
    if (!m_state->context) {
        Logger::logError("Failed to create audio context");
        alcCloseDevice(m_state->device);
        m_state->device = nullptr;
        return;
    }
    if (!alcMakeContextCurrent(m_state->context)) {
        Logger::logError("Failed to make audio context current");
        alcDestroyContext(m_state->context);
        alcCloseDevice(m_state->device);
        m_state->context = nullptr;
        m_state->device = nullptr;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->masterVolume = std::clamp(Config::getFloat("Audio.master_volume", m_state->masterVolume), 0.0f, 1.0f);
        for (const auto& e : kAudioTypeMeta) {
            m_state->typeVolumes[e.type]    = std::clamp(Config::getFloat(e.volumeKey, m_state->typeVolumes[e.type]), 0.0f, 1.0f);
            m_state->typeMuteStates[e.type] = Config::getBool(e.muteKey, m_state->typeMuteStates[e.type]);
        }
    }

    m_state->ready = true;
    Logger::logInfo("Audio system initialized");
}

OpenALAudioSystem::~OpenALAudioSystem() {
    if (!m_state) return;
    if (m_state->ready) {
        {
            std::lock_guard<std::mutex> lock(m_state->mutex);
            for (auto& pair : m_state->activeSounds) {
                alSourceStop(pair.second.source);
                alDeleteSources(1, &pair.second.source);
            }
            m_state->activeSounds.clear();
            for (auto& pair : m_state->soundLibrary) {
                alDeleteBuffers(1, &pair.second.buffer);
            }
            m_state->soundLibrary.clear();
        }
        alcMakeContextCurrent(nullptr);
        if (m_state->context) {
            alcDestroyContext(m_state->context);
            m_state->context = nullptr;
        }
        if (m_state->device) {
            alcCloseDevice(m_state->device);
            m_state->device = nullptr;
        }
        m_state->ready = false;
        Logger::logInfo("Audio system shut down");
    }
}

bool OpenALAudioSystem::isReady() const {
    return m_state && m_state->ready;
}

// --- sound load / unload ---------------------------------------------------

bool OpenALAudioSystem::loadSound(const std::string& name, const std::string& filePath, AudioType type) {
    if (!isReady()) {
        Logger::logError("Cannot load sound, audio system not initialized");
        return false;
    }
    std::lock_guard<std::mutex> lock(m_state->mutex);
    if (m_state->soundLibrary.find(name) != m_state->soundLibrary.end()) {
        Logger::logWarning("Sound already loaded: " + name);
        return true;
    }
    ALuint buffer = 0;
    alGenBuffers(1, &buffer);
    if (!checkALError("generating buffer")) return false;
    if (!loadAudioFile(filePath, buffer)) {
        alDeleteBuffers(1, &buffer);
        Logger::logError("Failed to load audio file: " + filePath);
        return false;
    }
    m_state->soundLibrary[name] = State::Sound{buffer, type, filePath};
    Logger::logInfo("Sound loaded: " + name);
    return true;
}

void OpenALAudioSystem::unloadSound(const std::string& name) {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->soundLibrary.find(name);
    if (it == m_state->soundLibrary.end()) {
        Logger::logWarning("Cannot unload sound, not found: " + name);
        return;
    }
    std::vector<unsigned int> toRemove;
    for (auto& pair : m_state->activeSounds) {
        if (pair.second.name == name) {
            alSourceStop(pair.second.source);
            alDeleteSources(1, &pair.second.source);
            toRemove.push_back(pair.first);
        }
    }
    for (unsigned int id : toRemove) {
        m_state->activeSounds.erase(id);
    }
    alDeleteBuffers(1, &it->second.buffer);
    m_state->soundLibrary.erase(it);
    Logger::logInfo("Sound unloaded: " + name);
}

void OpenALAudioSystem::unloadAllSounds() {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    for (auto& pair : m_state->activeSounds) {
        alSourceStop(pair.second.source);
        alDeleteSources(1, &pair.second.source);
    }
    m_state->activeSounds.clear();
    for (auto& pair : m_state->soundLibrary) {
        alDeleteBuffers(1, &pair.second.buffer);
    }
    m_state->soundLibrary.clear();
    Logger::logInfo("All sounds unloaded");
}

// --- playback --------------------------------------------------------------

unsigned int OpenALAudioSystem::playSound(const std::string& name, bool loop, float volume) {
    if (!isReady()) {
        Logger::logError("Cannot play sound, audio system not initialized");
        return 0;
    }
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->soundLibrary.find(name);
    if (it == m_state->soundLibrary.end()) {
        Logger::logWarning("Sound not found: " + name);
        return 0;
    }
    if (m_state->typeMuteStates[it->second.type]) {
        Logger::logInfo("Sound type is muted, not playing: " + name);
        return 0;
    }
    ALuint source = 0;
    alGenSources(1, &source);
    if (!checkALError("generating source")) return 0;
    alSourcei(source, AL_BUFFER, it->second.buffer);
    alSourcef(source, AL_GAIN, volume * m_state->masterVolume * m_state->typeVolumes[it->second.type]);
    alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    alSourcePlay(source);
    if (!checkALError("playing sound")) {
        alDeleteSources(1, &source);
        return 0;
    }
    const unsigned int soundId = m_state->nextSoundId++;
    m_state->activeSounds[soundId] = State::SoundInstance{
        source, name, true, false, loop, volume, it->second.type
    };
    return soundId;
}

void OpenALAudioSystem::stopSound(unsigned int soundId) {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->activeSounds.find(soundId);
    if (it == m_state->activeSounds.end()) return;
    alSourceStop(it->second.source);
    alDeleteSources(1, &it->second.source);
    m_state->activeSounds.erase(it);
}

void OpenALAudioSystem::pauseSound(unsigned int soundId) {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->activeSounds.find(soundId);
    if (it == m_state->activeSounds.end() || it->second.isPaused) return;
    alSourcePause(it->second.source);
    it->second.isPaused = true;
    it->second.isPlaying = false;
}

void OpenALAudioSystem::resumeSound(unsigned int soundId) {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->activeSounds.find(soundId);
    if (it == m_state->activeSounds.end() || !it->second.isPaused) return;
    alSourcePlay(it->second.source);
    it->second.isPaused = false;
    it->second.isPlaying = true;
}

// --- volume + mute ---------------------------------------------------------

void OpenALAudioSystem::setMasterVolume(float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->masterVolume = volume;
        for (auto& pair : m_state->activeSounds) {
            float finalVolume = pair.second.volume * volume * m_state->typeVolumes[pair.second.type];
            alSourcef(pair.second.source, AL_GAIN, finalVolume);
        }
    }
    Config::set("Audio.master_volume", volume);
}

float OpenALAudioSystem::getMasterVolume() const {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    return m_state->masterVolume;
}

void OpenALAudioSystem::setTypeVolume(AudioType type, float volume) {
    volume = std::clamp(volume, 0.0f, 1.0f);
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->typeVolumes[type] = volume;
        for (auto& pair : m_state->activeSounds) {
            if (pair.second.type == type) {
                float finalVolume = pair.second.volume * m_state->masterVolume * volume;
                alSourcef(pair.second.source, AL_GAIN, finalVolume);
            }
        }
    }
    Config::set(metaFor(type).volumeKey, volume);
}

float OpenALAudioSystem::getTypeVolume(AudioType type) const {
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->typeVolumes.find(type);
    return it != m_state->typeVolumes.end() ? it->second : 1.0f;
}

void OpenALAudioSystem::setTypeMuted(AudioType type, bool muted) {
    if (!isReady()) {
        Logger::logWarning("Cannot set mute state, audio system not initialized");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_state->mutex);
        m_state->typeMuteStates[type] = muted;
        for (auto& pair : m_state->activeSounds) {
            if (pair.second.type != type) continue;
            if (muted) {
                if (pair.second.isPlaying && !pair.second.isPaused) {
                    alSourceStop(pair.second.source);
                    pair.second.isPlaying = false;
                }
            } else if (!pair.second.isPaused) {
                alSourcePlay(pair.second.source);
                pair.second.isPlaying = true;
            }
        }
    }
    const auto& meta = metaFor(type);
    Config::set<bool>(meta.muteKey, muted);
    Logger::logInfo(std::string("Audio type ") + meta.name + (muted ? " muted" : " unmuted"));
}

bool OpenALAudioSystem::isTypeMuted(AudioType type) const {
    if (!isReady()) return false;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->typeMuteStates.find(type);
    return it != m_state->typeMuteStates.end() && it->second;
}

void OpenALAudioSystem::muteAll() {
    if (!isReady()) return;
    for (const auto& e : kAudioTypeMeta) setTypeMuted(e.type, true);
    Logger::logInfo("All audio types muted");
}

void OpenALAudioSystem::unmuteAll() {
    if (!isReady()) return;
    for (const auto& e : kAudioTypeMeta) setTypeMuted(e.type, false);
    Logger::logInfo("All audio types unmuted");
}

// --- 3D positioning --------------------------------------------------------

void OpenALAudioSystem::setListenerPosition(const float* position) {
    if (!isReady()) return;
    alListener3f(AL_POSITION, position[0], position[1], position[2]);
}

void OpenALAudioSystem::setListenerOrientation(const float* forward, const float* up) {
    if (!isReady()) return;
    float orientation[6] = {
        forward[0], forward[1], forward[2],
        up[0],      up[1],      up[2],
    };
    alListenerfv(AL_ORIENTATION, orientation);
}

void OpenALAudioSystem::setSoundPosition(unsigned int soundId, const float* position) {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    auto it = m_state->activeSounds.find(soundId);
    if (it == m_state->activeSounds.end()) return;
    alSource3f(it->second.source, AL_POSITION, position[0], position[1], position[2]);
}

// --- per-frame update ------------------------------------------------------

void OpenALAudioSystem::update() {
    if (!isReady()) return;
    std::lock_guard<std::mutex> lock(m_state->mutex);
    std::vector<unsigned int> toRemove;
    for (auto& pair : m_state->activeSounds) {
        if (pair.second.isLooping) continue;
        ALint state = 0;
        alGetSourcei(pair.second.source, AL_SOURCE_STATE, &state);
        if (state == AL_STOPPED) {
            alDeleteSources(1, &pair.second.source);
            toRemove.push_back(pair.first);
        }
    }
    for (unsigned int id : toRemove) {
        m_state->activeSounds.erase(id);
    }
}

} // namespace Sylva
