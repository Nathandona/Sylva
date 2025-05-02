#pragma once

#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Sylva {

class Platform {
public:
    Platform();
    ~Platform();
    
    // Initialize window and OpenGL context
    bool Initialize(const std::string& title, int width, int height);
    
    // Process window events and input
    void PollEvents();
    
    // Swap framebuffers
    void SwapBuffers();
    
    // Get time in seconds (for delta time calculation)
    float GetTime() const;
    
    // Check if window should close
    bool WindowShouldClose() const;
    
    // Clean up
    void Shutdown();
    
    // Window information
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    
    // Input state
    bool IsKeyPressed(int keyCode) const;
    bool IsMouseButtonPressed(int button) const;
    void GetMousePosition(double& x, double& y) const;
    
    // New function to set the window icon
    bool SetWindowIcon(const char* iconPath);
    
    // Get the GLFW window (for renderer to access)
    GLFWwindow* GetNativeWindow() const { return m_Window; }

private:
    GLFWwindow* m_Window;
    std::string m_Title;
    int m_Width;
    int m_Height;
};

} 