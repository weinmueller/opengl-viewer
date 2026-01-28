# OpenGL Viewer

IMPORTANT NOTE: Done with claude AI.

A high-performance OpenGL 4.6 viewer designed for visualizing large CAD models. Built with modern C++17 and optimized for rendering complex geometry.

## Features

- **Modern OpenGL 4.6** - Uses Direct State Access (DSA) for efficient GPU resource management
- **OBJ Mesh Loading** - Load multiple OBJ files simultaneously with automatic normal handling
- **Object Picking** - Click to select objects with visual highlight feedback
- **Mesh Subdivision** - Loop subdivision (smooth) and midpoint subdivision. Refines all edges by default; use `--angle` for crease preservation. Only subdivides visible objects when none selected.
- **Parallel Processing** - OpenMP-accelerated subdivision for large meshes (4-5x speedup)
- **Background Tessellation** - Non-blocking subdivision with real-time progress indicators. UI stays responsive during computation.
- **GPU Double-Buffering** - Fence-synchronized buffer swapping for smooth geometry updates
- **Back-face Culling** - Toggleable culling for ~50% faster rendering on closed meshes
- **Orbit Camera** - Intuitive camera controls with mouse and arrow keys
- **Blinn-Phong Lighting** - Realistic shading with directional light
- **Rim Lighting** - Fresnel-based edge highlighting for better shape visibility
- **Gradient Background** - Professional dark blue gradient backdrop
- **Wireframe Mode** - Toggle wireframe rendering for mesh inspection
- **Help Overlay** - In-window keyboard shortcut reference with toggle indicators (H key)
- **Progress Overlay** - Shows subdivision/LOD progress with phase name, percentage, and queued task count
- **Frustum Culling** - Skip rendering objects outside camera view (G key)
- **LOD System** - Automatic Level of Detail with QEM-based mesh simplification
- **6 LOD Levels** - 100% → 70% → 50% → 35% → 25% → 15% triangle reduction (gentler for smooth meshes)
- **Screen-Space LOD Selection** - Automatic detail adjustment based on object screen size
- **LOD Debug Colors** - Visualize LOD levels with color coding (K key)
- **G+Smo Multipatch Support** - Load NURBS/B-spline multipatch geometries from XML files
- **View-Dependent Tessellation** - Automatic refinement of patches based on screen-space size (4→128 samples)

## Screenshots

*Coming soon*

## Requirements

### Dependencies
- CMake 3.16+
- OpenGL 4.6 compatible GPU and drivers
- GLFW3
- OpenMP (usually included with GCC/Clang)
- G+Smo (optional, for multipatch NURBS/B-spline support)

### Ubuntu/Debian
```bash
sudo apt install cmake libgl-dev libglfw3-dev libomp-dev
```

### G+Smo (Optional)
For multipatch geometry support, G+Smo is detected automatically from:
- Environment variable `GISMO_ROOT` or `GISMO_DIR`
- CMake option `-DGISMO_ROOT=/path/to/gismo`
- System paths (`/usr/local`, `/usr`, `/opt/gismo`)
- Sibling directory `../gismo`

To build G+Smo from source:
```bash
git clone https://github.com/gismo/gismo.git
cd gismo && mkdir build && cd build
cmake .. && make -j$(nproc)
```

