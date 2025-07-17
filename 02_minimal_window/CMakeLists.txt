cmake_minimum_required(VERSION 3.20)
project(minimal_window LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Gui ShaderTools OPTIONAL_COMPONENTS GuiPrivate)

qt_add_executable(minimal_window
    main.cpp
)

target_link_libraries(minimal_window PRIVATE
    Qt::Core
    Qt::GuiPrivate
)

qt_add_shaders(minimal_window "shaders"
    PREFIX
        "/shaders"
    FILES
        "color.vert"
        "color.frag"
)
