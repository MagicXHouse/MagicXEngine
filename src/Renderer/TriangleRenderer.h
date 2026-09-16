#pragma once
#include <cstdint>
#include <memory>

namespace MagicXEngine::RHI {
class IRHIDevice;
class IRHIPipeline;
class IRHIBuffer;
}

namespace MagicXEngine::Renderer {

// 三角形 Demo 渲染器：只依赖 RHI 抽象接口，不包含任何 Vulkan 头文件。
// 换后端（如 OpenGL）时，此文件无需修改。
class TriangleRenderer {
public:
    explicit TriangleRenderer(RHI::IRHIDevice* device);
    ~TriangleRenderer();

    void Render(uint32_t frameIndex);

private:
    RHI::IRHIDevice* m_device;
    std::unique_ptr<RHI::IRHIPipeline> m_pipeline;
    std::unique_ptr<RHI::IRHIBuffer>  m_vertexBuffer;
};

} // namespace MagicXEngine::Renderer
