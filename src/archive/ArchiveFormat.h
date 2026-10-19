#ifndef FORGE_ARCHIVE_FORMAT_H
#define FORGE_ARCHIVE_FORMAT_H

#include <cstdint>
#include <array>
#include <string>
#include <vector>

namespace forge {
namespace archive {

static constexpr char MAGIC_SIGNATURE[5] = {'F', 'O', 'R', 'G', 'E'};
static constexpr uint16_t FORMAT_VERSION = 1;

struct ArchiveHeader {
    std::array<char, 5> magic{'F', 'O', 'R', 'G', 'E'};
    uint16_t version{FORMAT_VERSION};
    uint64_t fileCount{0};
    uint64_t reserved{0};
};

struct FileMetadata {
    std::string relativePath;
    uint64_t originalFileSize{0};
    uint64_t blockCount{0};
    std::array<uint8_t, 32> fileChecksum{}; // SHA-256
    uint64_t storedSize{0};
};

} // namespace archive
} // namespace forge

#endif // FORGE_ARCHIVE_FORMAT_H
