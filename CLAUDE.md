# Claude Context - OpenGL Mesh Viewer

## Project Overview
High-performance OpenGL 4.6 mesh viewer designed for large, complex Parasolid CAD files. Built with a focus on fast parallel rendering and extensibility.

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
- OBJ mesh loading
- Orbit camera (left mouse), pan (middle mouse), zoom (scroll)
- Blinn-Phong lighting with directional light
- Wireframe toggle (W key)
- Focus on scene (F key)

## Future Improvements

### Phase 7: Object Picking
- [ ] Implement color-based picking (render object IDs to framebuffer)
- [ ] Add ray casting alternative for precise picking
- [ ] Highlight selected objects
- [ ] Display object info on selection

### Phase 8: Dear ImGui Integration
- [ ] Add ImGui as external dependency
- [ ] Create dockable UI panels
- [ ] Scene hierarchy panel
- [ ] Object properties panel (transform, color)
- [ ] Rendering settings (lighting, wireframe, culling)
- [ ] Performance stats overlay

### Phase 9: Parasolid Integration
- [ ] Add Parasolid SDK dependency
- [ ] Implement ParasolidLoader class
- [ ] Tessellation quality settings
- [ ] Handle B-rep to mesh conversion
- [ ] Support assembly structures

### Phase 10: Multi-threaded Geometry Updates
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
- [ ] Multiple mesh loading
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
- Add GUI: Integrate ImGui in `Application::render()` after scene rendering
