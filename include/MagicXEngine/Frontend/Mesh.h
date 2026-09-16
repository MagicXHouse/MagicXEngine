#pragma once
#include <cstdint>
#include <vector>
#include "MagicXEngine/Core/Math.h"

namespace MagicXEngine::Frontend {

// 顶点：位置 + 颜色（后续可扩展法线 / UV 等）
struct Vertex {
    Math::Vec3 position;
    Math::Vec3 color;
};

// 网格数据：顶点 + 索引（由案例层填写）
struct MeshData {
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
};

} // namespace MagicXEngine::Frontend

