// Simple test for just the pattern preprocessing logic
#include <iostream>
#include <string>
#include <vector>
#include <cstring>

// Simplified PatternData structure
struct PatternData {
    uint8_t targetBits[32];
    int bitLength;           
    bool isValid;
};

// Copy of the fixed preprocessPattern function
PatternData preprocessPattern(const std::string& pattern) {
    PatternData result;
    result.isValid = false;
    result.bitLength = 0;
    
    // FIXED: Store the pattern as string for direct comparison after encoding
    // Skip "npub" prefix if present
    std::string normalized_pattern = pattern;
    if (pattern.rfind("npub", 0) == 0) {
        normalized_pattern = pattern.substr(4);
        if (normalized_pattern.length() > 0 && normalized_pattern[0] == '1') {
            normalized_pattern = normalized_pattern.substr(1);
        }
    }
    
    // Store the normalized pattern for string-based comparison
    if (normalized_pattern.length() > 0 && normalized_pattern.length() < 32) {
        strncpy((char*)result.targetBits, normalized_pattern.c_str(), normalized_pattern.length());
        result.targetBits[normalized_pattern.length()] = '\0';
        result.bitLength = normalized_pattern.length(); // Store as character count, not bits
        result.isValid = true;
    }
    
    return result;
}

void test_preprocessPattern_comprehensive() {
    std::cout << "=== Comprehensive Pattern Preprocessing Test ===" << std::endl;
    
    struct TestCase {
        std::string input;
        std::string expected_normalized;
        bool should_be_valid;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        // Basic cases
        {"npub1k0jra2", "k0jra2", true, "Basic npub1 prefix"},
        {"npub1k0", "k0", true, "Short npub1 prefix"},
        {"k0jra2", "k0jra2", true, "No prefix"},
        {"k0", "k0", true, "Short no prefix"},
        
        // Long pattern cases
        {"npub1abcdefghijklmnop", "abcdefghijklmnop", true, "Long pattern with prefix"},
        {"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz", true, "Very long pattern no prefix"},
        {"npub1abcdefghijklmnopqrstuvwxyz123", "abcdefghijklmnopqrstuvwxyz123", true, "Extra long with prefix"},
        
        // Edge cases
        {"npub1", "", false, "Only prefix"},
        {"npub", "", false, "Incomplete prefix"},
        {"", "", false, "Empty string"},
        {"a", "a", true, "Single character"},
        
        // Original bug case components
        {"npub1k0jra2", "k0jra2", true, "Original target pattern"},
        {"k0jru9", "k0jru9", true, "Original found pattern suffix"},
    };
    
    int passed = 0;
    int total = test_cases.size();
    
    for (const auto& test_case : test_cases) {
        std::cout << "\nTest: " << test_case.description << std::endl;
        std::cout << "  Input: '" << test_case.input << "'" << std::endl;
        
        PatternData result = preprocessPattern(test_case.input);
        
        bool validity_ok = (result.isValid == test_case.should_be_valid);
        bool pattern_ok = true;
        bool length_ok = true;
        
        if (result.isValid && test_case.should_be_valid) {
            std::string stored_pattern((char*)result.targetBits);
            pattern_ok = (stored_pattern == test_case.expected_normalized);
            length_ok = (result.bitLength == (int)test_case.expected_normalized.length());
            
            std::cout << "  Expected: '" << test_case.expected_normalized << "'" << std::endl;
            std::cout << "  Got: '" << stored_pattern << "'" << std::endl;
            std::cout << "  Expected length: " << test_case.expected_normalized.length() << std::endl;
            std::cout << "  Got length: " << result.bitLength << std::endl;
        }
        
        bool test_passed = validity_ok && pattern_ok && length_ok;
        if (test_passed) passed++;
        
        std::cout << "  Valid: " << (validity_ok ? "✅" : "❌") << std::endl;
        std::cout << "  Pattern: " << (pattern_ok ? "✅" : "❌") << std::endl;
        std::cout << "  Length: " << (length_ok ? "✅" : "❌") << std::endl;
        std::cout << "  Overall: " << (test_passed ? "✅ PASS" : "❌ FAIL") << std::endl;
    }
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    std::cout << "Success rate: " << (100.0 * passed / total) << "%" << std::endl;
}

