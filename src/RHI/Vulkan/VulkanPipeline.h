#pragma once
#include "../RHI.h"
#include <vulkan/vulkan.h>

namespace MagicXEngine::RHI {

// 图形管线封装。当前无描述符布局（三角形 demo 不需要 uniform/纹理）。
class VulkanPipeline : public IRHIPipeline {
public:
    VulkanPipeline(VkDevice device, VkRenderPass renderPass, const PipelineDesc& desc);
    ~VulkanPipeline() override;

    VkPipeline GetHandle() const { return m_pipeline; }

private:
    VkDevice         m_device;
    VkPipeline       m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_layout   = VK_NULL_HANDLE;
};

} // namespace MagicXEngine::RHI
