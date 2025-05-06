#include "Platform.h"
#include <iostream>
// Include stb_image.h for loading the icon
#include "../../vendor/stb/stb_image.h"

namespace Sylva {

// Error callback for GLFW
static void GLFWErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
}

// Static method to handle scroll callback
void Platform::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    // Get the Platform instance from the user pointer
    Platform* platform = static_cast<Platform*>(glfwGetWindowUserPointer(window));
    if (platform) {
        platform->m_ScrollYOffset = static_cast<float>(yoffset);
    }
}

// Static method to handle cursor position callback
void Platform::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    Platform* platform = static_cast<Platform*>(glfwGetWindowUserPointer(window));
    if (platform) {
        // Calculate delta from last position
        platform->m_MouseDeltaX = xpos - platform->m_LastMouseX;
        platform->m_MouseDeltaY = ypos - platform->m_LastMouseY;
        
        // Update last position
        platform->m_LastMouseX = xpos;
        platform->m_LastMouseY = ypos;
    }
}

Platform::Platform()
    : m_Window(nullptr)
    , m_Width(0)
    , m_Height(0)
    , m_LastMouseX(0.0)
    , m_LastMouseY(0.0)
    , m_MouseDeltaX(0.0)
    , m_MouseDeltaY(0.0)
    , m_ScrollYOffset(0.0f)
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
    
    // Store this instance in window user pointer for callbacks
    glfwSetWindowUserPointer(m_Window, this);
    
    // Set up callbacks
    glfwSetScrollCallback(m_Window, ScrollCallback);
    glfwSetCursorPosCallback(m_Window, CursorPosCallback);
    
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

void Platform::GetMouseDelta(double& dx, double& dy) const {
    dx = m_MouseDeltaX;
    dy = m_MouseDeltaY;
}

bool Platform::SetWindowIcon(const char* iconPath) {
    // Load the image using stb_image
    int width, height, channels;
    unsigned char* pixels = stbi_load(iconPath, &width, &height, &channels, 4); // Force RGBA
    
    if (!pixels) {
        std::cerr << "Failed to load icon: " << iconPath << std::endl;
        std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
        return false;
    }
    
    // Create a GLFW image structure
    GLFWimage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    
    // Set the window icon
    glfwSetWindowIcon(m_Window, 1, &image);
    
    // Free the image data
    stbi_image_free(pixels);
    
    return true;
}

float Platform::GetMouseScrollOffset() {
    float offset = m_ScrollYOffset;
    m_ScrollYOffset = 0.0f; // Reset after reading
    return offset;
}

} 