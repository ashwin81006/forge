#include "ArchiveReader.h"
#include "ArchiveFormat.h"
#include "PathSecurity.h"
#include "../io/BinaryIO.h"
#include "../io/Checksum.h"
#include "../engine/CompressionEngine.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstring>

namespace forge {
namespace archive {

static constexpr uint32_t MAX_SAFE_BLOCK_PAYLOAD = 64 * 1024 * 1024; // 64 MiB limit per block

std::vector<FileMetadata> ArchiveReader::listArchive(const std::string& archivePath) {
    std::ifstream in(archivePath, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open archive: " + archivePath);
    }

    char magic[5];
    if (!in.read(magic, 5) || std::memcmp(magic, MAGIC_SIGNATURE, 5) != 0) {
        throw std::runtime_error("Invalid archive: missing or incorrect magic signature");
    }

    uint16_t version = io::readUint16(in);
    if (version != FORMAT_VERSION) {
        throw std::runtime_error("Unsupported archive format version: " + std::to_string(version));
    }

    uint64_t fileCount = io::readUint64(in);
    io::readUint64(in); // reserved

    std::vector<FileMetadata> files;
    files.reserve(static_cast<size_t>(std::min(fileCount, static_cast<uint64_t>(100000))));

    for (uint64_t i = 0; i < fileCount; ++i) {
        uint16_t pathLen = io::readUint16(in);
        std::string path(pathLen, '\0');
        if (!in.read(&path[0], pathLen)) {
            throw std::runtime_error("Unexpected EOF reading file relative path");
        }
        PathSecurity::validatePathOrThrow(path);

        FileMetadata meta;
        meta.relativePath = path;
        meta.originalFileSize = io::readUint64(in);
        meta.blockCount = io::readUint64(in);
        if (!in.read(reinterpret_cast<char*>(meta.fileChecksum.data()), 32)) {
            throw std::runtime_error("Unexpected EOF reading file checksum");
        }

        uint64_t totalStored = 0;
        for (uint64_t b = 0; b < meta.blockCount; ++b) {
            io::readUint8(in); // method
            io::readUint32(in); // origBlockSize
            uint32_t storedPayload = io::readUint32(in);
            uint32_t metadataSize = io::readUint32(in);
            io::readUint32(in); // blockChecksum

            if (storedPayload > MAX_SAFE_BLOCK_PAYLOAD || metadataSize > MAX_SAFE_BLOCK_PAYLOAD) {
                throw std::runtime_error("Block size exceeds safe bounds");
            }

            totalStored += (storedPayload + metadataSize);
            in.seekg(storedPayload + metadataSize, std::ios::cur);
        }

        meta.storedSize = totalStored;
        files.push_back(std::move(meta));
    }

    return files;
}

void ArchiveReader::extractArchive(
    const std::string& archivePath,
    const std::string& destinationDir,
    ProgressCallback progress) {

    std::ifstream in(archivePath, std::ios::binary);
    if (!in) {
        throw std::runtime_error("Cannot open archive for extraction: " + archivePath);
    }

    char magic[5];
    if (!in.read(magic, 5) || std::memcmp(magic, MAGIC_SIGNATURE, 5) != 0) {
        throw std::runtime_error("Invalid archive: missing magic signature");
    }

    uint16_t version = io::readUint16(in);
    if (version != FORMAT_VERSION) {
        throw std::runtime_error("Unsupported archive version: " + std::to_string(version));
    }

    uint64_t fileCount = io::readUint64(in);
    io::readUint64(in); // reserved

    std::filesystem::path destRoot(destinationDir);
    std::filesystem::create_directories(destRoot);
    std::filesystem::path canonicalDest = std::filesystem::weakly_canonical(destRoot);

    engine::CompressionEngine engine;
    uint64_t cumulativeExtracted = 0;

    for (uint64_t fileIdx = 0; fileIdx < fileCount; ++fileIdx) {
        uint16_t pathLen = io::readUint16(in);
        std::string relPath(pathLen, '\0');
        if (!in.read(&relPath[0], pathLen)) {
            throw std::runtime_error("Archive corrupted: unexpected EOF in path");
        }
        PathSecurity::validatePathOrThrow(relPath);

        uint64_t origSize = io::readUint64(in);
        uint64_t blockCount = io::readUint64(in);
        std::array<uint8_t, 32> expectedChecksum{};
        if (!in.read(reinterpret_cast<char*>(expectedChecksum.data()), 32)) {
            throw std::runtime_error("Archive corrupted: unexpected EOF in file checksum");
        }

        std::filesystem::path target = destRoot / relPath;
        std::filesystem::path canonicalTarget = std::filesystem::weakly_canonical(target);

        // Security: Ensure target resides strictly inside destination folder
        auto [t1, t2] = std::mismatch(canonicalDest.begin(), canonicalDest.end(), canonicalTarget.begin());
        if (t1 != canonicalDest.end()) {
            throw std::runtime_error("Path traversal detected: destination escape attempt: " + relPath);
        }

        std::filesystem::create_directories(target.parent_path());
        std::ofstream out(target, std::ios::binary);
        if (!out) {
            throw std::runtime_error("Failed to create destination file: " + target.string());
        }

        io::SHA256 sha;
        for (uint64_t b = 0; b < blockCount; ++b) {
            engine::BlockRecord blk;
            blk.method = static_cast<engine::Method>(io::readUint8(in));
            blk.originalBlockSize = io::readUint32(in);
            blk.storedPayloadSize = io::readUint32(in);
            blk.metadataSize = io::readUint32(in);
            blk.blockChecksum = io::readUint32(in);

            if (blk.storedPayloadSize > MAX_SAFE_BLOCK_PAYLOAD || blk.metadataSize > MAX_SAFE_BLOCK_PAYLOAD) {
                throw std::runtime_error("Malformed archive block: size bounds exceeded");
            }

            blk.metadata.resize(blk.metadataSize);
            if (blk.metadataSize > 0) {
                if (!in.read(reinterpret_cast<char*>(blk.metadata.data()), blk.metadataSize)) {
                    throw std::runtime_error("Unexpected EOF reading block metadata");
                }
            }

            blk.payload.resize(blk.storedPayloadSize);
            if (blk.storedPayloadSize > 0) {
                if (!in.read(reinterpret_cast<char*>(blk.payload.data()), blk.storedPayloadSize)) {
                    throw std::runtime_error("Unexpected EOF reading block payload");
                }
            }

            std::vector<uint8_t> uncompressed = engine.decompressBlock(blk);
            if (!uncompressed.empty()) {
                out.write(reinterpret_cast<const char*>(uncompressed.data()), uncompressed.size());
                sha.update(uncompressed.data(), uncompressed.size());
            }

            cumulativeExtracted += uncompressed.size();
        }

        out.close();

        auto actualChecksum = sha.finalize();
        if (origSize > 0 && actualChecksum != expectedChecksum) {
            throw std::runtime_error("SHA-256 integrity verification failed for extracted file: " + relPath);
        }

        if (progress) {
            ProgressInfo p;
            p.currentFile = relPath;
            p.fileIndex = fileIdx + 1;
            p.totalFiles = static_cast<size_t>(fileCount);
            p.bytesExtracted = cumulativeExtracted;
            p.totalBytes = origSize;
            progress(p);
        }
    }
}

void ArchiveReader::extractFile(
    const std::string& archivePath,
    const std::string& relativePathToExtract,
    const std::string& destinationDir) {

    std::ifstream in(archivePath, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open archive: " + archivePath);

    char magic[5];
    if (!in.read(magic, 5) || std::memcmp(magic, MAGIC_SIGNATURE, 5) != 0) {
        throw std::runtime_error("Invalid archive magic");
    }

    uint16_t version = io::readUint16(in);
    if (version != FORMAT_VERSION) throw std::runtime_error("Unsupported archive version");

    uint64_t fileCount = io::readUint64(in);
    io::readUint64(in);

    engine::CompressionEngine engine;
    bool found = false;

    for (uint64_t fileIdx = 0; fileIdx < fileCount; ++fileIdx) {
        uint16_t pathLen = io::readUint16(in);
        std::string relPath(pathLen, '\0');
        in.read(&relPath[0], pathLen);
        PathSecurity::validatePathOrThrow(relPath);

        uint64_t origSize = io::readUint64(in);
        uint64_t blockCount = io::readUint64(in);
        std::array<uint8_t, 32> expectedChecksum{};
        in.read(reinterpret_cast<char*>(expectedChecksum.data()), 32);

        if (relPath == relativePathToExtract) {
            found = true;
            std::filesystem::path target = std::filesystem::path(destinationDir) / relPath;
            std::filesystem::create_directories(target.parent_path());
            std::ofstream out(target, std::ios::binary);

            io::SHA256 sha;
            for (uint64_t b = 0; b < blockCount; ++b) {
                engine::BlockRecord blk;
                blk.method = static_cast<engine::Method>(io::readUint8(in));
                blk.originalBlockSize = io::readUint32(in);
                blk.storedPayloadSize = io::readUint32(in);
                blk.metadataSize = io::readUint32(in);
                blk.blockChecksum = io::readUint32(in);

                blk.metadata.resize(blk.metadataSize);
                if (blk.metadataSize > 0) {
                    in.read(reinterpret_cast<char*>(blk.metadata.data()), blk.metadataSize);
                }
                blk.payload.resize(blk.storedPayloadSize);
                if (blk.storedPayloadSize > 0) {
                    in.read(reinterpret_cast<char*>(blk.payload.data()), blk.storedPayloadSize);
                }

                auto dec = engine.decompressBlock(blk);
                out.write(reinterpret_cast<const char*>(dec.data()), dec.size());
                sha.update(dec.data(), dec.size());
            }
            out.close();

            if (origSize > 0 && sha.finalize() != expectedChecksum) {
                throw std::runtime_error("SHA-256 integrity failed for " + relPath);
            }
            return;
        } else {
            // Skip blocks of other files
            for (uint64_t b = 0; b < blockCount; ++b) {
                io::readUint8(in);
                io::readUint32(in);
                uint32_t payload = io::readUint32(in);
                uint32_t meta = io::readUint32(in);
                io::readUint32(in);
                in.seekg(payload + meta, std::ios::cur);
            }
        }
    }

    if (!found) {
        throw std::runtime_error("File not found in archive: " + relativePathToExtract);
    }
}

} // namespace archive
} // namespace forge
