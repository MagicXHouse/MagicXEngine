#pragma once
#include <cstdint>

namespace MagicXEngine {

namespace Core { class Window; }
namespace RHI { class IRHIDevice; }

namespace Backend::UI {

// ===========================================================================
// ImGuiUI —— ImGui 插件：封装初始化、每帧 NewFrame 与 Render。
//
// 案例在 RunScene 的 update 回调里直接调用 ImGui:: 系列函数绘制 UI 即可
// （引擎已在回调前完成 NewFrame）。输入采用「手动喂入 ImGuiIO」方式，
// 复用引擎 Window 的输入查询，不覆盖 GLFW 回调。
// ===========================================================================
class ImGuiUI {
public:
    ImGuiUI(Core::Window& window, RHI::IRHIDevice* device);
    ~ImGuiUI();

    ImGuiUI(const ImGuiUI&) = delete;
    ImGuiUI& operator=(const ImGuiUI&) = delete;

    void NewFrame(float deltaTime);    // 喂输入 + ImGui::NewFrame
    void Render(uint32_t frameIndex);  // ImGui::Render + RenderDrawData（须在 render pass 内调用）

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace Backend::UI
} // namespace MagicXEngine
