#include "Platform.h"
#include <iostream>

namespace Sylva {

// Error callback for GLFW
static void GLFWErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

Platform::Platform()
    : m_Window(nullptr)
    , m_Width(0)
    , m_Height(0)
{
}

Platform::~Platform() {
    Shutdown();
}

bool Platform::Initialize(const std::string& title, int width, int height) {
    m_Title = title;
    m_Width = width;
    m_Height = height;
    
    std::cout << "Initializing GLFW..." << std::endl;
    
    // Set up error callback
    glfwSetErrorCallback(GLFWErrorCallback);
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Set OpenGL version and profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Make OpenGL context current
    glfwMakeContextCurrent(m_Window);
    
    // Enable vsync
    glfwSwapInterval(1);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    std::cout << "OpenGL Info:" << std::endl;
    std::cout << "  Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "  Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "  Version: " << glGetString(GL_VERSION) << std::endl;
    
    return true;
}

void Platform::PollEvents() {
    glfwPollEvents();
}

void Platform::SwapBuffers() {
    glfwSwapBuffers(m_Window);
}

float Platform::GetTime() const {
    return (float)glfwGetTime();
}

bool Platform::WindowShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void Platform::Shutdown() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    
    glfwTerminate();
}

bool Platform::IsKeyPressed(int keyCode) const {
    return glfwGetKey(m_Window, keyCode) == GLFW_PRESS;
}

bool Platform::IsMouseButtonPressed(int button) const {
    return glfwGetMouseButton(m_Window, button) == GLFW_PRESS;
}

void Platform::GetMousePosition(double& x, double& y) const {
    glfwGetCursorPos(m_Window, &x, &y);
}

} 