# OpenGL Viewer

IMPORTANT NOTE: Done with claude AI.

A high-performance OpenGL 4.6 viewer designed for visualizing large CAD models. Built with modern C++17 and optimized for rendering complex geometry.

## Features

- **Modern OpenGL 4.6** - Uses Direct State Access (DSA) for efficient GPU resource management
- **OBJ Mesh Loading** - Load multiple OBJ files simultaneously with automatic normal handling
- **Object Picking** - Click to select objects with visual highlight feedback
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
git clone https://github.com/weinmueller/opengl-viewer.git
cd opengl-viewer
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Run with a single mesh file
./MeshViewer path/to/mesh.obj

# Load multiple mesh files
./MeshViewer mesh1.obj mesh2.obj mesh3.obj

# Run with included samples
./MeshViewer ../assets/meshes/sphere.obj ../assets/meshes/torus.obj
```

### Controls

| Input | Action |
|-------|--------|
| Left Mouse Drag | Orbit camera |
| Middle Mouse Drag | Pan camera |
| Right Click | Select object (click background to deselect) |
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
│   ├── mesh.vert             # Main vertex shader
│   ├── mesh.frag             # Main fragment shader (Blinn-Phong)
│   ├── picking.vert          # Object picking vertex shader
│   └── picking.frag          # Object picking fragment shader
├── assets/
│   └── meshes/               # Sample mesh files
└── external/                 # Third-party libraries
```

## Roadmap

- [x] Object picking and selection
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
