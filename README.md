# FORGE: File Optimization & Recursive Generation Engine

FORGE is a modern, high-performance lossless compression and archiving desktop application built in C++17 and Qt 6. Designed as an intelligent, secure archive manager, it combines classical prefix coding (Huffman), dictionary coding (LZW), and block-level adaptive selection to deliver optimal storage density, deterministic round-trip fidelity, and robust archive security.

---

## Key Features

- **Adaptive Block-Level Method Selection**: Automatically evaluates **RAW**, **Huffman**, and **LZW** per block and stores the minimum total size including metadata.
- **Zero File Expansion**: If data is already compressed or high entropy (e.g. JPEG, MP4, ZIP), FORGE safely defaults to RAW with zero file expansion penalty.
- **Bounded-Memory Stream Processing**: Configurable block sizes (16 KiB, 64 KiB default, 256 KiB, 1024 KiB) allow processing multi-gigabyte archives within low memory constraints.
- **Custom `.forge` Binary Archive Format**: Robust little-endian binary layout storing directory structures, file metadata, and block payloads.
- **Cryptographic & Integrity Verification**: End-to-end file-level SHA-256 validation combined with per-block CRC-32 checksums ensure exact lossless recovery.
- **Security by Design**: Native protection against path-traversal attacks (`..`, drive letters, leading slashes) and corrupt/malicious payload bounds checking.
- **Modern Qt Desktop Interface**: User-friendly GUI featuring one-click compression, real-time progress, archive inspection, and SHA-256 extraction verification.

---

## Project Structure

```text
FORGE/
├── CMakeLists.txt              # Standard CMake build configuration
├── run_gui.bat                 # One-click desktop GUI launcher
├── build/                      # Fresh standalone build directory with all runtime DLLs
├── src/
│   ├── huffman/                # Huffman encoder & canonical decoder
│   ├── lzw/                    # 16-bit fixed-width LZW codec
│   ├── io/                     # BitWriter, BitReader, BinaryIO, Checksum (CRC32 & SHA-256)
│   ├── engine/                 # CompressionEngine, block processing & adaptive selection
│   ├── archive/                # ArchiveWriter, ArchiveReader, PathSecurity
│   └── gui/                    # Qt Widgets desktop interface (MainWindow & main_gui)
├── docs/                       # Formal design docs & archive specifications
└── project_memory.md           # Persistent development and architecture log
```

---

## How to Run

### Quick Start (One-Click)
Double-click `run_gui.bat` in the project root folder.

### Direct Launch
```powershell
.\build\forge.exe
```

---

## Build from Source (Optional)

### Prerequisites
- C++17 compliant compiler (`MSVC` or `MinGW g++`)
- Qt 6.x Widgets
- CMake 3.20+

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
