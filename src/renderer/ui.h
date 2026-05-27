#pragma once

#include "core/types.h"

namespace Sylva {

// Forward declarations
class Camera;

/**
 * @brief UI system
 * 
 * Handles rendering of user interface elements.
 */
namespace UI {

/**
 * @brief Initialize the UI system
 * @param windowWidth The width of the window
 * @param windowHeight The height of the window
 */
void initialize(int windowWidth, int windowHeight);

/**
 * @brief Release UI GL resources. Must be called before GL context is destroyed.
 */
void shutdown();

/**
 * @brief Resize the UI viewport
 * @param windowWidth The new width of the window
 * @param windowHeight The new height of the window
 */
void resize(int windowWidth, int windowHeight);

/**
 * @brief Render the crosshair at the center of the screen
 * @param camera The camera to align the crosshair with
 */
void renderCrosshair(const Camera& camera);

/**
 * @brief Render the HUD (heads-up display)
 */
void renderHUD();

/**
 * @brief Set the crosshair size
 * @param size The size in pixels
 */
void setCrosshairSize(float size);

/**
 * @brief Set the crosshair color
 * @param color The color (RGBA)
 */
void setCrosshairColor(const Vec4& color);

/**
 * @brief Set the crosshair thickness
 * @param thickness The thickness in pixels
 */
void setCrosshairThickness(float thickness);

/**
 * @brief Set the UI parameters
 * @param params The new parameters
 */
void setParams(const UIParams& params);

} // namespace UI

} // namespace Sylva 