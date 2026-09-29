# Raylib FPS Game

A small C++ 3D first-person game for Windows using **Raylib 6.0.0**.

## Features

- WASD movement and mouse-look
- Jumping and gravity
- Simple AABB wall collision
- Small 3D arena with walls and cover
- Moving targets that can be shot with the left mouse button
- Main menu and ESC-to-menu behavior
- FPS counter

## Requirements

- Windows 10/11
- Visual Studio Code
- MinGW-w64 with `g++`, or Visual Studio Build Tools
- Raylib 6.0.0
- CMake 3.20 or newer (recommended)

## Build with CMake in VS Code

1. Install Raylib 6.0.0. This project expects the CMake package to be available at `C:/raylib/lib/cmake/raylib` by default when `RAYLIB_PATH=C:/raylib` is supplied.
2. Open this folder in VS Code.
3. Select **CMake: Configure** from the Command Palette.
4. If Raylib is installed elsewhere, configure with:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DRAYLIB_PATH=C:/path/to/raylib
cmake --build build
```

5. Run `build/raylib_fps_game.exe`.

The VS Code settings in `.vscode/settings.json` provide the same `RAYLIB_PATH` cache variable. Change it there if necessary.

## Controls

- **W/A/S/D**: move
- **Mouse**: look around
- **Space**: jump
- **Left mouse button**: shoot targets
- **ESC**: return to menu
- **Enter / left mouse button**: start from the menu
