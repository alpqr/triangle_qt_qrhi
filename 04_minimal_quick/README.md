Minimal, QRhi-based, portable application to render a rotating triangle under the Qt Quick scene.

Effectively the underlay approach described in https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html#extending-the-scene-graph-with-qrhi-based-and-native-3d-rendering

3D API selection logic is defined by Qt Quick: defaults to D3D11 on Windows, Metal on macOS/iOS, OpenGL elsewhere.
See https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html#rendering-via-the-qt-rendering-hardware-interface for ways to override this.
