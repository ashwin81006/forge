// ═══════════════════════════════════════════════════════════════════════
// FORGE — Full Optimization & Recursive Generation Engine
// Main CLI Driver & Test Suite Runner
// ═══════════════════════════════════════════════════════════════════════

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include "src/huffman/Huffman.h"
#include "src/lzw/LZW.h"
#include "src/engine/CompressionEngine.h"
#include "src/archive/ArchiveWriter.h"
#include "src/archive/ArchiveReader.h"
#include "src/archive/PathSecurity.h"
#include "src/benchmark/Benchmark.h"

static int gTestsPassed = 0;
static int gTestsFailed = 0;

static void reportResult(const std::string& name, bool passed) {
    if (passed) {
        std::cout << "  [PASS] " << name << "\n";
        ++gTestsPassed;
    } else {
        std::cout << "  [FAIL] " << name << "\n";
        ++gTestsFailed;
    }
}

// ── Huffman Tests ───────────────────────────────────────────────────

static void testHuffmanRoundTrip(const std::string& name, const std::vector<uint8_t>& input) {
    Huffman huff;
    try {
        auto encoded = huff.compress(input);
        auto decoded = huff.decompress(encoded);
        reportResult(name, decoded == input);
    } catch (const std::exception& e) {
        std::cout << "  [FAIL] " << name << " — exception: " << e.what() << "\n";
        ++gTestsFailed;
    }
}

static void runHuffmanTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      HUFFMAN  CODEC  TESTS           ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    testHuffmanRoundTrip("Empty input", {});
    testHuffmanRoundTrip("Single byte", {0x42});
    testHuffmanRoundTrip("Two identical bytes", {0xAA, 0xAA});
    testHuffmanRoundTrip("Two different bytes", {0x00, 0xFF});
    testHuffmanRoundTrip("256 x 0x00", std::vector<uint8_t>(256, 0x00));
    testHuffmanRoundTrip("1000 x 0xFF", std::vector<uint8_t>(1000, 0xFF));

    std::vector<uint8_t> allBytes(256);
    for (int i = 0; i < 256; ++i) allBytes[i] = static_cast<uint8_t>(i);
    testHuffmanRoundTrip("All 256 byte values", allBytes);

    // Malformed stream rejection
    Huffman huff;
    try {
        Huffman::EncodedData badFreq{{0x00}, std::vector<uint64_t>(255, 0), 1, 8};
        huff.decompress(badFreq);
        reportResult("Huffman reject invalid freq table size", false);
    } catch (...) { reportResult("Huffman reject invalid freq table size", true); }

    try {
        Huffman::EncodedData badSum{{0x00}, std::vector<uint64_t>(256, 0), 10, 8};
        badSum.frequencies[0] = 5;
        huff.decompress(badSum);
        reportResult("Huffman reject frequency sum mismatch", false);
    } catch (...) { reportResult("Huffman reject frequency sum mismatch", true); }
}

// ── LZW Tests ───────────────────────────────────────────────────────

static void testLZWRoundTrip(const std::string& name, const std::vector<uint8_t>& input) {
    LZW lzw;
    try {
        auto encoded = lzw.compress(input);
        auto decoded = lzw.decompress(encoded);
        reportResult(name, decoded == input);
    } catch (const std::exception& e) {
        std::cout << "  [FAIL] " << name << " — exception: " << e.what() << "\n";
        ++gTestsFailed;
    }
}

static void runLZWTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      LZW  CODEC  TESTS               ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    testLZWRoundTrip("Empty input", {});
    testLZWRoundTrip("Single byte", {0x42});
    testLZWRoundTrip("Two identical bytes", {0xAA, 0xAA});
    testLZWRoundTrip("Two different bytes", {0x00, 0xFF});
    testLZWRoundTrip("256 x 0x00", std::vector<uint8_t>(256, 0x00));
    testLZWRoundTrip("1000 x 0xFF", std::vector<uint8_t>(1000, 0xFF));

    std::vector<uint8_t> allBytes(256);
    for (int i = 0; i < 256; ++i) allBytes[i] = static_cast<uint8_t>(i);
    testLZWRoundTrip("All 256 byte values", allBytes);

    // Malformed stream rejection
    LZW lzw;
    try {
        LZW::EncodedData badCount{{0x00, 0x00, 0x01}, 2, 2};
        lzw.decompress(badCount);
        reportResult("LZW reject data size mismatch", false);
    } catch (...) { reportResult("LZW reject data size mismatch", true); }

    try {
        LZW::EncodedData badFirst{{0x00, 0x01}, 1, 1};
        lzw.decompress(badFirst);
        reportResult("LZW reject first code >= 256", false);
    } catch (...) { reportResult("LZW reject first code >= 256", true); }
}

// ── Engine Tests (M3 + M4) ──────────────────────────────────────────

static void runEngineTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      ENGINE (M3+M4) TESTS            ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    forge::engine::CompressionEngine engine;

    // RAW fallback on random data
    auto randBytes = forge::benchmark::BenchmarkSuite::generateRandomBytes(1024);
    auto blk = engine.compressBlock(randBytes);
    reportResult("Adaptive selector chooses RAW on random data", blk.method == forge::engine::Method::RAW);

    // LZW selection on repetitive data
    auto repBytes = forge::benchmark::BenchmarkSuite::generateRepetitiveText(10 * 1024);
    auto blkRep = engine.compressBlock(repBytes);
    reportResult("Adaptive selector chooses LZW on repetitive text", blkRep.method == forge::engine::Method::LZW);

    // Block reset check
    auto decRep = engine.decompressBlock(blkRep);
    reportResult("Engine block decompression lossless", decRep == repBytes);
}

