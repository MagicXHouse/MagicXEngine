// ===========================================================================
// Case02 - GPU 驱动（间接绘制）验证
//
// 与 Case01 相同的三角形，但 renderMode = Indirect：
//   compute shader（cull.comp）写一条固定 DrawIndexedIndirectCommand 到 SSBO，
//   barrier 后由 DrawIndexedIndirect 间接绘制。
// 用 RenderDoc 抓帧可验证：compute dispatch 在 render pass 之前、间接缓冲内容、
//   以及 barrier 无 SYNC-HAZARD 报错。
// ===========================================================================
#include "MagicXEngine/Engine.h"
#include "MagicXEngine/Frontend/Scene.h"

using namespace MagicXEngine;

int main() {
    Frontend::Scene scene;
    scene.renderMode = Frontend::RenderMode::Indirect;

    Frontend::SceneObject triangle;

    triangle.mesh.vertices = {
        { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },  // 下    红
        { { 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },  // 右上  绿
        { {-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },  // 左上  蓝
    };
    triangle.mesh.indices = { 0, 1, 2 };
    triangle.transform = Frontend::Transform{};

    scene.objects.push_back(std::move(triangle));

    return RunScene(scene, "Case02 - GPUDriven");
}
