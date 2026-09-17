#pragma once
#include "MagicXEngine/Backend/RHI/RHI.h"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace MagicXEngine::RHI {

class VulkanSwapchain;

// 渲染上下文：命令缓冲录制时需要的共享状态（由 VulkanDevice 维护）。
struct VulkanRenderContext {
    VkDevice         device     = VK_NULL_HANDLE;
    VkRenderPass     renderPass = VK_NULL_HANDLE;
    VulkanSwapchain* swapchain  = nullptr;
    uint32_t         imageIndex = 0;
};

// 供 ImGui 等 Vulkan 专用插件获取的原生句柄。
struct VulkanNativeHandles {
    VkInstance       instance            = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice      = VK_NULL_HANDLE;
    VkDevice         device              = VK_NULL_HANDLE;
    VkQueue          graphicsQueue       = VK_NULL_HANDLE;
    uint32_t         graphicsQueueFamily = 0;
    VkRenderPass     renderPass          = VK_NULL_HANDLE;
    uint32_t         imageCount          = 0;
};

class VulkanCommandBuffer : public IRHICommandBuffer {
public:
    VulkanCommandBuffer(VulkanRenderContext* ctx, VkCommandBuffer cmd)
        : m_ctx(ctx), m_cmd(cmd) {}

    void BeginRenderPass() override;
    void EndRenderPass() override;
    void BindPipeline(IRHIPipeline* pipeline) override;
    void BindVertexBuffer(IRHIBuffer* buffer, uint64_t offset) override;
    void BindIndexBuffer(IRHIBuffer* buffer, uint64_t offset) override;
    void SetViewport(float x, float y, float width, float height,
                     float minDepth, float maxDepth) override;
    void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;
    void Draw(uint32_t vertexCount, uint32_t instanceCount,
              uint32_t firstVertex, uint32_t firstInstance) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                     uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
    void PushConstants(const void* data, uint32_t size, uint32_t offset) override;
    void BindComputePipeline(IRHIPipeline* pipeline) override;
    void BindDescriptorSet(IRHIDescriptorSet* set) override;
    void Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) override;
    void DrawIndexedIndirect(IRHIBuffer* indirectBuffer, uint64_t offset,
                             uint32_t drawCount, uint32_t stride) override;
    void PipelineBarrier(PipelineStage src, PipelineStage dst) override;

    VkCommandBuffer GetHandle() const { return m_cmd; }

private:
    VulkanRenderContext* m_ctx;
    VkCommandBuffer      m_cmd;
    VkPipelineLayout     m_currentLayout    = VK_NULL_HANDLE;
    VkPipelineBindPoint  m_currentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VkShaderStageFlags   m_currentPushStage = VK_SHADER_STAGE_VERTEX_BIT;
};

class VulkanDevice : public IRHIDevice {
public:
    explicit VulkanDevice(const WindowSurface& surface);
    ~VulkanDevice() override;

    // IRHIDevice
    std::unique_ptr<IRHIBuffer>   CreateBuffer(const BufferDesc& desc, const void* initialData) override;
    std::unique_ptr<IRHIPipeline> CreatePipeline(const PipelineDesc& desc) override;
    std::unique_ptr<IRHIPipeline> CreateComputePipeline(const ComputePipelineDesc& desc) override;
    std::unique_ptr<IRHIDescriptorSet> CreateDescriptorSet(
        const DescriptorSetLayoutDesc& layout,
        const std::vector<DescriptorBufferBinding>& bindings) override;

    uint32_t GetFramesInFlight() const override { return kFramesInFlight; }
    Format   GetSwapchainFormat() const override;
    uint32_t GetSwapchainWidth() const override;
    uint32_t GetSwapchainHeight() const override;

    // 原生句柄（供 ImGui 等 Vulkan 专用插件使用）
    VulkanNativeHandles GetNativeHandles() const;
    VkCommandBuffer     GetCommandBufferHandle(uint32_t frameIndex) const;

    IRHICommandBuffer* GetCommandBuffer(uint32_t frameIndex) override;
    void BeginFrame(uint32_t frameIndex, uint32_t& imageIndex) override;
    void EndFrame(uint32_t frameIndex, uint32_t imageIndex) override;
    void WaitIdle() override;
    void Resize(uint32_t width, uint32_t height) override;

private:
    void CreateInstance();
    void SetupDebugMessenger();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapchain();
    void CreateRenderPass();
    void InitRenderContext();
    void CreateCommandPoolAndBuffers();
    void CreateSyncObjects();
    void RecreateSwapchain();

    void OneTimeSubmit(const std::function<void(VkCommandBuffer)>& fn);
    int  RateDeviceSuitability(VkPhysicalDevice device);

    const WindowSurface& m_surfaceDesc;

    VkInstance               m_instance       = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR             m_surface        = VK_NULL_HANDLE;
    VkPhysicalDevice         m_physicalDevice = VK_NULL_HANDLE;
    VkDevice                 m_device         = VK_NULL_HANDLE;
    VkQueue                  m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue                  m_presentQueue   = VK_NULL_HANDLE;
    uint32_t                 m_graphicsFamily = 0;
    uint32_t                 m_presentFamily  = 0;
    bool                     m_validationEnabled = false;

    std::unique_ptr<VulkanSwapchain> m_swapchain;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<std::unique_ptr<VulkanCommandBuffer>> m_commandBufferWrappers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence>     m_inFlightFences;

    VulkanRenderContext m_renderContext;

    static constexpr uint32_t kFramesInFlight = 2;
};

} // namespace MagicXEngine::RHI

