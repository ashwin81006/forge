#include "ArchiveWriter.h"
#include "ArchiveFormat.h"
#include "PathSecurity.h"
#include "../io/BinaryIO.h"
#include "../io/Checksum.h"
#include "../engine/CompressionEngine.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <iostream>

namespace forge {
namespace archive {

void ArchiveWriter::compressFolder(
    const std::string& folderPath,
    const std::string& archivePath,
    size_t blockSize,
    bool adaptive,
    ProgressCallback progress) {

    std::filesystem::path root(folderPath);
    if (!std::filesystem::exists(root) || !std::filesystem::is_directory(root)) {
        throw std::runtime_error("Source folder does not exist or is not a directory: " + folderPath);
    }

    std::vector<std::string> relativeFiles;

    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             root, std::filesystem::directory_options::none)) {
        // Skip directory symlinks and directories
        if (entry.is_symlink() || entry.is_directory()) {
            continue;
        }
        if (entry.is_regular_file()) {
            auto rel = std::filesystem::relative(entry.path(), root);
            std::string posixPath = PathSecurity::normalizeToPosix(rel.string());
            PathSecurity::validatePathOrThrow(posixPath);
            relativeFiles.push_back(posixPath);
        }
    }

    compressFiles(folderPath, relativeFiles, archivePath, blockSize, adaptive, progress);
}

void ArchiveWriter::compressFiles(
    const std::string& baseDir,
    const std::vector<std::string>& relativeFiles,
    const std::string& archivePath,
    size_t blockSize,
    bool adaptive,
    ProgressCallback progress) {

    std::ofstream out(archivePath, std::ios::binary);
    if (!out) {
        throw std::runtime_error("Failed to create archive file: " + archivePath);
    }

    // 1. Write Archive Header
    out.write(MAGIC_SIGNATURE, 5);
    io::writeUint16(out, FORMAT_VERSION);
    io::writeUint64(out, static_cast<uint64_t>(relativeFiles.size()));
    io::writeUint64(out, 0); // reserved

    uint64_t totalBytes = 0;
    std::filesystem::path base(baseDir);
    for (const auto& rel : relativeFiles) {
        totalBytes += std::filesystem::file_size(base / rel);
    }

    uint64_t cumulativeProcessed = 0;
    engine::CompressionEngine engine;
    std::vector<uint8_t> buffer(blockSize);

    for (size_t fileIdx = 0; fileIdx < relativeFiles.size(); ++fileIdx) {
        const std::string& relPath = relativeFiles[fileIdx];
        PathSecurity::validatePathOrThrow(relPath);
        std::filesystem::path fullPath = base / relPath;

        uint64_t fileSize = std::filesystem::file_size(fullPath);
        uint64_t blockCount = (fileSize == 0) ? 0 : ((fileSize + blockSize - 1) / blockSize);

        // Compute SHA-256 of uncompressed file
        io::SHA256 sha;
        {
            std::ifstream in(fullPath, std::ios::binary);
            if (!in) {
                throw std::runtime_error("Failed to open source file: " + fullPath.string());
            }
            while (in) {
                in.read(reinterpret_cast<char*>(buffer.data()), blockSize);
                std::streamsize n = in.gcount();
                if (n > 0) {
                    sha.update(buffer.data(), static_cast<size_t>(n));
                }
            }
        }
        auto fileChecksum = sha.finalize();

        // 2. Write File Record Header
        if (relPath.size() > 65535) {
            throw std::length_error("File relative path exceeds 65535 bytes");
        }
        io::writeUint16(out, static_cast<uint16_t>(relPath.size()));
        out.write(relPath.data(), relPath.size());
        io::writeUint64(out, fileSize);
        io::writeUint64(out, blockCount);
        out.write(reinterpret_cast<const char*>(fileChecksum.data()), 32);

        // 3. Compress & Write Block Records
        if (fileSize > 0) {
            std::ifstream in(fullPath, std::ios::binary);
            while (in) {
                in.read(reinterpret_cast<char*>(buffer.data()), blockSize);
                std::streamsize bytesRead = in.gcount();
                if (bytesRead <= 0) break;

                engine::BlockRecord blk = engine.compressBlock(
                    buffer.data(),
                    static_cast<size_t>(bytesRead),
                    adaptive);

                // Write Block Record
                io::writeUint8(out, static_cast<uint8_t>(blk.method));
                io::writeUint32(out, blk.originalBlockSize);
                io::writeUint32(out, blk.storedPayloadSize);
                io::writeUint32(out, blk.metadataSize);
                io::writeUint32(out, blk.blockChecksum);

                if (blk.metadataSize > 0) {
                    out.write(reinterpret_cast<const char*>(blk.metadata.data()), blk.metadataSize);
                }
                if (blk.storedPayloadSize > 0) {
                    out.write(reinterpret_cast<const char*>(blk.payload.data()), blk.storedPayloadSize);
                }

                cumulativeProcessed += bytesRead;
                if (progress) {
                    ProgressInfo p;
                    p.currentFile = relPath;
                    p.fileIndex = fileIdx + 1;
                    p.totalFiles = relativeFiles.size();
                    p.bytesProcessed = cumulativeProcessed;
                    p.totalBytes = totalBytes;
                    progress(p);
                }
            }
        } else {
            if (progress) {
                ProgressInfo p;
                p.currentFile = relPath;
                p.fileIndex = fileIdx + 1;
                p.totalFiles = relativeFiles.size();
                p.bytesProcessed = cumulativeProcessed;
                p.totalBytes = totalBytes;
                progress(p);
            }
        }
    }
}

} // namespace archive
} // namespace forge
