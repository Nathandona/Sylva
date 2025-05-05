#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "Shader.h"

namespace Sylva {

// Class to handle debug drawing utilities like lines, circles, etc.
class DebugDrawing {
public:
    DebugDrawing();
    ~DebugDrawing();
    
    bool Initialize();
    void Shutdown();
    
    // Draw a debug line from start to end with specified color
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    
    // Draw a debug circle with specified center, radius, color, and number of segments
    void DrawCircle(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f), int segments = 36, const glm::vec3& normal = glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Draw a debug sphere with specified center, radius, color, and number of segments
    void DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f), int segments = 36);
    
    // Draw a debug box with specified center, size, and color
    void DrawBox(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f));
    
    // Draw coordinate axes at specified position with specified scale
    void DrawCoordinateAxes(const glm::vec3& position, float scale = 1.0f);
    
    // Render all debug primitives
    void Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
    // Clear all debug primitives
    void Clear();
    
private:
    // Line vertex data
    struct LineVertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    
    // OpenGL handles
    unsigned int m_LineVAO;
    unsigned int m_LineVBO;
    
    // Shader for drawing debug primitives
    std::shared_ptr<Shader> m_DebugShader;
    
    // Storage for lines to be drawn
    std::vector<LineVertex> m_LineVertices;
    
    // Flag to track initialization state
    bool m_Initialized;
};

} // namespace Sylva 