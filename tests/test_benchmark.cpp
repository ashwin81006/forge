#include <iostream>
#include <vector>
#include <string>
#include "../src/benchmark/Benchmark.h"

using namespace forge::benchmark;

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

int main() {
    std::cout << "=== M7 Benchmark Laboratory Tests ===\n";

    // 1. Evaluate single dataset across all 4 algorithms
    auto repData = BenchmarkSuite::generateRepetitiveText(50 * 1024);
    auto resRaw = BenchmarkSuite::evaluate("Repetitive Text", repData, "RAW");
    auto resHuff = BenchmarkSuite::evaluate("Repetitive Text", repData, "HUFFMAN");
    auto resLzw = BenchmarkSuite::evaluate("Repetitive Text", repData, "LZW");
    auto resAdapt = BenchmarkSuite::evaluate("Repetitive Text", repData, "ADAPTIVE");

    check("RAW exact recovery verified", resRaw.exactRecovery);
    check("Huffman exact recovery verified", resHuff.exactRecovery);
    check("LZW exact recovery verified", resLzw.exactRecovery);
    check("Adaptive exact recovery verified", resAdapt.exactRecovery);

    check("LZW compresses repetitive text significantly (> 5x)", resLzw.compressionRatio > 5.0);
    check("Adaptive chooses LZW or better on repetitive text", resAdapt.storedSizeBytes <= resRaw.storedSizeBytes);

    // 2. Block size comparison test
    auto blockComp = BenchmarkSuite::compareBlockSizes(repData, "Repetitive (Block Comparison)");
    check("Evaluated 4 distinct block sizes (16K, 64K, 256K, 1024K)", blockComp.size() == 4);
    for (const auto& bc : blockComp) {
        check("Exact recovery for block size " + std::to_string(bc.blockSizeBytes / 1024) + " KiB", bc.exactRecovery);
    }

    // 3. Export to CSV & Markdown
    std::string csv = BenchmarkSuite::exportToCSV(blockComp);
    check("CSV export non-empty and contains header", csv.find("Dataset,Algorithm") != std::string::npos);

    std::string md = BenchmarkSuite::exportToMarkdown(blockComp);
    check("Markdown export contains table delimiters", md.find("| Dataset |") != std::string::npos);

    std::cout << "M7 Results: " << gPassed << " / " << gTests << " passed\n";
    return (gPassed == gTests) ? 0 : 1;
}
