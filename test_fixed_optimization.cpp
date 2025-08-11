// Test case for the fixed NostrOptimized implementation
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include "NostrOptimized.h"

void test_preprocessPattern_fixed() {
    std::cout << "=== Testing Fixed preprocessPattern ===" << std::endl;
    
    struct TestCase {
        std::string input;
        std::string expected_normalized;
        bool should_be_valid;
    };
    
    std::vector<TestCase> test_cases = {
        {"npub1k0jra2", "k0jra2", true},
        {"npub1k0", "k0", true},
        {"k0jra2", "k0jra2", true},
        {"k0", "k0", true},
        {"npub1abc", "abc", true},
        {"abc", "abc", true},
        {"npub1abcdefghijklmnop", "abcdefghijklmnop", true},
        {"abcdefghijklmnopqrstuvwxyz", "abcdefghijklmnopqrstuvwxyz", true},
        {"", "", false}, // Empty pattern
        {"npub1", "", false}, // Only prefix
    };
    
    for (const auto& test_case : test_cases) {
        std::cout << "\nTesting pattern: '" << test_case.input << "'" << std::endl;
        
        NostrOptimized::PatternData result = NostrOptimized::preprocessPattern(test_case.input);
        
        std::cout << "  Expected valid: " << (test_case.should_be_valid ? "true" : "false") << std::endl;
        std::cout << "  Actual valid: " << (result.isValid ? "true" : "false") << std::endl;
        
        if (result.isValid) {
            std::string stored_pattern((char*)result.targetBits);
            std::cout << "  Expected normalized: '" << test_case.expected_normalized << "'" << std::endl;
            std::cout << "  Actual normalized: '" << stored_pattern << "'" << std::endl;
            std::cout << "  Pattern length: " << result.bitLength << std::endl;
            
            bool pattern_correct = (stored_pattern == test_case.expected_normalized);
            bool length_correct = (result.bitLength == (int)test_case.expected_normalized.length());
            
            std::cout << "  Pattern match: " << (pattern_correct ? "✅ PASS" : "❌ FAIL") << std::endl;
            std::cout << "  Length match: " << (length_correct ? "✅ PASS" : "❌ FAIL") << std::endl;
        }
        
        bool validity_correct = (result.isValid == test_case.should_be_valid);
        std::cout << "  Validity correct: " << (validity_correct ? "✅ PASS" : "❌ FAIL") << std::endl;
    }
}

void test_string_comparison_logic() {
    std::cout << "\n=== Testing String Comparison Logic ===" << std::endl;
    
    struct TestCase {
        std::string generated_npub;
        std::string target_pattern;
        bool should_match;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        // Bug case that was failing before
        {"npub1k0jru9s0nwspqmd22x7mc33gctsnnfvknndcv0x37mwnj5fc4csq9xwt3y", "npub1k0jra2", false, "Original bug case"},
        
        // Correct matches
        {"npub1k0jra2abcdef123456789", "npub1k0jra2", true, "Exact prefix match"},
        {"npub1k0rtyjta7xexa303k5ul", "npub1k0", true, "Short prefix match"},
        {"npub1k02dneqmmzz2fun4xrty", "k0", true, "Without npub prefix"},
        
        // Long pattern tests
        {"npub1abcdefghijklmnopqrstuvwxyz123456", "npub1abcdefghijklmnop", true, "Long prefix match"},
        {"npub1abcdefghijklmnopqrstuvwxyz123456", "abcdefghijklmnopqrstuvwxyz", true, "Very long pattern"},
        {"npub1abcdefghijklmnopqrstuvwxyz123456", "npub1abcdefghijklmnopqrstuvwxyz1", false, "Pattern too long"},
        
        // Edge cases
        {"npub1abc", "abc", true, "Exact match without prefix"},
        {"npub1abc", "npub1abc", true, "Exact match with prefix"},
        {"npub1abc", "abcd", false, "Pattern longer than generated"},
        {"npub1abc", "ab", true, "Pattern shorter than generated"},
        
        // Case sensitivity
        {"npub1abc", "ABC", false, "Case mismatch"},
        {"npub1ABC", "abc", false, "Case mismatch reverse"},
    };
    
    for (const auto& test_case : test_cases) {
        std::cout << "\nTesting: " << test_case.description << std::endl;
        std::cout << "  Generated: '" << test_case.generated_npub << "'" << std::endl;
        std::cout << "  Pattern: '" << test_case.target_pattern << "'" << std::endl;
        
        // Extract suffix after "npub1"
        const char* generated_suffix = test_case.generated_npub.c_str();
        if (strncmp(generated_suffix, "npub1", 5) == 0) {
            generated_suffix += 5;
        }
        
        // Preprocess the pattern
        NostrOptimized::PatternData pattern_data = NostrOptimized::preprocessPattern(test_case.target_pattern);
        
        if (pattern_data.isValid) {
            const char* target_pattern = (const char*)pattern_data.targetBits;
            int pattern_len = pattern_data.bitLength;
            
            bool actual_match = (strncmp(generated_suffix, target_pattern, pattern_len) == 0);
            
            std::cout << "  Generated suffix: '" << generated_suffix << "'" << std::endl;
            std::cout << "  Target pattern: '" << target_pattern << "'" << std::endl;
            std::cout << "  Pattern length: " << pattern_len << std::endl;
            std::cout << "  Expected match: " << (test_case.should_match ? "YES" : "NO") << std::endl;
            std::cout << "  Actual match: " << (actual_match ? "YES" : "NO") << std::endl;
            std::cout << "  Result: " << ((actual_match == test_case.should_match) ? "✅ PASS" : "❌ FAIL") << std::endl;
        } else {
            std::cout << "  Pattern preprocessing failed!" << std::endl;
            std::cout << "  Result: ❌ FAIL" << std::endl;
        }
    }
}

