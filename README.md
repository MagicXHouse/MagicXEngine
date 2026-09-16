# MagicXEngine

一个用于验证图形算法而设计的微型图形引擎。

## 三层架构

```
案例层 Cases ──> 前端 Frontend ──> 后端 Backend ──> RHI (Vulkan)
```

- **案例层（Cases）**：每个 case 一个可执行文件，均可设为 VS 启动项目独立运行。
- **前端（Frontend）**：用户描述场景数据的 API —— 顶点坐标、索引、颜色、transform、相机。
- **后端（Backend）**：`RenderScene` 管理前端传来的数据并驱动渲染；`RHI` 负责与底层图形 API 交互。

数据流：案例填场景 → 前端 Scene → 后端 RenderScene 上传 GPU → RHI 渲染。

## 目录结构

```
MagicXEngine/
├── CMakeLists.txt              # 引擎静态库 + 案例可执行文件
├── cmake/embed_shader.cmake    # SPIR-V 内嵌为 C++ 字节数组
├── shaders/                    # GLSL 源文件
├── third_party/glfw/           # 内置的 GLFW 3.4（离线可编译）
└── src/
    ├── Engine.{h,cpp}          # RunScene：窗口 + RHI 设备 + RenderScene + 渲染循环
    ├── Core/                   # 基础工具（窗口 / 日志 / 数学）
    │   ├── Window.{h,cpp}
    │   ├── Logger.{h,cpp}
    │   └── Math.h              # Vec3 / Mat4 / LookAt / Perspective
    ├── Frontend/               # ★ 前端：用户描述场景
    │   ├── Mesh.h              #   Vertex（位置+颜色）/ MeshData（顶点+索引）
    │   ├── Transform.h         #   位置 / 旋转 / 缩放 → 模型矩阵
    │   ├── Camera.h            #   相机（屏幕空间 / 透视）
    │   └── Scene.h             #   SceneObject / Scene
    ├── Backend/                # ★ 后端
    │   ├── RenderScene.{h,cpp} #   管理前端数据、上传 GPU、每帧渲染
    │   └── RHI/                #   RHI 抽象 + Vulkan 实现
    │       ├── RHITypes.h / RHI.h / RHI.cpp
    │       └── Vulkan/         #   VulkanRHI / Swapchain / Pipeline / Buffer
    └── Cases/                  # ★ 案例层（每个 case 一个可执行文件）
        └── Case01_Triangle.cpp # 彩色三角形
```

## 依赖

- **Vulkan SDK**（提供 vulkan.h、vulkan-1.lib、glslc、校验层）
  - 安装：`winget install KhronosGroup.VulkanSDK`
  - 无需手动设置环境变量：CMake 会自动定位 `C:\VulkanSDK` 下的 SDK；若装在别处，再设置 `VULKAN_SDK` 即可
- **CMake ≥ 3.20** 与任意 C++20 编译器（MSVC / clang / MinGW）
- **GLFW 3.4**：已内置到 `third_party/glfw`，无需联网下载

## 构建（Windows）

```powershell
cd D:\proj\MagicXEngine
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\Case01_Triangle.exe
```

> 生成的解决方案在 `build/MagicXEngine.sln`，可直接用 Visual Studio 打开调试（VS 2026 生成器则产出 `.slnx`）。

## 新增一个案例

1. 在 `src/Cases/` 下新建 `Case02_XXX.cpp`：
   ```cpp
   #include "Engine.h"
   #include "Frontend/Scene.h"
   using namespace MagicXEngine;
   int main() {
       Frontend::Scene scene;
       // 填场景：相机 + 对象（顶点/索引/颜色/transform）
       return RunScene(scene, "Case02 - XXX");
   }
   ```
2. 在 `CMakeLists.txt` 加一行：`add_case(Case02_XXX ${CMAKE_SOURCE_DIR}/src/Cases/Case02_XXX.cpp)`
3. 重新 configure 后，在 VS 里右键 `Case02_XXX` →「设为启动项目」即可独立运行。

## 后端扩展（RHI）

`RenderScene` 只依赖 `Backend/RHI/RHI.h`，不接触 Vulkan 头文件。新增 OpenGL 后端：

1. 在 `Backend/RHI/` 下新建 `OpenGL/`，实现 `IRHIDevice` 等接口；
2. 在 `RHI.cpp` 的 `CreateDevice()` 加入 `case Backend::OpenGL:`；
3. `Engine.cpp` 把 `RHI::Backend::Vulkan` 换成 `RHI::Backend::OpenGL` 即可切换，案例无需改动。

## 说明

- 着色器构建期由 `glslc` 编译为 SPIR-V 并内嵌，运行时不依赖 shader 文件路径。
- 校验层默认开启（`MAGICXENGINE_ENABLE_VALIDATION=ON`）。
- transform 经 MVP 矩阵（push constant）传到着色器，前端设置的位移/旋转/缩放真正生效。
- 交换链在窗口尺寸变化或 `OUT_OF_DATE` 时自动重建。
