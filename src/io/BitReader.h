#ifndef FORGE_BIT_READER_H
#define FORGE_BIT_READER_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <stdexcept>

namespace forge {
namespace io {

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size, uint64_t totalBits)
        : m_data(data), m_size(size), m_totalBits(totalBits), m_bitPos(0) {}

    explicit BitReader(const std::vector<uint8_t>& buf, uint64_t totalBits)
        : m_data(buf.data()), m_size(buf.size()), m_totalBits(totalBits), m_bitPos(0) {}

    bool readBit() {
        if (m_bitPos >= m_totalBits) {
            throw std::out_of_range("BitReader: attempt to read past end of stream");
        }
        size_t byteIdx = static_cast<size_t>(m_bitPos / 8);
        if (byteIdx >= m_size) {
            throw std::out_of_range("BitReader: buffer overflow");
        }
        int bitOffset = 7 - static_cast<int>(m_bitPos % 8);
        bool bit = ((m_data[byteIdx] >> bitOffset) & 1) != 0;
        ++m_bitPos;
        return bit;
    }

    uint32_t readBits(int count) {
        uint32_t result = 0;
        for (int i = 0; i < count; ++i) {
            result = (result << 1) | (readBit() ? 1 : 0);
        }
        return result;
    }

    bool hasBits() const {
        return m_bitPos < m_totalBits;
    }

    uint64_t remainingBits() const {
        return m_totalBits > m_bitPos ? (m_totalBits - m_bitPos) : 0;
    }

    uint64_t bitPosition() const {
        return m_bitPos;
    }

private:
    const uint8_t* m_data;
    size_t m_size;
    uint64_t m_totalBits;
    uint64_t m_bitPos;
};

} // namespace io
} // namespace forge

#endif // FORGE_BIT_READER_H
