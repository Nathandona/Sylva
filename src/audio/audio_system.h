#pragma once

#include <string>
#include <memory>

namespace Sylva {

enum class AudioType {
    SOUND_EFFECT,
    MUSIC,
    AMBIENT,
    VOICE,
};

/**
 * @brief Audio backend interface.
 *
 * Sylva ships one production implementation (OpenALAudioSystem). Tests can
 * substitute a fake derived class without needing an OpenAL device.
 *
 * All call-sites in the engine receive an IAudioSystem reference — never
 * the concrete class — so the audio dependency is explicit and replaceable.
 */
class IAudioSystem {
public:
    virtual ~IAudioSystem() = default;

    // Sound loading / management.
    virtual bool loadSound(const std::string& name, const std::string& filePath, AudioType type) = 0;
    virtual void unloadSound(const std::string& name) = 0;
    virtual void unloadAllSounds() = 0;

    // Playback controls. playSound returns a non-zero handle on success.
    virtual unsigned int playSound(const std::string& name, bool loop = false, float volume = 1.0f) = 0;
    virtual void stopSound(unsigned int soundId) = 0;
    virtual void pauseSound(unsigned int soundId) = 0;
    virtual void resumeSound(unsigned int soundId) = 0;

    // Volume controls.
    virtual void setMasterVolume(float volume) = 0;
    virtual float getMasterVolume() const = 0;
    virtual void setTypeVolume(AudioType type, float volume) = 0;
    virtual float getTypeVolume(AudioType type) const = 0;

    // Mute controls.
    virtual void setTypeMuted(AudioType type, bool muted) = 0;
    virtual bool isTypeMuted(AudioType type) const = 0;
    virtual void muteAll() = 0;
    virtual void unmuteAll() = 0;

    // 3D audio positioning.
    virtual void setListenerPosition(const float* position) = 0;
    virtual void setListenerOrientation(const float* forward, const float* up) = 0;
    virtual void setSoundPosition(unsigned int soundId, const float* position) = 0;

    // Per-frame tick — reclaims sources for sounds that finished playing.
    virtual void update() = 0;

    // True if the backend successfully initialized (e.g. device opened).
    virtual bool isReady() const = 0;
};

/**
 * @brief OpenAL-backed implementation of IAudioSystem.
 *
 * Constructor opens the default OpenAL device + context and reads volume /
 * mute settings from Config. Destructor closes the device. Holds its state
 * in a pImpl so this header doesn't pull in <AL/al.h>.
 */
class OpenALAudioSystem : public IAudioSystem {
public:
    OpenALAudioSystem();
    ~OpenALAudioSystem() override;

    OpenALAudioSystem(const OpenALAudioSystem&) = delete;
    OpenALAudioSystem& operator=(const OpenALAudioSystem&) = delete;

    bool loadSound(const std::string& name, const std::string& filePath, AudioType type) override;
    void unloadSound(const std::string& name) override;
    void unloadAllSounds() override;

    unsigned int playSound(const std::string& name, bool loop = false, float volume = 1.0f) override;
    void stopSound(unsigned int soundId) override;
    void pauseSound(unsigned int soundId) override;
    void resumeSound(unsigned int soundId) override;

    void setMasterVolume(float volume) override;
    float getMasterVolume() const override;
    void setTypeVolume(AudioType type, float volume) override;
    float getTypeVolume(AudioType type) const override;

    void setTypeMuted(AudioType type, bool muted) override;
    bool isTypeMuted(AudioType type) const override;
    void muteAll() override;
    void unmuteAll() override;

    void setListenerPosition(const float* position) override;
    void setListenerOrientation(const float* forward, const float* up) override;
    void setSoundPosition(unsigned int soundId, const float* position) override;

    void update() override;
    bool isReady() const override;

private:
    struct State;
    std::unique_ptr<State> m_state;
};

} // namespace Sylva
