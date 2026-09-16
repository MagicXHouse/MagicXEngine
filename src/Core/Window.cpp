#include "Window.h"

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

} // namespace MagicXEngine::Core
