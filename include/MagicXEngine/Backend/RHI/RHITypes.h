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
    Compute = 2,
};

enum class BufferUsage : uint32_t {
    Vertex      = 1u << 0,
    Index       = 1u << 1,
    Uniform     = 1u << 2,
    TransferSrc = 1u << 3,
    TransferDst = 1u << 4,
    Storage     = 1u << 5,
    Indirect    = 1u << 6,
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
    uint32_t    pushConstantSize = 0; // >0: 顶点阶段 push constant 大小（字节），如 MVP 矩阵
};

struct BufferDesc {
    uint64_t    size         = 0;
    BufferUsage usage        = BufferUsage::Vertex;
    bool        cpuAccessible = false;  // true: 主机可见内存（可 Map/Unmap）
};

// ===========================================================================
// 计算管线与描述符（GPU-Driven 基础：SSBO / compute / 间接绘制）
// ===========================================================================

// 描述符类型（当前只支持缓冲类）
enum class DescriptorType {
    StorageBuffer = 0,  // 结构化存储缓冲（SSBO，可读写）
    UniformBuffer = 1,  // 只读常量缓冲（UBO）
};

// 描述符集合中的单个绑定声明
struct DescriptorBinding {
    uint32_t       binding = 0;
    DescriptorType type    = DescriptorType::StorageBuffer;
    ShaderStage    stage   = ShaderStage::Compute; // 该绑定可见的着色器阶段
};

// 描述符集合布局：一组绑定
struct DescriptorSetLayoutDesc {
    std::vector<DescriptorBinding> bindings;
};

// 计算管线描述（与图形管线 PipelineDesc 并列）
struct ComputePipelineDesc {
    ShaderDesc            shader;              // 单个 compute shader
    DescriptorSetLayoutDesc descriptorSetLayout;
    uint32_t              pushConstantSize = 0;
};

// 简化的管线阶段（用于命令缓冲屏障；后续按需扩展）
enum class PipelineStage {
    Compute = 0,
    DrawIndirect = 1,
};

// 索引间接绘制命令（与 VkDrawIndexedIndirectCommand / D3D12 DrawIndexedArguments 布局一致，20 字节）
struct DrawIndexedIndirectCommand {
    uint32_t indexCount    = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex    = 0;
    int32_t  vertexOffset  = 0;
    uint32_t firstInstance = 0;
};

} // namespace MagicXEngine::RHI

