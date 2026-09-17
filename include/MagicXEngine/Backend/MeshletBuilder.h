#pragma once
#include <cstdint>
#include <vector>
#include "MagicXEngine/Core/Math.h"
#include "MagicXEngine/Frontend/Mesh.h"
#include "MagicXEngine/Frontend/Scene.h"

namespace MagicXEngine::Backend {

// ===========================================================================
// Meshlet：一小簇三角形（网格分块），带包围球 + 法线锥，用于 GPU 侧剔除。
// 内存布局与 GPU 侧 std430 一致（48 字节）。
// ===========================================================================
struct Meshlet {
    uint32_t firstIndex = 0;   // 在 meshlet 索引缓冲中的起始索引
    uint32_t indexCount = 0;   // 索引数量（= 三角形数 × 3）
    uint32_t _pad[2] = {};     // 对齐到 16 字节（匹配 std430 vec4 对齐）
    Math::Vec3 sphereCenter;   // 包围球中心
    float sphereRadius = 0.0f; // 包围球半径
    Math::Vec3 coneAxis;       // 法线锥轴（单位向量）
    float coneCutoff = 1.0f;   // cos(半角)：锥内三角形法线与 axis 点积的下界
};
static_assert(sizeof(Meshlet) == 48, "Meshlet 必须是 48 字节（std430 布局）");

// 构建结果：meshlet 描述列表 + 重排后的索引缓冲（仍指向原顶点缓冲）
struct MeshletBuildResult {
    std::vector<Meshlet>    meshlets;
    std::vector<uint32_t>  meshletIndices;
};

// 包围球（模型空间）
struct BoundingSphere {
    Math::Vec3 center;
    float radius = 0.0f;
};

// 计算网格的包围球（中心 = 顶点均值，半径 = 最大距离）
BoundingSphere ComputeBoundingSphere(const Frontend::MeshData& mesh);

// 把多个对象平铺到世界空间的单一网格（用各对象的 transform 变换顶点）
Frontend::MeshData FlattenObjects(const std::vector<Frontend::SceneObject>& objects);

// 贪心 BFS 构建 meshlet（不重排顶点）。maxVerts/maxTris 为每个 meshlet 的顶点/三角形上限。
MeshletBuildResult BuildMeshlets(const Frontend::MeshData& mesh,
                                 uint32_t maxVerts = 64,
                                 uint32_t maxTris = 126);

} // namespace MagicXEngine::Backend
