cmake_minimum_required(VERSION 4.1)
project(
  fluids
    VERSION 1.0
    LANGUAGES C CXX
)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# c/c++ options
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS ON)

# ccache
find_program(CCACHE_PROGRAM ccache)
set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})

# glslang for glsl
find_program(GLSL_VALIDATOR glslangValidator)

# dxc for hlsl
find_program(DXC_COMPILER dxc)


# options for deps
set(FASTGLTF_COMPILE_AS_CPP20 ON)

set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_INSTALL OFF)
set(GLFW_BUILD_X11 OFF)
set(BUILD_SHARED_LIBS OFF)

set(GLM_ENABLE_CXX_20 ON)
set(GLM_BUILD_TESTS OFF)
set(GLM_ENABLE_LANG_EXTENSIONS ON)
set(GLM_ENABLE_FAST_MATH ON)

# detect and enable best SIMD support
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
check_cxx_compiler_flag("-mavx" COMPILER_SUPPORTS_AVX)
check_cxx_compiler_flag("-msse4.2" COMPILER_SUPPORTS_SSE42)

if(COMPILER_SUPPORTS_AVX2)
  set(GLM_ENABLE_SIMD_AVX2 ON)
elseif(COMPILER_SUPPORTS_AVX)
  set(GLM_ENABLE_SIMD_AVX ON)
elseif(COMPILER_SUPPORTS_SSE42)
  set(GLM_ENABLE_SIMD_SSE4_2 ON)
endif()

set(QUILL_X86ARCH ON)

# required deps
find_package(Eigen3 5.0.0 REQUIRED NO_MODULE)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

# global compile options
# required if QUILL_X86ARCH is set
add_compile_options(-march=native -mtune=native)

# use lld
set(CMAKE_LINKER_TYPE LLD)

# add subdirectories
add_subdirectory(include/fastgltf)
add_subdirectory(include/glfw)
add_subdirectory(include/glm)
add_subdirectory(include/quill)
add_subdirectory(include/rapidobj)
add_subdirectory(include/vk-bootstrap)
add_subdirectory(include/volk)

# header-only lib
set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include/stb)

# imgui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/imgui)

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  # ${IMGUI_DIR}/imgui_demo.cpp # optional
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  # GLFW + Vulkan
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

# build imgui lib
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(
    imgui
  PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_DIR}/misc/cpp
)
target_link_libraries(
    imgui
  PUBLIC
    glfw
    Vulkan::Vulkan
)

# sources
file(GLOB_RECURSE PROJECT_SOURCES "src/*.cc")

# glsl shader sources
file(GLOB_RECURSE GLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag.glsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert.glsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp.glsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.geom.glsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.tesc.glsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.tese.glsl"
)

# hlsl shader sources
file(GLOB_RECURSE HLSL_SOURCE_FILES
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.vert.hlsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.frag.hlsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.comp.hlsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.geom.hlsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.tesc.hlsl"
  "${PROJECT_SOURCE_DIR}/assets/shaders/*.tese.hlsl"
)

# compile time definitions
add_compile_definitions(ASSET_DIR="${CMAKE_SOURCE_DIR}/assets")

# main bin
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# function to determine shader stage and target profile
function(get_shader_stage SHADER_PATH OUT_STAGE OUT_PROFILE)
  get_filename_component(SHADER_NAME ${SHADER_PATH} NAME)
  
  if(SHADER_NAME MATCHES "\\.vert\\.hlsl$")
    set(${OUT_STAGE} "vs" PARENT_SCOPE)
    set(${OUT_PROFILE} "vs_6_8" PARENT_SCOPE)
  elseif(SHADER_NAME MATCHES "\\.frag\\.hlsl$")
    set(${OUT_STAGE} "ps" PARENT_SCOPE)
    set(${OUT_PROFILE} "ps_6_8" PARENT_SCOPE)
  elseif(SHADER_NAME MATCHES "\\.comp\\.hlsl$")
    set(${OUT_STAGE} "cs" PARENT_SCOPE)
    set(${OUT_PROFILE} "cs_6_8" PARENT_SCOPE)
  elseif(SHADER_NAME MATCHES "\\.geom\\.hlsl$")
    set(${OUT_STAGE} "gs" PARENT_SCOPE)
    set(${OUT_PROFILE} "gs_6_8" PARENT_SCOPE)
  elseif(SHADER_NAME MATCHES "\\.tesc\\.hlsl$")
    set(${OUT_STAGE} "hs" PARENT_SCOPE)
    set(${OUT_PROFILE} "hs_6_8" PARENT_SCOPE)
  elseif(SHADER_NAME MATCHES "\\.tese\\.hlsl$")
    set(${OUT_STAGE} "ds" PARENT_SCOPE)
    set(${OUT_PROFILE} "ds_6_8" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unknown shader type: ${SHADER_NAME}")
  endif()
endfunction()

# compile glsl shaders
foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
  
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V 
      # -Os
      ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
    COMMENT "Compiling GLSL shader: ${FILE_NAME}"
  )
  list(APPEND SPIRV_GLSL_FILES ${SPIRV})
endforeach(GLSL)

# compile hlsl shaders
foreach(HLSL ${HLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${HLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/assets/shaders/${FILE_NAME}.spv")
  
  get_shader_stage(${HLSL} SHADER_STAGE SHADER_PROFILE)
  
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${DXC_COMPILER}
      -T ${SHADER_PROFILE}
      -E main
      -spirv
      -O3
      -Qstrip_debug
      ${HLSL}
      -Fo ${SPIRV}
    DEPENDS ${HLSL}
    COMMENT "Compiling HLSL shader: ${FILE_NAME}"
  )
  list(APPEND SPIRV_HLSL_FILES ${SPIRV})
endforeach(HLSL)

# config
target_precompile_headers(
    ${PROJECT_NAME}
  PUBLIC 
    # <optional>
    # <memory>
    # <string>
    <vector>
    # <unordered_map>
    # <glm/mat4x4.hpp>
    # <glm/vec4.hpp>
    # <vulkan/vulkan.h>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

target_compile_options(
  ${PROJECT_NAME}
  PRIVATE
    -Wall
    -Wextra
    -Werror
    # -O3
)

# headers
target_include_directories(
    ${PROJECT_NAME}
  PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src 
    ${STB_INCLUDE_DIR}
)

# link libs
target_link_libraries(
    ${PROJECT_NAME}
  PRIVATE
    Eigen3::Eigen
    fastgltf
    glfw
    glm
    GPUOpen::VulkanMemoryAllocator
    imgui
    quill::quill
    vk-bootstrap::vk-bootstrap
    Vulkan::Vulkan
    volk
)

# ninja run / make run
add_custom_target(run
  COMMAND
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  DEPENDS
    ${PROJECT_NAME}
  WORKING_DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}
)

# ninja shaders / make shaders
add_custom_target(
  shaders
  DEPENDS ${SPIRV_GLSL_FILES} ${SPIRV_HLSL_FILES}
)

# ninja build / make build
add_custom_target(build
  DEPENDS
    ${PROJECT_NAME}
    ${SPIRV_GLSL_FILES}
    ${SPIRV_HLSL_FILES}
  COMMENT "Building ${PROJECT_NAME} and shaders"
)

# ninja clean-project / make clean-project
add_custom_target(clean-project
  COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${SPIRV_GLSL_FILES}
  COMMAND ${CMAKE_COMMAND} -E remove -f ${SPIRV_HLSL_FILES}
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}.dir
  COMMENT "Cleaning ${PROJECT_NAME} binary and all shaders (keeping dependencies)"
)
