Minimal, window-less, QRhi-based, portable application to render 20 frames of a triangle into a texture, read it back, and save each frame to png images.

3D API selection logic: D3D11 on Windows, Metal on macOS/iOS, otherwise try Vulkan, if all else fails OpenGL
