
#include "Huffman.h"
#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace {

	bool hasExpectedPayloadSize(
		const std::vector<uint8_t>& data,
		uint64_t bitCount) {

		uint64_t expectedBytes = bitCount / 8;

		if (bitCount % 8 != 0)
			++expectedBytes;

		return expectedBytes == data.size();
	}

	struct Node {
		uint8_t value;
		uint64_t frequency;
		uint16_t minSymbol;
		uint32_t order;
		std::shared_ptr<Node> left;
		std::shared_ptr<Node> right;

		Node(uint8_t v, uint64_t f, uint32_t o)
			: value(v),
			frequency(f),
			minSymbol(v),
			order(o) {
		}

		Node(std::shared_ptr<Node> l,
			std::shared_ptr<Node> r,
			uint32_t o)
			: value(0),
			frequency(l->frequency + r->frequency),
			minSymbol(std::min(l->minSymbol, r->minSymbol)),
			order(o),
			left(std::move(l)),
			right(std::move(r)) {
		}

		bool isLeaf() const {
			return !left && !right;
		}
	};

	struct Compare {
		bool operator()(
			const std::shared_ptr<Node>& a,
			const std::shared_ptr<Node>& b) const {

			if (a->frequency != b->frequency)
				return a->frequency > b->frequency;

			if (a->minSymbol != b->minSymbol)
				return a->minSymbol > b->minSymbol;

			return a->order > b->order;
		}
	};

	using NodePtr = std::shared_ptr<Node>;

	NodePtr buildTree(
		const std::vector<uint64_t>& frequencies) {

		std::priority_queue<
			NodePtr,
			std::vector<NodePtr>,
			Compare
		> queue;

		uint32_t nextOrder = 0;

		for (size_t i = 0; i < frequencies.size(); ++i) {
			if (frequencies[i] > 0) {
				queue.push(std::make_shared<Node>(
					static_cast<uint8_t>(i),
					frequencies[i],
					nextOrder++
				));
			}
		}

		while (queue.size() > 1) {
			NodePtr left = queue.top();
			queue.pop();

			NodePtr right = queue.top();
			queue.pop();

			if (left->frequency >
				std::numeric_limits<uint64_t>::max()
				- right->frequency) {
				throw std::overflow_error(
					"Huffman frequency overflow");
			}

			queue.push(std::make_shared<Node>(
				left, right, nextOrder++
			));
		}

		return queue.empty() ? nullptr : queue.top();
	}

	void generateCodes(
		const NodePtr& node,
		const std::string& prefix,
		std::array<std::string, 256>& codes) {

		if (!node)
			return;

		if (node->isLeaf()) {
			codes[node->value] =
				prefix.empty() ? "0" : prefix;
			return;
		}

		generateCodes(node->left, prefix + "0", codes);
		generateCodes(node->right, prefix + "1", codes);
	}
}

Huffman::EncodedData Huffman::compress(
	const std::vector<uint8_t>& input) {

	EncodedData result{};
	result.originalSize = input.size();
	result.bitCount = 0;
	result.frequencies.assign(256, 0);

	for (uint8_t byte : input)
		++result.frequencies[byte];

	if (input.empty())
		return result;

	NodePtr root = buildTree(result.frequencies);

	std::array<std::string, 256> codes{};
	generateCodes(root, "", codes);

	uint8_t currentByte = 0;
	int bitsUsed = 0;

	for (uint8_t byte : input) {
		for (char bit : codes[byte]) {
			currentByte = static_cast<uint8_t>(
				(currentByte << 1) |
				(bit == '1' ? 1 : 0));

			++bitsUsed;
			++result.bitCount;

			if (bitsUsed == 8) {
				result.data.push_back(currentByte);
				currentByte = 0;
				bitsUsed = 0;
			}
		}
	}

	if (bitsUsed > 0) {
		currentByte = static_cast<uint8_t>(
			currentByte << (8 - bitsUsed));

		result.data.push_back(currentByte);
	}

	return result;
}

std::vector<uint8_t> Huffman::decompress(
	const EncodedData& encoded) {

	if (encoded.frequencies.size() != 256)
		throw std::invalid_argument(
			"Invalid Huffman frequency table");

	uint64_t total = 0;

	for (uint64_t frequency : encoded.frequencies) {
		if (total > encoded.originalSize ||
			frequency > encoded.originalSize - total) {
			throw std::invalid_argument(
				"Invalid Huffman frequencies");
		}

		total += frequency;
	}

	if (total != encoded.originalSize)
		throw std::invalid_argument(
			"Frequency total does not match original size");

	if (encoded.originalSize == 0) {
		if (encoded.bitCount != 0 ||
			!encoded.data.empty()) {
			throw std::invalid_argument(
				"Invalid empty Huffman stream");
		}

		return {};
	}

	NodePtr root = buildTree(encoded.frequencies);

	if (!root)
		throw std::invalid_argument(
			"Missing Huffman tree");

	if (!hasExpectedPayloadSize(
		encoded.data, encoded.bitCount)) {
		throw std::invalid_argument(
			"Invalid Huffman payload size");
	}

	if (encoded.originalSize >
		static_cast<uint64_t>(
			std::vector<uint8_t>().max_size())) {
		throw std::length_error(
			"Decoded output exceeds maximum vector size");
	}

	std::vector<uint8_t> output;
	output.reserve(
		static_cast<size_t>(encoded.originalSize));

	if (root->isLeaf()) {
		if (encoded.bitCount != encoded.originalSize)
			throw std::invalid_argument(
				"Invalid single-symbol bit count");

		for (uint64_t i = 0;
			i < encoded.bitCount; ++i) {

			size_t byteIndex =
				static_cast<size_t>(i / 8);

			int bitIndex =
				7 - static_cast<int>(i % 8);

			if (((encoded.data[byteIndex] >>
				bitIndex) & 1) != 0) {
				throw std::invalid_argument(
					"Invalid single-symbol code");
			}

			output.push_back(root->value);
		}

		if (output.size() != encoded.originalSize)
			throw std::invalid_argument(
				"Incorrect decoded size");

		return output;
	}

	NodePtr current = root;

	for (uint64_t i = 0;
		i < encoded.bitCount; ++i) {

		size_t byteIndex =
			static_cast<size_t>(i / 8);

		int bitIndex =
			7 - static_cast<int>(i % 8);

		bool bit =
			((encoded.data[byteIndex] >> bitIndex) & 1) != 0;

		current = bit ? current->right : current->left;

		if (!current)
			throw std::invalid_argument(
				"Invalid Huffman code");

		if (current->isLeaf()) {
			output.push_back(current->value);
			current = root;

			if (output.size() > encoded.originalSize)
				throw std::invalid_argument(
					"Decoded data exceeds expected size");
		}
	}

	if (current != root ||
		output.size() != encoded.originalSize) {
		throw std::invalid_argument(
			"Incomplete or invalid Huffman stream");
	}

	return output;
}
