cmake_minimum_required(VERSION 3.27)
project(VulkanEngine)

find_package(Vulkan REQUIRED)

include(cmake/Shaders.cmake)
include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG "0.9.9.8"
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG "v1.17.0"
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG "3.3.8"
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    microsoft-gsl
    GIT_REPOSITORY "https://github.com/microsoft/GSL.git"
    GIT_TAG "v4.0.0"
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(microsoft-gsl)

FetchContent_Declare(
    open-subdiv
    GIT_REPOSITORY "https://github.com/PixarAnimationStudios/OpenSubdiv.git"
    GIT_TAG "v3_7_0"
    GIT_SHALLOW TRUE
)

set(NO_PTEX ON CACHE BOOL "" FORCE)
set(NO_DOC ON CACHE BOOL "" FORCE)
set(NO_OMP ON CACHE BOOL "" FORCE)
set(NO_TBB ON CACHE BOOL "" FORCE)
set(NO_CUDA ON CACHE BOOL "" FORCE)
set(NO_OPENCL ON CACHE BOOL "" FORCE)
set(NO_CLEW ON CACHE BOOL "" FORCE)
set(NO_GLFW ON CACHE BOOL "" FORCE)
set(NO_EXAMPLES ON CACHE BOOL "" FORCE)
set(NO_TUTORIALS ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(open-subdiv)

if(TARGET osd_static_cpu AND NOT TARGET OpenSubdiv::osdCPU)
    add_library(OpenSubdiv::osdCPU ALIAS osd_static_cpu)
    add_library(osdCPU ALIAS osd_static_cpu)
endif()

set(OPENSUBDIV_ROOT_DIR "${open-subdiv_SOURCE_DIR}" CACHE PATH "" FORCE)
set(OPENSUBDIV_INCLUDE_DIR "${open-subdiv_SOURCE_DIR}" CACHE PATH "" FORCE)
set(OPENSUBDIV_osdCPU_LIBRARY osd_static_cpu CACHE STRING "" FORCE)
set(OPENSUBDIV_LIBRARIES osd_static_cpu CACHE STRING "" FORCE)

set(PXR_ENABLE_HAVE_TBB OFF CACHE BOOL "Disable TBB support in OpenUSD" FORCE)
set(PXR_ENABLE_HAVE_TBB OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    open-usd
    GIT_REPOSITORY "https://github.com/PixarAnimationStudios/OpenUSD.git"
    GIT_TAG "v26.08"
    GIT_SHALLOW TRUE
)

set(PXR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PXR_BUILD_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(PXR_ENABLE_PYTHON_SUPPORT OFF CACHE BOOL "" FORCE)
set(PXR_BUILD_IMAGING OFF CACHE BOOL "" FORCE)
set(PXR_ENABLE_TBB_SUPPORT OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(open-usd)


if(TARGET pxOsd)
    target_include_directories(pxOsd INTERFACE "$<BUILD_INTERFACE:${open-subdiv_SOURCE_DIR}>")
endif()
if(TARGET hdSt)
    target_include_directories(hdSt INTERFACE "$<BUILD_INTERFACE:${open-subdiv_SOURCE_DIR}>")
endif()

file(GLOB_RECURSE VulkanEngineSources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(VulkanEngine ${VulkanEngineSources})
target_link_libraries(VulkanEngine PRIVATE Vulkan::Vulkan)
target_link_libraries(VulkanEngine PRIVATE glm)
target_link_libraries(VulkanEngine PRIVATE glfw)
target_link_libraries(VulkanEngine PRIVATE Microsoft.GSL::GSL)
target_link_libraries(VulkanEngine PRIVATE spdlog)

target_link_libraries(VulkanEngine PRIVATE osdCPU)

target_include_directories(VulkanEngine PRIVATE
    "${open-usd_SOURCE_DIR}"
    "${open-usd_BINARY_DIR}/include"
)

target_link_libraries(VulkanEngine PRIVATE 
    usd 
    usdGeom 
    sdf 
    tf 
    vt 
    gf 
    arch
)

target_compile_definitions(VulkanEngine PRIVATE
    USD_PLUGIN_DIR="${open-usd_BINARY_DIR}"
)

target_include_directories(VulkanEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_features(VulkanEngine PRIVATE cxx_std_20)

target_precompile_headers(VulkanEngine PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/precomp.h")

file(GLOB_RECURSE ShaderSources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

add_shaders(VulkanEngineShaders ${ShaderSources})
add_dependencies(VulkanEngine VulkanEngineShaders)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/paving-stones.jpg" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config.txt" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")