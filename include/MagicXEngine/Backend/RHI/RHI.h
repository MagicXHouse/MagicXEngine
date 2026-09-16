#pragma once
// ===========================================================================
// RHI.h — 抽象渲染硬件接口（Renderer 只依赖这一层，不接触具体图形 API）。
//
// 新增后端（如 OpenGL）的步骤：
//   1. 在 RHITypes.h 的 Backend 枚举中加入对应值；
//   2. 新建 RHI/OpenGL/ 目录，实现 IRHIDevice 等接口；
//   3. 在 CreateDevice() 工厂函数中加入分支。
// ===========================================================================
#include "MagicXEngine/Backend/RHI/RHITypes.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace MagicXEngine::RHI {

class IRHIBuffer;
class IRHIPipeline;
class IRHICommandBuffer;
class IRHIDescriptorSet;

// 平台窗口描述，由窗口层填充后传给 RHI 创建表面。
struct WindowSurface {
    void*    nativeHandle = nullptr; // 目前为 GLFWwindow*，后续可换成 HWND/NSWindow 等
    uint32_t width        = 0;
    uint32_t height       = 0;
};

class IRHIBuffer {
public:
    virtual ~IRHIBuffer() = default;
    virtual void*    Map() = 0;
    virtual void     Unmap() = 0;
    virtual uint64_t GetSize() const = 0;
};

class IRHIPipeline {
public:
    virtual ~IRHIPipeline() = default;
};

// 描述符集合：绑定若干缓冲（SSBO/UBO）供着色器访问。
// 具体缓冲在 IRHIDevice::CreateDescriptorSet 时一次性写入。
class IRHIDescriptorSet {
public:
    virtual ~IRHIDescriptorSet() = default;
};

// 描述符创建时绑定的实际缓冲
struct DescriptorBufferBinding {
    uint32_t    binding = 0;  // 对应 DescriptorSetLayoutDesc 中的 binding 编号
    IRHIBuffer* buffer  = nullptr;
};

class IRHICommandBuffer {
public:
    virtual ~IRHICommandBuffer() = default;

    virtual void BeginRenderPass() = 0;
    virtual void EndRenderPass()   = 0;
    virtual void BindPipeline(IRHIPipeline* pipeline) = 0;
    virtual void BindVertexBuffer(IRHIBuffer* buffer, uint64_t offset = 0) = 0;
    virtual void BindIndexBuffer(IRHIBuffer* buffer, uint64_t offset = 0) = 0;
    virtual void SetViewport(float x, float y, float width, float height,
                             float minDepth = 0.0f, float maxDepth = 1.0f) = 0;
    virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1,
                      uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1,
                             uint32_t firstIndex = 0, int32_t vertexOffset = 0,
                             uint32_t firstInstance = 0) = 0;
    virtual void PushConstants(const void* data, uint32_t size, uint32_t offset = 0) = 0;

    // ---- 计算 / 间接绘制 / 屏障（GPU-Driven） ----
    virtual void BindComputePipeline(IRHIPipeline* pipeline) = 0;
    virtual void BindDescriptorSet(IRHIDescriptorSet* set) = 0;
    virtual void Dispatch(uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1) = 0;
    virtual void DrawIndexedIndirect(IRHIBuffer* indirectBuffer, uint64_t offset,
                                     uint32_t drawCount, uint32_t stride) = 0;
    virtual void PipelineBarrier(PipelineStage src, PipelineStage dst) = 0;
};

class IRHIDevice {
public:
    virtual ~IRHIDevice() = default;

    // ---- 资源创建 ----
    virtual std::unique_ptr<IRHIBuffer>  CreateBuffer(const BufferDesc& desc, const void* initialData = nullptr) = 0;
    virtual std::unique_ptr<IRHIPipeline> CreatePipeline(const PipelineDesc& desc) = 0;
    virtual std::unique_ptr<IRHIPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) = 0;
    virtual std::unique_ptr<IRHIDescriptorSet> CreateDescriptorSet(
        const DescriptorSetLayoutDesc& layout,
        const std::vector<DescriptorBufferBinding>& bindings) = 0;

    // ---- 交换链/帧 ----
    virtual uint32_t GetFramesInFlight() const = 0;
    virtual Format   GetSwapchainFormat() const = 0;
    virtual uint32_t GetSwapchainWidth() const = 0;
    virtual uint32_t GetSwapchainHeight() const = 0;

    // frameIndex: [0, GetFramesInFlight())，imageIndex 由 BeginFrame 输出
    virtual IRHICommandBuffer* GetCommandBuffer(uint32_t frameIndex) = 0;
    virtual void BeginFrame(uint32_t frameIndex, uint32_t& imageIndex) = 0;
    virtual void EndFrame(uint32_t frameIndex, uint32_t imageIndex) = 0;

    virtual void WaitIdle() = 0;
    virtual void Resize(uint32_t width, uint32_t height) = 0;
};

// ---------------------------------------------------------------------------
// 工厂：唯一一处根据后端类型选择实现的地方。
// ---------------------------------------------------------------------------
std::unique_ptr<IRHIDevice> CreateDevice(Backend backend, const WindowSurface& surface);

} // namespace MagicXEngine::RHI

