# FORGE: File Optimization & Recursive Generation Engine

[![C++17](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![Qt 6](https://img.shields.io/badge/GUI-Qt%206.9-41CD52?style=flat&logo=qt)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/Build-CMake%203.20%2B-064F8C?style=flat&logo=cmake)](https://cmake.org/)
[![Integrity](https://img.shields.io/badge/Integrity-SHA--256%20%2B%20CRC--32-success)](https://en.wikipedia.org/wiki/SHA-2)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)]()

**FORGE** is a modern, high-performance lossless data compression and archive management desktop application written in **C++17** with a **Qt 6** user interface. Engineered for deterministic reliability, storage efficiency, and filesystem safety, FORGE integrates classical prefix coding (Canonical Huffman), dictionary coding (Terry Welch LZW), and intelligent block-level adaptive selection within a custom, secure `.forge` binary container format.

---

## Table of Contents

- [Key Features](#key-features)
- [Architecture & Algorithm Design](#architecture--algorithm-design)
  - [Adaptive Block-Level Engine](#1-adaptive-block-level-engine)
  - [Canonical Huffman Coding](#2-canonical-huffman-coding)
  - [Terry Welch LZW Implementation](#3-terry-welch-lzw-implementation)
  - [Integrity & Path Security](#4-integrity--path-security)
- [Binary Archive Specification (.forge)](#binary-archive-specification-forge)
- [Benchmark & Experimental Results](#benchmark--experimental-results)
- [Project Directory Layout](#project-directory-layout)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Quick Run (Pre-built)](#quick-run-pre-built)
  - [Build from Source](#build-from-source)
- [Desktop GUI Overview](#desktop-gui-overview)
- [Documentation & References](#documentation--references)
- [License](#license)

---

## Key Features

- **Intelligent Adaptive Method Selection**: Evaluates **RAW**, **Huffman**, and **LZW** concurrently for each block, committing only the codec that produces the smallest stored payload including metadata.
- **Zero File Expansion Penalty**: High-entropy or pre-compressed formats (e.g., JPEG, MP4, ZIP, Git packfiles) automatically fall back to RAW blocks, ensuring compressed archives never expand beyond source boundaries.
- **Terry Welch LZW with Custom Hash Table**: Direct-mapped open-addressing hash table storing `(prefix, character)` pairs using bitwise mixing `((prefix << 5) ^ character) % 79003`. Completely eliminates heap allocations and string constructions during compression.
- **Bounded-Memory Stream Processing**: Fixed-memory chunking (16 KiB, 64 KiB default, 256 KiB, 1024 KiB) processes multi-gigabyte directory trees under strict memory limits (< 4 MiB active heap).
- **Dual-Layer Integrity Verification**:
  - **Per-Block CRC-32**: Validates payload integrity immediately upon block decompression.
  - **Per-File SHA-256**: Cryptographically authenticates full end-to-end round-trip reconstruction.
- **Security-First Path Traversal Sandbox**: Enforces strict POSIX relative path normalization, rejecting absolute paths, drive letters (`C:`), null bytes, and directory escapes (`..`).
- **Responsive Qt 6 Desktop Suite**: Threaded background compression/decompression, live percentage and throughput monitors, archive inspection tables, and one-click integrity verification.

---

## Architecture & Algorithm Design

### 1. Adaptive Block-Level Engine
Rather than applying a single compression algorithm globally across an entire file, FORGE divides input files into fixed-size chunks (default: 64 KiB). For each block:
1. The engine compresses the chunk using **Canonical Huffman Coding**.
2. The engine compresses the chunk using **Terry Welch LZW**.
3. It computes the total stored footprint (method ID + payload + metadata).
4. If neither method yields a size smaller than the uncompressed original, the block is stored as **RAW**.

$$\text{Selected Codec} = \arg\min_{m \in \{\text{RAW}, \text{HUFFMAN}, \text{LZW}\}} \big(\text{PayloadSize}_m + \text{MetadataSize}_m\big)$$

### 2. Canonical Huffman Coding
- **Greedy Priority Queue**: Constructs an optimal prefix-free binary tree by iteratively merging the two lowest-frequency nodes.
- **Deterministic Canonical Ordering**: Resolves frequency collisions with `(frequency, minSymbol, order)` tie-breaking, ensuring cross-platform, compiler-invariant canonical bit lengths.
- **Stream Packing**: Serializes code lengths followed by byte-aligned bitstreams using `BitWriter` and `BitReader`.

### 3. Terry Welch LZW Implementation
In standard implementations, LZW dictionary searches rely on dynamic heap strings (e.g. `std::string` keys in `std::unordered_map`), causing heavy allocation overhead. FORGE adopts Terry Welch's classic 1984 hash table architecture:
- **Integer Triplet Representation**: Each dictionary sequence is represented simply by a 16-bit prefix code and an 8-bit character: `(uint16_t prefix, uint8_t character) -> uint16_t code`.
- **Custom Open-Addressing Hash Table**:
  ```cpp
  inline size_t hashKey(uint16_t prefix, uint8_t character) {
      return ((static_cast<size_t>(prefix) << 5) ^ character) % 79003;
  }
  ```
- **Collision Resolution**: Linear probing over a prime-sized table ($79,003$ slots).
- **Dictionary Capacity**: 256 initial single-byte entries, dynamically expanding up to $65,536$ entries (16-bit code space). Once full, the dictionary freezes without clearing, allowing ongoing streaming with existing sequences.

### 4. Integrity & Path Security
- **Path Sanitization**: `PathSecurity::sanitizeRelativePath()` strips leading slashes, eliminates `.` and `..` segments, converts backslashes to `/`, and rejects Windows drive letters.
- **Tamper & Corruption Detection**: Any missing bytes, truncated blocks, or checksum mismatches raise explicit exceptions (`Unexpected EOF`, `CRC-32 mismatch`, `SHA-256 verification failed`), preventing corrupted files from overwriting disk assets.

---

## Binary Archive Specification (.forge)

FORGE archives use a strictly **Little-Endian** binary format:

```text
+-------------------------------------------------------------------+
|                          ARCHIVE HEADER                           |
|  Magic "FORGE" (5B) | Version (2B) | FileCount (8B) | Reserved(8B)|
+-------------------------------------------------------------------+
|                           FILE RECORD 1                           |
|  PathLen (2B) | RelativePath (PathLen) | OrigSize (8B)            |
|  BlockCount (8B) | SHA-256 Digest (32B)                           |
+-------------------------------------------------------------------+
|                         BLOCK RECORD 1.1                          |
|  MethodID (1B) | OrigBlockSize (4B) | PayloadSize (4B)            |
|  MetadataSize (4B) | CRC-32 Checksum (4B)                         |
|  Metadata Bytes (MetadataSize)                                    |
|  Payload Bytes (PayloadSize)                                      |
+-------------------------------------------------------------------+
|                         BLOCK RECORD 1.2                          |
|  ...                                                              |
+-------------------------------------------------------------------+
|                           FILE RECORD 2                           |
|  ...                                                              |
+-------------------------------------------------------------------+
```

### Method Identifiers:
- `0x00`: **RAW** (Uncompressed passthrough)
- `0x01`: **HUFFMAN** (Canonical prefix tree with serialized code table)
- `0x02`: **LZW** (16-bit Terry Welch dictionary codes)

---

## Benchmark & Experimental Results

Evaluated on AMD / Intel x86_64, Windows 11, GCC with `-O2` optimization using standard 64 KiB block chunking:

| Dataset | Type / Content | Original Size | Adaptive Stored | Ratio | Space Saved | Best Codec |
|:---|:---|---:|---:|---:|---:|:---:|
| **Repetitive Text** | Redundant test sequences | 102,400 B | 9,206 B | **11.12x** | **91.0%** | LZW |
| **C++ Source Code** | Source code & headers | 81,920 B | 13,704 B | **5.98x** | **83.3%** | LZW |
| **Natural Language**| Plain English prose | 81,920 B | 17,244 B | **4.75x** | **79.0%** | LZW |
| **Binary Executable**| Compiled binary sections | 65,536 B | 2,706 B | **24.22x** | **95.9%** | LZW |
| **Random Data** | Uniform high-entropy bytes | 65,536 B | 65,536 B | **1.00x** | **0.0%** | RAW (No expansion) |
| **Media / Archives**| JPEG / ZIP pre-compressed | 65,536 B | 65,536 B | **1.00x** | **0.0%** | RAW (No expansion) |

> **Key Observation**: On repetitive and structured data, Terry Welch LZW demonstrates remarkable compression (up to **24.2x**). On incompressible inputs, Adaptive Mode safely activates the **RAW fallback**, ensuring **0.0% file expansion**.

---

## Project Directory Layout

```text
forge/
├── CMakeLists.txt              # Standard CMake build configuration (Qt 6 + C++17)
├── README.md                   # Comprehensive project documentation
├── project_memory.json         # Development milestone tracking and architecture log
│
├── src/
│   ├── archive/                # Archive container writer, reader, and path security
│   │   ├── ArchiveFormat.h     # Binary header layouts, structs, and magic constants
│   │   ├── ArchiveWriter.h/.cpp# Recursive folder packer & streaming block compressor
│   │   ├── ArchiveReader.h/.cpp# Archive reader, TOC parser, and selective extractor
│   │   └── PathSecurity.h      # Path traversal sanitizer and directory bounds checker
│   │
│   ├── engine/                 # Adaptive compression pipeline
│   │   ├── BlockTypes.h        # Method enum, BlockHeader, and block structures
│   │   └── CompressionEngine.h/.cpp # Block compressor, decompressor, and adaptive logic
│   │
│   ├── huffman/                # Canonical Huffman coding
│   │   ├── Huffman.h/.cpp      # Min-heap priority queue, canonical tree, code generator
│   │
│   ├── lzw/                    # Terry Welch LZW codec
│   │   ├── LZW.h/.cpp          # Custom 79k-slot hash table encoder & decoder
│   │
│   ├── io/                     # Low-level I/O & verification
│   │   ├── BinaryIO.h          # Little-endian integer readers/writers
│   │   ├── BitWriter.h         # Byte-aligned bitstream serializer
│   │   ├── BitReader.h         # Streaming bitstream deserializer
│   │   └── Checksum.h          # Fast CRC-32 & full cryptographic SHA-256
│   │
│   └── gui/                    # Qt 6 Desktop User Interface
│       ├── MainWindow.h/.cpp   # Multi-tab GUI, asynchronous worker threads, signals
│       └── main_gui.cpp        # Qt application entry point
│
├── docs/                       # Formal technical specifications
│   ├── ALGORITHM_DESIGN.md     # Mathematical foundations, pseudocode & complexity
│   ├── ARCHIVE_FORMAT.md       # Exact byte offsets and protocol specifications
│   ├── BENCHMARK_REPORT.md     # Detailed benchmark methodology and tables
│   └── PROJECT_DOCUMENTATION.md# Software engineering overview & milestone review
│
└── build/                      # CMake build and runtime directory (forge.exe)
```

---

## Getting Started

### Prerequisites

- **Operating System**: Windows 10/11, Linux, or macOS
- **Compiler**: C++17 compatible compiler (MinGW GCC 11+, MSVC 2019+, or Clang 13+)
- **CMake**: Version 3.20 or newer
- **Qt 6**: Qt 6.2+ (Core, Gui, Widgets)

### Quick Run (Pre-built)

If the standalone executable is already built in your environment:
```powershell
.\build\forge.exe
```

### Build from Source

#### Windows (MinGW + Qt 6)
```powershell
# Configure CMake with Release build type
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Compile the target
cmake --build build --config Release

# Launch FORGE
.\build\forge.exe
```

#### Windows (Visual Studio / MSVC)
```cmd
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
.\build\Release\forge.exe
```

#### Linux / macOS
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/forge
```

---

## Desktop GUI Overview

The Qt 6 interface provides four functional tabs:

1. **Home**:
   - Welcome overview of FORGE capabilities and core compression metrics.
   - Quick navigation shortcuts to create archives or open the explorer.
2. **Create Archive**:
   - **Source Folder**: Select any local directory to compress recursively.
   - **Destination**: Choose target `.forge` archive path.
   - **Block Size**: Configure chunk boundaries (`16 KiB`, `64 KiB`, `256 KiB`, `1024 KiB`).
   - **Live Progress**: Asynchronous progress bar, current file indicator, and thread-safe execution.
3. **Archive Explorer**:
   - **TOC Inspection**: View all files stored in a `.forge` archive with original sizes, compressed sizes, and percentage space saved.
   - **Test Archive**: Run complete SHA-256 and CRC-32 validation across all files without writing to disk.
   - **Extract Archive**: Safely extract all archive contents to a target folder with sandbox path validation.
4. **About**:
   - Algorithm specifications, version details, and technical architecture references.

---

## Documentation & References

For comprehensive technical deep dives, consult the `docs/` folder:
- [**Algorithm Design & Complexity Analysis**](docs/ALGORITHM_DESIGN.md): Detailed proofs of greedy choice property, min-heap priority queue ordering, and time/space complexity analysis ($O(n)$ bounds).
- [**Binary Archive Format Specification**](docs/ARCHIVE_FORMAT.md): Exact byte-level field layouts, magic headers, and checksum placements.
- [**Benchmark Laboratory Report**](docs/BENCHMARK_REPORT.md): Comprehensive test data across repetitive text, source code, natural language, and high-entropy media.
- [**Project Documentation**](docs/PROJECT_DOCUMENTATION.md): Software engineering lifecycle, milestone completions, and testing verification report.

---

## License

This project was developed for academic, educational, and systems engineering research. Open source under standard academic reuse guidelines.
