cmake_minimum_required(VERSION 3.16)
project(CozyRoomDiorama LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # nice for the VS Code C/C++ extension

# ---------------------------------------------------------------------------
# Third-party libraries (all vendored in external/, nothing to install)
# ---------------------------------------------------------------------------
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND  OFF CACHE BOOL "" FORCE)   # Linux only, ignored on macOS/Windows
add_subdirectory(external/glfw)

add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

add_library(imgui STATIC
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/imgui_impl_glfw.cpp
    external/imgui/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC external/imgui)
target_link_libraries(imgui PUBLIC glfw)

# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB APP_HEADERS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.h)
add_executable(CozyRoom ${APP_SOURCES} ${APP_HEADERS})

target_include_directories(CozyRoom PRIVATE
    src
    external/glm
    external/stb)
target_link_libraries(CozyRoom PRIVATE glfw glad imgui ${CMAKE_DL_LIBS})

# Shaders and textures are loaded from the source tree, so the program can be
# started from any working directory (VS Code, terminal, Xcode...).
target_compile_definitions(CozyRoom PRIVATE RESOURCE_DIR="${CMAKE_SOURCE_DIR}")
if(APPLE)
    target_compile_definitions(CozyRoom PRIVATE GL_SILENCE_DEPRECATION)
endif()

if(MSVC)
    target_compile_options(CozyRoom PRIVATE /W3)
else()
    target_compile_options(CozyRoom PRIVATE -Wall -Wextra -Wno-unused-parameter)
    # third-party stb implementation: silence its warnings
    set_source_files_properties(src/StbImpl.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()
