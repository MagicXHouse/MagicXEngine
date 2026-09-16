#include "TriangleRenderer.h"

#include "RHI/RHI.h"

#include "triangle_vert_spv.h"
#include "triangle_frag_spv.h"

#include <cstddef>

namespace MagicXEngine::Renderer {

namespace {

struct Vertex {
    float position[3];
    float color[3];
};

// NDC 坐标，红绿蓝三色
constexpr Vertex kVertices[3] = {
    { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { {-0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
};

} // namespace

TriangleRenderer::TriangleRenderer(RHI::IRHIDevice* device) : m_device(device) {
    // ---- 图形管线：顶点 + 片元着色器 ----
    RHI::PipelineDesc desc;

    RHI::ShaderDesc vs;
    vs.stage = RHI::ShaderStage::Vertex;
    vs.spirv.assign(MagicXEngine::Shaders::triangle_vert_spv,
                    MagicXEngine::Shaders::triangle_vert_spv + MagicXEngine::Shaders::triangle_vert_spv_word_count);

    RHI::ShaderDesc fs;
    fs.stage = RHI::ShaderStage::Fragment;
    fs.spirv.assign(MagicXEngine::Shaders::triangle_frag_spv,
                    MagicXEngine::Shaders::triangle_frag_spv + MagicXEngine::Shaders::triangle_frag_spv_word_count);

    desc.shaders = { vs, fs };

    RHI::VertexBinding binding{};
    binding.binding = 0;
    binding.stride  = sizeof(Vertex);
    desc.vertexBindings = { binding };

    RHI::VertexAttribute pos{};
    pos.location = 0;
    pos.binding  = 0;
    pos.format   = RHI::Format::R32G32B32_SFLOAT;
    pos.offset   = offsetof(Vertex, position);

    RHI::VertexAttribute col{};
    col.location = 1;
    col.binding  = 0;
    col.format   = RHI::Format::R32G32B32_SFLOAT;
    col.offset   = offsetof(Vertex, color);

    desc.vertexAttributes = { pos, col };

    m_pipeline = m_device->CreatePipeline(desc);

    // ---- 顶点缓冲 ----
    RHI::BufferDesc bd{};
    bd.size          = sizeof(kVertices);
    bd.usage         = RHI::BufferUsage::Vertex;
    bd.cpuAccessible = true;
    m_vertexBuffer = m_device->CreateBuffer(bd, kVertices);
}

TriangleRenderer::~TriangleRenderer() = default;

void TriangleRenderer::Render(uint32_t frameIndex) {
    RHI::IRHICommandBuffer* cmd = m_device->GetCommandBuffer(frameIndex);

    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->BindVertexBuffer(m_vertexBuffer.get(), 0);

    const uint32_t w = m_device->GetSwapchainWidth();
    const uint32_t h = m_device->GetSwapchainHeight();
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    cmd->Draw(3);

    cmd->EndRenderPass();
}

} // namespace MagicXEngine::Renderer
