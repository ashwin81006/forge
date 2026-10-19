#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <numeric>
#include "../src/lzw/LZW.h"

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
    LZW lzw;
    auto enc = lzw.compress(in);
    auto dec = lzw.decompress(enc);
    check(name, dec == in);
}

int main() {
    std::cout << "=== LZW Comprehensive Unit Tests ===\n";

    testRoundTrip("Empty input", {});
    testRoundTrip("Single byte", {0x42});
    testRoundTrip("Two identical bytes", {0xAA, 0xAA});
    testRoundTrip("Two different bytes", {0x00, 0xFF});
    testRoundTrip("1000 identical bytes (code==nextCode trigger)", std::vector<uint8_t>(1000, 0x77));

    std::vector<uint8_t> allBytes(256);
    std::iota(allBytes.begin(), allBytes.end(), 0);
    testRoundTrip("All 256 byte values", allBytes);

    // Highly repetitive
    std::string pat = "ABCDEFGH12345678";
    std::vector<uint8_t> rep;
    for (int i = 0; i < 500; ++i) rep.insert(rep.end(), pat.begin(), pat.end());
    testRoundTrip("Repetitive pattern (8 KB)", rep);

    // Saturation test (>65,536 codes)
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> bigRand(300000);
    for (auto& b : bigRand) b = static_cast<uint8_t>(dist(rng));
    testRoundTrip("Dictionary saturation beyond 65,536 entries", bigRand);

    // Corrupted input rejection
    LZW lzw;
    bool caught1 = false, caught2 = false;
    try {
        LZW::EncodedData badCount{{1, 2, 3}, 2, 2}; // 3 bytes for 2 16-bit codes
        lzw.decompress(badCount);
    } catch (const std::exception&) { caught1 = true; }
    check("Rejects size mismatch", caught1);

    try {
        LZW::EncodedData badCode{{0x00, 0x01}, 1, 1}; // first code 256 (>= 256)
        lzw.decompress(badCode);
    } catch (const std::exception&) { caught2 = true; }
    check("Rejects first code >= 256", caught2);

    std::cout << "LZW Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
