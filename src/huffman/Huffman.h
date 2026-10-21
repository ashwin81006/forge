
#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace forge {
namespace huffman {

class Huffman {
public:
    struct EncodedData {
        std::vector<uint8_t> data;
        std::vector<uint64_t> frequencies;
        uint64_t originalSize;
        uint64_t bitCount;
    };

    EncodedData compress(const std::vector<uint8_t>& input);
    std::vector<uint8_t> decompress(const EncodedData& encoded);
};

} // namespace huffman
} // namespace forge

#endif
