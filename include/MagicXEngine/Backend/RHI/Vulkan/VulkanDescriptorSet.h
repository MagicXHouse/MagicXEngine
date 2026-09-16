#pragma once
#include "MagicXEngine/Backend/RHI/RHI.h"
#include <vulkan/vulkan.h>

namespace MagicXEngine::RHI {

// 描述符集合封装：按布局创建独立 pool，一次性写入缓冲绑定。
// 注意：这里按 DescriptorSetLayoutDesc 重新创建布局，与计算管线中用相同描述创建的
//       布局「一致定义」→ 兼容，可直接绑定到该管线。
class VulkanDescriptorSet : public IRHIDescriptorSet {
public:
    VulkanDescriptorSet(VkDevice device,
                        const DescriptorSetLayoutDesc& layout,
                        const std::vector<DescriptorBufferBinding>& bindings);
    ~VulkanDescriptorSet() override;

    VkDescriptorSet GetHandle() const { return m_set; }

private:
    VkDevice             m_device;
    VkDescriptorSetLayout m_layout = VK_NULL_HANDLE;
    VkDescriptorPool      m_pool   = VK_NULL_HANDLE;
    VkDescriptorSet       m_set    = VK_NULL_HANDLE;
};

} // namespace MagicXEngine::RHI
