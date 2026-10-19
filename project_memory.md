# FORGE Project Memory

## Current Status
- **Phase**: M2_LZW Complete / Post-Review Fixes Applied
- **Active Target**: M3 (Block Processing) + M4 (Adaptive Selection)
- **Completed Milestones**: M1 (Huffman), M2 (LZW)

---

## Build & Test Status
- **Compiler**: GCC 15.2.0 (MSYS2 UCRT64), C++17
- **Build Status**: Clean compilation (`0 warnings, 0 errors`)
- **Tests**: 34 / 34 Passed (16 Huffman, 18 LZW)

---

## Architectural & Algorithmic Decisions
1. **Block Processing**: Default 64 KiB independent blocks with state reset between blocks.
2. **Huffman Determinism**: Min-priority queue ordered by `(frequency, minSymbol, order)` to guarantee canonical trees across environments.
3. **LZW Codebook**: Fixed-width 16-bit codes, dictionary capacity capped at 65,536 entries. Freezes addition at max capacity and continues encoding with existing dictionary.
4. **Endianness**: Little-endian byte order used consistently for archive headers, fields, and LZW 16-bit packed code stream.
5. **Adaptive Selection Strategy**: Will compare RAW, Huffman, and LZW per block, selecting minimum stored size (including metadata).

---

## Issue Status
- **ISSUE-01 (LZW Endianness)**: **RESOLVED** — Switched code packing and unpacking to little-endian (`code & 0xFF`, `(code >> 8) & 0xFF`).
- **ISSUE-02 (LZW Memory Safeguards)**: **RESOLVED** — Added `vector::max_size()` validation on `originalSize` and mid-loop expansion bounds check.
- **ISSUE-03 (BitStream Modules)**: **OPEN** — `src/io/BitWriter.h` and `src/io/BitReader.h` to be integrated with M3/M5.
- **Edge Case Tests**: **RESOLVED** — Added dictionary saturation test (>65,536 codes) and corrupted stream rejection tests for both Huffman and LZW.

---

## Changelog
- **Phase 0 Review**: Initial review of M1 and M2 code. Initialized project memory.
- **Phase 0 Fixes**: Converted LZW to little-endian, added decompression memory bounds checks, and added saturation and malformed input test suites (34 tests passing).
