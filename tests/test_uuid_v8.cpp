#include <iostream>
#include <cassert>
#include <string_view>
#include <string> // For std::string in TEST_ASSERT_MSG
#include <vector>
#include <algorithm>
#include <ranges>
#include <iomanip>
#include "uuid_v8/uuid_v8.hpp"

// Helper for basic reporting
#define TEST_ASSERT(cond) \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    }

// Helper for basic reporting with custom message
#define TEST_ASSERT_MSG(cond, msg) \
    if (!(cond)) { \
        std::cerr << "Assertion failed: " << #cond << " at " << __FILE__ << ":" << __LINE__ << ": " << msg << std::endl; \
        std::exit(1); \
    }

void test_build_info() {
    std::cout << "Testing Build Information..." << std::endl;
    std::cout << "Version: " << CMAKE_EXPECTED_VERSION << std::endl;
    
    // Verify that CMake is passing the correct version macros
    TEST_ASSERT(CMAKE_EXPECTED_VERSION_MAJOR >= 0);
}

void test_utf8_logic() {
    std::cout << "Testing UTF-8 Primitives..." << std::endl;
    using namespace uuid_v8::detail;

    // Test codepoint lengths
    TEST_ASSERT(codepoint_length("CRAZY") == 5);
    TEST_ASSERT(codepoint_length("æøå") == 3);      // Norwegian characters (2 bytes each)
    TEST_ASSERT(codepoint_length("😊") == 1);       // Emoji (4 bytes)
    TEST_ASSERT(codepoint_length("") == 0);

    // Fun Norwegian test cases
    TEST_ASSERT(codepoint_length("MØKKAMANN") == 9);
    TEST_ASSERT(byte_length("MØKKAMANN") == 10);    // 8 ASCII + 1 'Ø' (2 bytes)

    TEST_ASSERT(codepoint_length("ÆLG") == 3);
    TEST_ASSERT(byte_length("ÆLG") == 4);          // 2 ASCII + 1 'Æ' (2 bytes)

    TEST_ASSERT(codepoint_length("ØRN") == 3);
    TEST_ASSERT(byte_length("ØRN") == 4);          // 2 ASCII + 1 'Ø' (2 bytes)

    // Test byte vs codepoint strategies
    TEST_ASSERT(uuid_v8::byte_strategy::length("æøå") == 6);
    TEST_ASSERT(uuid_v8::utf8_strategy::length("æøå") == 3);
}

void test_hashing() {
    std::cout << "Testing FNV-1a Hashing..." << std::endl;
    using namespace uuid_v8::detail;

    auto h1 = fnv1a_hash("abc");
    auto h2 = fnv1a_hash("abc");
    auto h3 = fnv1a_hash("abd");

    TEST_ASSERT(h1 == h2);
    TEST_ASSERT(h1 != h3);
    TEST_ASSERT(h1 != FNV_OFFSET_BASIS);
}

void test_hamming_and_masking() {
    std::cout << "Testing Hamming Distance and UUID Masking..." << std::endl;
    using namespace uuid_v8;

    // Create two UUIDs with raw data that differs only in the version/variant bits
    // Bits 48-51 are version, 62-63 are variant.
    uuid::raw_bytes r1{ 0x0ULL, 0x0ULL };
    uuid::raw_bytes r2{ 0x000F000000000000ULL, 0xC000000000000000ULL };

    uuid u1(r1);
    uuid u2(r2);

    // Hamming distance should ignore those bits, so distance should be 0
    int dist = uuid::hamming_distance(u1, u2);
    TEST_ASSERT(dist == 0);
    TEST_ASSERT(u1.similarity(u2) == 1.0f);

    // Now create a real difference in the custom data area
    uuid::raw_bytes r3{ 0x1ULL, 0x0ULL }; // Flip the very first bit
    uuid u3(r3);

    int dist2 = uuid::hamming_distance(u1, u3);
    TEST_ASSERT(dist2 == 1);
    
    float sim = u1.similarity(u3);
    // 121/122 similarity
    TEST_ASSERT(sim < 1.0f && sim > 0.99f);
}

