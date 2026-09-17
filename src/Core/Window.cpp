#include "MagicXEngine/Core/Window.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

namespace MagicXEngine::Core {

Window::Window(uint32_t width, uint32_t height, const std::string& title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // 使用 Vulkan，禁用 GLFW 内建 OpenGL 上下文
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_handle = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                                title.c_str(), nullptr, nullptr);
    if (!m_handle) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_handle, this);
    glfwSetFramebufferSizeCallback(m_handle, [](GLFWwindow* w, int newW, int newH) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        self->m_wasResized = true;
    });
    glfwSetKeyCallback(m_handle, [](GLFWwindow* w, int key, int /*scancode*/, int action, int /*mods*/) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (action == GLFW_PRESS)        self->m_keysDown.insert(key);
        else if (action == GLFW_RELEASE) self->m_keysDown.erase(key);
    });
    glfwSetMouseButtonCallback(m_handle, [](GLFWwindow* w, int button, int action, int /*mods*/) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (action == GLFW_PRESS)        self->m_mouseButtonsDown.insert(button);
        else if (action == GLFW_RELEASE) self->m_mouseButtonsDown.erase(button);
    });
    glfwSetCursorPosCallback(m_handle, [](GLFWwindow* w, double xpos, double ypos) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        self->m_mouseX = xpos;
        self->m_mouseY = ypos;
    });
    glfwSetScrollCallback(m_handle, [](GLFWwindow* w, double /*xoffset*/, double yoffset) {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(w));
        self->m_scrollY += yoffset;
    });
}

Window::~Window() {
    if (m_handle) {
        glfwDestroyWindow(m_handle);
    }
    glfwTerminate();
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(m_handle) != 0;
}

void Window::PollEvents() {
    glfwPollEvents();
}

uint32_t Window::GetWidth() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_handle, &w, &h);
    return static_cast<uint32_t>(w);
}

uint32_t Window::GetHeight() const {
    int w = 0, h = 0;
    glfwGetFramebufferSize(m_handle, &w, &h);
    return static_cast<uint32_t>(h);
}

bool Window::WasResized() const {
    return m_wasResized;
}

void Window::ConsumeResizeFlag() {
    m_wasResized = false;
}

void* Window::GetNativeHandle() const {
    return static_cast<void*>(m_handle);
}

std::vector<const char*> Window::GetRequiredInstanceExtensions() const {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);
    return std::vector<const char*>(extensions, extensions + count);
}

bool Window::IsKeyDown(int key) const {
    return m_keysDown.count(key) != 0;
}

bool Window::IsMouseButtonDown(int button) const {
    return m_mouseButtonsDown.count(button) != 0;
}

void Window::GetMousePos(double& x, double& y) const {
    x = m_mouseX;
    y = m_mouseY;
}

double Window::ConsumeScrollY() {
    const double v = m_scrollY;
    m_scrollY = 0.0;
    return v;
}

} // namespace MagicXEngine::Core

