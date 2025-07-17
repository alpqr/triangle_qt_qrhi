Minimal, QRhiWidget-based, portable application to render a rotating triangle.

Much less source code than minimal_window, because Qt takes care of a lot under the hood.

3D API selection logic is defined by QRhiWidget: defaults to D3D11 on Windows, Metal on macOS/iOS, OpenGL elsewhere. See https://doc.qt.io/qt-6/qrhiwidget.html#setApi

To be precise, the rendering here targets a texture that is then composited with the rest of the QWidget content in the window, although this is pretty much hidden to the example code.
