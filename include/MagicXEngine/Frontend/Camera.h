#pragma once
#include "MagicXEngine/Core/Math.h"

namespace MagicXEngine::Frontend {

// 相机：
//  - screenSpace = true（默认）：屏幕空间相机，view/proj 均为单位阵，顶点直接用 NDC 坐标；
//  - screenSpace = false：透视相机（lookAt + perspective），需要设置 position/target/fov 等。
struct Camera {
    bool      screenSpace = true;
    bool      orthographic = false;  // true: 正交投影（透视相机下有效）
    Math::Vec3 position{ 0.0f, 0.0f, 1.0f };
    Math::Vec3 target{ 0.0f, 0.0f, 0.0f };
    Math::Vec3 up{ 0.0f, 1.0f, 0.0f };
    float     fovYDeg   = 60.0f;
    float     nearPlane = 0.1f;
    float     farPlane  = 100.0f;
    float     orthoSize = 2.0f;      // 正交投影的可见高度（世界单位）

    Math::Mat4 View() const {
        if (screenSpace) return Math::Mat4::Identity();
        return Math::LookAt(position, target, up);
    }
    Math::Mat4 Projection(float aspect) const {
        if (screenSpace) return Math::Mat4::Identity();
        if (orthographic) {
            const float halfH = orthoSize * 0.5f;
            const float halfW = halfH * aspect;
            return Math::Orthographic(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
        }
        return Math::Perspective(fovYDeg, aspect, nearPlane, farPlane);
    }
};

} // namespace MagicXEngine::Frontend

