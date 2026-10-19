# FORGE Project Memory

## Current Status
- **Phase**: M9_FINAL (All Milestones Completed & Fully Verified)
- **Active Target**: Academic Demonstration & Submission
- **Completed Milestones**: M1, M2, M3, M4, M5, M6, M7, M8, M9

---

## Build & Test Status
- **Compiler**: GCC 15.2.0 (MSYS2 UCRT64), C++17 Standard
- **Build Status**: Clean compilation (`0 warnings, 0 errors`)
- **Unit & Integration Tests**: **62 / 62 Tests Passed (100%)**
  - `test_huffman`: 9 / 9 passed
  - `test_lzw`: 10 / 10 passed
  - `test_engine`: 12 / 12 passed
  - `test_archive`: 13 / 13 passed
  - `test_benchmark`: 13 / 13 passed
  - `test_roundtrip`: 5 / 5 passed
  - `forge` CLI runner: 25 / 25 passed

---

## Core Architecture & Algorithmic Decisions
1. **Bounded Block Streaming**: Files processed in 64 KiB independent blocks (configurable 16, 64, 256, 1024 KiB) maintaining working memory $< 4$ MiB.
2. **Adaptive Candidate Selection**: Evaluates RAW, Huffman, and LZW per block, selecting $\text{argmin}(S_m)$ with zero size penalty on incompressible data.
3. **Codec State Resets**: Fresh Huffman tree and LZW dictionary initialized per block to ensure complete independence.
4. **Binary Archive Format (`.forge`)**: Little-endian binary serialization with magic `"FORGE"`, format version 1, file records, and block records.
5. **Two-Tier Integrity Verification**: Block-level CRC-32 for fast payload validation and file-level cryptographic SHA-256 for exact lossless recovery.
6. **Path Traversal Security**: Strict validation rejecting `..`, drive letters, and leading slashes during archive extraction.
7. **Qt Decoupling**: Engine is 100% independent from Qt; Qt Widgets GUI (`MainWindow`) operates strictly through high-level engine APIs.

---

## Documentation Deliverables
- [README.md](file:///c:/01_Rithanya/06_College/SEM_5/DAA_Lab/Package/forge/README.md)
- [docs/PROJECT_DOCUMENTATION.md](file:///c:/01_Rithanya/06_College/SEM_5/DAA_Lab/Package/forge/docs/PROJECT_DOCUMENTATION.md)
- [docs/ALGORITHM_DESIGN.md](file:///c:/01_Rithanya/06_College/SEM_5/DAA_Lab/Package/forge/docs/ALGORITHM_DESIGN.md)
- [docs/ARCHIVE_FORMAT.md](file:///c:/01_Rithanya/06_College/SEM_5/DAA_Lab/Package/forge/docs/ARCHIVE_FORMAT.md)
- [docs/BENCHMARK_REPORT.md](file:///c:/01_Rithanya/06_College/SEM_5/DAA_Lab/Package/forge/docs/BENCHMARK_REPORT.md)
