# 2D-Triangle

A minimal OpenGL renderer demonstrating real-time 2D/3D transformations driven by keyboard input.

## Overview

This project renders a colored pyramid using modern OpenGL (3.3 Core Profile) and lets the user manipulate it live through the keyboard — translating, rotating, and scaling the model in real time. It was built to demonstrate the core OpenGL rendering pipeline: shader compilation and linking, vertex buffer setup with VAO/VBO/EBO, per-frame model matrix updates via GLM, and basic camera/projection setup.

## Features

- Real-time keyboard-driven transformations (translate, rotate, scale)
- Frame-rate-independent movement using delta time
- Indexed geometry rendering (VAO/VBO/EBO) to avoid duplicate vertex data
- Per-vertex color interpolation across triangle faces
- Perspective camera with configurable field of view
- Shader compilation and linking with error reporting
- Depth testing for correct face occlusion
- Window resize handling via framebuffer callback

## Tech Stack

- **Language:** C++
- **Graphics API:** OpenGL 3.3 (Core Profile)
- **Shading Language:** GLSL 330
- **Libraries:**
  - [GLFW](https://www.glfw.org/) — window and input management
  - [GLEW](http://glew.sourceforge.net/) — OpenGL extension loading
  - [GLM](https://github.com/g-truc/glm) — matrix/vector math
- **Toolchain:** g++ (MinGW64 on Windows)


## Installation / Getting Started

### Prerequisites

- A C++ compiler (g++ via MinGW64 recommended on Windows)
- GLFW, GLEW, and GLM installed and available to the compiler/linker

### Build

```bash
g++ main.cpp -o 2d-triangle -lglfw3 -lglew32 -lopengl32 -lgdi32
```

> Adjust library flags (`-l...`) to match your local GLFW/GLEW installation and platform.

### Run

```bash
./2d-triangle
```

> On Windows, ensure `glew32.dll` is present in the same directory as the executable.

## Usage

| Key(s) | Action |
|--------|--------|
| `W` / `S` | Translate up / down (while held) |
| `A` / `D` | Translate left / right (while held) |
| `Q` / `E` | Rotate 30° anticlockwise / clockwise around the Z axis |
| `R` / `F` | Scale along the Z axis (grow / shrink) |
| `ESC` | Close the window |

## Project Structure

```
2D-Triangle/
├── main.cpp     # Entry point: window setup, shaders, geometry, render loop
└── README.md
```

## Contributing

This is a personal/academic project and not currently open to external contributions. Feel free to fork it for your own experimentation.

## License

No license has been specified for this repository. All rights reserved unless stated otherwise.

## Contact

Feel free to reach out via GitHub: [@Zniniz](https://github.com/Zniniz)
