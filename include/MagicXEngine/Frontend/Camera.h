#pragma once
#include "MagicXEngine/Core/Math.h"

namespace MagicXEngine::Frontend {

// 相机：
//  - screenSpace = true（默认）：屏幕空间相机，view/proj 均为单位阵，顶点直接用 NDC 坐标；
//  - screenSpace = false：透视相机（lookAt + perspective），需要设置 position/target/fov 等。
struct Camera {
    bool      screenSpace = true;
    Math::Vec3 position{ 0.0f, 0.0f, 1.0f };
    Math::Vec3 target{ 0.0f, 0.0f, 0.0f };
    Math::Vec3 up{ 0.0f, 1.0f, 0.0f };
    float     fovYDeg   = 60.0f;
    float     nearPlane = 0.1f;
    float     farPlane  = 100.0f;

    Math::Mat4 View() const {
        if (screenSpace) return Math::Mat4::Identity();
        return Math::LookAt(position, target, up);
    }
    Math::Mat4 Projection(float aspect) const {
        if (screenSpace) return Math::Mat4::Identity();
        return Math::Perspective(fovYDeg, aspect, nearPlane, farPlane);
    }
};

} // namespace MagicXEngine::Frontend

