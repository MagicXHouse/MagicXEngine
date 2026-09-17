#include "MagicXEngine/Backend/RenderScene.h"

#include "MagicXEngine/Backend/MeshletBuilder.h"
#include "MagicXEngine/Backend/RHI/RHI.h"

#include "triangle_vert_spv.h"
#include "triangle_frag_spv.h"
#include "cull_comp_spv.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace MagicXEngine::Backend {

using namespace RHI;

// 剔除参数（push constant，与 cull.comp 的 PC 布局一致，100 字节）
struct MeshletCullParams {
    Math::Frustum frustum;      // 6 个视锥平面（模型空间，法线向内，已归一化）
    uint32_t      meshletCount;
};
static_assert(sizeof(MeshletCullParams) == 100, "MeshletCullParams 应为 100 字节");

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
        g.bounds    = ComputeBoundingSphere(obj.mesh);
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
    } else if (m_renderMode == Frontend::RenderMode::Meshlet) {
        // ---- meshlet 逐块视锥剔除资源 ----
        if (scene.objects.empty()) return;

        // 1) 平铺所有对象到世界空间的单一网格 + 构建 meshlet
        Frontend::MeshData combined = FlattenObjects(scene.objects);
        const MeshletBuildResult build = BuildMeshlets(combined, 64, 126);
        m_meshletCount = static_cast<uint32_t>(build.meshlets.size());

        // 2) 上传平铺后的顶点缓冲（渲染时作为 vertex buffer）
        BufferDesc vbDesc;
        vbDesc.size          = combined.vertices.size() * sizeof(Frontend::Vertex);
        vbDesc.usage         = BufferUsage::Vertex;
        vbDesc.cpuAccessible = true;
        m_flattenedVertexBuffer = m_device->CreateBuffer(vbDesc, combined.vertices.data());

        // 3) 上传 meshlet 描述 SSBO
        BufferDesc mbDesc;
        mbDesc.size  = build.meshlets.size() * sizeof(Meshlet);
        mbDesc.usage = BufferUsage::Storage;
        m_meshletBuffer = m_device->CreateBuffer(mbDesc, build.meshlets.data());

        // 3) 上传重排后的索引缓冲（渲染时作为 index buffer）
        BufferDesc ibDesc;
        ibDesc.size          = build.meshletIndices.size() * sizeof(uint32_t);
        ibDesc.usage         = BufferUsage::Index;
        ibDesc.cpuAccessible = true;
        m_meshletIndexBuffer = m_device->CreateBuffer(ibDesc, build.meshletIndices.data());

        // 4) 间接命令缓冲（每 meshlet 一条，compute 每帧写入）
        BufferDesc cmdDesc;
        cmdDesc.size  = m_meshletCount * sizeof(DrawIndexedIndirectCommand);
        cmdDesc.usage = BufferUsage::Storage | BufferUsage::Indirect;
        m_indirectBuffer = m_device->CreateBuffer(cmdDesc, nullptr);

        // 5) compute 管线（2 个 SSBO：meshlets + indirect commands）
        ComputePipelineDesc cdesc;
        ShaderDesc cs;
        cs.stage = ShaderStage::Compute;
        cs.spirv.assign(MagicXEngine::Shaders::cull_comp_spv,
                        MagicXEngine::Shaders::cull_comp_spv + MagicXEngine::Shaders::cull_comp_spv_word_count);
        cdesc.shader = cs;
        cdesc.descriptorSetLayout.bindings = {
            { 0, DescriptorType::StorageBuffer, ShaderStage::Compute }, // meshlets
            { 1, DescriptorType::StorageBuffer, ShaderStage::Compute }, // indirect commands
        };
        cdesc.pushConstantSize = sizeof(MeshletCullParams);
        m_computePipeline = m_device->CreateComputePipeline(cdesc);

        // 6) 描述符集合：绑定两个 SSBO
        DescriptorSetLayoutDesc layout;
        layout.bindings = {
            { 0, DescriptorType::StorageBuffer, ShaderStage::Compute },
            { 1, DescriptorType::StorageBuffer, ShaderStage::Compute },
        };
        std::vector<DescriptorBufferBinding> bindings = {
            { 0, m_meshletBuffer.get() },
            { 1, m_indirectBuffer.get() },
        };
        m_descriptorSet = m_device->CreateDescriptorSet(layout, bindings);
    }
}

