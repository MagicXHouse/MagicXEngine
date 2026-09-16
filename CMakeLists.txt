cmake_minimum_required(VERSION 3.20)
project(MagicXEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 保证中文注释/字符串正确解析（对库、案例、GLFW 均生效）
if(MSVC)
  add_compile_options(/utf-8)
endif()

option(MAGICXENGINE_ENABLE_VALIDATION "Enable Vulkan validation layers" ON)

# ---------------------------------------------------------------------------
# Vulkan SDK 自动定位（无需手动设置 VULKAN_SDK，默认 C:\VulkanSDK）
# ---------------------------------------------------------------------------
if(NOT DEFINED ENV{VULKAN_SDK})
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
# GLFW（已内置，离线可编译）
# ---------------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw)

# ---------------------------------------------------------------------------
# 着色器：glslc 编译为 SPIR-V 并内嵌
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
# 引擎静态库（Core + Frontend + Backend + RHI）
# ---------------------------------------------------------------------------
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/Core/*.cpp
  ${CMAKE_SOURCE_DIR}/src/Frontend/*.cpp
  ${CMAKE_SOURCE_DIR}/src/Backend/*.cpp
  ${CMAKE_SOURCE_DIR}/src/Engine.cpp
)

add_library(MagicXEngine STATIC
  ${ENGINE_SOURCES}
  ${GENERATED_DIR}/triangle_vert_spv.h
  ${GENERATED_DIR}/triangle_frag_spv.h
)

target_include_directories(MagicXEngine
  PUBLIC  ${CMAKE_SOURCE_DIR}/src
  PRIVATE ${GENERATED_DIR}
)

target_link_libraries(MagicXEngine PUBLIC Vulkan::Vulkan glfw)

if(MAGICXENGINE_ENABLE_VALIDATION)
  target_compile_definitions(MagicXEngine PRIVATE MAGICXENGINE_ENABLE_VALIDATION=1)
endif()

if(MSVC)
  target_compile_options(MagicXEngine PRIVATE /W3 /utf-8 /permissive-)
endif()

# ---------------------------------------------------------------------------
# 案例层：每个 case 一个可执行文件，均可设为 VS 启动项目独立运行
# ---------------------------------------------------------------------------
function(add_case NAME SOURCE)
  add_executable(${NAME} ${SOURCE})
  target_link_libraries(${NAME} PRIVATE MagicXEngine)
  if(MSVC)
    target_compile_options(${NAME} PRIVATE /W3 /permissive-)
  endif()
  set_target_properties(${NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    FOLDER "Cases")
endfunction()

add_case(Case01_Triangle ${CMAKE_SOURCE_DIR}/src/Cases/Case01_Triangle.cpp)
# 后续新增案例：add_case(Case02_XXX ${CMAKE_SOURCE_DIR}/src/Cases/Case02_XXX.cpp)

# VS 默认启动项目
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Case01_Triangle)
