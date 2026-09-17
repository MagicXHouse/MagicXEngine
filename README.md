# MagicXEngine

一个用于验证图形算法而设计的微型GPU-Driven图形引擎。

## 三层架构

```
案例层 Cases ──> 前端 Frontend ──> 后端 Backend ──> RHI (Vulkan)
```

- **案例层（Cases）**：每个 case 一个可执行文件，均可设为 VS 启动项目独立运行。
- **前端（Frontend）**：用户描述场景数据的 API —— 顶点坐标、索引、颜色、transform、相机。
- **后端（Backend）**：`RenderScene` 管理前端传来的数据并驱动渲染；`RHI` 负责与底层图形 API 交互。

数据流：案例填场景 → 前端 Scene → 后端 RenderScene 上传 GPU → RHI 渲染。

## 案例（Cases）

5 个 case 按「从直接绘制 → GPU 驱动 → meshlet 剔除」的演进路径编排，逐步验证 GPU-Driven 所需的能力：

> **演进路径**：Case01 直接绘制 → Case02 间接绘制（GPU 驱动雏形）→ Case03 相机/交互基础 → Case04 meshlet 逐块剔除 → Case05 量化 meshlet vs 普通剔除的差距。

| 案例 | 可执行文件 | 渲染模式 | 目的 |
|------|-----------|---------|------|
| Case01 | Case01_Triangle.exe | Direct | 跑通三层架构数据流 |
| Case02 | Case02_GPUDriven.exe | Indirect | compute 写间接命令 + 间接绘制 |
| Case03 | Case03_Perspective.exe | Direct | 透视/正交相机 + ImGui + 鼠标 |
| Case04 | Case04_Meshlet.exe | Meshlet | meshlet 逐块视锥剔除 |
| Case05 | Case05_Comparison.exe | Meshlet / Culled | meshlet vs 普通剔除（三角形数 + FPS） |

> 渲染模式（`Frontend::RenderMode`）：`Direct` 无剔除直绘；`Indirect` compute 写固定间接命令；`Meshlet` GPU 逐 meshlet 视锥剔除 + 间接绘制；`Culled` CPU 逐对象视锥剔除 + 直绘。

### Case01 - 彩色三角形（Case01_Triangle.exe）
- **做了什么**：画一个 RGB 彩色三角形，跑通三层架构完整数据流（案例层 → 前端 Scene/Mesh/Transform → 后端 RenderScene → RHI/Vulkan）。
- **技术点**：默认「屏幕空间」相机（NDC 直通、无投影矩阵）；顶点/索引/颜色上传；Direct 模式 DrawIndexed 直绘。
- **验证**：运行看到彩色三角形即可；改 triangle.transform 可验证 MVP 生效。

### Case02 - GPU 驱动冒烟测试（Case02_GPUDriven.exe）
- **做了什么**：与 Case01 相同的三角形，但 renderMode = Indirect —— compute shader（cull.comp）写一条固定 DrawIndexedIndirectCommand 到 SSBO，barrier 后由 DrawIndexedIndirect 间接绘制。
- **技术点**：compute pipeline、storage/indirect 缓冲、compute→draw 的 pipeline barrier、间接绘制。
- **验证**：RenderDoc 抓帧看 compute dispatch 在 render pass 之前、间接缓冲内容、barrier 无 SYNC-HAZARD。

### Case03 - 透视相机 + ImGui + 鼠标交互（Case03_Perspective.exe）
- **做了什么**：透视/正交相机下画彩色立方体，带 ImGui 面板和鼠标交互。
- **技术点**：透视投影（OpenGL 惯例 → GLToVulkanClip 转 Vulkan 裁剪空间）、正交投影、ImGui 插件、transform 交互。
- **交互**：左键旋转 / 中键移动 / 滚轮缩放；面板勾选 Orthographic 切正交、滑块调旋转/缩放、按钮重置。

