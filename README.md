# Procedural Terrain-Based City Builder
A 3D city-building simulation featuring procedurally generated terrain with dynamic texturing, building placement mechanics, day-night cycles, and ocean rendering. Built with modern OpenGL.

## Features

### Terrain Generation
- Procedurally generated heightmap terrain using Perlin noise
- Splatmap texture blending system combining 4 different ground textures
- Dynamic texturing based on:
  - Slope angle (rock textures on steep areas)
  - Elevation (snow at high altitudes, sand at low elevations)
- Vertex shader-based height displacement
- Automatically generated normal vectors

### Building System
- 5 distinct building types:
  - Studio Flat (simple cube)
  - Small House (cube with pyramid roof)
  - Family Home (L-shaped structure)
  - Tower (cylinder with circular roof)
  - Apartment Block (tall rectangular building)
- Building placement with:
  - UI selection of building type
  - Terrain flattening under buildings
  - Collision detection
  - Concrete foundation texture painting (via splatmap)
- Framebuffer-based picking system for accurate placement

### Environment
- Dynamic day/night cycle with:
  - Changing directional light color (yellowish at dawn, white at noon, orange at dusk)
  - Procedural skybox with color gradients
  - Sun/moon visualization
- Water simulation surrounding the island:
  - Wave animation using trigonometric functions
  - Proper normal calculation for reflections
  - Procedurally lowered edges to create natural shoreline
  - Restrictions against building underwater
 
### Dependencies
- OpenGL (4.3+)
- SDL2 (Cross-platform window and event management)
- SDL2-image with libjpeg-turbo support (Image loading)
- GLEW (OpenGL extension loading)
- GLM (OpenGL Mathematics library)
- Dear ImGui (v1.89+) with SDL2 and OpenGL3 bindings
