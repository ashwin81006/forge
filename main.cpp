
// ═══════════════════════════════════════════════════════════════════════
// FORGE — File Optimization & Recursive Generation Engine
// Test harness for M1 (Huffman) and M2 (LZW)
// ═══════════════════════════════════════════════════════════════════════

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "src/huffman/Huffman.h"
#include "src/lzw/LZW.h"

// ─── Helpers ────────────────────────────────────────────────────────────

static int  gTestsPassed = 0;
static int  gTestsFailed = 0;

static void reportResult(
    const std::string& name, bool passed) {

    if (passed) {
        std::cout << "  [PASS] " << name << "\n";
        ++gTestsPassed;
    }
    else {
        std::cout << "  [FAIL] " << name << "\n";
        ++gTestsFailed;
    }
}

static std::vector<uint8_t> makeBytes(const std::string& s) {
    return { s.begin(), s.end() };
}

static std::vector<uint8_t> makeRepeated(uint8_t val, size_t n) {
    return std::vector<uint8_t>(n, val);
}

static std::vector<uint8_t> makeRandom(size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> out(n);
    for (auto& b : out) b = static_cast<uint8_t>(dist(rng));
    return out;
}

static std::vector<uint8_t> makeAllBytes() {
    std::vector<uint8_t> out(256);
    std::iota(out.begin(), out.end(), 0);
    return out;
}

// ═══════════════════════════════════════════════════════════════════════
//  HUFFMAN TESTS
// ═══════════════════════════════════════════════════════════════════════

static void testHuffmanRoundTrip(
    const std::string& name,
    const std::vector<uint8_t>& input) {

    Huffman huff;

    try {
        auto encoded = huff.compress(input);
        auto decoded = huff.decompress(encoded);
        reportResult(name, decoded == input);
    }
    catch (const std::exception& e) {
        std::cout << "  [FAIL] " << name
                  << " — exception: " << e.what() << "\n";
        ++gTestsFailed;
    }
}

static void testHuffmanExpectException(
    const std::string& name,
    const Huffman::EncodedData& invalidData) {

    Huffman huff;
    try {
        huff.decompress(invalidData);
        reportResult(name, false);
    }
    catch (const std::exception&) {
        reportResult(name, true);
    }
}

