#pragma once

#include "core/types.h"
#include <memory>

namespace Sylva {

class Shader;

/**
 * @brief Screen-space UI overlay (crosshair, future HUD elements).
 *
 * Owns its OpenGL resources (VAO/VBO for crosshair geometry, shader).
 * Engine constructs one per window/context. Destructor releases GL state
 * — call it before the GL context is torn down.
 */
class UISystem {
public:
    /**
     * @brief Initializes the UI system at the given window dimensions.
     *        Loads the UI shader and uploads the crosshair geometry.
     */
    UISystem(int windowWidth, int windowHeight);
    ~UISystem();

    UISystem(const UISystem&) = delete;
    UISystem& operator=(const UISystem&) = delete;

    void resize(int windowWidth, int windowHeight);

    void renderCrosshair();

    void setCrosshairSize(float size);
    void setCrosshairColor(const Vec4& color);
    void setCrosshairThickness(float thickness);
    void setParams(const UIParams& params);

    bool isReady() const;

private:
    void updateCrosshairGeometry();

    UIParams m_params{};
    int m_windowWidth = 1280;
    int m_windowHeight = 720;
    unsigned int m_crosshairVAO = 0;
    unsigned int m_crosshairVBO = 0;
    std::unique_ptr<Shader> m_uiShader;
    bool m_ready = false;
};

} // namespace Sylva
