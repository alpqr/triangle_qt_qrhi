Six examples of integrating the same classic rotating triangle using the QRhi APIs into fully portable Qt applications.
Needs Qt >= 6.8.

* 01_minimal_offscreen: no on-screen window, just a console app reading back from a texture and saving to image files
* 02_minimal_window: plain QWindow, not using Qt Widgets or Qt Quick, and having full control over everything
* 03_minimal_widget: mix with widgets, using QRhiWidget
* 04_minimal_quick: underlay (or overlay) approach for Qt Quick
* 05_minimal_quick_item: mix with a Qt Quick scene
* 06_minimal_quick_rendernode: inline rendering for Qt Quick; not ideal for such arbitrary 3D content