static void runHuffmanTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      HUFFMAN  CODEC  TESTS           ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    // Edge cases
    testHuffmanRoundTrip("Empty input",           {});
    testHuffmanRoundTrip("Single byte",           { 0x42 });
    testHuffmanRoundTrip("Two identical bytes",   { 0xAA, 0xAA });
    testHuffmanRoundTrip("Two different bytes",   { 0x00, 0xFF });

    // Single-symbol (all same byte)
    testHuffmanRoundTrip("256 × 0x00",            makeRepeated(0x00, 256));
    testHuffmanRoundTrip("1000 × 0xFF",           makeRepeated(0xFF, 1000));

    // All 256 byte values
    testHuffmanRoundTrip("All 256 byte values",   makeAllBytes());

    // Text
    testHuffmanRoundTrip("Short text",
        makeBytes("Hello, FORGE!"));

    testHuffmanRoundTrip("Repetitive text",
        makeBytes("aaabbbcccaaabbbcccaaabbbcccaaabbbccc"));

    testHuffmanRoundTrip("Lorem paragraph",
        makeBytes(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Sed do eiusmod tempor incididunt ut labore et dolore magna "
            "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
            "ullamco laboris nisi ut aliquip ex ea commodo consequat."));

    // Binary patterns
    testHuffmanRoundTrip("Random 1 KiB",          makeRandom(1024));
    testHuffmanRoundTrip("Random 64 KiB",         makeRandom(65536));

    // Skewed distribution (good for Huffman)
    {
        std::vector<uint8_t> skewed;
        skewed.reserve(10000);
        for (int i = 0; i < 9000; ++i) skewed.push_back('a');
        for (int i = 0; i < 900;  ++i) skewed.push_back('b');
        for (int i = 0; i < 100;  ++i) skewed.push_back('c');
        testHuffmanRoundTrip("Skewed distribution (90/9/1)", skewed);
    }

    // Malformed / corrupted streams
    {
        Huffman::EncodedData badFreq{{0x00}, std::vector<uint64_t>(255, 0), 1, 8};
        testHuffmanExpectException("Huffman reject invalid freq table size", badFreq);

        Huffman::EncodedData badSum{{0x00}, std::vector<uint64_t>(256, 0), 10, 8};
        badSum.frequencies[0] = 5;
        testHuffmanExpectException("Huffman reject frequency sum mismatch", badSum);

        Huffman huff;
        auto enc = huff.compress(makeBytes("Hello"));
        enc.data.pop_back(); // truncate payload
        testHuffmanExpectException("Huffman reject truncated bitstream", enc);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  LZW TESTS
// ═══════════════════════════════════════════════════════════════════════

static void testLZWRoundTrip(
    const std::string& name,
    const std::vector<uint8_t>& input) {

    LZW lzw;

    try {
        auto encoded = lzw.compress(input);
        auto decoded = lzw.decompress(encoded);
        reportResult(name, decoded == input);
    }
    catch (const std::exception& e) {
        std::cout << "  [FAIL] " << name
                  << " — exception: " << e.what() << "\n";
        ++gTestsFailed;
    }
}

static void testLZWExpectException(
    const std::string& name,
    const LZW::EncodedData& invalidData) {

    LZW lzw;
    try {
        lzw.decompress(invalidData);
        reportResult(name, false);
    }
    catch (const std::exception&) {
        reportResult(name, true);
    }
}

static void runLZWTests() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      LZW  CODEC  TESTS               ║\n"
              <<   "╚══════════════════════════════════════╝\n";

    // Edge cases
    testLZWRoundTrip("Empty input",             {});
    testLZWRoundTrip("Single byte",             { 0x42 });
    testLZWRoundTrip("Two identical bytes",     { 0xAA, 0xAA });
    testLZWRoundTrip("Two different bytes",     { 0x00, 0xFF });

    // Single-symbol (all same byte) — triggers the code==nextCode case
    testLZWRoundTrip("256 × 0x00",              makeRepeated(0x00, 256));
    testLZWRoundTrip("1000 × 0xFF",             makeRepeated(0xFF, 1000));

    // All 256 byte values
    testLZWRoundTrip("All 256 byte values",     makeAllBytes());

    // Text
    testLZWRoundTrip("Short text",
        makeBytes("Hello, FORGE!"));

    testLZWRoundTrip("Repetitive text",
        makeBytes("aaabbbcccaaabbbcccaaabbbcccaaabbbccc"));

    testLZWRoundTrip("Lorem paragraph",
        makeBytes(
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
            "Sed do eiusmod tempor incididunt ut labore et dolore magna "
            "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
            "ullamco laboris nisi ut aliquip ex ea commodo consequat."));

    // Binary patterns
    testLZWRoundTrip("Random 1 KiB",            makeRandom(1024));
    testLZWRoundTrip("Random 64 KiB",           makeRandom(65536));

    // Highly repetitive (LZW's strength)
    {
        std::string pattern = "ABCABCABCABC";
        std::vector<uint8_t> data;
        data.reserve(pattern.size() * 500);
        for (int i = 0; i < 500; ++i)
            data.insert(data.end(), pattern.begin(), pattern.end());
        testLZWRoundTrip("Highly repetitive (ABC×2000)", data);
    }

    // Stress: push dictionary to max capacity
    {
        // Alternating patterns that generate many unique sequences
        std::vector<uint8_t> stress;
        stress.reserve(200000);
        for (int i = 0; i < 200000; ++i)
            stress.push_back(static_cast<uint8_t>(i % 251)); // prime mod
        testLZWRoundTrip("Dictionary stress (200 KB, mod-251)", stress);
    }

    // True dictionary saturation (>65536 entries, continue encoding)
    {
        std::mt19937 rng(1337);
        std::uniform_int_distribution<int> dist(0, 255);
        std::vector<uint8_t> data(300000);
        for (auto& b : data) b = static_cast<uint8_t>(dist(rng));
        testLZWRoundTrip("Full dictionary saturation (>65536 entries)", data);
    }

    // Malformed / corrupted streams
    {
        LZW::EncodedData badCount{{0x00, 0x00, 0x01}, 2, 2}; // 3 bytes for 2 codes
        testLZWExpectException("LZW reject data size mismatch", badCount);

        LZW::EncodedData badFirst{{0x00, 0x01}, 1, 1}; // first code 256
        testLZWExpectException("LZW reject first code >= 256", badFirst);

        LZW::EncodedData badCode{{0x41, 0x00, 0x00, 0x02}, 2, 2}; // code 512
        testLZWExpectException("LZW reject out-of-range code", badCode);
    }
}

// ═══════════════════════════════════════════════════════════════════════
//  MINI BENCHMARK  (compression ratio + speed)
// ═══════════════════════════════════════════════════════════════════════

struct BenchResult {
    std::string name;
    size_t      originalSize;
    size_t      huffmanSize;
    size_t      lzwSize;
    double      huffmanCompressMs;
    double      lzwCompressMs;
};

