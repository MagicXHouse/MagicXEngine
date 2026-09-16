#pragma once
#include <vector>
#include "MagicXEngine/Frontend/Camera.h"
#include "MagicXEngine/Frontend/Mesh.h"
#include "MagicXEngine/Frontend/Transform.h"

namespace MagicXEngine::Frontend {

// 场景对象：一个网格 + 一个变换
struct SceneObject {
    MeshData  mesh;
    Transform transform;
};

// 场景：前端向后台（RenderScene）传递的数据总集
struct Scene {
    std::vector<SceneObject> objects;
    Camera camera;
};

} // namespace MagicXEngine::Frontend

