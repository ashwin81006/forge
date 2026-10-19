#ifndef FORGE_ARCHIVE_WRITER_H
#define FORGE_ARCHIVE_WRITER_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstddef>
#include "ArchiveFormat.h"
#include "../engine/CompressionEngine.h"

namespace forge {
namespace archive {

class ArchiveWriter {
public:
    struct ProgressInfo {
        std::string currentFile;
        size_t fileIndex;
        size_t totalFiles;
        uint64_t bytesProcessed;
        uint64_t totalBytes;
    };

    using ProgressCallback = std::function<void(const ProgressInfo&)>;

    ArchiveWriter() = default;

    void compressFolder(
        const std::string& folderPath,
        const std::string& archivePath,
        size_t blockSize = engine::CompressionEngine::DEFAULT_BLOCK_SIZE,
        bool adaptive = true,
        ProgressCallback progress = nullptr);

    void compressFiles(
        const std::string& baseDir,
        const std::vector<std::string>& relativeFiles,
        const std::string& archivePath,
        size_t blockSize = engine::CompressionEngine::DEFAULT_BLOCK_SIZE,
        bool adaptive = true,
        ProgressCallback progress = nullptr);
};

} // namespace archive
} // namespace forge

#endif // FORGE_ARCHIVE_WRITER_H
