#include "VulkanHelpers.h"

namespace MagicXEngine::RHI {

VkFormat ToVulkan(Format format) {
    switch (format) {
        case Format::R8G8B8A8_UNORM:     return VK_FORMAT_R8G8B8A8_UNORM;
        case Format::B8G8R8A8_UNORM:     return VK_FORMAT_B8G8R8A8_UNORM;
        case Format::R16G16B16A16_SFLOAT: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Format::R32_SFLOAT:         return VK_FORMAT_R32_SFLOAT;
        case Format::R32G32_SFLOAT:      return VK_FORMAT_R32G32_SFLOAT;
        case Format::R32G32B32_SFLOAT:   return VK_FORMAT_R32G32B32_SFLOAT;
        case Format::D32_SFLOAT:         return VK_FORMAT_D32_SFLOAT;
        case Format::D24_UNORM_S8_UINT:  return VK_FORMAT_D24_UNORM_S8_UINT;
        default:                         return VK_FORMAT_UNDEFINED;
    }
}

VkBufferUsageFlags ToVulkan(BufferUsage usage) {
    VkBufferUsageFlags flags = 0;
    if (usage & BufferUsage::Vertex)      flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (usage & BufferUsage::Index)       flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (usage & BufferUsage::Uniform)     flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (usage & BufferUsage::TransferSrc) flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    if (usage & BufferUsage::TransferDst) flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    return flags;
}

Format FromVulkan(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:      return Format::R8G8B8A8_UNORM;
        case VK_FORMAT_B8G8R8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_SRGB:       return Format::B8G8R8A8_UNORM;
        case VK_FORMAT_R16G16B16A16_SFLOAT: return Format::R16G16B16A16_SFLOAT;
        case VK_FORMAT_R32_SFLOAT:          return Format::R32_SFLOAT;
        case VK_FORMAT_R32G32_SFLOAT:       return Format::R32G32_SFLOAT;
        case VK_FORMAT_R32G32B32_SFLOAT:    return Format::R32G32B32_SFLOAT;
        case VK_FORMAT_D32_SFLOAT:          return Format::D32_SFLOAT;
        case VK_FORMAT_D24_UNORM_S8_UINT:   return Format::D24_UNORM_S8_UINT;
        default:                            return Format::Undefined;
    }
}

void VkCheck(VkResult result, const std::string& what) {
    if (result != VK_SUCCESS) {
        throw std::runtime_error(what + " failed with VkResult=" +
                                 std::to_string(static_cast<int>(result)));
    }
}

uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(device, &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("Failed to find suitable memory type");
}

} // namespace MagicXEngine::RHI
