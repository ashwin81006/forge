# FORGE — Binary Archive Format Specification (.forge)

## 1. Specification Overview
- **Format Name**: FORGE Binary Archive Format
- **Extension**: `.forge`
- **Version**: 1
- **Byte Order**: Strictly **Little-Endian** across all multi-byte integers and bitstreams.

---

## 2. Byte-Level Layout

```text
+-------------------------------------------------------------------+
|                          ARCHIVE HEADER                           |
|  Magic (5 bytes) | Version (2) | File Count (8) | Reserved (8)   |
+-------------------------------------------------------------------+
|                           FILE RECORD 1                           |
|  PathLen (2) | RelPath (PathLen) | OrigSize (8) | BlockCount (8) |
|  SHA-256 Checksum (32 bytes)                                      |
+-------------------------------------------------------------------+
|                         BLOCK RECORD 1.1                          |
|  MethodID (1) | OrigBlockSize (4) | PayloadSize (4)              |
|  MetadataSize (4) | CRC-32 Checksum (4)                           |
|  Metadata Bytes (MetadataSize)                                    |
|  Payload Bytes (PayloadSize)                                      |
+-------------------------------------------------------------------+
|                         BLOCK RECORD 1.2                          |
|  ...                                                              |
+-------------------------------------------------------------------+
|                           FILE RECORD 2                           |
|  ...                                                              |
+-------------------------------------------------------------------+
```

---

## 3. Detailed Field Descriptions

### 3.1 Archive Header
| Offset | Field Name | Type | Size (Bytes) | Description |
|:---|:---|:---|:---:|:---|
| 0 | `magic_signature` | `char[5]` | 5 | ASCII `"FORGE"` (`0x46, 0x4F, 0x52, 0x47, 0x45`) |
| 5 | `format_version` | `uint16_t` | 2 | Little-endian integer, currently `1` |
| 7 | `file_count` | `uint64_t` | 8 | Total number of file records stored in archive |
| 15| `reserved` | `uint64_t` | 8 | Reserved for future expansion, set to `0` |

### 3.2 File Record Header
| Offset | Field Name | Type | Size (Bytes) | Description |
|:---|:---|:---|:---:|:---|
| 0 | `relative_path_length` | `uint16_t` | 2 | Length of UTF-8 relative path string ($L$) |
| 2 | `relative_path` | `char[L]` | $L$ | Normalized POSIX relative path (forward slashes) |
| $2+L$ | `original_file_size` | `uint64_t` | 8 | Uncompressed file size in bytes |
| $10+L$ | `block_count` | `uint64_t` | 8 | Number of block records belonging to this file |
| $18+L$ | `file_checksum` | `uint8_t[32]` | 32 | Cryptographic SHA-256 digest of original file |

### 3.3 Block Record Header
| Offset | Field Name | Type | Size (Bytes) | Description |
|:---|:---|:---|:---:|:---|
| 0 | `method_id` | `uint8_t` | 1 | Compression Method: `0`=RAW, `1`=HUFFMAN, `2`=LZW |
| 1 | `original_block_size` | `uint32_t` | 4 | Size of uncompressed block data |
| 5 | `stored_payload_size` | `uint32_t` | 4 | Byte length of compressed payload |
| 9 | `metadata_size` | `uint32_t` | 4 | Byte length of codec reconstruction metadata |
| 13 | `block_checksum` | `uint32_t` | 4 | CRC-32 (IEEE 802.3) of uncompressed block |
| 17 | `metadata` | `uint8_t[]` | $M$ | Codec metadata bytes |
| $17+M$ | `payload` | `uint8_t[]` | $P$ | Compressed payload stream |

---

## 4. Method Payload & Metadata Layouts

### 4.1 Method 0: RAW
- **Metadata**: 0 bytes (`metadata_size = 0`).
- **Payload**: Raw uncompressed bytes (`stored_payload_size = original_block_size`).

### 4.2 Method 1: HUFFMAN
- **Metadata** (2060 bytes):
  - `original_size` (`uint32_t`, 4 bytes)
  - `bit_count` (`uint64_t`, 8 bytes)
  - `frequencies` (`uint64_t[256]`, 2048 bytes) — little-endian frequency table.
- **Payload**: Bitstream packed into bytes (MSB to LSB per byte), zero-padded to byte boundary.

### 4.3 Method 2: LZW
- **Metadata** (12 bytes):
  - `original_size` (`uint32_t`, 4 bytes)
  - `code_count` (`uint64_t`, 8 bytes)
- **Payload**: Sequence of `code_count` 16-bit codes stored in little-endian order (`code_count * 2` bytes).

---

## 5. Security & Validation Rules
1. **Magic Signature**: Must match `"FORGE"`. Any mismatch rejects immediately.
2. **Version**: Must equal `1`. Unsupported versions are rejected.
3. **Path Traversal Protection**:
   - Reject empty paths.
   - Reject drive letters (e.g. `C:`).
   - Reject leading `/` or `\`.
   - Reject any path segment containing `..`.
   - Confirm canonical destination resides within the extraction root folder.
4. **Memory Allocation Safety**:
   - `stored_payload_size` and `metadata_size` are validated against a 64 MiB ceiling prior to buffer allocation.
   - Vectors are bounded by `max_size()`.
5. **Integrity Validation**:
   - Each block's uncompressed data is verified against its CRC-32 before emission.
   - The entire extracted file is validated against stored SHA-256 before extraction completes.
