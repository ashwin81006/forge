
#include "LZW.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <cstddef>

// ─────────────────────────────────────────────────────────────────────
// LZW Compression
//
// Algorithm:
//   1. Initialize dictionary with all 256 single-byte entries.
//   2. Read input bytes, extending the current sequence while it
//      exists in the dictionary.
//   3. When the extended sequence is NOT found:
//        a. Emit the code for the current (known) sequence.
//        b. Add the new extended sequence to the dictionary if the
//           dictionary has not reached MAX_DICT_SIZE.
//        c. Reset the current sequence to the byte that caused the miss.
//   4. After input is exhausted, emit the code for the final sequence.
//   5. Pack all 16-bit codes into the output byte stream (big-endian).
//
// Dictionary representation:
//   Encoder — std::unordered_map<std::string, uint16_t>
//     Fast O(1) amortized lookup of arbitrary byte sequences.
//
//   Decoder — std::vector<std::string>
//     Index-addressable; entry i stores the full byte sequence for
//     code i.  Rebuilt implicitly during decompression (the classic
//     LZW trick: the decoder mirrors the encoder's dictionary).
//
// Code format:
//   Fixed-width 16-bit codes.  The first 256 codes (0–255) represent
//   single bytes.  New sequences get codes 256, 257, … up to 65535.
//   Once the dictionary is full the encoder and decoder simply stop
//   adding entries; existing codes remain valid for the rest of the
//   stream.
// ─────────────────────────────────────────────────────────────────────

LZW::EncodedData LZW::compress(
    const std::vector<uint8_t>& input) {

    EncodedData result{};
    result.originalSize = input.size();
    result.codeCount = 0;

    if (input.empty())
        return result;

    // ── Build initial dictionary (single-byte entries) ──────────────

    std::unordered_map<std::string, uint16_t> dict;
    dict.reserve(MAX_DICT_SIZE);

    for (uint32_t i = 0; i < INITIAL_DICT_SIZE; ++i) {
        dict[std::string(1, static_cast<char>(i))] =
            static_cast<uint16_t>(i);
    }

    uint32_t nextCode = INITIAL_DICT_SIZE;

    // ── Collect codes before packing ────────────────────────────────

    std::vector<uint16_t> codes;
    codes.reserve(input.size());  // upper bound: one code per byte

    std::string current(1, static_cast<char>(input[0]));

    for (size_t i = 1; i < input.size(); ++i) {
        std::string extended = current;
        extended += static_cast<char>(input[i]);

        auto it = dict.find(extended);

        if (it != dict.end()) {
            // Extended sequence already in dictionary — keep going.
            current = std::move(extended);
        }
        else {
            // Emit code for the known sequence.
            codes.push_back(dict.at(current));

            // Add the new sequence if room remains.
            if (nextCode < MAX_DICT_SIZE) {
                dict[extended] = static_cast<uint16_t>(nextCode);
                ++nextCode;
            }

            // Reset to the single byte that caused the miss.
            current.assign(1, static_cast<char>(input[i]));
        }
    }

    // Emit the final sequence.
    codes.push_back(dict.at(current));

    // ── Pack 16-bit codes into bytes (big-endian) ───────────────────

    result.codeCount = codes.size();
    result.data.reserve(codes.size() * 2);

    for (uint16_t code : codes) {
        result.data.push_back(
            static_cast<uint8_t>((code >> 8) & 0xFF));
        result.data.push_back(
            static_cast<uint8_t>(code & 0xFF));
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────
// LZW Decompression
//
// Rebuilds the dictionary by mirroring the encoder's logic.
// Handles the special case where the decoder receives a code that
// equals nextCode (the "code-not-yet-in-dictionary" case in classic
// LZW) by constructing the entry from the previous string plus its
// own first character.
// ─────────────────────────────────────────────────────────────────────

std::vector<uint8_t> LZW::decompress(
    const EncodedData& encoded) {

    if (encoded.originalSize == 0) {
        if (encoded.codeCount != 0 || !encoded.data.empty())
            throw std::invalid_argument(
                "Invalid empty LZW stream");
        return {};
    }

    // ── Validate packed data size ───────────────────────────────────

    if (encoded.data.size() != encoded.codeCount * 2)
        throw std::invalid_argument(
            "LZW data size does not match code count");

    if (encoded.codeCount == 0)
        throw std::invalid_argument(
            "LZW stream has no codes but non-zero original size");

    // ── Unpack 16-bit codes ─────────────────────────────────────────

    std::vector<uint16_t> codes;
    codes.reserve(encoded.codeCount);

    for (uint64_t i = 0; i < encoded.codeCount; ++i) {
        size_t byteIdx = static_cast<size_t>(i * 2);

        uint16_t code = static_cast<uint16_t>(
            (static_cast<uint16_t>(encoded.data[byteIdx]) << 8) |
             static_cast<uint16_t>(encoded.data[byteIdx + 1]));

        codes.push_back(code);
    }

    // ── Build initial dictionary (single-byte entries) ──────────────

    std::vector<std::string> dict;
    dict.reserve(MAX_DICT_SIZE);

    for (uint32_t i = 0; i < INITIAL_DICT_SIZE; ++i) {
        dict.emplace_back(1, static_cast<char>(i));
    }

    uint32_t nextCode = INITIAL_DICT_SIZE;

    // ── Decode ──────────────────────────────────────────────────────

    if (codes[0] >= INITIAL_DICT_SIZE)
        throw std::invalid_argument(
            "LZW first code is not a single-byte entry");

    std::vector<uint8_t> output;
    output.reserve(
        static_cast<size_t>(encoded.originalSize));

    std::string previous = dict[codes[0]];

    for (char c : previous)
        output.push_back(static_cast<uint8_t>(c));

    for (size_t i = 1; i < codes.size(); ++i) {
        uint16_t code = codes[i];
        std::string entry;

        if (code < dict.size()) {
            // Code already in dictionary.
            entry = dict[code];
        }
        else if (code == nextCode) {
            // Special case: code equals the next code to be added.
            // This happens when the encoder emits a code for a
            // sequence that was just added in the same step.
            entry = previous + previous[0];
        }
        else {
            throw std::invalid_argument(
                "Invalid LZW code encountered: " +
                std::to_string(code));
        }

        for (char c : entry)
            output.push_back(static_cast<uint8_t>(c));

        // Add new dictionary entry (previous + first char of entry).
        if (nextCode < MAX_DICT_SIZE) {
            dict.push_back(previous + entry[0]);
            ++nextCode;
        }

        previous = std::move(entry);
    }

    // ── Validate output ─────────────────────────────────────────────

    if (output.size() != encoded.originalSize)
        throw std::invalid_argument(
            "LZW decoded size (" +
            std::to_string(output.size()) +
            ") does not match original size (" +
            std::to_string(encoded.originalSize) + ")");

    return output;
}
