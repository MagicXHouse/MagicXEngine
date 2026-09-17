#include "MagicXEngine/Backend/RHI/Vulkan/VulkanRHI.h"

#include "MagicXEngine/Backend/RHI/Vulkan/VulkanBuffer.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanComputePipeline.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanDescriptorSet.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanHelpers.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanPipeline.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanSwapchain.h"

#include "MagicXEngine/Core/Logger.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace MagicXEngine::RHI {

// ===========================================================================
// 文件内工具函数
// ===========================================================================
namespace {

const std::vector<const char*> kValidationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> kDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    bool IsComplete() const { return graphics.has_value() && present.has_value(); }
};

QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> families(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

    for (uint32_t i = 0; i < count; ++i) {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }
        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.present = i;
        }
        if (indices.IsComplete()) break;
    }
    return indices;
}

bool CheckValidationLayerSupport() {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    for (const char* name : kValidationLayers) {
        bool found = false;
        for (const auto& l : layers) {
            if (std::strcmp(l.layerName, name) == 0) { found = true; break; }
        }
        if (!found) return false;
    }
    return true;
}

bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> available(count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available.data());

    std::set<std::string> required(kDeviceExtensions.begin(), kDeviceExtensions.end());
    for (const auto& ext : available) {
        required.erase(ext.extensionName);
    }
    return required.empty();
}

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*userData*/) {
    const char* sev = "INFO";
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)   sev = "ERROR";
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) sev = "WARN";
    std::fprintf(stderr, "[Vulkan][%s] %s\n", sev, data->pMessage);
    return VK_FALSE;
}

} // namespace

