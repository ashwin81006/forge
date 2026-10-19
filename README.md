# FORGE: Full Optimization & Recursive Generation Engine

FORGE is a modular, high-performance lossless compression, archiving, and benchmarking engine built in C++17. Designed as an academic Design and Analysis of Algorithms (DAA) project, it combines classical prefix coding (Huffman), dictionary coding (LZW), and block-level adaptive selection to deliver optimal storage density, deterministic round-trip fidelity, and robust archive security.

---

## Key Features

- **Adaptive Block-Level Method Selection**: Automatically evaluates **RAW**, **Huffman**, and **LZW** per block and selects $\text{argmin}(S_m)$, storing the minimum total size including metadata.
- **Bounded-Memory Stream Processing**: Configurable block sizes (16 KiB, 64 KiB default, 256 KiB, 1024 KiB) allow processing arbitrarily large files without loading them entirely into RAM.
- **Independent Algorithm State**: Huffman tree and LZW dictionary states are completely reset across blocks, preventing cross-block state contamination.
- **Custom `.forge` Binary Archive Format**: Robust little-endian binary layout storing directory structures, file metadata, and block payloads.
- **Cryptographic & Integrity Verification**: File-level SHA-256 validation combined with per-block CRC-32 checksums ensure exact lossless recovery.
- **Security by Design**: Native protection against path-traversal attacks (`..`, drive letters, leading slashes) and corrupt/malicious payload bounds checking.
- **Comprehensive Benchmark Laboratory**: Evaluates compression ratio, space saved, throughput, and memory across multiple data topologies.
- **Decoupled Architecture**: Compression engine compiles and operates completely independently of GUI frameworks; optional Qt Widgets interface connects strictly via engine APIs.

---

## Project Structure

```text
FORGE/
├── CMakeLists.txt              # Standard CMake build configuration
├── main.cpp                    # Unified CLI tool & verification test suite
├── src/
│   ├── huffman/                # Huffman encoder & canonical decoder
│   ├── lzw/                    # 16-bit fixed-width LZW codec
│   ├── io/                     # BitWriter, BitReader, BinaryIO, Checksum (CRC32 & SHA-256)
│   ├── engine/                 # CompressionEngine, block processing & adaptive selection
│   ├── archive/                # ArchiveWriter, ArchiveReader, PathSecurity
│   ├── benchmark/              # BenchmarkSuite & report generation
│   └── gui/                    # Qt Widgets desktop interface (MainWindow)
├── tests/                      # Modular unit, integration, and round-trip tests
├── docs/                       # Formal design docs & benchmark reports
├── project_memory.json         # Persistent machine-readable development state
└── project_memory.md           # Persistent human-readable architecture log
```

---

## Build Instructions

### Prerequisites
- C++17 compliant compiler (`g++`, `clang++`, or `MSVC`)
- CMake 3.20+ (optional, can also compile directly via GCC)
- Optional: Qt6 or Qt5 Widgets (for GUI build)

### Building with CMake
```bash
cmake -B build -S .
cmake --build build --config Release
```

### Direct Compilation with GCC (No CMake required)
```bash
# Build unified CLI and test suite
g++ -std=c++17 -O2 -Wall -Wextra main.cpp \
    src/huffman/Huffman.cpp \
    src/lzw/LZW.cpp \
    src/engine/CompressionEngine.cpp \
    src/archive/ArchiveWriter.cpp \
    src/archive/ArchiveReader.cpp \
    src/benchmark/Benchmark.cpp \
    -o forge.exe
```

---

## Usage & Example Commands

### 1. Run Complete Verification & Benchmarks
```bash
./forge
```

### 2. Compress a Folder to `.forge` Archive
```bash
./forge -c samples/dataset -o backup.forge
```

### 3. List Archive Contents
```bash
./forge -l backup.forge
```

### 4. Extract `.forge` Archive with SHA-256 Verification
```bash
./forge -x backup.forge -o restored_folder
```

### 5. Run Benchmark Laboratory
```bash
./forge --bench
```

---

## Test Targets

Modular unit tests are located in `tests/`:
- `tests/test_huffman.cpp`: Huffman tree canonical ordering, edge cases, malformed streams.
- `tests/test_lzw.cpp`: LZW 16-bit code stream, dictionary saturation (>65,536 entries), corruption rejection.
- `tests/test_engine.cpp`: Block-level resets, bounded memory streaming, adaptive candidate selection.
- `tests/test_archive.cpp`: Nested directory preservation, path-traversal rejection, corrupted archive rejection.
- `tests/test_benchmark.cpp`: Multi-dataset performance metrics and CSV/Markdown export validation.
- `tests/test_roundtrip.cpp`: Large multi-megabyte streams and multi-block-size lossless roundtrips.

---

## Limitations

- **Fixed-Width LZW**: Uses 16-bit fixed-width codes with a 65,536-entry dictionary cap. Variable-bit codes (9-16 bit dynamic growth) are reserved for version 2.
- **Local Single-Node Processing**: Does not include distributed or multi-threaded cloud archiving features.
- **Uncompressed Folders**: Symlinks are intentionally bypassed to prevent circular references and security escapes.
