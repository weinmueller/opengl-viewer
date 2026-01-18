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

## Technology Stack
- C++17
- OpenGL 4.6 Core Profile (DSA - Direct State Access)
- GLFW3 (windowing)
- GLAD (OpenGL loader)
- GLM (math)
- tinyobjloader (OBJ parsing)

## Build Commands
```bash
cd build && cmake .. && make
./MeshViewer assets/meshes/cube.obj
```

## Current Features
- OBJ mesh loading (multiple files supported)
- Object picking with right-click selection
- Selection highlighting (orange tint)
- Mesh subdivision (S = Loop smooth, D = midpoint)
- Boundary-preserving subdivision (corners and edges kept)
- Mesh statistics printed to terminal
- Orbit camera (left mouse), pan (middle mouse), zoom (scroll)
- Blinn-Phong lighting with directional light
- Wireframe toggle (W key)
- Focus on scene (F key)

## Future Improvements

### Phase 7: Performance Optimization (>1M triangles)
- [ ] Parallel subdivision using std::execution or OpenMP
- [ ] Optimize adjacency building with better data structures
- [ ] Reduce memory allocations during subdivision
- [ ] Consider GPU-based subdivision (compute shaders)

### Phase 8: Multi-threaded Geometry Updates
- [ ] Implement GeometryBuffer for large pre-allocated storage
- [ ] Add GeometryUpdate queue for thread-safe updates
- [ ] Double-buffered geometry with fence synchronization
- [ ] Background tessellation thread
- [ ] Progress indicators for long operations

### Performance Optimizations
- [ ] Implement frustum culling (`src/renderer/Frustum.h`)
- [ ] Add `glMultiDrawElementsIndirect` for batched rendering
- [ ] Implement LOD (Level of Detail) system
- [ ] Add occlusion culling for complex scenes
- [ ] Persistent mapped buffers for streaming updates

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
