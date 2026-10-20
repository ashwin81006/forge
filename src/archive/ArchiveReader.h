#ifndef FORGE_ARCHIVE_READER_H
#define FORGE_ARCHIVE_READER_H

#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <cstddef>
#include "ArchiveFormat.h"

namespace forge {
namespace archive {

class ArchiveReader {
public:
    struct ProgressInfo {
        std::string currentFile;
        size_t fileIndex;
        size_t totalFiles;
        uint64_t bytesExtracted;
        uint64_t totalBytes;
    };

    using ProgressCallback = std::function<void(const ProgressInfo&)>;

    ArchiveReader() = default;

    std::vector<FileMetadata> listArchive(const std::string& archivePath);

    void extractArchive(
        const std::string& archivePath,
        const std::string& destinationDir,
        ProgressCallback progress = nullptr);

    void extractFile(
        const std::string& archivePath,
        const std::string& relativePathToExtract,
        const std::string& destinationDir);
};

} // namespace archive
} // namespace forge

#endif // FORGE_ARCHIVE_READER_H
