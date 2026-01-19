# OpenGL Viewer

IMPORTANT NOTE: Done with claude AI.

A high-performance OpenGL 4.6 viewer designed for visualizing large CAD models. Built with modern C++17 and optimized for rendering complex geometry.

## Features

- **Modern OpenGL 4.6** - Uses Direct State Access (DSA) for efficient GPU resource management
- **OBJ Mesh Loading** - Load multiple OBJ files simultaneously with automatic normal handling
- **Object Picking** - Click to select objects with visual highlight feedback
- **Mesh Subdivision** - Loop subdivision (smooth) and midpoint subdivision with crease/boundary preservation
- **Parallel Processing** - OpenMP-accelerated subdivision for large meshes (4-5x speedup)
- **GPU Double-Buffering** - Fence-synchronized buffer swapping for smooth geometry updates
- **Back-face Culling** - Toggleable culling for ~50% faster rendering on closed meshes
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
- OpenMP (usually included with GCC/Clang)

### Ubuntu/Debian
```bash
sudo apt install cmake libgl-dev libglfw3-dev libomp-dev
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

# Set crease angle threshold for subdivision (default: 30 degrees)
./MeshViewer --angle 45 mesh.obj

# Show help
./MeshViewer --help
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `--angle <degrees>` | Crease angle threshold for subdivision (default: 30). Edges with dihedral angle greater than this are kept sharp. |
| `--help` | Show help message |

### Controls

| Input | Action |
|-------|--------|
| Left Mouse Drag | Orbit camera |
| Middle Mouse Drag | Pan camera |
| Right Click | Select object (click background to deselect) |
| Scroll Wheel | Zoom in/out |
| S | Subdivide mesh (Loop - smooth) |
| D | Subdivide mesh (midpoint - keeps shape) |
| W | Toggle wireframe |
| C | Toggle back-face culling |
| F | Focus on scene |
| ESC | Exit |

Mesh statistics (vertices, triangles) are printed to the terminal after loading and subdivision.

## Project Structure

```
OpenGL/
├── src/
│   ├── main.cpp              # Entry point
│   ├── Application.h/cpp     # Main application
│   ├── core/                 # Window, Shader, Timer
│   ├── renderer/             # Camera, Renderer
│   ├── scene/                # Scene graph, Objects
│   ├── mesh/                 # Mesh loading and GPU resources
│   └── geometry/             # Subdivision algorithms
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
- [x] Mesh subdivision (Loop and midpoint)
- [x] Parallel subdivision with OpenMP
- [x] GPU double-buffering for geometry updates
- [x] Back-face culling with toggle
- [x] Cached normal matrix optimization
- [ ] Object-level frustum culling
- [ ] GPU-based subdivision (compute shaders)
- [ ] LOD (Level of Detail) system
- [ ] Material and texture support

## Third-Party Libraries

- [GLFW](https://www.glfw.org/) - Windowing and input
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [GLM](https://github.com/g-truc/glm) - Mathematics library
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - OBJ file parsing
- [OpenMP](https://www.openmp.org/) - Parallel processing

## License

MIT License - See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
