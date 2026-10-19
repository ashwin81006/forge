# FORGE — Project Documentation

## 1. Problem Statement
General-purpose archiving tools often treat compression as a monolithic pipeline, applying a single algorithm to an entire archive or file. In heterogeneous data streams (e.g., binaries containing code segments, repetitive strings, and compressed media), a monolithic strategy fails:
- Dictionary coding (LZW) excels on repeated phrases but suffers overhead on random or short data.
- Entropy coding (Huffman) excels on skewed character distributions but cannot capture multi-byte phrase redundancies.
- Incompressible blocks become larger when compressed due to codebook and header overhead.

FORGE solves this by partitioning files into bounded blocks, dynamically evaluating candidates (**RAW**, **Huffman**, **LZW**), selecting the minimum stored representation including all metadata, and packing them into an integrity-checked, secure binary archive format (`.forge`).

---

## 2. Objectives & Scope
- **Lossless Recovery**: Exact, deterministic bit-for-bit reconstruction verified by SHA-256 and CRC-32.
- **Bounded Memory**: Streaming block-based architecture preventing out-of-memory errors on large datasets.
- **Security**: Strict rejection of directory traversal attacks (`..`, drive letters, absolute paths) and malicious archive size fields.
- **Framework Independence**: The core compression and archive engine compiles with standard C++17 without external dependencies or Qt.
- **Non-Scope**: No databases, cloud storage, dynamic programming or AI prediction models, or variable-width LZW in Version 1.

---

## 3. System Architecture & Module Responsibilities

```text
[ Input File / Folder ]
          │
          ▼
   [ ArchiveWriter ] ──► Traversals & File Path Security
          │
          ▼
 [ CompressionEngine ] ──► Bounded Block Reader (16K - 1024K)
    ├── Huffman Codec  ──► Frequency-based greedy prefix coding
    ├── LZW Codec      ──► 16-bit dictionary phrase coding
    └── RAW Fallback   ──► Unmodified bytes if compression expands
          │
          ▼
   [ Binary Layout ]   ──► .forge (Header + File Records + Block Records)
          │
          ▼
   [ ArchiveReader ]  ──► Validates Header, Blocks, CRC32, SHA-256 & Extracts
```

| Module | Location | Primary Responsibilities |
|:---|:---|:---|
| `io` | `src/io/` | BitWriter, BitReader, BinaryIO (little-endian), CRC32, SHA-256 |
| `huffman` | `src/huffman/` | Min-heap canonical tree construction, prefix bitstream packing |
| `lzw` | `src/lzw/` | 16-bit fixed-width dictionary encoder & mirrored reconstruction decoder |
| `engine` | `src/engine/` | Block chunking, codec state resets, adaptive candidate selection |
| `archive` | `src/archive/` | Directory traversal, `.forge` serialization, path-traversal validation |
| `benchmark` | `src/benchmark/`| Standard test datasets, metrics (ratio, speed, memory), CSV/MD reports |
| `gui` | `src/gui/` | Qt Widgets desktop application interfacing solely via engine APIs |

---

## 4. Technology Stack
- **Language**: C++17 (Strict standard compliance, ISO/IEC 14882:2017)
- **Standard Libraries**: `<filesystem>`, `<chrono>`, `<random>`, `<vector>`, `<queue>`, `<memory>`, `<array>`
- **Build System**: CMake 3.20+ with native GCC/Clang/MSVC toolchain compatibility
- **GUI Framework**: Optional Qt Widgets (Qt6 / Qt5)

---

## 5. Development Milestones Summary
- **M1 (Huffman)**: Canonical deterministic Huffman codec with single-byte, repeated-byte, all-256-symbol handling.
- **M2 (LZW)**: Fixed-width 16-bit dictionary codec with 65,536-entry freeze rule and little-endian code packing.
- **M3 (Block Processing)**: Bounded memory stream processing with independent state reset per block.
- **M4 (Adaptive Selection)**: Cost-model candidate evaluation choosing $\text{argmin}(S_m)$.
- **M5 (Archive Format)**: Custom `.forge` layout with little-endian fields, file headers, and CRC-32 blocks.
- **M6 (Extraction & Security)**: Directory restoration with path-traversal prevention and SHA-256 verification.
- **M7 (Benchmark Laboratory)**: Standard test suite evaluating compression ratio, speed, memory, and block sizes.
- **M8 (Qt Widgets GUI)**: Asynchronous desktop interface for compression, extraction, browsing, and benchmarking.
- **M9 (Final Validation)**: Comprehensive unit tests, end-to-end integration, documentation, and report generation.
