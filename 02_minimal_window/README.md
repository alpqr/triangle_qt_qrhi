Minimal, purely QWindow-based (no QWidgets, no Qt Quick), portable application to render a rotating triangle.

3D API selection logic: D3D11 on Windows, Metal on macOS/iOS, otherwise try Vulkan, if all else fails OpenGL. Use command-line arguments to override. See ```minimal_window --help```
