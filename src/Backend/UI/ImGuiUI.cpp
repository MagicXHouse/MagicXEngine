#include "MagicXEngine/Backend/UI/ImGuiUI.h"

#include "MagicXEngine/Backend/RHI/Vulkan/VulkanRHI.h"
#include "MagicXEngine/Core/Window.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"

#include <stdexcept>
#include <string>

namespace MagicXEngine::Backend::UI {

namespace {

void CheckVkResult(VkResult err) {
    if (err != VK_SUCCESS) {
        throw std::runtime_error("ImGui Vulkan error: VkResult=" +
                                 std::to_string(static_cast<int>(err)));
    }
}

} // namespace

struct ImGuiUI::Impl {
    Core::Window&      window;
    RHI::VulkanDevice* device;
    VkDevice           vkDevice       = VK_NULL_HANDLE;
    VkDescriptorPool   descriptorPool = VK_NULL_HANDLE;

    Impl(Core::Window& w, RHI::IRHIDevice* dev)
        : window(w), device(static_cast<RHI::VulkanDevice*>(dev)) {
        // 仅支持 Vulkan 后端（本引擎当前唯一后端）
        ImGui::CreateContext();

        const RHI::VulkanNativeHandles h = device->GetNativeHandles();
        vkDevice = h.device;

        // ImGui 所需 descriptor pool（FREE_DESCRIPTOR_SET_BIT 供后端释放 set）
        VkDescriptorPoolSize poolSize{};
        poolSize.type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 8;

        VkDescriptorPoolCreateInfo poolCI{};
        poolCI.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolCI.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolCI.maxSets       = 8;
        poolCI.poolSizeCount = 1;
        poolCI.pPoolSizes    = &poolSize;
        if (vkCreateDescriptorPool(vkDevice, &poolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("vkCreateDescriptorPool (ImGui) failed");
        }

        ImGui_ImplVulkan_InitInfo init{};
        init.ApiVersion                    = VK_API_VERSION_1_0;
        init.Instance                      = h.instance;
        init.PhysicalDevice                = h.physicalDevice;
        init.Device                        = h.device;
        init.QueueFamily                   = h.graphicsQueueFamily;
        init.Queue                         = h.graphicsQueue;
        init.DescriptorPool                = descriptorPool;
        init.MinImageCount                 = 2;
        init.ImageCount                    = h.imageCount;
        init.PipelineInfoMain.RenderPass   = h.renderPass;
        init.PipelineInfoMain.Subpass      = 0;
        init.PipelineInfoMain.MSAASamples  = VK_SAMPLE_COUNT_1_BIT;
        init.CheckVkResultFn               = &CheckVkResult;
        ImGui_ImplVulkan_Init(&init);
    }

    ~Impl() {
        ImGui_ImplVulkan_Shutdown();
        if (descriptorPool) {
            vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        ImGui::DestroyContext();
    }
};

ImGuiUI::ImGuiUI(Core::Window& window, RHI::IRHIDevice* device)
    : m_impl(new Impl(window, device)) {}

ImGuiUI::~ImGuiUI() {
    delete m_impl;
}

void ImGuiUI::NewFrame(float deltaTime) {
    // 1) 手动喂输入（替代 ImGui_ImplGlfw_NewFrame，避免覆盖 Window 的 GLFW 回调）
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(m_impl->window.GetWidth()),
                            static_cast<float>(m_impl->window.GetHeight()));
    io.DeltaTime = deltaTime;

    double mx = 0.0, my = 0.0;
    m_impl->window.GetMousePos(mx, my);
    io.AddMousePosEvent(static_cast<float>(mx), static_cast<float>(my));
    for (int b = 0; b < 3; ++b) {  // 左/右/中键
        io.AddMouseButtonEvent(b, m_impl->window.IsMouseButtonDown(b));
    }
    const double scroll = m_impl->window.ConsumeScrollY();
    if (scroll != 0.0) {
        io.AddMouseWheelEvent(0.0f, static_cast<float>(scroll));
    }

    // 2) Vulkan 后端 + ImGui 帧开始
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

void ImGuiUI::Render(uint32_t frameIndex) {
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    VkCommandBuffer cmd = m_impl->device->GetCommandBufferHandle(frameIndex);
    ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
}

} // namespace MagicXEngine::Backend::UI
