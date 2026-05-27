#include "ui.h"
#include "shader.h"
#include "core/logger.h"
#include "core/config.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Sylva {

UISystem::UISystem(int windowWidth, int windowHeight) : m_windowWidth(windowWidth), m_windowHeight(windowHeight) {
    Logger::logDebug("Initializing UI system");

    m_params.crosshairSize = Config::getFloat("UI.crosshair_size", m_params.crosshairSize);
    m_params.crosshairThickness = Config::getFloat("UI.crosshair_thickness", m_params.crosshairThickness);

    try {
        m_uiShader = std::make_unique<Shader>("assets/shaders/ui.vert", "assets/shaders/ui.frag");
    } catch (const std::exception& e) {
        Logger::logError("Failed to load UI shader: " + std::string(e.what()));
        return;
    }

    glGenVertexArrays(1, &m_crosshairVAO);
    glGenBuffers(1, &m_crosshairVBO);
    updateCrosshairGeometry();

    m_ready = true;
    Logger::logInfo("UI system initialized");
}

UISystem::~UISystem() {
    if (m_crosshairVBO != 0) {
        glDeleteBuffers(1, &m_crosshairVBO);
        m_crosshairVBO = 0;
    }
    if (m_crosshairVAO != 0) {
        glDeleteVertexArrays(1, &m_crosshairVAO);
        m_crosshairVAO = 0;
    }
    m_uiShader.reset();
    m_ready = false;
    Logger::logDebug("UI shutdown");
}

bool UISystem::isReady() const {
    return m_ready;
}

void UISystem::updateCrosshairGeometry() const {
    if (m_crosshairVAO == 0 || m_crosshairVBO == 0) {
        Logger::logWarning("Cannot update crosshair geometry, VAO/VBO not initialized");
        return;
    }

    const float halfSize = m_params.crosshairSize / 2.0f;
    const float halfThickness = m_params.crosshairThickness / 2.0f;
    const float cx = m_windowWidth / 2.0f;
    const float cy = m_windowHeight / 2.0f;

    const float vertices[] = {
        // Horizontal bar
        cx - halfSize,
        cy - halfThickness,
        cx + halfSize,
        cy - halfThickness,
        cx + halfSize,
        cy + halfThickness,
        cx - halfSize,
        cy - halfThickness,
        cx + halfSize,
        cy + halfThickness,
        cx - halfSize,
        cy + halfThickness,
        // Vertical bar
        cx - halfThickness,
        cy - halfSize,
        cx + halfThickness,
        cy - halfSize,
        cx + halfThickness,
        cy + halfSize,
        cx - halfThickness,
        cy - halfSize,
        cx + halfThickness,
        cy + halfSize,
        cx - halfThickness,
        cy + halfSize,
    };

    glBindVertexArray(m_crosshairVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_crosshairVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void UISystem::resize(int windowWidth, int windowHeight) {
    m_windowWidth = windowWidth;
    m_windowHeight = windowHeight;
    updateCrosshairGeometry();
    Logger::logDebug("UI resized to " + std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
}

void UISystem::renderCrosshair() {
    if (!m_uiShader || m_crosshairVAO == 0) {
        Logger::logWarning("Cannot render crosshair, UI system not properly initialized");
        return;
    }

    GLboolean depthTestEnabled = GL_FALSE;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_uiShader->use();
    const glm::mat4 projection =
        glm::ortho(0.0f, static_cast<float>(m_windowWidth), 0.0f, static_cast<float>(m_windowHeight));
    m_uiShader->setMat4("projection", projection);
    m_uiShader->setVec4("color",
                        glm::vec4(m_params.crosshairColor.x,
                                  m_params.crosshairColor.y,
                                  m_params.crosshairColor.z,
                                  m_params.crosshairColor.w));

    glBindVertexArray(m_crosshairVAO);
    glDrawArrays(GL_TRIANGLES, 0, 12);
    glBindVertexArray(0);

    if (depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
}

void UISystem::setCrosshairSize(float size) {
    m_params.crosshairSize = size;
    updateCrosshairGeometry();
}

void UISystem::setCrosshairColor(const Vec4& color) {
    m_params.crosshairColor = color;
}

void UISystem::setCrosshairThickness(float thickness) {
    m_params.crosshairThickness = thickness;
    updateCrosshairGeometry();
}

void UISystem::setParams(const UIParams& params) {
    m_params = params;
    updateCrosshairGeometry();
}

} // namespace Sylva
