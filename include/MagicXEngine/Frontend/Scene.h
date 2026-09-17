#pragma once
#include <vector>
#include "MagicXEngine/Frontend/Camera.h"
#include "MagicXEngine/Frontend/Mesh.h"
#include "MagicXEngine/Frontend/Transform.h"

namespace MagicXEngine::Frontend {

// 渲染模式：Direct = CPU 逐对象直绘；Indirect = GPU 驱动（compute 生成间接绘制命令）
enum class RenderMode {
    Direct = 0,
    Indirect = 1,   // GPU 驱动冒烟测试（compute 写固定命令）
    Meshlet = 2,    // meshlet 逐块视锥剔除 + 间接绘制
    Culled = 3,     // CPU 逐对象视锥剔除 + 直绘（普通剔除基线）
};

// 场景对象：一个网格 + 一个变换
struct SceneObject {
    MeshData  mesh;
    Transform transform;
};

// 场景：前端向后台（RenderScene）传递的数据总集
struct Scene {
    std::vector<SceneObject> objects;
    Camera camera;
    RenderMode renderMode = RenderMode::Direct;
};

} // namespace MagicXEngine::Frontend

