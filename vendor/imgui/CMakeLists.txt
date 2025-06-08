cmake_minimum_required(VERSION 3.26)

target_include_directories(imgui PUBLIC ./)

target_sources(imgui
PUBLIC
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_sdl3.cpp
)

target_link_libraries(imgui PRIVATE SDL3::SDL3)
