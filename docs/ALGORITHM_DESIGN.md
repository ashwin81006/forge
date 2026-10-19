# FORGE — Algorithm Design & Complexity Analysis

## 1. Huffman Coding Engine

### 1.1 Algorithmic Basis & Greedy Choice Justification
Huffman's algorithm constructs an optimal prefix-free binary tree using a greedy approach.
- **Greedy Choice Property**: At each step, merging the two nodes with the lowest frequencies minimizes the weighted path length $L(T) = \sum f(c) \cdot d_T(c)$.
- **Optimal Substructure**: The optimal tree for an alphabet $\Sigma$ contains the optimal sub-tree for the reduced alphabet $\Sigma'$ formed by combining the two least frequent symbols.
- **Deterministic Canonical Ordering**: In FORGE, tie-breaking in the min-priority queue uses `(frequency, minSymbol, order)` to ensure identical tree structures across compilers and architectures.

### 1.2 Huffman Pseudocode
```text
Algorithm BuildHuffmanTree(frequencies):
    Q = MinPriorityQueue()
    For symbol from 0 to 255:
        If frequencies[symbol] > 0:
            Q.insert(Node(symbol, frequencies[symbol], order++))
    
    While Q.size() > 1:
        left = Q.extractMin()
        right = Q.extractMin()
        parent = Node(left.freq + right.freq, min(left.sym, right.sym), left, right, order++)
        Q.insert(parent)
        
    Return Q.extractMin()
```

### 1.3 Complexity Analysis
- **Frequency Counting**: $O(n)$, where $n$ is block size.
- **Tree Construction**: $O(k \log k)$, where $k \le 256$ is distinct symbols ($k \log k \le 256 \times 8 = 2048$ operations, constant bound).
- **Code Generation**: $O(k)$ tree traversal.
- **Encoding**: $O(n \cdot \bar{L} / 8)$, where $\bar{L}$ is average code length.
- **Decoding**: $O(\text{bits}) = O(n \cdot \bar{L})$.
- **Working Space**: $O(k)$ for tree and table + $O(n)$ for encoded output buffer.

---

## 2. LZW Coding Engine

### 2.1 Algorithmic Basis & Dictionary Limit
LZW is an adaptive dictionary compression algorithm based on Lempel-Ziv-Welch (1984).
- **Initialization**: Pre-populates dictionary with 256 single-byte entries (codes 0 to 255).
- **Fixed-Width Codes**: 16-bit codes emitted in little-endian byte order.
- **Dictionary Cap Rule**: Maximum entries $N = 65536$. When $nextCode == 65536$, dictionary additions are frozen. The encoder continues emitting codes using the existing dictionary, and the decoder mirrors this exact condition without flushing or clearing.
- **Decoder Special Case**: When $code == nextCode$, the decoder reconstructs the sequence as $entry = previous + previous[0]$.

### 2.2 LZW Pseudocode
```text
Algorithm LZW_Compress(input):
    Initialize Dict with { (byte, code) for byte in 0..255 }
    nextCode = 256
    current = String(input[0])
    
    For i from 1 to input.length - 1:
        candidate = current + input[i]
        If candidate in Dict:
            current = candidate
        Else:
            EmitCode(Dict[current])
            If nextCode < 65536:
                Dict[candidate] = nextCode++
            current = String(input[i])
            
    EmitCode(Dict[current])
```

### 2.3 Complexity Analysis
- **Encoding Time**: Expected $O(n)$ with hash table lookups; bounded string copies.
- **Decoding Time**: $O(m)$ where $m$ is uncompressed output length.
- **Working Space**: $O(D \cdot \bar{S})$, where $D \le 65536$ entries and $\bar{S}$ is average sequence length. Bounded to $\approx 2$–$4$ MiB.

---

## 3. Adaptive Block-Level Selection

### 3.1 Decision Rule
For each block of size $B$ (default 64 KiB):
1. Compute RAW stored size: $S_{\text{RAW}} = B$.
2. Compute Huffman candidate stored size: $S_{\text{HUFF}} = \text{metadata} + |\text{payload}|$. Metadata includes $2048$ bytes frequency table $+ 12$ bytes headers $= 2060$ bytes.
3. Compute LZW candidate stored size: $S_{\text{LZW}} = \text{metadata} + |\text{payload}|$. Metadata includes $12$ bytes headers $+ 2 \times \text{codeCount}$ bytes payload.
4. Selection Formula:
$$\text{Selected} = \text{argmin}_{m \in \{\text{RAW}, \text{HUFFMAN}, \text{LZW}\}} S_m$$
If $\min(S_{\text{HUFF}}, S_{\text{LZW}}) \ge S_{\text{RAW}}$, the block defaults to **RAW**.

### 3.2 Algorithm Comparison & Trade-Offs

| Metric | RAW | Huffman | LZW | Adaptive FORGE |
|:---|:---|:---|:---|:---|
| **Best Data Type** | Incompressible / Media | Skewed Single Symbols | Repetitive Phrases & Source Code | Any heterogeneous mixture |
| **Expansion Risk** | 0% (exact size) | Expands on high entropy | Expands on random bytes (up to 2x) | 0% (falls back to RAW) |
| **Compression Time**| Instantaneous ($O(n)$ memcpy) | Moderate ($O(n)$) | Moderate ($O(n)$) | Sum of candidates evaluated |
| **Decompression Time**| Instantaneous | Fast | Very Fast | Fast |
| **Metadata Overhead** | 0 bytes | 2060 bytes | 12 bytes | 0 to 2060 bytes |

### 3.3 When Adaptive Selection Wins vs When It Incurs Overhead
- **Wins**: In multi-file archives or mixed-content files where text, binary code, and pre-compressed media coexist. Each block adopts its optimal representation without degrading other blocks.
- **Overhead**: In homogeneous workloads where one algorithm dominates, evaluating both Huffman and LZW per block consumes extra CPU time ($\approx 2$–$10$ ms per block). However, the resulting compressed size is mathematically guaranteed to never exceed RAW input size.
