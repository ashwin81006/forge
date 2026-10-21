cmake_minimum_required(VERSION 3.20)

project(forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH "C:/Qt/6.9.3/mingw_64")
endif()

find_package(Qt6 6.9.3 REQUIRED COMPONENTS Widgets)

add_executable(forge
    src/gui/main_gui.cpp
    src/gui/MainWindow.cpp
    src/huffman/Huffman.cpp
    src/lzw/LZW.cpp
    src/engine/CompressionEngine.cpp
    src/archive/ArchiveWriter.cpp
    src/archive/ArchiveReader.cpp
)

target_include_directories(forge PRIVATE
    .
    src
    src/huffman
    src/lzw
    src/io
    src/engine
    src/archive
    src/gui
)

target_link_libraries(forge PRIVATE Qt6::Widgets)