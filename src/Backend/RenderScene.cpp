#include "MagicXEngine/Backend/RenderScene.h"

#include "MagicXEngine/Backend/RHI/RHI.h"

#include "triangle_vert_spv.h"
#include "triangle_frag_spv.h"
#include "cull_comp_spv.h"

#include <cstddef>

namespace MagicXEngine::Backend {

using namespace RHI;

RenderScene::RenderScene(RHI::IRHIDevice* device) : m_device(device) {}

RenderScene::~RenderScene() = default;

void RenderScene::Load(const Frontend::Scene& scene) {
    m_camera     = scene.camera;
    m_renderMode = scene.renderMode;

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
    desc.depthTestEnable  = true;               // 深度测试
    desc.depthWriteEnable = true;               // 深度写入

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

    // ---- GPU 驱动（间接绘制）资源 ----
    if (m_renderMode == Frontend::RenderMode::Indirect) {
        // 间接命令缓冲：compute 每帧写入，图形侧间接读取
        BufferDesc indBuf;
        indBuf.size  = sizeof(DrawIndexedIndirectCommand);
        indBuf.usage = BufferUsage::Storage | BufferUsage::Indirect;
        m_indirectBuffer = m_device->CreateBuffer(indBuf, nullptr);

        // compute 管线：写一条固定命令（见 cull.comp）
        ComputePipelineDesc cdesc;
        ShaderDesc cs;
        cs.stage = ShaderStage::Compute;
        cs.spirv.assign(MagicXEngine::Shaders::cull_comp_spv,
                        MagicXEngine::Shaders::cull_comp_spv + MagicXEngine::Shaders::cull_comp_spv_word_count);
        cdesc.shader = cs;
        cdesc.descriptorSetLayout.bindings = {
            { 0, DescriptorType::StorageBuffer, ShaderStage::Compute },
        };
        m_computePipeline = m_device->CreateComputePipeline(cdesc);

        // 描述符集合：把 indirect buffer 绑到 binding 0
        DescriptorSetLayoutDesc layout;
        layout.bindings = { { 0, DescriptorType::StorageBuffer, ShaderStage::Compute } };
        std::vector<DescriptorBufferBinding> bindings = { { 0, m_indirectBuffer.get() } };
        m_descriptorSet = m_device->CreateDescriptorSet(layout, bindings);
    }
}

void RenderScene::Update(const Frontend::Scene& scene) {
    m_camera = scene.camera;
    // 同步对象变换（对象数量需与 Load 时一致）
    for (size_t i = 0; i < m_objects.size() && i < scene.objects.size(); ++i) {
        m_objects[i].transform = scene.objects[i].transform;
    }
}

void RenderScene::Render(uint32_t frameIndex, const std::function<void()>& uiRender) {
    IRHICommandBuffer* cmd = m_device->GetCommandBuffer(frameIndex);

    const uint32_t w = m_device->GetSwapchainWidth();
    const uint32_t h = m_device->GetSwapchainHeight();
    const float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;

    if (m_renderMode == Frontend::RenderMode::Indirect) {
        RenderIndirect(cmd, aspect, w, h, uiRender);
        return;
    }

    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    const Math::Mat4 proj = ComputeProjection(aspect);
    for (const auto& obj : m_objects) {
        // MVP = 投影 * 视图 * 模型（transform 在这里生效）
        const Math::Mat4 mvp = proj * m_camera.View() * obj.transform.Matrix();
        cmd->PushConstants(&mvp, sizeof(mvp));

        cmd->BindVertexBuffer(obj.vertexBuffer.get(), 0);
        if (obj.indexBuffer) {
            cmd->BindIndexBuffer(obj.indexBuffer.get(), 0);
            cmd->DrawIndexed(obj.indexCount);
        } else {
            cmd->Draw(obj.vertexCount);
        }
    }

    if (uiRender) uiRender();
    cmd->EndRenderPass();
}

void RenderScene::RenderIndirect(RHI::IRHICommandBuffer* cmd, float aspect,
                                 uint32_t w, uint32_t h,
                                 const std::function<void()>& uiRender) {
    if (m_objects.empty()) {
        cmd->BeginRenderPass();
        if (uiRender) uiRender();
        cmd->EndRenderPass();
        return;
    }

    // 1) compute 在 render pass 外：生成间接绘制命令（见 cull.comp）
    cmd->BindComputePipeline(m_computePipeline.get());
    cmd->BindDescriptorSet(m_descriptorSet.get());
    cmd->Dispatch(1, 1, 1);

    // 2) 同步：compute 写 indirect buffer → 间接绘制读
    cmd->PipelineBarrier(PipelineStage::Compute, PipelineStage::DrawIndirect);

    // 3) 图形侧：一次间接绘制
    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    const GpuObject& obj = m_objects[0];
    const Math::Mat4 proj = ComputeProjection(aspect);
    const Math::Mat4 mvp = proj * m_camera.View() * obj.transform.Matrix();
    cmd->PushConstants(&mvp, sizeof(mvp));
    cmd->BindVertexBuffer(obj.vertexBuffer.get(), 0);
    if (obj.indexBuffer) {
        cmd->BindIndexBuffer(obj.indexBuffer.get(), 0);
    }
    cmd->DrawIndexedIndirect(m_indirectBuffer.get(), 0, 1, sizeof(DrawIndexedIndirectCommand));

    if (uiRender) uiRender();
    cmd->EndRenderPass();
}

Math::Mat4 RenderScene::ComputeProjection(float aspect) const {
    Math::Mat4 proj = m_camera.Projection(aspect);
    if (!m_camera.screenSpace) {
        // 投影矩阵（Perspective/Orthographic）是 OpenGL 惯例（Y 向上、Z ∈ [-1,1]），
        // 需转换为 Vulkan 裁剪空间（Y 向下、Z ∈ [0,1]）：翻转 Y + 重映射 Z。
        proj = Math::Mat4::GLToVulkanClip() * proj;
    }
    return proj;
}

} // namespace MagicXEngine::Backend

