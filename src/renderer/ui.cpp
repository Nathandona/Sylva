#include "ui.h"
#include "camera.h"
#include "shader.h"
#include "core/logger.h"
#include "core/config.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>

namespace Sylva {
namespace UI {

// UI parameters
static UIParams s_params;

// Window dimensions
static int s_windowWidth = 1280;
static int s_windowHeight = 720;

// OpenGL objects
static unsigned int s_crosshairVAO = 0;
static unsigned int s_crosshairVBO = 0;
static Shader* s_uiShader = nullptr;

static void updateCrosshairGeometry() {
    if (s_crosshairVAO == 0 || s_crosshairVBO == 0) {
        Logger::logWarning("Cannot update crosshair geometry, VAO/VBO not initialized");
        return;
    }
    
    // Calculate crosshair dimensions
    float halfSize = s_params.crosshairSize / 2.0f;
    float halfThickness = s_params.crosshairThickness / 2.0f;
    
    // Center of the screen
    float centerX = s_windowWidth / 2.0f;
    float centerY = s_windowHeight / 2.0f;
    
    // Crosshair vertices (2 quads for a plus shape)
    float vertices[] = {
        // Horizontal bar (quad)
        centerX - halfSize, centerY - halfThickness,
        centerX + halfSize, centerY - halfThickness,
        centerX + halfSize, centerY + halfThickness,
        centerX - halfSize, centerY - halfThickness,
        centerX + halfSize, centerY + halfThickness,
        centerX - halfSize, centerY + halfThickness,
        
        // Vertical bar (quad)
        centerX - halfThickness, centerY - halfSize,
        centerX + halfThickness, centerY - halfSize,
        centerX + halfThickness, centerY + halfSize,
        centerX - halfThickness, centerY - halfSize,
        centerX + halfThickness, centerY + halfSize,
        centerX - halfThickness, centerY + halfSize
    };
    
    // Bind VAO and VBO
    glBindVertexArray(s_crosshairVAO);
    
    // Upload data
    glBindBuffer(GL_ARRAY_BUFFER, s_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Set attribute pointers
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void initialize(int windowWidth, int windowHeight) {
    Logger::logDebug("Initializing UI system");
    
    // Set window dimensions
    s_windowWidth = windowWidth;
    s_windowHeight = windowHeight;
    
    // Load UI parameters from config
    s_params.crosshairSize = Config::getFloat("UI.crosshair_size", s_params.crosshairSize);
    s_params.crosshairThickness = Config::getFloat("UI.crosshair_thickness", s_params.crosshairThickness);
    
    // Create shader
    try {
        s_uiShader = new Shader("assets/shaders/ui.vert", "assets/shaders/ui.frag");
    } catch (const std::exception& e) {
        Logger::logError("Failed to load UI shader: " + std::string(e.what()));
        return;
    }
    
    // Create crosshair geometry
    glGenVertexArrays(1, &s_crosshairVAO);
    glGenBuffers(1, &s_crosshairVBO);
    
    // Update crosshair geometry
    updateCrosshairGeometry();
    
    Logger::logInfo("UI system initialized");
}

void resize(int windowWidth, int windowHeight) {
    s_windowWidth = windowWidth;
    s_windowHeight = windowHeight;
    
    // Update crosshair geometry for new window size
    updateCrosshairGeometry();
    
    Logger::logDebug("UI resized to " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
}

void renderCrosshair(const Camera& camera) {
    if (s_uiShader == nullptr || s_crosshairVAO == 0) {
        Logger::logWarning("Cannot render crosshair, UI system not properly initialized");
        return;
    }
    
    // Save OpenGL state
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    
    // Disable depth testing for UI
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for transparent UI elements
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Use UI shader
    s_uiShader->use();
    
    // Set orthographic projection
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(s_windowWidth), 
                                      0.0f, static_cast<float>(s_windowHeight));
    s_uiShader->setMat4("projection", projection);
    
    // Set color
    s_uiShader->setVec4("color", glm::vec4(s_params.crosshairColor.x, 
                                          s_params.crosshairColor.y, 
                                          s_params.crosshairColor.z, 
                                          s_params.crosshairColor.w));
    
    // Render crosshair
    glBindVertexArray(s_crosshairVAO);
    glDrawArrays(GL_TRIANGLES, 0, 12); // 2 quads, 6 vertices each
    glBindVertexArray(0);
    
    // Restore OpenGL state
    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
    
    // Log the render operation
    Logger::logDebug("Rendered crosshair");
}

void renderHUD() {
    // This would involve rendering more UI elements
    // using the same shader and approach as the crosshair
    Logger::logDebug("Rendering HUD");
}

void setCrosshairSize(float size) {
    s_params.crosshairSize = size;
    updateCrosshairGeometry();
}

void setCrosshairColor(const Vec4& color) {
    s_params.crosshairColor = color;
}

void setCrosshairThickness(float thickness) {
    s_params.crosshairThickness = thickness;
    updateCrosshairGeometry();
}

void setParams(const UIParams& params) {
    s_params = params;
    updateCrosshairGeometry();
}

} // namespace UI
} // namespace Sylva 