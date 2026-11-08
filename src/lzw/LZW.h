#ifndef LZW_H
#define LZW_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace forge {
namespace lzw {

class LZW {
public:
    static constexpr uint16_t INITIAL_DICT_SIZE = 256;
    static constexpr uint32_t MAX_DICT_SIZE = 65536;

    struct EncodedData {
        std::vector<uint8_t> data;
        uint64_t originalSize;
        uint64_t codeCount;
    };

    EncodedData compress(const std::vector<uint8_t>& input);
    std::vector<uint8_t> decompress(const EncodedData& encoded);
};

}
}

#endif
