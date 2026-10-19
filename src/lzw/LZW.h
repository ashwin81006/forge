
#ifndef LZW_H
#define LZW_H

#include <cstddef>
#include <cstdint>
#include <vector>

class LZW {
public:
    // Fixed-width 16-bit codes, dictionary capped at 65536 entries.
    static constexpr uint16_t INITIAL_DICT_SIZE = 256;
    static constexpr uint32_t MAX_DICT_SIZE = 65536;

    struct EncodedData {
        std::vector<uint8_t> data;      // Packed 16-bit codes as bytes (little-endian)
        uint64_t originalSize;          // Original uncompressed byte count
        uint64_t codeCount;             // Number of 16-bit codes emitted
    };

    EncodedData compress(const std::vector<uint8_t>& input);
    std::vector<uint8_t> decompress(const EncodedData& encoded);
};

#endif
