#include "platform/window.h"
#include "core/logger.h"
#include "core/config.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

namespace Sylva {

// Initialize static member
bool Window::s_glfwInitialized = false;

// GLFW error callback function
static void glfwErrorCallback(int error, const char* description) {
    Logger::logError("GLFW Error (" + std::to_string(error) + "): " + description);
}

// GLFW framebuffer size callback function (for window resizing)
static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Retrieve Window instance from GLFW user pointer
    Window* userWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (userWindow && userWindow->getGLFWwindow() == window) {
        // Update OpenGL viewport
        glViewport(0, 0, width, height);

        // Call user's resize callback if set
        auto callback = userWindow->getResizeCallback();
        if (callback) {
            callback(width, height);
        }
    }
}

Window::Window(int width, int height, const std::string& title) {
    // Initialize GLFW if not already initialized
    if (!s_glfwInitialized) {
        initialize();
    }

    // Create the window
    create(width, height, title);
}

Window::~Window() {
    shutdown();
}

bool Window::initialize() {
    if (s_glfwInitialized) {
        return true; // Already initialized
    }

    // Set GLFW error callback
    glfwSetErrorCallback(glfwErrorCallback);

    // Initialize GLFW
    if (!glfwInit()) {
        Logger::logError("Failed to initialize GLFW");
        return false;
    }

    // Set GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

// MAC OS compatibility
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    Logger::logInfo("GLFW initialized successfully");
    s_glfwInitialized = true;
    return true;
}

bool Window::create(int width, int height, const std::string& title, bool fullscreen) {
    if (m_window) {
        Logger::logWarning("Window already exists. Destroying existing window before creating a new one.");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    m_width = width;
    m_height = height;
    m_title = title;
    m_fullscreen = fullscreen;

    // Create GLFW window
    GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
    m_window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);

    if (!m_window) {
        Logger::logError("Failed to create GLFW window");
        return false;
    }

    // Set this window as the current OpenGL context
    glfwMakeContextCurrent(m_window);

    // Set window user pointer to this instance for callbacks
    glfwSetWindowUserPointer(m_window, this);

    // Initialize GLAD (OpenGL function loader)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        Logger::logError("Failed to initialize GLAD");
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        return false;
    }

    // Set viewport
    glViewport(0, 0, width, height);

    // Set up callbacks
    setupCallbacks();

    // Set vsync based on config
    bool vsync = Config::getBool("Graphics.vsync", true);
    setVSync(vsync);

    // Initialize time tracking
    m_lastFrameTime = static_cast<float>(glfwGetTime());
    m_deltaTime = 0.0f;

    Logger::logInfo("Window created: " + std::to_string(width) + "x" + std::to_string(height) + " - " + title +
                    (fullscreen ? " (Fullscreen)" : ""));

    return true;
}

void Window::shutdown() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        Logger::logInfo("Window destroyed");
    }

    // Only terminate GLFW if we initialized it
    if (s_glfwInitialized) {
        glfwTerminate();
        s_glfwInitialized = false;
        Logger::logInfo("GLFW terminated");
    }
}

bool Window::shouldClose() const {
    return m_window ? glfwWindowShouldClose(m_window) : true;
}

void Window::setShouldClose(bool shouldClose) {
    if (m_window) {
        glfwSetWindowShouldClose(m_window, shouldClose);
    }
}

void Window::swapBuffers() {
    if (m_window) {
        glfwSwapBuffers(m_window);

        // Update delta time
        float currentTime = static_cast<float>(glfwGetTime());
        m_deltaTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;
    }
}

void Window::pollEvents() {
    glfwPollEvents();
}

GLFWwindow* Window::getGLFWwindow() const {
    return m_window;
}

int Window::getWidth() const {
    return m_width;
}

int Window::getHeight() const {
    return m_height;
}

void Window::setTitle(const std::string& title) {
    if (m_window) {
        m_title = title;
        glfwSetWindowTitle(m_window, title.c_str());
    }
}

void Window::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
    Logger::logInfo("VSync " + std::string(enabled ? "enabled" : "disabled"));
}

float Window::getDeltaTime() const {
    return m_deltaTime;
}

void Window::setResizeCallback(std::function<void(int, int)> callback) {
    m_resizeCallback = callback;
}

std::function<void(int, int)> Window::getResizeCallback() const {
    return m_resizeCallback;
}

void Window::setupCallbacks() {
    if (!m_window)
        return;

    // Set framebuffer size callback
    glfwSetFramebufferSizeCallback(m_window, glfwFramebufferSizeCallback);

    // Additional callbacks can be set here:
    // - Key callback
    // - Mouse position callback
    // - Mouse button callback
    // - Scroll callback
    // Depending on your requirements
}

} // namespace Sylva