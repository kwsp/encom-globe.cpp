cmake_minimum_required(VERSION 3.25)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(EncomGlobe 
  LANGUAGES C CXX
  VERSION 0.1.0
)

# Enable testing
enable_testing()

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick ShaderTools QuickControls2)
find_package(Qt6 COMPONENTS GuiPrivate)  # For RHI headers

# Core library
add_subdirectory(lib)

# Example application
add_subdirectory(example)
