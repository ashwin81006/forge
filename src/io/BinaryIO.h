#ifndef FORGE_BINARY_IO_H
#define FORGE_BINARY_IO_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace forge {
namespace io {

// ── Little-Endian Writers ──────────────────────────────────────────

inline void writeUint8(std::vector<uint8_t>& buf, uint8_t val) {
    buf.push_back(val);
}

inline void writeUint16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
}

inline void writeUint32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

inline void writeUint64(std::vector<uint8_t>& buf, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        buf.push_back(static_cast<uint8_t>((val >> (i * 8)) & 0xFF));
    }
}

inline void writeUint8(std::ostream& os, uint8_t val) {
    os.put(static_cast<char>(val));
}

inline void writeUint16(std::ostream& os, uint16_t val) {
    char b[2] = {
        static_cast<char>(val & 0xFF),
        static_cast<char>((val >> 8) & 0xFF)
    };
    os.write(b, 2);
}

inline void writeUint32(std::ostream& os, uint32_t val) {
    char b[4] = {
        static_cast<char>(val & 0xFF),
        static_cast<char>((val >> 8) & 0xFF),
        static_cast<char>((val >> 16) & 0xFF),
        static_cast<char>((val >> 24) & 0xFF)
    };
    os.write(b, 4);
}

inline void writeUint64(std::ostream& os, uint64_t val) {
    char b[8];
    for (int i = 0; i < 8; ++i) {
        b[i] = static_cast<char>((val >> (i * 8)) & 0xFF);
    }
    os.write(b, 8);
}

// ── Little-Endian Readers (from Buffer) ───────────────────────────

inline uint8_t readUint8(const uint8_t*& ptr, size_t& remaining) {
    if (remaining < 1) throw std::out_of_range("Buffer underflow reading uint8");
    uint8_t val = *ptr++;
    remaining -= 1;
    return val;
}

inline uint16_t readUint16(const uint8_t*& ptr, size_t& remaining) {
    if (remaining < 2) throw std::out_of_range("Buffer underflow reading uint16");
    uint16_t val = static_cast<uint16_t>(ptr[0]) |
                   (static_cast<uint16_t>(ptr[1]) << 8);
    ptr += 2;
    remaining -= 2;
    return val;
}

inline uint32_t readUint32(const uint8_t*& ptr, size_t& remaining) {
    if (remaining < 4) throw std::out_of_range("Buffer underflow reading uint32");
    uint32_t val = static_cast<uint32_t>(ptr[0]) |
                   (static_cast<uint32_t>(ptr[1]) << 8) |
                   (static_cast<uint32_t>(ptr[2]) << 16) |
                   (static_cast<uint32_t>(ptr[3]) << 24);
    ptr += 4;
    remaining -= 4;
    return val;
}

inline uint64_t readUint64(const uint8_t*& ptr, size_t& remaining) {
    if (remaining < 8) throw std::out_of_range("Buffer underflow reading uint64");
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(ptr[i]) << (i * 8));
    }
    ptr += 8;
    remaining -= 8;
    return val;
}

// ── Little-Endian Readers (from Stream) ───────────────────────────

inline uint8_t readUint8(std::istream& is) {
    int c = is.get();
    if (c == EOF) throw std::runtime_error("Unexpected EOF reading uint8");
    return static_cast<uint8_t>(c);
}

inline uint16_t readUint16(std::istream& is) {
    char b[2];
    if (!is.read(b, 2)) throw std::runtime_error("Unexpected EOF reading uint16");
    return static_cast<uint16_t>(static_cast<uint8_t>(b[0])) |
           (static_cast<uint16_t>(static_cast<uint8_t>(b[1])) << 8);
}

inline uint32_t readUint32(std::istream& is) {
    char b[4];
    if (!is.read(b, 4)) throw std::runtime_error("Unexpected EOF reading uint32");
    return static_cast<uint32_t>(static_cast<uint8_t>(b[0])) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[1])) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[2])) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b[3])) << 24);
}

inline uint64_t readUint64(std::istream& is) {
    char b[8];
    if (!is.read(b, 8)) throw std::runtime_error("Unexpected EOF reading uint64");
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(static_cast<uint8_t>(b[i])) << (i * 8));
    }
    return val;
}

} // namespace io
} // namespace forge

#endif // FORGE_BINARY_IO_H
