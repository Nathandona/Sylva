#pragma once

#include "Camera.h"
#include "CameraController.h"
#include "../core/Logger.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <string>
#include <vector>
#include <fstream>

namespace Sylva {

// Forward declarations
class Renderer;

// Class to implement camera debugging functionality as described in the plan
class CameraDebug {
public:
    CameraDebug(Camera* camera, CameraController* cameraController, Renderer* renderer);
    ~CameraDebug();

    // Enable or disable debugging
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }
    
    // Update and log camera data
    void Update(float deltaTime);
    
    // Draw debug visualizations
    void DrawDebugVisualizations();
    
    // Set orbit visualization properties
    void SetShowOrbitPath(bool show) { m_ShowOrbitPath = show; }
    void SetShowDebugLines(bool show) { m_ShowDebugLines = show; }
    
    // Log specific camera properties
    void LogCameraProperties() const;
    
    // Start/Stop recording camera position over time
    void StartRecording();
    void StopRecording();
    
    // Set log interval (in seconds)
    void SetLogInterval(float interval) { m_LogInterval = interval; }
    
    // Analysis helpers
    void AnalyzeLoggedData();
    
private:
    // Log camera position and orientation
    void LogCameraPositionAndOrientation();
    
    // Calculate and log the delta between camera and player
    void LogCameraPlayerDelta();
    
    // Draw orbit path visualization
    void DrawOrbitPathVisualization();
    
    // Draw debug lines
    void DrawDebugLines();
    
    // Non-owning pointers to related components
    Camera* m_Camera;
    CameraController* m_CameraController;
    Renderer* m_Renderer;
    
    // Debug options
    bool m_Enabled;
    bool m_ShowOrbitPath;
    bool m_ShowDebugLines;
    
    // Recording state
    bool m_IsRecording;
    float m_LogTimer;
    float m_LogInterval;
    std::ofstream m_LogFile;
    
    // Previous values for delta calculation
    glm::vec3 m_PreviousCameraPosition;
    glm::vec3 m_PreviousPlayerPosition;
    
    // History for debug visualization
    struct PositionRecord {
        glm::vec3 cameraPosition;
        glm::vec3 playerPosition;
        float timestamp;
    };
    std::vector<PositionRecord> m_PositionHistory;
    
    // Constants
    static constexpr float DEFAULT_LOG_INTERVAL = 0.2f; // Log 5 times per second by default
    static constexpr size_t MAX_HISTORY_SIZE = 300;     // About 60 seconds at 5 Hz
    static constexpr int ORBIT_SEGMENTS = 36;           // Number of segments to use when visualizing the orbit path
};

} // namespace Sylva 