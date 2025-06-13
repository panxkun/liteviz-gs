cmake_minimum_required(VERSION 3.14)
set(CMAKE_BUILD_TYPE Release)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
project(liteviz)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "No dev warnings" FORCE)


include(CMakeDependentOption)
include(SuperBuildDepends)
superbuild_option(cpp17)
superbuild_option(modern-cpp)

superbuild_depend(eigen)
superbuild_depend(tinyply)
superbuild_depend(imgui)
superbuild_depend(glfw)
superbuild_depend(glad)
superbuild_extern(tbb)

add_library(liteviz-core
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(liteviz-core 
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(liteviz-core
    GLU
    depends::glad
    depends::glfw
    depends::imgui
    depends::tinyply
    depends::tbb
)

target_compile_definitions(liteviz-core
    PUBLIC 
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_executable(viewer app/main.cpp)
target_link_libraries(viewer liteviz-core)
