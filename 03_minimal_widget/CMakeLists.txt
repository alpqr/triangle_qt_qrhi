cmake_minimum_required(VERSION 3.20)
project(minimal_widget LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Gui Widgets ShaderTools OPTIONAL_COMPONENTS GuiPrivate)

qt_add_executable(minimal_widget
    main.cpp
)

target_link_libraries(minimal_widget PRIVATE
    Qt::Core
    Qt::GuiPrivate
    Qt::Widgets
)

qt_add_shaders(minimal_widget "shaders"
    PREFIX
        "/shaders"
    FILES
        "color.vert"
        "color.frag"
)
