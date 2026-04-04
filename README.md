# QtQuick project template

A minimal CMake template for cross-platform QtQuick (Qt 6) applications.

## Features

- **Qt 6.5+** with QtQuick and QtQuick.Controls, set up via `find_package` and `qt_add_qml_module`.
- **QML loaded by module name** — project name flows from `CMakeLists.txt` into `main.cpp` automatically via `configure_file`, so renaming the project is a one-line change.
- **CMakePresets** for cross-platform builds:
  - `gcc` — (non-Windows) GCC + Ninja Multi-Config
  - `clang` — (non-Windows) Clang + Ninja Multi-Config
  - `cl` — (Windows x64) MSVC + Ninja Multi-Config
  - `win64` — (Windows x64) Visual Studio 2022
  - `clang-cl` — (Windows x64) ClangCL + Ninja Multi-Config
- **Qt location** is not hardcoded. Set `QTDIR` (or `CMAKE_PREFIX_PATH`) in your environment before configuring:
  - macOS (Homebrew): `export QTDIR=/opt/homebrew/opt/qt6`
  - Linux (installer): `export QTDIR=~/Qt/6.x.x/gcc_64`
  - Windows (installer): `set QTDIR=C:\Qt\6.x.x\msvc2022_64`
- **QML language server** (`qmlls`) configured automatically at CMake configure time via a generated `.qmlls.ini`, with the correct Qt QML import path resolved from `qmake`.
- **GitHub Actions CI** for Linux, macOS, and Windows with Qt cached between runs. macOS produces a self-contained `.app` bundle via `macdeployqt`; Windows produces a deployment directory via `windeployqt`.

## Building

```sh
cmake --preset clang # configure
cmake --build build-clang --config Release # build
```

## Using as a template

1. Change `project(QtQuickPlayground ...)` in `CMakeLists.txt` to your project name.
2. Rename `src/Main.qml` if desired and update `QML_FILES` in `src/CMakeLists.txt`.
3. Set your Qt path in the environment and configure.