CMake options:
```bash
cmake -DWITH_GISMO=OFF ..          # Disable G+Smo support
cmake -DGISMO_ROOT=/path/to/gismo .. # Specify G+Smo location
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

# Set crease angle threshold to preserve sharp edges (default: 180 = smooth all)
./MeshViewer --angle 30 mesh.obj

# Load G+Smo multipatch geometry (requires G+Smo)
./MeshViewer assets/gismo/teapot.xml

# Show help
./MeshViewer --help
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `--angle <degrees>` | Crease angle threshold for subdivision (default: 180). Edges with dihedral angle greater than this are kept sharp. Use lower values (e.g., 30) to preserve sharp edges on cubes, etc. |
| `--help` | Show help message |

### Controls

| Input | Action |
|-------|--------|
| Left Mouse Drag | Orbit camera |
| Middle Mouse Drag | Pan camera |
| Right Click | Select object (click background to deselect) |
| Scroll Wheel | Zoom in/out |
| Arrow Keys | Orbit camera |
| S | Subdivide mesh (Loop - smooth) |
| D | Subdivide mesh (midpoint - keeps shape) |
| W | Toggle wireframe |
| C | Toggle back-face culling |
| G | Toggle frustum culling |
| L | Toggle LOD system |
| K | Toggle LOD debug colors |
| F | Focus on scene |
| H | Show help overlay (keyboard shortcuts) |
| ESC | Cancel active subdivision/LOD generation / Exit |

Mesh statistics (vertices, triangles) are printed to the terminal after loading and subdivision.

## Project Structure

```
OpenGL/
├── src/
│   ├── app/                  # Application layer
│   │   ├── main.cpp          # Entry point
│   │   └── Application.h/cpp # Main application
│   ├── core/                 # Window, Shader, Timer
│   ├── util/                 # Utilities (Result, TextRenderer)
│   ├── async/                # Background task system
│   │   ├── TaskManager.h     # Template base for background tasks
│   │   ├── Progress.h        # Unified progress tracking
│   │   ├── SubdivisionTask.h # Subdivision task data
│   │   ├── LODTask.h         # LOD generation task data
│   │   └── TessellationTask.h # Tessellation task data
│   ├── renderer/             # Camera, Renderer
│   ├── scene/                # Scene graph, Objects
│   ├── mesh/                 # Mesh loading and GPU resources
│   ├── geometry/             # Subdivision algorithms
│   ├── lod/                  # Level of Detail system
│   ├── multipatch/           # G+Smo multipatch support
│   │   ├── GismoLoader.h/cpp # G+Smo XML file loader
│   │   ├── PatchObject.h/cpp # Patch with dynamic tessellation
│   │   ├── MultiPatchManager.h/cpp # View-dependent refinement
│   │   └── TessellationManager.h/cpp # Background tessellation
│   └── ui/                   # User interface overlays
├── shaders/
│   ├── mesh.vert/frag        # Main mesh rendering
│   ├── picking.vert/frag     # Object picking
│   ├── text.vert/frag        # Text rendering
│   └── background.vert/frag  # Gradient background
├── assets/
│   ├── meshes/               # Sample OBJ files
│   └── gismo/                # Sample G+Smo multipatch files
└── external/                 # Third-party libraries (GLAD, GLM, tinyobjloader)
```

## Roadmap

- [x] Object picking and selection
- [x] Mesh subdivision (Loop and midpoint)
- [x] Parallel subdivision with OpenMP
- [x] GPU double-buffering for geometry updates
- [x] Back-face culling with toggle
- [x] Cached normal matrix optimization
- [x] In-window help overlay (H key)
- [x] Rim lighting for better mesh visibility
- [x] Gradient background
- [x] Object-level frustum culling (G key)
- [x] Background tessellation with progress indicators
- [x] LOD (Level of Detail) system with QEM mesh simplification
- [x] Code refactoring (TaskManager template, shared TextRenderer, unified Progress)
- [x] G+Smo multipatch support with view-dependent tessellation
- [ ] GPU-based subdivision (compute shaders)
- [ ] Material and texture support

## Third-Party Libraries

- [GLFW](https://www.glfw.org/) - Windowing and input
- [GLAD](https://glad.dav1d.de/) - OpenGL loader
- [GLM](https://github.com/g-truc/glm) - Mathematics library
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) - OBJ file parsing
- [OpenMP](https://www.openmp.org/) - Parallel processing
- [G+Smo](https://github.com/gismo/gismo) - Geometry + Simulation Modules (optional, for multipatch support)

## License

MIT License - See [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.
