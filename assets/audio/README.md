# Audio Assets

This directory contains all audio files used in the Sylva engine. Audio files are organized into the following categories:

## Directories

- **music/**: Background music tracks, typically longer and looped during gameplay.
- **sfx/**: Sound effects for gameplay actions, UI interactions, etc.
- **ambient/**: Environmental sounds and ambient audio loops.
- **voice/**: Voice acting, character dialogue, and narration.

## Supported Formats

The audio system supports the following file formats:
- WAV (.wav) - Uncompressed, high quality, larger file size
- OGG Vorbis (.ogg) - Compressed, good quality, smaller file size

## Naming Conventions

Audio files should follow this naming convention:
- Use lowercase with underscores
- Prefix with category if not in category folder: `ui_click.wav`, `amb_forest.ogg`
- Include variations with numbered suffixes: `footstep_01.wav`, `footstep_02.wav`

## Usage in Code

Load and play audio files using the AudioSystem:

```cpp
// Load sound (once, typically at initialization)
AudioSystem::loadSound("click", "assets/audio/sfx/ui_click.wav", AudioType::SOUND_EFFECT);

// Play sound (returns sound ID for controlling playback)
unsigned int soundId = AudioSystem::playSound("click");

// Other controls
AudioSystem::pauseSound(soundId);
AudioSystem::resumeSound(soundId);
AudioSystem::stopSound(soundId);
```

## Volume Control

Global volume settings can be adjusted in `config/default_config.ini` or at runtime:

```cpp
AudioSystem::setMasterVolume(0.8f);  // 80% master volume
AudioSystem::setTypeVolume(AudioType::MUSIC, 0.6f);  // 60% music volume
``` 