#include "MagicXEngine/Backend/RenderScene.h"

#include "MagicXEngine/Backend/RHI/RHI.h"

#include "triangle_vert_spv.h"
#include "triangle_frag_spv.h"

#include <cstddef>

namespace MagicXEngine::Backend {

using namespace RHI;

RenderScene::RenderScene(RHI::IRHIDevice* device) : m_device(device) {}

RenderScene::~RenderScene() = default;

void RenderScene::Load(const Frontend::Scene& scene) {
    m_camera = scene.camera;

    // ---- 渲染管线（所有对象共享：位置+颜色，MVP 走 push constant） ----
    PipelineDesc desc;

    ShaderDesc vs;
    vs.stage = ShaderStage::Vertex;
    vs.spirv.assign(MagicXEngine::Shaders::triangle_vert_spv,
                    MagicXEngine::Shaders::triangle_vert_spv + MagicXEngine::Shaders::triangle_vert_spv_word_count);
    ShaderDesc fs;
    fs.stage = ShaderStage::Fragment;
    fs.spirv.assign(MagicXEngine::Shaders::triangle_frag_spv,
                    MagicXEngine::Shaders::triangle_frag_spv + MagicXEngine::Shaders::triangle_frag_spv_word_count);
    desc.shaders = { vs, fs };

    VertexBinding binding;
    binding.binding = 0;
    binding.stride  = sizeof(Frontend::Vertex);
    desc.vertexBindings = { binding };

    VertexAttribute pos;
    pos.location = 0;
    pos.binding  = 0;
    pos.format   = Format::R32G32B32_SFLOAT;
    pos.offset   = offsetof(Frontend::Vertex, position);
    VertexAttribute col;
    col.location = 1;
    col.binding  = 0;
    col.format   = Format::R32G32B32_SFLOAT;
    col.offset   = offsetof(Frontend::Vertex, color);
    desc.vertexAttributes = { pos, col };

    desc.pushConstantSize = sizeof(Math::Mat4); // MVP 矩阵（64 字节）

    m_pipeline = m_device->CreatePipeline(desc);

    // ---- 上传每个对象的顶点/索引数据 ----
    m_objects.clear();
    m_objects.reserve(scene.objects.size());
    for (const auto& obj : scene.objects) {
        GpuObject g;

        BufferDesc vb;
        vb.size          = obj.mesh.vertices.size() * sizeof(Frontend::Vertex);
        vb.usage         = BufferUsage::Vertex;
        vb.cpuAccessible = true;
        g.vertexBuffer = m_device->CreateBuffer(vb, obj.mesh.vertices.data());
        g.vertexCount  = static_cast<uint32_t>(obj.mesh.vertices.size());

        if (!obj.mesh.indices.empty()) {
            BufferDesc ib;
            ib.size          = obj.mesh.indices.size() * sizeof(uint32_t);
            ib.usage         = BufferUsage::Index;
            ib.cpuAccessible = true;
            g.indexBuffer = m_device->CreateBuffer(ib, obj.mesh.indices.data());
            g.indexCount  = static_cast<uint32_t>(obj.mesh.indices.size());
        }

        g.transform = obj.transform;
        m_objects.push_back(std::move(g));
    }
}

void RenderScene::Render(uint32_t frameIndex) {
    IRHICommandBuffer* cmd = m_device->GetCommandBuffer(frameIndex);

    const uint32_t w = m_device->GetSwapchainWidth();
    const uint32_t h = m_device->GetSwapchainHeight();
    const float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    for (const auto& obj : m_objects) {
        // MVP = 投影 * 视图 * 模型（transform 在这里生效）
        const Math::Mat4 mvp = m_camera.Projection(aspect) * m_camera.View() * obj.transform.Matrix();
        cmd->PushConstants(&mvp, sizeof(mvp));

        cmd->BindVertexBuffer(obj.vertexBuffer.get(), 0);
        if (obj.indexBuffer) {
            cmd->BindIndexBuffer(obj.indexBuffer.get(), 0);
            cmd->DrawIndexed(obj.indexCount);
        } else {
            cmd->Draw(obj.vertexCount);
        }
    }

    cmd->EndRenderPass();
}

} // namespace MagicXEngine::Backend

