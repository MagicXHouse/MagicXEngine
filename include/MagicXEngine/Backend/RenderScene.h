#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "MagicXEngine/Frontend/Scene.h"

namespace MagicXEngine::RHI {
class IRHIDevice;
class IRHIPipeline;
class IRHIBuffer;
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

    // 每帧渲染（frameIndex: 帧在飞行索引）
    void Render(uint32_t frameIndex);

private:
    struct GpuObject {
        std::unique_ptr<RHI::IRHIBuffer> vertexBuffer;
        std::unique_ptr<RHI::IRHIBuffer> indexBuffer;
        uint32_t          vertexCount = 0;
        uint32_t          indexCount  = 0;
        Frontend::Transform transform;
    };

    RHI::IRHIDevice* m_device = nullptr;
    std::unique_ptr<RHI::IRHIPipeline> m_pipeline;
    std::vector<GpuObject> m_objects;
    Frontend::Camera m_camera;
};

} // namespace MagicXEngine::Backend