// ===========================================================================
// VulkanCommandBuffer
// ===========================================================================
void VulkanCommandBuffer::BeginRenderPass() {
    VkRenderPassBeginInfo bi{};
    bi.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    bi.renderPass      = m_ctx->renderPass;
    bi.framebuffer     = m_ctx->swapchain->GetFramebuffer(m_ctx->imageIndex);
    bi.renderArea.offset = { 0, 0 };
    bi.renderArea.extent = m_ctx->swapchain->GetExtent();

    VkClearValue clearValues[2];
    clearValues[0].color        = { { 0.02f, 0.02f, 0.04f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };
    bi.clearValueCount = 2;
    bi.pClearValues    = clearValues;

    vkCmdBeginRenderPass(m_cmd, &bi, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanCommandBuffer::EndRenderPass() {
    vkCmdEndRenderPass(m_cmd);
}

void VulkanCommandBuffer::BindPipeline(IRHIPipeline* pipeline) {
    auto* p = static_cast<VulkanPipeline*>(pipeline);
    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p->GetHandle());
    m_currentLayout    = p->GetLayout();
    m_currentBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    m_currentPushStage = VK_SHADER_STAGE_VERTEX_BIT;
}

void VulkanCommandBuffer::BindVertexBuffer(IRHIBuffer* buffer, uint64_t offset) {
    auto* vb = static_cast<VulkanBuffer*>(buffer);
    VkBuffer handle = vb->GetHandle();
    VkDeviceSize off = static_cast<VkDeviceSize>(offset);
    vkCmdBindVertexBuffers(m_cmd, 0, 1, &handle, &off);
}

void VulkanCommandBuffer::BindIndexBuffer(IRHIBuffer* buffer, uint64_t offset) {
    auto* ib = static_cast<VulkanBuffer*>(buffer);
    vkCmdBindIndexBuffer(m_cmd, ib->GetHandle(), static_cast<VkDeviceSize>(offset),
                         VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::SetViewport(float x, float y, float width, float height,
                                      float minDepth, float maxDepth) {
    VkViewport vp{};
    vp.x = x; vp.y = y; vp.width = width; vp.height = height;
    vp.minDepth = minDepth; vp.maxDepth = maxDepth;
    vkCmdSetViewport(m_cmd, 0, 1, &vp);
}

void VulkanCommandBuffer::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    VkRect2D scissor{};
    scissor.offset = { x, y };
    scissor.extent = { width, height };
    vkCmdSetScissor(m_cmd, 0, 1, &scissor);
}

void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount,
                               uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(m_cmd, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                      uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance) {
    vkCmdDrawIndexed(m_cmd, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::PushConstants(const void* data, uint32_t size, uint32_t offset) {
    vkCmdPushConstants(m_cmd, m_currentLayout, m_currentPushStage, offset, size, data);
}

void VulkanCommandBuffer::BindComputePipeline(IRHIPipeline* pipeline) {
    auto* p = static_cast<VulkanComputePipeline*>(pipeline);
    vkCmdBindPipeline(m_cmd, VK_PIPELINE_BIND_POINT_COMPUTE, p->GetHandle());
    m_currentLayout    = p->GetLayout();
    m_currentBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    m_currentPushStage = VK_SHADER_STAGE_COMPUTE_BIT;
}

void VulkanCommandBuffer::BindDescriptorSet(IRHIDescriptorSet* set) {
    auto* ds = static_cast<VulkanDescriptorSet*>(set);
    VkDescriptorSet handle = ds->GetHandle();
    vkCmdBindDescriptorSets(m_cmd, m_currentBindPoint, m_currentLayout, 0, 1, &handle, 0, nullptr);
}

void VulkanCommandBuffer::Dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) {
    vkCmdDispatch(m_cmd, groupX, groupY, groupZ);
}

void VulkanCommandBuffer::DrawIndexedIndirect(IRHIBuffer* indirectBuffer, uint64_t offset,
                                              uint32_t drawCount, uint32_t stride) {
    auto* ib = static_cast<VulkanBuffer*>(indirectBuffer);
    vkCmdDrawIndexedIndirect(m_cmd, ib->GetHandle(), static_cast<VkDeviceSize>(offset),
                             drawCount, stride);
}

void VulkanCommandBuffer::PipelineBarrier(PipelineStage src, PipelineStage dst) {
    // 简化实现：全局内存屏障。当前唯一用到且支持的转换是
    //   compute 写存储缓冲（SHADER_WRITE）→ 间接绘制读（INDIRECT_COMMAND_READ）。
    VkMemoryBarrier memBarrier{};
    memBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    vkCmdPipelineBarrier(m_cmd,
                         ToVulkan(src), ToVulkan(dst),
                         0,
                         1, &memBarrier,
                         0, nullptr,
                         0, nullptr);
}

// ===========================================================================
// VulkanDevice
// ===========================================================================
VulkanDevice::VulkanDevice(const WindowSurface& surface) : m_surfaceDesc(surface) {
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();

    // 顺序：交换链(确定颜色格式) -> 渲染通道 -> 帧缓冲 -> 渲染上下文 -> 命令池 -> 同步对象
    CreateSwapchain();
    CreateRenderPass();
    m_swapchain->CreateFramebuffers(m_renderPass);
    InitRenderContext();
    CreateCommandPoolAndBuffers();
    CreateSyncObjects();
}

VulkanDevice::~VulkanDevice() {
    if (m_device) {
        vkDeviceWaitIdle(m_device);
    }

    for (VkFence f : m_inFlightFences)     { if (f) vkDestroyFence(m_device, f, nullptr); }
    for (VkSemaphore s : m_renderFinishedSemaphores) { if (s) vkDestroySemaphore(m_device, s, nullptr); }
    for (VkSemaphore s : m_imageAvailableSemaphores) { if (s) vkDestroySemaphore(m_device, s, nullptr); }

    if (m_commandPool) { vkDestroyCommandPool(m_device, m_commandPool, nullptr); m_commandPool = VK_NULL_HANDLE; }
    if (m_renderPass)  { vkDestroyRenderPass(m_device, m_renderPass, nullptr); m_renderPass = VK_NULL_HANDLE; }

    m_swapchain.reset(); // 销毁交换链/图像视图/帧缓冲

    if (m_debugMessenger) {
        auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (fn) fn(m_instance, m_debugMessenger, nullptr);
        m_debugMessenger = VK_NULL_HANDLE;
    }
    if (m_surface)  { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); m_surface = VK_NULL_HANDLE; }
    if (m_device)   { vkDestroyDevice(m_device, nullptr); m_device = VK_NULL_HANDLE; }
    if (m_instance) { vkDestroyInstance(m_instance, nullptr); m_instance = VK_NULL_HANDLE; }
}

void VulkanDevice::CreateInstance() {
#ifdef MAGICXENGINE_ENABLE_VALIDATION
    m_validationEnabled = CheckValidationLayerSupport();
    if (!m_validationEnabled) {
        Core::LogWarn("Vulkan validation layers requested but not available; disabled.");
    }
#endif

    uint32_t glfwExtCount = 0;
    const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);
    if (m_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "MagicXEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "MagicXEngine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo        = &appInfo;
    ci.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    ci.ppEnabledExtensionNames = extensions.data();
    if (m_validationEnabled) {
        ci.enabledLayerCount   = static_cast<uint32_t>(kValidationLayers.size());
        ci.ppEnabledLayerNames = kValidationLayers.data();
    }

    VkCheck(vkCreateInstance(&ci, nullptr, &m_instance), "vkCreateInstance");
}

void VulkanDevice::SetupDebugMessenger() {
    if (!m_validationEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    ci.pfnUserCallback = DebugCallback;

    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (fn) {
        VkCheck(fn(m_instance, &ci, nullptr, &m_debugMessenger),
                "vkCreateDebugUtilsMessengerEXT");
    }
}

void VulkanDevice::CreateSurface() {
    GLFWwindow* window = static_cast<GLFWwindow*>(m_surfaceDesc.nativeHandle);
    VkCheck(glfwCreateWindowSurface(m_instance, window, nullptr, &m_surface),
            "glfwCreateWindowSurface");
}

int VulkanDevice::RateDeviceSuitability(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    auto indices = FindQueueFamilies(device, m_surface);
    if (!indices.IsComplete()) return 0;
    if (!VulkanSwapchain::IsSurfaceSupported(device, m_surface)) return 0;
    if (!CheckDeviceExtensionSupport(device)) return 0;

    int score = 1;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)   score += 1000;
    else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) score += 100;
    score += static_cast<int>(props.limits.maxImageDimension2D) / 1000;
    return score;
}

void VulkanDevice::PickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    VkPhysicalDevice best = VK_NULL_HANDLE;
    int bestScore = 0;
    for (VkPhysicalDevice d : devices) {
        int score = RateDeviceSuitability(d);
        if (score > bestScore) { bestScore = score; best = d; }
    }
    if (best == VK_NULL_HANDLE) throw std::runtime_error("No suitable Vulkan device found");

    m_physicalDevice = best;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(best, &props);
    Core::LogInfo(std::string("Selected GPU: ") + props.deviceName);
}

void VulkanDevice::CreateLogicalDevice() {
    auto indices = FindQueueFamilies(m_physicalDevice, m_surface);
    m_graphicsFamily = indices.graphics.value();
    m_presentFamily  = indices.present.value();

    std::vector<VkDeviceQueueCreateInfo> queueCIs;
    std::set<uint32_t> uniqueFamilies = { m_graphicsFamily, m_presentFamily };
    float priority = 1.0f;
    for (uint32_t family : uniqueFamilies) {
        VkDeviceQueueCreateInfo q{};
        q.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        q.queueFamilyIndex = family;
        q.queueCount       = 1;
        q.pQueuePriorities = &priority;
        queueCIs.push_back(q);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo ci{};
    ci.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount    = static_cast<uint32_t>(queueCIs.size());
    ci.pQueueCreateInfos       = queueCIs.data();
    ci.pEnabledFeatures        = &features;
    ci.enabledExtensionCount   = static_cast<uint32_t>(kDeviceExtensions.size());
    ci.ppEnabledExtensionNames = kDeviceExtensions.data();
    // 设备层自 Vulkan 1.0 起已废弃，必须为 0；校验层只需在实例层启用
    ci.enabledLayerCount       = 0;

    VkCheck(vkCreateDevice(m_physicalDevice, &ci, nullptr, &m_device), "vkCreateDevice");
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
}

void VulkanDevice::CreateSwapchain() {
    m_swapchain = std::make_unique<VulkanSwapchain>(m_physicalDevice, m_device, m_surface);
    m_swapchain->CreateSwapchain(m_graphicsFamily, m_presentFamily,
                                 m_surfaceDesc.width, m_surfaceDesc.height);
}

void VulkanDevice::CreateRenderPass() {
    VkAttachmentDescription color{};
    color.format         = m_swapchain->GetFormat();
    color.samples        = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref{};
    ref.attachment = 0;
    ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depth{};
    depth.format         = VK_FORMAT_D32_SFLOAT;
    depth.samples        = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthRef{};
    depthRef.attachment = 1;
    depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &ref;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass    = 0;
    dep.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = { color, depth };

    VkRenderPassCreateInfo ci{};
    ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ci.attachmentCount = 2;
    ci.pAttachments    = attachments;
    ci.subpassCount    = 1;
    ci.pSubpasses      = &subpass;
    ci.dependencyCount = 1;
    ci.pDependencies   = &dep;
    VkCheck(vkCreateRenderPass(m_device, &ci, nullptr, &m_renderPass), "vkCreateRenderPass");
}

void VulkanDevice::InitRenderContext() {
    m_renderContext.device     = m_device;
    m_renderContext.renderPass = m_renderPass;
    m_renderContext.swapchain  = m_swapchain.get();
}

void VulkanDevice::CreateCommandPoolAndBuffers() {
    VkCommandPoolCreateInfo ci{};
    ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ci.queueFamilyIndex = m_graphicsFamily;
    VkCheck(vkCreateCommandPool(m_device, &ci, nullptr, &m_commandPool), "vkCreateCommandPool");

    m_commandBuffers.resize(kFramesInFlight);
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_commandPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = kFramesInFlight;
    VkCheck(vkAllocateCommandBuffers(m_device, &ai, m_commandBuffers.data()),
            "vkAllocateCommandBuffers");

    m_commandBufferWrappers.clear();
    for (VkCommandBuffer cmd : m_commandBuffers) {
        m_commandBufferWrappers.push_back(
            std::make_unique<VulkanCommandBuffer>(&m_renderContext, cmd));
    }
}

void VulkanDevice::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(kFramesInFlight);
    m_renderFinishedSemaphores.resize(kFramesInFlight);
    m_inFlightFences.resize(kFramesInFlight);

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT; // 首帧即可通过

    for (uint32_t i = 0; i < kFramesInFlight; ++i) {
        VkCheck(vkCreateSemaphore(m_device, &si, nullptr, &m_imageAvailableSemaphores[i]),
                "vkCreateSemaphore");
        VkCheck(vkCreateSemaphore(m_device, &si, nullptr, &m_renderFinishedSemaphores[i]),
                "vkCreateSemaphore");
        VkCheck(vkCreateFence(m_device, &fi, nullptr, &m_inFlightFences[i]),
                "vkCreateFence");
    }
}

void VulkanDevice::RecreateSwapchain() {
    vkDeviceWaitIdle(m_device);
    m_swapchain->Destroy();

    GLFWwindow* window = static_cast<GLFWwindow*>(m_surfaceDesc.nativeHandle);
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    if (w <= 0 || h <= 0) { w = 1; h = 1; } // 最小化时占位

    m_swapchain->CreateSwapchain(m_graphicsFamily, m_presentFamily,
                                 static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    m_swapchain->CreateFramebuffers(m_renderPass);
}

void VulkanDevice::OneTimeSubmit(const std::function<void(VkCommandBuffer)>& fn) {
    VkCommandBufferAllocateInfo ai{};
    ai.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool        = m_commandPool;
    ai.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    ai.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkCheck(vkAllocateCommandBuffers(m_device, &ai, &cmd), "vkAllocateCommandBuffers");

    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCheck(vkBeginCommandBuffer(cmd, &bi), "vkBeginCommandBuffer");

    fn(cmd);

    VkCheck(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    VkSubmitInfo si{};
    si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount = 1;
    si.pCommandBuffers    = &cmd;
    VkCheck(vkQueueSubmit(m_graphicsQueue, 1, &si, VK_NULL_HANDLE), "vkQueueSubmit");
    VkCheck(vkQueueWaitIdle(m_graphicsQueue), "vkQueueWaitIdle");

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &cmd);
}

// ===========================================================================
// IRHIDevice 实现
// ===========================================================================
std::unique_ptr<IRHIBuffer> VulkanDevice::CreateBuffer(const BufferDesc& desc,
                                                       const void* initialData) {
    VkBufferUsageFlags usage = ToVulkan(desc.usage);
    VkMemoryPropertyFlags memProps = desc.cpuAccessible
        ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if (initialData && !desc.cpuAccessible) {
        // 设备本地内存：经 staging 缓冲一次性拷贝上传
        auto staging = std::make_unique<VulkanBuffer>(
            m_device, m_physicalDevice, desc.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            initialData);
        auto result = std::make_unique<VulkanBuffer>(
            m_device, m_physicalDevice, desc.size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, nullptr);
        OneTimeSubmit([&](VkCommandBuffer cmd) {
            VkBufferCopy region{};
            region.size = desc.size;
            vkCmdCopyBuffer(cmd, staging->GetHandle(), result->GetHandle(), 1, &region);
        });
        return result;
    }

    return std::make_unique<VulkanBuffer>(m_device, m_physicalDevice, desc.size,
                                          usage, memProps, initialData);
}

std::unique_ptr<IRHIPipeline> VulkanDevice::CreatePipeline(const PipelineDesc& desc) {
    return std::make_unique<VulkanPipeline>(m_device, m_renderPass, desc);
}

std::unique_ptr<IRHIPipeline> VulkanDevice::CreateComputePipeline(const ComputePipelineDesc& desc) {
    return std::make_unique<VulkanComputePipeline>(m_device, desc);
}

std::unique_ptr<IRHIDescriptorSet> VulkanDevice::CreateDescriptorSet(
    const DescriptorSetLayoutDesc& layout,
    const std::vector<DescriptorBufferBinding>& bindings) {
    return std::make_unique<VulkanDescriptorSet>(m_device, layout, bindings);
}

Format VulkanDevice::GetSwapchainFormat() const {
    return FromVulkan(m_swapchain->GetFormat());
}

uint32_t VulkanDevice::GetSwapchainWidth() const {
    return m_swapchain->GetExtent().width;
}

uint32_t VulkanDevice::GetSwapchainHeight() const {
    return m_swapchain->GetExtent().height;
}

VulkanNativeHandles VulkanDevice::GetNativeHandles() const {
    VulkanNativeHandles h;
    h.instance            = m_instance;
    h.physicalDevice      = m_physicalDevice;
    h.device              = m_device;
    h.graphicsQueue       = m_graphicsQueue;
    h.graphicsQueueFamily = m_graphicsFamily;
    h.renderPass          = m_renderPass;
    h.imageCount          = m_swapchain->GetImageCount();
    return h;
}

VkCommandBuffer VulkanDevice::GetCommandBufferHandle(uint32_t frameIndex) const {
    return m_commandBuffers[frameIndex];
}

IRHICommandBuffer* VulkanDevice::GetCommandBuffer(uint32_t frameIndex) {
    return m_commandBufferWrappers[frameIndex].get();
}

void VulkanDevice::BeginFrame(uint32_t frameIndex, uint32_t& imageIndex) {
    vkWaitForFences(m_device, 1, &m_inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);

    while (true) {
        VkResult res = vkAcquireNextImageKHR(
            m_device, m_swapchain->GetHandle(), UINT64_MAX,
            m_imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            RecreateSwapchain();
            continue;
        }
        VkCheck(res, "vkAcquireNextImageKHR");
        break;
    }

    vkResetFences(m_device, 1, &m_inFlightFences[frameIndex]);
    m_renderContext.imageIndex = imageIndex;

    VkCommandBuffer cmd = m_commandBuffers[frameIndex];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCheck(vkBeginCommandBuffer(cmd, &bi), "vkBeginCommandBuffer");
}

void VulkanDevice::EndFrame(uint32_t frameIndex, uint32_t imageIndex) {
    VkCheck(vkEndCommandBuffer(m_commandBuffers[frameIndex]), "vkEndCommandBuffer");

    VkSemaphore waitSemaphores[]   = { m_imageAvailableSemaphores[frameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[frameIndex] };

    VkSubmitInfo submit{};
    submit.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount   = 1;
    submit.pWaitSemaphores      = waitSemaphores;
    submit.pWaitDstStageMask    = waitStages;
    submit.commandBufferCount   = 1;
    submit.pCommandBuffers      = &m_commandBuffers[frameIndex];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores    = signalSemaphores;
    VkCheck(vkQueueSubmit(m_graphicsQueue, 1, &submit, m_inFlightFences[frameIndex]),
            "vkQueueSubmit");

    VkSwapchainKHR swapchainHandle = m_swapchain->GetHandle();

    VkPresentInfoKHR present{};
    present.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores    = signalSemaphores;
    present.swapchainCount     = 1;
    present.pSwapchains        = &swapchainHandle;
    present.pImageIndices      = &imageIndex;

    VkResult res = vkQueuePresentKHR(m_presentQueue, &present);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    } else {
        VkCheck(res, "vkQueuePresentKHR");
    }
}

void VulkanDevice::WaitIdle() {
    vkDeviceWaitIdle(m_device);
}

void VulkanDevice::Resize(uint32_t /*width*/, uint32_t /*height*/) {
    RecreateSwapchain();
}

} // namespace MagicXEngine::RHI
