# Claude Context - OpenGL Viewer

## Project Overview
High-performance OpenGL 4.6 viewer designed for large, complex CAD meshes. Built with a focus on fast parallel rendering and extensibility.

## Architecture

### Application Layer (`src/app/`)
- **Application** (`src/app/Application.h`) - Main application loop and input handling
- **main.cpp** (`src/app/main.cpp`) - Entry point and command-line parsing

### Core Components (`src/core/`)
- **Window** (`src/core/Window.h`) - GLFW wrapper with input callbacks
- **Shader** (`src/core/Shader.h`) - Shader compilation and uniform management
- **Timer** (`src/core/Timer.h`) - Frame timing and FPS calculation
- **Texture** (`src/core/Texture.h`) - OpenGL texture loading with DSA and mipmapping

### Utilities (`src/util/`)
- **Result** (`src/util/Result.h`) - Generic error handling type (`Result<T, E>`)
- **TextRenderer** (`src/util/TextRenderer.h`) - Shared bitmap font rendering for UI overlays

### Async Task System (`src/async/`)
- **TaskManager** (`src/async/TaskManager.h`) - Template base class for background task processing
- **Progress** (`src/async/Progress.h`) - Unified atomic progress tracking struct
- **SubdivisionTask** (`src/async/SubdivisionTask.h`) - Subdivision task data and progress
- **LODTask** (`src/async/LODTask.h`) - LOD generation task data and progress
- **TessellationTask** (`src/async/TessellationTask.h`) - Patch tessellation task data and progress

### Rendering (`src/renderer/`)
- **Renderer** (`src/renderer/Renderer.h`) - Main render loop with Blinn-Phong lighting
- **Camera** (`src/renderer/Camera.h`) - Orbit camera with pan/zoom

### Scene Management (`src/scene/`)
- **Scene** (`src/scene/Scene.h`) - Collection of scene objects
- **SceneObject** (`src/scene/SceneObject.h`) - Renderable object with transform
- **BoundingBox** (`src/scene/BoundingBox.h`) - AABB for culling/picking

### Mesh System (`src/mesh/`)
- **Mesh** (`src/mesh/Mesh.h`) - GPU resources (VAO/VBO/EBO) using DSA
- **MeshData** (`src/mesh/MeshData.h`) - CPU-side vertex/index data
- **MeshLoader** (`src/mesh/MeshLoader.h`) - Abstract loader interface
- **ObjLoader** (`src/mesh/ObjLoader.h`) - OBJ file support via tinyobjloader

### Geometry Processing (`src/geometry/`)
- **Subdivision** (`src/geometry/Subdivision.h`) - Loop and midpoint subdivision algorithms
- **SubdivisionManager** (`src/geometry/SubdivisionManager.h`) - Inherits TaskManager for background subdivision

### Level of Detail (`src/lod/`)
- **LODLevel** (`src/lod/LODLevel.h`) - Single LOD level with mesh data and threshold
- **LODMesh** (`src/lod/LODMesh.h`) - Container managing multiple LOD levels per object
- **LODSelector** (`src/lod/LODSelector.h`) - Namespace with screen-space LOD selection functions
- **MeshSimplifier** (`src/lod/MeshSimplifier.h`) - QEM-based mesh decimation algorithm
- **LODManager** (`src/lod/LODManager.h`) - Inherits TaskManager for background LOD generation

### Multipatch System (`src/multipatch/`)
- **GismoLoader** (`src/multipatch/GismoLoader.h`) - Loads G+Smo XML multipatch files, tessellates patches
- **PatchObject** (`src/multipatch/PatchObject.h`) - SceneObject subclass with dynamic tessellation support
- **MultiPatchManager** (`src/multipatch/MultiPatchManager.h`) - Manages view-dependent tessellation refinement
- **TessellationManager** (`src/multipatch/TessellationManager.h`) - Inherits TaskManager for background tessellation

