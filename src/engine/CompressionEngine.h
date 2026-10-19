#ifndef FORGE_COMPRESSION_ENGINE_H
#define FORGE_COMPRESSION_ENGINE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <istream>
#include <ostream>
#include <functional>
#include <string>
#include "BlockTypes.h"

namespace forge {
namespace engine {

class CompressionEngine {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 64 * 1024; // 64 KiB

    struct Config {
        size_t blockSize{DEFAULT_BLOCK_SIZE};
        bool adaptive{true};
        Method fallbackMethod{Method::RAW};
        std::function<void(size_t blocksDone, uint64_t bytesProcessed)> progressCallback{nullptr};
    };

    struct BlockStats {
        Method selectedMethod;
        uint32_t originalSize;
        uint32_t storedSize;
        double huffmanMs{0.0};
        double lzwMs{0.0};
        uint32_t huffmanStoredSize{0};
        uint32_t lzwStoredSize{0};
    };

    CompressionEngine() = default;

    // Single-block operations
    BlockRecord compressBlock(
        const uint8_t* data,
        size_t size,
        bool adaptive = true,
        Method forceMethod = Method::RAW,
        BlockStats* statsOut = nullptr);

    BlockRecord compressBlock(
        const std::vector<uint8_t>& data,
        bool adaptive = true,
        Method forceMethod = Method::RAW,
        BlockStats* statsOut = nullptr);

    std::vector<uint8_t> decompressBlock(const BlockRecord& block);

    // Stream operations (bounded memory)
    std::vector<BlockRecord> compressStream(
        std::istream& is,
        const Config& config);

    std::vector<BlockRecord> compressStream(
        std::istream& is);

    void decompressStream(
        const std::vector<BlockRecord>& blocks,
        std::ostream& os,
        const std::function<void(size_t blocksDone)>& progress = nullptr);

private:
    BlockRecord buildRawBlock(const uint8_t* data, size_t size);
    BlockRecord buildHuffmanBlock(const uint8_t* data, size_t size);
    BlockRecord buildLzwBlock(const uint8_t* data, size_t size);
};

} // namespace engine
} // namespace forge

#endif // FORGE_COMPRESSION_ENGINE_H
