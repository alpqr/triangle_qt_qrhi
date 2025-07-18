cmake_minimum_required(VERSION 3.20)
project(minimal_quick_item LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Gui Qml Quick ShaderTools OPTIONAL_COMPONENTS GuiPrivate)

qt_add_executable(minimal_quick_item
    main.cpp
    rhiitem.cpp rhiitem.h
)

target_link_libraries(minimal_quick_item PRIVATE
    Qt::Core
    Qt::GuiPrivate
    Qt::Qml
    Qt::Quick
)

qt_add_shaders(minimal_quick_item "shaders"
    PREFIX
        "/shaders"
    FILES
        "color.vert"
        "color.frag"
)

qt_add_qml_module(minimal_quick_item
    URI TestApp
    QML_FILES
        main.qml
    RESOURCE_PREFIX
        /
    NO_RESOURCE_TARGET_PATH
)