// ── Archive Tests (M5 + M6) ─────────────────────────────────────────

static void runArchiveTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      ARCHIVE (M5+M6) TESTS           ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    std::filesystem::path testDir = "temp_forge_test_archive";
    std::filesystem::path srcDir = testDir / "src";
    std::filesystem::path archiveFile = testDir / "sample.forge";
    std::filesystem::path extDir = testDir / "out";

    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(srcDir / "sub");

    // Write sample files
    {
        std::ofstream(srcDir / "hello.txt") << "Hello, FORGE System!";
        std::ofstream(srcDir / "sub" / "data.bin", std::ios::binary) << std::string(10000, 'Z');
    }

    try {
        forge::archive::ArchiveWriter writer;
        writer.compressFolder(srcDir.string(), archiveFile.string(), 64 * 1024, true);
        reportResult("Archive creation succeeds", std::filesystem::exists(archiveFile));

        forge::archive::ArchiveReader reader;
        auto list = reader.listArchive(archiveFile.string());
        reportResult("Archive list contains 2 files", list.size() == 2);

        reader.extractArchive(archiveFile.string(), extDir.string());
        reportResult("Extracted nested file exists and matches size",
                     std::filesystem::file_size(extDir / "sub" / "data.bin") == 10000);

        reportResult("Path traversal rejection verified",
                     !forge::archive::PathSecurity::isSafeRelativePath("../evil.sh"));
    } catch (const std::exception& e) {
        std::cout << "Archive test failure: " << e.what() << "\n";
        reportResult("Archive operations without error", false);
    }

    std::filesystem::remove_all(testDir);
}

// ── Benchmark Suite ─────────────────────────────────────────────────

static void runBenchmarks() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      BENCHMARK LABORATORY            ║\n"
              <<   "╚══════════════════════════════════════╝\n\n";

    auto results = forge::benchmark::BenchmarkSuite::runStandardSuite();
    std::string md = forge::benchmark::BenchmarkSuite::exportToMarkdown(results);
    std::cout << md << "\n";
}

static void printUsage() {
    std::cout << "FORGE CLI Usage:\n"
              << "  forge                      Run full verification test suite & benchmarks\n"
              << "  forge --test               Run all unit & integration tests\n"
              << "  forge --bench              Run benchmark suite and output markdown table\n"
              << "  forge -c <dir> -o <file>   Compress folder into .forge archive\n"
              << "  forge -x <file> -o <dir>   Extract .forge archive to destination directory\n"
              << "  forge -l <file>            List contents of .forge archive\n";
}

int main(int argc, char* argv[]) {
    if (argc >= 2) {
        std::string cmd = argv[1];
        if (cmd == "--help" || cmd == "-h") {
            printUsage();
            return 0;
        } else if (cmd == "--bench") {
            runBenchmarks();
            return 0;
        } else if (cmd == "-c" && argc >= 4) {
            std::string src = argv[2];
            std::string dst = (argc >= 5 && std::string(argv[3]) == "-o") ? argv[4] : (src + ".forge");
            std::cout << "Compressing folder: " << src << " -> " << dst << "\n";
            forge::archive::ArchiveWriter writer;
            writer.compressFolder(src, dst);
            std::cout << "Done.\n";
            return 0;
        } else if (cmd == "-x" && argc >= 4) {
            std::string arch = argv[2];
            std::string dst = (argc >= 5 && std::string(argv[3]) == "-o") ? argv[4] : ".";
            std::cout << "Extracting archive: " << arch << " -> " << dst << "\n";
            forge::archive::ArchiveReader reader;
            reader.extractArchive(arch, dst);
            std::cout << "Extraction complete and SHA-256 verified.\n";
            return 0;
        } else if (cmd == "-l" && argc >= 3) {
            std::string arch = argv[2];
            forge::archive::ArchiveReader reader;
            auto list = reader.listArchive(arch);
            std::cout << "\nArchive listing: " << arch << " (" << list.size() << " files)\n";
            std::cout << std::left << std::setw(40) << "Path"
                      << std::right << std::setw(12) << "Original"
                      << std::setw(12) << "Stored"
                      << std::setw(10) << "Blocks\n";
            std::cout << std::string(74, '-') << "\n";
            for (const auto& item : list) {
                std::cout << std::left << std::setw(40) << item.relativePath
                          << std::right << std::setw(12) << item.originalFileSize
                          << std::setw(12) << item.storedSize
                          << std::setw(10) << item.blockCount << "\n";
            }
            return 0;
        }
    }

    std::cout << "\n  ╔═══════════════════════════════════════════════════╗\n"
              <<   "  ║  FORGE — Full Optimization & Recursive            ║\n"
              <<   "  ║           Generation Engine                       ║\n"
              <<   "  ║  Complete Milestone Verification Test Suite       ║\n"
              <<   "  ╚═══════════════════════════════════════════════════╝\n";

    runHuffmanTests();
    runLZWTests();
    runEngineTests();
    runArchiveTests();
    runBenchmarks();

    std::cout << "\n══════════════════════════════════════\n"
              << "  Results: " << gTestsPassed << " passed, " << gTestsFailed << " failed\n"
              << "══════════════════════════════════════\n\n";

    return gTestsFailed > 0 ? 1 : 0;
}
