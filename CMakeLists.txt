cmake_minimum_required(VERSION 3.26)
project(car C CXX)

set(CMAKE_PREFIX_PATH "~/cmake/")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
# set(CMAKE_BUILD_TYPE Debug)
#set(Jolt_ROOT "../JoltPhysics/Build/Debug/")
#set(SDL3_ROOT "~/SDL/build/Debug/")
#set(SDL3_image_ROOT "~/SDL_image/build/Debug/")
#set(SDL3_mixer_ROOT "~/SDL_mixer/build/Debug/")
#set(assimp_ROOT "~../assimp-src/build/Debug/generated")
#set(SDL3_ttf_ROOT "~/Lib/SDL_ttf/build/Debug")
#set(Freetype_ROOT "~/cmake/freetype-2.13.3-win64/")


find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(assimp REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(SDL3_mixer REQUIRED)
# find_package(SDL3_ttf REQUIRED)
find_package(Freetype REQUIRED)

add_library(imgui)
add_subdirectory(vendor/imgui)

#include_directories(include ~/cmake/freetype-2.13.3-win64/include/freetype2/)
include_directories(include)

#add_subdirectory(../assimp-src assimp)
#add_subdirectory(C:/Users/Ev/SDL_image SDL_image EXCLUDE_FROM_ALL)
#add_subdirectory(../JoltPhysics/Build/ JoltPhysics)

add_compile_options(-Wall)
add_executable(car)

target_sources(car
PRIVATE
    src/main.cpp
    src/main_game.cpp
    src/model.cpp
    src/shader.cpp
    src/texture.cpp
    src/camera.cpp
    src/convert.cpp
    src/physics.cpp
    src/vehicle.cpp
    src/input.cpp
    src/audio.cpp
    src/render.cpp
    src/world.cpp
    src/player.cpp
    glad/glad.c
)

#target_include_directories(car PUBLIC ../JoltPhysics)
target_link_libraries(car PRIVATE SDL3::SDL3 SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer Freetype::Freetype assimp::assimp Jolt::Jolt imgui
    -static-libgcc -static-libstdc++)
#target_link_libraries(physics_test PRIVATE SDL3::SDL3)
