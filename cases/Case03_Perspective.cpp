// ===========================================================================
// Case03 - ImGui 面板 + 鼠标交互（旋转/移动/缩放）
//
// 交互：
//   - ImGui 面板：勾选「正交投影」切换透视/正交，滑块调旋转/缩放，按钮重置
//   - 鼠标左键拖拽：旋转模型（绕 X/Y 轴）
//   - 鼠标中键拖拽：移动模型（屏幕平面）
//   - 滚轮：缩放模型
// ===========================================================================
#include "MagicXEngine/Engine.h"
#include "imgui.h"

using namespace MagicXEngine;

// 辅助：给网格添加一个矩形面（4 顶点 + 2 三角形）
static void AddFace(Frontend::MeshData& mesh,
                    const Math::Vec3& a, const Math::Vec3& b,
                    const Math::Vec3& c, const Math::Vec3& d,
                    const Math::Vec3& color) {
    const uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
    mesh.vertices.push_back({ a, color });
    mesh.vertices.push_back({ b, color });
    mesh.vertices.push_back({ c, color });
    mesh.vertices.push_back({ d, color });
    mesh.indices.insert(mesh.indices.end(),
                        { base, base + 1, base + 2, base + 2, base + 3, base });
}

int main() {
    Frontend::Scene scene;

    scene.camera.screenSpace = false;
    scene.camera.position    = { 2.2f, 2.0f, 2.5f };
    scene.camera.target      = { 0.0f, 0.0f, 0.0f };
    scene.camera.up          = { 0.0f, 1.0f, 0.0f };
    scene.camera.fovYDeg     = 60.0f;
    scene.camera.nearPlane   = 0.1f;
    scene.camera.farPlane    = 100.0f;
    scene.camera.orthoSize   = 4.0f;

    Frontend::SceneObject cube;
    const float s = 0.8f; // 半边长

    AddFace(cube.mesh, {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s, s, s}, {1,0,0}); // 前 +Z 红
    AddFace(cube.mesh, { s,-s,-s}, {-s,-s,-s}, {-s, s,-s}, { s, s,-s}, {0,1,0}); // 后 -Z 绿
    AddFace(cube.mesh, {-s,-s,-s}, {-s,-s, s}, {-s, s, s}, {-s, s,-s}, {0,0,1}); // 左 -X 蓝
    AddFace(cube.mesh, { s,-s, s}, { s,-s,-s}, { s, s,-s}, { s, s, s}, {1,1,0}); // 右 +X 黄
    AddFace(cube.mesh, {-s, s, s}, { s, s, s}, { s, s,-s}, {-s, s,-s}, {1,0,1}); // 上 +Y 品红
    AddFace(cube.mesh, {-s,-s,-s}, { s,-s,-s}, { s,-s, s}, {-s,-s, s}, {0,1,1}); // 下 -Y 青
    scene.objects.push_back(std::move(cube));

    auto update = [&](Frontend::Scene& s, Core::Window& /*window*/, float /*dt*/) {
        auto& t = s.objects[0].transform;
        ImGuiIO& io = ImGui::GetIO();

        // 3D 交互（鼠标悬停在 ImGui 面板上时不响应）
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {      // 左键拖拽：旋转
                t.rotation.y += io.MouseDelta.x * 0.01f;
                t.rotation.x += io.MouseDelta.y * 0.01f;
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {    // 中键拖拽：移动
                t.position.x += io.MouseDelta.x * 0.005f;
                t.position.y -= io.MouseDelta.y * 0.005f;
            }
            if (io.MouseWheel != 0.0f) {                          // 滚轮：缩放
                t.scale = t.scale * (1.0f + io.MouseWheel * 0.1f);
            }
        }

        // ImGui panel
        ImGui::Begin("Settings");
        ImGui::Checkbox("Orthographic", &s.camera.orthographic);
        ImGui::SliderFloat3("Rotation", &t.rotation.x, -3.14159f, 3.14159f);
        ImGui::SliderFloat3("Scale", &t.scale.x, 0.1f, 5.0f);
        if (ImGui::Button("Reset")) {
            t = Frontend::Transform{};
        }
        ImGui::TextWrapped("LMB drag = Rotate | MMB drag = Move | Wheel = Scale");
        ImGui::End();
    };

    return RunScene(scene, "Case03 - ImGui + Mouse", update);
}
