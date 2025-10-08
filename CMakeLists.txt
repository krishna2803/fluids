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
set(CMAKE_CXX_EXTENSIONS OFF)

# ccache
find_program(CCACHE_PROGRAM ccache)
set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})


# options for deps
set(FASTGLTF_COMPILE_AS_CPP20 ON)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW Library only" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW Library only" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW Library only" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "GLFW Library only" FORCE)

set(GLM_BUILD_TESTS OFF CACHE BOOL "GLM Library only" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static GLM library" FORCE)

# required deps
find_package(Eigen3 5.0.0 REQUIRED NO_MODULE)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

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

# main bin
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# config
target_precompile_headers(
    ${PROJECT_NAME}
  PUBLIC 
    <optional>
    <vector>
    <memory>
    <string>
    <vector>
    <unordered_map>
    <glm/mat4x4.hpp>
    <glm/vec4.hpp>
    <vulkan/vulkan.h>
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)

set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)

target_compile_options(
  ${PROJECT_NAME}
  PRIVATE
    -Wall
    -Wextra
    -Werror
    -march=native
    -mtune=native
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
