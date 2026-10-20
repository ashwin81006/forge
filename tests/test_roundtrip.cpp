#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cassert>
#include <sstream>

#include "../src/engine/CompressionEngine.h"
#include "../src/io/Checksum.h"

using namespace forge::engine;
using namespace forge::io;

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

int main() {
    std::cout << "=== FORGE End-to-End Round-Trip & Integrity Tests ===\n";
    CompressionEngine engine;

    // 1. Large 1 MB stream roundtrip with SHA-256 comparison
    {
        std::mt19937 rng(999);
        std::uniform_int_distribution<int> dist(0, 255);
        std::vector<uint8_t> oneMeg(1024 * 1024);
        for (auto& b : oneMeg) b = static_cast<uint8_t>(dist(rng));

        auto origSha = SHA256::hash(oneMeg);

        std::stringstream ssIn(std::string(oneMeg.begin(), oneMeg.end()));
        auto blocks = engine.compressStream(ssIn);

        std::stringstream ssOut;
        engine.decompressStream(blocks, ssOut);
        std::string decompStr = ssOut.str();
        std::vector<uint8_t> decomp(decompStr.begin(), decompStr.end());

        auto decompSha = SHA256::hash(decomp);

        check("1 MB stream roundtrip exact SHA-256 match", origSha == decompSha);
    }

    // 2. Multi-block size roundtrips (16K, 64K, 256K, 1024K)
    {
        std::string base = "FORGE_ROUNDTRIP_INTEGRITY_CHECK_";
        std::vector<uint8_t> data;
        for (int i = 0; i < 5000; ++i) data.insert(data.end(), base.begin(), base.end());

        std::vector<size_t> blockSizes = {16 * 1024, 64 * 1024, 256 * 1024, 1024 * 1024};
        for (size_t bs : blockSizes) {
            CompressionEngine::Config cfg;
            cfg.blockSize = bs;
            std::stringstream ssIn(std::string(data.begin(), data.end()));
            auto blocks = engine.compressStream(ssIn, cfg);

            std::stringstream ssOut;
            engine.decompressStream(blocks, ssOut);
            std::string s = ssOut.str();
            std::vector<uint8_t> outVec(s.begin(), s.end());

            check("Block size " + std::to_string(bs / 1024) + " KiB exact match", outVec == data);
        }
    }

    std::cout << "Round-Trip Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
