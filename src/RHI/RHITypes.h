#pragma once
// ===========================================================================
// RHITypes.h — 后端无关的共享类型/枚举（不包含任何 Vulkan/OpenGL 头文件）。
// 未来新增 RHI 后端（OpenGL/Metal/D3D12）时，此文件保持不变。
// ===========================================================================
#include <cstdint>
#include <string>
#include <vector>

namespace MagicXEngine::RHI {

enum class Backend {
    Vulkan,
    OpenGL,   // 预留：后续扩展
};

enum class Format {
    Undefined = 0,
    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,
    R16G16B16A16_SFLOAT,
    R32_SFLOAT,
    R32G32_SFLOAT,
    R32G32B32_SFLOAT,
    D32_SFLOAT,
    D24_UNORM_S8_UINT,
};

enum class ShaderStage {
    Vertex = 0,
    Fragment = 1,
};

enum class BufferUsage : uint32_t {
    Vertex      = 1u << 0,
    Index       = 1u << 1,
    Uniform     = 1u << 2,
    TransferSrc = 1u << 3,
    TransferDst = 1u << 4,
};

inline BufferUsage operator|(BufferUsage a, BufferUsage b) {
    return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool operator&(BufferUsage a, BufferUsage b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

enum class CullMode { None, Front, Back };
enum class FrontFace { CounterClockwise, Clockwise };
enum class PolygonMode { Fill, Line, Point };

struct VertexAttribute {
    uint32_t location = 0;
    uint32_t binding  = 0;
    Format   format   = Format::Undefined;
    uint32_t offset   = 0;
};

struct VertexBinding {
    uint32_t binding = 0;
    uint32_t stride  = 0;
};

struct ShaderDesc {
    ShaderStage          stage      = ShaderStage::Vertex;
    std::vector<uint32_t> spirv;       // SPIR-V 字节码
    std::string          entryPoint = "main";
};

struct PipelineDesc {
    std::vector<ShaderDesc>     shaders;
    std::vector<VertexBinding>  vertexBindings;
    std::vector<VertexAttribute> vertexAttributes;
    CullMode    cullMode    = CullMode::None;
    FrontFace   frontFace   = FrontFace::CounterClockwise;
    PolygonMode polygonMode = PolygonMode::Fill;
    bool        blendEnable = false;
};

struct BufferDesc {
    uint64_t    size         = 0;
    BufferUsage usage        = BufferUsage::Vertex;
    bool        cpuAccessible = false;  // true: 主机可见内存（可 Map/Unmap）
};

} // namespace MagicXEngine::RHI
