#include "DebugDrawing.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include "../core/Logger.h"

namespace Sylva {

DebugDrawing::DebugDrawing()
    : m_LineVAO(0)
    , m_LineVBO(0)
    , m_Initialized(false)
{
}

DebugDrawing::~DebugDrawing() {
    Shutdown();
}

bool DebugDrawing::Initialize() {
    if (m_Initialized) {
        LOG_WARNING("DebugDrawing is already initialized");
        return true;
    }
    
    // Create line VAO and VBO
    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    
    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    // Create debug shader for drawing lines
    const char* vertexShaderSrc = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        
        uniform mat4 uViewProjection;
        
        out vec3 vertexColor;
        
        void main() {
            gl_Position = uViewProjection * vec4(aPos, 1.0);
            vertexColor = aColor;
        }
    )";
    
    const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(vertexColor, 1.0);
        }
    )";
    
    // Create shader from source strings
    m_DebugShader = std::make_shared<Shader>();
    // if (!m_DebugShader->CompileFromSource(vertexShaderSrc, fragmentShaderSrc)) {
    //     LOG_ERROR("Failed to compile debug drawing shader");
    //     Shutdown();
    //     return false;
    // }
    
    m_Initialized = true;
    LOG_INFO("DebugDrawing initialized successfully");
    return true;
}

void DebugDrawing::Shutdown() {
    if (!m_Initialized) return;
    
    if (m_LineVAO) {
        glDeleteVertexArrays(1, &m_LineVAO);
        m_LineVAO = 0;
    }
    
    if (m_LineVBO) {
        glDeleteBuffers(1, &m_LineVBO);
        m_LineVBO = 0;
    }
    
    m_DebugShader.reset();
    m_LineVertices.clear();
    
    m_Initialized = false;
    LOG_INFO("DebugDrawing shutdown");
}

void DebugDrawing::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3& color) {
    if (!m_Initialized) {
        LOG_WARNING("DebugDrawing is not initialized");
        return;
    }
    
    // Add start vertex
    LineVertex startVertex;
    startVertex.position = start;
    startVertex.color = color;
    m_LineVertices.push_back(startVertex);
    
    // Add end vertex
    LineVertex endVertex;
    endVertex.position = end;
    endVertex.color = color;
    m_LineVertices.push_back(endVertex);
}

void DebugDrawing::DrawCircle(const glm::vec3& center, float radius, const glm::vec3& color, int segments, const glm::vec3& normal) {
    if (!m_Initialized) {
        LOG_WARNING("DebugDrawing is not initialized");
        return;
    }
    
    // Create a basis for the circle plane
    glm::vec3 u, v;
    
    // Find a vector not parallel to normal
    if (std::abs(normal.x) < 0.707f) {
        u = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normal));
    } else {
        u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    }
    
    // Create second basis vector perpendicular to both normal and u
    v = glm::normalize(glm::cross(normal, u));
    
    // Make sure u and v have the correct length
    u *= radius;
    v *= radius;
    
    // Draw the circle as a series of line segments
    const float angleIncrement = 2.0f * glm::pi<float>() / static_cast<float>(segments);
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = i * angleIncrement;
        float angle2 = (i + 1) * angleIncrement;
        
        glm::vec3 point1 = center + u * cos(angle1) + v * sin(angle1);
        glm::vec3 point2 = center + u * cos(angle2) + v * sin(angle2);
        
        DrawLine(point1, point2, color);
    }
}

void DebugDrawing::DrawSphere(const glm::vec3& center, float radius, const glm::vec3& color, int segments) {
    if (!m_Initialized) {
        LOG_WARNING("DebugDrawing is not initialized");
        return;
    }
    
    // Draw three circles along each major axis
    DrawCircle(center, radius, color, segments, glm::vec3(1.0f, 0.0f, 0.0f)); // YZ plane
    DrawCircle(center, radius, color, segments, glm::vec3(0.0f, 1.0f, 0.0f)); // XZ plane
    DrawCircle(center, radius, color, segments, glm::vec3(0.0f, 0.0f, 1.0f)); // XY plane
}

void DebugDrawing::DrawBox(const glm::vec3& center, const glm::vec3& size, const glm::vec3& color) {
    if (!m_Initialized) {
        LOG_WARNING("DebugDrawing is not initialized");
        return;
    }
    
    // Calculate the corners of the box
    glm::vec3 halfSize = size * 0.5f;
    
    glm::vec3 corners[8] = {
        center + glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z), // 0: left bottom back
        center + glm::vec3( halfSize.x, -halfSize.y, -halfSize.z), // 1: right bottom back
        center + glm::vec3( halfSize.x,  halfSize.y, -halfSize.z), // 2: right top back
        center + glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z), // 3: left top back
        center + glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z), // 4: left bottom front
        center + glm::vec3( halfSize.x, -halfSize.y,  halfSize.z), // 5: right bottom front
        center + glm::vec3( halfSize.x,  halfSize.y,  halfSize.z), // 6: right top front
        center + glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)  // 7: left top front
    };
    
    // Bottom face
    DrawLine(corners[0], corners[1], color);
    DrawLine(corners[1], corners[5], color);
    DrawLine(corners[5], corners[4], color);
    DrawLine(corners[4], corners[0], color);
    
    // Top face
    DrawLine(corners[3], corners[2], color);
    DrawLine(corners[2], corners[6], color);
    DrawLine(corners[6], corners[7], color);
    DrawLine(corners[7], corners[3], color);
    
    // Connecting edges
    DrawLine(corners[0], corners[3], color);
    DrawLine(corners[1], corners[2], color);
    DrawLine(corners[5], corners[6], color);
    DrawLine(corners[4], corners[7], color);
}

void DebugDrawing::DrawCoordinateAxes(const glm::vec3& position, float scale) {
    if (!m_Initialized) {
        LOG_WARNING("DebugDrawing is not initialized");
        return;
    }
    
    // X-axis (red)
    DrawLine(position, position + glm::vec3(scale, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    
    // Y-axis (green)
    DrawLine(position, position + glm::vec3(0.0f, scale, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Z-axis (blue)
    DrawLine(position, position + glm::vec3(0.0f, 0.0f, scale), glm::vec3(0.0f, 0.0f, 1.0f));
}

void DebugDrawing::Render(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!m_Initialized || m_LineVertices.empty()) {
        return;
    }
    
    // Save GL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    // Enable depth test but set it to always pass, so lines are drawn behind geometry but still depth sorted
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    
    m_DebugShader->Use();
    m_DebugShader->SetMat4("uViewProjection", projectionMatrix * viewMatrix);
    
    glBindVertexArray(m_LineVAO);
    
    // Update line VBO data
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, m_LineVertices.size() * sizeof(LineVertex), m_LineVertices.data(), GL_DYNAMIC_DRAW);
    
    // Draw lines
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_LineVertices.size()));
    
    glBindVertexArray(0);
    
    // Restore GL state
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS); // Restore default depth function
    } else {
        glDisable(GL_DEPTH_TEST);
    }
}

void DebugDrawing::Clear() {
    m_LineVertices.clear();
}

} // namespace Sylva 