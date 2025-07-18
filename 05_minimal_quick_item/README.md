Minimal, QRhi-based, portable application to render a rotating triangle as a true QQuickItem.

Implemented using QQuickRhiItem, see https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html#the-texture-based-approach and https://doc.qt.io/qt-6/qquickrhiitem.html

3D API selection logic is defined by Qt Quick: defaults to D3D11 on Windows, Metal on macOS/iOS, OpenGL elsewhere.
See https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph-renderer.html#rendering-via-the-qt-rendering-hardware-interface for ways to override this.

Like minimal_widget, and unlike minimal_window and minimal_quick, the custom QRhi rendering targets a texture in this example, not directly the color buffer for the window/swapchain.
This is what allows Qt Quick to transform and blend the rendered content freely with the rest of the scene, allowing RhiItem to behave like a proper, visual QQuickItem (which, in contrast,
RhiUnderlay in the minimal_quick example was not).
