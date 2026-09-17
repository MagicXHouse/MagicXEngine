#pragma once
#include <cmath>

// ===========================================================================
// 最小数学库：Vec3 / Mat4（4x4 列主序，与 Vulkan/OpenGL 布局一致）
// 列主序：m[col*4 + row]，即 m[0..3] 为第一列（x 轴）……
// ===========================================================================
namespace MagicXEngine::Math {

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
};

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator*(const Vec3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }

inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 Cross(const Vec3& a, const Vec3& b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
inline Vec3 Normalize(const Vec3& v) {
    const float len = std::sqrt(Dot(v, v));
    return (len > 1e-8f) ? Vec3{ v.x / len, v.y / len, v.z / len } : Vec3{};
}

struct Mat4 {
    float m[16] = {};

    static Mat4 Identity() {
        Mat4 r;
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }
    static Mat4 Translate(const Vec3& t) {
        Mat4 r = Identity();
        r.m[12] = t.x; r.m[13] = t.y; r.m[14] = t.z;
        return r;
    }
    static Mat4 Scale(const Vec3& s) {
        Mat4 r;
        r.m[0] = s.x; r.m[5] = s.y; r.m[10] = s.z; r.m[15] = 1.0f;
        return r;
    }
    // OpenGL 裁剪空间 → Vulkan 裁剪空间：
    //   翻转 Y（NDC Y 向下）+ 重映射 Z（OpenGL [-w,w] → Vulkan [0,w]）
    static Mat4 GLToVulkanClip() {
        Mat4 r;
        r.m[0]  = 1.0f;
        r.m[5]  = -1.0f;
        r.m[10] = 0.5f;
        r.m[14] = 0.5f;
        r.m[15] = 1.0f;
        return r;
    }
    static Mat4 RotateX(float rad) {
        Mat4 r = Identity();
        const float c = std::cos(rad), s = std::sin(rad);
        r.m[5] = c;  r.m[6] = s;
        r.m[9] = -s; r.m[10] = c;
        return r;
    }
    static Mat4 RotateY(float rad) {
        Mat4 r = Identity();
        const float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[8] = s;
        r.m[2] = -s; r.m[10] = c;
        return r;
    }
    static Mat4 RotateZ(float rad) {
        Mat4 r = Identity();
        const float c = std::cos(rad), s = std::sin(rad);
        r.m[0] = c;  r.m[4] = -s;
        r.m[1] = s;  r.m[5] = c;
        return r;
    }
    // 欧拉角 XYZ（弧度），定义在 operator* 之后
    static Mat4 RotateXYZ(const Vec3& rad);
};

// 矩阵乘法（列主序）：c = a * b，c 的第 j 列 = a * (b 的第 j 列)
inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            r.m[col * 4 + row] = sum;
        }
    }
    return r;
}

inline Mat4 Mat4::RotateXYZ(const Vec3& rad) {
    return RotateX(rad.x) * RotateY(rad.y) * RotateZ(rad.z);
}

// 右手系 lookAt 视图矩阵
inline Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    const Vec3 f = Normalize(target - eye);
    const Vec3 s = Normalize(Cross(f, up));
    const Vec3 u = Cross(s, f);
    Mat4 r = Mat4::Identity();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -Dot(s, eye);
    r.m[13] = -Dot(u, eye);
    r.m[14] =  Dot(f, eye);
    return r;
}

// 透视投影（OpenGL 惯例，Y 轴向上；Vulkan 需注意 NDC Y 翻转，demo 用屏幕空间相机规避）
inline Mat4 Perspective(float fovYDeg, float aspect, float nearPlane, float farPlane) {
    constexpr float kPi = 3.14159265358979323846f;
    const float f = 1.0f / std::tan(fovYDeg * 0.5f * kPi / 180.0f);
    Mat4 r;
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return r;
}

// 正交投影（OpenGL 惯例，Y 轴向上，z 映射到 [-1,1]）
inline Mat4 Orthographic(float left, float right, float bottom, float top,
                         float nearPlane, float farPlane) {
    Mat4 r;
    r.m[0]  = 2.0f / (right - left);
    r.m[5]  = 2.0f / (top - bottom);
    r.m[10] = -2.0f / (farPlane - nearPlane);
    r.m[12] = -(right + left) / (right - left);
    r.m[13] = -(top + bottom) / (top - bottom);
    r.m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    r.m[15] = 1.0f;
    return r;
}

} // namespace MagicXEngine::Math