void test_version_variant_integrity() {
    std::cout << "Testing Version/Variant Integrity..." << std::endl;
    using namespace uuid_v8;

    // Create raw bytes with version bits set to 0xF and variant bits set to 0x3 (all 1s)
    // We want to verify the class forces them to the correct values (0x8 and 0x2 respectively)
    uuid::raw_bytes input{ 0x000F000000000000ULL, 0xC000000000000000ULL };
    uuid u(input);

    auto output = u.as_bytes();

    // Version 8 check: Bits 48-51 of high word must be 1000 (0x8)
    TEST_ASSERT((output.high & 0x000F000000000000ULL) == 0x0008000000000000ULL);

    // Variant check: Bits 62-63 of low word must be 10 (binary 10... -> 0x8 in the nibble)
    TEST_ASSERT((output.low & 0xC000000000000000ULL) == 0x8000000000000000ULL);
}

void test_ngram_boundaries() {
    std::cout << "Testing N-gram Boundary Handling..." << std::endl;
    using namespace uuid_v8;

    // "ÆLG" is 3 code points. 
    // With N=3 and virtual padding of 2 spaces on each side:
    // "  Æ", " ÆL", "ÆLG", "LG ", "G  " -> 5 trigrams
    size_t count = uuid::count_grams<3, utf8_strategy>("ÆLG");
    TEST_ASSERT(count == 5);
}

void test_similarity_metrics() {
    std::cout << "Testing Similarity Metrics (Jaccard vs Hamming)..." << std::endl;
    using namespace uuid_v8;

    uuid u1, u2;
    u1.generate<3, 2>("MØKKAMANN");
    u2.generate<3, 2>("MØKKAMANN"); // Identical input

    TEST_ASSERT(u1.similarity(u2) == 1.0f);
    TEST_ASSERT(u1.jaccard_similarity(u2) == 1.0f);
    TEST_ASSERT(uuid::shared_bits(u1, u2) == u1.popcount());

    uuid u3;
    u3.generate<3, 2>("ÆLG"); // Different input
    
    TEST_ASSERT(u1.jaccard_similarity(u3) < 1.0f);
    TEST_ASSERT(uuid::shared_bits(u1, u3) < u1.popcount());
}

void test_container_integration() {
    std::cout << "Testing Container Integration and Ranges..." << std::endl;
    using namespace uuid_v8;

    std::vector<uuid> collection;
    
    uuid target; target.generate("MØKKAMANN");
    std::cout << "  Target 'MØKKAMANN' popcount: " << target.popcount() << std::endl;
    
    // Populate a "database"
    uuid entry1; entry1.generate("ÆLG");
    uuid entry2; entry2.generate("ØRN");
    uuid entry3; entry3.generate("MØKKAMANN"); // Exact match
    
    collection.push_back(entry1);
    collection.push_back(entry2);
    collection.push_back(entry3);

    // 1. Finding an exact match using C++20 ranges
    auto it = std::ranges::find(collection, target);
    TEST_ASSERT(it != collection.end());
    TEST_ASSERT(*it == entry3);

    // 2. Finding the "Closest" similar type (Fuzzy Search)
    // We use max_element with a projection. The projection calculates the Jaccard 
    // similarity on the fly, and max_element finds the item that maximizes it.
    uuid query; query.generate("MØKKAMENN"); // Typo: 'E' instead of 'A'
    
    auto best_match_it = std::ranges::max_element(collection, 
        {}, // Use default std::less
        [&](const uuid& u) { return query.jaccard_similarity(u); } // Projection
    );

    TEST_ASSERT(best_match_it != collection.end());
    TEST_ASSERT(*best_match_it == entry3); // Should find MØKKAMANN as closest
}

/**
 * Eksempel på en brukerdefinert strategi injisert via maler.
 * Denne strategien stripper bort punktum, men bevarer alt annet 
 * (inkludert multi-byte norske tegn som Æ, Ø, Å).
 */
struct norwegian_stripping_strategy : uuid_v8::case_insensitive_strategy {
    // Vi overstyrer is_filtered for å kun filtrere punktum, men lar andre tegn passere.
    static constexpr bool is_filtered(std::string_view s) noexcept {
        return s == "."; // Kun punktum skal fjernes helt fra strømmen
    }
    // Vi arver normalize fra case_insensitive for å håndtere A-Z og konvertere til mellomrom for ikke-bokstaver/tall
};