static BenchResult benchmark(
    const std::string& name,
    const std::vector<uint8_t>& data) {

    BenchResult r{};
    r.name = name;
    r.originalSize = data.size();

    Huffman huff;
    LZW    lzw;

    // Huffman
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        auto enc = huff.compress(data);
        auto t1 = std::chrono::high_resolution_clock::now();

        // Total stored size = payload + 256 × 8 bytes (frequency table)
        r.huffmanSize = enc.data.size() + 256 * sizeof(uint64_t);
        r.huffmanCompressMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    // LZW
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        auto enc = lzw.compress(data);
        auto t1 = std::chrono::high_resolution_clock::now();

        // Total stored size = 2 bytes per code (fixed-width 16-bit)
        r.lzwSize = enc.data.size();
        r.lzwCompressMs =
            std::chrono::duration<double, std::milli>(t1 - t0).count();
    }

    return r;
}

static void runBenchmarks() {
    std::cout << "\n╔══════════════════════════════════════╗\n"
              <<   "║      MINI  BENCHMARK                 ║\n"
              <<   "╚══════════════════════════════════════╝\n\n";

    std::vector<BenchResult> results;

    // 1. Repetitive text
    {
        std::string s;
        for (int i = 0; i < 1000; ++i) s += "the quick brown fox ";
        results.push_back(benchmark("Repetitive text (20 KB)",
            makeBytes(s)));
    }

    // 2. C++ source-like text
    {
        std::string s;
        for (int i = 0; i < 200; ++i) {
            s += "int main() {\n";
            s += "    std::cout << \"Hello\" << std::endl;\n";
            s += "    return 0;\n";
            s += "}\n\n";
        }
        results.push_back(benchmark("C++ source-like (13 KB)",
            makeBytes(s)));
    }

    // 3. All-identical bytes
    results.push_back(benchmark("All zeroes (64 KB)",
        makeRepeated(0x00, 65536)));

    // 4. Random data
    results.push_back(benchmark("Random bytes (64 KB)",
        makeRandom(65536)));

    // 5. All 256 values cycling
    {
        auto cycle = makeAllBytes();
        std::vector<uint8_t> data;
        data.reserve(256 * 256);
        for (int i = 0; i < 256; ++i)
            data.insert(data.end(), cycle.begin(), cycle.end());
        results.push_back(benchmark("Cycling 0-255 (64 KB)",
            data));
    }

    // ── Print table ─────────────────────────────────────────────────

    std::cout << std::left
              << std::setw(30) << "Dataset"
              << std::right
              << std::setw(10) << "Original"
              << std::setw(12) << "Huffman"
              << std::setw(10) << "Ratio"
              << std::setw(10) << "ms"
              << std::setw(12) << "LZW"
              << std::setw(10) << "Ratio"
              << std::setw(10) << "ms"
              << std::setw(10) << "Winner"
              << "\n";

    std::cout << std::string(114, '-') << "\n";

    for (const auto& r : results) {
        double huffRatio = (r.originalSize > 0)
            ? static_cast<double>(r.originalSize) / r.huffmanSize : 0;
        double lzwRatio  = (r.originalSize > 0)
            ? static_cast<double>(r.originalSize) / r.lzwSize : 0;

        std::string winner =
            (r.huffmanSize <= r.lzwSize) ? "Huffman" : "LZW";

        std::cout << std::left
                  << std::setw(30) << r.name
                  << std::right
                  << std::setw(10) << r.originalSize
                  << std::setw(12) << r.huffmanSize
                  << std::setw(10) << std::fixed
                  << std::setprecision(2) << huffRatio << "x"
                  << std::setw(9)  << std::setprecision(2)
                  << r.huffmanCompressMs
                  << std::setw(12) << r.lzwSize
                  << std::setw(10) << std::setprecision(2)
                  << lzwRatio << "x"
                  << std::setw(9)  << std::setprecision(2)
                  << r.lzwCompressMs
                  << std::setw(10) << winner
                  << "\n";
    }

    std::cout << "\n  Note: Huffman stored size includes the 2048-byte "
              << "frequency table overhead.\n"
              << "  Note: LZW stored size = 2 bytes per code "
              << "(fixed 16-bit codes).\n";
}

// ═══════════════════════════════════════════════════════════════════════
//  MAIN
// ═══════════════════════════════════════════════════════════════════════

int main() {
    std::cout
        << "\n"
        << "  ╔═══════════════════════════════════════════════════╗\n"
        << "  ║  FORGE — File Optimization & Recursive           ║\n"
        << "  ║           Generation Engine                      ║\n"
        << "  ║  M1 + M2 Test Suite                              ║\n"
        << "  ╚═══════════════════════════════════════════════════╝\n";

    runHuffmanTests();
    runLZWTests();
    runBenchmarks();

    std::cout << "\n══════════════════════════════════════\n"
              << "  Results: "
              << gTestsPassed << " passed, "
              << gTestsFailed << " failed\n"
              << "══════════════════════════════════════\n\n";

    return gTestsFailed > 0 ? 1 : 0;
}
