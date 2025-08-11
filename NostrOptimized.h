// Optimized Nostr npub pattern matching without full Bech32 encoding
#ifndef NOSTR_OPTIMIZED_H
#define NOSTR_OPTIMIZED_H

#include "Point.h"
#include "Bech32.h"
#include <string>
#include <vector>
#include <cstring>

// Fast npub pattern matching by directly comparing X coordinate bits
class NostrOptimized {
public:
    // Pre-process pattern for fast matching
    struct PatternData {
        uint8_t targetBits[32];  // Expected bit pattern from Bech32 decode
        int bitLength;           // Length in bits
        bool isValid;
    };
    
    // ULTRA-OPTIMIZED: All functions inlined for zero call overhead
    static inline PatternData preprocessPattern(const std::string& pattern) {
        PatternData result;
        result.isValid = false;
        result.bitLength = 0;
        
        // MICRO-OPTIMIZED: 最小限の文字列処理
        const char* start_ptr = pattern.c_str();
        int offset = 0;
        if (pattern.size() > 4 && strncmp(start_ptr, "npub", 4) == 0) {
            offset = 4;
            if (pattern.size() > 5 && start_ptr[4] == '1') offset = 5;
        }
        
        int len = pattern.size() - offset;
        if (len > 0 && len < 32) {
            // 直接コピー、lenは既知
            for (int i = 0; i < len; i++) {
                result.targetBits[i] = start_ptr[offset + i];
            }
            result.targetBits[len] = '\0';
            result.bitLength = len;
            result.isValid = true;
        }
        
        return result;
    }
    
    static inline void generateNpubDirect(const Point& p, char* output_buffer) {
        static thread_local uint8_t xbytes[32];
        const_cast<Int&>(p.x).Get32Bytes(xbytes);
        if (!bech32_encode_data(output_buffer, "npub", xbytes, 32)) {
            strcpy(output_buffer, "ERROR: Failed to encode npub");
        }
    }
    
