# OpenGL Viewer

IMPORTANT NOTE: Done with claude AI.

A high-performance OpenGL 4.6 viewer designed for visualizing large CAD models. Built with modern C++17 and optimized for rendering complex geometry.

## Features

- **Modern OpenGL 4.6** - Uses Direct State Access (DSA) for efficient GPU resource management
- **OBJ Mesh Loading** - Load and display OBJ files with automatic normal handling
- **Orbit Camera** - Intuitive camera controls for 3D navigation
- **Blinn-Phong Lighting** - Realistic shading with directional light
- **Wireframe Mode** - Toggle wireframe rendering for mesh inspection

## Screenshots

*Coming soon*

## Requirements

### Dependencies
- CMake 3.16+
- OpenGL 4.6 compatible GPU and drivers
- GLFW3

### Ubuntu/Debian
```bash
sudo apt install cmake libgl-dev libglfw3-dev
```

## Building

```bash
git clone https://github.com/yourusername/opengl-viewer.git
cd opengl-viewer
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Run with a mesh file
./MeshViewer path/to/mesh.obj

# Run with included sample
./MeshViewer assets/meshes/cube.obj
```

### Controls

| Input | Action |
|-------|--------|
| Left Mouse Drag | Orbit camera |
| Middle Mouse Drag | Pan camera |
| Scroll Wheel | Zoom in/out |
| W | Toggle wireframe |
| F | Focus on scene |
| ESC | Exit |

## Project Structure

```
OpenGL/
├── src/
│   ├── main.cpp              # Entry point
│   ├── Application.h/cpp     # Main application
│   ├── core/                 # Window, Shader, Timer
│   ├── renderer/             # Camera, Renderer
│   ├── scene/                # Scene graph, Objects
│   └── mesh/                 # Mesh loading and GPU resources
├── shaders/
│   ├── mesh.vert             # Vertex shader
│   └── mesh.frag             # Fragment shader
├── assets/
│   └── meshes/               # Sample mesh files
└── external/                 # Third-party libraries
```

## Roadmap

- [ ] Object picking and selection
- [ ] Dear ImGui integration for GUI
- [ ] Parasolid CAD format support
- [ ] Multi-threaded geometry loading
- [ ] Frustum culling and LOD
- [ ] Material and texture support

## Third-Party Libraries

- [GLFW](https://www.glfw.org/) - Windowing and input
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [GLM](https://github.com/g-truc/glm) - Mathematics library
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - OBJ file parsing

## License

MIT License - See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
