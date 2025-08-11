// Test case for NostrOptimized pattern matching bug
#include <iostream>
#include <string>
#include <vector>
#include "NostrOptimized.h"
#include "Bech32.h"

void test_preprocessPattern() {
    std::cout << "=== Testing preprocessPattern function ===" << std::endl;
    
    // Test cases
    std::vector<std::string> test_patterns = {
        "npub1k0jra2",
        "npub1k0",
        "k0jra2",
        "k0",
        "npub1abc",
        "abc"
    };
    
    for (const auto& pattern : test_patterns) {
        std::cout << "\nTesting pattern: '" << pattern << "'" << std::endl;
        
        NostrOptimized::PatternData result = NostrOptimized::preprocessPattern(pattern);
        
        std::cout << "  isValid: " << (result.isValid ? "true" : "false") << std::endl;
        std::cout << "  bitLength: " << result.bitLength << std::endl;
        std::cout << "  targetBits (hex): ";
        
        for (int i = 0; i < result.bitLength / 8 && i < 10; i++) {
            printf("%02x ", result.targetBits[i]);
        }
        std::cout << std::endl;
    }
}

void test_bech32_decode_encode() {
    std::cout << "\n=== Testing Bech32 encode/decode roundtrip ===" << std::endl;
    
    // Create a test X coordinate (32 bytes)
    uint8_t test_x_coord[32] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11,
        0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
    };
    
    // Encode to npub
    char npub_result[128];
    bool encode_success = bech32_encode_data(npub_result, "npub", test_x_coord, 32);
    
    std::cout << "Encode success: " << (encode_success ? "true" : "false") << std::endl;
    if (encode_success) {
        std::cout << "Generated npub: " << npub_result << std::endl;
        
        // Test if various prefixes would match
        std::vector<std::string> test_prefixes = {
            std::string(npub_result).substr(0, 10), // First 10 chars
            std::string(npub_result).substr(0, 15), // First 15 chars
            std::string(npub_result).substr(5, 5),  // 5 chars after "npub1"
            std::string(npub_result).substr(5, 3),  // 3 chars after "npub1"
        };
        
        for (const auto& prefix : test_prefixes) {
            std::cout << "  Testing prefix: '" << prefix << "'" << std::endl;
            
            // Test our preprocessPattern function
            NostrOptimized::PatternData pattern_data = NostrOptimized::preprocessPattern(prefix);
            
            // Test our compareBits function
            // (This would need actual Point object to test fully)
            std::cout << "    Pattern valid: " << (pattern_data.isValid ? "true" : "false") << std::endl;
        }
    }
}

void test_actual_mismatch() {
    std::cout << "\n=== Testing the actual bug case ===" << std::endl;
    
    std::string expected_pattern = "npub1k0jra2";
    std::string found_result = "npub1k0jru9s0nwspqmd22x7mc33gctsnnfvknndcv0x37mwnj5fc4csq9xwt3y";
    
    std::cout << "Expected pattern: " << expected_pattern << std::endl;
    std::cout << "Found result: " << found_result << std::endl;
    
    // Extract suffix after "npub1"
    const char* expected_suffix = expected_pattern.c_str() + 5; // Skip "npub1"
    const char* found_suffix = found_result.c_str() + 5;       // Skip "npub1"
    
    std::cout << "Expected suffix: " << expected_suffix << std::endl;
    std::cout << "Found suffix: " << found_suffix << std::endl;
    
    // Test string comparison
    int expected_len = strlen(expected_suffix);
    bool should_match = (strncmp(found_suffix, expected_suffix, expected_len) == 0);
    
    std::cout << "Should match (strncmp): " << (should_match ? "YES" : "NO") << std::endl;
    std::cout << "This reveals the bug: " << (should_match ? "LOGIC ERROR" : "CORRECT REJECTION") << std::endl;
    
    // Test our preprocessPattern with both
    NostrOptimized::PatternData expected_data = NostrOptimized::preprocessPattern(expected_pattern);
    std::cout << "\nExpected pattern preprocessed:" << std::endl;
    std::cout << "  Valid: " << expected_data.isValid << std::endl;
    std::cout << "  BitLength: " << expected_data.bitLength << std::endl;
}

int main() {
    std::cout << "NostrOptimized Pattern Matching Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    test_preprocessPattern();
    test_bech32_decode_encode();
    test_actual_mismatch();
    
    std::cout << "\n=== Test completed ===" << std::endl;
    return 0;
}
