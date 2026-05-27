#pragma once

#include <string>
#include <unordered_map>

namespace Sylva {

enum class AudioType {
    SOUND_EFFECT,  // Short sounds played once or occasionally
    MUSIC,         // Background music tracks, typically looped
    AMBIENT,       // Environmental sounds, typically looped
    VOICE,         // Voice/dialogue audio
};

class AudioSystem {
public:
    static bool initialize();
    static void shutdown();
    
    // Sound loading and management
    static bool loadSound(const std::string& name, const std::string& filePath, AudioType type);
    static void unloadSound(const std::string& name);
    static void unloadAllSounds();
    
    // Playback controls
    static unsigned int playSound(const std::string& name, bool loop = false, float volume = 1.0f);
    static void stopSound(unsigned int soundId);
    static void pauseSound(unsigned int soundId);
    static void resumeSound(unsigned int soundId);
    
    // Volume controls
    static void setMasterVolume(float volume); // 0.0 to 1.0
    static float getMasterVolume();
    static void setTypeVolume(AudioType type, float volume);
    static float getTypeVolume(AudioType type);
    
    // Mute controls
    static void setTypeMuted(AudioType type, bool muted);
    static bool isTypeMuted(AudioType type);
    static void muteAll();
    static void unmuteAll();
    
    // 3D audio positioning
    static void setListenerPosition(const float* position);
    static void setListenerOrientation(const float* forward, const float* up);
    static void setSoundPosition(unsigned int soundId, const float* position);
    
    // System status
    static bool isInitialized();
    static void update(); // Call each frame to update audio system
    
private:
    // Implementation details will be in the cpp file
    static bool initialized;
    static float masterVolume;
    static std::unordered_map<AudioType, float> typeVolumes;
    static std::unordered_map<AudioType, bool> typeMuteStates;
    
    // Prevent instantiation - this is a static utility class
    AudioSystem() = delete;
    ~AudioSystem() = delete;
    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;
};

} // namespace Sylva 