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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
set(JPH_SHARED_LIBRARY ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(assimp REQUIRED)
#find_package(Jolt CONFIG REQUIRED)
find_package(SDL3_mixer REQUIRED)
# find_package(SDL3_ttf REQUIRED)
find_package(Freetype REQUIRED)

add_library(imgui)
add_subdirectory(vendor/imgui)

#add_library(Jolt)
add_subdirectory(vendor/JoltPhysics/Build)

#include_directories(include ~/cmake/freetype-2.13.3-win64/include/freetype2/)
include_directories(include)

#add_subdirectory(../assimp-src assimp)
#add_subdirectory(C:/Users/Ev/SDL_image SDL_image EXCLUDE_FROM_ALL)
#add_subdirectory(../JoltPhysics/Build/ JoltPhysics)

add_compile_options(-Wall)
add_executable(car)
#set_property(TARGET car PROPERTY POSITION_INDEPENDENT_CODE FALSE)

target_sources(car
PRIVATE
    src/main.cpp
    src/main_game.cpp
    src/physics.cpp
    src/model.cpp
    src/shader.cpp
    src/texture.cpp
    src/camera.cpp
    src/convert.cpp
    src/vehicle.cpp
    src/input.cpp
    src/audio.cpp
    src/render.cpp
    src/render_internal.cpp
    src/render_lights.cpp
    src/render_shadow.cpp
    src/world.cpp
    src/player.cpp
    glad/glad.c
)

#target_include_directories(car PUBLIC ../JoltPhysics)
target_link_libraries(car SDL3::SDL3 SDL3_image::SDL3_image
    SDL3_mixer::SDL3_mixer Freetype::Freetype assimp::assimp Jolt::Jolt imgui
    -static-libgcc -static-libstdc++)

# CPack stuff

install(TARGETS car RUNTIME_DEPENDENCY_SET deps
    RUNTIME DESTINATION bin)
# Install dependencies but exclude system libraries
IF (WIN32)
    install(RUNTIME_DEPENDENCY_SET deps
        PRE_EXCLUDE_REGEXES "api-ms" "ext-ms-"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        DIRECTORIES
            $<TARGET_FILE_DIR:SDL3::SDL3>
            $<TARGET_FILE_DIR:SDL3_image::SDL3_image>
            $<TARGET_FILE_DIR:SDL3_mixer::SDL3_mixer>
            $<TARGET_FILE_DIR:Jolt::Jolt>
            $<TARGET_FILE_DIR:Freetype::Freetype>
            $<TARGET_FILE_DIR:assimp::assimp>
        )
ELSE()
    install(RUNTIME_DEPENDENCY_SET deps 
        PRE_EXCLUDE_REGEXES "libc\\.so" "libm\\.so" "ld-linux"
    )
ENDIF()

# Put shaders in the install directory
install(DIRECTORY shaders DESTINATION .)
# Put data in install directory
install(DIRECTORY data DESTINATION .)
# Enable packaging
IF (WIN32)
    set(CPACK_GENERATOR "ZIP")
ELSE()
    set(CPACK_GENERATOR "TGZ")
ENDIF()

include(CPack)

