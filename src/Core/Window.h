#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct GLFWwindow;

namespace MagicXEngine::Core {

// 窗口抽象：内部使用 GLFW，对外只暴露高层接口，便于日后换成 SDL/Win32。
class Window {
public:
    using FramebufferResizeCallback = std::function<void(uint32_t, uint32_t)>;

    Window(uint32_t width, uint32_t height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ShouldClose() const;
    void PollEvents();

    uint32_t GetWidth() const;   // 帧缓冲（像素）尺寸
    uint32_t GetHeight() const;
    bool WasResized() const;
    void ConsumeResizeFlag();

    // 原生句柄（GLFWwindow*），供 RHI 创建 Vulkan 表面
    void* GetNativeHandle() const;

    // Vulkan 实例需要从窗口系统获取的扩展名列表
    std::vector<const char*> GetRequiredInstanceExtensions() const;

private:
    GLFWwindow* m_handle = nullptr;
    bool m_wasResized = false;
};

} // namespace MagicXEngine::Core
