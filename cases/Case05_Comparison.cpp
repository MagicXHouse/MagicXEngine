// ===========================================================================
// Case05 - meshlet 剔除 vs 普通（逐对象）剔除 对比（含 FPS）
//
// 多对象场景（25x25 = 625 个 patch，每个 25x25 四边形，约 78 万三角形）。
// 轨道相机观察；checkbox 是 A/B 开关：勾选=meshlet 渲染、取消=普通渲染。
// 面板实时显示：当前模式 + 真实 FPS + 两种剔除各画多少三角形。
// ===========================================================================
#include "MagicXEngine/Engine.h"
#include "MagicXEngine/Backend/MeshletBuilder.h"
#include "imgui.h"
#include <cmath>

using namespace MagicXEngine;

int main() {
    Frontend::Scene scene;
    scene.renderMode = Frontend::RenderMode::Meshlet;

    scene.camera.screenSpace = false;
    scene.camera.nearPlane   = 1.0f;
    scene.camera.farPlane    = 2000.0f;

    const int PATCH_N = 25;
    const int PATCH_SIZE = 25;
    const float cellSize = 1.0f;
    const float patchExtent = PATCH_SIZE * cellSize;
    const float halfGrid = PATCH_N * patchExtent * 0.5f;

    for (int py = 0; py < PATCH_N; ++py) {
        for (int px = 0; px < PATCH_N; ++px) {
            Frontend::SceneObject patch;
            const float ox = px * patchExtent - halfGrid;
            const float oz = py * patchExtent - halfGrid;
            const float r = static_cast<float>(px) / PATCH_N;
            const float g = static_cast<float>(py) / PATCH_N;
            for (int y = 0; y <= PATCH_SIZE; ++y) {
                for (int x = 0; x <= PATCH_SIZE; ++x) {
                    patch.mesh.vertices.push_back({
                        { ox + x * cellSize, 0.0f, oz + y * cellSize },
                        { r, g, 0.4f }
                    });
                }
            }
            for (int y = 0; y < PATCH_SIZE; ++y) {
                for (int x = 0; x < PATCH_SIZE; ++x) {
                    const uint32_t i00 = y * (PATCH_SIZE + 1) + x, i10 = i00 + 1;
                    const uint32_t i01 = (y + 1) * (PATCH_SIZE + 1) + x, i11 = i01 + 1;
                    patch.mesh.indices.insert(patch.mesh.indices.end(), { i00, i10, i11, i00, i11, i01 });
                }
            }
            scene.objects.push_back(std::move(patch));
        }
    }

    // 预计算
    std::vector<Backend::BoundingSphere> patchSpheres;
    patchSpheres.reserve(scene.objects.size());
    for (const auto& obj : scene.objects) {
        patchSpheres.push_back(Backend::ComputeBoundingSphere(obj.mesh));
    }
    const auto flattened = Backend::FlattenObjects(scene.objects);
    const auto meshletBuild = Backend::BuildMeshlets(flattened, 64, 126);
    const uint32_t totalTris = static_cast<uint32_t>(flattened.indices.size()) / 3;

    // 轨道相机 + FPS
    float yaw = 0.7f, pitch = 0.5f, dist = 400.0f;
    Math::Vec3 target{ 0.0f, 0.0f, 0.0f };
    float fps = 0.0f;

    auto update = [&](Frontend::Scene& s, Core::Window& window, float dt) {
        ImGuiIO& io = ImGui::GetIO();

        // 轨道相机
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                yaw   -= io.MouseDelta.x * 0.005f;
                pitch += io.MouseDelta.y * 0.005f;
                if (pitch < 0.05f) pitch = 0.05f;
                if (pitch > 1.4f)  pitch = 1.4f;
            }
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
                target.x -= io.MouseDelta.x * 0.1f;
                target.z -= io.MouseDelta.y * 0.1f;
            }
            if (io.MouseWheel != 0.0f) {
                dist *= (1.0f - io.MouseWheel * 0.1f);
                if (dist < 60.0f)   dist = 60.0f;
                if (dist > 1200.0f) dist = 1200.0f;
            }
        }
        Math::Vec3 pos;
        pos.x = target.x + dist * std::cos(pitch) * std::sin(yaw);
        pos.y = target.y + dist * std::sin(pitch);
        pos.z = target.z + dist * std::cos(pitch) * std::cos(yaw);
        s.camera.position = pos;
        s.camera.target   = target;
        s.camera.up       = { 0.0f, 1.0f, 0.0f };

        // 模式切换（A/B）
        bool useMeshlet = (s.renderMode == Frontend::RenderMode::Meshlet);
        if (ImGui::Checkbox("Use meshlet culling", &useMeshlet)) {
            s.renderMode = useMeshlet ? Frontend::RenderMode::Meshlet
                                      : Frontend::RenderMode::Culled;
        }

        // stats（世界空间视锥）
        const float aspect = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
        const Math::Frustum frustum = Math::ExtractFrustumPlanes(s.camera.Projection(aspect) * s.camera.View());

        uint32_t normalTris = 0, visiblePatches = 0;
        for (size_t i = 0; i < s.objects.size(); ++i) {
            if (!Math::SphereOutsideFrustum(frustum, patchSpheres[i].center, patchSpheres[i].radius)) {
                ++visiblePatches;
                normalTris += static_cast<uint32_t>(s.objects[i].mesh.indices.size()) / 3;
            }
        }
        uint32_t meshletTris = 0, visibleMeshlets = 0;
        for (const auto& m : meshletBuild.meshlets) {
            if (!Math::SphereOutsideFrustum(frustum, m.sphereCenter, m.sphereRadius)) {
                ++visibleMeshlets;
                meshletTris += m.indexCount / 3;
            }
        }

        // FPS（平滑）
        if (dt > 0.0001f) {
            fps = fps * 0.9f + (1.0f / dt) * 0.1f;
        }

        ImGui::Begin("Meshlet vs Normal Culling");
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Current mode: %s",
                           useMeshlet ? "Meshlet (GPU)" : "Normal (CPU per-object)");
        ImGui::Text("FPS: %.0f", fps);
        ImGui::Text("Drawing now: %u tris", useMeshlet ? meshletTris : normalTris);
        ImGui::Separator();
        ImGui::Text("Total: %u tris | %zu objects | %zu meshlets",
                    totalTris, s.objects.size(), meshletBuild.meshlets.size());
        ImGui::Text("Normal culling : %u tris (%u/%zu objects)", normalTris, visiblePatches, s.objects.size());
        ImGui::Text("Meshlet culling: %u tris (%u/%zu meshlets)", meshletTris, visibleMeshlets, meshletBuild.meshlets.size());
        ImGui::Text("Saved: %.1f%%", 100.0f * (1.0f - static_cast<float>(meshletTris) / static_cast<float>(totalTris)));
        ImGui::TextWrapped("LMB=orbit  MMB=pan  wheel=zoom. Toggle the checkbox and watch FPS change.");
        ImGui::End();
    };

    return RunScene(scene, "Case05 - Meshlet vs Normal Culling", update);
}
