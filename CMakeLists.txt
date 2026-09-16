cmake_minimum_required(VERSION 3.20)
project(MagicXEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(MAGICXENGINE_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)

# ---------------------------------------------------------------------------
# Vulkan SDK (provides vulkan.h, vulkan-1.lib, glslc). Requires VULKAN_SDK env
# var or the SDK on PATH. Install via: winget install KhronosGroup.VulkanSDK
# ---------------------------------------------------------------------------
if(NOT DEFINED ENV{VULKAN_SDK})
  # VULKAN_SDK 未设置时，在 Windows 默认安装目录自动查找最新版本
  if(WIN32 AND EXISTS "C:/VulkanSDK")
    file(GLOB _vulkan_sdk_versions "C:/VulkanSDK/*")
    list(SORT _vulkan_sdk_versions COMPARE NATURAL ORDER DESCENDING)
    foreach(_vulkan_sdk_dir IN LISTS _vulkan_sdk_versions)
      if(EXISTS "${_vulkan_sdk_dir}/Include/vulkan/vulkan.h")
        set(ENV{VULKAN_SDK} "${_vulkan_sdk_dir}")
        message(STATUS "Auto-detected VULKAN_SDK = ${_vulkan_sdk_dir}")
        break()
      endif()
    endforeach()
  endif()
endif()
find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------------------
# Windowing library: GLFW（已内置到 third_party/glfw，离线可编译，不依赖网络下载）
# Abstracted behind Core::Window so it can be swapped later (e.g. SDL/Win32).
# ---------------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)

# ---------------------------------------------------------------------------
# Shaders: compile GLSL -> SPIR-V with glslc, then embed as C++ byte arrays so
# the demo has no runtime file-path dependency. Sources live in shaders/.
# ---------------------------------------------------------------------------
set(SHADER_DIR    ${CMAKE_SOURCE_DIR}/shaders)
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

function(embed_shader SHADER_SRC ARRAY_NAME OUT_HEADER)
  set(SPV ${GENERATED_DIR}/${ARRAY_NAME}.spv)
  add_custom_command(
    OUTPUT ${SPV}
    COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o ${SPV} ${SHADER_SRC}
    DEPENDS ${SHADER_SRC}
    COMMENT "glslc: ${SHADER_SRC}"
    VERBATIM)
  add_custom_command(
    OUTPUT ${OUT_HEADER}
    COMMAND ${CMAKE_COMMAND}
            -DSPV_FILE=${SPV} -DARRAY_NAME=${ARRAY_NAME} -DOUT_HEADER=${OUT_HEADER}
            -P ${CMAKE_SOURCE_DIR}/cmake/embed_shader.cmake
    DEPENDS ${SPV}
    COMMENT "Embedding SPIR-V: ${ARRAY_NAME}"
    VERBATIM)
endfunction()

embed_shader(${SHADER_DIR}/triangle.vert triangle_vert_spv ${GENERATED_DIR}/triangle_vert_spv.h)
embed_shader(${SHADER_DIR}/triangle.frag triangle_frag_spv ${GENERATED_DIR}/triangle_frag_spv.h)

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)

add_executable(MagicXEngine
  ${ENGINE_SOURCES}
  ${GENERATED_DIR}/triangle_vert_spv.h
  ${GENERATED_DIR}/triangle_frag_spv.h)

target_include_directories(MagicXEngine PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${GENERATED_DIR})

target_link_libraries(MagicXEngine PRIVATE Vulkan::Vulkan glfw)

if(MAGICXENGINE_ENABLE_VALIDATION)
  target_compile_definitions(MagicXEngine PRIVATE MAGICXENGINE_ENABLE_VALIDATION=1)
endif()

if(MSVC)
  target_compile_options(MagicXEngine PRIVATE /W3 /utf-8 /permissive-)
endif()

set_target_properties(MagicXEngine PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