### User Interface (`src/ui/`)
- **HelpOverlay** (`src/ui/HelpOverlay.h`) - In-window help display (uses TextRenderer)
- **ProgressOverlay** (`src/ui/ProgressOverlay.h`) - Progress bar display (uses TextRenderer)

## Technology Stack
- C++17
- OpenGL 4.6 Core Profile (DSA - Direct State Access)
- OpenMP (parallel subdivision)
- GLFW3 (windowing)
- GLAD (OpenGL loader)
- GLM (math)
- tinyobjloader (OBJ parsing)
- stb_image (texture loading - PNG, JPG, TGA, BMP)
- G+Smo (optional, multipatch NURBS/B-spline geometry)

## Build Commands
```bash
cd build && cmake .. && make
./MeshViewer assets/meshes/cube.obj

# With crease angle to preserve sharp edges (default is 180 = smooth all)
./MeshViewer --angle 30 assets/meshes/cube.obj

# Load textured OBJ (MTL file with map_Kd texture reference)
./MeshViewer assets/meshes/textured/textured_cube.obj

# Use specific default texture for all objects
./MeshViewer --texture checker cube.obj      # Built-in checker
./MeshViewer --texture wood sphere.obj       # Built-in wood
./MeshViewer -t uv_test torus.obj            # Short flag

# Built-in textures: default_grid, checker, uv_test, brushed_metal, wood, concrete

# Load G+Smo multipatch (requires G+Smo)
./MeshViewer assets/gismo/teapot.xml

# G+Smo cmake options
cmake -DWITH_GISMO=OFF ..             # Disable G+Smo support
cmake -DGISMO_ROOT=/path/to/gismo ..  # Specify G+Smo location
# Or set environment: export GISMO_ROOT=/path/to/gismo
```

## Current Features
- OBJ mesh loading (multiple files supported) with MTL material support
- **Diffuse texture mapping** via MTL `map_Kd` (PNG, JPG, TGA, BMP formats) - toggle with T key
- **Built-in textures** - default_grid, checker, uv_test, brushed_metal, wood, concrete
- Default texture via `--texture` flag or `assets/textures/default_grid.png`
- Object picking with right-click selection
- Selection highlighting (orange tint)
- Mesh subdivision (S = Loop smooth, D = midpoint) - only visible objects when none selected
- Smooth subdivision by default (refines all edges); use `--angle` for crease preservation
- **Parallel Loop subdivision** using OpenMP (4-5x speedup on multi-core CPUs)
- **Background tessellation** with progress indicators (non-blocking UI during subdivision)
- **GPU double-buffering** with fence synchronization for smooth geometry updates
- Configurable crease angle via `--angle` command line option (default: 180°, use lower values like 30° to preserve sharp edges)
- Automatic normal recalculation after subdivision for correct lighting
- Automatic vertex welding for meshes with split vertices (per-face normals)
- Mesh statistics printed to terminal
- Orbit camera (left mouse or arrow keys), pan (middle mouse), zoom (scroll)
- Blinn-Phong lighting with directional light
- Rim/Fresnel lighting for better edge visibility
- Gradient background (dark blue vertical gradient)
- Wireframe toggle (W key)
- Back-face culling toggle (C key) for performance vs. mesh compatibility
- Focus on scene (F key)
- In-window help overlay (H key) with toggle state indicators
- Frustum culling toggle (G key)
- **LOD (Level of Detail) system** with automatic generation on mesh load
- QEM-based mesh simplification (6 LOD levels: 100%, 70%, 50%, 35%, 25%, 15%)
- LOD toggle (L key) and debug colors (K key) showing LOD levels
- Screen-space LOD selection with hysteresis to prevent popping
- Automatic LOD regeneration after mesh subdivision
- ESC cancels active subdivision/LOD generation (or exits if idle)
- **G+Smo multipatch support** - Load NURBS/B-spline geometries from XML files
- **View-dependent tessellation** - Automatic patch refinement based on screen-space size
- Tessellation levels: 4, 8, 12, 16, 24, 32, 48, 64, 128 samples per direction
- Background tessellation with progress overlay
- Per-patch screen-space size calculation with hysteresis to prevent thrashing

