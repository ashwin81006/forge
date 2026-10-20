#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <numeric>
#include "../src/huffman/Huffman.h"

static int gTests = 0, gPassed = 0;

static void check(const std::string& name, bool cond) {
    ++gTests;
    if (cond) {
        ++gPassed;
        std::cout << "  [PASS] " << name << "\n";
    } else {
        std::cout << "  [FAIL] " << name << "\n";
    }
}

static void testRoundTrip(const std::string& name, const std::vector<uint8_t>& in) {
    Huffman huff;
    auto enc = huff.compress(in);
    auto dec = huff.decompress(enc);
    check(name, dec == in);
}

int main() {
    std::cout << "=== Huffman Comprehensive Unit Tests ===\n";

    testRoundTrip("Empty input", {});
    testRoundTrip("Single byte", {0x42});
    testRoundTrip("Two identical bytes", {0xAA, 0xAA});
    testRoundTrip("Two different bytes", {0x00, 0xFF});
    testRoundTrip("1000 identical bytes", std::vector<uint8_t>(1000, 0x77));

    std::vector<uint8_t> allBytes(256);
    std::iota(allBytes.begin(), allBytes.end(), 0);
    testRoundTrip("All 256 byte values", allBytes);

    std::mt19937 rng(1234);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> rand64k(65536);
    for (auto& b : rand64k) b = static_cast<uint8_t>(dist(rng));
    testRoundTrip("Random 64 KiB", rand64k);

    // Corrupted input rejection
    Huffman huff;
    bool caught1 = false, caught2 = false;
    try {
        Huffman::EncodedData badFreq{{0x01}, std::vector<uint64_t>(100, 0), 1, 8};
        huff.decompress(badFreq);
    } catch (const std::exception&) { caught1 = true; }
    check("Rejects invalid frequency table size", caught1);

    try {
        auto enc = huff.compress({1, 2, 3});
        enc.data.pop_back();
        huff.decompress(enc);
    } catch (const std::exception&) { caught2 = true; }
    check("Rejects truncated bitstream", caught2);

    std::cout << "Huffman Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
