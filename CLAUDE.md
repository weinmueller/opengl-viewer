# Claude Context - OpenGL Viewer

## Project Overview
High-performance OpenGL 4.6 viewer designed for large, complex CAD meshes. Built with a focus on fast parallel rendering and extensibility.

## Architecture

### Core Components
- **Window** (`src/core/Window.h`) - GLFW wrapper with input callbacks
- **Shader** (`src/core/Shader.h`) - Shader compilation and uniform management
- **Timer** (`src/core/Timer.h`) - Frame timing and FPS calculation

### Rendering
- **Renderer** (`src/renderer/Renderer.h`) - Main render loop with Blinn-Phong lighting
- **Camera** (`src/renderer/Camera.h`) - Orbit camera with pan/zoom

### Scene Management
- **Scene** (`src/scene/Scene.h`) - Collection of scene objects
- **SceneObject** (`src/scene/SceneObject.h`) - Renderable object with transform
- **BoundingBox** (`src/scene/BoundingBox.h`) - AABB for culling/picking

### Mesh System
- **Mesh** (`src/mesh/Mesh.h`) - GPU resources (VAO/VBO/EBO) using DSA
- **MeshData** (`src/mesh/MeshData.h`) - CPU-side vertex/index data
- **MeshLoader** (`src/mesh/MeshLoader.h`) - Abstract loader interface
- **ObjLoader** (`src/mesh/ObjLoader.h`) - OBJ file support via tinyobjloader

### Geometry Processing
- **Subdivision** (`src/geometry/Subdivision.h`) - Loop and midpoint subdivision algorithms
- **SubdivisionManager** (`src/geometry/SubdivisionManager.h`) - Background thread management for subdivision
- **SubdivisionTask** (`src/geometry/SubdivisionTask.h`) - Task and progress tracking structures

### Level of Detail (LOD)
- **LODLevel** (`src/lod/LODLevel.h`) - Single LOD level with mesh data and threshold
- **LODMesh** (`src/lod/LODMesh.h`) - Container managing multiple LOD levels per object
- **LODSelector** (`src/lod/LODSelector.h`) - Screen-space size calculation for LOD selection
- **MeshSimplifier** (`src/lod/MeshSimplifier.h`) - QEM-based mesh decimation algorithm
- **LODManager** (`src/lod/LODManager.h`) - Background thread manager for LOD generation
- **LODTask** (`src/lod/LODTask.h`) - Task and progress tracking for LOD generation

### User Interface
- **HelpOverlay** (`src/ui/HelpOverlay.h`) - In-window help with bitmap font text rendering
- **ProgressOverlay** (`src/ui/ProgressOverlay.h`) - Subdivision and LOD generation progress bar display

## Technology Stack
- C++17
- OpenGL 4.6 Core Profile (DSA - Direct State Access)
- OpenMP (parallel subdivision)
- GLFW3 (windowing)
- GLAD (OpenGL loader)
- GLM (math)
- tinyobjloader (OBJ parsing)

## Build Commands
```bash
cd build && cmake .. && make
./MeshViewer assets/meshes/cube.obj

# With crease angle to preserve sharp edges (default is 180 = smooth all)
./MeshViewer --angle 30 assets/meshes/cube.obj
```

## Current Features
- OBJ mesh loading (multiple files supported)
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
- [ ] Material support (MTL files)
- [ ] Texture mapping
- [ ] Screenshot export (PNG)
- [ ] Camera animation/keyframes
- [ ] Measurement tools
- [ ] Section planes/clipping

## Code Conventions
- Use modern C++ (smart pointers, RAII)
- OpenGL 4.6 DSA functions (glCreate*, glNamed*)
- Header-only for simple structs (MeshData, BoundingBox, Timer)
- Separate .h/.cpp for classes with implementation

## Key Files for Modifications
- Add new loaders: Inherit from `MeshLoader`, register in `MeshLoader::createForFile()`
- Add rendering features: Modify `Renderer::render()` and shaders
- Add input handling: Use callbacks in `Application::setupCallbacks()`
