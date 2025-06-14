cmake_minimum_required(VERSION 3.26)
project(car C CXX)

set(CMAKE_PREFIX_PATH "~/cmake/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
# set(CMAKE_BUILD_TYPE Debug)
set(Jolt_ROOT "../JoltPhysics/Build/Debug/")
set(SDL3_ROOT "~/SDL/build/Debug/")
set(SDL3_image_ROOT "~/SDL_image/build/Debug/")
set(SDL3_mixer_ROOT "~/SDL_mixer/build/Debug/")
set(assimp_ROOT "~../assimp-src/build/Debug/generated")

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(assimp REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(SDL3_mixer REQUIRED)

add_library(imgui)
add_subdirectory(vendor/imgui)

#add_subdirectory(../assimp-src assimp)
#add_subdirectory(C:/Users/Ev/SDL_image SDL_image EXCLUDE_FROM_ALL)
#add_subdirectory(../JoltPhysics/Build/ JoltPhysics)

add_compile_options(-Wall)
add_executable(car)

target_sources(car
PRIVATE
    main.cpp
    model.cpp
    shader.cpp
    texture.cpp
    camera.cpp
    convert.cpp
    physics.cpp
    vehicle.cpp
    input.cpp
    audio.cpp
    render.cpp
    glad/glad.c
)

#target_include_directories(car PUBLIC ../JoltPhysics)
target_link_libraries(car PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_mixer::SDL3_mixer assimp::assimp Jolt::Jolt imgui -static-libgcc -static-libstdc++)
#target_link_libraries(physics_test PRIVATE SDL3::SDL3)
