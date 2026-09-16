#pragma once
#include "../RHI.h"
#include <vulkan/vulkan.h>

namespace MagicXEngine::RHI {

// Vulkan 缓冲实现。构造时若传入 initialData 且内存主机可见，则直接映射拷贝上传；
// 设备本地内存 + 初始数据的上传由 VulkanDevice::CreateBuffer 通过 staging 完成。
class VulkanBuffer : public IRHIBuffer {
public:
    VulkanBuffer(VkDevice device, VkPhysicalDevice physical,
                 VkDeviceSize size, VkBufferUsageFlags usage,
                 VkMemoryPropertyFlags memoryProps,
                 const void* initialData);
    ~VulkanBuffer() override;

    void*    Map() override;
    void     Unmap() override;
    uint64_t GetSize() const override;

    VkBuffer GetHandle() const { return m_buffer; }

private:
    VkDevice         m_device;
    VkPhysicalDevice m_physical;
    VkBuffer         m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory   m_memory = VK_NULL_HANDLE;
    VkDeviceSize     m_size   = 0;
};

} // namespace MagicXEngine::RHI
