cmake_minimum_required(VERSION 3.14)
project(LITEVIZ-GS)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/viewer)

find_package(Eigen3 3.3 REQUIRED NO_MODULE)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

set(GLAD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/glad)
add_library(glad STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glad/include
)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
)

set(TINIPLY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/tinyply)
add_library(tinyply STATIC
    ${TINIPLY_DIR}/source/tinyply.cpp
)
target_include_directories(tinyply PUBLIC
    ${TINIPLY_DIR}/source
)

add_library(liteviz 
    ${SOURCE_DIR}
)
target_include_directories(liteviz PUBLIC
    ${SOURCE_DIR}
    ${GLAD_DIR}/inlc
    ${IMGUI_DIR}
    ${TINIPLY_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)
target_link_libraries(liteviz
    glfw
    GLU
    glad
    dl
    imgui
    tinyply
)

add_executable(liteviz_viewer app/main.cpp)
target_link_libraries(liteviz_viewer liteviz)
