cmake_minimum_required(VERSION 3.20)
project(minimal_quick_rendernode LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core Gui Qml Quick ShaderTools OPTIONAL_COMPONENTS GuiPrivate)

qt_add_executable(minimal_quick_rendernode
    main.cpp
    rhirendernode.cpp rhirendernode.h
)

target_link_libraries(minimal_quick_rendernode PRIVATE
    Qt::Core
    Qt::GuiPrivate
    Qt::Qml
    Qt::Quick
)

qt_add_shaders(minimal_quick_rendernode "shaders"
    PREFIX
        "/shaders"
    FILES
        "color.vert"
        "color.frag"
)

qt_add_qml_module(minimal_quick_rendernode
    URI TestApp
    QML_FILES
        main.qml
    RESOURCE_PREFIX
        /
    NO_RESOURCE_TARGET_PATH
)
