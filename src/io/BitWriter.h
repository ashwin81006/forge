#ifndef FORGE_BIT_WRITER_H
#define FORGE_BIT_WRITER_H

#include <cstdint>
#include <cstddef>
#include <vector>

namespace forge {
namespace io {

class BitWriter {
public:
    explicit BitWriter(std::vector<uint8_t>& target)
        : m_buffer(target), m_currentByte(0), m_bitCount(0) {}

    void writeBit(bool bit) {
        m_currentByte = static_cast<uint8_t>((m_currentByte << 1) | (bit ? 1 : 0));
        ++m_bitCount;
        if (m_bitCount == 8) {
            m_buffer.push_back(m_currentByte);
            m_currentByte = 0;
            m_bitCount = 0;
        }
    }

    void writeBits(uint32_t value, int count) {
        for (int i = count - 1; i >= 0; --i) {
            writeBit(((value >> i) & 1) != 0);
        }
    }

    void flush(uint8_t padBit = 0) {
        if (m_bitCount > 0) {
            for (int i = m_bitCount; i < 8; ++i) {
                m_currentByte = static_cast<uint8_t>((m_currentByte << 1) | (padBit & 1));
            }
            m_buffer.push_back(m_currentByte);
            m_currentByte = 0;
            m_bitCount = 0;
        }
    }

    int bufferedBits() const {
        return m_bitCount;
    }

private:
    std::vector<uint8_t>& m_buffer;
    uint8_t m_currentByte;
    int m_bitCount;
};

} // namespace io
} // namespace forge

#endif // FORGE_BIT_WRITER_H
