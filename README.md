# Procedural Terrain-Based City Builder
This repository contains a city-building application where users can place buildings and create settlements on a procedurally generated 3D terrain. The terrain and its textures are created using GPU-based rendering techniques, including procedural Perlin noise and multi-texture blending.

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