void RenderScene::Update(const Frontend::Scene& scene) {
    m_camera     = scene.camera;
    m_renderMode = scene.renderMode;  // 支持运行时切换渲染模式
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
    if (m_renderMode == Frontend::RenderMode::Meshlet) {
        RenderMeshlet(cmd, aspect, w, h, uiRender);
        return;
    }
    if (m_renderMode == Frontend::RenderMode::Culled) {
        RenderCulled(cmd, aspect, w, h, uiRender);
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

void RenderScene::RenderMeshlet(RHI::IRHICommandBuffer* cmd, float aspect,
                                uint32_t w, uint32_t h,
                                const std::function<void()>& uiRender) {
    if (m_objects.empty() || m_meshletCount == 0) {
        cmd->BeginRenderPass();
        if (uiRender) uiRender();
        cmd->EndRenderPass();
        return;
    }

    // 1) compute 在 render pass 外：逐 meshlet 视锥剔除（世界空间）
    {
        MeshletCullParams params{};
        const Math::Mat4 rawProj = m_camera.Projection(aspect);
        params.frustum      = Math::ExtractFrustumPlanes(rawProj * m_camera.View());
        params.meshletCount = m_meshletCount;

        cmd->BindComputePipeline(m_computePipeline.get());
        cmd->BindDescriptorSet(m_descriptorSet.get());
        cmd->PushConstants(&params, sizeof(params));
        cmd->Dispatch((m_meshletCount + 63) / 64, 1, 1);
    }

    // 2) 同步：compute 写 indirect buffer → 间接绘制读
    cmd->PipelineBarrier(PipelineStage::Compute, PipelineStage::DrawIndirect);

    // 3) 图形侧：一次间接绘制所有 meshlet
    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    const Math::Mat4 mvp = ComputeProjection(aspect) * m_camera.View();  // 网格已在世界空间
    cmd->PushConstants(&mvp, sizeof(mvp));
    cmd->BindVertexBuffer(m_flattenedVertexBuffer.get(), 0);
    cmd->BindIndexBuffer(m_meshletIndexBuffer.get(), 0);
    cmd->DrawIndexedIndirect(m_indirectBuffer.get(), 0, m_meshletCount, sizeof(DrawIndexedIndirectCommand));

    if (uiRender) uiRender();
    cmd->EndRenderPass();
}

void RenderScene::RenderCulled(RHI::IRHICommandBuffer* cmd, float aspect,
                               uint32_t w, uint32_t h,
                               const std::function<void()>& uiRender) {
    // 世界空间视锥（普通剔除：对象包围球变换到世界空间后测试）
    const Math::Mat4 rawProj = m_camera.Projection(aspect);
    const Math::Frustum frustum = Math::ExtractFrustumPlanes(rawProj * m_camera.View());
    const Math::Mat4 proj = ComputeProjection(aspect);

    cmd->BeginRenderPass();
    cmd->BindPipeline(m_pipeline.get());
    cmd->SetViewport(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h));
    cmd->SetScissor(0, 0, w, h);

    for (const auto& obj : m_objects) {
        const Math::Mat4 model = obj.transform.Matrix();
        const Math::Vec3 centerW = Math::TransformPoint(model, obj.bounds.center);
        const float maxScale = std::max({ std::abs(obj.transform.scale.x),
                                          std::abs(obj.transform.scale.y),
                                          std::abs(obj.transform.scale.z) });
        const float radiusW = obj.bounds.radius * maxScale;
        if (Math::SphereOutsideFrustum(frustum, centerW, radiusW)) continue;

        const Math::Mat4 mvp = proj * m_camera.View() * model;
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

