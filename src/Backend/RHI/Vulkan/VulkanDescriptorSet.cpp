#include "MagicXEngine/Backend/RHI/Vulkan/VulkanDescriptorSet.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanBuffer.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanHelpers.h"

#include <stdexcept>
#include <vector>

namespace MagicXEngine::RHI {

VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device,
                                         const DescriptorSetLayoutDesc& layout,
                                         const std::vector<DescriptorBufferBinding>& bindings)
    : m_device(device) {
    if (layout.bindings.empty()) {
        throw std::runtime_error("Descriptor set requires at least one binding");
    }

    // ---- 布局（与计算管线中相同 DescriptorSetLayoutDesc 生成的布局一致定义 → 兼容） ----
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
    layoutBindings.reserve(layout.bindings.size());
    for (const auto& b : layout.bindings) {
        VkDescriptorSetLayoutBinding v{};
        v.binding         = b.binding;
        v.descriptorType  = ToVulkan(b.type);
        v.descriptorCount = 1;
        v.stageFlags      = ToVulkan(b.stage);
        layoutBindings.push_back(v);
    }

    VkDescriptorSetLayoutCreateInfo layoutCI{};
    layoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCI.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    layoutCI.pBindings    = layoutBindings.data();
    VkCheck(vkCreateDescriptorSetLayout(m_device, &layoutCI, nullptr, &m_layout),
            "vkCreateDescriptorSetLayout");

    // ---- pool（每个描述符集合一个独立 pool，销毁时一并释放） ----
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(layout.bindings.size());
    for (const auto& b : layout.bindings) {
        VkDescriptorPoolSize ps{};
        ps.type            = ToVulkan(b.type);
        ps.descriptorCount = 1;
        poolSizes.push_back(ps);
    }

    VkDescriptorPoolCreateInfo poolCI{};
    poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCI.maxSets       = 1;
    poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCI.pPoolSizes    = poolSizes.data();
    VkCheck(vkCreateDescriptorPool(m_device, &poolCI, nullptr, &m_pool),
            "vkCreateDescriptorPool");

    // ---- 分配 set ----
    VkDescriptorSetAllocateInfo allocCI{};
    allocCI.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocCI.descriptorPool     = m_pool;
    allocCI.descriptorSetCount = 1;
    allocCI.pSetLayouts        = &m_layout;
    VkCheck(vkAllocateDescriptorSets(m_device, &allocCI, &m_set),
            "vkAllocateDescriptorSets");

    // ---- 写入缓冲绑定（buffer info 先行收集，避免 vector 重分配导致指针悬空） ----
    std::vector<VkDescriptorBufferInfo> bufferInfos(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        auto* vb = static_cast<VulkanBuffer*>(bindings[i].buffer);
        bufferInfos[i].buffer = vb->GetHandle();
        bufferInfos[i].offset = 0;
        bufferInfos[i].range  = VK_WHOLE_SIZE;
    }

    std::vector<VkWriteDescriptorSet> writes(bindings.size());
    for (size_t i = 0; i < bindings.size(); ++i) {
        // 从布局中反查该 binding 的描述符类型
        DescriptorType type = DescriptorType::StorageBuffer;
        for (const auto& lb : layout.bindings) {
            if (lb.binding == bindings[i].binding) { type = lb.type; break; }
        }

        VkWriteDescriptorSet w{};
        w.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        w.dstSet          = m_set;
        w.dstBinding      = bindings[i].binding;
        w.dstArrayElement = 0;
        w.descriptorCount = 1;
        w.descriptorType  = ToVulkan(type);
        w.pBufferInfo     = &bufferInfos[i];
        writes[i] = w;
    }

    vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

VulkanDescriptorSet::~VulkanDescriptorSet() {
    if (m_pool)   { vkDestroyDescriptorPool(m_device, m_pool, nullptr); m_pool = VK_NULL_HANDLE; }
    if (m_layout) { vkDestroyDescriptorSetLayout(m_device, m_layout, nullptr); m_layout = VK_NULL_HANDLE; }
}

} // namespace MagicXEngine::RHI
