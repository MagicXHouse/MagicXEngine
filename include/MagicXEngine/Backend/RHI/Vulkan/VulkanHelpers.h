#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include <string>
#include "MagicXEngine/Backend/RHI/RHITypes.h"

namespace MagicXEngine::RHI {

// 引擎枚举 -> Vulkan 枚举
VkFormat            ToVulkan(Format format);
VkBufferUsageFlags  ToVulkan(BufferUsage usage);
// Vulkan 枚举 -> 引擎枚举
Format              FromVulkan(VkFormat format);

// 校验 VkResult，失败抛异常
void VkCheck(VkResult result, const std::string& what);

// 从设备内存中挑选满足属性要求的内存类型
uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

} // namespace MagicXEngine::RHI

