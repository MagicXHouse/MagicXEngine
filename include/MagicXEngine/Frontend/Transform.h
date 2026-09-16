#pragma once
#include "MagicXEngine/Core/Math.h"

namespace MagicXEngine::Frontend {

// 变换：位置 / 旋转（欧拉角，弧度）/ 缩放 → 模型矩阵（T * R * S）
struct Transform {
    Math::Vec3 position{ 0.0f, 0.0f, 0.0f };
    Math::Vec3 rotation{ 0.0f, 0.0f, 0.0f };
    Math::Vec3 scale{ 1.0f, 1.0f, 1.0f };

    Math::Mat4 Matrix() const {
        return Math::Mat4::Translate(position) *
               Math::Mat4::RotateXYZ(rotation) *
               Math::Mat4::Scale(scale);
    }
};

} // namespace MagicXEngine::Frontend

