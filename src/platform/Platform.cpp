#include "Platform.h"
#include <iostream>
// Include stb_image.h for loading the icon
#include "../renderer/stb_image.h"

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

} 