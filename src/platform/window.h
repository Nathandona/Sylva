#pragma once

#include <string>
#include <functional>

// Forward declarations
struct GLFWwindow;

namespace Sylva {

/**
 * @brief Window management class that abstracts GLFW functionality
 */
class Window {
public:
    /**
     * @brief Default constructor
     */
    Window() = default;

    /**
     * @brief Construct a new Window with given dimensions and title
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param title Window title
     */
    Window(int width, int height, const std::string& title);

    /**
     * @brief Destructor that ensures GLFW cleanup
     */
    ~Window();

    // Delete copy constructor and assignment operator
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    /**
     * @brief Initializes GLFW and sets up error callbacks
     * @return True if initialization succeeded, false otherwise
     */
    static bool initialize();

    /**
     * @brief Creates the window with the specified parameters
     * @param width Window width in pixels
     * @param height Window height in pixels
     * @param title Window title
     * @param fullscreen Whether to create a fullscreen window
     * @return True if window creation succeeded, false otherwise
     */
    bool create(int width, int height, const std::string& title, bool fullscreen = false);

    /**
     * @brief Terminates the window and GLFW
     */
    void shutdown();

    /**
     * @brief Checks if the window should close
     * @return True if the window should close, false otherwise
     */
    bool shouldClose() const;

    /**
     * @brief Sets the window close flag
     * @param shouldClose True to mark window for closing, false otherwise
     */
    void setShouldClose(bool shouldClose);

    /**
     * @brief Swaps the front and back buffers
     */
    void swapBuffers();

    /**
     * @brief Processes pending window events
     */
    void pollEvents();

    /**
     * @brief Gets the underlying GLFW window handle
     * @return Pointer to the GLFW window
     */
    GLFWwindow* getGLFWwindow() const;

    /**
     * @brief Gets the window width
     * @return Window width in pixels
     */
    int getWidth() const;

    /**
     * @brief Gets the window height
     * @return Window height in pixels
     */
    int getHeight() const;

    /**
     * @brief Sets the window title
     * @param title New window title
     */
    void setTitle(const std::string& title);

    /**
     * @brief Sets the vertical sync
     * @param enabled True to enable vsync, false to disable
     */
    void setVSync(bool enabled);

    /**
     * @brief Calculates the time between frames for smooth animations
     * @return Delta time in seconds
     */
    float getDeltaTime() const;

    /**
     * @brief Sets up window resize callback
     * @param callback Function to call when window is resized
     */
    void setResizeCallback(std::function<void(int, int)> callback);

    /**
     * @brief Gets the window resize callback
     * @return Resize callback function
     */
    std::function<void(int, int)> getResizeCallback() const;

private:
    /**
     * @brief Sets up GLFW callbacks for errors, key events, mouse events, etc.
     */
    void setupCallbacks();

    GLFWwindow* m_window = nullptr;                 ///< GLFW window handle
    int m_width = 0;                                ///< Window width
    int m_height = 0;                               ///< Window height
    std::string m_title;                            ///< Window title
    bool m_fullscreen = false;                      ///< Fullscreen flag
    float m_lastFrameTime = 0.0f;                   ///< Time of the last frame
    float m_deltaTime = 0.0f;                       ///< Time between frames
    std::function<void(int, int)> m_resizeCallback; ///< Window resize callback
    static bool s_glfwInitialized;                  ///< GLFW initialization flag
};

} // namespace Sylva