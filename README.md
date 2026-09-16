# MagicXEngine

一个用于验证图形算法而设计的微型图形引擎。

## 目录结构

```
MagicXEngine/
├── CMakeLists.txt            # 构建脚本（自动拉取 GLFW、用 glslc 编译并内嵌 shader）
├── cmake/embed_shader.cmake  # 把 SPIR-V 内嵌为 C++ 字节数组
├── shaders/                  # GLSL 源文件（triangle.vert / triangle.frag）
└── src/
    ├── main.cpp              # 入口
    ├── Engine.{h,cpp}        # 窗口 + RHI 设备 + 渲染器的生命周期与帧循环
    ├── Core/
    │   ├── Window.{h,cpp}    # 窗口抽象（内部用 GLFW，可换 SDL/Win32）
    │   └── Logger.{h,cpp}
    ├── RHI/                  # ★ 渲染硬件抽象层（Renderer 只依赖这里）
    │   ├── RHITypes.h        # 后端无关的枚举/结构（不包含任何图形 API 头）
    │   ├── RHI.h             # IRHIDevice / IRHIBuffer / IRHIPipeline / IRHICommandBuffer
    │   ├── RHI.cpp           # CreateDevice() 工厂（选择后端）
    │   └── Vulkan/           # Vulkan 实现
    │       ├── VulkanRHI.{h,cpp}      # 实例/设备/交换链/命令缓冲/同步/帧循环
    │       ├── VulkanSwapchain.{h,cpp}
    │       ├── VulkanPipeline.{h,cpp}
    │       ├── VulkanBuffer.{h,cpp}
    │       └── VulkanHelpers.{h,cpp}  # 枚举映射 / 内存类型 / VkResult 校验
    └── Renderer/
        └── TriangleRenderer.{h,cpp}    # 高层渲染器（三角形 Demo）
```

## 依赖

- **Vulkan SDK**（提供 vulkan.h、vulkan-1.lib、glslc、校验层）
  - 安装：`winget install KhronosGroup.VulkanSDK`
  - 安装后需设置环境变量 `VULKAN_SDK`（SDK 安装器通常会自动设置，若当前终端没有则重启终端）
- **CMake ≥ 3.20** 与任意 C++20 编译器（MSVC / clang / MinGW）
- **GLFW**：优先使用系统安装，否则由 CMake 自动从 GitHub 拉取（3.4）

## 构建（Windows）

```powershell
cd D:\proj\MagicXEngine
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
.\build\Release\MagicXEngine.exe
```

> 使用 Visual Studio 生成器时，运行 `cmake --build build --config Release` 即可。

## 架构说明：RHI 抽象与后端扩展

高层代码（`Renderer/`、`Engine.cpp`）**只依赖 `RHI/RHI.h` 与 `RHI/RHITypes.h`**，
完全不接触 Vulkan 头文件。各后端在 `RHI/<Backend>/` 目录下实现 `IRHIDevice` 等接口，
由 `RHI.cpp` 的 `CreateDevice()` 工厂统一创建。

**新增 OpenGL 后端的步骤：**

1. `RHITypes.h` 中 `Backend` 枚举已有 `OpenGL` 占位；
2. 新建 `src/RHI/OpenGL/`，实现 `IRHIDevice` / `IRHIBuffer` / `IRHIPipeline` / `IRHICommandBuffer`；
3. 在 `RHI.cpp` 的 `CreateDevice()` 中加入 `case Backend::OpenGL:`；
4. `Engine.cpp` 中把 `RHI::Backend::Vulkan` 换成 `RHI::Backend::OpenGL` 即可切换，`TriangleRenderer` 无需改动。

### 关键接口

- `IRHIDevice`：资源创建（缓冲/管线）、帧循环（BeginFrame/EndFrame）、交换链尺寸/格式查询。
- `IRHICommandBuffer`：Begin/EndRenderPass、BindPipeline、BindVertexBuffer、SetViewport/Scissor、Draw。
- `WindowSurface`：把平台窗口句柄（当前为 `GLFWwindow*`）传给 RHI 创建表面。

## 说明

- 着色器在构建期由 `glslc` 编译为 SPIR-V，并内嵌进可执行文件，运行时不依赖 shader 文件路径。
- 校验层默认开启（`MAGICXENGINE_ENABLE_VALIDATION=ON`），调试期会输出 Vulkan 校验信息，可在 CMake 中关闭。
- 交换链在窗口尺寸变化或 `OUT_OF_DATE` 时自动重建。
