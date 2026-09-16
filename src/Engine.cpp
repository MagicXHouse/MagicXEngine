#include "Engine.h"

#include "Core/Logger.h"
#include "Core/Window.h"
#include "RHI/RHI.h"
#include "Renderer/TriangleRenderer.h"

namespace MagicXEngine {

Engine::Engine() {
    m_window = std::make_unique<Core::Window>(800, 600, "MagicXEngine - Vulkan Triangle");

    RHI::WindowSurface surface;
    surface.nativeHandle = m_window->GetNativeHandle();
    surface.width        = m_window->GetWidth();
    surface.height       = m_window->GetHeight();

    // 唯一一处选择后端的地方；将来切换 OpenGL 只需改这里的 Backend 枚举
    m_device = RHI::CreateDevice(RHI::Backend::Vulkan, surface);

    m_renderer = std::make_unique<Renderer::TriangleRenderer>(m_device.get());
}

Engine::~Engine() {
    if (m_device) m_device->WaitIdle();
    m_renderer.reset();
    m_device.reset();
    m_window.reset();
}

void Engine::Run() {
    const uint32_t framesInFlight = m_device->GetFramesInFlight();
    uint32_t frameIndex = 0;

    while (!m_window->ShouldClose()) {
        m_window->PollEvents();

        if (m_window->WasResized()) {
            m_window->ConsumeResizeFlag();
            m_device->Resize(m_window->GetWidth(), m_window->GetHeight());
        }

        uint32_t imageIndex = 0;
        m_device->BeginFrame(frameIndex, imageIndex);

        m_renderer->Render(frameIndex);

        m_device->EndFrame(frameIndex, imageIndex);

        frameIndex = (frameIndex + 1) % framesInFlight;
    }

    m_device->WaitIdle();
}

} // namespace MagicXEngine
