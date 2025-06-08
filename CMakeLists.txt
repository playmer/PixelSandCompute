cmake_minimum_required(VERSION 3.21)

project(PixelSandCompute)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

add_subdirectory(SDL)

add_executable(PixelSandCompute)

target_include_directories(PixelSandCompute PUBLIC imgui imgui/backends)
target_link_libraries(PixelSandCompute PUBLIC SDL3::SDL3)

target_sources(PixelSandCompute 
PUBLIC 
    main.cpp
    imgui/imgui.cpp
    imgui/imgui.h
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_internal.h
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imstb_rectpack.h
    imgui/imstb_textedit.h
    imgui/imstb_truetype.h
    imgui/backends/imgui_impl_sdl3.cpp
    imgui/backends/imgui_impl_sdl3.h
    imgui/backends/imgui_impl_sdlgpu3.cpp
    imgui/backends/imgui_impl_sdlgpu3.h
    imgui/backends/imgui_impl_sdlgpu3_shaders.h
)

set_property(TARGET PixelSandCompute PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")