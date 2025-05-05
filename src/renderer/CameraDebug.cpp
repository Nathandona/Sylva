#include "CameraDebug.h"
#include "Renderer.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Sylva {

// Helper function to get current timestamp as string
std::string GetTimeStampString() {
    auto now = std::time(nullptr);
    auto tm = std::localtime(&now);
    
    std::stringstream ss;
    ss << std::put_time(tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

CameraDebug::CameraDebug(Camera* camera, CameraController* cameraController, Renderer* renderer)
    : m_Camera(camera)
    , m_CameraController(cameraController)
    , m_Renderer(renderer)
    , m_Enabled(false)
    , m_ShowOrbitPath(true)
    , m_ShowDebugLines(true)
    , m_IsRecording(false)
    , m_LogTimer(0.0f)
    , m_LogInterval(DEFAULT_LOG_INTERVAL)
    , m_PreviousCameraPosition(0.0f)
    , m_PreviousPlayerPosition(0.0f)
{
    if (m_Camera && m_CameraController) {
        m_PreviousCameraPosition = m_Camera->GetPosition();
        
        // Get player position if available
        const Player* player = m_CameraController->GetPlayerTarget();
        if (player) {
            m_PreviousPlayerPosition = player->GetPosition();
        }
    }
    
    // Register with the renderer
    if (m_Renderer) {
        m_Renderer->RegisterCameraDebug(this);
        m_Renderer->SetDebugDrawingEnabled(true);
    }
    
    LOG_INFO("Camera Debug system initialized");
}

CameraDebug::~CameraDebug() {
    if (m_IsRecording) {
        StopRecording();
    }
    
    LOG_INFO("Camera Debug system shutdown");
}

void CameraDebug::Update(float deltaTime) {
    if (!m_Enabled || !m_Camera || !m_CameraController) return;
    
    // Log camera data periodically
    m_LogTimer += deltaTime;
    if (m_LogTimer >= m_LogInterval) {
        m_LogTimer = 0.0f;
        
        LogCameraPositionAndOrientation();
        LogCameraPlayerDelta();
        
        // Store in history for visualization
        const Player* player = m_CameraController->GetPlayerTarget();
        if (player) {
            PositionRecord record;
            record.cameraPosition = m_Camera->GetPosition();
            record.playerPosition = player->GetPosition();
            record.timestamp = static_cast<float>(glfwGetTime()); // Use GLFW time for consistent timing
            
            m_PositionHistory.push_back(record);
            
            // Limit history size
            if (m_PositionHistory.size() > MAX_HISTORY_SIZE) {
                m_PositionHistory.erase(m_PositionHistory.begin());
            }
            
            // If recording to file, write to file as well
            if (m_IsRecording && m_LogFile.is_open()) {
                glm::vec3 deltaPos = record.cameraPosition - record.playerPosition;
                float distance = glm::length(deltaPos);
                
                m_LogFile << record.timestamp << "," 
                          << record.cameraPosition.x << "," 
                          << record.cameraPosition.y << "," 
                          << record.cameraPosition.z << ","
                          << record.playerPosition.x << "," 
                          << record.playerPosition.y << "," 
                          << record.playerPosition.z << ","
                          << m_Camera->GetYaw() << ","
                          << m_Camera->GetPitch() << ","
                          << distance << std::endl;
            }
        }
    }
    
    // Update previous positions for next delta calculation
    m_PreviousCameraPosition = m_Camera->GetPosition();
    
    const Player* player = m_CameraController->GetPlayerTarget();
    if (player) {
        m_PreviousPlayerPosition = player->GetPosition();
    }
}

void CameraDebug::DrawDebugVisualizations() {
    if (!m_Enabled || !m_Camera || !m_CameraController || !m_Renderer) return;
    
    if (m_ShowOrbitPath) {
        DrawOrbitPathVisualization();
    }
    
    if (m_ShowDebugLines) {
        DrawDebugLines();
    }
}

void CameraDebug::LogCameraProperties() const {
    if (!m_Camera || !m_CameraController) return;
    
    const Player* player = m_CameraController->GetPlayerTarget();
    
    LOG_INFO("--- Camera Properties ---");
    LOG_INFO("Camera Position: ", glm::to_string(m_Camera->GetPosition()));
    LOG_INFO("Camera Orientation: Pitch=", m_Camera->GetPitch(), " Yaw=", m_Camera->GetYaw());
    LOG_INFO("Camera Front: ", glm::to_string(m_Camera->GetFront()));
    LOG_INFO("Camera Up: ", glm::to_string(m_Camera->GetUp()));
    LOG_INFO("Camera Right: ", glm::to_string(m_Camera->GetRight()));
    
    if (player) {
        glm::vec3 playerPos = player->GetPosition();
        LOG_INFO("Player Position: ", glm::to_string(playerPos));
        LOG_INFO("Player Yaw: ", player->GetYaw());
        
        glm::vec3 deltaPos = m_Camera->GetPosition() - playerPos;
        LOG_INFO("Camera-Player Delta: ", glm::to_string(deltaPos));
        LOG_INFO("Camera-Player Distance: ", glm::length(deltaPos));
    }
    
    // Log orbit settings
    LOG_INFO("Orbit Distance: ", m_CameraController->GetOrbitDistance());
    LOG_INFO("Min Orbit Distance: ", m_CameraController->GetMinOrbitDistance());
    LOG_INFO("Max Orbit Distance: ", m_CameraController->GetMaxOrbitDistance());
    LOG_INFO("Vertical Offset: ", m_CameraController->GetVerticalOffset());
    LOG_INFO("Look-at Offset: ", m_CameraController->GetLookAtOffset());
    LOG_INFO("Camera Mode: ", static_cast<int>(m_CameraController->GetCameraMode()));
    LOG_INFO("-----------------------");
}

void CameraDebug::StartRecording() {
    if (m_IsRecording) {
        LOG_WARNING("Camera debug recording is already active");
        return;
    }
    
    // Create a log file with timestamp
    std::string filename = "camera_debug_" + GetTimeStampString() + ".csv";
    m_LogFile.open(filename, std::ios::out);
    
    if (!m_LogFile.is_open()) {
        LOG_ERROR("Failed to open camera debug log file: ", filename);
        return;
    }
    
    // Write CSV header
    m_LogFile << "Timestamp,CamX,CamY,CamZ,PlayerX,PlayerY,PlayerZ,Yaw,Pitch,Distance" << std::endl;
    
    m_IsRecording = true;
    m_PositionHistory.clear();
    
    LOG_INFO("Started camera debug recording to ", filename);
}

void CameraDebug::StopRecording() {
    if (!m_IsRecording) {
        LOG_WARNING("No camera debug recording is active");
        return;
    }
    
    m_IsRecording = false;
    
    if (m_LogFile.is_open()) {
        m_LogFile.close();
        LOG_INFO("Stopped camera debug recording");
    }
}

void CameraDebug::AnalyzeLoggedData() {
    if (m_PositionHistory.empty()) {
        LOG_WARNING("No camera debug data available to analyze");
        return;
    }
    
    // Calculate statistics
    float minDistance = std::numeric_limits<float>::max();
    float maxDistance = 0.0f;
    float sumDistance = 0.0f;
    
    glm::vec3 minDelta(0.0f);
    glm::vec3 maxDelta(0.0f);
    
    // Calculate deltas and distances
    std::vector<float> distances;
    distances.reserve(m_PositionHistory.size());
    
    for (const auto& record : m_PositionHistory) {
        glm::vec3 delta = record.cameraPosition - record.playerPosition;
        float distance = glm::length(delta);
        
        minDistance = std::min(minDistance, distance);
        maxDistance = std::max(maxDistance, distance);
        sumDistance += distance;
        
        // Track min/max deltas per component
        minDelta.x = std::min(minDelta.x, delta.x);
        minDelta.y = std::min(minDelta.y, delta.y);
        minDelta.z = std::min(minDelta.z, delta.z);
        
        maxDelta.x = std::max(maxDelta.x, delta.x);
        maxDelta.y = std::max(maxDelta.y, delta.y);
        maxDelta.z = std::max(maxDelta.z, delta.z);
        
        distances.push_back(distance);
    }
    
    float avgDistance = sumDistance / static_cast<float>(m_PositionHistory.size());
    
    // Calculate standard deviation of distance
    float sumSquaredDiff = 0.0f;
    for (float dist : distances) {
        float diff = dist - avgDistance;
        sumSquaredDiff += diff * diff;
    }
    
    float stdDevDistance = std::sqrt(sumSquaredDiff / static_cast<float>(m_PositionHistory.size()));
    
    // Log analysis results
    LOG_INFO("--- Camera Debug Analysis ---");
    LOG_INFO("Data points: ", m_PositionHistory.size());
    LOG_INFO("Time span: ", m_PositionHistory.back().timestamp - m_PositionHistory.front().timestamp, " seconds");
    LOG_INFO("Camera-Player Distance - Min: ", minDistance, " Max: ", maxDistance, " Avg: ", avgDistance, " StdDev: ", stdDevDistance);
    LOG_INFO("Delta X - Min: ", minDelta.x, " Max: ", maxDelta.x, " Range: ", maxDelta.x - minDelta.x);
    LOG_INFO("Delta Y - Min: ", minDelta.y, " Max: ", maxDelta.y, " Range: ", maxDelta.y - minDelta.y);
    LOG_INFO("Delta Z - Min: ", minDelta.z, " Max: ", maxDelta.z, " Range: ", maxDelta.z - minDelta.z);
    
    // Detect anomalies
    LOG_INFO("--- Anomaly Detection ---");
    
    // Check for large distance variations - possible sign of camera jumps
    if (maxDistance / minDistance > 2.0f) {
        LOG_WARNING("ANOMALY: Large variation in camera-player distance detected (ratio > 2.0)");
    }
    
    // Check for standard deviation greater than 10% of average distance
    if (stdDevDistance > 0.1f * avgDistance) {
        LOG_WARNING("ANOMALY: High standard deviation in camera-player distance (>10% of average)");
    }
    
    // Check for expected orbit distance
    if (m_CameraController) {
        float expectedDistance = m_CameraController->GetOrbitDistance();
        if (std::abs(avgDistance - expectedDistance) > 0.5f) {
            LOG_WARNING("ANOMALY: Average camera distance (", avgDistance, 
                      ") differs significantly from configured orbit distance (", expectedDistance, ")");
        }
    }
    
    LOG_INFO("----------------------------");
}

void CameraDebug::LogCameraPositionAndOrientation() {
    if (!m_Camera) return;
    
    glm::vec3 position = m_Camera->GetPosition();
    float yaw = m_Camera->GetYaw();
    float pitch = m_Camera->GetPitch();
    
    LOG_DEBUG("Camera: Pos(", position.x, ",", position.y, ",", position.z, 
             ") Rot(", pitch, ",", yaw, ")");
}

void CameraDebug::LogCameraPlayerDelta() {
    if (!m_Camera || !m_CameraController) return;
    
    const Player* player = m_CameraController->GetPlayerTarget();
    if (!player) return;
    
    glm::vec3 cameraPos = m_Camera->GetPosition();
    glm::vec3 playerPos = player->GetPosition();
    
    glm::vec3 deltaPos = cameraPos - playerPos;
    float distance = glm::length(deltaPos);
    
    // Check if distance is stable (should be constant for orbit camera)
    float expectedDistance = m_CameraController->GetOrbitDistance();
    float distanceDiff = std::abs(distance - expectedDistance);
    
    if (distanceDiff > 0.1f && m_CameraController->GetCameraMode() == CameraController::CameraMode::ThirdPersonOrbit) {
        LOG_WARNING("Camera-Player distance (", distance, 
                  ") differs from expected orbit distance (", expectedDistance, 
                  ") by ", distanceDiff);
    }
    
    // Calculate deltas from previous position
    glm::vec3 cameraDelta = cameraPos - m_PreviousCameraPosition;
    glm::vec3 playerDelta = playerPos - m_PreviousPlayerPosition;
    
    LOG_DEBUG("Delta: Camera-Player(", deltaPos.x, ",", deltaPos.y, ",", deltaPos.z, 
             ") Distance=", distance, 
             " Movement: Camera(", glm::length(cameraDelta), 
             ") Player(", glm::length(playerDelta), ")");
}

void CameraDebug::DrawOrbitPathVisualization() {
    if (!m_Camera || !m_CameraController || !m_Renderer) return;
    
    const Player* player = m_CameraController->GetPlayerTarget();
    if (!player) return;
    
    // Get player position and orbit parameters
    glm::vec3 playerPos = player->GetPosition();
    float orbitDistance = m_CameraController->GetOrbitDistance();
    float verticalOffset = m_CameraController->GetVerticalOffset();
    
    // Calculate orbit center (player position + vertical offset)
    glm::vec3 orbitCenter = playerPos + glm::vec3(0.0f, verticalOffset, 0.0f);
    
    // Draw orbit visualization using Renderer debug drawing methods
    // Draw orbit path as a circle around the player at the current height
    m_Renderer->DrawDebugCircle(orbitCenter, orbitDistance, glm::vec3(0.0f, 1.0f, 0.0f), ORBIT_SEGMENTS);
    
    // Draw a vertical line from player to the orbit center for better visualization
    m_Renderer->DrawDebugLine(playerPos, orbitCenter, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Draw player center and orientation axes
    m_Renderer->DrawDebugCoordinateAxes(playerPos, 1.0f);
    
    // Draw a sphere at the player's position for visualization
    m_Renderer->DrawDebugSphere(playerPos, 0.25f, glm::vec3(0.0f, 0.0f, 1.0f), 8);
    
    // Calculate and visualize the ideal camera position based on player yaw and orbit parameters
    float playerYaw = player->GetYaw();
    float yawRad = glm::radians(playerYaw + 180.0f); // 180 degrees behind player
    
    // Calculate ideal orbit position (without collision, player yaw + 180 degrees = behind player)
    float offsetX = orbitDistance * std::sin(yawRad);
    float offsetZ = orbitDistance * std::cos(yawRad);
    glm::vec3 idealCameraPos = orbitCenter + glm::vec3(offsetX, 0.0f, offsetZ);
    
    // Draw a line from orbit center to ideal camera position
    m_Renderer->DrawDebugLine(orbitCenter, idealCameraPos, glm::vec3(1.0f, 1.0f, 0.0f));
    
    // Draw a sphere at the ideal camera position
    m_Renderer->DrawDebugSphere(idealCameraPos, 0.15f, glm::vec3(1.0f, 1.0f, 0.0f), 8);
    
    // Draw a line from ideal camera position to actual camera position to visualize any deviations
    glm::vec3 actualCameraPos = m_Camera->GetPosition();
    m_Renderer->DrawDebugLine(idealCameraPos, actualCameraPos, glm::vec3(1.0f, 0.5f, 0.0f));
    
    // Calculate and display distance between ideal and actual camera position
    float deviationDistance = glm::distance(idealCameraPos, actualCameraPos);
    if (deviationDistance > 0.1f) {
        LOG_DEBUG("Camera position deviation: ", deviationDistance);
    }
}

void CameraDebug::DrawDebugLines() {
    if (!m_Camera || !m_CameraController || !m_Renderer) return;
    
    const Player* player = m_CameraController->GetPlayerTarget();
    if (!player) return;
    
    // Get positions
    glm::vec3 cameraPos = m_Camera->GetPosition();
    glm::vec3 playerPos = player->GetPosition();
    glm::vec3 playerLookAt = playerPos + glm::vec3(0.0f, m_CameraController->GetLookAtOffset(), 0.0f);
    
    // Draw debug lines using Renderer debug drawing methods
    // Player to look-at point (green)
    m_Renderer->DrawDebugLine(playerPos, playerLookAt, glm::vec3(0.0f, 1.0f, 0.0f)); 
    
    // Look-at point to camera (yellow)
    m_Renderer->DrawDebugLine(playerLookAt, cameraPos, glm::vec3(1.0f, 1.0f, 0.0f)); 
    
    // Player to camera (red)
    m_Renderer->DrawDebugLine(playerPos, cameraPos, glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Draw camera orientation axes
    m_Renderer->DrawDebugCoordinateAxes(cameraPos, 0.5f);
    
    // Draw tracking history if available
    if (m_PositionHistory.size() > 1) {
        // Draw camera path (cyan)
        for (size_t i = 1; i < m_PositionHistory.size(); ++i) {
            m_Renderer->DrawDebugLine(
                m_PositionHistory[i-1].cameraPosition,
                m_PositionHistory[i].cameraPosition,
                glm::vec3(0.0f, 0.8f, 1.0f)
            );
        }
        
        // Draw player path (yellow)
        for (size_t i = 1; i < m_PositionHistory.size(); ++i) {
            m_Renderer->DrawDebugLine(
                m_PositionHistory[i-1].playerPosition,
                m_PositionHistory[i].playerPosition,
                glm::vec3(1.0f, 0.8f, 0.0f)
            );
        }
    }
}

} // namespace Sylva 