    static inline void batchMatch(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                                 const PatternData* patterns, int patternCount, bool results[4]) {
        static thread_local char npub1[128], npub2[128], npub3[128], npub4[128];
        
        generateNpubDirect(p1, npub1);
        generateNpubDirect(p2, npub2);
        generateNpubDirect(p3, npub3);
        generateNpubDirect(p4, npub4);
        
        const char* suffix1 = (strncmp(npub1, "npub1", 5) == 0) ? (npub1 + 5) : npub1;
        const char* suffix2 = (strncmp(npub2, "npub1", 5) == 0) ? (npub2 + 5) : npub2;
        const char* suffix3 = (strncmp(npub3, "npub1", 5) == 0) ? (npub3 + 5) : npub3;
        const char* suffix4 = (strncmp(npub4, "npub1", 5) == 0) ? (npub4 + 5) : npub4;
        
        results[0] = results[1] = results[2] = results[3] = false;
        
        for (int p = 0; p < patternCount; p++) {
            const PatternData& pattern = patterns[p];
            if (__builtin_expect(!pattern.isValid, 0)) continue;
            
            const char* target_pattern = (const char*)pattern.targetBits;
            int pattern_len = pattern.bitLength;
            
            if (__builtin_expect(!results[0], 1)) {
                bool match = false;
                switch (pattern_len) {
                    case 1: match = (suffix1[0] == target_pattern[0]); break;
                    case 2: match = (suffix1[0] == target_pattern[0]) && (suffix1[1] == target_pattern[1]); break;
                    case 3: match = (suffix1[0] == target_pattern[0]) && (suffix1[1] == target_pattern[1]) && (suffix1[2] == target_pattern[2]); break;
                    case 4: match = (suffix1[0] == target_pattern[0]) && (suffix1[1] == target_pattern[1]) && (suffix1[2] == target_pattern[2]) && (suffix1[3] == target_pattern[3]); break;
                    case 5: match = (suffix1[0] == target_pattern[0]) && (suffix1[1] == target_pattern[1]) && (suffix1[2] == target_pattern[2]) && (suffix1[3] == target_pattern[3]) && (suffix1[4] == target_pattern[4]); break;
                    case 6: match = (suffix1[0] == target_pattern[0]) && (suffix1[1] == target_pattern[1]) && (suffix1[2] == target_pattern[2]) && (suffix1[3] == target_pattern[3]) && (suffix1[4] == target_pattern[4]) && (suffix1[5] == target_pattern[5]); break;
                    default: match = (strncmp(suffix1, target_pattern, pattern_len) == 0); break;
                }
                if (match) results[0] = true;
            }
            
            if (__builtin_expect(!results[1], 1)) {
                bool match = false;
                switch (pattern_len) {
                    case 1: match = (suffix2[0] == target_pattern[0]); break;
                    case 2: match = (suffix2[0] == target_pattern[0]) && (suffix2[1] == target_pattern[1]); break;
                    case 3: match = (suffix2[0] == target_pattern[0]) && (suffix2[1] == target_pattern[1]) && (suffix2[2] == target_pattern[2]); break;
                    case 4: match = (suffix2[0] == target_pattern[0]) && (suffix2[1] == target_pattern[1]) && (suffix2[2] == target_pattern[2]) && (suffix2[3] == target_pattern[3]); break;
                    case 5: match = (suffix2[0] == target_pattern[0]) && (suffix2[1] == target_pattern[1]) && (suffix2[2] == target_pattern[2]) && (suffix2[3] == target_pattern[3]) && (suffix2[4] == target_pattern[4]); break;
                    case 6: match = (suffix2[0] == target_pattern[0]) && (suffix2[1] == target_pattern[1]) && (suffix2[2] == target_pattern[2]) && (suffix2[3] == target_pattern[3]) && (suffix2[4] == target_pattern[4]) && (suffix2[5] == target_pattern[5]); break;
                    default: match = (strncmp(suffix2, target_pattern, pattern_len) == 0); break;
                }
                if (match) results[1] = true;
            }
            
            if (__builtin_expect(!results[2], 1)) {
                bool match = false;
                switch (pattern_len) {
                    case 1: match = (suffix3[0] == target_pattern[0]); break;
                    case 2: match = (suffix3[0] == target_pattern[0]) && (suffix3[1] == target_pattern[1]); break;
                    case 3: match = (suffix3[0] == target_pattern[0]) && (suffix3[1] == target_pattern[1]) && (suffix3[2] == target_pattern[2]); break;
                    case 4: match = (suffix3[0] == target_pattern[0]) && (suffix3[1] == target_pattern[1]) && (suffix3[2] == target_pattern[2]) && (suffix3[3] == target_pattern[3]); break;
                    case 5: match = (suffix3[0] == target_pattern[0]) && (suffix3[1] == target_pattern[1]) && (suffix3[2] == target_pattern[2]) && (suffix3[3] == target_pattern[3]) && (suffix3[4] == target_pattern[4]); break;
                    case 6: match = (suffix3[0] == target_pattern[0]) && (suffix3[1] == target_pattern[1]) && (suffix3[2] == target_pattern[2]) && (suffix3[3] == target_pattern[3]) && (suffix3[4] == target_pattern[4]) && (suffix3[5] == target_pattern[5]); break;
                    default: match = (strncmp(suffix3, target_pattern, pattern_len) == 0); break;
                }
                if (match) results[2] = true;
            }
            
            if (__builtin_expect(!results[3], 1)) {
                bool match = false;
                switch (pattern_len) {
                    case 1: match = (suffix4[0] == target_pattern[0]); break;
                    case 2: match = (suffix4[0] == target_pattern[0]) && (suffix4[1] == target_pattern[1]); break;
                    case 3: match = (suffix4[0] == target_pattern[0]) && (suffix4[1] == target_pattern[1]) && (suffix4[2] == target_pattern[2]); break;
                    case 4: match = (suffix4[0] == target_pattern[0]) && (suffix4[1] == target_pattern[1]) && (suffix4[2] == target_pattern[2]) && (suffix4[3] == target_pattern[3]); break;
                    case 5: match = (suffix4[0] == target_pattern[0]) && (suffix4[1] == target_pattern[1]) && (suffix4[2] == target_pattern[2]) && (suffix4[3] == target_pattern[3]) && (suffix4[4] == target_pattern[4]); break;
                    case 6: match = (suffix4[0] == target_pattern[0]) && (suffix4[1] == target_pattern[1]) && (suffix4[2] == target_pattern[2]) && (suffix4[3] == target_pattern[3]) && (suffix4[4] == target_pattern[4]) && (suffix4[5] == target_pattern[5]); break;
                    default: match = (strncmp(suffix4, target_pattern, pattern_len) == 0); break;
                }
                if (match) results[3] = true;
            }
        }
    }
    
    static inline bool fastMatch(const Point& p, const std::vector<PatternData>& patterns) {
        return false; // Not used in optimized path
    }
    
private:
    // Convert X coordinate to Bech32 data bits efficiently 
    static void xCoordToBech32Bits(const Point& p, uint8_t bits[32]);
    
    // Fast bit-level comparison
    static bool compareBits(const uint8_t* data, const uint8_t* pattern, int bitLen);
};

#endif // NOSTR_OPTIMIZED_H
