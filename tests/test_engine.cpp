#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <random>
#include <numeric>
#include <cassert>

#include "../src/engine/CompressionEngine.h"
#include "../src/io/Checksum.h"

using namespace forge::engine;
using namespace forge::io;

static int gTests = 0;
static int gPassed = 0;

static void check(const std::string& name, bool cond) {
    ++gTests;
    if (cond) {
        ++gPassed;
        std::cout << "  [PASS] " << name << "\n";
    } else {
        std::cout << "  [FAIL] " << name << "\n";
    }
}

int main() {
    std::cout << "=== M3 + M4 Engine & Adaptive Selection Tests ===\n";
    CompressionEngine engine;

    // 1. Single block roundtrip (empty)
    {
        std::vector<uint8_t> empty;
        auto blk = engine.compressBlock(empty);
        auto dec = engine.decompressBlock(blk);
        check("Empty block roundtrip", dec == empty && blk.method == Method::RAW);
    }

    // 2. Adaptive selection chooses RAW on incompressible data
    {
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> dist(0, 255);
        std::vector<uint8_t> randomBytes(1024);
        for (auto& b : randomBytes) b = static_cast<uint8_t>(dist(rng));

        CompressionEngine::BlockStats stats;
        auto blk = engine.compressBlock(randomBytes.data(), randomBytes.size(), true, Method::RAW, &stats);
        auto dec = engine.decompressBlock(blk);
        check("Adaptive selector chooses RAW on random bytes", blk.method == Method::RAW);
        check("Random block roundtrip lossless", dec == randomBytes);
    }

    // 3. Adaptive selection chooses LZW on repetitive text
    {
        std::string pattern = "the quick brown fox jumps over the lazy dog. ";
        std::vector<uint8_t> repData;
        for (int i = 0; i < 200; ++i) {
            repData.insert(repData.end(), pattern.begin(), pattern.end());
        }
        auto blk = engine.compressBlock(repData);
        auto dec = engine.decompressBlock(blk);
        check("Adaptive selector chooses LZW on repetitive data", blk.method == Method::LZW);
        check("LZW block roundtrip lossless", dec == repData);
    }

    // 4. Adaptive selection chooses Huffman on skewed symbol data with high entropy
    {
        std::mt19937 g(42);
        std::geometric_distribution<int> dist(0.15);
        std::vector<uint8_t> skewed;
        skewed.reserve(50000);
        for (int i = 0; i < 50000; ++i) {
            skewed.push_back(static_cast<uint8_t>(dist(g) % 128));
        }

        CompressionEngine::BlockStats stats;
        auto blk = engine.compressBlock(skewed.data(), skewed.size(), true, Method::RAW, &stats);
        auto dec = engine.decompressBlock(blk);
        check("Adaptive selector chooses Huffman on geometric distribution", blk.method == Method::HUFFMAN);
        check("Huffman block roundtrip lossless", dec == skewed);
    }

    // 5. Codec state reset between blocks (M3)
    {
        std::string s1 = "AAAAABBBBBCCCCCDDDDDEEEEE";
        std::string s2 = "1111122222333334444455555";
        std::vector<uint8_t> d1(s1.begin(), s1.end());
        std::vector<uint8_t> d2(s2.begin(), s2.end());

        auto b1 = engine.compressBlock(d1);
        auto b2 = engine.compressBlock(d2);

        auto dec1 = engine.decompressBlock(b1);
        auto dec2 = engine.decompressBlock(b2);

        check("Block 1 resets and decodes independently", dec1 == d1);
        check("Block 2 resets and decodes independently", dec2 == d2);
    }

    // 6. Stream multi-block processing with bounded memory (M3)
    {
        // 200 KiB stream using 64 KiB blocks -> 4 blocks
        std::string text = "FORGE Block Processing Engine Test String Repeated Multi Block. ";
        std::vector<uint8_t> bigData;
        bigData.reserve(200 * 1024);
        while (bigData.size() < 200 * 1024) {
            bigData.insert(bigData.end(), text.begin(), text.end());
        }
        bigData.resize(200 * 1024);

        std::stringstream ssIn(std::string(bigData.begin(), bigData.end()));
        CompressionEngine::Config cfg;
        cfg.blockSize = 64 * 1024;
        auto blocks = engine.compressStream(ssIn, cfg);

        check("Stream produced expected block count", blocks.size() == 4);

        std::stringstream ssOut;
        engine.decompressStream(blocks, ssOut);
        std::string restored = ssOut.str();
        std::vector<uint8_t> restoredVec(restored.begin(), restored.end());

        check("Stream decompressed exactly matches original", restoredVec == bigData);
    }

    // 7. Checksum corruption detection
    {
        std::vector<uint8_t> data(1000, 0x55);
        auto blk = engine.compressBlock(data);
        blk.payload[0] ^= 0xFF; // corrupt 1 byte

        bool caught = false;
        try {
            engine.decompressBlock(blk);
        } catch (const std::exception&) {
            caught = true;
        }
        check("Corrupted block payload triggers CRC32 mismatch exception", caught);
    }

    std::cout << "M3+M4 Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
