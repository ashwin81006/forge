#ifndef FORGE_BLOCK_TYPES_H
#define FORGE_BLOCK_TYPES_H

#include <cstdint>
#include <vector>
#include <string>

namespace forge {
namespace engine {

enum class Method : uint8_t {
    RAW = 0,
    HUFFMAN = 1,
    LZW = 2
};

inline const char* methodName(Method m) {
    switch (m) {
        case Method::RAW: return "RAW";
        case Method::HUFFMAN: return "HUFFMAN";
        case Method::LZW: return "LZW";
        default: return "UNKNOWN";
    }
}

struct BlockRecord {
    Method method{Method::RAW};
    uint32_t originalBlockSize{0};
    uint32_t storedPayloadSize{0};
    uint32_t metadataSize{0};
    uint32_t blockChecksum{0}; // CRC32 of uncompressed block
    std::vector<uint8_t> metadata;
    std::vector<uint8_t> payload;

    uint32_t totalStoredSize() const {
        return storedPayloadSize + metadataSize;
    }
};

} // namespace engine
} // namespace forge

#endif // FORGE_BLOCK_TYPES_H
