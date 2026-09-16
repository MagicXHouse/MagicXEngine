#pragma once
#include <string>
#include "Frontend/Scene.h"

namespace MagicXEngine {

// 运行一个场景：创建窗口 + RHI 设备 + RenderScene，进入渲染循环。
// 每个案例（Case）调用此函数即可独立运行。
int RunScene(const Frontend::Scene& scene, const std::string& title = "MagicXEngine");

} // namespace MagicXEngine
