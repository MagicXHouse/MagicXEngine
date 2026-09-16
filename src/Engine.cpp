#include "Engine.h"

#include "Backend/RenderScene.h"
#include "Backend/RHI/RHI.h"
#include "Core/Logger.h"
#include "Core/Window.h"

#include <exception>

namespace MagicXEngine {

int RunScene(const Frontend::Scene& scene, const std::string& title) {
    Core::Window window(800, 600, title);

    RHI::WindowSurface surface;
    surface.nativeHandle = window.GetNativeHandle();
    surface.width        = window.GetWidth();
    surface.height       = window.GetHeight();

    // 唯一选择后端的地方；将来切换 OpenGL 改这里
    auto device = RHI::CreateDevice(RHI::Backend::Vulkan, surface);

    Backend::RenderScene renderScene(device.get());
    renderScene.Load(scene);

    const uint32_t framesInFlight = device->GetFramesInFlight();
    uint32_t frameIndex = 0;

    while (!window.ShouldClose()) {
        window.PollEvents();

        if (window.WasResized()) {
            window.ConsumeResizeFlag();
            device->Resize(window.GetWidth(), window.GetHeight());
        }

        uint32_t imageIndex = 0;
        device->BeginFrame(frameIndex, imageIndex);

        renderScene.Render(frameIndex);

        device->EndFrame(frameIndex, imageIndex);
        frameIndex = (frameIndex + 1) % framesInFlight;
    }

    device->WaitIdle();
    return 0;
}

} // namespace MagicXEngine
