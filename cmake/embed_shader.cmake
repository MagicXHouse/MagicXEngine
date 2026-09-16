# Generates a C++ header with the SPIR-V binary as a uint32_t array.
# Args: SPV_FILE (input .spv), ARRAY_NAME (identifier), OUT_HEADER (output .h)
if(NOT EXISTS "${SPV_FILE}")
  message(FATAL_ERROR "SPV_FILE not found: ${SPV_FILE}")
endif()

file(READ "${SPV_FILE}" HEX_CONTENT HEX)
string(LENGTH "${HEX_CONTENT}" HEX_LEN)

set(BODY "")
set(I 0)
# SPIR-V 按小端序存储；HEX 读取得到文件字节序，需将每 4 字节逆序合成 uint32
while(I LESS HEX_LEN)
  string(SUBSTRING "${HEX_CONTENT}" ${I} 2 B0)
  math(EXPR I "${I} + 2")
  string(SUBSTRING "${HEX_CONTENT}" ${I} 2 B1)
  math(EXPR I "${I} + 2")
  string(SUBSTRING "${HEX_CONTENT}" ${I} 2 B2)
  math(EXPR I "${I} + 2")
  string(SUBSTRING "${HEX_CONTENT}" ${I} 2 B3)
  math(EXPR I "${I} + 2")
  set(BODY "${BODY}0x${B3}${B2}${B1}${B0},")
endwhile()
math(EXPR WORD_COUNT "${HEX_LEN} / 8")

file(WRITE "${OUT_HEADER}"
"// Auto-generated from a GLSL shader in shaders/. Do not edit.
#pragma once
#include <cstdint>

namespace MagicXEngine::Shaders {
inline constexpr uint32_t ${ARRAY_NAME}[] = { ${BODY} };
inline constexpr uint32_t ${ARRAY_NAME}_word_count = ${WORD_COUNT};
} // namespace MagicXEngine::Shaders
")
