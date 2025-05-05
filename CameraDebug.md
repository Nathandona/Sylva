# Camera Debugging System for Sylva Engine

This document explains how to use the Camera Debugging system implemented in the Sylva Engine to help debug and troubleshoot third-person camera behavior.

## Overview

The Camera Debugging system provides:

1. Visual debugging of the camera's orbit path, position, and orientation
2. Logging of camera properties and movement
3. Recording of camera and player positions over time
4. Analysis tools to identify anomalies in camera behavior
5. Configuration options through `config.ini`

## Keyboard Controls

The following keyboard shortcuts can be used to control the camera debugging system:

| Key | Function |
|-----|----------|
| `F1` | Toggle camera debugging (visualization and logging) |
| `F2` | Log current camera properties to console |
| `F3` | Start/stop recording camera positions to a CSV file |
| `F4` | Analyze and report on logged camera data |
| `C`  | Cycle between camera modes (Third-Person Orbit, Fixed Follow, Free) |

## Configuration

Camera debugging can be configured in the `config.ini` file:

```ini
# Camera Debug Options
enableCameraDebug = false    # Enable debug visualization on startup
logCameraPositions = false   # Start recording positions on startup
cameraDebugLogInterval = 0.2 # How often to log positions (in seconds)
showOrbitPath = true         # Show the orbit path visualization
showDebugLines = true        # Show debug lines connecting camera and player
```

## Visual Debugging

When camera debugging is enabled, you'll see:

1. **Orbit Path** (green circle): The expected circular path the camera should follow around the player
2. **Player-to-Camera Line** (red): Line connecting the player to the actual camera position
3. **Look-at Line** (yellow): Line connecting the player's look-at point to the camera
4. **Coordinate Axes**: Both at the player position and camera position
5. **Ideal Camera Position** (yellow sphere): Where the camera would be without collision detection
6. **Movement History**: Cyan lines showing the recent camera movement path

## Log Files

When recording is active, the system creates a CSV file named `camera_debug_YYYYMMDD_HHMMSS.csv` with the following data:

- Timestamp
- Camera position (X,Y,Z)
- Player position (X,Y,Z)
- Camera orientation (Yaw, Pitch)
- Camera-Player distance

These files can be loaded into spreadsheets or visualization tools for further analysis.

## Analysis

When you press `F4`, the system analyzes the recorded data and reports:

- Min/Max/Average camera-player distance
- Standard deviation of the distance
- Range of movement on each axis
- Detects any anomalies (unexpected jumps, irregular distances)

## Implementation

The camera debugging system consists of:

- `CameraDebug`: Main class containing logging and visualization logic
- `DebugDrawing`: Utility class for rendering debug visuals
- `Core`: Integration with the engine and user input handling

## Troubleshooting Common Issues

When using the debugging system, look for these common issues:

1. **Distance Anomalies**: If the camera-player distance varies significantly, check the orbit calculation or collision detection.
2. **Position Jumps**: Sudden jumps in camera position may indicate issues with smoothing or input processing.
3. **Coordinate System Issues**: If the orbit path appears to be in the wrong plane, check the coordinate system assumptions.
4. **Orbit Calculation**: If the camera is not following the orbit path correctly, check the trigonometry in the position calculation.

## Extending the System

To extend this debugging system:

1. Add new visualization options to `CameraDebug.h/.cpp`
2. Implement additional analysis in `AnalyzeLoggedData()`
3. Add new configuration options to `config.ini`

## Credits

This camera debugging system was implemented following the plan outlined in `Plan.md` to provide effective tools for diagnosing camera behavior issues in the Sylva Engine. 