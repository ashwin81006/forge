#ifndef FORGE_BENCHMARK_H
#define FORGE_BENCHMARK_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "../engine/BlockTypes.h"
#include "../engine/CompressionEngine.h"

namespace forge {
namespace benchmark {

struct MetricResult {
    std::string datasetName;
    std::string algorithmName; // RAW, HUFFMAN, LZW, ADAPTIVE
    size_t blockSizeBytes{64 * 1024};
    uint64_t originalSizeBytes{0};
    uint64_t storedSizeBytes{0};
    double compressionRatio{1.0};
    double spaceSavedPercent{0.0};
    double compressTimeMs{0.0};
    double decompressTimeMs{0.0};
    size_t peakMemoryBytes{0};
    bool exactRecovery{false};
    std::vector<engine::Method> methodsPerBlock;
};

class BenchmarkSuite {
public:
    static MetricResult evaluate(
        const std::string& datasetName,
        const std::vector<uint8_t>& data,
        const std::string& algorithm,
        size_t blockSize = engine::CompressionEngine::DEFAULT_BLOCK_SIZE);

    static std::vector<MetricResult> runStandardSuite();
    static std::vector<MetricResult> compareBlockSizes(const std::vector<uint8_t>& data, const std::string& name = "Dataset");

    static std::string exportToCSV(const std::vector<MetricResult>& results);
    static std::string exportToMarkdown(const std::vector<MetricResult>& results);

    // Standard dataset generators
    static std::vector<uint8_t> generateRepetitiveText(size_t size);
    static std::vector<uint8_t> generateSourceCodeText(size_t size);
    static std::vector<uint8_t> generateNaturalLanguage(size_t size);
    static std::vector<uint8_t> generateBinaryExecutable(size_t size);
    static std::vector<uint8_t> generateRandomBytes(size_t size);
    static std::vector<uint8_t> generateCompressedLikeData(size_t size);
};

} // namespace benchmark
} // namespace forge

#endif // FORGE_BENCHMARK_H
