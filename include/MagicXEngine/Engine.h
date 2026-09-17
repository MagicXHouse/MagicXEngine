#pragma once
#include <functional>
#include <string>
#include "MagicXEngine/Core/Window.h"
#include "MagicXEngine/Frontend/Scene.h"

namespace MagicXEngine {

// 每帧更新回调：读取输入、修改场景（相机/对象变换）。deltaTime 单位为秒。
using SceneUpdateFn = std::function<void(Frontend::Scene& scene, Core::Window& window, float deltaTime)>;

// 运行一个场景：创建窗口 + RHI 设备 + RenderScene，进入渲染循环。
// 每个案例（Case）调用此函数即可独立运行。可传入 update 回调实现交互/动画。
int RunScene(const Frontend::Scene& scene, const std::string& title = "MagicXEngine",
             SceneUpdateFn update = nullptr);

} // namespace MagicXEngine

