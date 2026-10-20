#ifndef FORGE_PATH_SECURITY_H
#define FORGE_PATH_SECURITY_H

#include <string>
#include <filesystem>
#include <algorithm>
#include <stdexcept>

namespace forge {
namespace archive {

class PathSecurity {
public:
    static bool isSafeRelativePath(const std::string& path) {
        if (path.empty()) return false;

        // Drive letter check (e.g. C:)
        if (path.find(':') != std::string::npos) return false;

        // Absolute path check
        if (path[0] == '/' || path[0] == '\\') return false;

        std::filesystem::path p(path);
        if (p.is_absolute()) return false;

        for (const auto& part : p) {
            std::string s = part.string();
            if (s == "..") return false;
        }

        return true;
    }

    static void validatePathOrThrow(const std::string& path) {
        if (!isSafeRelativePath(path)) {
            throw std::runtime_error("Unsafe path traversal detected in archive entry: " + path);
        }
    }

    static std::string normalizeToPosix(const std::string& path) {
        std::string res = path;
        std::replace(res.begin(), res.end(), '\\', '/');
        return res;
    }
};

} // namespace archive
} // namespace forge

#endif // FORGE_PATH_SECURITY_H
