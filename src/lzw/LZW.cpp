#include "LZW.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace forge {
namespace lzw {

namespace {

constexpr size_t HASH_TABLE_SIZE = 79003;

inline size_t hashKey(uint16_t prefix, uint8_t character) {
    return ((static_cast<size_t>(prefix) << 5) ^ character) % HASH_TABLE_SIZE;
}

}

LZW::EncodedData LZW::compress(const std::vector<uint8_t>& input) {
    EncodedData result{};
    result.originalSize = input.size();
    result.codeCount = 0;

    if (input.empty())
        return result;

    std::vector<uint16_t> prefixTable(HASH_TABLE_SIZE, 0);
    std::vector<uint8_t>  charTable(HASH_TABLE_SIZE, 0);
    std::vector<uint16_t> codeTable(HASH_TABLE_SIZE, 0);

    uint32_t nextCode = INITIAL_DICT_SIZE;

    std::vector<uint16_t> codes;
    codes.reserve(input.size());

    uint16_t prefix = input[0];

    for (size_t i = 1; i < input.size(); ++i) {
        uint8_t c = input[i];

        size_t idx = hashKey(prefix, c);
        bool found = false;

        while (codeTable[idx] != 0) {
            if (prefixTable[idx] == prefix && charTable[idx] == c) {
                prefix = codeTable[idx];
                found = true;
                break;
            }
            idx = (idx + 1) % HASH_TABLE_SIZE;
        }

        if (!found) {
            codes.push_back(prefix);

            if (nextCode < MAX_DICT_SIZE) {
                prefixTable[idx] = prefix;
                charTable[idx]   = c;
                codeTable[idx]   = static_cast<uint16_t>(nextCode);
                ++nextCode;
            }

            prefix = c;
        }
    }

    codes.push_back(prefix);

    result.codeCount = codes.size();
    result.data.reserve(codes.size() * 2);

    for (uint16_t code : codes) {
        result.data.push_back(static_cast<uint8_t>(code & 0xFF));
        result.data.push_back(static_cast<uint8_t>((code >> 8) & 0xFF));
    }

    return result;
}

std::vector<uint8_t> LZW::decompress(const EncodedData& encoded) {
    if (encoded.originalSize == 0) {
        if (encoded.codeCount != 0 || !encoded.data.empty())
            throw std::invalid_argument("Invalid empty LZW stream");
        return {};
    }

    if (encoded.data.size() != encoded.codeCount * 2)
        throw std::invalid_argument("LZW data size does not match code count");

    if (encoded.codeCount == 0)
        throw std::invalid_argument("LZW stream has no codes but non-zero original size");

    std::vector<uint16_t> codes;
    codes.reserve(encoded.codeCount);

    for (uint64_t i = 0; i < encoded.codeCount; ++i) {
        size_t byteIdx = static_cast<size_t>(i * 2);

        uint16_t code = static_cast<uint16_t>(
            static_cast<uint16_t>(encoded.data[byteIdx]) |
            (static_cast<uint16_t>(encoded.data[byteIdx + 1]) << 8));

        codes.push_back(code);
    }

    std::vector<std::string> dict;
    dict.reserve(MAX_DICT_SIZE);

    for (uint32_t i = 0; i < INITIAL_DICT_SIZE; ++i) {
        dict.emplace_back(1, static_cast<char>(i));
    }

    uint32_t nextCode = INITIAL_DICT_SIZE;

    if (codes[0] >= INITIAL_DICT_SIZE)
        throw std::invalid_argument("LZW first code is not a single-byte entry");

    if (encoded.originalSize > static_cast<uint64_t>(std::vector<uint8_t>().max_size())) {
        throw std::length_error("Decoded output exceeds maximum vector size");
    }

    std::vector<uint8_t> output;
    output.reserve(static_cast<size_t>(encoded.originalSize));

    std::string previous = dict[codes[0]];

    for (char c : previous)
        output.push_back(static_cast<uint8_t>(c));

    for (size_t i = 1; i < codes.size(); ++i) {
        uint16_t code = codes[i];
        std::string entry;

        if (code < dict.size()) {
            entry = dict[code];
        }
        else if (code == nextCode) {
            entry = previous + previous[0];
        }
        else {
            throw std::invalid_argument("Invalid LZW code encountered: " + std::to_string(code));
        }

        for (char c : entry) {
            output.push_back(static_cast<uint8_t>(c));
            if (output.size() > encoded.originalSize) {
                throw std::invalid_argument("Decoded data exceeds expected size");
            }
        }

        if (nextCode < MAX_DICT_SIZE) {
            dict.push_back(previous + entry[0]);
            ++nextCode;
        }

        previous = std::move(entry);
    }

    if (output.size() != encoded.originalSize)
        throw std::invalid_argument(
            "LZW decoded size (" + std::to_string(output.size()) +
            ") does not match original size (" + std::to_string(encoded.originalSize) + ")");

    return output;
}

}
}
