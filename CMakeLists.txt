cmake_minimum_required(VERSION 3.20)

project(forge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(forge
	main.cpp
	src/huffman/Huffman.cpp
	src/lzw/LZW.cpp
)