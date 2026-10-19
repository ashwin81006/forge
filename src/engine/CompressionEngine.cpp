#include "CompressionEngine.h"
#include "../huffman/Huffman.h"
#include "../lzw/LZW.h"
#include "../io/BinaryIO.h"
#include "../io/Checksum.h"

#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace forge {
namespace engine {

BlockRecord CompressionEngine::buildRawBlock(const uint8_t* data, size_t size) {
    BlockRecord r;
    r.method = Method::RAW;
    r.originalBlockSize = static_cast<uint32_t>(size);
    r.storedPayloadSize = static_cast<uint32_t>(size);
    r.metadataSize = 0;
    r.payload.assign(data, data + size);
    return r;
}

BlockRecord CompressionEngine::buildHuffmanBlock(const uint8_t* data, size_t size) {
    Huffman huff;
    std::vector<uint8_t> input(data, data + size);
    auto enc = huff.compress(input);

    BlockRecord r;
    r.method = Method::HUFFMAN;
    r.originalBlockSize = static_cast<uint32_t>(size);
    r.storedPayloadSize = static_cast<uint32_t>(enc.data.size());

    // Serialize metadata: originalSize (4) + bitCount (8) + frequencies (256 * 8)
    io::writeUint32(r.metadata, static_cast<uint32_t>(enc.originalSize));
    io::writeUint64(r.metadata, enc.bitCount);
    for (size_t i = 0; i < 256; ++i) {
        io::writeUint64(r.metadata, enc.frequencies[i]);
    }
    r.metadataSize = static_cast<uint32_t>(r.metadata.size());
    r.payload = std::move(enc.data);
    return r;
}

BlockRecord CompressionEngine::buildLzwBlock(const uint8_t* data, size_t size) {
    LZW lzw;
    std::vector<uint8_t> input(data, data + size);
    auto enc = lzw.compress(input);

    BlockRecord r;
    r.method = Method::LZW;
    r.originalBlockSize = static_cast<uint32_t>(size);
    r.storedPayloadSize = static_cast<uint32_t>(enc.data.size());

    // Serialize metadata: originalSize (4) + codeCount (8)
    io::writeUint32(r.metadata, static_cast<uint32_t>(enc.originalSize));
    io::writeUint64(r.metadata, enc.codeCount);
    r.metadataSize = static_cast<uint32_t>(r.metadata.size());
    r.payload = std::move(enc.data);
    return r;
}

BlockRecord CompressionEngine::compressBlock(
    const uint8_t* data,
    size_t size,
    bool adaptive,
    Method forceMethod,
    BlockStats* statsOut) {

    uint32_t checksum = io::CRC32::calculate(data, size);

    if (size == 0) {
        BlockRecord empty = buildRawBlock(data, size);
        empty.blockChecksum = checksum;
        if (statsOut) {
            statsOut->selectedMethod = Method::RAW;
            statsOut->originalSize = 0;
            statsOut->storedSize = 0;
        }
        return empty;
    }

    if (!adaptive) {
        BlockRecord r;
        switch (forceMethod) {
            case Method::HUFFMAN: r = buildHuffmanBlock(data, size); break;
            case Method::LZW:     r = buildLzwBlock(data, size); break;
            default:              r = buildRawBlock(data, size); break;
        }
        r.blockChecksum = checksum;
        if (statsOut) {
            statsOut->selectedMethod = r.method;
            statsOut->originalSize = static_cast<uint32_t>(size);
            statsOut->storedSize = r.totalStoredSize();
        }
        return r;
    }

    // Adaptive Selection: evaluate RAW, HUFFMAN, LZW
    auto t0 = std::chrono::high_resolution_clock::now();
    BlockRecord huff = buildHuffmanBlock(data, size);
    auto t1 = std::chrono::high_resolution_clock::now();

    auto t2 = std::chrono::high_resolution_clock::now();
    BlockRecord lzw = buildLzwBlock(data, size);
    auto t3 = std::chrono::high_resolution_clock::now();

    BlockRecord raw = buildRawBlock(data, size);

    uint32_t rawSize = raw.totalStoredSize();
    uint32_t huffSize = huff.totalStoredSize();
    uint32_t lzwSize = lzw.totalStoredSize();

    if (statsOut) {
        statsOut->originalSize = static_cast<uint32_t>(size);
        statsOut->huffmanStoredSize = huffSize;
        statsOut->lzwStoredSize = lzwSize;
        statsOut->huffmanMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        statsOut->lzwMs = std::chrono::duration<double, std::milli>(t3 - t2).count();
    }

    BlockRecord selected;
    if (huffSize < rawSize && huffSize <= lzwSize) {
        selected = std::move(huff);
    } else if (lzwSize < rawSize) {
        selected = std::move(lzw);
    } else {
        selected = std::move(raw);
    }

    selected.blockChecksum = checksum;

    if (statsOut) {
        statsOut->selectedMethod = selected.method;
        statsOut->storedSize = selected.totalStoredSize();
    }

    return selected;
}

BlockRecord CompressionEngine::compressBlock(
    const std::vector<uint8_t>& data,
    bool adaptive,
    Method forceMethod,
    BlockStats* statsOut) {

    return compressBlock(data.data(), data.size(), adaptive, forceMethod, statsOut);
}

std::vector<uint8_t> CompressionEngine::decompressBlock(const BlockRecord& block) {
    if (block.metadata.size() != block.metadataSize) {
        throw std::invalid_argument("Block metadata size mismatch");
    }
    if (block.payload.size() != block.storedPayloadSize) {
        throw std::invalid_argument("Block payload size mismatch");
    }

    std::vector<uint8_t> uncompressed;

    switch (block.method) {
        case Method::RAW: {
            uncompressed = block.payload;
            break;
        }

        case Method::HUFFMAN: {
            if (block.metadataSize < 4 + 8 + 256 * 8) {
                throw std::invalid_argument("Huffman metadata truncated");
            }
            const uint8_t* ptr = block.metadata.data();
            size_t remaining = block.metadata.size();

            Huffman::EncodedData enc;
            enc.originalSize = io::readUint32(ptr, remaining);
            enc.bitCount = io::readUint64(ptr, remaining);
            enc.frequencies.resize(256);
            for (size_t i = 0; i < 256; ++i) {
                enc.frequencies[i] = io::readUint64(ptr, remaining);
            }
            enc.data = block.payload;

            Huffman huff;
            uncompressed = huff.decompress(enc);
            break;
        }

        case Method::LZW: {
            if (block.metadataSize < 4 + 8) {
                throw std::invalid_argument("LZW metadata truncated");
            }
            const uint8_t* ptr = block.metadata.data();
            size_t remaining = block.metadata.size();

            LZW::EncodedData enc;
            enc.originalSize = io::readUint32(ptr, remaining);
            enc.codeCount = io::readUint64(ptr, remaining);
            enc.data = block.payload;

            LZW lzw;
            uncompressed = lzw.decompress(enc);
            break;
        }

        default:
            throw std::invalid_argument("Unknown compression method ID: " +
                                       std::to_string(static_cast<int>(block.method)));
    }

    if (uncompressed.size() != block.originalBlockSize) {
        throw std::runtime_error("Decompressed block size (" +
                                 std::to_string(uncompressed.size()) +
                                 ") != expected (" +
                                 std::to_string(block.originalBlockSize) + ")");
    }

    uint32_t actualCrc = io::CRC32::calculate(uncompressed.data(), uncompressed.size());
    if (actualCrc != block.blockChecksum) {
        throw std::runtime_error("Block CRC32 checksum mismatch: corrupted data");
    }

    return uncompressed;
}

std::vector<BlockRecord> CompressionEngine::compressStream(
    std::istream& is,
    const Config& config) {

    std::vector<BlockRecord> records;
    std::vector<uint8_t> buffer(config.blockSize);
    uint64_t totalBytes = 0;
    size_t blockIdx = 0;

    while (is) {
        is.read(reinterpret_cast<char*>(buffer.data()), config.blockSize);
        std::streamsize bytesRead = is.gcount();
        if (bytesRead <= 0) break;

        BlockRecord rec = compressBlock(
            buffer.data(),
            static_cast<size_t>(bytesRead),
            config.adaptive,
            config.fallbackMethod);

        records.push_back(std::move(rec));
        totalBytes += bytesRead;
        ++blockIdx;

        if (config.progressCallback) {
            config.progressCallback(blockIdx, totalBytes);
        }
    }

    return records;
}

std::vector<BlockRecord> CompressionEngine::compressStream(std::istream& is) {
    Config cfg;
    return compressStream(is, cfg);
}

void CompressionEngine::decompressStream(
    const std::vector<BlockRecord>& blocks,
    std::ostream& os,
    const std::function<void(size_t blocksDone)>& progress) {

    for (size_t i = 0; i < blocks.size(); ++i) {
        std::vector<uint8_t> dec = decompressBlock(blocks[i]);
        if (!dec.empty()) {
            os.write(reinterpret_cast<const char*>(dec.data()), dec.size());
        }
        if (progress) {
            progress(i + 1);
        }
    }
}

} // namespace engine
} // namespace forge
