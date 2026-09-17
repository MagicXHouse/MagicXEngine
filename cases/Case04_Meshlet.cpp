// ===========================================================================
// Case04 - meshlet 逐块视锥剔除
//
// 大网格（150x150 = 45000 三角形）被分成约 460 个 meshlet，
// compute shader 逐 meshlet 做视锥剔除，视锥外的 meshlet instanceCount=0 跳过绘制。
// 用 RenderDoc 抓帧可观察 indirect buffer 中 instanceCount 的 3/0 分布。
// ===========================================================================
#include "MagicXEngine/Engine.h"
#include "imgui.h"

using namespace MagicXEngine;

int main() {
    Frontend::Scene scene;
    scene.renderMode = Frontend::RenderMode::Meshlet;

    // 透视相机，俯视大网格
    scene.camera.screenSpace = false;
    scene.camera.position    = { 0.0f, 40.0f, 60.0f };
    scene.camera.target      = { 0.0f, 0.0f, 0.0f };
    scene.camera.up          = { 0.0f, 1.0f, 0.0f };
    scene.camera.fovYDeg     = 60.0f;
    scene.camera.nearPlane   = 1.0f;
    scene.camera.farPlane    = 200.0f;

    // 大网格：XZ 平面（地面），150x150 四边形
    Frontend::SceneObject grid;
    const int N = 150;
    const float size = 150.0f;
    for (int y = 0; y <= N; ++y) {
        for (int x = 0; x <= N; ++x) {
            const float px = static_cast<float>(x) / N * size - size * 0.5f;
            const float pz = static_cast<float>(y) / N * size - size * 0.5f;
            const float r  = static_cast<float>(x) / N;
            const float g  = static_cast<float>(y) / N;
            grid.mesh.vertices.push_back({ { px, 0.0f, pz }, { r, g, 0.3f } });
        }
    }
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            const uint32_t i00 = y * (N + 1) + x, i10 = i00 + 1;
            const uint32_t i01 = (y + 1) * (N + 1) + x, i11 = i01 + 1;
            grid.mesh.indices.insert(grid.mesh.indices.end(), { i00, i10, i11, i00, i11, i01 });
        }
    }
    scene.objects.push_back(std::move(grid));

    auto update = [&](Frontend::Scene& s, Core::Window& /*window*/, float /*dt*/) {
        auto& t = s.objects[0].transform;
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                t.rotation.y += io.MouseDelta.x * 0.01f;
                t.rotation.x += io.MouseDelta.y * 0.01f;
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                t.position.x += io.MouseDelta.x * 0.05f;
                t.position.y -= io.MouseDelta.y * 0.05f;
            }
            if (io.MouseWheel != 0.0f) {
                t.scale = t.scale * (1.0f + io.MouseWheel * 0.1f);
            }
        }

        ImGui::Begin("Meshlet Culling");
        ImGui::TextWrapped("Grid: 150x150 (45000 tris) -> ~460 meshlets");
        ImGui::TextWrapped("LMB drag = Rotate | MMB drag = Move | Wheel = Scale");
        ImGui::TextWrapped("Capture a frame in RenderDoc to inspect instanceCount 3/0");
        ImGui::End();
    };

    return RunScene(scene, "Case04 - Meshlet Culling", update);
}
