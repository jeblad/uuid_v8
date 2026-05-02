# uuid_v8

A small single header library to generate and parse UUIDv8

## Core Concept: The Nilsimsa-style Similarity Digest

This library implements a **Locality Sensitive Hash (LSH)** embedded within the 122 custom bits of a 128-bit UUIDv8 structure. Unlike cryptographic hashes (like SHA-256) where a single bit change results in a completely different hash, this digest ensures that similar inputs produce similar bitmasks.

By measuring the **Hamming distance** or **Jaccard similarity** of these UUIDs, you can perform fuzzy matching on targets without storing the original plain-text data.

### Key Architectural Features

* **N-gram Sliding Window:** Processes input as sequences of $N$ characters (typically bigrams or trigrams).
* **Sparse Encoding ($M$ bits):** Every N-gram sets $M$ independent bits in the 122-bit custom area. This redundancy allows the "signal" to cut through noisy streams.
* **Loop Peeling:** The hot generation loop is split into phases (leading-padding, steady-state, trailing-padding) to eliminate boundary branches, allowing the CPU to run at full speed.
* **Zero-Allocation:** Operates entirely on the stack during the hot path (generation and streaming). No heap allocations occur during normal operation, avoiding "stalls" in high-frequency systems.
* **Exception-Free (noexcept):** Critical execution paths are marked `noexcept`, ensuring deterministic performance and making it safe for use in low-level daemons and real-time character devices.
* **RFC 4122 Compliance:** Automatically manages the Version 8 (bits 48-51) and Variant (bits 62-63) fields, so the result is always a valid UUID.

## Advanced Functionality

### 1. Dependency Injection (Policy-Based Design)

The "Beast" is highly configurable through C++20 template policies. Users can inject custom behavior for:

* **IndexingStrategy:** Toggle between `utf8_strategy` (code-point aware) and `byte_strategy` (raw binary).
* **NormalizationStrategy:** Control noise reduction (e.g., `case_insensitive_strategy`) or implement a **Stripping Filter** to skip punctuation entirely.
* **HashingStrategy:** Swap the default FNV-1a for alternative algorithms.
* **SparsityStrategy:** Customize how bits are mapped into the bitmask.

### 2. Similarity Metrics

The library provides two ways to compare identifiers:

* **Hamming Distance:** Absolute bitwise difference. Useful for fixed-width comparisons.
* **Jaccard Similarity:** Calculated as $\frac{\text{Intersection}}{\text{Union}}$. This is the gold standard for sparse digests, as it ignores "empty space" and focuses purely on the shared "texture" of the strings.

### 3. Real-time Streaming Observer

For "snooping" on character streams (e.g., monitoring a daemon's input), the `observer` class provides a rolling matcher:

* **Triple Ring-Buffer:** Efficiently manages the sliding window across a continuous stream.
* **Weighted Bitset:** Uses an array of counters to track the "presence" of N-grams. As characters arrive, the oldest gram's influence is subtracted and the newest is added.
* **Callback Interface:** Automatically triggers a signal when the rolling similarity to a target UUID exceeds a user-defined threshold.

## Algorithm Overview

The core algorithm processes the target using a single-pass sliding window of UTF-8 N-grams. Each N-gram is hashed via FNV-1a and mapped to specific bit positions in the 128-bit integer. This technique is highly sensitive to the "texture" of the input, making it effective for distinguishing between closely related datasets. The original Nilsimsa algorithm, upon which this approach is based, was first introduced anonymously circa 2001.

Unlike some academic modifications (e.g., Damiani et al., 2002) which utilize a second refinement pass, this library sticks to a performance-oriented single-pass strategy. We favor increasing the sliding window resolution or sparse encoding density to manage quantization error rather than introducing multi-pass computational overhead.

For example, a target "CRAZY HORSE" is processed by splitting it into conceptual "words". Each word is then further broken down into a sliding window of N code points (defaulting to 3, but configurable from 2 upwards). For N=3, "CRAZY" becomes "  C", " CR", "CRA", "RAZ", "AZY", "ZY ", "Y  ".

## Building and Testing

To build the project and run the foundational tests, use the following commands:

```bash
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

### Versioning Workflow

This project uses a git `pre-commit` hook to keep the version strings in `uuid_v8.hpp` synchronized with Git tags and commit hashes. The hook is automatically installed into your local `.git/hooks` directory when you run the `cmake` configuration command. This ensures that every commit contains an updated and tested header file.

## Acknowledgements

*Created with assistance from AI tools (Gemini 2.5, 3.0, and 3.1, in both Flash and Pro versions) across all parts of this work.*

This project was developed independently, with no external financial or institutional support other than the AI tools mentioned. The views and conclusions contained herein are those of the author(s) and should not be interpreted as representing the official policies or endorsements, either expressed or implied, of any external agency or entity.
