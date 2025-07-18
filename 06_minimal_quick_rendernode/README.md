Minimal, QRhi-based, portable application to render a rotating triangle using QSGRenderNode backing a QQuickItem.

This example is provided only for completeness, and should generally be used for integrating 3D rendering into the Qt Quick scene. See notes in rhirendernode.cpp.

3D API selection logic is defined by Qt Quick: defaults to D3D11 on Windows, Metal on macOS/iOS, OpenGL elsewhere.
See https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html#rendering-via-the-qt-rendering-hardware-interface for ways to override this.
