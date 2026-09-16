// ===========================================================================
// Case01 - 彩色三角形（第一个案例）
//
// 展示三层架构的完整数据流：
//   案例层（本文件）→ 前端（Scene/Mesh/Transform）→ 后端（RenderScene）→ RHI（Vulkan）
// ===========================================================================
#include "MagicXEngine/Engine.h"
#include "MagicXEngine/Frontend/Scene.h"

using namespace MagicXEngine;

int main() {
    Frontend::Scene scene;

    // 相机：默认「屏幕空间」相机（NDC 直通），三角形直接用 NDC 坐标。
    // 如需透视相机，设置 scene.camera.screenSpace = false 并配置 position/target/fov。

    Frontend::SceneObject triangle;

    // 顶点：位置 + 颜色
    triangle.mesh.vertices = {
        { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // 下    红
        { { 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 右上  绿
        { {-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },  // 左上  蓝
    };
    // 索引
    triangle.mesh.indices = { 0, 1, 2 };
    // 变换（可改成位移/旋转/缩放来验证 transform 生效）
    triangle.transform = Frontend::Transform{};

    scene.objects.push_back(std::move(triangle));

    return RunScene(scene, "Case01 - Triangle");
}