void test_noise_robustness() {
    std::cout << "Testing Noise Robustness and Case Folding..." << std::endl;
    using namespace uuid_v8;

    uuid u_upper, u_lower, u_noisy;

    // Bruker en enkel ASCII-streng for å demonstrere stripping med case_insensitive_strategy
    u_upper.generate<3, 2, utf8_strategy, case_insensitive_strategy>("ABC");
    u_lower.generate<3, 2, utf8_strategy, case_insensitive_strategy>("abc");
    u_noisy.generate<3, 2, utf8_strategy, case_insensitive_strategy>("A.B.C");

    TEST_ASSERT_MSG(u_upper.jaccard_similarity(u_lower) > 0.99f, "Case-folding failed for 'ABC' vs 'abc'");
    TEST_ASSERT_MSG(u_upper.jaccard_similarity(u_noisy) > 0.99f, "Stripping punctuation failed for 'ABC' vs 'A.B.C'");

    // Verify case-sensitive mode distinguishes between cases
    uuid u_sens_upper, u_sens_lower;
    u_sens_upper.generate<3, 2, utf8_strategy, case_sensitive_strategy>("ABC");
    u_sens_lower.generate<3, 2, utf8_strategy, case_sensitive_strategy>("abc");
    TEST_ASSERT(u_sens_upper != u_sens_lower);
}

void test_strategy_defaults_and_overrides() {
    std::cout << "Testing Strategy Defaults and Overrides..." << std::endl;
    using namespace uuid_v8;

    // 1. Test Default (nop_normalization_strategy)
    // Defaults are: N=3, M=2, utf8_strategy, nop_normalization_strategy
    uuid u_default_1, u_default_2;
    u_default_1.generate("ABC");
    u_default_2.generate("abc");

    // Without case-folding (the default), these must be different
    TEST_ASSERT_MSG(u_default_1 != u_default_2, "Default should be case-sensitive (NOP)");
    TEST_ASSERT(u_default_1.jaccard_similarity(u_default_2) < 1.0f);

    // 2. Test Override (case_insensitive_strategy)
    uuid u_override_1, u_override_2;
    u_override_1.generate<3, 2, utf8_strategy, case_insensitive_strategy>("ABC");
    u_override_2.generate<3, 2, utf8_strategy, case_insensitive_strategy>("abc");

    // With injected case-folding, these must be identical
    TEST_ASSERT_MSG(u_override_1 == u_override_2, "Injected strategy should perform case-folding");
    TEST_ASSERT(u_override_1.jaccard_similarity(u_override_2) > 0.99f);
    
    std::cout << "  Verified: Defaults are clean (NOP), Injections are active." << std::endl;
}

void test_streaming_observer() {
    std::cout << "Testing Streaming Observer (Ring Buffer Matcher)..." << std::endl;
    using namespace uuid_v8;

    uuid target;
    // Vi må bruke samme strategi her som i observeren for at de skal matche (case-folding)
    target.generate<3, 2, utf8_strategy, case_insensitive_strategy>("MOK"); 
    
    bool matched = false;
    // Align the observer with the target's normalization policy
    observer<3, 2, 3, utf8_strategy, case_insensitive_strategy> obs(target, 0.8f, [&](const uuid&, float score) {
        std::cout << "  Observer callback fired! Score: " << score << std::endl;
        matched = true;
    });

    // Push noise, then the target.
    // The observer needs the same characters that generate() uses,
    // including the trailing spaces that complete the sliding window.
    std::string stream = "---MOK  ---";
    for (char c : stream) {
        obs.push(c);
    }

    TEST_ASSERT(matched);
}

int main() {
    try {
        test_build_info();
        test_utf8_logic();
        test_hashing();
        test_hamming_and_masking();
        test_version_variant_integrity();
        test_ngram_boundaries();
        test_similarity_metrics();
        test_container_integration();
        test_strategy_defaults_and_overrides();
        test_noise_robustness();
        test_streaming_observer();

        std::cout << "\n-----------------------------------" << std::endl;
        std::cout << "All Foundational Tests Passed!" << std::endl;
        std::cout << "The Beast's heart is beating." << std::endl;
        std::cout << "-----------------------------------" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}