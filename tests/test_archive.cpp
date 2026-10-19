#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <random>
#include <cassert>

#include "../src/archive/ArchiveWriter.h"
#include "../src/archive/ArchiveReader.h"
#include "../src/archive/PathSecurity.h"
#include "../src/io/Checksum.h"

using namespace forge::archive;
using namespace forge::io;

static int gTests = 0;
static int gPassed = 0;

static void check(const std::string& name, bool cond) {
    ++gTests;
    if (cond) {
        ++gPassed;
        std::cout << "  [PASS] " << name << "\n";
    } else {
        std::cout << "  [FAIL] " << name << "\n";
    }
}

static void createDummyFile(const std::filesystem::path& p, size_t size, uint8_t seed = 42) {
    std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p, std::ios::binary);
    std::vector<uint8_t> buf(size);
    for (size_t i = 0; i < size; ++i) {
        buf[i] = static_cast<uint8_t>((i + seed) % 256);
    }
    if (size > 0) {
        out.write(reinterpret_cast<const char*>(buf.data()), size);
    }
}

int main() {
    std::cout << "=== M5 + M6 Archive Creation & Extraction Tests ===\n";

    std::filesystem::path testDir = "test_archive_sandbox";
    std::filesystem::path srcDir = testDir / "src";
    std::filesystem::path outArchive = testDir / "test.forge";
    std::filesystem::path extractDir = testDir / "extracted";

    std::filesystem::remove_all(testDir);
    std::filesystem::create_directories(srcDir);

    // Create nested test files
    createDummyFile(srcDir / "empty.txt", 0);
    createDummyFile(srcDir / "one_byte.bin", 1, 0xAA);
    createDummyFile(srcDir / "sub1" / "text.txt", 1500, 'A');
    createDummyFile(srcDir / "sub1" / "sub2" / "binary.dat", 130 * 1024, 0x17); // spans 3 blocks at 64K

    // 1. Compress folder to .forge archive
    ArchiveWriter writer;
    writer.compressFolder(srcDir.string(), outArchive.string(), 64 * 1024, true);
    check("Archive file created successfully", std::filesystem::exists(outArchive));

    // 2. List archive
    ArchiveReader reader;
    auto files = reader.listArchive(outArchive.string());
    check("Archive lists correct file count (4)", files.size() == 4);

    // 3. Extract complete archive
    reader.extractArchive(outArchive.string(), extractDir.string());
    check("Extracted empty file exists", std::filesystem::exists(extractDir / "empty.txt") && std::filesystem::file_size(extractDir / "empty.txt") == 0);
    check("Extracted single-byte file matches", std::filesystem::file_size(extractDir / "one_byte.bin") == 1);
    check("Extracted multi-block file matches original size", std::filesystem::file_size(extractDir / "sub1" / "sub2" / "binary.dat") == 130 * 1024);

    // Verify byte-level equality with SHA-256
    {
        std::ifstream f1(srcDir / "sub1" / "sub2" / "binary.dat", std::ios::binary);
        std::ifstream f2(extractDir / "sub1" / "sub2" / "binary.dat", std::ios::binary);
        std::vector<uint8_t> b1((std::istreambuf_iterator<char>(f1)), std::istreambuf_iterator<char>());
        std::vector<uint8_t> b2((std::istreambuf_iterator<char>(f2)), std::istreambuf_iterator<char>());
        check("Full roundtrip extracted file is byte-for-byte identical", b1 == b2);
    }

    // 4. Selective single-file extraction
    {
        std::filesystem::path singleExtractDir = testDir / "single_extract";
        reader.extractFile(outArchive.string(), "sub1/text.txt", singleExtractDir.string());
        check("Single file extraction succeeds", std::filesystem::exists(singleExtractDir / "sub1" / "text.txt") &&
              std::filesystem::file_size(singleExtractDir / "sub1" / "text.txt") == 1500);
    }

    // 5. Path traversal security validation
    check("Rejects ../ traversal", !PathSecurity::isSafeRelativePath("../escape.txt"));
    check("Rejects sub/../../traversal", !PathSecurity::isSafeRelativePath("sub/../../escape.txt"));
    check("Rejects drive letter C:", !PathSecurity::isSafeRelativePath("C:/windows/system32/cmd.exe"));
    check("Rejects absolute path /etc/passwd", !PathSecurity::isSafeRelativePath("/etc/passwd"));
    check("Accepts safe nested relative path", PathSecurity::isSafeRelativePath("docs/reports/summary.pdf"));

    // 6. Corrupted archive magic rejection
    {
        std::filesystem::path badArchive = testDir / "bad_magic.forge";
        std::filesystem::copy_file(outArchive, badArchive);
        std::fstream f(badArchive, std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(0);
        f.write("XXXXX", 5);
        f.close();

        bool rejected = false;
        try {
            reader.extractArchive(badArchive.string(), (testDir / "bad_extract").string());
        } catch (const std::exception&) {
            rejected = true;
        }
        check("Rejects corrupted magic header", rejected);
    }

    // Clean up sandbox
    std::filesystem::remove_all(testDir);

    std::cout << "M5+M6 Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