### Case04 - meshlet 逐块视锥剔除（Case04_Meshlet.exe）
- **做了什么**：150×150 大网格（45000 三角形）分成约 460 个 meshlet，compute shader 逐 meshlet 做视锥剔除，视锥外的 meshlet instanceCount=0 跳过绘制。
- **技术点**：meshlet 构建（贪心 BFS）、meshlet 描述 SSBO、逐 meshlet 视锥剔除 compute、间接多命令绘制。
- **验证**：RenderDoc 看 indirect buffer 中 instanceCount 的 3/0 分布。
- **注意**：meshlet 路径已改为「多对象平铺到世界空间」，此 case 的鼠标交互（移动网格）暂不生效，作静态剔除验证；交互演示见 Case05。

### Case05 - meshlet vs 普通剔除 对比（Case05_Comparison.exe）
- **做了什么**：25×25 个 patch（约 78 万三角形）场景，量化「CPU 逐对象剔除」与「GPU 逐 meshlet 剔除」的三角形数差异和真实 FPS。ImGui 的 checkbox 是 A/B 开关。
- **技术点**：多对象平铺（FlattenObjects）、世界空间视锥剔除、FPS 测量、运行时切换渲染模式。
- **交互**：左键旋转 / 中键平移 / 滚轮缩放相机；勾选「Use meshlet culling」在 meshlet/普通渲染间切换，观察 FPS 与「Drawing now」变化。

## 目录结构

```
MagicXEngine/
├── CMakeLists.txt              # 引擎静态库 + 案例可执行文件
├── cmake/embed_shader.cmake    # SPIR-V 内嵌为 C++ 字节数组
├── shaders/                    # GLSL 源文件
├── third_party/glfw/           # 内置的 GLFW 3.4（离线可编译）
├── include/                    # ★ 头文件（声明/接口）
│   └── MagicXEngine/
│       ├── Engine.h
│       ├── Core/               #   Window.h / Logger.h / Math.h
│       ├── Frontend/           #   Mesh.h / Transform.h / Camera.h / Scene.h
│       └── Backend/
│           ├── RenderScene.h
│           └── RHI/            #   RHITypes.h / RHI.h
│               └── Vulkan/     #   Vulkan*.h
├── src/                        # ★ 源文件（实现）
│   ├── Engine.cpp
│   ├── Core/                   #   Window.cpp / Logger.cpp
│   └── Backend/
│       ├── RenderScene.cpp
│       └── RHI/                #   RHI.cpp
│           └── Vulkan/         #   Vulkan*.cpp
└── cases/                      # ★ 案例层（每个 case 一个可执行文件，说明见下方「案例」）
    ├── Case01_Triangle.cpp     # 彩色三角形（直接绘制）
    ├── Case02_GPUDriven.cpp    # GPU 驱动（间接绘制）冒烟测试
    ├── Case03_Perspective.cpp  # 透视相机 + ImGui + 鼠标交互
    ├── Case04_Meshlet.cpp      # meshlet 逐块视锥剔除
    └── Case05_Comparison.cpp   # meshlet vs 普通剔除 对比
```

## 依赖

- **Vulkan SDK**（提供 vulkan.h、vulkan-1.lib、glslc、校验层）
  - 安装：`winget install KhronosGroup.VulkanSDK`
  - 无需手动设置环境变量：CMake 会自动定位 `C:\VulkanSDK` 下的 SDK；若装在别处，再设置 `VULKAN_SDK` 即可
- **CMake ≥ 3.20** 与任意 C++20 编译器（MSVC / clang / MinGW）
- **GLFW 3.4**：已内置到 `third_party/glfw`，无需联网下载

## 构建（Windows）

```powershell
打开项目所在路径，打开git bash，输入：
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\Case01_Triangle.exe
```

> 生成的解决方案在 `build/MagicXEngine.sln`，可直接用 Visual Studio 打开调试（VS 2026 生成器则产出 `.slnx`）。

## 新增一个案例

1. 在 `cases/` 下新建 `Case02_XXX.cpp`：
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
2. 在 `CMakeLists.txt` 加一行：`add_case(Case02_XXX ${CMAKE_SOURCE_DIR}/cases/Case02_XXX.cpp)`
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

