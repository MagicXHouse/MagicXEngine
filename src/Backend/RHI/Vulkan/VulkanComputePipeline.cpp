#include "MagicXEngine/Backend/RHI/Vulkan/VulkanComputePipeline.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanHelpers.h"

#include <vector>

namespace MagicXEngine::RHI {

VulkanComputePipeline::VulkanComputePipeline(VkDevice device, const ComputePipelineDesc& desc)
    : m_device(device) {
    // ---- 描述符集合布局 ----
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    bindings.reserve(desc.descriptorSetLayout.bindings.size());
    for (const auto& b : desc.descriptorSetLayout.bindings) {
        VkDescriptorSetLayoutBinding v{};
        v.binding         = b.binding;
        v.descriptorType  = ToVulkan(b.type);
        v.descriptorCount = 1;
        v.stageFlags      = ToVulkan(b.stage);
        bindings.push_back(v);
    }

    VkDescriptorSetLayoutCreateInfo setLayoutCI{};
    setLayoutCI.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    setLayoutCI.bindingCount = static_cast<uint32_t>(bindings.size());
    setLayoutCI.pBindings    = bindings.data();
    VkCheck(vkCreateDescriptorSetLayout(m_device, &setLayoutCI, nullptr, &m_setLayout),
            "vkCreateDescriptorSetLayout");

    // ---- 着色器模块 ----
    VkShaderModuleCreateInfo moduleCI{};
    moduleCI.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCI.codeSize = desc.shader.spirv.size() * sizeof(uint32_t);
    moduleCI.pCode    = desc.shader.spirv.data();

    VkShaderModule module = VK_NULL_HANDLE;
    VkCheck(vkCreateShaderModule(m_device, &moduleCI, nullptr, &module), "vkCreateShaderModule");

    VkPipelineShaderStageCreateInfo stageCI{};
    stageCI.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCI.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    stageCI.module = module;
    stageCI.pName  = desc.shader.entryPoint.c_str();

    // ---- 管线布局：描述符集合 + push constant（COMPUTE 阶段） ----
    VkPushConstantRange pushRange{};
    if (desc.pushConstantSize > 0) {
        pushRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushRange.offset     = 0;
        pushRange.size       = desc.pushConstantSize;
    }

    VkPipelineLayoutCreateInfo layoutCI{};
    layoutCI.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutCI.setLayoutCount = 1;
    layoutCI.pSetLayouts    = &m_setLayout;
    if (desc.pushConstantSize > 0) {
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges    = &pushRange;
    }
    VkCheck(vkCreatePipelineLayout(m_device, &layoutCI, nullptr, &m_layout),
            "vkCreatePipelineLayout");

    // ---- 计算管线 ----
    VkComputePipelineCreateInfo pci{};
    pci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pci.stage  = stageCI;
    pci.layout = m_layout;

    try {
        VkCheck(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, 1, &pci, nullptr, &m_pipeline),
                "vkCreateComputePipelines");
    } catch (...) {
        vkDestroyShaderModule(m_device, module, nullptr);
        if (m_layout)    { vkDestroyPipelineLayout(m_device, m_layout, nullptr); m_layout = VK_NULL_HANDLE; }
        if (m_setLayout) { vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr); m_setLayout = VK_NULL_HANDLE; }
        throw;
    }

    // 着色器模块仅在创建管线时需要，之后即可销毁
    vkDestroyShaderModule(m_device, module, nullptr);
}

VulkanComputePipeline::~VulkanComputePipeline() {
    if (m_pipeline)  { vkDestroyPipeline(m_device, m_pipeline, nullptr); m_pipeline = VK_NULL_HANDLE; }
    if (m_layout)    { vkDestroyPipelineLayout(m_device, m_layout, nullptr); m_layout = VK_NULL_HANDLE; }
    if (m_setLayout) { vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr); m_setLayout = VK_NULL_HANDLE; }
}

} // namespace MagicXEngine::RHI
