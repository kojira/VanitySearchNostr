// Simple test for pattern matching logic
#include <iostream>
#include <string>
#include <cstring>

// Recreate the problematic logic from the bug
bool test_pattern_match(const std::string& generated_npub, const std::string& target_pattern) {
    std::cout << "Testing: '" << generated_npub << "' against pattern '" << target_pattern << "'" << std::endl;
    
    // Extract suffix after "npub1" from generated npub
    const char *full = generated_npub.c_str();
    const char *npubSuffix = (generated_npub.rfind("npub1", 0) == 0) ? (full + 5) : full;
    
    // Normalize target pattern: allow with or without leading "npub"/"npub1"
    const char *p = target_pattern.c_str();
    if (target_pattern.rfind("npub", 0) == 0) {
        p += 4;
        if (*p == '1') p++;
    }
    
    std::cout << "  Generated suffix: '" << npubSuffix << "'" << std::endl;
    std::cout << "  Target pattern: '" << p << "'" << std::endl;
    
    // Compare user suffix as prefix of generated suffix
    int patternLen = strlen(p);
    bool matches = (patternLen <= (int)strlen(npubSuffix) && strncmp(npubSuffix, p, patternLen) == 0);
    
    std::cout << "  Pattern length: " << patternLen << std::endl;
    std::cout << "  Matches: " << (matches ? "YES" : "NO") << std::endl;
    
    return matches;
}

int main() {
    std::cout << "=== Simple Pattern Matching Test ===" << std::endl;
    
    // Test the reported bug case
    std::cout << "\n1. Testing the bug case:" << std::endl;
    bool bug_result = test_pattern_match(
        "npub1k0jru9s0nwspqmd22x7mc33gctsnnfvknndcv0x37mwnj5fc4csq9xwt3y",
        "npub1k0jra2"
    );
    std::cout << "BUG: Should be NO, got: " << (bug_result ? "YES (BUG!)" : "NO (correct)") << std::endl;
    
    // Test correct matches
    std::cout << "\n2. Testing correct matches:" << std::endl;
    test_pattern_match("npub1k0jra2abcdef...", "npub1k0jra2");
    test_pattern_match("npub1k0rtyjta7xexa303k5ulexg8303r7qg99dvwhchq8hn002q94cvqj7p948", "npub1k0");
    test_pattern_match("npub1k02dneqmmzz2fun4xrty5epklrnyf9ef0unt2hf9urhw9stmzldsmevuyt", "k0");
    
    // Test edge cases
    std::cout << "\n3. Testing edge cases:" << std::endl;
    test_pattern_match("npub1abc", "abc");
    test_pattern_match("npub1abc", "npub1abc");
    test_pattern_match("npub1abc", "npub1abcd"); // Should not match
    
    std::cout << "\n=== Test completed ===" << std::endl;
    return 0;
}
