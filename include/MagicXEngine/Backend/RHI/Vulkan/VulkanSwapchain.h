#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace MagicXEngine::RHI {

// 交换链封装：负责交换链、图像视图、帧缓冲的创建/重建。
class VulkanSwapchain {
public:
    VulkanSwapchain(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface);
    ~VulkanSwapchain();

    // 第一步：创建交换链 + 图像视图（确定颜色格式）
    void CreateSwapchain(uint32_t graphicsFamily, uint32_t presentFamily,
                         uint32_t defaultWidth, uint32_t defaultHeight);
    // 第二步：基于渲染通道创建帧缓冲（在 render pass 创建之后调用）
    void CreateFramebuffers(VkRenderPass renderPass);
    // 销毁所有资源（幂等）
    void Destroy();

    VkSwapchainKHR  GetHandle() const { return m_swapchain; }
    VkFormat        GetFormat() const { return m_format; }
    VkExtent2D      GetExtent() const { return m_extent; }
    uint32_t        GetImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    VkFramebuffer   GetFramebuffer(uint32_t index) const { return m_framebuffers[index]; }

    // 该物理设备是否支持在给定表面上显示（有可用格式与呈现模式）
    static bool IsSurfaceSupported(VkPhysicalDevice physical, VkSurfaceKHR surface);

private:
    VkSurfaceFormatKHR ChooseFormat(const std::vector<VkSurfaceFormatKHR>& available);
    VkPresentModeKHR   ChoosePresentMode(const std::vector<VkPresentModeKHR>& available);
    VkExtent2D         ChooseExtent(const VkSurfaceCapabilitiesKHR& caps,
                                    uint32_t defaultWidth, uint32_t defaultHeight);

    VkPhysicalDevice m_physical;
    VkDevice         m_device;
    VkSurfaceKHR     m_surface;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat       m_format    = VK_FORMAT_UNDEFINED;
    VkExtent2D     m_extent{};

    std::vector<VkImage>       m_images;
    std::vector<VkImageView>   m_imageViews;
    std::vector<VkFramebuffer> m_framebuffers;
};

} // namespace MagicXEngine::RHI

