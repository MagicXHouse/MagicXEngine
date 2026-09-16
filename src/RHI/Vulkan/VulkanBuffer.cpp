#include "VulkanBuffer.h"
#include "VulkanHelpers.h"
#include <cstring>

namespace MagicXEngine::RHI {

VulkanBuffer::VulkanBuffer(VkDevice device, VkPhysicalDevice physical,
                           VkDeviceSize size, VkBufferUsageFlags usage,
                           VkMemoryPropertyFlags memoryProps,
                           const void* initialData)
    : m_device(device), m_physical(physical), m_size(size) {
    VkBufferCreateInfo ci{};
    ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    ci.size        = size;
    ci.usage       = usage;
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkCheck(vkCreateBuffer(m_device, &ci, nullptr, &m_buffer), "vkCreateBuffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_device, m_buffer, &memReq);

    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = memReq.size;
    ai.memoryTypeIndex = FindMemoryType(m_physical, memReq.memoryTypeBits, memoryProps);
    VkCheck(vkAllocateMemory(m_device, &ai, nullptr, &m_memory), "vkAllocateMemory");
    VkCheck(vkBindBufferMemory(m_device, m_buffer, m_memory, 0), "vkBindBufferMemory");

    if (initialData) {
        void* ptr = Map();
        std::memcpy(ptr, initialData, static_cast<size_t>(size));
        Unmap();
    }
}

VulkanBuffer::~VulkanBuffer() {
    if (m_memory) { vkFreeMemory(m_device, m_memory, nullptr); m_memory = VK_NULL_HANDLE; }
    if (m_buffer) { vkDestroyBuffer(m_device, m_buffer, nullptr); m_buffer = VK_NULL_HANDLE; }
}

void* VulkanBuffer::Map() {
    void* ptr = nullptr;
    VkCheck(vkMapMemory(m_device, m_memory, 0, m_size, 0, &ptr), "vkMapMemory");
    return ptr;
}

void VulkanBuffer::Unmap() {
    vkUnmapMemory(m_device, m_memory);
}

uint64_t VulkanBuffer::GetSize() const { return m_size; }

} // namespace MagicXEngine::RHI