## Future Improvements

### Performance Optimizations
- [x] Parallel subdivision using OpenMP
- [x] Double-buffered geometry with fence synchronization
- [x] Back-face culling (toggle with C key)
- [x] Cached normal matrix (avoids per-frame inverse computation)
- [x] Object-level frustum culling (G key to toggle)
- [x] Background tessellation thread with progress indicators
- [ ] GPU-based subdivision (compute shaders)
- [ ] Add `glMultiDrawElementsIndirect` for batched rendering
- [x] Implement LOD (Level of Detail) system
- [ ] Add occlusion culling for complex scenes
- [ ] Persistent mapped buffers for streaming updates

### User Interface
- [x] Improved mesh visibility (rim lighting)
- [x] Gradient background
- [x] In-window help overlay (H key to show controls)

### Additional Features
- [x] G+Smo multipatch support with view-dependent tessellation
- [x] Material support (MTL files) - diffuse texture path extraction
- [x] Texture mapping - diffuse textures with mipmapping
- [ ] Screenshot export (PNG)
- [ ] Camera animation/keyframes
- [ ] Measurement tools
- [ ] Section planes/clipping

## Code Conventions
- Use modern C++ (smart pointers, RAII)
- OpenGL 4.6 DSA functions (glCreate*, glNamed*)
- Header-only for simple structs (MeshData, BoundingBox, Timer, Progress)
- Separate .h/.cpp for classes with implementation
- Template classes for shared patterns (TaskManager)
- Namespaces for stateless utility functions (LODSelector)

## Key Files for Modifications
- Add new loaders: Inherit from `MeshLoader`, register in `MeshLoader::createForFile()`
- Add rendering features: Modify `Renderer::render()` and shaders
- Add input handling: Use callbacks in `Application::setupCallbacks()`
- Add background tasks: Inherit from `TaskManager<YourTaskType>` in `src/async/`
- Add UI overlays: Use `TextRenderer` for text/quad rendering
- Modify tessellation levels: Edit `TessellationThresholds` in `MultiPatchManager.h` and levels array in `.cpp`
- Add new patch types: Extend `PatchObject` or add new geometry evaluation in `GismoLoader::tessellatePatch()`
- Add new textures: Place PNG/JPG files in `assets/textures/`, use via `--texture filename`

## Design Patterns

### TaskManager Template
Background task managers inherit from `TaskManager<TaskType>`:
```cpp
class SubdivisionManager : public TaskManager<SubdivisionTask> {
protected:
    void processTask(SubdivisionTask& task) override;
    bool applyTaskResult(SubdivisionTask& task) override;
};
```

### Progress Tracking
All background tasks use unified `Progress` struct with atomic members:
- `phase`, `phaseProgress`, `totalProgress` - track completion
- `cancelled`, `completed`, `hasError` - status flags
- `phaseNames` - configurable phase descriptions

### Result Type
Use `Result<T>` for operations that can fail:
```cpp
Result<MeshData> load() {
    if (error) return Result<MeshData>::error("Failed");
    return Result<MeshData>::ok(data);
}
```

### View-Dependent Tessellation
MultiPatchManager handles dynamic tessellation refinement:
- Each patch stores a tessellation callback (captures G+Smo geometry reference)
- Screen-space size calculated per patch using bounding box projection
- Tessellation level selected via logarithmic interpolation (4→128)
- Hysteresis (20%) prevents rapid level switching
- Background tessellation via TessellationManager

```cpp
// Tessellation levels based on screen size
TessellationThresholds thresholds;
thresholds.minLevel = 4;      // Below 20px screen size
thresholds.maxLevel = 128;    // Above 500px screen size
```
