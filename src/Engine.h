#pragma once
#include <memory>

namespace MagicXEngine::Core { class Window; }
namespace MagicXEngine::RHI { class IRHIDevice; }
namespace MagicXEngine::Renderer { class TriangleRenderer; }

namespace MagicXEngine {

// 引擎主循环：负责窗口、RHI 设备、渲染器的生命周期与帧循环。
class Engine {
public:
    Engine();
    ~Engine();

    void Run();

private:
    std::unique_ptr<Core::Window>          m_window;
    std::unique_ptr<RHI::IRHIDevice>       m_device;
    std::unique_ptr<Renderer::TriangleRenderer> m_renderer;
};

} // namespace MagicXEngine
