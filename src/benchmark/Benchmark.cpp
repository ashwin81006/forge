#include "Benchmark.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>

namespace forge {
namespace benchmark {

MetricResult BenchmarkSuite::evaluate(
    const std::string& datasetName,
    const std::vector<uint8_t>& data,
    const std::string& algorithm,
    size_t blockSize) {

    MetricResult res;
    res.datasetName = datasetName;
    res.algorithmName = algorithm;
    res.blockSizeBytes = blockSize;
    res.originalSizeBytes = data.size();

    engine::CompressionEngine engine;
    bool adaptive = (algorithm == "ADAPTIVE");
    engine::Method forceMethod = engine::Method::RAW;
    if (algorithm == "HUFFMAN") forceMethod = engine::Method::HUFFMAN;
    else if (algorithm == "LZW") forceMethod = engine::Method::LZW;

    std::vector<engine::BlockRecord> blocks;
    blocks.reserve((data.size() + blockSize - 1) / blockSize);

    // 1. Measure Compression Time
    auto t0 = std::chrono::high_resolution_clock::now();
    size_t offset = 0;
    while (offset < data.size()) {
        size_t chunkSize = std::min(blockSize, data.size() - offset);
        engine::BlockRecord blk = engine.compressBlock(
            data.data() + offset, chunkSize, adaptive, forceMethod);
        res.methodsPerBlock.push_back(blk.method);
        res.storedSizeBytes += blk.totalStoredSize();
        blocks.push_back(std::move(blk));
        offset += chunkSize;
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    res.compressTimeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // 2. Measure Decompression Time & Lossless Check
    std::vector<uint8_t> restored;
    restored.reserve(data.size());

    auto t2 = std::chrono::high_resolution_clock::now();
    for (const auto& blk : blocks) {
        auto dec = engine.decompressBlock(blk);
        restored.insert(restored.end(), dec.begin(), dec.end());
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    res.decompressTimeMs = std::chrono::duration<double, std::milli>(t3 - t2).count();

    res.exactRecovery = (restored == data);

    if (res.storedSizeBytes > 0) {
        res.compressionRatio = static_cast<double>(res.originalSizeBytes) / res.storedSizeBytes;
    }
    if (res.originalSizeBytes > 0) {
        res.spaceSavedPercent = (1.0 - (static_cast<double>(res.storedSizeBytes) / res.originalSizeBytes)) * 100.0;
    }

    // Peak buffer memory estimate (active block buffer + dictionary/tree working memory)
    res.peakMemoryBytes = blockSize * 2 + (algorithm == "LZW" ? (65536 * 32) : (256 * sizeof(uint64_t)));

    return res;
}

std::vector<MetricResult> BenchmarkSuite::runStandardSuite() {
    std::vector<MetricResult> results;
    struct DatasetItem {
        std::string name;
        std::vector<uint8_t> data;
    };

    std::vector<DatasetItem> datasets = {
        {"1. Repetitive Text", generateRepetitiveText(100 * 1024)},
        {"2. C++ Source Files", generateSourceCodeText(80 * 1024)},
        {"3. Natural Language", generateNaturalLanguage(80 * 1024)},
        {"4. Binary Executable", generateBinaryExecutable(64 * 1024)},
        {"5. Random Bytes", generateRandomBytes(64 * 1024)},
        {"6. Compressed-like Data", generateCompressedLikeData(64 * 1024)}
    };

    std::vector<std::string> algorithms = {"RAW", "HUFFMAN", "LZW", "ADAPTIVE"};

    for (const auto& ds : datasets) {
        for (const auto& alg : algorithms) {
            results.push_back(evaluate(ds.name, ds.data, alg, 64 * 1024));
        }
    }

    return results;
}

std::vector<MetricResult> BenchmarkSuite::compareBlockSizes(const std::vector<uint8_t>& data, const std::string& name) {
    std::vector<MetricResult> results;
    std::vector<size_t> sizes = {16 * 1024, 64 * 1024, 256 * 1024, 1024 * 1024};
    for (size_t bs : sizes) {
        std::string label = name + " (" + std::to_string(bs / 1024) + " KiB)";
        results.push_back(evaluate(label, data, "ADAPTIVE", bs));
    }
    return results;
}

std::string BenchmarkSuite::exportToCSV(const std::vector<MetricResult>& results) {
    std::ostringstream ss;
    ss << "Dataset,Algorithm,BlockSize_KiB,OriginalSize,StoredSize,Ratio,SpaceSavedPercent,Compress_ms,Decompress_ms,ExactRecovery\n";
    for (const auto& r : results) {
        ss << "\"" << r.datasetName << "\","
           << r.algorithmName << ","
           << (r.blockSizeBytes / 1024) << ","
           << r.originalSizeBytes << ","
           << r.storedSizeBytes << ","
           << std::fixed << std::setprecision(2) << r.compressionRatio << ","
           << std::setprecision(2) << r.spaceSavedPercent << "%,"
           << std::setprecision(2) << r.compressTimeMs << ","
           << std::setprecision(2) << r.decompressTimeMs << ","
           << (r.exactRecovery ? "PASS" : "FAIL") << "\n";
    }
    return ss.str();
}

std::string BenchmarkSuite::exportToMarkdown(const std::vector<MetricResult>& results) {
    std::ostringstream ss;
    ss << "| Dataset | Algorithm | Block | Original | Stored | Ratio | Saved % | Comp (ms) | Decomp (ms) | Status |\n";
    ss << "|:---|:---|:---:|---:|---:|---:|---:|---:|---:|:---:|\n";
    for (const auto& r : results) {
        ss << "| " << r.datasetName << " "
           << "| " << r.algorithmName << " "
           << "| " << (r.blockSizeBytes / 1024) << "K "
           << "| " << r.originalSizeBytes << " "
           << "| " << r.storedSizeBytes << " "
           << "| " << std::fixed << std::setprecision(2) << r.compressionRatio << "x "
           << "| " << std::setprecision(1) << r.spaceSavedPercent << "% "
           << "| " << std::setprecision(2) << r.compressTimeMs << " "
           << "| " << std::setprecision(2) << r.decompressTimeMs << " "
           << "| " << (r.exactRecovery ? "PASS" : "**FAIL**") << " |\n";
    }
    return ss.str();
}

std::vector<uint8_t> BenchmarkSuite::generateRepetitiveText(size_t size) {
    std::string phrase = "FORGE_FAST_OPTIMIZED_RECURSIVE_GENERATION_ENGINE_2026 ";
    std::vector<uint8_t> out;
    out.reserve(size);
    while (out.size() < size) {
        out.insert(out.end(), phrase.begin(), phrase.end());
    }
    out.resize(size);
    return out;
}

std::vector<uint8_t> BenchmarkSuite::generateSourceCodeText(size_t size) {
    std::string code = 
        "#include <iostream>\n"
        "#include <vector>\n"
        "template<typename T>\n"
        "class Node {\n"
        "    T value;\n"
        "    Node* left{nullptr};\n"
        "    Node* right{nullptr};\n"
        "public:\n"
        "    Node(T v) : value(v) {}\n"
        "    void traverse();\n"
        "};\n";
    std::vector<uint8_t> out;
    out.reserve(size);
    while (out.size() < size) {
        out.insert(out.end(), code.begin(), code.end());
    }
    out.resize(size);
    return out;
}

std::vector<uint8_t> BenchmarkSuite::generateNaturalLanguage(size_t size) {
    std::string text = 
        "Algorithm design focuses on optimizing time and space complexity. "
        "Lossless compression enables exact data reconstruction without fidelity loss. "
        "Greedy strategies make locally optimal choices at each stage, while dictionary "
        "encoders construct phrases based on repeated subpatterns in linear time. ";
    std::vector<uint8_t> out;
    out.reserve(size);
    while (out.size() < size) {
        out.insert(out.end(), text.begin(), text.end());
    }
    out.resize(size);
    return out;
}

std::vector<uint8_t> BenchmarkSuite::generateBinaryExecutable(size_t size) {
    std::vector<uint8_t> out(size);
    // Typical x86/x64 opcodes mixed with zeros and relative offsets
    uint8_t opcodes[] = {0x55, 0x48, 0x89, 0xE5, 0x48, 0x83, 0xEC, 0x20, 0x00, 0x00, 0x00, 0xC3, 0x90, 0x90};
    size_t opLen = sizeof(opcodes);
    for (size_t i = 0; i < size; ++i) {
        out[i] = opcodes[i % opLen];
    }
    return out;
}

std::vector<uint8_t> BenchmarkSuite::generateRandomBytes(size_t size) {
    std::vector<uint8_t> out(size);
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : out) b = static_cast<uint8_t>(dist(rng));
    return out;
}

std::vector<uint8_t> BenchmarkSuite::generateCompressedLikeData(size_t size) {
    // High entropy data simulating already-compressed JPEG/PNG/ZIP
    return generateRandomBytes(size);
}

} // namespace benchmark
} // namespace forge
