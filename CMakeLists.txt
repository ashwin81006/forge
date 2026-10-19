cmake_minimum_required(VERSION 3.20)

project(forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(src)
include_directories(src/huffman)
include_directories(src/lzw)
include_directories(src/io)
include_directories(src/engine)
include_directories(src/archive)
include_directories(src/benchmark)

# Core engine sources
set(FORGE_ENGINE_SOURCES
    src/huffman/Huffman.cpp
    src/lzw/LZW.cpp
    src/engine/CompressionEngine.cpp
    src/archive/ArchiveWriter.cpp
    src/archive/ArchiveReader.cpp
    src/benchmark/Benchmark.cpp
)

# Main executable (CLI & test runner)
add_executable(forge
    main.cpp
    ${FORGE_ENGINE_SOURCES}
)

# Modular test targets
add_executable(test_huffman tests/test_huffman.cpp src/huffman/Huffman.cpp)
add_executable(test_lzw tests/test_lzw.cpp src/lzw/LZW.cpp)
add_executable(test_engine tests/test_engine.cpp ${FORGE_ENGINE_SOURCES})
add_executable(test_archive tests/test_archive.cpp ${FORGE_ENGINE_SOURCES})
add_executable(test_benchmark tests/test_benchmark.cpp ${FORGE_ENGINE_SOURCES})
add_executable(test_roundtrip tests/test_roundtrip.cpp ${FORGE_ENGINE_SOURCES})

# Optional Qt GUI (Engine runs without Qt)
find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Widgets QUIET)
endif()

if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    add_executable(forge_gui
        src/gui/main_gui.cpp
        src/gui/MainWindow.cpp
        ${FORGE_ENGINE_SOURCES}
    )
    target_link_libraries(forge_gui PRIVATE Qt6::Widgets)
elseif(Qt5_FOUND)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    set(CMAKE_AUTOUIC ON)
    add_executable(forge_gui
        src/gui/main_gui.cpp
        src/gui/MainWindow.cpp
        ${FORGE_ENGINE_SOURCES}
    )
    target_link_libraries(forge_gui PRIVATE Qt5::Widgets)
endif()
