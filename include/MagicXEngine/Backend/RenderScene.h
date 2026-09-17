#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "MagicXEngine/Frontend/Scene.h"

namespace MagicXEngine::RHI {
class IRHIDevice;
class IRHIPipeline;
class IRHIBuffer;
class IRHICommandBuffer;
class IRHIDescriptorSet;
}

namespace MagicXEngine::Backend {

// ===========================================================================
// RenderScene —— 后端：管理前端传来的场景数据，转换为 GPU 资源（经 RHI）并渲染。
// ===========================================================================
class RenderScene {
public:
    explicit RenderScene(RHI::IRHIDevice* device);
    ~RenderScene();

    // 上传场景几何数据、创建渲染管线
    void Load(const Frontend::Scene& scene);

    // 每帧同步相机与对象变换（交互/动画用，不重建 GPU 资源）
    void Update(const Frontend::Scene& scene);

    // 每帧渲染（frameIndex: 帧在飞行索引）。uiRender 在 render pass 内、场景绘制后调用（用于 ImGui 叠加层）。
    void Render(uint32_t frameIndex, const std::function<void()>& uiRender = nullptr);

private:
    struct GpuObject {
        std::unique_ptr<RHI::IRHIBuffer> vertexBuffer;
        std::unique_ptr<RHI::IRHIBuffer> indexBuffer;
        uint32_t          vertexCount = 0;
        uint32_t          indexCount  = 0;
        Frontend::Transform transform;
    };

    // Indirect 模式：compute 生成间接命令 → barrier → 间接绘制
    void RenderIndirect(RHI::IRHICommandBuffer* cmd, float aspect, uint32_t w, uint32_t h,
                        const std::function<void()>& uiRender);

    // 投影矩阵（透视相机时应用 Vulkan NDC Y 翻转）
    Math::Mat4 ComputeProjection(float aspect) const;

    RHI::IRHIDevice* m_device = nullptr;
    std::unique_ptr<RHI::IRHIPipeline> m_pipeline;
    std::vector<GpuObject> m_objects;
    Frontend::Camera m_camera;

    // GPU 驱动（间接绘制）资源
    Frontend::RenderMode m_renderMode = Frontend::RenderMode::Direct;
    std::unique_ptr<RHI::IRHIPipeline>      m_computePipeline;
    std::unique_ptr<RHI::IRHIDescriptorSet> m_descriptorSet;
    std::unique_ptr<RHI::IRHIBuffer>        m_indirectBuffer;
};

} // namespace MagicXEngine::Backend

