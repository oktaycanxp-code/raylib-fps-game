cmake_minimum_required(VERSION 3.20)
project(raylib_fps_game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set RAYLIB_PATH to your Raylib 6.0.0 installation, for example:
# cmake -S . -B build -DRAYLIB_PATH=C:/raylib
set(RAYLIB_PATH "" CACHE PATH "Path to the Raylib installation")

if (RAYLIB_PATH)
    set(raylib_DIR "${RAYLIB_PATH}/lib/cmake/raylib")
endif()

find_package(raylib 6.0 REQUIRED)

add_executable(raylib_fps_game src/main.cpp)
target_link_libraries(raylib_fps_game PRIVATE raylib)

if (WIN32)
    target_link_libraries(raylib_fps_game PRIVATE winmm gdi32 opengl32)
endif()
