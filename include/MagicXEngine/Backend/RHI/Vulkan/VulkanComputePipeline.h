#pragma once
#include "MagicXEngine/Backend/RHI/RHI.h"
#include <vulkan/vulkan.h>

namespace MagicXEngine::RHI {

// 计算管线封装：创建 compute shader 管线 + 描述符集合布局 + push constant（COMPUTE 阶段）。
// 复用 IRHIPipeline 接口，由 IRHIDevice::CreateComputePipeline 构造。
class VulkanComputePipeline : public IRHIPipeline {
public:
    VulkanComputePipeline(VkDevice device, const ComputePipelineDesc& desc);
    ~VulkanComputePipeline() override;

    VkPipeline           GetHandle() const { return m_pipeline; }
    VkPipelineLayout     GetLayout() const { return m_layout; }
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_setLayout; }

private:
    VkDevice              m_device;
    VkPipeline            m_pipeline  = VK_NULL_HANDLE;
    VkPipelineLayout      m_layout    = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
};

} // namespace MagicXEngine::RHI
