#include "MagicXEngine/Backend/MeshletBuilder.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

namespace MagicXEngine::Backend {

namespace {

using Edge = uint64_t;

inline Edge MakeEdge(uint32_t a, uint32_t b) {
    if (a > b) std::swap(a, b);
    return (static_cast<Edge>(a) << 32) | static_cast<Edge>(b);
}

} // namespace

BoundingSphere ComputeBoundingSphere(const Frontend::MeshData& mesh) {
    BoundingSphere s;
    if (mesh.vertices.empty()) return s;

    Math::Vec3 center{ 0.0f, 0.0f, 0.0f };
    for (const auto& v : mesh.vertices) center = center + v.position;
    center = center * (1.0f / static_cast<float>(mesh.vertices.size()));

    float radiusSq = 0.0f;
    for (const auto& v : mesh.vertices) {
        const Math::Vec3 d = v.position - center;
        radiusSq = std::max(radiusSq, Dot(d, d));
    }
    s.center = center;
    s.radius = std::sqrt(radiusSq);
    return s;
}

Frontend::MeshData FlattenObjects(const std::vector<Frontend::SceneObject>& objects) {
    Frontend::MeshData combined;
    uint32_t vertexBase = 0;
    for (const auto& obj : objects) {
        const Math::Mat4 model = obj.transform.Matrix();
        for (const auto& v : obj.mesh.vertices) {
            combined.vertices.push_back({ Math::TransformPoint(model, v.position), v.color });
        }
        for (uint32_t idx : obj.mesh.indices) {
            combined.indices.push_back(vertexBase + idx);
        }
        vertexBase += static_cast<uint32_t>(obj.mesh.vertices.size());
    }
    return combined;
}

MeshletBuildResult BuildMeshlets(const Frontend::MeshData& mesh,
                                 uint32_t maxVerts, uint32_t maxTris) {
    MeshletBuildResult result;
    const uint32_t triCount = static_cast<uint32_t>(mesh.indices.size()) / 3;
    if (triCount == 0) return result;

    // 边 → 相邻三角形 映射
    std::unordered_map<Edge, std::vector<uint32_t>> edgeTris;
    edgeTris.reserve(triCount * 2);
    for (uint32_t t = 0; t < triCount; ++t) {
        const uint32_t i0 = mesh.indices[t * 3 + 0];
        const uint32_t i1 = mesh.indices[t * 3 + 1];
        const uint32_t i2 = mesh.indices[t * 3 + 2];
        edgeTris[MakeEdge(i0, i1)].push_back(t);
        edgeTris[MakeEdge(i1, i2)].push_back(t);
        edgeTris[MakeEdge(i2, i0)].push_back(t);
    }

    std::vector<uint8_t> used(triCount, 0);
    std::vector<uint32_t> vertStamp(mesh.vertices.size(), 0);
    uint32_t stamp = 0;

    for (uint32_t seed = 0; seed < triCount; ++seed) {
        if (used[seed]) continue;
        ++stamp;

        std::vector<uint32_t> tris;   // 本 meshlet 的三角形
        std::vector<uint32_t> verts;  // 本 meshlet 的顶点（去重）

        auto wouldFit = [&](uint32_t t) {
            uint32_t newCount = 0;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = mesh.indices[t * 3 + k];
                if (vertStamp[v] != stamp) ++newCount;
            }
            return (verts.size() + newCount) <= maxVerts;
        };
        auto commit = [&](uint32_t t) {
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = mesh.indices[t * 3 + k];
                if (vertStamp[v] != stamp) { vertStamp[v] = stamp; verts.push_back(v); }
            }
            tris.push_back(t);
            used[t] = 1;
        };

        std::queue<uint32_t> q;
        q.push(seed);
        while (!q.empty()) {
            const uint32_t t = q.front();
            q.pop();
            if (used[t]) continue;        // 重复入队兜底
            if (tris.size() >= maxTris) break;
            if (!wouldFit(t)) continue;   // 顶点超限：跳过，留给后续 meshlet
            commit(t);

            for (int k = 0; k < 3; ++k) {
                const Edge e = MakeEdge(mesh.indices[t * 3 + k],
                                        mesh.indices[t * 3 + (k + 1) % 3]);
                auto it = edgeTris.find(e);
                if (it == edgeTris.end()) continue;
                for (uint32_t n : it->second) {
                    if (!used[n] && wouldFit(n)) q.push(n);
                }
            }
        }

        if (tris.empty()) continue; // 理论上不会发生（种子必可加入）

        // ---- 包围球：中心 = 顶点均值，半径 = 最大距离 ----
        Math::Vec3 center{ 0.0f, 0.0f, 0.0f };
        for (uint32_t v : verts) center = center + mesh.vertices[v].position;
        center = center * (1.0f / static_cast<float>(verts.size()));
        float radiusSq = 0.0f;
        for (uint32_t v : verts) {
            const Math::Vec3 d = mesh.vertices[v].position - center;
            radiusSq = std::max(radiusSq, Dot(d, d));
        }

        // ---- 法线锥：轴 = 各面法线之和归一化，cutoff = min(法线·轴) ----
        Math::Vec3 axisSum{ 0.0f, 0.0f, 0.0f };
        for (uint32_t t : tris) {
            const Math::Vec3& p0 = mesh.vertices[mesh.indices[t * 3 + 0]].position;
            const Math::Vec3& p1 = mesh.vertices[mesh.indices[t * 3 + 1]].position;
            const Math::Vec3& p2 = mesh.vertices[mesh.indices[t * 3 + 2]].position;
            axisSum = axisSum + Normalize(Cross(p1 - p0, p2 - p0));
        }
        const Math::Vec3 axis = Normalize(axisSum);
        float cutoff = 1.0f;
        for (uint32_t t : tris) {
            const Math::Vec3& p0 = mesh.vertices[mesh.indices[t * 3 + 0]].position;
            const Math::Vec3& p1 = mesh.vertices[mesh.indices[t * 3 + 1]].position;
            const Math::Vec3& p2 = mesh.vertices[mesh.indices[t * 3 + 2]].position;
            cutoff = std::min(cutoff, Dot(Normalize(Cross(p1 - p0, p2 - p0)), axis));
        }

        // ---- 输出 ----
        Meshlet m;
        m.firstIndex   = static_cast<uint32_t>(result.meshletIndices.size());
        m.indexCount   = static_cast<uint32_t>(tris.size()) * 3;
        m.sphereCenter = center;
        m.sphereRadius = std::sqrt(radiusSq);
        m.coneAxis     = axis;
        m.coneCutoff   = cutoff;
        result.meshlets.push_back(m);

        for (uint32_t t : tris) {
            result.meshletIndices.push_back(mesh.indices[t * 3 + 0]);
            result.meshletIndices.push_back(mesh.indices[t * 3 + 1]);
            result.meshletIndices.push_back(mesh.indices[t * 3 + 2]);
        }
    }

    return result;
}

} // namespace MagicXEngine::Backend
