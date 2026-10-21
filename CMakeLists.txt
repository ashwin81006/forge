cmake_minimum_required(VERSION 3.20)

project(forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(.)
include_directories(src)
include_directories(src/huffman)
include_directories(src/lzw)
include_directories(src/io)
include_directories(src/engine)
include_directories(src/archive)
include_directories(src/gui)

# Core engine sources
set(FORGE_ENGINE_SOURCES
    src/huffman/Huffman.cpp
    src/lzw/LZW.cpp
    src/engine/CompressionEngine.cpp
    src/archive/ArchiveWriter.cpp
    src/archive/ArchiveReader.cpp
)

# Automatically locate Qt6 on Windows
if(WIN32)
    # If CMAKE_PREFIX_PATH was specified as C:/Qt/6.9.3, also check the kit folder
    if(DEFINED CMAKE_PREFIX_PATH AND EXISTS "${CMAKE_PREFIX_PATH}/mingw_64")
        list(APPEND CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}/mingw_64")
    endif()

    list(APPEND CMAKE_PREFIX_PATH
        "C:/Qt/6.9.3/mingw_64"
        "C:/Qt/6.9.3"
    )
    file(GLOB QT6_CANDIDATES "C:/Qt/6.*/mingw_64" "C:/Qt/6.*/msvc*")
    if(QT6_CANDIDATES)
        list(APPEND CMAKE_PREFIX_PATH ${QT6_CANDIDATES})
    endif()
endif()

# Qt GUI Application (Qt6 preferred, fallback to Qt5)
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets QUIET)
endif()

if(NOT Qt6_FOUND AND NOT Qt5_FOUND)
    message(FATAL_ERROR
        "\n========================================================================\n"
        "Qt could not be found automatically by CMake.\n"
        "Please specify your Qt installation path using:\n"
        "  cmake -S . -B build -DCMAKE_PREFIX_PATH=\"C:/Qt/6.9.3/mingw_64\"\n"
        "Or simply run 'run_gui.bat' which configures this automatically.\n"
        "========================================================================\n")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(forge
    src/gui/main_gui.cpp
    src/gui/MainWindow.cpp
    ${FORGE_ENGINE_SOURCES}
)

if(Qt6_FOUND)
    target_link_libraries(forge PRIVATE Qt6::Widgets)
else()
    target_link_libraries(forge PRIVATE Qt5::Widgets)
endif()