void test_performance_comparison() {
    std::cout << "\n=== Testing Performance Characteristics ===" << std::endl;
    
    // Test that the optimized version produces same results as manual comparison
    std::vector<std::string> test_npubs = {
        "npub1k0jra2abcdefghijklmnopqrstuvwxyz123456789",
        "npub1k0jru9s0nwspqmd22x7mc33gctsnnfvknndcv0x37mwnj5fc4csq9xwt3y",
        "npub1k0rtyjta7xexa303k5ulexg8303r7qg99dvwhchq8hn002q94cvqj7p948",
        "npub1abcdefghijklmnopqrstuvwxyz1234567890abcdef",
    };
    
    std::vector<std::string> test_patterns = {
        "npub1k0jra2",
        "npub1k0",
        "k0",
        "abcdefghijklmnop",
    };
    
    for (const auto& npub : test_npubs) {
        for (const auto& pattern : test_patterns) {
            // Manual comparison (reference implementation)
            const char* npub_suffix = (strncmp(npub.c_str(), "npub1", 5) == 0) ? (npub.c_str() + 5) : npub.c_str();
            
            std::string normalized_pattern = pattern;
            if (pattern.rfind("npub", 0) == 0) {
                normalized_pattern = pattern.substr(4);
                if (normalized_pattern.length() > 0 && normalized_pattern[0] == '1') {
                    normalized_pattern = normalized_pattern.substr(1);
                }
            }
            
            bool manual_result = (strncmp(npub_suffix, normalized_pattern.c_str(), normalized_pattern.length()) == 0);
            
            // Optimized comparison
            NostrOptimized::PatternData pattern_data = NostrOptimized::preprocessPattern(pattern);
            bool optimized_result = false;
            
            if (pattern_data.isValid) {
                const char* target_pattern = (const char*)pattern_data.targetBits;
                int pattern_len = pattern_data.bitLength;
                optimized_result = (strncmp(npub_suffix, target_pattern, pattern_len) == 0);
            }
            
            std::cout << "  " << npub.substr(0, 20) << "... vs '" << pattern << "'" << std::endl;
            std::cout << "    Manual: " << (manual_result ? "MATCH" : "NO") << std::endl;
            std::cout << "    Optimized: " << (optimized_result ? "MATCH" : "NO") << std::endl;
            std::cout << "    Consistent: " << ((manual_result == optimized_result) ? "✅ PASS" : "❌ FAIL") << std::endl;
        }
    }
}

int main() {
    std::cout << "Fixed NostrOptimized Implementation Test Suite" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    test_preprocessPattern_fixed();
    test_string_comparison_logic();
    test_performance_comparison();
    
    std::cout << "\n=== Test Suite Completed ===" << std::endl;
    return 0;
}