void test_actual_bug_case() {
    std::cout << "\n=== Original Bug Case Test ===" << std::endl;
    
    // Simulate the exact bug scenario
    std::string found_npub = "npub1k0jru9s0nwspqmd22x7mc33gctsnnfvknndcv0x37mwnj5fc4csq9xwt3y";
    std::string target_pattern = "npub1k0jra2";
    
    std::cout << "Found npub: " << found_npub << std::endl;
    std::cout << "Target pattern: " << target_pattern << std::endl;
    
    // Extract suffix
    const char* found_suffix = (strncmp(found_npub.c_str(), "npub1", 5) == 0) ? 
                               (found_npub.c_str() + 5) : found_npub.c_str();
    
    // Preprocess target pattern
    PatternData target_data = preprocessPattern(target_pattern);
    
    std::cout << "Found suffix: '" << found_suffix << "'" << std::endl;
    
    if (target_data.isValid) {
        const char* target_processed = (const char*)target_data.targetBits;
        int target_len = target_data.bitLength;
        
        std::cout << "Target processed: '" << target_processed << "'" << std::endl;
        std::cout << "Target length: " << target_len << std::endl;
        
        bool would_match = (strncmp(found_suffix, target_processed, target_len) == 0);
        
        std::cout << "Would match: " << (would_match ? "YES" : "NO") << std::endl;
        std::cout << "Expected: NO (should reject this)" << std::endl;
        std::cout << "Result: " << (would_match ? "❌ FAIL - False positive!" : "✅ PASS - Correctly rejected") << std::endl;
        
        // Show character-by-character comparison
        std::cout << "\nCharacter comparison:" << std::endl;
        for (int i = 0; i < target_len && i < 10; i++) {
            std::cout << "  Position " << i << ": '" << found_suffix[i] << "' vs '" << target_processed[i] << "' -> " 
                      << (found_suffix[i] == target_processed[i] ? "MATCH" : "DIFF") << std::endl;
        }
    } else {
        std::cout << "Target pattern preprocessing failed!" << std::endl;
    }
}

void test_long_patterns() {
    std::cout << "\n=== Long Pattern Test ===" << std::endl;
    
    struct LongTestCase {
        std::string generated;
        std::string pattern;
        bool should_match;
        std::string description;
    };
    
    std::vector<LongTestCase> long_tests = {
        {
            "npub1abcdefghijklmnopqrstuvwxyz1234567890",
            "npub1abcdefghijklmnop",
            true,
            "Long pattern should match beginning"
        },
        {
            "npub1abcdefghijklmnopqrstuvwxyz1234567890",
            "abcdefghijklmnopqrstuvwxyz",
            true,
            "Very long pattern without prefix"
        },
        {
            "npub1abcdefghijklmnopqrstuvwxyz1234567890",
            "npub1abcdefghijklmnopqrstuvwxyz12345",
            true,
            "Extra long pattern"
        },
        {
            "npub1abcdefghijklmnopqrstuvwxyz1234567890",
            "npub1abcdefghijklmnopqrstuvwxyz1234567890z",
            false,
            "Pattern longer than generated"
        },
        {
            "npub1short",
            "npub1verylongpatternthatdoesnotfit",
            false,
            "Pattern much longer than generated"
        }
    };
    
    for (const auto& test : long_tests) {
        std::cout << "\nTest: " << test.description << std::endl;
        std::cout << "  Generated: " << test.generated << std::endl;
        std::cout << "  Pattern: " << test.pattern << std::endl;
        
        // Extract suffix
        const char* gen_suffix = (strncmp(test.generated.c_str(), "npub1", 5) == 0) ? 
                                (test.generated.c_str() + 5) : test.generated.c_str();
        
        // Preprocess pattern
        PatternData pattern_data = preprocessPattern(test.pattern);
        
        if (pattern_data.isValid) {
            const char* target = (const char*)pattern_data.targetBits;
            int target_len = pattern_data.bitLength;
            
            bool actual_match = (strlen(gen_suffix) >= target_len) && 
                               (strncmp(gen_suffix, target, target_len) == 0);
            
            std::cout << "  Generated suffix: " << gen_suffix << std::endl;
            std::cout << "  Target: '" << target << "' (length: " << target_len << ")" << std::endl;
            std::cout << "  Expected: " << (test.should_match ? "MATCH" : "NO MATCH") << std::endl;
            std::cout << "  Actual: " << (actual_match ? "MATCH" : "NO MATCH") << std::endl;
            std::cout << "  Result: " << ((actual_match == test.should_match) ? "✅ PASS" : "❌ FAIL") << std::endl;
        } else {
            std::cout << "  Pattern preprocessing failed!" << std::endl;
            std::cout << "  Result: ❌ FAIL" << std::endl;
        }
    }
}

int main() {
    std::cout << "Pattern Logic Test Suite (Fixed Implementation)" << std::endl;
    std::cout << "===============================================" << std::endl;
    
    test_preprocessPattern_comprehensive();
    test_actual_bug_case();
    test_long_patterns();
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    return 0;
}
