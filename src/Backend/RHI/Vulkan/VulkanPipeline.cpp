#include "MagicXEngine/Backend/RHI/Vulkan/VulkanPipeline.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanHelpers.h"

namespace MagicXEngine::RHI {

static VkCullModeFlags ToVkCullMode(CullMode m) {
    switch (m) {
        case CullMode::None:  return VK_CULL_MODE_NONE;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:  return VK_CULL_MODE_BACK_BIT;
    }
    return VK_CULL_MODE_NONE;
}

static VkFrontFace ToVkFrontFace(FrontFace f) {
    return (f == FrontFace::Clockwise) ? VK_FRONT_FACE_CLOCKWISE
                                       : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

static VkPolygonMode ToVkPolygonMode(PolygonMode m) {
    switch (m) {
        case PolygonMode::Fill:  return VK_POLYGON_MODE_FILL;
        case PolygonMode::Line:  return VK_POLYGON_MODE_LINE;
        case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
    }
    return VK_POLYGON_MODE_FILL;
}

VulkanPipeline::VulkanPipeline(VkDevice device, VkRenderPass renderPass,
                               const PipelineDesc& desc)
    : m_device(device) {
    // ---- 着色器模块 ----
    std::vector<VkShaderModule> modules;
    modules.reserve(desc.shaders.size());

    std::vector<VkPipelineShaderStageCreateInfo> stages;
    stages.reserve(desc.shaders.size());

    for (const auto& s : desc.shaders) {
        VkShaderModuleCreateInfo ci{};
        ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = s.spirv.size() * sizeof(uint32_t);
        ci.pCode    = s.spirv.data();

        VkShaderModule module = VK_NULL_HANDLE;
        VkCheck(vkCreateShaderModule(m_device, &ci, nullptr, &module), "vkCreateShaderModule");
        modules.push_back(module);

        VkPipelineShaderStageCreateInfo ssi{};
        ssi.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        ssi.stage  = (s.stage == ShaderStage::Vertex) ? VK_SHADER_STAGE_VERTEX_BIT
                                                      : VK_SHADER_STAGE_FRAGMENT_BIT;
        ssi.module = module;
        ssi.pName  = s.entryPoint.c_str();
        stages.push_back(ssi);
    }

    // ---- 顶点输入 ----
    std::vector<VkVertexInputBindingDescription> bindings;
    bindings.reserve(desc.vertexBindings.size());
    for (const auto& b : desc.vertexBindings) {
        VkVertexInputBindingDescription v{};
        v.binding   = b.binding;
        v.stride    = b.stride;
        v.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindings.push_back(v);
    }

    std::vector<VkVertexInputAttributeDescription> attributes;
    attributes.reserve(desc.vertexAttributes.size());
    for (const auto& a : desc.vertexAttributes) {
        VkVertexInputAttributeDescription v{};
        v.location = a.location;
        v.binding  = a.binding;
        v.format   = ToVulkan(a.format);
        v.offset   = a.offset;
        attributes.push_back(v);
    }

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions      = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions    = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ---- 动态视口/裁剪 ----
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = ToVkPolygonMode(desc.polygonMode);
    rasterizer.cullMode                = ToVkCullMode(desc.cullMode);
    rasterizer.frontFace               = ToVkFrontFace(desc.frontFace);
    rasterizer.depthBiasEnable         = VK_FALSE;
    rasterizer.lineWidth               = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable  = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable       = desc.depthTestEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable      = desc.depthWriteEnable ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.stencilTestEnable     = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = desc.blendEnable ? VK_TRUE : VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // ---- 管线布局（push constant：MVP 矩阵） ----
    VkPushConstantRange pushRange{};
    if (desc.pushConstantSize > 0) {
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset     = 0;
        pushRange.size       = desc.pushConstantSize;
    }

    VkPipelineLayoutCreateInfo layoutCI{};
    layoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (desc.pushConstantSize > 0) {
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges    = &pushRange;
    }
    VkCheck(vkCreatePipelineLayout(m_device, &layoutCI, nullptr, &m_layout),
            "vkCreatePipelineLayout");

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount          = static_cast<uint32_t>(stages.size());
    pci.pStages             = stages.data();
    pci.pVertexInputState   = &vertexInput;
    pci.pInputAssemblyState = &inputAssembly;
    pci.pViewportState      = &viewportState;
    pci.pRasterizationState = &rasterizer;
    pci.pMultisampleState   = &multisampling;
    pci.pDepthStencilState  = &depthStencil;
    pci.pColorBlendState    = &colorBlending;
    pci.pDynamicState       = &dynamicState;
    pci.layout              = m_layout;
    pci.renderPass          = renderPass;
    pci.subpass             = 0;

    try {
        VkCheck(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pci, nullptr, &m_pipeline),
                "vkCreateGraphicsPipelines");
    } catch (...) {
        // 创建失败时清理已创建的着色器模块与布局，避免资源泄漏
        for (VkShaderModule m : modules) {
            vkDestroyShaderModule(m_device, m, nullptr);
        }
        if (m_layout) {
            vkDestroyPipelineLayout(m_device, m_layout, nullptr);
            m_layout = VK_NULL_HANDLE;
        }
        throw;
    }

    // 着色器模块仅在创建管线时需要，之后即可销毁
    for (VkShaderModule m : modules) {
        vkDestroyShaderModule(m_device, m, nullptr);
    }
}

VulkanPipeline::~VulkanPipeline() {
    if (m_pipeline) { vkDestroyPipeline(m_device, m_pipeline, nullptr); m_pipeline = VK_NULL_HANDLE; }
    if (m_layout)   { vkDestroyPipelineLayout(m_device, m_layout, nullptr); m_layout = VK_NULL_HANDLE; }
}

} // namespace MagicXEngine::RHI
