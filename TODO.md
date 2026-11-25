- command pool class. 1 command pool per frame
- command encoder class. command pool owns command buffers

- load buffer of triangles into gpu and render that instead of triangles
- part 1: get a gltf to render with camera controls as proof of the RHI.
- MetalCmdEncoder begin_rendering() to not the swapchain and instead to a texture

- target: mesh shader rendering
- Cube in compute shader, with camera controls
- shading
- Chunk rendering: DDA
- Multiple chunks

## Copying buffers
