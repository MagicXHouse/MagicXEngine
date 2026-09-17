#include "MagicXEngine/Engine.h"

#include "MagicXEngine/Backend/RenderScene.h"
#include "MagicXEngine/Backend/RHI/RHI.h"
#include "MagicXEngine/Backend/UI/ImGuiUI.h"
#include "MagicXEngine/Core/Logger.h"
#include "MagicXEngine/Core/Window.h"

#include <chrono>
#include <exception>

namespace MagicXEngine {

int RunScene(const Frontend::Scene& scene, const std::string& title, SceneUpdateFn update) {
    Core::Window window(800, 600, title);

    RHI::WindowSurface surface;
    surface.nativeHandle = window.GetNativeHandle();
    surface.width        = window.GetWidth();
    surface.height       = window.GetHeight();

    // 唯一选择后端的地方；将来切换 OpenGL 改这里
    auto device = RHI::CreateDevice(RHI::Backend::Vulkan, surface);

    Backend::RenderScene renderScene(device.get());
    renderScene.Load(scene);

    // ImGui 插件（UI 叠加层）
    Backend::UI::ImGuiUI ui(window, device.get());

    // 交互用可变副本（update 回调修改它，RenderScene 每帧同步）
    Frontend::Scene mutableScene = scene;

    const uint32_t framesInFlight = device->GetFramesInFlight();
    uint32_t frameIndex = 0;

    auto lastTime = std::chrono::steady_clock::now();

    while (!window.ShouldClose()) {
        window.PollEvents();

        const auto now = std::chrono::steady_clock::now();
        const float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        ui.NewFrame(dt);
        if (update) {
            update(mutableScene, window, dt);
        }
        renderScene.Update(mutableScene);

        if (window.WasResized()) {
            window.ConsumeResizeFlag();
            device->Resize(window.GetWidth(), window.GetHeight());
        }

        uint32_t imageIndex = 0;
        device->BeginFrame(frameIndex, imageIndex);

        renderScene.Render(frameIndex, [&]() { ui.Render(frameIndex); });

        device->EndFrame(frameIndex, imageIndex);
        frameIndex = (frameIndex + 1) % framesInFlight;
    }

    device->WaitIdle();
    return 0;
}

} // namespace MagicXEngine

