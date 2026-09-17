#include "MagicXEngine/Backend/RHI/Vulkan/VulkanSwapchain.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanHelpers.h"
#include <algorithm>

namespace MagicXEngine::RHI {

VulkanSwapchain::VulkanSwapchain(VkPhysicalDevice physical, VkDevice device, VkSurfaceKHR surface)
    : m_physical(physical), m_device(device), m_surface(surface) {}

VulkanSwapchain::~VulkanSwapchain() { Destroy(); }

bool VulkanSwapchain::IsSurfaceSupported(VkPhysicalDevice physical, VkSurfaceKHR surface) {
    uint32_t formatCount = 0, presentModeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical, surface, &presentModeCount, nullptr);
    return formatCount > 0 && presentModeCount > 0;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseFormat(const std::vector<VkSurfaceFormatKHR>& available) {
    for (const auto& f : available) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return f;
        }
    }
    return available[0];
}

VkPresentModeKHR VulkanSwapchain::ChoosePresentMode(const std::vector<VkPresentModeKHR>& available) {
    for (const auto& m : available) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) return m;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseExtent(const VkSurfaceCapabilitiesKHR& caps,
                                         uint32_t defaultWidth, uint32_t defaultHeight) {
    if (caps.currentExtent.width != UINT32_MAX) {
        return caps.currentExtent;
    }
    VkExtent2D extent = { defaultWidth, defaultHeight };
    extent.width  = std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(extent.height, caps.minImageExtent.height, caps.maxImageExtent.height);
    return extent;
}

void VulkanSwapchain::CreateSwapchain(uint32_t graphicsFamily, uint32_t presentFamily,
                                      uint32_t defaultWidth, uint32_t defaultHeight) {
    VkSurfaceCapabilitiesKHR caps{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical, m_surface, &caps);

    uint32_t formatCount = 0, presentModeCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &formatCount, nullptr);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &presentModeCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical, m_surface, &formatCount, formats.data());
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical, m_surface, &presentModeCount, presentModes.data());

    VkSurfaceFormatKHR format = ChooseFormat(formats);
    VkPresentModeKHR mode     = ChoosePresentMode(presentModes);
    m_extent                  = ChooseExtent(caps, defaultWidth, defaultHeight);
    m_format                  = format.format;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR ci{};
    ci.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface          = m_surface;
    ci.minImageCount    = imageCount;
    ci.imageFormat      = format.format;
    ci.imageColorSpace  = format.colorSpace;
    ci.imageExtent      = m_extent;
    ci.imageArrayLayers = 1;
    ci.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    ci.preTransform     = caps.currentTransform;
    ci.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode      = mode;
    ci.clipped          = VK_TRUE;
    ci.oldSwapchain     = VK_NULL_HANDLE;

    uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
    if (graphicsFamily != presentFamily) {
        ci.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        ci.queueFamilyIndexCount = 2;
        ci.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        ci.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        ci.queueFamilyIndexCount = 0;
        ci.pQueueFamilyIndices   = nullptr;
    }

    VkCheck(vkCreateSwapchainKHR(m_device, &ci, nullptr, &m_swapchain), "vkCreateSwapchainKHR");

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, nullptr);
    m_images.resize(count);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &count, m_images.data());

    m_imageViews.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        VkImageViewCreateInfo vi{};
        vi.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image    = m_images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format   = m_format;
        vi.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                          VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
        vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        vi.subresourceRange.baseMipLevel   = 0;
        vi.subresourceRange.levelCount     = 1;
        vi.subresourceRange.baseArrayLayer = 0;
        vi.subresourceRange.layerCount     = 1;
        VkCheck(vkCreateImageView(m_device, &vi, nullptr, &m_imageViews[i]), "vkCreateImageView");
    }
}

void VulkanSwapchain::CreateDepthResources() {
    VkImageCreateInfo ci{};
    ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType     = VK_IMAGE_TYPE_2D;
    ci.format        = VK_FORMAT_D32_SFLOAT;
    ci.extent        = { m_extent.width, m_extent.height, 1 };
    ci.mipLevels     = 1;
    ci.arrayLayers   = 1;
    ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    ci.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkCheck(vkCreateImage(m_device, &ci, nullptr, &m_depthImage), "vkCreateImage (depth)");

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(m_device, m_depthImage, &memReq);
    VkMemoryAllocateInfo ai{};
    ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize  = memReq.size;
    ai.memoryTypeIndex = FindMemoryType(m_physical, memReq.memoryTypeBits,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkCheck(vkAllocateMemory(m_device, &ai, nullptr, &m_depthMemory), "vkAllocateMemory (depth)");
    VkCheck(vkBindImageMemory(m_device, m_depthImage, m_depthMemory, 0), "vkBindImageMemory (depth)");

    VkImageViewCreateInfo vi{};
    vi.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi.image    = m_depthImage;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format   = VK_FORMAT_D32_SFLOAT;
    vi.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT;
    vi.subresourceRange.baseMipLevel   = 0;
    vi.subresourceRange.levelCount     = 1;
    vi.subresourceRange.baseArrayLayer = 0;
    vi.subresourceRange.layerCount     = 1;
    VkCheck(vkCreateImageView(m_device, &vi, nullptr, &m_depthImageView),
            "vkCreateImageView (depth)");
}

void VulkanSwapchain::CreateFramebuffers(VkRenderPass renderPass) {
    CreateDepthResources();

    m_framebuffers.resize(m_imageViews.size());
    for (size_t i = 0; i < m_imageViews.size(); ++i) {
        VkImageView attachments[] = { m_imageViews[i], m_depthImageView };

        VkFramebufferCreateInfo ci{};
        ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        ci.renderPass      = renderPass;
        ci.attachmentCount = 2;
        ci.pAttachments    = attachments;
        ci.width           = m_extent.width;
        ci.height          = m_extent.height;
        ci.layers          = 1;
        VkCheck(vkCreateFramebuffer(m_device, &ci, nullptr, &m_framebuffers[i]),
                "vkCreateFramebuffer");
    }
}

void VulkanSwapchain::Destroy() {
    for (VkFramebuffer fb : m_framebuffers) {
        if (fb) vkDestroyFramebuffer(m_device, fb, nullptr);
    }
    m_framebuffers.clear();
    for (VkImageView iv : m_imageViews) {
        if (iv) vkDestroyImageView(m_device, iv, nullptr);
    }
    m_imageViews.clear();
    if (m_depthImageView) { vkDestroyImageView(m_device, m_depthImageView, nullptr); m_depthImageView = VK_NULL_HANDLE; }
    if (m_depthImage)     { vkDestroyImage(m_device, m_depthImage, nullptr); m_depthImage = VK_NULL_HANDLE; }
    if (m_depthMemory)    { vkFreeMemory(m_device, m_depthMemory, nullptr); m_depthMemory = VK_NULL_HANDLE; }
    if (m_swapchain) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    m_images.clear();
}

} // namespace MagicXEngine::RHI
