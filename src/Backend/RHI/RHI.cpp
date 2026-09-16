#include "MagicXEngine/Backend/RHI/RHI.h"
#include "MagicXEngine/Backend/RHI/Vulkan/VulkanRHI.h"

#include <stdexcept>

namespace MagicXEngine::RHI {

// 工厂：根据后端类型创建对应设备。新增 OpenGL 后端时在此加入分支即可。
std::unique_ptr<IRHIDevice> CreateDevice(Backend backend, const WindowSurface& surface) {
    switch (backend) {
        case Backend::Vulkan:
            return std::make_unique<VulkanDevice>(surface);
        case Backend::OpenGL:
            throw std::runtime_error("OpenGL backend is not implemented yet");
        default:
            throw std::runtime_error("Unknown RHI backend");
    }
}

} // namespace MagicXEngine::RHI

